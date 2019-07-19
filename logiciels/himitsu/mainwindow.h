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

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class CaesarCipher;
class CipherEngine;
class EnigmaCipher;
class LorenzCipher;
class QSlider;
class VigenereCipher;

class MainWindow : public QMainWindow {
	Q_OBJECT

	Ui::MainWindow *ui = nullptr;
	QString m_ciphered_text = "";
	CipherEngine *m_current_engine = nullptr;
	CipherEngine *m_previous_engine = nullptr;
	CaesarCipher *m_caesar_cipher = nullptr;
	EnigmaCipher *m_enigma_cipher = nullptr;
	LorenzCipher *m_lorenz_cipher = nullptr;
	VigenereCipher *m_vigenere_cipher = nullptr;

	QSlider *m_letter_dist[26] = {};

	void setCurrentEngine(CipherEngine *cur);
	void cipher(const char letter, int mode);
	void resetLetterDistribution();

private Q_SLOTS:
	void recompute();
	void setCipheredText();	
	void setCipherEngine(int index);
	void changeCipherMode();	
	void hideDistributionWidgets(bool show);

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	MainWindow(MainWindow const &) = default;
	MainWindow &operator=(MainWindow const &) = default;
};
