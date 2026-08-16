#define _GNU_SOURCE
// tg5050
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <sched.h>

#include <msettings.h>

#include "defines.h"
#include "platform.h"

// Smart Pro S device descriptor. Single model -- no runtime variant selector
// (unlike tg5040, which picks Brick vs Smart Pro from DEVICE=brick).
#define TG5050_CPU_SPEED_PATH  "/sys/devices/system/cpu/cpu4/cpufreq/scaling_cur_freq"
#define TG5050_GPU_TEMP_PATH   "/sys/devices/virtual/thermal/thermal_zone5/temp"
#define TG5050_GPU_FREQ_PATH   "/sys/devices/platform/soc@3000000/1800000.gpu/devfreq/1800000.gpu/cur_freq"
#define TG5050_GPU_USAGE_PATH  "/sys/devices/platform/soc@3000000/1800000.gpu/sunxi_gpu/sunxi_gpu_freq"
#define TG5050_RUMBLE_GPIO     "/sys/class/gpio/gpio236/value"
static const DeviceDescriptor TG5050 = {
	.scale = 2, .width = 1280, .height = 720,
	.main_row_count = 10, .quick_switcher_count = 4, .padding = 10,
	.joy_l3 = 9, .joy_r3 = 10,
	.joy_l4 = JOY_NA, .joy_r4 = JOY_NA, .joy_menu_alt = JOY_NA,
	.joy_plus = 128, .joy_minus = 129,
	.cpu_speed_path = TG5050_CPU_SPEED_PATH,
	.gpu_temp_path = TG5050_GPU_TEMP_PATH,
	.rumble_gpio_path = TG5050_RUMBLE_GPIO,
	.gpu_speed_fixed = 0,
	.gpu_freq_path = TG5050_GPU_FREQ_PATH,
	.gpu_usage_path = TG5050_GPU_USAGE_PATH,
};
const DeviceDescriptor* deviceModel = &TG5050;
#include "api.h"
#include "utils.h"

#include "scaler.h"
#include <time.h>
#include <pthread.h>

#include <dirent.h>

static SDL_Joystick **joysticks = NULL;
static int num_joysticks = 0;
void PLAT_initInput(void) {
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		LOG_error("Failed initializing joysticks: %s\n", SDL_GetError());
	num_joysticks = SDL_NumJoysticks();
    if (num_joysticks > 0) {
        joysticks = (SDL_Joystick **)malloc(sizeof(SDL_Joystick *) * num_joysticks);
        for (int i = 0; i < num_joysticks; i++) {
			joysticks[i] = SDL_JoystickOpen(i);
			LOG_info("Opening joystick %d: %s\n", i, SDL_JoystickName(joysticks[i]));
        }
    }
}



///////////////////////////////






void PLAT_getGPUUsage() {
	// cat /sys/devices/platform/soc@3000000/1800000.gpu/sunxi_gpu/sunxi_gpu_freq | grep -o '[0-9]*%' | tr -d '%'
	
    char buffer[256];
    buffer[0] = '\0';
    getFile("/sys/devices/platform/soc@3000000/1800000.gpu/sunxi_gpu/sunxi_gpu_freq", buffer, sizeof(buffer));
    
    // Parse the percentage value from the buffer
    // Look for a number followed by '%'
    char *ptr = buffer;
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            // Found start of a number
            char *start = ptr;
            while (*ptr >= '0' && *ptr <= '9') {
                ptr++;
            }
            // Check if followed by '%'
            if (*ptr == '%') {
                *ptr = '\0'; // Temporarily null-terminate
                perf.gpu_usage = (double)atoi(start);
                return;
            }
        } else {
            ptr++;
        }
    }
    
    // If no percentage found, set to 0.0
    perf.gpu_usage = 0.0;
}

static struct WIFI_connection connection = {
	.valid = false,
	.ssid = {0},
	.ip = {0},
	.freq = -1,
	.rssi = -1,
	.link_speed = -1,
	.noise = -1,
};

static inline void connection_reset(struct WIFI_connection *connection_info)
{
	connection_info->valid = false;
	connection_info->freq = -1;
	connection_info->link_speed = -1;
	connection_info->noise = -1;
	connection_info->rssi = -1;
	*connection_info->ip = '\0';
	*connection_info->ssid = '\0';
}

static bool bluetoothConnected = false;


int PLAT_isUSBConnected(void)
{
	// Not implemented for this platform yet.
	return 0;
}

void PLAT_enableBacklight(int enable) {
	if (enable) {
		SetBrightness(GetBrightness());
	}
	else {
		SetRawBrightness(0);
	}
}



///////////////////////////////

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9; // Convert to seconds
}
double get_process_cpu_time_sec() {
	// this gives cpu time in nanoseconds needed to accurately calculate cpu usage in very short time frames. 
	// unfortunately about 20ms between meassures seems the lowest i can go to get accurate results
	// maybe in the future i will find and even more granual way to get cpu time, but might just be a limit of C or Linux alltogether
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9; // Convert to seconds
}

static pthread_mutex_t currentcpuinfo;
// a roling average for the display values of about 2 frames, otherwise they are unreadable jumping too fast up and down and stuff to read
#define ROLLING_WINDOW 120  

void PLAT_pinToCores(int core_type)
{
	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    // Add all potential cores to the mask, even if some are sleeping right now
	int from = core_type == CPU_CORE_EFFICIENCY ? 0 : 4;
	int to = core_type == CPU_CORE_EFFICIENCY ? 3 : 7;
    for (int i = from; i <= to; i++) {
        CPU_SET(i, &cpuset);
    }

	//// This will SUCCEED as long as at least one of the cores is online
    pthread_t current_thread = pthread_self();
    int s = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    
    if (s != 0)
        LOG_error("Failed to pin: Are all cores sleeping?\n");
}



#define MAX_STRENGTH 0xFFFF
#define RUMBLE_PATH "/sys/class/gpio/gpio236/value"
#define RUMBLE_LEVEL_PATH "/sys/class/motor/level"

void PLAT_setRumble(int strength) {
	if(strength > 0 && strength < MAX_STRENGTH) {
		putInt(RUMBLE_LEVEL_PATH, strength);
	}
	else {
		putInt(RUMBLE_LEVEL_PATH, 0);
	}

	putInt(RUMBLE_PATH, (strength) ? 1 : 0);
}



char* PLAT_getModel(void) {
	char* model = getenv("TRIMUI_MODEL");
	if (model) return model;
	return "Trimui Smart Pro S";
}




void PLAT_initDefaultLeds() {
	lightsDefault[0] = (LightSettings) {
		"Joystick L",
		"l",
		4,
		1000,
		100,
		0xFFFFFF,
		0xFFFFFF,
		0,
		{},
		1,
		100,
		0
	};
	lightsDefault[1] = (LightSettings) {
		"Joystick R",
		"r",
		4,
		1000,
		100,
		0xFFFFFF,
		0xFFFFFF,
		0,
		{},
		1,
		100,
		0
	};
	lightsDefault[2] = (LightSettings) {
		"Logo",
		"m",
		4,
		1000,
		100,
		0xFFFFFF,
		0xFFFFFF,
		0,
		{},
		1,
		100,
		0
	};
}
void PLAT_initLeds(LightSettings *lights) 
{
	PLAT_initDefaultLeds();
	FILE *file = PLAT_OpenSettings("ledsettings.txt");

    if (file == NULL)
    {
        LOG_warn("Unable to open led settings file\n");
    }
	else {
		char line[256];
		int current_light = -1;
		while (fgets(line, sizeof(line), file))
		{
			if (line[0] == '[')
			{
				// Section header
				char light_name[255];
				if (sscanf(line, "[%49[^]]]", light_name) == 1)
				{
					current_light++;
					if (current_light < MAX_LIGHTS)
					{
						strncpy(lights[current_light].name, light_name, 255 - 1);
						lights[current_light].name[255 - 1] = '\0'; // Ensure null-termination
						lights[current_light].cycles = -1; // cycles (times animation loops) should basically always be -1 for unlimited unless specifically set
					}
					else
					{
						LOG_info("Maximum number of lights (%d) exceeded. Ignoring further sections.\n", MAX_LIGHTS);
						current_light = -1; // Reset if max_lights exceeded
					}
				}
			}
			else if (current_light >= 0 && current_light < MAX_LIGHTS)
			{
				int temp_value;
				uint32_t temp_color;
				char filename[255];

				if (sscanf(line, "filename=%s", &filename) == 1)
				{
					strncpy(lights[current_light].filename, filename, 255 - 1);
					continue;
				}
				if (sscanf(line, "effect=%d", &temp_value) == 1)
				{
					lights[current_light].effect = temp_value;
					continue;
				}
				if (sscanf(line, "color1=%x", &temp_color) == 1)
				{
					lights[current_light].color1 = temp_color;
					continue;
				}
				if (sscanf(line, "color2=%x", &temp_color) == 1)
				{
					lights[current_light].color2 = temp_color;
					continue;
				}
				if (sscanf(line, "speed=%d", &temp_value) == 1)
				{
					lights[current_light].speed = temp_value;
					continue;
				}
				if (sscanf(line, "brightness=%d", &temp_value) == 1)
				{
					lights[current_light].brightness = temp_value;
					continue;
				}
				if (sscanf(line, "trigger=%d", &temp_value) == 1)
				{
					lights[current_light].trigger = temp_value;
					continue;
				}
				if (sscanf(line, "inbrightness=%d", &temp_value) == 1)
				{
					lights[current_light].inbrightness = temp_value;
					continue;
				}
			}
		}
		fclose(file);
	}
}

#define LED_PATH1 "/sys/class/led_anim/max_scale"
#define LED_PATH2 "/sys/class/led_anim/max_scale_lr"
#define LED_PATH3 "/sys/class/led_anim/max_scale_f1f2" 

void PLAT_setLedInbrightness(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
	snprintf(filepath, sizeof(filepath), LED_PATH1);
	if (strcmp(led->filename, "f2") != 0) {
		// do nothhing for f2
		file = fopen(filepath, "w");
		if (file != NULL)
		{
			fprintf(file, "%i\n", led->inbrightness);
			fclose(file);
		}
	}
}
void PLAT_setLedBrightness(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
	snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale");
	if (strcmp(led->filename, "f2") != 0) {
		// do nothhing for f2
		file = fopen(filepath, "w");
		if (file != NULL)
		{
			fprintf(file, "%i\n", led->brightness);
			fclose(file);
		}
	}
}

//////////////////////////////////////////////


#define INPUTD_PATH "/tmp/trimui_inputd"

typedef struct TurboBtnPath {
	int brn_id;
	char *path;
} TurboBtnPath;

static TurboBtnPath turbo_mapping[] = {
    {BTN_ID_A, INPUTD_PATH "/turbo_a"},
    {BTN_ID_B, INPUTD_PATH "/turbo_b"},
    {BTN_ID_X, INPUTD_PATH "/turbo_x"},
    {BTN_ID_Y, INPUTD_PATH "/turbo_y"},
    {BTN_ID_L1, INPUTD_PATH "/turbo_l"},
    {BTN_ID_L2, INPUTD_PATH "/turbo_l2"},
    {BTN_ID_R1, INPUTD_PATH "/turbo_r"},
    {BTN_ID_R2, INPUTD_PATH "/turbo_r2"},
	{0, NULL}
};

int toggle_file(const char *path) {
    if (access(path, F_OK) == 0) {
        unlink(path);
        return 0;
    } else {
        int fd = open(path, O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            close(fd);
            return 1;
        }
        return -1; // error
    }
}



//////////////////////////////////////////////


#define MAX_LINE_LENGTH 200
#define ZONE_PATH "/usr/share/zoneinfo"
#define ZONE_TAB_PATH ZONE_PATH "/zone.tab"

static char cached_timezones[MAX_TIMEZONES][MAX_TZ_LENGTH];
static int cached_tz_count = -1;

int compare_timezones(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}



char *PLAT_getCurrentTimezone() {
	// easy enough, get current index from config and return the string
	int tz_index = CFG_getCurrentTimezone();
	if (tz_index < 0 || tz_index >= cached_tz_count) {
		LOG_warn("Error: Current timezone index %d out of bounds.\n", tz_index);
		return NULL;
	}

	char *output = (char *)malloc(256);
	if (!output)
		return NULL;

	strncpy(output, cached_timezones[tz_index], 256 - 1);
	output[256 - 1] = '\0'; // Ensure null-termination

	return output;
}

void PLAT_setCurrentTimezone(const char* tz) {
	if (cached_tz_count == -1) {
		LOG_warn("Error: Timezones not initialized. Call PLAT_initTimezones first.\n");
        return;
    }

	if(!tz || strlen(tz) == 0) {
		LOG_warn("Error: Invalid timezone string.\n");
		return;
	}

	// get index of timezone
	int tz_index = -1;
	for (int i = 0; i < cached_tz_count; i++) {
		if (strcmp(cached_timezones[i], tz) == 0) {
			tz_index = i;
			break;
		}
	}

	if (tz_index == -1) {
		LOG_warn("Error: Timezone %s not found in cached list.\n", tz);
		return;
	}

	// set in config
	CFG_setCurrentTimezone(tz_index);

	// This fixes the timezone until the next reboot
	char *tz_path = (char *)malloc(256);
	if (!tz_path) {
		return;
	}
	snprintf(tz_path, 256, ZONE_PATH "/%s", tz);
	// replace existing symlink
	if (unlink("/tmp/localtime") == -1) {
		LOG_debug("Failed to remove existing symlink: %s\n", strerror(errno));
	}
	if (symlink(tz_path, "/tmp/localtime") == -1) {
		LOG_error("Failed to set timezone: %s\n", strerror(errno));
	}
	free(tz_path);

	// apply timezone to RTC and kernel
	system("hwclock -u -w && hwclock --systz -u");
}

bool PLAT_getNetworkTimeSync(void) {
	return CFG_getNTP();
}

void PLAT_setNetworkTimeSync(bool on) {
	CFG_setNTP(on);
	if (on) {
		system("/etc/init.d/S49ntp restart &");
	} else {
		system("/etc/init.d/S49ntp stop &");
	}
}

/////////////////////////

// We use the generic video implementation here
#include "generic_platform.c"
#include "generic_video.c"

/////////////////////////

// We use the generic wifi implementation here
#define WIFI_SOCK_DIR "/etc/wifi/sockets"
#include "generic_wifi.c"

/////////////////////////

// We use the generic bluetooth implementation here
#include "generic_bt.c"
