#ifndef DEVICE_H
#define DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

// A device model is described by data, not code: the fields below are the
// only things that differ between hardware variants. Each platform defines
// one or more descriptors and points deviceModel at the active one (see
// resolveDeviceModel in the platform layer). Adding a device becomes a
// matter of adding a descriptor rather than another runtime branch.
typedef struct DeviceDescriptor {
	int scale;                 // FIXED_SCALE
	int width;                 // FIXED_WIDTH
	int height;                // FIXED_HEIGHT
	int main_row_count;        // MAIN_ROW_COUNT
	int quick_switcher_count;  // QUICK_SWITCHER_COUNT
	int padding;               // PADDING
	int joy_l3;                // JOY_L3
	int joy_r3;                // JOY_R3
	int joy_l4;                // JOY_L4 (Brick Pro rear triggers)
	int joy_r4;                // JOY_R4 (Brick Pro rear triggers)
	int joy_menu_alt;          // JOY_MENU_ALT (Brick Pro)
	int joy_plus;              // JOY_PLUS
	int joy_minus;             // JOY_MINUS
	// sysfs hardware nodes that differ across platforms -- the seams a future
	// device redefines. Identical for Brick and Smart Pro; tg5050, for example,
	// uses cpu4 not cpu0, thermal_zone5 not zone2, gpio236 not gpio227.
	const char* cpu_speed_path;
	const char* gpu_temp_path;
	const char* rumble_gpio_path;
	// GPU frequency/utilization: some devices read a sysfs node, others report
	// a fixed clock. gpu_freq_path NULL => PLAT_getGPUSpeed uses gpu_speed_fixed;
	// gpu_usage_path NULL => GPU utilization is unavailable on this device.
	int gpu_speed_fixed;
	const char* gpu_freq_path;
	const char* gpu_usage_path;
} DeviceDescriptor;
extern const DeviceDescriptor* deviceModel;

#ifdef __cplusplus
}
#endif

#endif // DEVICE_H
