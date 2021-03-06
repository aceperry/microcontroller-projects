#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lib/Hd44780/Hd44780_Direct.h"
#include "lib/usb/rawhid.h"

using namespace digitalcave;

#define BACKLIGHT_PORT					PORTB
#define BACKLIGHT_DDR					DDRB
#define BACKLIGHT_PIN					0
#define TIMEOUT_MILLIS					2000

#define STATE_SPLASH					0
#define STATE_DISPLAY					1
#define STATE_SLEEP						2
//The state machine goes as follows: After USB enumeration, start in splash screen.
//When you receive a message from the client software (in any state), go to display and show the message.
//When at display, if you do not receive a message for TIMEOUT seconds, go back to splash screen
//When at splash screen, if you do not receive a message for TIMEOUT seconds, turn display off.

static uint8_t rx_buffer[64];
static volatile uint16_t timeout_millis = 0;
static volatile uint8_t state;
static Hd44780_Direct display(display.FUNCTION_LINE_2 | display.FUNCTION_SIZE_5x8, &PORTB, 1, &PORTB, 2, &PORTD, 4, &PORTD, 5, &PORTD, 6, &PORTD, 7);

void initUsb(){
	// Initialize the USB, and then wait for the host to set configuration.
	// If the AVR is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured());

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);
}

void initDisplay(){
	uint8_t custom[64] = {
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,			// 0 (Use 0xFE)
		0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1f,			// 1
		0x0,0x0,0x0,0x0,0x0,0x0,0x1f,0x1f,			// 2
		0x0,0x0,0x0,0x0,0x0,0x1f,0x1f,0x1f,			// 3
		0x0,0x0,0x0,0x0,0x1f,0x1f,0x1f,0x1f,		// 4
		0x0,0x0,0x0,0x1f,0x1f,0x1f,0x1f,0x1f,		// 5
		0x0,0x0,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,		// 6
		0x0,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f		// 7
													// 8 (Use 0xFF)
	};
	display.set_cgram_address(0x00);
	_delay_ms(64);
	display.write_bytes(custom, 64); }

void initTimer(){
	TCCR0A = 0x0;
	TCCR0B |= _BV(CS02);
	OCR0A = F_CPU / 256 / 1000;		//Fire every millisecond
	TIMSK0 = _BV(OCIE0A);
	sei();
}

void doSplash(){
	state = STATE_SPLASH;
	timeout_millis = 0;
	
	display.set_ddram_address(0x00);
	display.write_bytes((char*) "  Wyatt  Olson  ", 16);
	display.set_ddram_address(0x40);
	display.write_bytes((char*) " digitalcave.ca ", 16);
}

void doDisplay(){
	BACKLIGHT_PORT |= _BV(BACKLIGHT_PIN);
	
	state = STATE_DISPLAY;
	timeout_millis = 0;
	
	display.set_ddram_address(0x00);
	display.write_bytes((char*) &rx_buffer[1], 16);
	display.set_ddram_address(0x40);
	display.write_bytes((char*) &rx_buffer[17], 16);
}

void doSleep(){
	BACKLIGHT_PORT &= ~_BV(BACKLIGHT_PIN);
	
	display.set_ddram_address(0x00);
	display.write_bytes((char*) "                ", 16);
	display.set_ddram_address(0x40);
	display.write_bytes((char*) "                ", 16);
}

int main (void){
	initUsb();
	initDisplay();
	initTimer();
	BACKLIGHT_DDR |= _BV(BACKLIGHT_PIN);
	
	doSplash();
	
	while (1) {
		// if received data, do something with it
		uint8_t count = usb_rawhid_recv(rx_buffer, 0);
		if (count >= 33 && rx_buffer[0] == 0x01) {
			doDisplay();
		}
		
		if (state != STATE_SLEEP && timeout_millis > TIMEOUT_MILLIS){
			if (state == STATE_DISPLAY){
				doSplash();
			}
			else if (state == STATE_SPLASH){
				doSleep();
			}
		}
	}
}

EMPTY_INTERRUPT(TIMER0_COMPB_vect)
EMPTY_INTERRUPT(TIMER0_OVF_vect)
ISR(TIMER0_COMPA_vect){
	timeout_millis++;
}
