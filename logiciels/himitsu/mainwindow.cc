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

#include <QKeyEvent>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "engines/cipherengine.h"
#include "util/utils.h"

enum {
	CEASER_CIPHER   = 0,
	VIGENERE_CIPHER = 1,
	ENIGMA_CIPHER   = 2,
	LORENZ_CIPHER   = 3,
};

enum {
	CIPHER   = 0,
	DECIPHER = 1,
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_ciphered_text("")
    , m_current_engine(nullptr)
    , m_previous_engine(nullptr)
    , m_caesar_cipher(new CaesarCipher())
    , m_enigma_cipher(new EnigmaCipher())
    , m_lorenz_cipher(new LorenzCipher())
    , m_vigenere_cipher(new VigenereCipher())
{
	ui->setupUi(this);

	ui->m_engine_layout->addWidget(m_caesar_cipher);
	ui->m_engine_layout->addWidget(m_enigma_cipher);
	ui->m_engine_layout->addWidget(m_lorenz_cipher);
	ui->m_engine_layout->addWidget(m_vigenere_cipher);

	m_caesar_cipher->hide();
	m_enigma_cipher->hide();
	m_lorenz_cipher->hide();
	m_vigenere_cipher->hide();

	for (int i(0); i < 26; ++i) {
		m_letter_dist[i] = new QSlider(Qt::Orientation::Vertical);
		m_letter_dist[i]->setValue(0);
		m_letter_dist[i]->setMaximum(200);
		m_letter_dist[i]->setHidden(!ui->m_view_distribution->isChecked());
		ui->m_distribution_layout->addWidget(m_letter_dist[i]);
	}

	setCipherEngine(ui->m_engine_box->currentIndex());
	m_previous_engine = m_current_engine;
}

MainWindow::~MainWindow()
{
	delete m_caesar_cipher;
	delete m_enigma_cipher;
	delete m_lorenz_cipher;
	delete m_vigenere_cipher;
	delete ui;
}

void MainWindow::setCipheredText()
{
	auto current_text = ui->m_text->toPlainText();

	if (current_text.isEmpty()) {
		m_current_engine->reset();
	}
	else if (current_text.size() > m_ciphered_text.size()) {
		auto last_char = current_text[current_text.size() - 1].toLatin1();
		cipher(last_char, ui->m_cipher_mode->currentIndex());
	}
	else if (current_text.size() < m_ciphered_text.size()) {
		m_ciphered_text.resize(current_text.size());
		m_current_engine->updateEngine(true);
	}

	ui->m_new_text->setText(m_ciphered_text);
}

void MainWindow::setCurrentEngine(CipherEngine *cur)
{
	if (m_previous_engine != nullptr) {
		m_previous_engine = m_current_engine;
		disconnect(ui->m_reset_engine, SIGNAL(clicked()), m_previous_engine, SLOT(reset()));
		disconnect(ui->m_cipher_mode, SIGNAL(currentIndexChanged(int)), m_previous_engine, SLOT(reset()));
		disconnect(m_previous_engine, SIGNAL(reencode()), this, SLOT(recompute()));
		m_previous_engine->hide();
	}

	m_current_engine = cur;

	m_current_engine->show();
	connect(ui->m_reset_engine, SIGNAL(clicked()), m_current_engine, SLOT(reset()));
	connect(ui->m_cipher_mode, SIGNAL(currentIndexChanged(int)), m_current_engine, SLOT(reset()));
	connect(m_current_engine, SIGNAL(reencode()), this, SLOT(recompute()));

	resetLetterDistribution();
}

void MainWindow::setCipherEngine(int index)
{
	switch (index) {
		case CEASER_CIPHER:
			setCurrentEngine(m_caesar_cipher);
			break;
		case VIGENERE_CIPHER:
			setCurrentEngine(m_vigenere_cipher);
			break;
		case ENIGMA_CIPHER:
			setCurrentEngine(m_enigma_cipher);
			break;
		case LORENZ_CIPHER:
			setCurrentEngine(m_lorenz_cipher);
			break;
	}

	recompute();
}

void MainWindow::recompute()
{
	m_ciphered_text.clear();
	resetLetterDistribution();
	auto current_text = ui->m_text->toPlainText();

	for (auto ch : current_text) {
		cipher(ch.toLatin1(), ui->m_cipher_mode->currentIndex());
	}

	ui->m_new_text->setText(m_ciphered_text);
}

void MainWindow::changeCipherMode()
{
	ui->m_text->setText(m_ciphered_text);
	recompute();
}

void MainWindow::cipher(const char letter, int mode)
{
	char result;

	switch (mode) {
		default:
		case CIPHER:
			result = m_current_engine->getCipheredChar(letter);
			break;
		case DECIPHER:
			result = m_current_engine->getDecipheredChar(letter);
			break;
	}

	m_ciphered_text.push_back(result);

	m_current_engine->updateEngine(false);

	if (isalpha(result)) {
		auto index = letter_index(result);
		m_letter_dist[index]->setValue(m_letter_dist[index]->value() + 1);
	}
}

void MainWindow::resetLetterDistribution()
{
	for (int i(0); i < 26; ++i) {
		m_letter_dist[i]->setValue(0);
	}
}

void MainWindow::hideDistributionWidgets(bool show)
{
	for (int i(0); i < 26; ++i) {
		m_letter_dist[i]->setHidden(!show);
	}
}
