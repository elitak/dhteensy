#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
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
#define MODE_SHIFTED 4

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t hex_keys[16]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};


uint16_t idle_count=0;

#define KEYS_DOWN_MAX 16
#define MODE_TRACK_MAX 16

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

int compare (const void * a, const void * b)
{
    return ( *(uint8_t*)a - *(uint8_t*)b );
}

int get_modes(uint8_t *keys_down, uint8_t keys_down_n) {
    int modes = MODE_NORMAL; // = 0

    for (int i=0; i<keys_down_n; i++) { // go through each key that's currently depressed
        int keyid = keys_down[i]; // keyid is the index into our keymapping table
        uint16_t keycode = pgm_read_word(normal_keys + keyid); // lookup the keycode in the "normal" layer, since we're only interested in metakeys.

        // XXX really either NAS or FN can be active at one time (flags interpreted later)
        if      (keycode == KEY_DH_NAS) { modes |= MODE_NAS; }
        else if (keycode == KEY_DH_FN) { modes |= MODE_FN; }
        else if (keycode == KEY_DH_SHIFT) { modes |= MODE_SHIFTED; }

    }
    return modes;
}

int get_modifiers(uint8_t *keys_down, uint8_t keys_down_n) {
    int mods = 0;

    for (int i=0; i<keys_down_n; i++) {
        int keyid = keys_down[i];
        uint16_t keycode = pgm_read_word(normal_keys + keyid);

        if (keycode == KEY_DH_SHIFT) { mods |= KEY_SHIFT; }
        else if (keycode == KEY_DH_CTRL) { mods |= KEY_CTRL; }
        else if (keycode == KEY_DH_ALT) { mods |= KEY_ALT; }
        else if (keycode == 0xe3 //LeftGUI
                 || keycode == 0xe7 //RightGUI
                 //|| keycode == KEY_DH_NORM:
                 ) {
            mods |= KEY_GUI;
        }
    }
    return mods;
}

// modal keys that should not count towards number depressed (for purposes of detecting change between polls)
int get_discounted(uint8_t *keys_down, uint8_t keys_down_n) {
    int discounted = 0;

    for (int i=0; i<keys_down_n; i++) {
        int keyid = keys_down[i];
        uint16_t keycode = pgm_read_word(normal_keys + keyid);

        switch (keycode) {
            case KEY_DH_FN:
            case KEY_DH_NAS:
            case KEY_DH_CTRL: // had a bug before these were added: when using (ctrl+)alt+tab, would keep sending 'tab'
            case KEY_DH_ALT:  // 
            // TODO: does shift need to be here?
                discounted++;
        }
    }
    return discounted;
}


// TODO
// * toggle mode in levers
// * winkey off of lever
// * naslock key should go to 10k while pressed (right now does nothing). Give consideration to hexadecimal in that mode.
// * capslock should be moved away and that key used as another metakey
// * rework NAS layer? right now typing eg (''),; is way too hard
// * mouse mode
// * reset button:
//    - test to see if the teensy can reset itself by writing 0 to a pin connected to the reset
//    - look into two reset modes (to program vs. restart code), using the external button to program would be nice.
//    - once thats all done maybe code in different uses for the reset button based on press length and also use leds as feedback

uint8_t process_keys(uint8_t *keys_down, uint8_t keys_down_n) {
    uint8_t keyid, i, nkeys=0, is_shifted;
    uint16_t keycode;
    uint8_t dh_keyboard_keys[6]={0,0,0,0,0,0};
    int8_t auto_shift=0;
    int8_t no_auto_shift=0;
    uint8_t kmode;
    uint8_t something_new = 0;
    static uint16_t key_track[KEYS_DOWN_MAX];
    static uint8_t num_down_last = 0;
    uint16_t mode_track[MODE_TRACK_MAX];
    uint8_t mode_track_n = 0;
    static uint16_t mode_track_last[MODE_TRACK_MAX];
    static uint8_t mode_track_last_n = 0;
    uint8_t discounted = get_discounted(keys_down, keys_down_n);
    uint8_t dh_keyboard_modifier_keys = get_modifiers(keys_down, keys_down_n);
    uint8_t mode = get_modes(keys_down, keys_down_n);

    // canonicalize the list of keys down by sorting it
    qsort(keys_down, keys_down_n, sizeof(uint8_t), compare);

    // Always at least set the mode-LEDs, before returning
    if (mode & MODE_FN) {
        set_led(LED_FN);
    } else if (mode & MODE_NAS) {
        set_led(LED_NAS);
    } else {
        set_led(LED_NORM);
    }

    // Detect whether any actionable changes have occurred since previous invocation,
    // and save the keydown state across the whole board.
    for (i=0; i<keys_down_n; i++) {
        if (keys_down[i] != key_track[i]) something_new = 1;
        key_track[i] = keys_down[i];
    }
    if (num_down_last != keys_down_n - discounted) something_new = 1;
    num_down_last = keys_down_n - discounted;

    // Return early if no action needed.
    if (keys_down_n == discounted) {
        //reset key tracker in case same key is pressed next time
        key_track[0] = 0xff; // XXX 0xff is an unused keyid
    } else if (!something_new) {
        // return if nothing (besides maybe metakeys) has changed
        // and if no keys are down at all (in that case we need to unpress the last keys)
        return 0;
    }

    // Now actually process the keys that are down
    for (i=0; i<keys_down_n; i++) {
        keycode = 0;
        keyid = keys_down[i];
        is_shifted = 0;

        kmode = mode; // the default mode of each key is what's currently depressed.
        // Mode track records (modeflags)(keybyte) as short, in the order they get scanned.
        for (uint8_t j=0; j<mode_track_last_n; j++) {
            // Test each's lower byte for equality to see if only the mode changed.
            if (keyid == (mode_track_last[j] & 0xff)) {
                kmode = mode_track_last[j] >> 8; // if key was down previously use its original mode
            }
        }
        mode_track[mode_track_n++] = kmode << 8 | keyid; // record current mode

        if (kmode & MODE_FN) {
            if (kmode & MODE_SHIFTED) keycode = pgm_read_word(fnS_keys+keyid);
            if (!keycode) keycode = pgm_read_word(fn_keys+keyid); else is_shifted = 1;
        } else if (kmode & MODE_NAS) {
            if (kmode & MODE_SHIFTED) keycode = pgm_read_word(nasS_keys+keyid);
            if (!keycode) keycode = pgm_read_word(nas_keys+keyid); else is_shifted = 1;
        } else {
            if (kmode & MODE_SHIFTED) keycode = pgm_read_word(normalS_keys+keyid);
            if (!keycode) keycode = pgm_read_word(normal_keys+keyid); else is_shifted = 1;
        }

        if (keycode & 0x80) continue; // f0-ff are special, already handled

        // ninth LSBit of 16-bit keycode means "shifted"
        // keep a count of auto-shifted vs unshifted keys
        if (keycode & 1<<8)
            auto_shift++;
        else if (is_shifted)
            // only count unshifted that are pressed on shifted layers
            no_auto_shift++;

        keycode &= 0xff; // mask to single byte

        if(nkeys>5) break;
        dh_keyboard_keys[nkeys]=keycode;
        nkeys++;
    } // end second pass

    // save mode state
    for (int i = 0; i<MODE_TRACK_MAX; i++)
        mode_track_last[i] = mode_track[i];
    mode_track_last_n = mode_track_n;

    if (no_auto_shift>0 && mode & MODE_SHIFTED) {
        if (auto_shift>0) return 0; 
        // if we pressed an unshifted key from a shifted layer, don't include the shift modifier itself.
        dh_keyboard_modifier_keys ^= KEY_SHIFT;
    // we have some auto-shift keys down
    } else if (auto_shift>0) {
        if (no_auto_shift>0) return 0; 
        dh_keyboard_modifier_keys |= KEY_SHIFT;
    }

    return usb_keyboard_send(dh_keyboard_keys, dh_keyboard_modifier_keys);
}

uint8_t key_down(uint8_t key, uint8_t *keys_down, uint8_t *keys_down_n) {
        // XXX TODO
        // (I highly doubt this will ever be an issue even in GAMES, since most
        // bindings use keys on the normal layer and reserve shift/ctrl/alt as
        // raw metakeys)
        // This exhibits weird (corner case) behavior. e.g., can't repeat
        // shifted+unshifted chars while both pressed (e.g., {]). this is not
        // really an issue as repeating groups of more than one character is
        // unneeded, except maybe in GAMES This behavior only triggers when all
        // keys are pressed during the same 10ms window anyway, and reorders
        // the keypresses into the order they were scanned... what do 'normal'
        // keyboards even do in these cases?
	if(*keys_down_n>=15) 
		return -1;
	keys_down[*keys_down_n] = key;
	(*keys_down_n)++;
	return 0;
}

int main(void)
{
	uint8_t i, b, selector;
        uint8_t keys_down[KEYS_DOWN_MAX];
        uint8_t keys_down_n;

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
					key_down((selector << 2) +i, keys_down, &keys_down_n);
		}
		process_keys(keys_down, keys_down_n);
                if (rst_read()) {debug_dump();}
		_delay_ms(10);
	}
}

/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
