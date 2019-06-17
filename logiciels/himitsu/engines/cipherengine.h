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

#pragma once

#include <QWidget>

namespace Ui {
class CaesarCipher;
class EnigmaCipher;
class LorenzCipher;
class VigenereCipher;
}

class LetterWheel;
class PinWheel;
class QLineEdit;

class CipherEngine : public QWidget {
	Q_OBJECT

public:
	CipherEngine() = default;
	~CipherEngine() = default;

	virtual char getCipheredChar(char to_encrypt) = 0;
	virtual char getDecipheredChar(char to_decrypt) = 0;
	virtual int getCipherKey() const = 0;
	virtual void updateEngine(const bool backward) = 0;

public Q_SLOTS:
	virtual void reset() = 0;

Q_SIGNALS:
	void reencode();
};

class CaesarCipher final : public CipherEngine {
	Q_OBJECT

	Ui::CaesarCipher *ui;
	int m_original_key;

private Q_SLOTS:
	void onCipherKeyChange();

public Q_SLOTS:
	void reset();

public:
	CaesarCipher();
	~CaesarCipher();

	char getCipheredChar(char to_encrypt);
	char getDecipheredChar(char to_decrypt);
	int getCipherKey() const;
	void updateEngine(const bool backward);
};

class VigenereCipher final : public CipherEngine {
	Q_OBJECT

	Ui::VigenereCipher *ui;

	int m_index;
	QString m_original_key;

private Q_SLOTS:
	void onCipherKeyChange();

public Q_SLOTS:
	void reset();

public:
	VigenereCipher();
	~VigenereCipher();

	char getCipheredChar(char to_encrypt);
	char getDecipheredChar(char to_decrypt);
	int getCipherKey() const;
	void updateEngine(const bool backward);
};

class EnigmaCipher final : public CipherEngine {
	Q_OBJECT

	Ui::EnigmaCipher *ui;
	std::vector<int> m_plug_board;
	std::string m_reflect;

	enum {
		ROTOR_1 = 0,
		ROTOR_2 = 1,
		ROTOR_3 = 2,
	};

	enum {
		NUM_ROTORS = ROTOR_3 + 1
	};

	LetterWheel *m_rotors[NUM_ROTORS];

	int m_original_rotor_pos[NUM_ROTORS];

	/**
	 * http://www.cryptomuseum.com/crypto/enigma/g/index.htm
	 */
	const std::string m_original_keys[NUM_ROTORS] = {
	    "LPGSZMHAEOQKVXRFYBUTNICJDW",
	    "SLVGBTFXJQOHEWIRZYAMKPCNDU",
	    "CJGDPSHKTURAWZXFMYNQOBVLIE"
	};

	void addSwapPair(const char a, const char b);
	char toPlugBoard(const char to_scrumble) const;

public Q_SLOTS:
	void reset();

public:
	EnigmaCipher();
	~EnigmaCipher();

	char getCipheredChar(char to_encrypt);
	char getDecipheredChar(char to_decrypt);
	int getCipherKey() const;
	void updateEngine(const bool backward);
};

class LorenzCipher final : public CipherEngine {
	Q_OBJECT

	Ui::LorenzCipher *ui;

	enum {
		WHEEL_PHI_1 = 0,
		WHEEL_PHI_2 = 1,
		WHEEL_PHI_3 = 2,
		WHEEL_PHI_4 = 3,
		WHEEL_PHI_5 = 4,
		WHEEL_MU_37 = 5,
		WHEEL_MU_61 = 6,
		WHEEL_CHI_1 = 7,
		WHEEL_CHI_2 = 8,
		WHEEL_CHI_3 = 9,
		WHEEL_CHI_4 = 10,
		WHEEL_CHI_5 = 11,
	};

	enum {
		NUM_WHEELS = WHEEL_CHI_5 + 1
	};

	PinWheel *m_wheels[NUM_WHEELS];

	const int m_wheels_range[NUM_WHEELS] = {
	    43, 47, 51, 53, 59, 37, 61, 41, 31, 29, 26, 23
	};

	int m_original_keys[NUM_WHEELS];
	QLineEdit *m_pin_edit[NUM_WHEELS];

private Q_SLOTS:
	void generateRandomPin();
	void showPins(bool show);

public Q_SLOTS:
	void reset();

public:
	LorenzCipher();
	~LorenzCipher();

	char getCipheredChar(char to_encrypt);
	char getDecipheredChar(char to_decrypt);
	int getCipherKey() const;
	void updateEngine(const bool backward);
};
