#include "ports.h"
#include <util/delay.h>

void port0_init(uint8_t src) {
    REMAP_PINS(
        src, 6, DDRB
      , src, 5, DDRB
      , src, 4, DDRB
      , src, 3, DDRB
      , src, 2, DDRB
      , src, 1, DDRB
      , src, 0, DDRB
      , src, 7, DDRE
      )
  }

void port0_write(uint8_t src) {
    REMAP_PINS(
        src, 6, PORTB
      , src, 5, PORTB
      , src, 4, PORTB
      , src, 3, PORTB
      , src, 2, PORTB
      , src, 1, PORTB
      , src, 0, PORTB
      , src, 7, PORTE
      )
  }

uint8_t port0_read(void) {
    uint8_t acc = 0;
    REMAP_PINS_INV(
        PINB, 6, acc
      , PINB, 5, acc
      , PINB, 4, acc
      , PINB, 3, acc
      , PINB, 2, acc
      , PINB, 1, acc
      , PINB, 0, acc
      , PINE, 7, acc
      )
    return acc;
}

void port1_init(uint8_t src) {
    REMAP_PINS(
    #ifdef ALT_PINS_1
        src, 5, DDRE
    #else
        src, 0, DDRE
    #endif
      , src, 7, DDRB
      , src, 0, DDRD
      , src, 1, DDRD
      , src, 2, DDRD
      , src, 3, DDRD
      , src, 4, DDRD
      , src, 5, DDRD
      )
  }

void port1_write(uint8_t src) {
    REMAP_PINS(
    #ifdef ALT_PINS_1
        src, 5, PORTE
    #else
        src, 0, PORTE
    #endif
      , src, 7, PORTB
      , src, 0, PORTD
      , src, 1, PORTD
      , src, 2, PORTD
      , src, 3, PORTD
      , src, 4, PORTD
      , src, 5, PORTD
      )
  }

uint8_t port1_read(void) {
    uint8_t acc = 0;
    REMAP_PINS_INV(
    #ifdef ALT_PINS_1
        PINE, 5, acc
    #else
        PINE, 0, acc
    #endif
      , PINB, 7, acc
      , PIND, 0, acc
      , PIND, 1, acc
      , PIND, 2, acc
      , PIND, 3, acc
      , PIND, 4, acc
      , PIND, 5, acc
      )
    return acc;
}

void port2_init(uint8_t src) {
    REMAP_PINS(
        src, 7, DDRF
      , src, 6, DDRF
      , src, 5, DDRF
      , src, 4, DDRF
      , src, 3, DDRF
      , src, 2, DDRF
      , src, 1, DDRF
      , src, 0, DDRF
      )
  }

void port2_write(uint8_t src) {
    REMAP_PINS(
        src, 7, PORTF
      , src, 6, PORTF
      , src, 5, PORTF
      , src, 4, PORTF
      , src, 3, PORTF
      , src, 2, PORTF
      , src, 1, PORTF
      , src, 0, PORTF
      )
  }

uint8_t port2_read(void) {
    uint8_t acc = 0;
    REMAP_PINS_INV(
        PINF, 7, acc
      , PINF, 6, acc
      , PINF, 5, acc
      , PINF, 4, acc
      , PINF, 3, acc
      , PINF, 2, acc
      , PINF, 1, acc
      , PINF, 0, acc
      )
    return acc;
}

void rst_init(void) {
    DDRD = (DDRD & ~(1<<6)) | 0 << 6;
}

uint8_t rst_read(void) {
    return !!(PIND & 1<<6);
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
