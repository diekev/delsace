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

#include "controle_rampe_couleur.h"

#include <QMouseEvent>
#include <QPainter>

ControleRampeCouleur::ControleRampeCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 256);

	m_rampe.points.push_back(PointRampeCouleur{0.0f, {0.0f, 0.0f, 0.0f, 1.0f}});
	m_rampe.points.push_back(PointRampeCouleur{0.5f, {0.0f, 1.0f, 0.0f, 1.0f}});
	m_rampe.points.push_back(PointRampeCouleur{1.0f, {1.0f, 1.0f, 1.0f, 1.0f}});
}

void ControleRampeCouleur::paintEvent(QPaintEvent *)
{
	const auto &largeur = size().width();
	const auto &hauteur = size().height();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	/* dessine l'arrière plan À FAIRE : échiquier */
	painter.setBrush(QColor(127, 127, 127));
	painter.drawRect(this->rect());

	/* dessine le dégradé */
	QLinearGradient degrade(QPoint(0, 0), QPoint(largeur, 0));

	for (const auto &point : m_rampe.points) {
		degrade.setColorAt(point.position, QColor(point.couleur[0] * 255, point.couleur[1] * 255, point.couleur[2] * 255, point.couleur[3] * 255));
	}

	painter.setBrush(QBrush(degrade));
	painter.drawRect(this->rect());

	/* dessine les lignes de contrôles */
	painter.setPen(QColor(0, 0, 0));

	for (const auto &point : m_rampe.points) {
		painter.drawLine(largeur * point.position, 0, largeur * point.position, hauteur);
	}
}

void ControleRampeCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &x = event->pos().x() / static_cast<float>(size().width());

	/* fenêtre de 5 pixels */
	const auto &taille_fenetre = 5.0f / static_cast<float>(size().width());

	point_courant = nullptr;

	for (auto &point : m_rampe.points) {
		auto dist_x = point.position - x;

		if (std::abs(dist_x) < taille_fenetre) {
			point_courant = &point;
		}
	}
}

void ControleRampeCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (point_courant != nullptr) {
		const auto &x = event->pos().x() / static_cast<float>(size().width());

		point_courant->position = std::max(0.0f, std::min(1.0f, x));
		update();
	}
}

void ControleRampeCouleur::mouseReleaseEvent(QMouseEvent *)
{
	point_courant = nullptr;
}
