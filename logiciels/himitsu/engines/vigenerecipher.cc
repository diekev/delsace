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

#include "cipherengine.h"
#include "ui_vigenere.h"

#include "util/utils.h"

VigenereCipher::VigenereCipher()
    : ui(new Ui::VigenereCipher)
{
	ui->setupUi(this);
	m_index = 0;
	m_original_key = ui->m_key_edit->text();
}

VigenereCipher::~VigenereCipher()
{
	delete ui;
}

char VigenereCipher::getCipheredChar(char to_encrypt)
{
	if (isalpha(to_encrypt)) {
		to_encrypt = letter_add(to_encrypt, getCipherKey());
	}

	return to_encrypt;
}

char VigenereCipher::getDecipheredChar(char to_decrypt)
{
	if (isalpha(to_decrypt)) {
		to_decrypt = letter_sub(to_decrypt, getCipherKey());
	}

	return to_decrypt;
}

int VigenereCipher::getCipherKey() const
{
	auto text = ui->m_key_edit->text();

	if (!text.isEmpty()) {
		auto index = m_index % text.size();
		return letter_index(text[index].toLatin1()) + 1;
	}

	return 0;
}

void VigenereCipher::updateEngine(const bool backward)
{
	m_index += ((backward) ? -1 : 1);
}

void VigenereCipher::onCipherKeyChange()
{
	reset();
}

void VigenereCipher::reset()
{
	m_index = 0;
	Q_EMIT reencode();
}
