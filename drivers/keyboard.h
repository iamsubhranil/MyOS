#ifndef KEYCODES_H
#define KEYCODES_H

#include <sys/system.h>

struct Keyboard {
	static void init(u8 deviceNum);
	// blockOnZero blocks the caller until the next non zero ASCII
	static char getCharacter(bool blockOnZero = false);

	enum Key : int {
		Num_0 = '0',
		Num_1 = '1',
		Num_2 = '2',
		Num_3 = '3',
		Num_4 = '4',
		Num_5 = '5',
		Num_6 = '6',
		Num_7 = '7',
		Num_8 = '8',
		Num_9 = '9',

		Char_A = 'A',
		Char_a = 'a',

		Key_BreakCode
	};

	static void handleKeyboard(Register *r);
};

#endif
