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

/* ************************************************************************** */

#undef DEBOGAGE_CONTROLES

auto operator+(const Point &p1, const Point &p2)
{
	return Point{
		p1.x + p2.x,
		p1.y + p2.y
	};
}

auto operator-(const Point &p1, const Point &p2)
{
	return Point{
		p1.x - p2.x,
		p1.y - p2.y
	};
}

auto operator*(const Point &p1, const float f)
{
	return Point{
		p1.x * f,
		p1.y * f
	};
}

auto longueur(const Point &p)
{
	return std::sqrt(p.x * p.x + p.y * p.y);
}

auto longueur(const Point &p1, const Point &p2)
{
	return longueur(p1 - p2);
}

/* ************************************************************************** */

/**
 * Voir http://whizkidtech.redprince.net/bezier/circle/
 * et rB290361776e5858b3903a83c0cddf722b8340e699
 */
static constexpr auto KAPPA = 2.5614f;

void calcule_controles_courbe(CourbeBezier &courbe)
{
	const auto nombre_points = courbe.points.size();

	/* fais pointer les controles vers le centre des points environnants */
	for (size_t i = 0; i < nombre_points; ++i) {
		auto &p = courbe.points[i];

		if (i == 0) {
			p.co[POINT_CONTROLE2] = courbe.points[i + 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE1] = p.co[POINT_CENTRE] * 2.0f - p.co[POINT_CONTROLE2];
		}
		else if (i == nombre_points - 1) {
			p.co[POINT_CONTROLE1] = courbe.points[i - 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE2] = p.co[POINT_CENTRE] * 2.0f - p.co[POINT_CONTROLE1];
		}
		else {
			p.co[POINT_CONTROLE1] = courbe.points[i - 1].co[POINT_CENTRE];
			p.co[POINT_CONTROLE2] = courbe.points[i + 1].co[POINT_CENTRE];
		}
	}

	/* corrige les controles pour qu'ils soient tangeants à la courbe */
	for (auto &p : courbe.points) {
		auto &p0 = p.co[POINT_CONTROLE1];
		const auto &p1 = p.co[POINT_CENTRE];
		auto &p2 = p.co[POINT_CONTROLE2];

		auto vec_a = Point{p1.x - p0.x, p1.y - p0.y};
		auto vec_b = Point{p2.x - p1.x, p2.y - p1.y};

		auto len_a = longueur(vec_a);
		auto len_b = longueur(vec_b);

		if (len_a == 0.0f) {
			len_a = 1.0f;
		}

		if (len_b == 0.0f) {
			len_b = 1.0f;
		}

		Point tangeante;
		tangeante.x = vec_b.x / len_b + vec_a.x / len_a;
		tangeante.y = vec_b.y / len_b + vec_a.y / len_a;

		auto len_t = longueur(tangeante) * KAPPA;

		if (len_t != 0.0f) {
			// point a
			len_a /= len_t;
			p0 = p1 + tangeante * -len_a;

			// point b
			len_b /= len_t;
			p2 = p1 + tangeante *  len_b;
		}
	}

	/* corrige premier et dernier point pour pointer vers le controle le plus proche */
	if (nombre_points > 2) {
		// premier
		{
			auto &p0 = courbe.points[0];

			auto hlen = longueur(p0.co[POINT_CENTRE], p0.co[POINT_CONTROLE2]); /* original handle length */
			/* clip handle point */
			auto vec = courbe.points[1].co[POINT_CONTROLE1];

			if (vec.x < p0.co[POINT_CENTRE].x) {
				vec.x = p0.co[POINT_CENTRE].x;
			}

			vec = vec - p0.co[POINT_CENTRE];
			auto nlen = longueur(vec);

			if (nlen > 1e-6f) {
				vec = vec * (hlen / nlen);
				p0.co[POINT_CONTROLE2] = vec + p0.co[POINT_CENTRE];
				p0.co[POINT_CONTROLE1] = p0.co[POINT_CENTRE] - vec;
			}
		}

		// dernier
		{
			auto &p0 = courbe.points[nombre_points - 1];

			auto hlen = longueur(p0.co[POINT_CENTRE], p0.co[POINT_CONTROLE1]); /* original handle length */
			/* clip handle point */
			auto vec = courbe.points[nombre_points - 2].co[POINT_CONTROLE2];

			if (vec.x > p0.co[POINT_CENTRE].x) {
				vec.x = p0.co[POINT_CENTRE].x;
			}

			vec = vec - p0.co[POINT_CENTRE];
			auto nlen = longueur(vec);

			if (nlen > 1e-6f) {
				vec = vec * (hlen / nlen);
				p0.co[POINT_CONTROLE1] = p0.co[POINT_CENTRE] + vec;
				p0.co[POINT_CONTROLE2] = p0.co[POINT_CENTRE] - vec;
			}
		}
	}

	construit_table_courbe(courbe);
}

ControleCourbeCouleur::ControleCourbeCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(300, 300);

	ajoute_point_courbe(m_courbe, 0.0f, 0.0f);
	ajoute_point_courbe(m_courbe, 1.0f, 1.0f);
	ajoute_point_courbe(m_courbe, 0.6f, 0.4f);
	ajoute_point_courbe(m_courbe, 0.4f, 0.2f);

	calcule_controles_courbe(m_courbe);
}

enum {
	COURBE_ROUGE  = 0,
	COURBE_VERTE  = 1,
	COURBE_BLEUE  = 2,
	COURBE_VALEUR = 3,
};

static QColor COULEURS_COURBES[4] = {
	QColor(255, 0, 0),
	QColor(0, 255, 0),
	QColor(0, 0, 255),
	QColor(255, 255, 255),
};

void ControleCourbeCouleur::paintEvent(QPaintEvent *)
{
	const auto role = COURBE_VALEUR;
	const auto hauteur = size().height();
	const auto largeur = size().width();

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
		painter.drawLine(l, 0, l, hauteur);
		painter.drawLine(0, h, largeur, h);

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
		painter.drawLine(l, 0, l, hauteur);
		painter.drawLine(0, h, largeur, h);

		h += decalage_hauteur;
		l += decalage_largeur;
	}

	/* dessine la courbe */

	auto stylo_colore = QPen(COULEURS_COURBES[role]);
	stylo_colore.setWidthF(1.0f);
	painter.setPen(stylo_colore);

	auto p1 = m_courbe.extension_min.co[POINT_CENTRE];
	auto p2 = m_courbe.table[0];

	painter.drawLine(p1.x * largeur,
					 (1.0f - p1.y) * hauteur,
					 p2.x * largeur,
					 (1.0f - p2.y) * hauteur);

	for (size_t i = 0; i < m_courbe.table.size() - 1; ++i) {
		p1 = m_courbe.table[i];
		p2 = m_courbe.table[i + 1];

		painter.drawLine(p1.x * largeur,
						 (1.0f - p1.y) * hauteur,
						 p2.x * largeur,
						 (1.0f - p2.y) * hauteur);
	}

	p1 = m_courbe.table[m_courbe.table.size() - 1];
	p2 = m_courbe.extension_max.co[POINT_CENTRE];

	painter.drawLine(p1.x * largeur,
					 (1.0f - p1.y) * hauteur,
					 p2.x * largeur,
					 (1.0f - p2.y) * hauteur);

	/* dessine les points */
	stylo_colore = QPen(COULEURS_COURBES[role]);
	stylo_colore.setWidthF(5.0f);

	auto stylo_blanc = QPen(Qt::yellow);
	stylo_blanc.setWidthF(5.0f);

	for (const PointBezier &point : m_courbe.points) {
		if (&point == m_point_courant) {
			painter.setPen(stylo_blanc);
		}
		else {
			painter.setPen(stylo_colore);
		}

		painter.drawPoint(point.co[POINT_CENTRE].x * largeur,
						  (1.0f - point.co[POINT_CENTRE].y) * hauteur);

#ifdef DEBOGAGE_CONTROLES
		painter.drawPoint(point.co[POINT_CONTROLE1].x * largeur,
						  (1.0f - point.co[POINT_CONTROLE1].y) * hauteur);

		painter.drawPoint(point.co[POINT_CONTROLE2].x * largeur,
						  (1.0f - point.co[POINT_CONTROLE2].y) * hauteur);
#endif
	}

#ifdef DEBOGAGE_CONTROLES
	/* dessine les connexions entre points de controles et points centraux */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(1.0f);

	painter.setPen(stylo);

	for (const PointBezier &point : m_courbe.points) {
		painter.drawLine(point.co[POINT_CONTROLE1].x * largeur,
						 (1.0f - point.co[POINT_CONTROLE1].y) * hauteur,
						 point.co[POINT_CENTRE].x * largeur,
						 (1.0f - point.co[POINT_CENTRE].y) * hauteur);

		painter.drawLine(point.co[POINT_CENTRE].x * largeur,
						 (1.0f - point.co[POINT_CENTRE].y) * hauteur,
						 point.co[POINT_CONTROLE2].x * largeur,
						 (1.0f - point.co[POINT_CONTROLE2].y) * hauteur);
	}
#endif
}

void ControleCourbeCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &x = event->pos().x() / static_cast<float>(size().width());
	const auto &y = event->pos().y() / static_cast<float>(size().height());

	/* fenêtre de 10 pixels */
	const auto &taille_fenetre_x = 10.0f / static_cast<float>(size().width());
	const auto &taille_fenetre_y = 10.0f / static_cast<float>(size().height());

	m_point_courant = nullptr;
	m_type_point = -1;

	for (PointBezier &point : m_courbe.points) {
		auto dist_x = point.co[POINT_CENTRE].x - x;

		if (std::abs(dist_x) > taille_fenetre_x) {
			continue;
		}

		auto dist_y = point.co[POINT_CENTRE].y - (1.0f - y);

		if (std::abs(dist_y) < taille_fenetre_y) {
			m_point_courant = &point;
		}
	}
}

void ControleCourbeCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (m_point_courant != nullptr) {
		const auto &x = event->pos().x() / static_cast<float>(size().width());
		const auto &y = event->pos().y() / static_cast<float>(size().height());

		m_point_courant->co[POINT_CENTRE].x = std::max(0.0f, std::min(1.0f, x));
		m_point_courant->co[POINT_CENTRE].y = std::max(0.0f, std::min(1.0f, 1.0f - y));

		calcule_controles_courbe(m_courbe);

		update();
	}
}

void ControleCourbeCouleur::mouseReleaseEvent(QMouseEvent *)
{
	//m_point_courant = nullptr;
}

void ControleCourbeCouleur::mouseDoubleClickEvent(QMouseEvent *event)
{
	const auto &x = event->pos().x() / static_cast<float>(size().width());
	const auto &y = event->pos().y() / static_cast<float>(size().height());

	ajoute_point_courbe(m_courbe, x, 1.0f - y);
	calcule_controles_courbe(m_courbe);

	update();
}
