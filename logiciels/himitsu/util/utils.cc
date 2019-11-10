/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <QSpinBox>

#include "utils.h"

void rotate_wheel(QSpinBox *spin_box, int increment)
{
	spin_box->setValue(spin_box->value() + increment);
}

/* À FAIRE: implement routine for special characters and numbers */
int beaudot_encode(const char ch)
{
	switch (ch) {
		case ' ': return 0b00100; // same
		case 'q': case 'Q': return 0b10111; // 1
		case 'w': case 'W': return 0b10011; // 2
		case 'e': case 'E': return 0b00001; // 3
		case 'r': case 'R': return 0b01010; // 4
		case 't': case 'T': return 0b10000; // 5
		case 'y': case 'Y': return 0b10101; // 6
		case 'u': case 'U': return 0b00111; // 7
		case 'i': case 'I': return 0b00110; // 8
		case 'o': case 'O': return 0b11000; // 9
		case 'p': case 'P': return 0b10110;
		case 'a': case 'A': return 0b00011; // -
		case 's': case 'S': return 0b00101; // BELL
		case 'd': case 'D': return 0b01001; // $
		case 'f': case 'F': return 0b01101; // !
		case 'g': case 'G': return 0b11010; // &
		case 'h': case 'H': return 0b10100; // #
		case 'j': case 'J': return 0b01011; // '
		case 'k': case 'K': return 0b01111; // (
		case 'l': case 'L': return 0b10010; // )
		case 'z': case 'Z': return 0b10001; // "
		case 'x': case 'X': return 0b11101; // /
		case 'c': case 'C': return 0b01110; // :
		case 'v': case 'V': return 0b11110; // ;
		case 'b': case 'B': return 0b11001; // ?
		case 'n': case 'N': return 0b01100; // ,
		case 'm': case 'M': return 0b11100; // .

		/* À FAIRE: the following cases are wrong, need other chars */
		case '[': return 0b01000; // retour à la ligne CR
		case ']': return 0b00010; // retour à la ligne LF
		case '<': return 0b11011; // passage aux chiffres
		case '>': return 0b11111; // passage aux lettres
	}

	return 0b00000;
}

/* À FAIRE: implement routine for special characters and numbers */
char beaudot_decode(const int code)
{
	switch (code) {
		case 0b00100: return ' '; // same
		case 0b10111: return 'q'; // 1
		case 0b10011: return 'w'; // 2
		case 0b00001: return 'e'; // 3
		case 0b01010: return 'r'; // 4
		case 0b10000: return 't'; // 5
		case 0b10101: return 'y'; // 6
		case 0b00111: return 'u'; // 7
		case 0b00110: return 'i'; // 8
		case 0b11000: return 'o'; // 9
		case 0b10110: return 'p';
		case 0b00011: return 'a'; // -
		case 0b00101: return 's'; // BELL
		case 0b01001: return 'd'; // $
		case 0b01101: return 'f'; // !
		case 0b11010: return 'g'; // &
		case 0b10100: return 'h'; // #
		case 0b01011: return 'j'; // '
		case 0b01111: return 'k'; // (
		case 0b10010: return 'l'; // )
		case 0b10001: return 'z'; // "
		case 0b11101: return 'x'; // /
		case 0b01110: return 'c'; // :
		case 0b11110: return 'v'; // :
		case 0b11001: return 'b'; // ?
		case 0b01100: return 'n'; // ,
		case 0b11100: return 'm'; // .

		/* À FAIRE: the following cases are wrong, need other chars */
		case 0b01000: return '['; // retour à la ligne CR
		case 0b00010: return ']'; // retour à la ligne LF
		case 0b11011: return '<'; // passage aux chiffres
		case 0b11111: return '>'; // passage aux lettres
	}

	return '\0';
}
