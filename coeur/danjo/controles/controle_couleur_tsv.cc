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

#include "controle_couleur_tsv.h"

#include <QMouseEvent>
#include <QPainter>

#include "types/outils.h"

namespace danjo {

static constexpr auto TAILLE_SELECTEUR_MAX = 256.0;
static constexpr auto TAILLE_SELECTEUR_MIN = 32.0;

/* ************************************************************************** */

ControleSatVal::ControleSatVal(QWidget *parent)
	: QWidget(parent)
{
	setFixedSize(static_cast<int>(TAILLE_SELECTEUR_MAX), static_cast<int>(TAILLE_SELECTEUR_MAX));
}

void ControleSatVal::couleur(const couleur32 &c)
{
	m_hsv = c;
	m_pos_x = static_cast<double>(m_hsv.v) * TAILLE_SELECTEUR_MAX;
	m_pos_y = static_cast<double>(m_hsv.b) * TAILLE_SELECTEUR_MAX;
	update();
}

float ControleSatVal::saturation() const
{
	return static_cast<float>(m_pos_x / TAILLE_SELECTEUR_MAX);
}

float ControleSatVal::valeur() const
{
	return static_cast<float>(m_pos_y / TAILLE_SELECTEUR_MAX);
}

void ControleSatVal::paintEvent(QPaintEvent *)
{
	QPainter peintre(this);

	for (auto s = 0.0; s < TAILLE_SELECTEUR_MAX; s += 1.0) {
		for (auto v = 0.0; v < TAILLE_SELECTEUR_MAX; v += 1.0) {
			peintre.setPen(QColor::fromHsvF(static_cast<double>(m_hsv.r), s / TAILLE_SELECTEUR_MAX, v / TAILLE_SELECTEUR_MAX));
			peintre.drawPoint(static_cast<int>(s), static_cast<int>(TAILLE_SELECTEUR_MAX - v));
		}
	}

	peintre.setPen(Qt::white);

	peintre.drawEllipse(static_cast<int>(m_pos_x) - 4,
						static_cast<int>(TAILLE_SELECTEUR_MAX - m_pos_y) - 4,
						8,
						8);
}

void ControleSatVal::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	m_pos_x = restreint(event->pos().x(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));

	m_pos_y = restreint(static_cast<int>(TAILLE_SELECTEUR_MAX) - event->pos().y(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));

	m_souris_pressee = true;
	update();

	Q_EMIT(valeur_changee());
}

void ControleSatVal::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_souris_pressee) {
		return;
	}

	m_pos_x = restreint(event->pos().x(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));

	m_pos_y = restreint(static_cast<int>(TAILLE_SELECTEUR_MAX) - event->pos().y(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));
	update();

	Q_EMIT(valeur_changee());
}

void ControleSatVal::mouseReleaseEvent(QMouseEvent *)
{
	m_souris_pressee = false;
}

/* ************************************************************************** */

SelecteurTeinte::SelecteurTeinte(QWidget *parent)
	: QWidget(parent)
{
	setFixedSize(static_cast<int>(TAILLE_SELECTEUR_MAX), static_cast<int>(TAILLE_SELECTEUR_MIN));
}

void SelecteurTeinte::teinte(float t)
{
	m_teinte = static_cast<double>(t);
	m_pos_x = TAILLE_SELECTEUR_MAX * m_teinte;
	update();
}

float SelecteurTeinte::teinte() const
{
	return static_cast<float>(m_pos_x / TAILLE_SELECTEUR_MAX);
}

void SelecteurTeinte::paintEvent(QPaintEvent *)
{
	QPainter peintre(this);

	QLinearGradient degrade(QPoint(0, 0), QPoint(static_cast<int>(TAILLE_SELECTEUR_MAX), 0));

	for (auto i = 0.0; i <= 32.0; i += 1.0) {
		degrade.setColorAt(i / 32.0, QColor::fromHsvF(i / 32.0, 1.0, 1.0));
	}

	peintre.setBrush(QBrush(degrade));
	peintre.drawRect(this->rect());

	peintre.setPen(Qt::white);

	peintre.drawEllipse(static_cast<int>(m_pos_x) - 4,
						static_cast<int>(TAILLE_SELECTEUR_MIN * 0.5) - 4,
						8,
						8);
}

void SelecteurTeinte::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	m_pos_x = restreint(event->pos().x(), 0, static_cast<int>(TAILLE_SELECTEUR_MAX));
	m_souris_pressee = true;
	update();

	Q_EMIT(valeur_changee());
}

void SelecteurTeinte::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_souris_pressee) {
		return;
	}

	m_pos_x = restreint(event->pos().x(), 0, static_cast<int>(TAILLE_SELECTEUR_MAX));
	update();

	Q_EMIT(valeur_changee());
}

void SelecteurTeinte::mouseReleaseEvent(QMouseEvent *)
{
	m_souris_pressee = false;
}

ControleValeurCouleur::ControleValeurCouleur(QWidget *parent)
	: QWidget(parent)
{
	setFixedSize(static_cast<int>(TAILLE_SELECTEUR_MIN), static_cast<int>(TAILLE_SELECTEUR_MAX));
}

void ControleValeurCouleur::valeur(float t)
{
	m_valeur = static_cast<double>(t);
	m_pos_y = TAILLE_SELECTEUR_MAX * m_valeur;
	update();
}

float ControleValeurCouleur::valeur() const
{
	return static_cast<float>(m_pos_y / TAILLE_SELECTEUR_MAX);
}

void ControleValeurCouleur::paintEvent(QPaintEvent *)
{
	QPainter peintre(this);

	QLinearGradient degrade(QPoint(0, 0), QPoint(0, static_cast<int>(TAILLE_SELECTEUR_MAX)));

	for (double i = 0.0; i <= 32.0; i += 1.0) {
		auto v = i / 32.0;
		degrade.setColorAt(1.0 - v, QColor::fromRgbF(v, v, v));
	}

	peintre.setBrush(QBrush(degrade));
	peintre.drawRect(this->rect());

	peintre.setPen(Qt::white);

	peintre.drawEllipse(static_cast<int>(TAILLE_SELECTEUR_MIN * 0.5) - 4,
						static_cast<int>(TAILLE_SELECTEUR_MAX - m_pos_y) - 4,
						8,
						8);
}

void ControleValeurCouleur::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}

	m_souris_pressee = true;
	m_pos_y = restreint(static_cast<int>(TAILLE_SELECTEUR_MAX) - event->pos().y(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));
	update();

	Q_EMIT(valeur_changee());
}

void ControleValeurCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (!m_souris_pressee) {
		return;
	}

	m_pos_y = restreint(static_cast<int>(TAILLE_SELECTEUR_MAX) - event->pos().y(),
						0,
						static_cast<int>(TAILLE_SELECTEUR_MAX));
	update();

	Q_EMIT(valeur_changee());
}

void ControleValeurCouleur::mouseReleaseEvent(QMouseEvent *)
{
	m_souris_pressee = false;
}

/* ************************************************************************** */

}  /* namespace danjo */
