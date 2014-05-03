/* Keyboard example with debug channel, for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// These are masks. Unset bits are illuminated, so you'll likely want to use
// the 1's complement of these values.
#define LED_NAS  0x01
#define LED_NORM 0x02
#define LED_FN   0x04
#define LED_10K  0x08

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard_debug.h"
#include "print.h"

#include "dhkeys.h"
#include "keymaps-elitak.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t hex_keys[16]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};

#define MODE_NORMAL 0
#define MODE_NAS 1
#define MODE_FN 2

uint16_t idle_count=0;

// duplicated this so that we can safely change it without worrying about interrupts
uint8_t keys_down[16];
uint8_t keys_down_n;
int lastsum;
#define MODE_TRACK_MAX 16
uint8_t mode_track_last[MODE_TRACK_MAX];
uint8_t mode_track_last_n=0;

void reload(void);

void port0_init(uint8_t mask) {
        DDRB = (DDRB & ~(1<<6)) | (!!(mask & 1<<0) << 6);
        DDRB = (DDRB & ~(1<<5)) | (!!(mask & 1<<1) << 5);
        DDRB = (DDRB & ~(1<<4)) | (!!(mask & 1<<2) << 4);
        DDRB = (DDRB & ~(1<<3)) | (!!(mask & 1<<3) << 3);
        DDRB = (DDRB & ~(1<<2)) | (!!(mask & 1<<4) << 2);
        DDRB = (DDRB & ~(1<<1)) | (!!(mask & 1<<5) << 1);
        DDRB = (DDRB & ~(1<<0)) | (!!(mask & 1<<6) << 0);
        DDRE = (DDRE & ~(1<<7)) | (!!(mask & 1<<7) << 7);
}

void port0_write(uint8_t data) {
        PORTB = (PORTB & ~(1<<6)) | (!!(data & 1<<0) << 6);
        PORTB = (PORTB & ~(1<<5)) | (!!(data & 1<<1) << 5);
        PORTB = (PORTB & ~(1<<4)) | (!!(data & 1<<2) << 4);
        PORTB = (PORTB & ~(1<<3)) | (!!(data & 1<<3) << 3);
        PORTB = (PORTB & ~(1<<2)) | (!!(data & 1<<4) << 2);
        PORTB = (PORTB & ~(1<<1)) | (!!(data & 1<<5) << 1);
        PORTB = (PORTB & ~(1<<0)) | (!!(data & 1<<6) << 0);
        PORTE = (PORTE & ~(1<<7)) | (!!(data & 1<<7) << 7);
}

void port1_init(uint8_t mask) {
        // PE0, PB7, PD0-6
        DDRE = (DDRE & ~(1<<0)) | (!!(mask & 1<<0) << 0);
        DDRB = (DDRB & ~(1<<7)) | (!!(mask & 1<<1) << 7);
        DDRD = (DDRD & ~(1<<0)) | (!!(mask & 1<<2) << 0);
        DDRD = (DDRD & ~(1<<1)) | (!!(mask & 1<<3) << 1);
        DDRD = (DDRD & ~(1<<2)) | (!!(mask & 1<<4) << 2);
        DDRD = (DDRD & ~(1<<3)) | (!!(mask & 1<<5) << 3);
        DDRD = (DDRD & ~(1<<4)) | (!!(mask & 1<<6) << 4);
        DDRD = (DDRD & ~(1<<5)) | (!!(mask & 1<<7) << 5);
}

void port1_write(uint8_t data) {
        PORTE = (PORTE & ~(1<<0)) | (!!(data & 1<<0) << 0);
        PORTB = (PORTB & ~(1<<7)) | (!!(data & 1<<1) << 7);
        PORTD = (PORTD & ~(1<<0)) | (!!(data & 1<<2) << 0);
        PORTD = (PORTD & ~(1<<1)) | (!!(data & 1<<3) << 1);
        PORTD = (PORTD & ~(1<<2)) | (!!(data & 1<<4) << 2);
        PORTD = (PORTD & ~(1<<3)) | (!!(data & 1<<5) << 3);
        PORTD = (PORTD & ~(1<<4)) | (!!(data & 1<<6) << 4);
        PORTD = (PORTD & ~(1<<5)) | (!!(data & 1<<7) << 5);
}

uint8_t port1_read(void) {
        uint8_t acc = 0;
        acc |= !!(PIND & 1<<5); acc <<= 1;
        acc |= !!(PIND & 1<<4); acc <<= 1;
        acc |= !!(PIND & 1<<3); acc <<= 1;
        acc |= !!(PIND & 1<<2); acc <<= 1;
        acc |= !!(PIND & 1<<1); acc <<= 1;
        acc |= !!(PIND & 1<<0); acc <<= 1;
        acc |= !!(PINB & 1<<7); acc <<= 1;
        acc |= !!(PINE & 1<<0);
        return acc;
}

void port2_init(uint8_t mask) {
        // same pins but reverse order
        DDRF = 0;
        for (uint8_t i = 0; i < 8; i++) {
            DDRF <<= 1;
            DDRF |= mask>>i & 1;
        }
}

void port2_write(uint8_t data) {
        // same pins but reverse order
        PORTF = 0;
        for (uint8_t i = 0; i < 8; i++) {
            PORTF <<= 1;
            PORTF |= data>>i & 1;
        }
}

void init_ports(void) {
        port0_init(0xFF); // right LEDs.
        port1_init(0x0F); // keys. lower half is the selector (write); upper is used to read that set (read)
        port2_init(0xFF); // left LEDs.
        DIDR0 = 0x00;
        DIDR1 = 0x00;
}

void set_selector(uint8_t selector) {
        port1_write(selector & 0x0F);
}

uint8_t read_keys(void) {
        return port1_read() >> 4;
}

uint8_t scan_line(uint8_t selector) {
	set_selector(selector);
	_delay_us(100);
	return(read_keys());
}

void set_led(uint8_t led) {
        port0_write(led);
        port2_write(0x00);
}

uint8_t process_keys(void) {

	uint8_t k, i, keycode=0,nkeys=0;
	uint8_t dh_keyboard_modifier_keys=0;
	uint8_t dh_keyboard_keys[6]={0,0,0,0,0,0};
	uint8_t reload_flag=0;
	int8_t auto_shift=0;
	int8_t no_auto_shift=0;
	uint8_t mode=MODE_NORMAL;
	uint8_t kmode;
	int sum=0;
	uint8_t mode_track_n=0;
	uint8_t mode_track[MODE_TRACK_MAX];
	

	// first pass for special keys
	for (i=0; i<keys_down_n; i++) {
		k = keys_down[i];
		keycode=pgm_read_byte(normal_keys+k);

		// this is an ugly hack to avoid sending new events if
		// the only thing that has changed is the FN or NAS state

		if (keycode != KEY_DH_FN && keycode != KEY_DH_NAS)
			sum += keycode;

		switch(keycode) {
		case KEY_DH_NAS:
			mode=MODE_NAS;
			break;
		case KEY_DH_FN:
			mode=MODE_FN;
			reload_flag++;
			break;
		case KEY_DH_SHIFT:
			dh_keyboard_modifier_keys |= KEY_SHIFT;
			break;
		case KEY_DH_CTRL:
			dh_keyboard_modifier_keys |= KEY_CTRL;
			break;
		case KEY_DH_ALT:
			dh_keyboard_modifier_keys |= KEY_ALT;
			break;
		case KEY_DH_NORM:
			dh_keyboard_modifier_keys |= KEY_GUI; // Super
			reload_flag++;
			break;
		}
	}

	// set mode LEDs
	switch(mode) {
	case MODE_NORMAL:
		set_led(~LED_NORM);
		break;
	case MODE_NAS:
		set_led(~LED_NAS);
		break;
	case MODE_FN:
		set_led(~LED_FN);
		break;
	}

	if (sum == lastsum) // return if nothing has changed
		return 0;
	lastsum=sum;

	// second pass for the rest

	for (i=0; i<keys_down_n; i++) {
		keycode=0;
		k=keys_down[i];
		if (k==35) reload_flag++;

		kmode=mode;
		for (uint8_t j=0;j<mode_track_last_n;j++) {
			if (k == (mode_track_last[j] & 0x3F)) { // lower 6 bits
				kmode=(mode_track_last[j] >> 6); // if key was down previously use its original mode
			}
		}

		mode_track[mode_track_n++]=(kmode<<6)+k; // record current mode

		switch(kmode) {
		case MODE_NORMAL:
			keycode = pgm_read_byte(normal_keys+k);
			break;
		case MODE_NAS:
			keycode = pgm_read_byte(nas_keys+k);
			break;
		case MODE_FN:
			keycode = pgm_read_byte(fn_keys+k);
			break;
		}


		if (keycode>=0xF0) continue; // special, already handled

		// high bit set means shifted
		// keep a count of auto-shifted vs unshifted keys
		if ((keycode & (1<<7)))
			auto_shift++;
		else
			no_auto_shift++;

		keycode &= 0x7f; // zero high bit

		if(nkeys>5) break;
		dh_keyboard_keys[nkeys]=keycode;
		nkeys++;
	}

	if (reload_flag>2) reload();

	for (i=0;i<mode_track_n;i++) {
		mode_track_last[i]=mode_track[i];
	}
	mode_track_last_n = mode_track_n;

	// we have some auto-shift keys down
	if (auto_shift>0) {
		// don't update if we have both auto-shift and non-auto-shift keys
		if (no_auto_shift>0)
			return 0; 
		dh_keyboard_modifier_keys |= KEY_SHIFT;
	}

	for (i=0; i<6; i++) {
		keyboard_keys[i]=dh_keyboard_keys[i];
	}
	keyboard_modifier_keys=dh_keyboard_modifier_keys;
	return usb_keyboard_send();
}

uint8_t key_down(uint8_t key) {
	if(keys_down_n>=15) 
		return -1;
	keys_down[keys_down_n]=key;
	keys_down_n++;
	return 0;
}

int main(void)
{
	uint8_t i, b, selector;

	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// init ports
	init_ports();

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.

	usb_init();
	while (!usb_configured()) /* wait */ ;

	set_led(~LED_NORM);

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	/*
	for (i=0; i<MODE_TRACK_MAX; i++) {
		mode_track_last[i]=0;
	}
	*/
	while (1) {
		// zero out dh kbd buffer
		for (i=0; i<16; i++) {
			keys_down[i]=0;
		}
		keys_down_n=0;

		for (selector=0; selector<14; selector++) {
			b=scan_line(selector);
			for(i=0;i<4; i++) 
				if(b & (1<<i))
					key_down((selector << 2) +i);
		}
		process_keys();
		_delay_ms(10);
	}
}

/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
