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

#include <algorithm>

#include "cipherengine.h"
#include "ui_enigma.h"

#include "util/utils.h"
#include "widgets/wheelwidget.h"

EnigmaCipher::EnigmaCipher()
    : ui(new Ui::EnigmaCipher)
{
	ui->setupUi(this);

	for (int i(ROTOR_1); i < NUM_ROTORS; ++i) {
		m_rotors[i] = new LetterWheel(26, this);
		m_rotors[i]->setKeys(m_original_keys[i]);
		m_original_rotor_pos[i] = m_rotors[i]->value();
		ui->m_rotor_layout->addWidget(m_rotors[i]);
	}

	m_reflect = "IMETCGFRAYSQBZXWLHKDVUPOJN";

	m_plug_board.redimensionne(26);
	std::iota(m_plug_board.debut(), m_plug_board.fin(), 0);

	addSwapPair('A', 'T');
	addSwapPair('B', 'J');
	addSwapPair('E', 'G');
	addSwapPair('P', 'Z');
	addSwapPair('M', 'N');
	addSwapPair('D', 'X');
}

EnigmaCipher::~EnigmaCipher()
{
	delete ui;
}

char EnigmaCipher::getCipheredChar(char to_encrypt)
{
	if (isalpha(to_encrypt)) {
		to_encrypt = static_cast<char>(toupper(to_encrypt));
		to_encrypt = toPlugBoard(to_encrypt);
		to_encrypt = letter_add(to_encrypt, getCipherKey());
		to_encrypt = toPlugBoard(to_encrypt);
	}

	return to_encrypt;
}

char EnigmaCipher::getDecipheredChar(char to_decrypt)
{
	if (isalpha(to_decrypt)) {
		to_decrypt = static_cast<char>(toupper(to_decrypt));
		to_decrypt = toPlugBoard(to_decrypt);
		to_decrypt = letter_sub(to_decrypt, getCipherKey());
		to_decrypt = toPlugBoard(to_decrypt);
	}

	return to_decrypt;
}

int EnigmaCipher::getCipherKey() const
{
	/* pass through right rotor */
	const auto a = letter_index(m_rotors[ROTOR_3]->key());

	/* pass through middle rotor */
	const auto b = letter_index(m_rotors[ROTOR_2]->key(a));

	/* pass through left rotor */
	const auto c = letter_index(m_rotors[ROTOR_1]->key(b));

	/* pass through reflector */
	const auto refl = letter_index(m_reflect[c]);

	/* pass through left rotor */
	const auto d = letter_index(m_rotors[ROTOR_1]->key(refl));

	/* pass through middle rotor */
	const auto e = letter_index(m_rotors[ROTOR_2]->key(d));

	/* pass through right rotor */
	return letter_index(m_rotors[ROTOR_3]->key(e));
}

void EnigmaCipher::updateEngine(const bool backward)
{
	auto inc = (backward) ? -1 : 1;

	m_rotors[ROTOR_3]->rotate(inc);

	if (m_rotors[ROTOR_3]->value() == 1) {
		m_rotors[ROTOR_2]->rotate(inc);

		if (m_rotors[ROTOR_2]->value() == 1) {
			m_rotors[ROTOR_1]->rotate(inc);
		}
	}
}

void EnigmaCipher::addSwapPair(const char a, const char b)
{
	std::swap(m_plug_board[letter_index(a)], m_plug_board[letter_index(b)]);
}

char EnigmaCipher::toPlugBoard(const char to_scrumble) const
{
	auto first = (isupper(to_scrumble)) ? 'A' : 'a';
	return static_cast<char>(m_plug_board[letter_index(to_scrumble)] + first);
}

void EnigmaCipher::reset()
{
	for (int i(ROTOR_1); i < NUM_ROTORS; ++i) {
		m_rotors[i]->setKeys(m_original_keys[i]);
		m_rotors[i]->setValue(m_original_rotor_pos[i]);
	}

	Q_EMIT reencode();
}
