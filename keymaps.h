#include "dhkeys.h"

// maybe change this to DH_SHIFT(key) form
#define DH_SHIFT 0x80

const char PROGMEM normal_keys[]=
	{ KEY_H, KEY_U, KEY_ESC, KEY_DELETE,
	  KEY_J, KEY_QUOTE, KEY_A, KEY_LEFT_BRACE,
	  KEY_M, KEY_COMMA, KEY_Z, KEY_X,
	  KEY_Y, KEY_I, KEY_Q, KEY_W,
	  KEY_K, KEY_SEMICOLON, KEY_S, KEY_B,
	  KEY_N, KEY_O, KEY_TILDE, KEY_E,
	  KEY_L, KEY_P, KEY_D, KEY_T,
	  KEY_PERIOD, KEY_SLASH, KEY_C, KEY_V,
	  KEY_RIGHT_BRACE, KEY_P, KEY_QUOTE + DH_SHIFT, KEY_R,
	  KEY_SEMICOLON, KEY_BACKSLASH, KEY_F, KEY_G,
	  KEY_DH_ALT, KEY_BACKSPACE, KEY_DH_CTRL, KEY_TAB,
	  KEY_DH_NAS, KEY_DH_NASLK, KEY_DH_SHIFT, KEY_CAPS_LOCK,
	  KEY_SPACE, KEY_DH_FN, KEY_ENTER, KEY_DH_NORM,
};

const char PROGMEM nas_keys[]=
	{ 
	  // 6 & del !
	  KEY_6, KEY_7 + DH_SHIFT, KEY_DELETE, KEY_1 + DH_SHIFT,
	  // 7 _ 1 ~
	  KEY_7, KEY_MINUS + DH_SHIFT, KEY_1, KEY_TILDE + DH_SHIFT,
	  // + , = x
	  KEY_EQUAL + DH_SHIFT, KEY_COMMA, KEY_EQUAL, KEY_X,
	  // ( * esc @
	  KEY_6 + DH_SHIFT, KEY_8 + DH_SHIFT, KEY_ESC, KEY_2 + DH_SHIFT,
	  // 8 : 3 numlk
	  KEY_8, KEY_SEMICOLON + DH_SHIFT, KEY_2, KEY_B,
	  // ; ( < #
	  KEY_SEMICOLON, KEY_9 + DH_SHIFT, KEY_COMMA + DH_SHIFT, KEY_3 + DH_SHIFT,
	  // 9 ent 3 >
	  KEY_9, KEY_P, KEY_3, KEY_PERIOD + DH_SHIFT,
	  // . ? % -
	  KEY_PERIOD, KEY_SLASH + DH_SHIFT, KEY_5 + DH_SHIFT, KEY_MINUS,
	  // 10off ) / $
	  KEY_RIGHT_BRACE, KEY_0 + DH_SHIFT, KEY_SLASH, KEY_4 + DH_SHIFT,
	  // 0 10kon 4 5
	  KEY_0, KEY_BACKSLASH, KEY_4, KEY_5,

	  KEY_DH_ALT, KEY_BACKSPACE, KEY_DH_CTRL, KEY_TAB,
	  KEY_DH_NAS, KEY_DH_NASLK, KEY_DH_SHIFT, KEY_CAPS_LOCK,
	  KEY_SPACE, KEY_DH_FN, KEY_ENTER, KEY_DH_NORM,
};

const char PROGMEM fn_keys[]=
	{ 
	  // <- uparrow del f2
	  KEY_LEFT, KEY_UP, KEY_DELETE, KEY_F2,
	  // home -> l/r scrllk
	  KEY_HOME, KEY_RIGHT, KEY_A, KEY_SCROLL_LOCK,
	  // downar f7 f1 f3
	  KEY_DOWN, KEY_F7, KEY_F1, KEY_F3,
	  // end f8 esc f4
	  KEY_END, KEY_F8, KEY_ESC, KEY_F4,
	  // arrowon shift mb3 numlk
	  KEY_K, KEY_SEMICOLON, KEY_S, KEY_NUM_LOCK,
	  // ins f10 = f6
	  KEY_INSERT, KEY_F10, KEY_EQUAL, KEY_F6,
	  // print ent mouseon ent
	  KEY_PRINTSCREEN, KEY_P, KEY_D, KEY_ENTER,
	  // f9 pgdn f5 downarr
	  KEY_F9, KEY_PAGE_DOWN, KEY_F5, KEY_DOWN,
	  // f11 pgup arrleft arrup
	  KEY_F11, KEY_PAGE_UP, KEY_LEFT, KEY_UP,
	  //  pause f12 home arrright
	  KEY_PAUSE, KEY_F12, KEY_HOME, KEY_RIGHT,

	  KEY_DH_ALT, KEY_BACKSPACE, KEY_DH_CTRL, KEY_TAB,
	  KEY_DH_NAS, KEY_DH_NASLK, KEY_DH_SHIFT, KEY_CAPS_LOCK,
	  KEY_SPACE, KEY_DH_FN, KEY_ENTER, KEY_DH_NORM,
};
