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

#include "controle_echelle_valeur.h"

#include <iostream>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

/* ************************************************************************** */

static constexpr auto NOMBRE_PALIER = 8;

static const float PALIERS[NOMBRE_PALIER] = {
	1000.0f,
	100.0f,
	10.0f,
	1.0f,
	0.1f,
	0.01f,
	0.001f,
	0.0001f,
};

ControleEchelleDecimale::ControleEchelleDecimale(QWidget *parent)
	: QWidget(parent)
{
	auto metriques = this->fontMetrics();
	setFixedHeight(metriques.height() * 3.0f * NOMBRE_PALIER);
	setFixedWidth(metriques.width("1000.0") * 1.2f);
}

void ControleEchelleDecimale::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	auto rectangle = this->rect();

	/* Couleur d'arrière plan. */
	QBrush pinceaux(QColor(40, 40, 40));
	painter.fillRect(rectangle, pinceaux);

	/* dessine les séparation entre paliers */
	auto hauteur_palier = rectangle.height() / NOMBRE_PALIER;
	auto largeur_palier = rectangle.width();

	QPen stylo = QPen(Qt::white);
	painter.setPen(stylo);

	for (int i = 1; i < NOMBRE_PALIER; ++i) {
		painter.drawLine(0, hauteur_palier * i, largeur_palier, hauteur_palier * i);
	}

	/* dessine les nombres */
	for (int i = 0; i < NOMBRE_PALIER; ++i) {
		if (m_souris_pressee && i == m_case) {
			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 0.083f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 0.083f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(PALIERS[i]));

			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 0.583f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 0.583f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(m_valeur));
		}
		else {
			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 1.0f / 3.0f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 1.0f / 3.0f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(PALIERS[i]));
		}
	}
}

void ControleEchelleDecimale::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MouseButton::LeftButton) {
		const auto y = event->pos().y();

		auto rectangle = this->rect();
		auto hauteur_palier = rectangle.height() / NOMBRE_PALIER;
		m_case = y / hauteur_palier;

		QApplication::setOverrideCursor(Qt::SplitHCursor);

		m_vieil_x = event->pos().x();
		m_souris_pressee = true;
		update();
	}
}

void ControleEchelleDecimale::mouseMoveEvent(QMouseEvent *event)
{
	if (m_souris_pressee) {
		const auto x = event->pos().x();
		m_valeur += (x - m_vieil_x) * PALIERS[m_case];
		m_vieil_x = x;

		m_valeur = std::max(std::min(m_valeur, m_max), m_min);

		Q_EMIT(valeur_changee(m_valeur));

		update();
	}
}

void ControleEchelleDecimale::mouseReleaseEvent(QMouseEvent *event)
{
	QApplication::restoreOverrideCursor();
	m_souris_pressee = false;
	update();
	Q_EMIT(edition_terminee());
}

void ControleEchelleDecimale::valeur(float v)
{
	m_valeur = v;
}

void ControleEchelleDecimale::plage(float min, float max)
{
	m_min = min;
	m_max = max;
}

/* ************************************************************************** */

static constexpr auto NOMBRE_PALIER_ENTIER = 4;

static const int PALIERS_ENTIER[NOMBRE_PALIER_ENTIER] = {
	1000,
	100,
	10,
	1,
};

ControleEchelleEntiere::ControleEchelleEntiere(QWidget *parent)
	: QWidget(parent)
{
	auto metriques = this->fontMetrics();
	setFixedHeight(metriques.height() * 3.0f * NOMBRE_PALIER_ENTIER);
	setFixedWidth(metriques.width("1000.0") * 1.2f);
}

void ControleEchelleEntiere::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	auto rectangle = this->rect();

	/* Couleur d'arrière plan. */
	QBrush pinceaux(QColor(40, 40, 40));
	painter.fillRect(rectangle, pinceaux);

	/* dessine les séparation entre paliers */
	auto hauteur_palier = rectangle.height() / NOMBRE_PALIER_ENTIER;
	auto largeur_palier = rectangle.width();

	QPen stylo = QPen(Qt::white);
	painter.setPen(stylo);

	for (int i = 1; i < NOMBRE_PALIER_ENTIER; ++i) {
		painter.drawLine(0, hauteur_palier * i, largeur_palier, hauteur_palier * i);
	}

	/* dessine les nombres */
	for (int i = 0; i < NOMBRE_PALIER_ENTIER; ++i) {
		if (m_souris_pressee && i == m_case) {
			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 0.083f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 0.083f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(PALIERS_ENTIER[i]));

			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 0.583f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 0.583f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(m_valeur));
		}
		else {
			painter.drawText(largeur_palier * 0.1f,
							 hauteur_palier * 1.0f / 3.0f + hauteur_palier * i,
							 largeur_palier,
							 hauteur_palier * 1.0f / 3.0f + hauteur_palier * (i + 1),
							 Qt::AlignLeft,
							 QString::number(PALIERS_ENTIER[i]));
		}
	}
}

void ControleEchelleEntiere::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MouseButton::LeftButton) {
		const auto y = event->pos().y();

		auto rectangle = this->rect();
		auto hauteur_palier = rectangle.height() / NOMBRE_PALIER_ENTIER;
		m_case = y / hauteur_palier;

		QApplication::setOverrideCursor(Qt::SplitHCursor);

		m_vieil_x = event->pos().x();
		m_souris_pressee = true;
		update();
	}
}

void ControleEchelleEntiere::mouseMoveEvent(QMouseEvent *event)
{
	if (m_souris_pressee) {
		const auto x = event->pos().x();
		m_valeur += (x - m_vieil_x) * PALIERS_ENTIER[m_case];
		m_vieil_x = x;

		m_valeur = std::max(std::min(m_valeur, m_max), m_min);

		Q_EMIT(valeur_changee(m_valeur));

		update();
	}
}

void ControleEchelleEntiere::mouseReleaseEvent(QMouseEvent *event)
{
	QApplication::restoreOverrideCursor();
	m_souris_pressee = false;
	update();
	Q_EMIT(edition_terminee());
}

void ControleEchelleEntiere::valeur(int v)
{
	m_valeur = v;
}

void ControleEchelleEntiere::plage(int min, int max)
{
	m_min = min;
	m_max = max;
}
