// generic_platform.c -- PLAT_ functions identical across tg5040 and tg5050.
// Text-included near the end of each platform's platform.c (after the file-local
// static helpers and globals it references). Phase C device-HAL dedup: divergent
// functions (LEDs, timezones, NTP, rumble, model, input init) stay per-platform;
// a new device only reimplements those.

void PLAT_quitInput(void) {
	if (joysticks) {
        for (int i = 0; i < num_joysticks; i++) {
            if (SDL_JoystickGetAttached(joysticks[i])) {
				LOG_info("Closing joystick %d: %s\n", i, SDL_JoystickName(joysticks[i]));
				SDL_JoystickClose(joysticks[i]);
			}
        }
        free(joysticks);
        joysticks = NULL;
        num_joysticks = 0;
    }
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void PLAT_updateInput(const SDL_Event *event) {
	switch (event->type) {
    case SDL_JOYDEVICEADDED: {
        int device_index = event->jdevice.which;
        SDL_Joystick *new_joy = SDL_JoystickOpen(device_index);
        if (new_joy) {
            joysticks = (SDL_Joystick **)realloc(joysticks, sizeof(SDL_Joystick *) * (num_joysticks + 1));
            joysticks[num_joysticks++] = new_joy;
            LOG_info("Joystick added at index %d: %s\n", device_index, SDL_JoystickName(new_joy));
        } else {
            LOG_error("Failed to open added joystick at index %d: %s\n", device_index, SDL_GetError());
        }
        break;
    }

    case SDL_JOYDEVICEREMOVED: {
        SDL_JoystickID removed_id = event->jdevice.which;
        for (int i = 0; i < num_joysticks; ++i) {
            if (SDL_JoystickInstanceID(joysticks[i]) == removed_id) {
                LOG_info("Joystick removed: %s\n", SDL_JoystickName(joysticks[i]));
                SDL_JoystickClose(joysticks[i]);

                // Shift down the remaining entries
                for (int j = i; j < num_joysticks - 1; ++j)
                    joysticks[j] = joysticks[j + 1];
                num_joysticks--;

                if (num_joysticks == 0) {
                    free(joysticks);
                    joysticks = NULL;
                } else {
                    joysticks = (SDL_Joystick **)realloc(joysticks, sizeof(SDL_Joystick *) * num_joysticks);
                }
                break;
            }
        }
        break;
    }

    default:
        break;
    }
}

void PLAT_getBatteryStatus(int* is_charging, int* charge) {
	PLAT_getBatteryStatusFine(is_charging, charge);

	// worry less about battery and more about the game you're playing
	     if (*charge>80) *charge = 100;
	else if (*charge>60) *charge =  80;
	else if (*charge>40) *charge =  60;
	else if (*charge>20) *charge =  40;
	else if (*charge>10) *charge =  20;
	else           		 *charge =  10;
}

void PLAT_getCPUTemp() {
	perf.cpu_temp = getInt("/sys/devices/virtual/thermal/thermal_zone0/temp")/1000;
}

void PLAT_getCPUSpeed()
{
	perf.cpu_speed = getInt(deviceModel->cpu_speed_path)/1000;
}

void PLAT_getGPUTemp() {
	perf.gpu_temp = getInt(deviceModel->gpu_temp_path)/1000;
}

void PLAT_getGPUSpeed() {
	perf.gpu_speed = deviceModel->gpu_freq_path ? getInt(deviceModel->gpu_freq_path)/1000000 : deviceModel->gpu_speed_fixed; // MHz
}

void PLAT_getNetworkStatus(int* is_online)
{
	if(WIFI_enabled())
		WIFI_connectionInfo(&connection);
	else
		connection_reset(&connection);
	
	if(is_online)
		*is_online = (connection.valid && connection.ssid[0] != '\0');
	
	if(BT_enabled()) {
		bluetoothConnected = PLAT_bluetoothConnected();
	}
	else
		bluetoothConnected = false;
}

void PLAT_getBatteryStatusFine(int *is_charging, int *charge)
{	
	if(is_charging) {
		int time_to_full = getInt("/sys/class/power_supply/axp2202-battery/time_to_full_now");
		int charger_present = getInt("/sys/class/power_supply/axp2202-usb/online"); 
		*is_charging = (charger_present == 1) && (time_to_full > 0);
	}
	if(charge) {
		*charge = getInt("/sys/class/power_supply/axp2202-battery/capacity");
	}
}

void PLAT_powerOff(int reboot) {
	if (CFG_getHaptics()) {
		VIB_singlePulse(VIB_bootStrength, VIB_bootDuration_ms);
	}
	system("rm -f /tmp/nextui_exec && sync");
	sleep(2);

	SetRawVolume(MUTE_VOLUME_RAW);
	PLAT_enableBacklight(0);
	SND_quit();
	VIB_quit();
	PWR_quit();
	GFX_quit();

	system("cat /dev/zero > /dev/fb0 2>/dev/null");
	if(reboot > 0)
		touch("/tmp/reboot");
	else
		touch("/tmp/poweroff");
	sync();
	exit(0);
}

int PLAT_supportsDeepSleep(void) { return 1; }

void *PLAT_cpu_monitor(void *arg) {
    if (!Perf_tryBeginCPUMonitor()) return NULL;

    double prev_real_time = get_time_sec();
    double prev_cpu_time = get_process_cpu_time_sec();

    double cpu_usage_history[ROLLING_WINDOW] = {0};
    int history_index = 0;
    int history_count = 0;

    while (Perf_isCPUMonitorEnabled()) {
        double curr_real_time = get_time_sec();
        double curr_cpu_time = get_process_cpu_time_sec();

        double elapsed_real_time = curr_real_time - prev_real_time;
        double elapsed_cpu_time = curr_cpu_time - prev_cpu_time;

        if (elapsed_real_time > 0) {
            double cpu_usage = (elapsed_cpu_time / elapsed_real_time) * 100.0;

            pthread_mutex_lock(&currentcpuinfo);

            cpu_usage_history[history_index] = cpu_usage;
            history_index = (history_index + 1) % ROLLING_WINDOW;
            if (history_count < ROLLING_WINDOW) history_count++;

            double sum_cpu_usage = 0;
            for (int i = 0; i < history_count; i++) sum_cpu_usage += cpu_usage_history[i];
            perf.cpu_usage = sum_cpu_usage / history_count;

            pthread_mutex_unlock(&currentcpuinfo);
        }

        prev_real_time = curr_real_time;
        prev_cpu_time = curr_cpu_time;
        usleep(100000);
    }

    Perf_endCPUMonitor();
    return NULL;
}

void PLAT_setCPUSpeed(int speed) {
	const char* mode;
	switch (speed) {
		case CPU_SPEED_AUTO: mode = "auto"; break;
		case CPU_SPEED_PERFORMANCE: mode = "performance"; break;
		case CPU_SPEED_POWERSAVE: mode = "powersave"; break;
		default: return;
	}
	
	const char* system_path = getenv("SYSTEM_PATH");
	if (!system_path) {
		LOG_info("WARNING: SYSTEM_PATH not set, cannot run governor script\n");
		return;
	}
	char cmd[512];
	int n = snprintf(cmd, sizeof(cmd), "sh \"%s/bin/governor.sh\" \"%s\"", system_path, mode);
	if (n < 0 || n >= (int)sizeof(cmd)) {
		LOG_info("WARNING: SYSTEM_PATH too long for governor script path\n");
		return;
	}
	int ret = system(cmd);
	if (ret != 0) LOG_info("WARNING: governor script exited with status %d for mode '%s'\n", ret, mode);
}

int PLAT_pickSampleRate(int requested, int max) {
	// bluetooth: allow limiting the maximum to improve compatibility
	if(PLAT_bluetoothConnected())
		return MIN(requested, CFG_getBluetoothSamplingrateLimit());

	return MIN(requested, max);
}

void PLAT_overrideMute(int mute) {
	putInt("/sys/class/speaker/mute", mute);
}

void PLAT_getOsVersionInfo(char* output_str, size_t max_len)
{
	return getFile("/etc/version", output_str,max_len);
}

bool PLAT_btIsConnected(void)
{
	return bluetoothConnected;
}

ConnectionStrength PLAT_connectionStrength(void) {
	if(!WIFI_enabled() || !connection.valid || connection.rssi == -1)
		return SIGNAL_STRENGTH_OFF;
	else if (connection.rssi == 0)
		return SIGNAL_STRENGTH_DISCONNECTED;
	else if (connection.rssi >= -60)
		return SIGNAL_STRENGTH_HIGH;
	else if (connection.rssi >= -70)
		return SIGNAL_STRENGTH_MED;
	else
		return SIGNAL_STRENGTH_LOW;
}

void PLAT_setLedEffect(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
    snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/effect_%s", led->filename);
    file = fopen(filepath, "w");
    if (file != NULL)
    {
        fprintf(file, "%i\n", led->effect);
        fclose(file);
    }
}

void PLAT_setLedEffectCycles(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
    snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/effect_cycles_%s", led->filename);
    file = fopen(filepath, "w");
    if (file != NULL)
    {
        fprintf(file, "%i\n", led->cycles);
        fclose(file);
    }
}

void PLAT_setLedEffectSpeed(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
    snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/effect_duration_%s", led->filename);
    file = fopen(filepath, "w");
    if (file != NULL)
    {
        fprintf(file, "%i\n", led->speed);
        fclose(file);
    }
}

void PLAT_setLedColor(LightSettings *led)
{
    char filepath[256];
    FILE *file;
    // first set brightness
    snprintf(filepath, sizeof(filepath), "/sys/class/led_anim/effect_rgb_hex_%s", led->filename);
    file = fopen(filepath, "w");
    if (file != NULL)
    {
        fprintf(file, "%06X\n", led->color1);
        fclose(file);
    }
}

bool PLAT_canTurbo(void) { return true; }

int PLAT_toggleTurbo(int btn_id)
{
	// avoid extra file IO on each call
	static int initialized = 0;
	if (!initialized) {
		mkdir(INPUTD_PATH, 0755);
		initialized = 1;
	}

	for (int i = 0; turbo_mapping[i].path; i++) {
		if (turbo_mapping[i].brn_id == btn_id) {
			return toggle_file(turbo_mapping[i].path);
		}
	}
	return 0;
}

void PLAT_clearTurbo() {
	for (int i = 0; turbo_mapping[i].path; i++) {
		unlink(turbo_mapping[i].path);
	}
}

int PLAT_setDateTime(int y, int m, int d, int h, int i, int s) {
	char cmd[512];
	sprintf(cmd, "date -s '%d-%d-%d %d:%d:%d'; hwclock -u -w", y,m,d,h,i,s);
	system(cmd);
	return 0; // why does this return an int?
}

void PLAT_initTimezones() {
    if (cached_tz_count != -1) { // Already initialized
        return;
    }
    
    FILE *file = fopen(ZONE_TAB_PATH, "r");
    if (!file) {
        LOG_info("Error opening file %s\n", ZONE_TAB_PATH);
        return;
    }
    
    char line[MAX_LINE_LENGTH];
    cached_tz_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comment lines
        if (line[0] == '#' || strlen(line) < 3) {
            continue;
        }
        
        char *token = strtok(line, "\t"); // Skip country code
        if (!token) continue;
        
        token = strtok(NULL, "\t"); // Skip latitude/longitude
        if (!token) continue;
        
        token = strtok(NULL, "\t\n"); // Extract timezone
        if (!token) continue;
        
        // Check for duplicates before adding
        int duplicate = 0;
        for (int i = 0; i < cached_tz_count; i++) {
            if (strcmp(cached_timezones[i], token) == 0) {
                duplicate = 1;
                break;
            }
        }
        
        if (!duplicate && cached_tz_count < MAX_TIMEZONES) {
            strncpy(cached_timezones[cached_tz_count], token, MAX_TZ_LENGTH - 1);
            cached_timezones[cached_tz_count][MAX_TZ_LENGTH - 1] = '\0'; // Ensure null-termination
            cached_tz_count++;
        }
    }
    
    fclose(file);
    
    // Sort the list alphabetically
    qsort(cached_timezones, cached_tz_count, MAX_TZ_LENGTH, compare_timezones);
}

void PLAT_getTimezones(char timezones[MAX_TIMEZONES][MAX_TZ_LENGTH], int *tz_count) {
    if (cached_tz_count == -1) {
        LOG_warn("Error: Timezones not initialized. Call PLAT_initTimezones first.\n");
        *tz_count = 0;
        return;
    }
    
    memcpy(timezones, cached_timezones, sizeof(cached_timezones));
    *tz_count = cached_tz_count;
}

