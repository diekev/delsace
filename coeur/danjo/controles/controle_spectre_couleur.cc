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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "controle_spectre_couleur.h"

#include <QPainter>

static constexpr auto DEBUT_SPECTRE = 380.0;
static constexpr auto FIN_SPECTRE = 700.0;
static constexpr auto TAILLE_SPECTRE = FIN_SPECTRE - DEBUT_SPECTRE;

constexpr auto position_spectre(const float spectre)
{
	return (spectre - DEBUT_SPECTRE) / TAILLE_SPECTRE;
}

ControleSpectreCouleur::ControleSpectreCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 256);
}

void ControleSpectreCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QLinearGradient degrade(QPoint(0, 0), QPoint(size().width(), 0));
	/* couleurs extraites de https://fr.wikipedia.org/wiki/Spectre_visible */
	degrade.setColorAt(position_spectre(DEBUT_SPECTRE), QColor(0, 0, 0));
	degrade.setColorAt(position_spectre(445.0), QColor(39, 0, 91));
	degrade.setColorAt(position_spectre(455.0), QColor(42, 0, 123));
	degrade.setColorAt(position_spectre(470.0), QColor(0, 47, 131));
	degrade.setColorAt(position_spectre(480.0), QColor(0, 71, 105));
	degrade.setColorAt(position_spectre(485.0), QColor(0, 81, 98));
	degrade.setColorAt(position_spectre(500.0), QColor(0, 114, 95));
	degrade.setColorAt(position_spectre(525.0), QColor(0, 175, 108));
	degrade.setColorAt(position_spectre(555.0), QColor(89, 192, 0));
	degrade.setColorAt(position_spectre(574.0), QColor(202, 179, 0));
	degrade.setColorAt(position_spectre(577.0), QColor(210, 169, 0));
	degrade.setColorAt(position_spectre(582.0), QColor(215, 147, 0));
	degrade.setColorAt(position_spectre(586.0), QColor(222, 132, 0));
	degrade.setColorAt(position_spectre(590.0), QColor(231, 119, 0));
	degrade.setColorAt(position_spectre(600.0), QColor(245, 80, 0));
	degrade.setColorAt(position_spectre(615.0), QColor(234, 0, 33));
	degrade.setColorAt(position_spectre(650.0), QColor(122, 0, 34));
	degrade.setColorAt(position_spectre(FIN_SPECTRE), QColor(0, 0, 0));

	painter.setBrush(QBrush(degrade));

	painter.drawRect(this->rect());
}
