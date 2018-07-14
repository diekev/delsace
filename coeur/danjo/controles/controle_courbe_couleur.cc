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

auto interp(const point &p1, const point &p2, const double &facteur)
{
	return point{
		p1.x * (facteur) + p2.x * (1.0 - facteur),
		p1.y * (facteur) + p2.y * (1.0 - facteur)
	};
}

auto operator+(const point &p1, const point &p2)
{
	return point{
		p1.x + p2.x,
		p1.y + p2.y
	};
}

auto &operator<<(std::ostream &os, const point &p)
{
	os << '[' << p.x << ',' << p.y << ']';
	return os;
}

/* ************************************************************************** */

ControleCourbeCouleur::ControleCourbeCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(300, 300);

	c.points.push_back({0.0, 0.0});
	c.points.push_back({0.5, 0.5});
	c.points.push_back({1.0, 1.0});
}

void ControleCourbeCouleur::paintEvent(QPaintEvent *)
{
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

	/* dessine les courbes */

	/* principale */
#if 0
	painter.setPen(QColor(255, 255, 255));
	painter.drawLine(0, hauteur, largeur, 0);

	/* bleue */
	painter.setPen(QColor(0, 0, 255));
	painter.drawLine(0, hauteur, largeur, 0);

	/* verte */
	painter.setPen(QColor(0, 255, 0));
	painter.drawLine(0, hauteur, largeur, 0);

	/* rouge */
	painter.setPen(QColor(255, 0, 0));
	painter.drawLine(0, hauteur, largeur, 0);
#else

	for (size_t i = 0; i < c.points.size() - 1; ++i) {
		const auto &p1 = c.points[i];
		const auto &p2 = c.points[i + 1];

		painter.setPen(QColor(255, 0, 0));
		painter.drawLine((1.0 - p1.x) * largeur, (p1.y) * hauteur,
						 (1.0 - p2.x) * largeur, (p2.y) * hauteur);

		const auto &p3 = point{std::min(p1.y, p2.y), std::max(p1.x, p2.x)};

		const auto &p4 = interp(interp(p1, p3, 0.5), interp(p3, p2, 0.5), 0.5);

		std::cerr << "P1 : " << p1 << '\n';
		std::cerr << "P2 : " << p2 << '\n';
		std::cerr << "P3 : " << p3 << '\n';
		std::cerr << "P4 : " << p4 << '\n';

		painter.setPen(QColor(255, 255, 255));
		painter.drawLine((1.0 - p1.x) * largeur, (p1.y) * hauteur,
						 (1.0 - p4.x) * largeur, (p4.y) * hauteur);
		painter.drawLine((1.0 - p4.x) * largeur, (p4.y) * hauteur,
						 (1.0 - p2.x) * largeur, (p2.y) * hauteur);
	}

	/* dessine les points */
	for (const auto &point : c.points) {
		QPen stylo;
		stylo.setColor(QColor(255, 0, 0));
		stylo.setWidthF(5.0f);
		painter.setPen(stylo);
		painter.drawEllipse((1.0 - point.x) * largeur - 2, point.y * hauteur - 2, 5, 5);
	}
#endif
}

void ControleCourbeCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &x = event->pos().x() / static_cast<float>(size().width());
	const auto &y = event->pos().y() / static_cast<float>(size().height());

	point_courant = nullptr;

	for (auto &point : c.points) {
		auto dist_x = point.x - (1.0f - x);
		auto dist_y = point.y - y;
		auto dist = std::sqrt(dist_x * dist_x + dist_y * dist_y);

		if (dist < 0.1f) {
			point_courant = &point;
		}
	}
}

void ControleCourbeCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (point_courant != nullptr) {
		const auto &x = event->pos().x() / static_cast<float>(size().width());
		const auto &y = event->pos().y() / static_cast<float>(size().height());

		point_courant->x = std::max(0.0f, std::min(1.0f, 1.0f - x));
		point_courant->y = std::max(0.0f, std::min(1.0f, y));
		update();
	}
}

void ControleCourbeCouleur::mouseReleaseEvent(QMouseEvent *)
{
	point_courant = nullptr;
}
