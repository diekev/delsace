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

#include "controle_roue_couleur.h"

#include <cmath>
#include <QMouseEvent>
#include <QPainter>

/* le diamètre de la roue de couleur relatif à la taille de sa boîte englobante */
static constexpr auto DIAMETRE_ROUE = 0.72;
static constexpr auto RAYON_ROUE = DIAMETRE_ROUE * 0.5;
static constexpr auto DECALAGE_ROUE = (1.0 - DIAMETRE_ROUE) * 0.5;

/* le diamètre du cercle relatif à la taille de sa boîte englobante */
static constexpr auto DIAMETRE_CERCLE = 0.86;
static constexpr auto DECALAGE_CERCLE = (1.0 - DIAMETRE_CERCLE) * 0.5;

/* taille de la cible relative de sa boîte englobante */
static constexpr auto TAILLE_CIBLE = 0.83;
static constexpr auto DECALAGE_CIBLE = (1.0 - TAILLE_CIBLE);

ControleRoueCouleur::ControleRoueCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(256, 256);
}

void ControleRoueCouleur::paintEvent(QPaintEvent *)
{
	const auto hauteur = size().height();
	const auto largeur = size().width();

	QPen stylo;
	QBrush pinceaux(QColor(40, 40, 40));

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(this->rect(), pinceaux);

	/* dessine le cercle de contour */
	stylo.setColor(QColor(0, 0, 0));
	stylo.setWidthF(2.0);

	painter.setPen(stylo);

	painter.drawEllipse(largeur * DECALAGE_CERCLE,
						hauteur * DECALAGE_CERCLE,
						largeur * DIAMETRE_CERCLE,
						hauteur * DIAMETRE_CERCLE);

	/* dessine la cible */
	painter.setPen(QColor(55, 55, 55));

	const auto decalage_hauteur = hauteur / 2.0;
	const auto decalage_largeur = largeur / 2.0;
	const auto hauteur_cible = hauteur * TAILLE_CIBLE;
	const auto largeur_cible = largeur * TAILLE_CIBLE;

	painter.drawLine(decalage_largeur, hauteur * DECALAGE_CIBLE, decalage_largeur, hauteur_cible);
	painter.drawLine(largeur * DECALAGE_CIBLE, decalage_hauteur, largeur_cible, decalage_hauteur);

	/* dessine le cercle de couleur */
	QConicalGradient degrade(decalage_hauteur, decalage_largeur, 0.0);
	degrade.setColorAt(0.000, QColor::fromHsv(  0, m_distance * 255, 255));
	degrade.setColorAt(0.125, QColor::fromHsv( 45, m_distance * 255, 255));
	degrade.setColorAt(0.250, QColor::fromHsv( 90, m_distance * 255, 255));
	degrade.setColorAt(0.375, QColor::fromHsv(135, m_distance * 255, 255));
	degrade.setColorAt(0.500, QColor::fromHsv(180, m_distance * 255, 255));
	degrade.setColorAt(0.625, QColor::fromHsv(225, m_distance * 255, 255));
	degrade.setColorAt(0.750, QColor::fromHsv(270, m_distance * 255, 255));
	degrade.setColorAt(0.875, QColor::fromHsv(315, m_distance * 255, 255));
	degrade.setColorAt(1.000, QColor::fromHsv(359, m_distance * 255, 255));

	painter.setPen(QPen(QBrush(degrade), 0.03125 * hauteur));

	painter.drawArc(largeur * DECALAGE_ROUE,
					hauteur * DECALAGE_ROUE,
					largeur * DIAMETRE_ROUE,
					hauteur * DIAMETRE_ROUE,
					0.0,
					5760.0);

	/* dessine le contour du selecteur */
	stylo.setColor(QColor::fromHsv(m_angle, m_distance * 255, 255));
	//stylo.setColor(QColor(144, 144, 144));
	stylo.setWidthF(3.0);

	painter.setPen(stylo);

	painter.drawEllipse(largeur * (m_controle_x - rayon_controle),
						hauteur * (m_controle_y - rayon_controle),
						largeur * diametre_controle,
						hauteur * diametre_controle);
}

void ControleRoueCouleur::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MouseButton::LeftButton) {
		ajourne_position(event->pos().x(), event->pos().y());
		souris_pressee = true;
		update();
	}
}

void ControleRoueCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (souris_pressee) {
		ajourne_position(event->pos().x(), event->pos().y());
		update();
	}
}

void ControleRoueCouleur::mouseReleaseEvent(QMouseEvent *)
{
	souris_pressee = false;
}

void ControleRoueCouleur::ajourne_position(int x, int y)
{
	m_controle_x = x / static_cast<double>(this->rect().width());
	m_controle_y = y / static_cast<double>(this->rect().height());

	const auto controle_x = m_controle_x - 0.5f;
	const auto controle_y = 1.0f - m_controle_y - 0.5f;

	m_distance = std::sqrt(controle_x * controle_x + controle_y * controle_y) / RAYON_ROUE;

	auto angle = std::atan2(controle_y, controle_x);

	/* restreint la position du contrôle à l'intérieur du cercle de sélection */
	m_distance = std::min(m_distance, 1.0f);
	m_controle_x = 0.5f + RAYON_ROUE * m_distance * std::cos(angle);
	m_controle_y = 0.5f - RAYON_ROUE * m_distance * std::sin(angle);

	if (angle < 0.0f) {
		angle = 2.0 * M_PI + angle;
	}

	m_angle = static_cast<int>(angle * 180 / M_PI) % 360;
	//		const auto hauteur = size().height();
	//		const auto largeur = size().width();
	//		std::cerr << "Centre : " << hauteur / 2 << ',' << largeur / 2 << '\n';
	//		std::cerr << "Position : " << x << ',' << y << '\n';
	//		std::cerr << "Angle : " << m_angle << '\n';
	//		std::cerr << "Distance : " << m_distance << '\n';
}
