#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard_debug.h"
#include "print.h"
#include "ports.h"
#include "debug.h"

#include "dhkeys.h"
#include "keymaps-elitak.h"


// These are masks. Unset bits are illuminated, so you'll likely want to use
// the 1's complement of these values.
#define LED_NAS  0x01
#define LED_NORM 0x02
#define LED_FN   0x04
#define LED_10K  0x08
// These are shifted over four bits even though they're on the lower bits of a
// different port. This is an abstraction handled by set_led().
#define LED_SCROLL 0x10
#define LED_NUM 0x20
#define LED_MOUSE 0x40
#define LED_CAPS 0x80

#define MODE_NORMAL 0
#define MODE_NAS 1
#define MODE_FN 2

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t hex_keys[16]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};


uint16_t idle_count=0;

// duplicated this so that we can safely change it without worrying about interrupts
uint8_t keys_down[16];
uint8_t keys_down_n;
int lastsum;
#define MODE_TRACK_MAX 16
uint8_t mode_track_last[MODE_TRACK_MAX];
uint8_t mode_track_last_n=0;

void init_ports(void) {
        port0_init(0x0F); // right LEDs.
        port1_init(0x0F); // keys. lower half is the selector (write); upper is used to read that set (read)
        port2_init(0x0F); // left LEDs.
        rst_init(); //reset button on left
}

void set_led(uint8_t led) {
        // keyboard_leds is state pulled from the host, not maintained by us in this file.
        led |= keyboard_leds & 0x02 ? LED_CAPS : 0; // TODO ENUM these flags in usb_keyboard_debug.c
        led |= keyboard_leds & 0x01 ? LED_NUM : 0;
        led |= keyboard_leds & 0x04 ? LED_SCROLL : 0;
        port0_write(~(led & 0x0F));
        port2_write(~(led>>4 & 0x0F));
}

// TODO
// * toggle mode in levers
// * winkey off of lever
// * naslock key should go to 10k while pressed (right now does nothing). Give consideration to hexadecimal in that mode.
// * capslock should be moved away and that key used as another metakey
// * rework NAS layer? right now typing eg (''),; is way too hard
// * better abstract "autoshifting". |0x80 is messy and just wrong since it conflates the US layout with USB key codes
// * mouse mode
// * reset button hardwired to teensy if possible:
//    - have 1 key dump wire states to see if its in the selector operated pins or a separate dedicated pin.
//    - test to see if the teensy can reset itself by writing 0 to a pin connected to the reset
//    - look into two reset modes (to program vs. restart code)
//    - once thats all done maybe code in different uses for ther eset button based on press length and also use leds as feedback


uint8_t process_keys(void) {

    uint8_t k, i, keycode=0,nkeys=0;
    uint8_t dh_keyboard_modifier_keys=0;
    uint8_t dh_keyboard_keys[6]={0,0,0,0,0,0};
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
                break;
        }
    }

    // set mode LEDs
    switch(mode) {
        case MODE_NORMAL:
            set_led(LED_NORM);
            break;
        case MODE_NAS:
            set_led(LED_NAS);
            break;
        case MODE_FN:
            set_led(LED_FN);
            break;
    }

    if (sum == lastsum) // return if nothing has changed
        return 0;
    lastsum=sum;

    // second pass for the rest

    for (i=0; i<keys_down_n; i++) {
        keycode=0;
        k=keys_down[i];

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

	set_led(LED_NORM);

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
                if (rst_read()) {debug_dump();}
		_delay_ms(10);
	}
}

/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
