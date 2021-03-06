#include "Clock.h"
#include "Color.h"
#include "lib/draw/draw.h"
#include "lib/rtc/mcp79410/mcp79410.h"
#include <util/delay.h>
#include <stdlib.h>

using namespace digitalcave;

Clock::Clock() {
}

Clock::~Clock() {
}

void Clock::run() {
	Color color;
	mcp79410_time_t time;
	uint16_t buttons;
	ws2812_t c = color.white;
	uint8_t running = 1;

	while (running) {
		mcp79410_get(time);
		
		if (running == 1) {
			// hour and minute hands drawn as lines
			uint8_t h = time.hour % 12;
			uint8_t m = time.minute & 12;
			
			c = color.white;
			
			set_pixel(5, 0, c, OVERLAY_REPLACE);
			set_pixel(0, 5, c, OVERLAY_REPLACE);
			set_pixel(10, 0, c, OVERLAY_REPLACE);
			set_pixel(5, 10, c, OVERLAY_REPLACE);
			
			c = color.red;
			
			if (minute == 0) draw_line(5, 5, 5, 0, c, OVERLAY_REPLACE);
			else if (minute == 1) draw_line(5, 5, 7, 0, c, OVERLAY_REPLACE);
			else if (minute == 2) draw_line(5, 5, 10, 3, c, OVERLAY_REPLACE);
			else if (minute == 3) draw_line(5, 5, 10, 5, c, OVERLAY_REPLACE);
			else if (minute == 4) draw_line(5, 5, 10, 7, c, OVERLAY_REPLACE);
			else if (minute == 5) draw_line(5, 5, 7, 10, c, OVERLAY_REPLACE);
			else if (minute == 6) draw_line(5, 5, 5, 10, c, OVERLAY_REPLACE);
			else if (minute == 7) draw_line(5, 5, 3, 10, c, OVERLAY_REPLACE);
			else if (minute == 8) draw_line(5, 5, 0, 7, c, OVERLAY_REPLACE);
			else if (minute == 9) draw_line(5, 5, 0, 5, c, OVERLAY_REPLACE);
			else if (minute == 10) draw_line(5, 5, 0, 3, c, OVERLAY_REPLACE);
			else if (minute == 11) draw_line(5, 5, 3, 0, c, OVERLAY_REPLACE);
			
			c = color.blue;
			
			if (hour == 0) draw_line(5, 5, 5, 2, c, OVERLAY_REPLACE);
			else if (hour == 1) draw_line(5, 5, 6, 2, c, OVERLAY_REPLACE);
			else if (hour == 2) draw_line(5, 5, 8, 4, c, OVERLAY_REPLACE);
			else if (hour == 3) draw_line(5, 5, 8, 5, c, OVERLAY_REPLACE);
			else if (hour == 4) draw_line(5, 5, 8, 6, c, OVERLAY_REPLACE);
			else if (hour == 5) draw_line(5, 5, 6, 8, c, OVERLAY_REPLACE);
			else if (hour == 6) draw_line(5, 5, 5, 8, c, OVERLAY_REPLACE);
			else if (hour == 7) draw_line(5, 5, 4, 8, c, OVERLAY_REPLACE);
			else if (hour == 8) draw_line(5, 5, 2, 6, c, OVERLAY_REPLACE);
			else if (hour == 9) draw_line(5, 5, 2, 5, c, OVERLAY_REPLACE);
			else if (hour == 10) draw_line(5, 5, 2, 4, c, OVERLAY_REPLACE);
			else if (hour == 11) draw_line(5, 5, 4, 2, c, OVERLAY_REPLACE);
		}
		else if (running == 2) {
			// hour and minute hands drawn as blocks
			uint8_t h = time.hour % 12;
			uint8_t m = time.minute & 12;
			
			c = color.white;
			draw_rectangle(2, 2, 9, 9, DRAW_UNFILLED, c, OVERLAY_REPLACE);

			c = color.red;
			
			if (minute == 0) draw_rectangle(5, 0, 6, 1, c, OVERLAY_REPLACE);
			else if (minute == 1) draw_rectangle(7, 0, 8, 1, c, OVERLAY_REPLACE);
			else if (minute == 2) draw_rectangle(10, 3, 11, 4, c, OVERLAY_REPLACE);
			else if (minute == 3) draw_rectangle(10, 5, 11, 6, c, OVERLAY_REPLACE);
			else if (minute == 4) draw_rectangle(10, 7, 11, 8, c, OVERLAY_REPLACE);
			else if (minute == 5) draw_rectangle(7, 10, 8, 11, c, OVERLAY_REPLACE);
			else if (minute == 6) draw_rectangle(5, 10, 6, 11, c, OVERLAY_REPLACE);
			else if (minute == 7) draw_rectangle(3, 10, 4, 11, c, OVERLAY_REPLACE);
			else if (minute == 8) draw_rectangle(0, 7, 1, 8, c, OVERLAY_REPLACE);
			else if (minute == 9) draw_rectangle(0, 5, 1, 6, c, OVERLAY_REPLACE);
			else if (minute == 10) draw_rectangle(0, 3, 1, 4, c, OVERLAY_REPLACE);
			else if (minute == 11) draw_rectangle(3, 0, 4, 1, c, OVERLAY_REPLACE);
			
			c = color.blue;
			
			if (hour == 0) draw_rectangle(5, 3, 6, 4, c, OVERLAY_REPLACE);
			else if (hour == 1) draw_rectangle(6, 3, 7, 4, c, OVERLAY_REPLACE);
			else if (hour == 2) draw_rectangle(7, 4, 8, 5, c, OVERLAY_REPLACE);
			else if (hour == 3) draw_rectangle(7, 5, 8, 6, c, OVERLAY_REPLACE);
			else if (hour == 4) draw_rectangle(7, 7, 8, 8, c, OVERLAY_REPLACE);
			else if (hour == 5) draw_rectangle(6, 7, 7, 8, c, OVERLAY_REPLACE);
			else if (hour == 6) draw_rectangle(5, 7, 6, 8, c, OVERLAY_REPLACE);
			else if (hour == 7) draw_rectangle(4, 7, 5, 8, c, OVERLAY_REPLACE);
			else if (hour == 8) draw_rectangle(3, 6, 4, 7, c, OVERLAY_REPLACE);
			else if (hour == 9) draw_rectangle(3, 5, 4, 6, c, OVERLAY_REPLACE);
			else if (hour == 10) draw_rectangle(3, 4, 4, 5, c, OVERLAY_REPLACE);
			else if (hour == 11) draw_rectangle(4, 3, 5, 4, c, OVERLAY_REPLACE);
		}
		else if (running == 3) {
			// BCD
			uint8_t one = time.hour;
			uint8_t ten = 0;
			while (one > 10) { 
				ten++;
				one -= 10;
			}
			if (ten & 0x01) draw_rectangle(9,0,11,1,DRAW_FILLED,color.red,OVERLAY_REPLACE);
			if (ten & 0x02) draw_rectangle(6,0,8,1,DRAW_FILLED,color.red,OVERLAY_REPLACE);
			
			if (one & 0x01) draw_rectangle(9,2,11,3,DRAW_FILLED,color.yellow,OVERLAY_REPLACE);
			if (one & 0x02) draw_rectangle(6,2,8,3,DRAW_FILLED,color.yellow,OVERLAY_REPLACE);
			if (one & 0x04) draw_rectangle(3,2,5,3,DRAW_FILLED,color.yellow,OVERLAY_REPLACE);
			if (one & 0x08) draw_rectangle(0,2,3,3,DRAW_FILLED,color.yellow,OVERLAY_REPLACE);

			one = time.minute;
			ten = 0;
			while (one > 10) { 
				ten++;
				one -= 10;
			}
			if (ten & 0x01) draw_rectangle(9,4,11,5,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			if (ten & 0x02) draw_rectangle(6,4,8,5,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			if (ten & 0x04) draw_rectangle(3,4,5,5,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			
			if (one & 0x01) draw_rectangle(9,6,11,7,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x02) draw_rectangle(6,6,8,7,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x04) draw_rectangle(3,6,5,7,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x08) draw_rectangle(0,6,3,7,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);

			one = time.second;
			ten = 0;
			while (one > 10) { 
				ten++;
				one -= 10;
			}
			if (ten & 0x01) draw_rectangle(9,8,11,9,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			if (ten & 0x02) draw_rectangle(6,8,8,9,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			if (ten & 0x04) draw_rectangle(3,8,5,9,DRAW_FILLED,color.green,OVERLAY_REPLACE);
			
			if (one & 0x01) draw_rectangle(9,10,11,11,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x02) draw_rectangle(6,10,8,11,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x04) draw_rectangle(3,10,5,11,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
			if (one & 0x08) draw_rectangle(0,10,3,11,DRAW_FILLED,color.cyan,OVERLAY_REPLACE);
		}
		else if (running == 4) {
			// one pixel for every hour, minute, second
			
		}

		matrix_write_buffer();
		
		void psx_read_gamepad();
		buttons = psx_buttons();
		if (buttons & PSB_PAD_UP || buttons & PSB_LEFT) running -= 1;
		else if (buttons & PSB_PAD_DOWN || buttons & PSB_RIGHT) running += 1;
		
		if (running == 0) running = 4;
		if (running == 5) running = 1;

		if (buttons & PSB_TRIANGLE) running = 0;

		_delay_ms(100);
	}
}
