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

#include "types/outils.h"
#include "types/rampe_couleur.h"

/* ************************************************************************** */

ControleRampeCouleur::ControleRampeCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 256);
	auto metriques = this->fontMetrics();
	setFixedHeight(static_cast<int>(static_cast<float>(metriques.height()) * 4.0f));
}

void ControleRampeCouleur::installe_rampe(RampeCouleur *rampe)
{
	m_rampe = rampe;
	m_point_courant = trouve_point_selectionne(*m_rampe);
}

void ControleRampeCouleur::paintEvent(QPaintEvent *)
{
	const auto &largeur = size().width();
	const auto &metriques = this->fontMetrics();
	const auto &hauteur_fonte = metriques.height();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	/* dessine le dégradé */
	const auto largeur_degrade = largeur - hauteur_fonte * 2;
	QLinearGradient degrade(QPoint(hauteur_fonte, 0),
							QPoint(hauteur_fonte + largeur_degrade, 0));

	for (auto i = 0.0; i <= 32.0; i += 1.0) {
		auto couleur = evalue_rampe_couleur(*m_rampe, static_cast<float>(i / 32.0));
		degrade.setColorAt(i / 32.0, converti_couleur(couleur));
	}

	painter.setBrush(QBrush(degrade));
	painter.drawRect(hauteur_fonte,
					 0,
					 largeur_degrade,
					 hauteur_fonte * 2);

	/* dessine les lignes de contrôles */
#if 0
	painter.setPen(QColor(0, 0, 0));

	for (const auto &point : m_rampe.points) {
		painter.drawLine(largeur_degrade * point.position + hauteur_fonte,
						 0,
						 largeur_degrade * point.position + hauteur_fonte,
						 hauteur);
	}
#endif

	/* dessine les poignées de contrôles */
	QPoint points_triangle[3];

	for (const auto &point : m_rampe->points) {
		painter.setBrush(QBrush(converti_couleur(point.couleur)));

		{
			auto x = static_cast<int>(static_cast<float>(largeur_degrade) * point.position) + hauteur_fonte;
			auto y = static_cast<int>(static_cast<float>(hauteur_fonte) * 2.5f);

			points_triangle[0] = QPoint( x - hauteur_fonte / 2, y);
			points_triangle[2] = QPoint( x + hauteur_fonte / 2, y);
			points_triangle[1] = QPoint( x, hauteur_fonte * 2);

			painter.drawPolygon(points_triangle, 3);
		}

		{
			auto l = hauteur_fonte;
			auto h = hauteur_fonte;
			auto x = static_cast<float>(largeur_degrade) * point.position + static_cast<float>(hauteur_fonte) * 0.5f;
			auto y = static_cast<float>(hauteur_fonte) * 2.5f;

			painter.drawRect(static_cast<int>(x), static_cast<int>(y), l, h);
		}
	}
}

float ControleRampeCouleur::position_degrade(float x)
{
	const auto &largeur = size().width();
	const auto &metriques = this->fontMetrics();
	const auto &hauteur_fonte = metriques.height();
	const auto largeur_degrade = largeur - hauteur_fonte * 2;
	const auto echelle = static_cast<float>(largeur) / static_cast<float>(largeur_degrade);

	x = x / static_cast<float>(largeur);
	x = 2.0f * x - 1.0f;
	x *= echelle;
	x = x * 0.5f + 0.5f;

	return x;
}

void ControleRampeCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &largeur = size().width();
	const auto &metriques = this->fontMetrics();
	const auto &hauteur_fonte = metriques.height();
	const auto &taille_fenetre = (static_cast<float>(hauteur_fonte) * 0.5f) / static_cast<float>(largeur);
	const auto &x = position_degrade(static_cast<float>(event->pos().x()));

	PointRampeCouleur *point_courant = nullptr;
	m_point_selectionne = false;

	for (auto &point : m_rampe->points) {
		auto dist_x = point.position - x;

		if (std::abs(dist_x) < taille_fenetre) {
			point_courant = &point;
		}
	}

	if (point_courant != nullptr) {
		if (point_courant != m_point_courant) {
			if (m_point_courant != nullptr) {
				m_point_courant->selectionne = false;
			}
		}

		m_point_courant = point_courant;
		m_point_courant->selectionne = true;
		m_point_selectionne = true;
		Q_EMIT point_change();
		update();
	}
}

void ControleRampeCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (m_point_selectionne) {
		auto x = position_degrade(static_cast<float>(event->pos().x()));
		x = std::max(0.0f, std::min(1.0f, x));

		m_point_courant->position = x;
		tri_points_rampe(*m_rampe);
		m_point_courant = trouve_point_selectionne(*m_rampe);
		Q_EMIT position_modifie(x);
		update();
	}
}

void ControleRampeCouleur::mouseReleaseEvent(QMouseEvent *)
{
	m_point_selectionne = false;
}

void ControleRampeCouleur::mouseDoubleClickEvent(QMouseEvent *event)
{
	const auto &x = position_degrade(static_cast<float>(event->pos().x()));

	auto couleur = couleur32{1.0f, 0.0f, 1.0f, 1.0f};
	ajoute_point_rampe(*m_rampe, x, couleur);
	update();

	Q_EMIT controle_ajoute();
}

void ControleRampeCouleur::ajourne_position(float x)
{
	if (m_point_courant != nullptr) {
		m_point_courant->position = x;
	}

	update();
}
