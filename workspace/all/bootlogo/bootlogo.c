#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <msettings.h>

#include "defines.h"
#include "api.h"
#include "utils.h"

static volatile sig_atomic_t quit = 0;

static void sigHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        quit = true;
        break;
    default:
        break;
    }
}

static SDL_Surface *screen;

static int run_program(const char *path, char *const argv[]) {
    pid_t pid = fork();
    int status;

    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(path, argv);
        _exit(127);
    }
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) return 0;
    errno = EIO;
    return -1;
}

static int copy_bootlogo(const char *source) {
    static const char destination[] = "/mnt/boot/bootlogo.bmp";
    char buffer[4096];
    int input = open(source, O_RDONLY | O_NOFOLLOW);
    int output;
    struct stat st;

    if (input < 0 || fstat(input, &st) != 0 || !S_ISREG(st.st_mode)) goto fail_input;
    output = open(destination, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 420);
    if (output < 0) goto fail_input;
    for (;;) {
        ssize_t read_size = read(input, buffer, sizeof(buffer));
        if (read_size < 0) {
            if (errno == EINTR) continue;
            close(output);
            unlink(destination);
            goto fail_input;
        }
        if (read_size == 0) break;
        for (ssize_t written = 0; written < read_size;) {
            ssize_t result = write(output, buffer + written, (size_t)(read_size - written));
            if (result < 0 && errno == EINTR) continue;
            if (result <= 0) {
                close(output);
                unlink(destination);
                goto fail_input;
            }
            written += result;
        }
    }
    if (fsync(output) != 0) {
        close(output);
        unlink(destination);
        goto fail_input;
    }
    int close_output = close(output);
    int close_input = close(input);
    if (close_output != 0 || close_input != 0) {
        unlink(destination);
        return -1;
    }
    return 0;

fail_input:
    if (input >= 0) close(input);
    return -1;
}

static int apply_logo(const char *logo_path) {
    static const char boot_path[] = "/mnt/boot/";
    char *mount_argv[] = { "mount", "-t", "vfat", "/dev/mmcblk0p1", (char *)boot_path, NULL };
    char *umount_argv[] = { "umount", (char *)boot_path, NULL };
    char *reboot_argv[] = { "reboot", NULL };
    struct stat st;

    if ((mkdir(boot_path, S_IRWXU) != 0 && errno != EEXIST) ||
        stat(boot_path, &st) != 0 || !S_ISDIR(st.st_mode) ||
        run_program("mount", mount_argv) != 0) return -1;
    if (copy_bootlogo(logo_path) != 0) {
        run_program("umount", umount_argv);
        return -1;
    }
    sync();
    if (run_program("umount", umount_argv) != 0) return -1;
    return run_program("reboot", reboot_argv);
}

SDL_Surface** images;
char **image_paths;
static int selected = 0;
static int count = 0;

int loadImages()
{
    char* device = getenv("DEVICE");
    // This needs to get a bit more flexible down the line, but for now we either expect the files
    // in the pak root directory or in the "brick" subfolder.
    char basepath[MAX_PATH];
    if(exactMatch("brick", device) || exactMatch("brickpro", device)) {
        snprintf(basepath, sizeof(basepath), "%s/Bootlogo.pak/brick/", TOOLS_PATH);
    }
    else {
        snprintf(basepath, sizeof(basepath), "%s/Bootlogo.pak/smartpro/", TOOLS_PATH);
    }

    // grab all bmp files in the directory and load them with IMG_Load, 
    // keep them in an array of SDL_Surface pointers
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(basepath)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".bmp") != NULL) {
                char path[MAX_PATH];
                snprintf(path, sizeof(path), "%s%s", basepath, ent->d_name);
                SDL_Surface *bmp = IMG_Load(path);
                if (bmp) {
                    SDL_Surface **new_images = realloc(images, sizeof(*images) * (count + 1));
                    char **new_paths;
                    char *new_path;
                    if (!new_images) {
                        SDL_FreeSurface(bmp);
                        continue;
                    }
                    images = new_images;
                    new_paths = realloc(image_paths, sizeof(*image_paths) * (count + 1));
                    if (!new_paths) {
                        SDL_FreeSurface(bmp);
                        continue;
                    }
                    image_paths = new_paths;
                    new_path = strdup(path);
                    if (!new_path) {
                        SDL_FreeSurface(bmp);
                        continue;
                    }
                    images[count] = bmp;
                    image_paths[count++] = new_path;
                }
            }
        }
        closedir(dir);
    } else {
        // could not open directory
        LOG_error("could not open directory");
        if (CFG_getHaptics()) {
            VIB_triplePulse(5, 150, 200);
        }
        return 0;
    }
    return count;
}

void unloadImages()
{
    for (int i = 0; i < count; i++)
    {
        SDL_FreeSurface(images[i]);
    }
    free(images);
    for (int i = 0; i < count; i++) free(image_paths[i]);
    free(image_paths);
    images = NULL;
    image_paths = NULL;
    count = 0;
}

int main(int argc, char *argv[])
{
    InitSettings();

    PWR_setCPUSpeed(CPU_SPEED_AUTO);

    screen = GFX_init(MODE_MENU);
    PAD_init();
    PWR_init();

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    loadImages();

    int dirty = 1;
    int show_setting = 0;
    int was_online = PWR_isOnline();
    int had_bt = PLAT_btIsConnected();
    while (!quit)
    {
        GFX_startFrame();
        PAD_poll();

        // This might be too harsh, but ignore all combos with MENU (most likely a shortcut for someone else)
        if (PAD_justPressed(BTN_MENU))
        {
            // ?
        }
        else
        {
            if (count > 0 && PAD_justRepeated(BTN_LEFT))
            {
                selected -= 1;
                if (selected < 0)
                    selected = count - 1;
                dirty = 1;
            }
            else if (count > 0 && PAD_justRepeated(BTN_RIGHT))
            {
                selected += 1;
                if (selected >= count)
                    selected = 0;
                dirty = 1;
            }
            else if (count > 0 && PAD_justPressed(BTN_A))
            {
                if (apply_logo(image_paths[selected]) != 0)
                    LOG_error("failed to apply boot logo: %s\n", strerror(errno));
            }
            else if (PAD_justPressed(BTN_B))
            {
                quit = 1;
            }
        }

        PWR_update(&dirty, NULL, NULL, NULL);

        int is_online = PWR_isOnline();
        if (was_online != is_online)
            dirty = 1;
        was_online = is_online;

        int has_bt = PLAT_btIsConnected();
        if (had_bt != has_bt)
            dirty = 1;
        had_bt = has_bt;

        if (dirty)
        {
            GFX_clear(screen);

            if(count > 0) {
                // render the selected image, centered on screen
                SDL_Surface *image = images[selected];
                SDL_Rect image_rect = {
                    screen->w /2 - image->w /2,
                    screen->h /2 - image->h / 2,
                    image->w,
                    image->h};
                SDL_BlitSurface(image, NULL, screen, &image_rect);
            }

            GFX_blitButtonGroup((char *[]){"L/R", "SCROLL", NULL}, 0, screen, 0);
            GFX_blitButtonGroup((char *[]){"A", "SET", "B", "BACK", NULL}, 1, screen, 1);

            GFX_flip(screen);
            dirty = 0;
        }
        else
            GFX_sync();
    }

    unloadImages();

    QuitSettings();
    PWR_quit();
    PAD_quit();
    GFX_quit();

    return EXIT_SUCCESS;
}