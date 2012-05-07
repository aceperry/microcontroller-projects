#include "clock.h"
#include "../segment/segment.h"

#include <avr/sfr_defs.h>

static volatile uint8_t _mode = 0;
static uint8_t _segments[6] = {0,0,0,0,0,0};
static uint8_t _segment_flags = 0;
static uint8_t _matrix_red[8] = {0,0,0,0,0,0,0,0};
static uint8_t _matrix_grn[8] = {0,0,0,0,0,0,0,0};

static uint8_t _b25[25][8] = {
	{0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00},
	{0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
	{0x1, 0x2, 0x4, 0x4, 0x4, 0x4, 0x2, 0x1},
	{0x8, 0x4, 0x2, 0x1, 0x1, 0x2, 0x4, 0x8},
	{0x0, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18},
	{0x0, 0x0, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0},
	{0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18},
	{0x1, 0x2, 0x4, 0xff, 0xff, 0x4, 0x2, 0x1},
	{0x8, 0x4, 0x2, 0xff, 0xff, 0x2, 0x4, 0x8},
	{0x0, 0xf8, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18},
	{0x0, 0x0, 0x0, 0x0, 0x0, 0x3c, 0x42, 0x81},
	{0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x5a, 0x99},
	{0x1, 0x2, 0x4, 0x4, 0x4, 0x3c, 0x42, 0x81},
	{0x8, 0x4, 0x2, 0x1, 0x1, 0x3e, 0x46, 0x89},
	{0x0, 0xf8, 0x18, 0x18, 0x18, 0x3c, 0x5a, 0x99},
	{0x0, 0x0, 0x0, 0x0, 0x81, 0x42, 0x24, 0x18},
	{0x18, 0x18, 0x18, 0x18, 0x99, 0x5a, 0x3c, 0x18},
	{0x1, 0x2, 0x4, 0x4, 0x85, 0x46, 0x26, 0x19},
	{0x8, 0x4, 0x2, 0x1, 0x81, 0x42, 0x24, 0x18},
	{0x0, 0xf8, 0x18, 0x18, 0x99, 0x5a, 0x3c, 0x18},
	{0x2, 0x2, 0x2, 0xfe, 0xfe, 0x0, 0x0, 0x0},
	{0x1a, 0x1a, 0x1a, 0xfe, 0xfe, 0x18, 0x18, 0x18},
	{0x3, 0x2, 0x6, 0xfe, 0xfe, 0x4, 0x2, 0x1},
	{0xa, 0x6, 0x2, 0xff, 0xff, 0x2, 0x4, 0x8},
	{0x2, 0xfa, 0x1a, 0xfe, 0xfe, 0x18, 0x18, 0x18},
};

void clock_mode(uint8_t mode) {
	_mode = mode;
}

/*
 * Sets a char array according to the time in the given mode.
 */
void clock_segments(uint8_t result[]) {
	for (uint8_t i = 0; i < 4; i++) {
		result[i] = _segments[i];
	}
}

uint8_t clock_segment_flags() {
	return _segment_flags;
}


/*
 * Sets a matrix array according to the time in the given mode.
 */
void clock_matrix(uint8_t red[], uint8_t grn[]) {
	for (uint8_t i = 0; i < 8; i++) {
		red[i] = _matrix_red[i];
		grn[i] = _matrix_grn[i];
	}
}

void clock_clear_matrix() {
	for (uint8_t i = 0; i < 8; i++) {
		_matrix_red[i] = 0;
		_matrix_grn[i] = 0;
	}
}

/*
 * This is the normal method of keeping time used worldwide
 */
void clock_traditional(uint32_t ms) {
	//milliseconds to traditional (24:59:59)
	uint8_t hr = ms / 3600000;	// 1/24 day (hour)
	ms -= 3600000 * (uint32_t) hr;
	uint8_t mn = ms / 60000;	// 1/1440 day (minute)
	ms -= 60000 * (uint32_t) mn;
	uint8_t sc = ms / 1000;		// 1/86400 day (second)	
	
	uint8_t digits[6];
	digits[0] = 0;
	while (hr > 9) {
		digits[0] += 1;
		hr -= 10;
	}
	digits[1] = hr;
	
	digits[2] = 0;
	while (mn > 9) {
		digits[2] += 1;
		mn -= 10;
	}
	digits[3] = mn;
	
	digits[4] = 0;
		while(sc > 9) {
		digits[4] += 1;
		sc -= 10;
	}
	digits[5] = sc;

	for(uint8_t i = 0; i < 4; i++) {
		_segments[i] = segment_decimal(digits[i]);
	}
	if (_segments[0] == 0) {
		_segments[0] = segment_character(' ');
	}
	
	// flash the colon
	if ((sc & _BV(0)) == _BV(0)) {
		_segment_flags = _BV(0);
	} else {
		_segment_flags = 0x00;
	}
	
	clock_clear_matrix();

	// 3 2x8 bars with a space sparating them, hr on top, sec on bottom
	for (uint8_t i = 0; i < 6; i++) { // segments (rows)
		// build a 1x6 bar
		uint8_t v = digits[i];
		for (uint8_t j = 0; j < 8; j++) { // bits (cols)
			uint8_t row = i;
			if (i > 1) row++; // blank rows
			if (i > 3) row++;
			if ((v & _BV(j)) != 0) {
				if (i == 2 || i == 3) {
					_matrix_grn[row] |= 1 << (7-(j*2));
					_matrix_grn[row] |= 1 << (6-(j*2));
				} else {
					_matrix_red[row] |= 1 << (7-(j*2));
					_matrix_red[row] |= 1 << (6-(j*2));
				}
			}
		}
	}
}


void clock_dni(uint32_t ms) {
	//milliseconds to d'ni (24:24:24)
	uint8_t a = ms / 3456000;	// 1/25 day = 57 m 36 s
	ms -= 3456000 * (uint32_t) a;
	uint8_t b = ms / 138240;	// 1/625 day = 2 m 18.24 s
	ms -= 138240 * (uint32_t) b;
	ms *= 10;					// bump up the precision
	uint8_t c = ms / 55296;		// 1/15625 day ~= 5.5 s
	ms -= 55296 * (uint32_t) c;
	uint8_t d = ms / 2212;		// 1/390625 day ~= .22 s
	
	uint8_t digits[6];
	digits[1] = a;
	digits[0] = 0;
	while (digits[1] > 9) {
		digits[0] += 1;
		digits[1] -= 10;
	}
	
	digits[3] = b;
	digits[2] = 0;
	while (digits[3] > 9) {
		digits[2] += 1;
		digits[3] -= 10;
	}
	
	for(uint8_t i = 0; i < 4; i++) {
		_segments[i] = segment_decimal(digits[i]);
	}
	if (_segments[0] == 0) {
		_segments[0] = segment_character(' ');
	}
	
	// flash the colon
	if ((c & _BV(0)) == _BV(0)) {
		_segment_flags = _BV(0);
	} else {
		_segment_flags = 0x00;
	}
	
	if (d < 12) {
		for (uint8_t i = 0; i < 8; i++) {
			_matrix_red[i] = _b25[a][i];
			_matrix_grn[i] = 0x00;
		}
	} else {
		for (uint8_t i = 0; i < 8; i++) {
			_matrix_red[i] = 0x00;
			_matrix_grn[i] = _b25[b][i];
		}
	}
}


/*
 * This method was used by the ancient Mayans
 */
void clock_vigesimal(uint32_t ms) {
	uint8_t digits[4];
	//milliseconds to vigesimal (19.19.19.19)
	digits[0] = ms / 4320000;	// 1/20 day = 1 h 12 m
	ms -= 4320000 * (uint32_t) digits[0];
	digits[1] = ms / 216000;	// 1/400 day = 3 m 36 s
	ms -= 216000 * (uint32_t) digits[1];
	digits[2] = ms / 10800;		// 1/8000 day = 10.8 s
	ms -= 10800 * (uint32_t) digits[2];
	digits[3] = ms / 540;		// 1/160000 day = .54 s
	
	for (uint8_t i = 0; i < 4; i++) {
		if (digits[i] == 0 && (i == 0 || digits[i-1] == 0)) {
			_segments[i] = segment_character(' ');
		} else {
			_segments[i] = segment_vigesimal(digits[i]);
		}
	}
	_segment_flags = 0x00;
	
	clock_clear_matrix();

	for (uint8_t i = 0; i < 4; i++) { // segments (rows)
		// build a 4x4 mayan number square
	
		uint8_t v = digits[i];
		int row = 3;
		if (v > 4) row--;
		if (v > 9) row--;
		if (v > 14) row--;
		if (i > 1) row += 4; // mode c and d down four rows
	
		// draw the dots
		uint8_t sh = (i % 2 == 1) ? 4 : 0; // shift b and d over 4 cols
		if (v % 5 > 0) _matrix_red[row] |= (0x01 << sh);
		if (v % 5 > 1) _matrix_red[row] |= (0x02 << sh);
		if (v % 5 > 2) _matrix_red[row] |= (0x04 << sh);
		if (v % 5 > 3) _matrix_red[row] |= (0x08 << sh);
	
		// draw the lines
		if (v > 4) _matrix_grn[row+1] |= (0x0F << sh); 
		if (v > 9) _matrix_grn[row+2] |= (0x0F << sh); 
		if (v > 14) _matrix_grn[row+3] |= (0x0F << sh); 
	}
}

/*
 * This method was first proposed in the 1850s by John W. Nystrom
 */
void clock_hexadecimal(uint32_t ms) {
	uint8_t digits[4];
	//milliseconds to hexadecimal (F_FF_F)
	digits[0] = ms / 5400000;		// 1/16 day (hex hour) = 1 h 30 m
	ms -= 5400000 * (uint32_t) digits[0];
	digits[1] = ms / 337500;		// 1/256 day (hex maxime) = 5 m 37.5 s
	ms -= 337500 * (uint32_t) digits[1];
	ms *= 100;						// bump up the precision
	digits[2] = ms / 2109375;		// 1/4096 day (hex minute) ~= 21 seconds
	ms -= 2109375 * (uint32_t) digits[2];
	ms *= 100;						// bump up the precision again
	digits[3] = ms / 13183593;		// 1/65536 day (hex second) ~= 1.32 seconds
	
	_segments[3] = segment_hexadecimal(digits[2]);
	_segments[2] = segment_hexadecimal(digits[1]);
	_segments[0] = segment_hexadecimal(digits[0]);
	
	// flash segment 1
	if ((digits[3] & _BV(0)) == _BV(0)) {
		_segments[1] = segment_character('_');
	} else {
		_segments[1] = segment_character(' ');
	}
	
	_segment_flags = 0;
	
	clock_clear_matrix();

	// 2x2 pixels for each bit, hr on top, sc on bottom
	for (uint8_t i = 0; i < 4; i++) { // segments (rows)
		// build a 1x8 bar
		int v = digits[i];
		for (uint8_t j = 0; j < 4; j++) { // bits (cols)
			if ((v & _BV(j)) != 0) {
				if (i == 1 || i == 3) {
					_matrix_grn[(i*2)+0] |= 1 << (7-(j*2));
					_matrix_grn[(i*2)+1] |= 1 << (7-(j*2));
					_matrix_grn[(i*2)+0] |= 1 << (6-(j*2));
					_matrix_grn[(i*2)+1] |= 1 << (6-(j*2));
				} else {
					_matrix_red[(i*2)+0] |= 1 << (7-(j*2));
					_matrix_red[(i*2)+1] |= 1 << (7-(j*2));
					_matrix_red[(i*2)+0] |= 1 << (6-(j*2));
					_matrix_red[(i*2)+1] |= 1 << (6-(j*2));
				}
			}
		}
	}
}

void clock_dozenal(uint32_t ms) {
	uint8_t digits[5];
	//milliseconds to decimal (BBB.BB)
	digits[0] = ms / 7200000;	// 1/12 day = 2 h
	ms -= 7200000 * (uint32_t) digits[0];
	digits[1] = ms / 600000;	// 1/144 day = 10 m
	ms -= 600000 * (uint32_t) digits[1];
	digits[2] = ms / 50000;		// 1/1728 day = 50 s
	ms -= 50000 * (uint32_t) digits[2];
	digits[3] = ms / 4167;		// 1/20736 day ~= 4.167 s
	ms -= 4167 * (uint32_t) digits[3];
	digits[4] = ms / 347;		// 1/248832 day ~= .347 s
	
	for (uint8_t i = 0; i < 4; i++) {
		_segments[i] = segment_decimal(digits[i]);
	}
	if (digits[0] == 0) {
		_segments[0] = segment_character(' ');
		if (digits[1] == 0) {
			_segments[1] = segment_character(' ');
		}
	}
	
	// add the decimal place
	_segments[2] = segment_dp(_segments[2]);

	// flash the extra dot for digit 5
	if ((digits[4] & _BV(0)) == _BV(0)) {
		_segment_flags = _BV(1);
	} else {
		_segment_flags = 0x00;
	}
	
	clock_clear_matrix();

	// draws 3x3 dice patterns in each cornern
	// draws a flashing dot in the middle for centibeats
	for (uint8_t i = 0; i < 4; i++) { // segments (rows)
		uint8_t v = digits[i];
		int row = 0;
		if (i > 1) row += 4; // move down four rows
		if (i == 1 || i == 2) { // make green instead of red
			uint8_t sh = (i == 1) ? 4 : 0; // shift over three cols
			if (v == 0) {
				_matrix_grn[row+0] = (0x04 << sh);
			} else if (v == 1) {
				_matrix_grn[row+0] = (0x08 << sh);
			} else if (v == 2) {
				_matrix_grn[row+1] = (0x08 << sh);
			} else if (v == 3) {
				_matrix_grn[row+2] = (0x08 << sh);
			} else if (v == 4) {
				_matrix_grn[row+3] = (0x08 << sh);
			} else if (v == 5) {
				_matrix_grn[row+3] = (0x04 << sh);
			} else if (v == 6) {
				_matrix_grn[row+3] = (0x02 << sh);
			} else if (v == 7) {
				_matrix_grn[row+3] = (0x01 << sh);
			} else if (v == 8) {
				_matrix_grn[row+2] = (0x01 << sh);
			} else if (v == 9) {
				_matrix_grn[row+1] = (0x01 << sh);
			} else if (v == 10) {
				_matrix_grn[row+0] = (0x01 << sh);
			} else if (v == 11) {
				_matrix_grn[row+0] = (0x02 << sh);
			}
		} else {
			uint8_t sh = (i == 3) ? 4 : 0; // shift over 4 cols
			if (v == 0) {
				_matrix_red[row+0] = (0x04 << sh);
			} else if (v == 1) {
				_matrix_red[row+0] = (0x08 << sh);
			} else if (v == 2) {
				_matrix_red[row+1] = (0x08 << sh);
			} else if (v == 3) {
				_matrix_red[row+2] = (0x08 << sh);
			} else if (v == 4) {
				_matrix_red[row+3] = (0x08 << sh);
			} else if (v == 5) {
				_matrix_red[row+3] = (0x04 << sh);
			} else if (v == 6) {
				_matrix_red[row+3] = (0x02 << sh);
			} else if (v == 7) {
				_matrix_red[row+3] = (0x01 << sh);
			} else if (v == 8) {
				_matrix_red[row+2] = (0x01 << sh);
			} else if (v == 9) {
				_matrix_red[row+1] = (0x01 << sh);
			} else if (v == 10) {
				_matrix_red[row+0] = (0x01 << sh);
			} else if (v == 11) {
				_matrix_red[row+0] = (0x02 << sh);
			}
		}
	}
}

/*
 * This method was introduced during the French Revolution in 1793
 * It's also equivalent to Swatch time
 */
void clock_decimal(uint32_t ms) {
	uint8_t digits[5];
	//milliseconds to decimal (999.99)
	digits[0] = ms / 8640000;	// 1/10 day (deciday) = 2 h 24 m
	ms -= 8640000 * (uint32_t) digits[0];
	digits[1] = ms / 864000;	// 1/100 day (centiday) = 14 m 24 s
	ms -= 864000 * (uint32_t) digits[1];
	digits[2] = ms / 86400;		// 1/1000 day (milliday; beat) = 1 m 26.4 s
	ms -= 86400 * (uint32_t) digits[2];
	digits[3] = ms / 8640;		// 1/10000 day (decibeat)= 8.64 s
	ms -= 8640 * (uint32_t) digits[3];
	digits[4] = ms / 864;		// 1/100000 day (centibeat) / .864 s
	
	for (uint8_t i = 0; i < 4; i++) {
		_segments[i] = segment_decimal(digits[i]);
	}
	if (digits[0] == 0) {
		_segments[0] = segment_character(' ');
		if (digits[1] == 0) {
			_segments[1] = segment_character(' ');
		}
	}
	
	// add the decimal place
	_segments[2] = segment_dp(_segments[2]);

	// flash the extra dot for digit 5
	if ((digits[4] & _BV(0)) == _BV(0)) {
		_segment_flags = _BV(1);
	} else {
		_segment_flags = 0x00;
	}	
	
	clock_clear_matrix();

	// draws 3x3 dice patterns in each cornern
	// draws a flashing dot in the middle for centibeats
	for (uint8_t i = 0; i < 4; i++) { // segments (rows)
		uint8_t v = digits[i];
		int row = 2;
		if (i > 1) row += 5; // move md and ud down five rows
		if (i == 1 || i == 2) { // make cd and md green instead of red
			uint8_t sh = (i == 1) ? 5 : 0; // shift cd over three cols
			if (v == 1) {
				_matrix_grn[row-1] = (0x02 << sh);
			} else if (v == 2) {
				_matrix_grn[row-0] = (0x01 << sh);
				_matrix_grn[row-2] = (0x04 << sh);
			} else if (v == 3) {
				_matrix_grn[row-0] = (0x01 << sh);
				_matrix_grn[row-1] = (0x02 << sh);
				_matrix_grn[row-2] = (0x04 << sh);
			} else if (v == 4) {
				_matrix_grn[row-0] = (0x05 << sh);
				_matrix_grn[row-2] = (0x05 << sh);
			} else if (v == 5) {
				_matrix_grn[row-0] = (0x05 << sh);
				_matrix_grn[row-1] = (0x02 << sh);
				_matrix_grn[row-2] = (0x05 << sh);
			} else if (v == 6) {
				_matrix_grn[row-0] = (0x05 << sh);
				_matrix_grn[row-1] = (0x05 << sh);
				_matrix_grn[row-2] = (0x05 << sh);
			} else if (v == 7) {
				_matrix_grn[row-0] = (0x05 << sh);
				_matrix_grn[row-1] = (0x07 << sh);
				_matrix_grn[row-2] = (0x05 << sh);
			} else if (v == 8) {
				_matrix_grn[row-0] = (0x07 << sh);
				_matrix_grn[row-1] = (0x05 << sh);
				_matrix_grn[row-2] = (0x07 << sh);
			} else if (v == 9) {
				_matrix_grn[row-0] = (0x07 << sh);
				_matrix_grn[row-1] = (0x07 << sh);
				_matrix_grn[row-2] = (0x07 << sh);
			}
		} else {
			uint8_t sh = (i == 3) ? 5 : 0; // shift ud over three cols
			if (v == 1) {
				_matrix_red[row-1] = (0x02 << sh);
			} else if (v == 2) {
				_matrix_red[row-0] = (0x01 << sh);
				_matrix_red[row-2] = (0x04 << sh);
			} else if (v == 3) {
				_matrix_red[row-0] = (0x01 << sh);
				_matrix_red[row-1] = (0x02 << sh);
				_matrix_red[row-2] = (0x04 << sh);
			} else if (v == 4) {
				_matrix_red[row-0] = (0x05 << sh);
				_matrix_red[row-2] = (0x05 << sh);
			} else if (v == 5) {
				_matrix_red[row-0] = (0x05 << sh);
				_matrix_red[row-1] = (0x02 << sh);
				_matrix_red[row-2] = (0x05 << sh);
			} else if (v == 6) {
				_matrix_red[row-0] = (0x05 << sh);
				_matrix_red[row-1] = (0x05 << sh);
				_matrix_red[row-2] = (0x05 << sh);
			} else if (v == 7) {
				_matrix_red[row-0] = (0x05 << sh);
				_matrix_red[row-1] = (0x07 << sh);
				_matrix_red[row-2] = (0x05 << sh);
			} else if (v == 8) {
				_matrix_red[row-0] = (0x07 << sh);
				_matrix_red[row-1] = (0x05 << sh);
				_matrix_red[row-2] = (0x07 << sh);
			} else if (v == 9) {
				_matrix_red[row-0] = (0x07 << sh);
				_matrix_red[row-1] = (0x07 << sh);
				_matrix_red[row-2] = (0x07 << sh);
			}
		}
	}

	uint8_t nd = digits[4];
	if (nd == 1 || nd == 4 || nd == 7 || nd == 3 || nd == 6 || nd == 9) {
		_matrix_red[3] = 0x18;
		_matrix_red[4] = 0x18;
	}
	if (nd == 2 || nd == 5 || nd == 8 || nd == 3 || nd == 6 || nd == 9) {
		_matrix_grn[3] = 0x18;
		_matrix_grn[4] = 0x18;
	}
}

void clock_octal(uint32_t ms) {
	//milliseconds to octal (777.777)
	uint8_t digits[6];
	digits[0] = ms / 10800000;		// 1/8 day = 3 h
	ms -= 10800000 * (uint32_t) digits[0] ;
	digits[1] = ms / 1350000;		// 1/64 day = 22 m 30 s
	ms -= 1350000 * (uint32_t) digits[1];
	digits[2] = ms / 168750;		// 1/512 day ~= 2 m 49 s
	ms -= 168750 * (uint32_t) digits[2];
	ms *= 100;						// bump up the precision
	digits[3] = ms / 2109375;		// 1/4096 day ~= 21 s
	ms -= 2109375 * (uint32_t) digits[3];
	ms *= 100;						// bump up the precision again
	digits[4] = ms / 26367187;		// 1/32768 day ~= 2.63 s
	ms -= 26367187 * (uint32_t) digits[4];
	ms *= 100;						// bump up the precision again
	digits[5] = ms / 329589843;		// 1/262144 day ~= .329 s

	for (uint8_t i = 0; i < 4; i++) {
		_segments[i] = segment_decimal(digits[i]);
	}
	if (digits[0] == 0) {
		_segments[0] = segment_character(' ');
		if (digits[1] == 0) {
			_segments[1] = segment_character(' ');
		}
	}
	
	// add the decimal place
	_segments[2] = segment_dp(_segments[2]);

	// flash the extra dot for digit 5
	if ((digits[4] & _BV(0)) == _BV(0)) {
		_segment_flags = _BV(1);
	} else {
		_segment_flags = 0x00;
	}

	clock_clear_matrix();

	// 1x2 pixels for each bit, a on top, f on bottom
	for (uint8_t i = 0; i < 6; i++) { // segments (rows)
		// build a 1x6 bar
		uint8_t v = _segments[i];
		for (uint8_t j = 0; j < 3; j++) { // bits (cols)
			if ((v & _BV(j)) != 0) {
				if (i == 1 || i == 3 || i == 5) {
					_matrix_grn[i+1] |= 2 << (5-(j*2));
					_matrix_grn[i+1] |= 2 << (4-(j*2));
				} else {
					_matrix_red[i+1] |= 2 << (5-(j*2));
					_matrix_red[i+1] |= 2 << (4-(j*2));
				}
			}
		}
	}
}

void clock_update(uint32_t ms) {
	switch (_mode) {
		case 0: clock_traditional(ms); break;
		case 1: clock_vigesimal(ms); break;
		case 2: clock_hexadecimal(ms); break;
		case 3: clock_dozenal(ms); break;
		case 4: clock_decimal(ms); break;
		case 5: clock_octal(ms); break;
		case 6: clock_dni(ms); break;
	}
}

uint32_t clock_size_b() {
	switch (_mode) {
		case 0: return 3600000;  // traditional 1/24 day = 1 h
		case 1: return 216000;   // vigesimal 1/20 day = 1 h 12 m
		case 2: return 337500;   // hexadecimal 1/256 day = 1 h 30 m
		case 3: return 600000;   // dozenal 1/144 day = 10 m
		case 4: return 864000;   // decimal 1/100 day = 14 m 24 s
		case 5: return 1350000;  // octal 1/64 day = 22 m 30 s
		case 6: return 138240;   // d'ni 1/625 day = 2 m 18.24 s
	}
	return 0;
}

uint32_t clock_size_d() {
	switch(_mode) {
		case 0: return 60000;   // traditional 1/1440 day = 1 m
		case 1: return 540;     // vigesimal 1/160000 day = .54 s
		case 2: return 1318;    // hexadecimal 1/65536 day ~= 1.32 seconds
		case 3: return 4167;    // dozenal 1/20736 day ~= 4.167 s
		case 4: return 8640;    // decimal 1/10000 day = 8.64 s
		case 5: return 21094;   // octal 1/4096 day ~= 21 s
		case 6: return 2212;    // d'ni 1/390625 day ~= .22 s
	}
	return 0;
}
