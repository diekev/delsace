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

#include "controle_teinte_couleur.h"

#include <QPainter>

ControleTeinteCouleur::ControleTeinteCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 256);
}

void ControleTeinteCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QLinearGradient degrade(QPoint(0, 0), QPoint(size().width(), 0));
	degrade.setColorAt(0.000, QColor::fromHsv(  0, 255, 255));
	degrade.setColorAt(0.125, QColor::fromHsv( 45, 255, 255));
	degrade.setColorAt(0.250, QColor::fromHsv( 90, 255, 255));
	degrade.setColorAt(0.375, QColor::fromHsv(135, 255, 255));
	degrade.setColorAt(0.500, QColor::fromHsv(180, 255, 255));
	degrade.setColorAt(0.625, QColor::fromHsv(225, 255, 255));
	degrade.setColorAt(0.750, QColor::fromHsv(270, 255, 255));
	degrade.setColorAt(0.875, QColor::fromHsv(315, 255, 255));
	degrade.setColorAt(1.000, QColor::fromHsv(359, 255, 255));

	painter.setBrush(QBrush(degrade));

	painter.drawRect(this->rect());
}
