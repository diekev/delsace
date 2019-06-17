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

#include <QLineEdit>

#include "cipherengine.h"
#include "ui_lorenz_sz40.h"

#include "util/utils.h"
#include "widgets/wheelwidget.h"

LorenzCipher::LorenzCipher()
    : ui(new Ui::LorenzCipher)
{
	ui->setupUi(this);

	for (int i(WHEEL_PHI_1); i < NUM_WHEELS; ++i) {
		m_wheels[i] = new PinWheel(m_wheels_range[i]);
		m_original_keys[i] = m_wheels[i]->value();

		/* set pin edit widgets */
		m_pin_edit[i] = new QLineEdit(this);
		m_pin_edit[i]->setText(m_wheels[i]->pinStr());
		ui->m_pin_edit_layout->addWidget(m_pin_edit[i]);

		connect(m_pin_edit[i], SIGNAL(textChanged(QString)), m_wheels[i], SLOT(setPins(QString)));
	}

	showPins(ui->checkBox->isChecked());

	for (int i(WHEEL_PHI_1); i <= WHEEL_PHI_5; ++i) {
		ui->m_phi_layout->addWidget(m_wheels[i]);
	}

	for (int i(WHEEL_MU_37); i <= WHEEL_MU_61; ++i) {
		ui->m_mu_layout->addWidget(m_wheels[i]);
	}

	for (int i(WHEEL_CHI_1); i <= WHEEL_CHI_5; ++i) {
		ui->m_chi_layout->addWidget(m_wheels[i]);
	}
}

LorenzCipher::~LorenzCipher()
{
	delete ui;
}

char LorenzCipher::getCipheredChar(char to_encrypt)
{
	if (isalpha(to_encrypt) || is_elem(to_encrypt, ' ', '<', '>', '[', ']', '\0')) {
		auto beaudot_char = beaudot_encode(to_encrypt);
		auto is_upper = isupper(to_encrypt);
		auto new_char = beaudot_char ^ getCipherKey();

		to_encrypt = beaudot_decode(new_char);

		if (is_upper) {
			to_encrypt = toupper(to_encrypt);
		}
	}

	return to_encrypt;
}

char LorenzCipher::getDecipheredChar(char to_decrypt)
{
	return getCipheredChar(to_decrypt);
}

int LorenzCipher::getCipherKey() const
{
	auto key = 0;

	for (int i(WHEEL_PHI_1); i <= WHEEL_PHI_5; ++i) {
		key |= ((m_wheels[i]->pinValue() ^ m_wheels[WHEEL_CHI_1 + i]->pinValue()) << i);
	}

	return key;
}

void LorenzCipher::updateEngine(const bool backward)
{
	auto inc = (backward) ? -1 : 1;

	for (int i(WHEEL_CHI_1); i <= WHEEL_CHI_5; ++i) {
		m_wheels[i]->rotate(inc);
	}

	if (!backward) {
		m_wheels[WHEEL_MU_61]->rotate(inc);

		if (m_wheels[WHEEL_MU_61]->pinValue() == 1) {
			m_wheels[WHEEL_MU_37]->rotate(inc);
		}
	}

	if (m_wheels[WHEEL_MU_37]->pinValue() == 1) {
		for (int i(WHEEL_PHI_1); i <= WHEEL_PHI_5; ++i) {
			m_wheels[i]->rotate(inc);
		}
	}

	if (backward) {
		if (m_wheels[WHEEL_MU_61]->pinValue() == 1) {
			m_wheels[WHEEL_MU_37]->rotate(inc);
		}

		m_wheels[WHEEL_MU_61]->rotate(inc);
	}
}

auto LorenzCipher::generateRandomPin() -> void
{
	for (int i(WHEEL_PHI_1); i < NUM_WHEELS; ++i) {
		m_wheels[i]->generatePins();
		m_pin_edit[i]->setText(m_wheels[i]->pinStr());
	}
}

void LorenzCipher::showPins(bool show)
{
	for (int i(WHEEL_PHI_1); i < NUM_WHEELS; ++i) {
		m_pin_edit[i]->setHidden(!show);
	}
	ui->m_generate_pin->setHidden(!show);
}

void LorenzCipher::reset()
{
	for (int i(WHEEL_PHI_1); i < NUM_WHEELS; ++i) {
		m_wheels[i]->setValue(m_original_keys[i]);
	}

	/* TODO: reset rng */

	Q_EMIT reencode();
}
