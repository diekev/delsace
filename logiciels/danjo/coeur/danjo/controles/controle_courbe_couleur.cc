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

#include "controle_courbe_couleur.h"

#include <cmath>
#include <iostream>
#include <QMouseEvent>
#include <QPainter>

#include "types/courbe_bezier.h"

/* ************************************************************************** */

#undef DEBOGAGE_CONTROLES

static QColor COULEURS_COURBES[5] = {
	QColor(255, 255, 255),
	QColor(255, 0, 0),
	QColor(0, 255, 0),
	QColor(0, 0, 255),
	QColor(255, 255, 255),
};

/* ************************************************************************** */

ControleCourbeCouleur::ControleCourbeCouleur(QWidget *parent)
	: QWidget(parent)
{
	setMinimumSize(300, 300);
}

void ControleCourbeCouleur::change_mode(int mode)
{
	m_mode = mode;
	update();
}

void ControleCourbeCouleur::installe_courbe(CourbeBezier *courbe)
{
	m_courbe = courbe;
	m_point_courant = courbe->point_courant;
	update();
}

void ControleCourbeCouleur::paintEvent(QPaintEvent *)
{
	const auto hauteur = size().height();
	const auto largeur = size().width();
	auto const hauteurf = static_cast<float>(hauteur);
	auto const largeurf = static_cast<float>(largeur);

	QBrush pinceaux(QColor(40, 40, 40));
	QPainter painter(this);
	painter.fillRect(this->rect(), pinceaux);

	/* dessine les grilles secondaires */
	painter.setPen(QColor(45, 45, 45));

	auto decalage_hauteur = hauteur / 20.0;
	auto decalage_largeur = largeur / 20.0;
	auto h = decalage_hauteur;
	auto l = decalage_largeur;

	for (int i = 0; i < 19; ++i) {
		painter.drawLine(static_cast<int>(l), 0, static_cast<int>(l), hauteur);
		painter.drawLine(0, static_cast<int>(h), largeur, static_cast<int>(h));

		h += decalage_hauteur;
		l += decalage_largeur;
	}

	/* dessine les grilles principales */
	painter.setPen(QColor(55, 55, 55));

	decalage_hauteur = hauteur / 5.0;
	decalage_largeur = largeur / 5.0;
	h = decalage_hauteur;
	l = decalage_largeur;

	for (int i = 0; i < 4; ++i) {
		painter.drawLine(static_cast<int>(l), 0, static_cast<int>(l), hauteur);
		painter.drawLine(0, static_cast<int>(h), largeur, static_cast<int>(h));

		h += decalage_hauteur;
		l += decalage_largeur;
	}

	/* dessine la courbe */

	auto stylo_colore = QPen(COULEURS_COURBES[m_mode]);
	stylo_colore.setWidthF(1.0);
	painter.setPen(stylo_colore);

	auto p1 = m_courbe->extension_min.co[POINT_CENTRE];
	auto p2 = m_courbe->table[0];

	painter.drawLine(static_cast<int>(p1.x * largeurf),
					 static_cast<int>((1.0f - p1.y) * hauteurf),
					 static_cast<int>(p2.x * largeurf),
					 static_cast<int>((1.0f - p2.y) * hauteurf));

	for (auto i = 0; i < m_courbe->table.taille() - 1; ++i) {
		p1 = m_courbe->table[i];
		p2 = m_courbe->table[i + 1];

		painter.drawLine(static_cast<int>(p1.x * largeurf),
						 static_cast<int>((1.0f - p1.y) * hauteurf),
						 static_cast<int>(p2.x * largeurf),
						 static_cast<int>((1.0f - p2.y) * hauteurf));
	}

	p1 = m_courbe->table[m_courbe->table.taille() - 1];
	p2 = m_courbe->extension_max.co[POINT_CENTRE];

	painter.drawLine(static_cast<int>(p1.x * largeurf),
					 static_cast<int>((1.0f - p1.y) * hauteurf),
					 static_cast<int>(p2.x * largeurf),
					 static_cast<int>((1.0f - p2.y) * hauteurf));

	/* dessine les points */
	stylo_colore = QPen(COULEURS_COURBES[m_mode]);
	stylo_colore.setWidthF(5.0);

	auto stylo_blanc = QPen(Qt::yellow);
	stylo_blanc.setWidthF(5.0);

	for (const PointBezier &point : m_courbe->points) {
		if (&point == m_point_courant) {
			painter.setPen(stylo_blanc);
		}
		else {
			painter.setPen(stylo_colore);
		}

		painter.drawPoint(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

#ifdef DEBOGAGE_CONTROLES
		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE1].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CONTROLE1].y) * hauteurf));

		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
#endif
	}

#ifdef DEBOGAGE_CONTROLES
	/* dessine les connexions entre points de controles et points centraux */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(1.0f);

	painter.setPen(stylo);

	for (const PointBezier &point : m_courbe->points) {
		painter.drawLine(static_cast<int>(point.co[POINT_CONTROLE1].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE1].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

		painter.drawLine(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
	}
#endif
}

void ControleCourbeCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	/* fenêtre de 10 pixels */
	const auto &taille_fenetre_x = 10.0f / static_cast<float>(size().width());
	const auto &taille_fenetre_y = 10.0f / static_cast<float>(size().height());

	PointBezier *point_courant = nullptr;
	m_point_selectionne = false;
	m_type_point = -1;

	for (PointBezier &point : m_courbe->points) {
		auto dist_x = point.co[POINT_CENTRE].x - x;

		if (std::abs(dist_x) > taille_fenetre_x) {
			continue;
		}

		auto dist_y = point.co[POINT_CENTRE].y - (1.0f - y);

		if (std::abs(dist_y) < taille_fenetre_y) {
			point_courant = &point;
		}
	}

	if (point_courant != nullptr) {
		if (point_courant != m_courbe->point_courant) {
			m_courbe->point_courant = point_courant;
			Q_EMIT point_change();
			update();
		}

		m_point_courant = point_courant;
		m_point_selectionne = true;
	}
}

void ControleCourbeCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (m_point_selectionne) {
		const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
		const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

		m_point_courant->co[POINT_CENTRE].x = std::max(0.0f, std::min(1.0f, x));
		m_point_courant->co[POINT_CENTRE].y = std::max(0.0f, std::min(1.0f, 1.0f - y));

		calcule_controles_courbe(*m_courbe);

		update();

		Q_EMIT position_changee(m_point_courant->co[POINT_CENTRE].x,
								m_point_courant->co[POINT_CENTRE].y);
	}
}

void ControleCourbeCouleur::mouseReleaseEvent(QMouseEvent *)
{
	m_point_selectionne = false;
}

void ControleCourbeCouleur::mouseDoubleClickEvent(QMouseEvent *event)
{
	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	ajoute_point_courbe(*m_courbe, x, 1.0f - y);
	calcule_controles_courbe(*m_courbe);

	update();
}

void ControleCourbeCouleur::ajourne_position_x(float v)
{
	m_point_courant->co[POINT_CENTRE].x = v;
	calcule_controles_courbe(*m_courbe);
	update();
}

void ControleCourbeCouleur::ajourne_position_y(float v)
{
	m_point_courant->co[POINT_CENTRE].y = v;
	calcule_controles_courbe(*m_courbe);
	update();
}
