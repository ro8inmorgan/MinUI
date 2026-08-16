// tg5040
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

#include <msettings.h>

#include "defines.h"
#include "platform.h"
#include "api.h"
#include "utils.h"

#include "scaler.h"
#include <time.h>
#include <pthread.h>

#include <dirent.h>

int is_brick = 0;
int is_brickpro = 0;

// Shared tg5040 hardware node paths -- the same on Brick and Smart Pro. These
// are exactly the seams a different platform redefines (tg5050 would use
// cpu4/thermal_zone5/gpio236 here), so they belong to the descriptor.
#define TG5040_CPU_SPEED_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
#define TG5040_GPU_TEMP_PATH  "/sys/devices/virtual/thermal/thermal_zone2/temp"
#define TG5040_RUMBLE_GPIO    "/sys/class/gpio/gpio227/value"

// The two tg5040 models as data. They differ only in screen geometry, menu
// layout, and a few joystick codes; the hardware paths below are shared.
static const DeviceDescriptor TG5040_BRICK = {
	.scale = 3, .width = 1024, .height = 768,
	.main_row_count = 7, .quick_switcher_count = 3, .padding = 5,
	.joy_l3 = 9, .joy_r3 = 10,
	.joy_l4 = JOY_NA, .joy_r4 = JOY_NA, .joy_menu_alt = JOY_NA,
	.joy_plus = 14, .joy_minus = 13,
	.cpu_speed_path = TG5040_CPU_SPEED_PATH,
	.gpu_temp_path = TG5040_GPU_TEMP_PATH,
	.rumble_gpio_path = TG5040_RUMBLE_GPIO,
	.gpu_speed_fixed = 660,
	.gpu_freq_path = NULL,
	.gpu_usage_path = NULL,
};
static const DeviceDescriptor TG5040_SMART_PRO = {
	.scale = 2, .width = 1280, .height = 720,
	.main_row_count = 10, .quick_switcher_count = 4, .padding = 10,
	.joy_l3 = JOY_NA, .joy_r3 = JOY_NA,
	.joy_l4 = JOY_NA, .joy_r4 = JOY_NA, .joy_menu_alt = JOY_NA,
	.joy_plus = 128, .joy_minus = 129,
	.cpu_speed_path = TG5040_CPU_SPEED_PATH,
	.gpu_temp_path = TG5040_GPU_TEMP_PATH,
	.rumble_gpio_path = TG5040_RUMBLE_GPIO,
	.gpu_speed_fixed = 660,
	.gpu_freq_path = NULL,
	.gpu_usage_path = NULL,
};
// TrimUI Brick Pro: a distinct tg5040 device (is_brickpro) sharing the Brick's
// 1024x768 screen and menu layout, but adding the Smart Pro's dual analog
// thumbsticks plus two rear trigger buttons (L4/R4, joy codes 11/12) and an
// alternate menu button (joy 15). It drives five addressable LED zones (F1, F2,
// top bar, joysticks, rear triggers) and carries its own brightness curve and
// display-cal preset (see libmsettings/msettings.c). UNVERIFIED: model string
// and joystick codes follow upstream #766; confirm against actual hardware.
static const DeviceDescriptor TG5040_BRICK_PRO = {
	.scale = 3, .width = 1024, .height = 768,
	.main_row_count = 7, .quick_switcher_count = 3, .padding = 5,
	.joy_l3 = 9, .joy_r3 = 10,
	.joy_l4 = 11, .joy_r4 = 12, .joy_menu_alt = 15,
	.joy_plus = 14, .joy_minus = 13,
	.cpu_speed_path = TG5040_CPU_SPEED_PATH,
	.gpu_temp_path = TG5040_GPU_TEMP_PATH,
	.rumble_gpio_path = TG5040_RUMBLE_GPIO,
	.gpu_speed_fixed = 660,
	.gpu_freq_path = NULL,
	.gpu_usage_path = NULL,
};
// Default to Smart Pro so the geometry/button macros are valid even before
// init runs; resolveDeviceModel() refines it once DEVICE is known.
const DeviceDescriptor* deviceModel = &TG5040_SMART_PRO;

// Single source of truth for which model we're on. Sets is_brick and is_brickpro
// (Brick Pro is a distinct device, not part of the Brick family) and points
// deviceModel at the active descriptor.
static void resolveDeviceModel(void) {
	const char* device = getenv("DEVICE");
	is_brick = exactMatch("brick", device);
	is_brickpro = exactMatch("brickpro", device);
	if (is_brickpro)      deviceModel = &TG5040_BRICK_PRO;
	else if (is_brick)    deviceModel = &TG5040_BRICK;
	else                  deviceModel = &TG5040_SMART_PRO;
}

void PLAT_initPlatform(void) {
	resolveDeviceModel();
}

static SDL_Joystick **joysticks = NULL;
static int num_joysticks = 0;
void PLAT_initInput(void) {
	resolveDeviceModel();
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
	// The UDC (USB Device Controller) reports "configured" once a host has
	// enumerated us as a USB gadget. This is independent of merely being
	// plugged into a charger, so it lets us tell "connected to a computer"
	// apart from "connected to power".
	DIR *dir = opendir("/sys/class/udc");
	if (!dir)
		return 0;

	int connected = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue;

		char path[512];
		snprintf(path, sizeof(path), "/sys/class/udc/%s/state", entry->d_name);

		char state[32] = {0};
		getFile(path, state, sizeof(state));
		if (strncmp(state, "configured", 10) == 0)
		{
			connected = 1;
			break;
		}
	}

	closedir(dir);
	return connected;
}

void PLAT_enableBacklight(int enable) {
	if (enable) {
		if (is_brick || is_brickpro) SetRawBrightness(8);
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



#define MAX_STRENGTH 0xFFFF
#define MIN_VOLTAGE 500000
// Brick Pro has a 3.3V motor driver, but its getting very annoying on higher rumble strengths, so we limit it to 2.5V for now.
#define MAX_VOLTAGE (is_brickpro ? 2500000 : 3300000)
#define RUMBLE_VOLTAGE_PATH "/sys/class/motor/voltage"

void PLAT_setRumble(int strength) {
	int voltage = MAX_VOLTAGE;

	if(strength > 0 && strength < MAX_STRENGTH) {
		voltage = MIN_VOLTAGE + (int)(strength * ((long long)(MAX_VOLTAGE - MIN_VOLTAGE) / MAX_STRENGTH));
		putInt(RUMBLE_VOLTAGE_PATH, voltage);
	}
	else {
		putInt(RUMBLE_VOLTAGE_PATH, MAX_VOLTAGE);
	}

	// enable rumble - removed the FN switch disabling haptics
	// did not make sense 
	putInt(deviceModel->rumble_gpio_path, (strength) ? 1 : 0);
}



char* PLAT_getModel(void) {
	char* model = getenv("TRIMUI_MODEL");
	if (model) return model;
	return "Trimui Smart Pro";
}




void PLAT_initDefaultLeds() {
	resolveDeviceModel();
	if(is_brickpro) {
		lightsDefault[0] = (LightSettings) {"FN 1 key","f1",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[1] = (LightSettings) {"FN 2 key","f2",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[2] = (LightSettings) {"Topbar","m",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[3] = (LightSettings) {"Joysticks L/R","lr",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[4] = (LightSettings) {"Triggers L/R","rear",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
	}
	else if(is_brick) {
		lightsDefault[0] = (LightSettings) {"FN 1 key","f1",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[1] = (LightSettings) {"FN 2 key","f2",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[2] = (LightSettings) {"Topbar","m",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[3] = (LightSettings) {"L/R triggers","lr",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
	}
	else {
		lightsDefault[0] = (LightSettings) {"Joystick L","l",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[1] = (LightSettings) {"Joystick R","r",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
		lightsDefault[2] = (LightSettings) {"Logo","m",4,1000,100,0xFFFFFF,0xFFFFFF,0,{},1,100,0};
	}
}
void PLAT_initLeds(LightSettings *lights) 
{
	resolveDeviceModel();

	PLAT_initDefaultLeds();
	FILE *file;
	if(is_brick) {
		file = PLAT_OpenSettings("ledsettings_brick.txt");
	}
	else if(is_brickpro) {
		file = PLAT_OpenSettings("ledsettings_brickpro.txt");
	}
	else {
		file = PLAT_OpenSettings("ledsettings.txt");
	}

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
#define LED_PATH4 "/sys/class/led_anim/max_scale_rear"

void PLAT_setLedInbrightness(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
	if(is_brick || is_brickpro) {
		if (strcmp(led->filename, "m") == 0) {
			snprintf(filepath, sizeof(filepath), LED_PATH1);
		} else if (strcmp(led->filename, "f1") == 0) {
			snprintf(filepath, sizeof(filepath),LED_PATH3);
		} else  {
			snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale_%s", led->filename);
		}
	} else {
		snprintf(filepath, sizeof(filepath), LED_PATH1);
	}
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
	if(is_brick || is_brickpro) {
		if (strcmp(led->filename, "m") == 0) {
			snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale");
		} else if (strcmp(led->filename, "f1") == 0) {
			snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale_f1f2");
		} else  {
			snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale_%s", led->filename);
		}
	} else {
		snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/max_scale");
	}
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

	char *output = (char *)malloc(256);
	if (!output) {
		return NULL;
	}
	FILE *fp = popen("uci get system.@system[0].zonename", "r");
	if (!fp) {
		free(output);
		return NULL;
	}
	fgets(output, 256, fp);
	pclose(fp);
	trimTrailingNewlines(output);

	return output;
}

void PLAT_setCurrentTimezone(const char* tz) {
	if (cached_tz_count == -1) {
		LOG_warn("Error: Timezones not initialized. Call PLAT_initTimezones first.\n");
        return;
    }

	// This makes it permanent
	char *zonename = (char *)malloc(256);
	if (!zonename)
		return;
	snprintf(zonename, 256, "uci set system.@system[0].zonename=\"%s\"", tz);
	system(zonename);
	//system("uci set system.@system[0].zonename=\"Europe/Berlin\"");
	system("uci del -q system.@system[0].timezone");
	system("uci commit system");
	free(zonename);

	// This fixes the timezone until the next reboot
	char *tz_path = (char *)malloc(256);
	if (!tz_path) {
		return;
	}
	snprintf(tz_path, 256, ZONE_PATH "/%s", tz);
	// replace existing symlink
	if (unlink("/tmp/localtime") == -1) {
		LOG_error("Failed to remove existing symlink: %s\n", strerror(errno));
	}
	if (symlink(tz_path, "/tmp/localtime") == -1) {
		LOG_error("Failed to set timezone: %s\n", strerror(errno));
	}
	free(tz_path);

	// apply timezone to kernel
	system("date -k");
}

bool PLAT_getNetworkTimeSync(void) {
	char *output = (char *)malloc(256);
	if (!output) {
		return false;
	}
	FILE *fp = popen("uci get system.ntp.enable", "r");
	if (!fp) {
		free(output);
		return false;
	}
	fgets(output, 256, fp);
	pclose(fp);
	bool result = (output[0] == '1');
	free(output);
	return result;
}

void PLAT_setNetworkTimeSync(bool on) {
	// note: this is not the service residing at /etc/init.d/ntpd - that one has hardcoded time server URLs and does not interact with UCI.
	if (on) {
		// permanment
		system("uci set system.ntp.enable=1");
		system("uci commit system");
		system("/etc/init.d/ntpd reload");
	} else {
		// permanment
		system("uci set system.ntp.enable=0");
		system("uci commit system");
		system("/etc/init.d/ntpd stop");
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
