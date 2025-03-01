#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>

#include <msettings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

// #include "defines.h"

#define VOLUME_MIN 		0
#define VOLUME_MAX 		20
#define BRIGHTNESS_MIN 	0
#define BRIGHTNESS_MAX 	10

#define CODE_PLUS		318
#define CODE_MINUS		317

#define CODE_L1 310
#define CODE_R1 311
#define CODE_SELECT 314
#define CODE_START 315

//	for ev.value
#define RELEASED	0
#define PRESSED		1
#define REPEAT		2

#define INPUT_COUNT 2
static int inputs[INPUT_COUNT];
static struct input_event ev;

int main (int argc, char *argv[]) {
	InitSettings();
	
	// TODO: will require two inputs
	// input_fd = open("/dev/input/event0", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	inputs[0] = open("/dev/input/event2", O_RDONLY | O_NONBLOCK | O_CLOEXEC); // gamepad/menu
	inputs[1] = open("/dev/input/event3", O_RDONLY | O_NONBLOCK | O_CLOEXEC); // volume
	
	uint32_t input;
	uint32_t val;
	uint32_t menu_pressed = 0;
	
	uint32_t up_pressed = 0;
	uint32_t up_just_pressed = 0;
	uint32_t up_repeat_at = 0;
	
	uint32_t down_pressed = 0;
	uint32_t down_just_pressed = 0;
	uint32_t down_repeat_at = 0;
	
	uint8_t ignore;
	uint32_t then;
	uint32_t now;
	struct timeval tod;

	// TODO: tmp
	uint32_t select_pressed = 0;
	uint32_t start_pressed = 0;
	
	gettimeofday(&tod, NULL);
	then = tod.tv_sec * 1000 + tod.tv_usec / 1000; // essential SDL_GetTicks()
	ignore = 0;
	
	while (1) {
		gettimeofday(&tod, NULL);
		now = tod.tv_sec * 1000 + tod.tv_usec / 1000;
		if (now-then>1000) ignore = 1; // ignore input that arrived during sleep
		
		for (int i=0; i<INPUT_COUNT; i++) {
			int input_fd = inputs[i];
			while(read(input_fd, &ev, sizeof(ev))==sizeof(ev)) {
				if (ignore) continue;
				val = ev.value;

				if (( ev.type != EV_KEY ) || ( val > REPEAT )) continue;
				//printf("code: %i (%i)\n", ev.code, val); fflush(stdout);
				switch (ev.code) {
					case CODE_MINUS:
						menu_pressed = val;
					break;
					// TODO: tmp
					case CODE_L1: 
						down_pressed = down_just_pressed = val;
						if (val) down_repeat_at = now + 300;
					break;
					case CODE_R1: 
						up_pressed = up_just_pressed = val;
						if (val) up_repeat_at = now + 300;
					break;
					case CODE_START: start_pressed = val; break;
					case CODE_SELECT: select_pressed = val; break;
					default:
					break;
				}
			}
		}
		
		if (ignore) {
			menu_pressed = 0;
			up_pressed = up_just_pressed = 0;
			down_pressed = down_just_pressed = 0;
			up_repeat_at = 0;
			down_repeat_at = 0;
		}

		// TODO: tmp
		if (up_pressed && down_pressed && start_pressed && select_pressed) {
			system("sync && killall -9 minarch.elf"); // TODO: tmp
			system("sync && killall -9 launch.sh"); // TODO: tmp
		}
		
		if (up_just_pressed || (up_pressed && now>=up_repeat_at)) {
			if (menu_pressed) {
				// printf("brightness up\n"); fflush(stdout);
				val = GetBrightness();
				if (val<BRIGHTNESS_MAX) SetBrightness(++val);
			}
			else if(start_pressed) {
				// printf("volume up\n"); fflush(stdout);
				val = GetVolume();
				if (val<VOLUME_MAX) SetVolume(++val);
			}
			
			if (up_just_pressed) up_just_pressed = 0;
			else up_repeat_at += 100;
		}
		
		if (down_just_pressed || (down_pressed && now>=down_repeat_at)) {
			if (menu_pressed) {
				// printf("brightness down\n"); fflush(stdout);
				val = GetBrightness();
				if (val>BRIGHTNESS_MIN) SetBrightness(--val);
			}
			else if(start_pressed) {
				// printf("volume down\n"); fflush(stdout);
				val = GetVolume();
				if (val>VOLUME_MIN) SetVolume(--val);
			}
			
			if (down_just_pressed) down_just_pressed = 0;
			else down_repeat_at += 100;
		}
		
		then = now;
		ignore = 0;
		
		usleep(16666); // 60fps
	}
}
