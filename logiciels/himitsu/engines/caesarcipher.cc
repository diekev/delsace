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
#include "ui_caesar.h"

#include "util/utils.h"

CaesarCipher::CaesarCipher()
    : ui(new Ui::CaesarCipher)
{
	ui->setupUi(this);
	m_original_key = ui->m_letter_shift->value();
}

CaesarCipher::~CaesarCipher()
{
	delete ui;
}

char CaesarCipher::getCipheredChar(char to_encrypt)
{
	if (isalpha(to_encrypt)) {
		to_encrypt = letter_add(to_encrypt, getCipherKey());
	}

	return to_encrypt;
}

char CaesarCipher::getDecipheredChar(char to_decrypt)
{
	if (isalpha(to_decrypt)) {
		to_decrypt = letter_sub(to_decrypt, getCipherKey());
	}

	return to_decrypt;
}

int CaesarCipher::getCipherKey() const
{
	return ui->m_letter_shift->value();
}

void CaesarCipher::updateEngine(const bool /*backward*/)
{
	/* pass */
}

void CaesarCipher::onCipherKeyChange()
{
	reset();
}

void CaesarCipher::reset()
{
	Q_EMIT reencode();
}
