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

	painter.drawEllipse(QRectF(DECALAGE_CERCLE,
							   DECALAGE_CERCLE,
							   DIAMETRE_CERCLE,
							   DIAMETRE_CERCLE));

	/* dessine la cible */
	painter.setPen(QColor(55, 55, 55));

	const auto decalage_hauteur = static_cast<int>(hauteur / 2.0);
	const auto decalage_largeur = static_cast<int>(largeur / 2.0);
	const auto hauteur_cible = static_cast<int>(hauteur * TAILLE_CIBLE);
	const auto largeur_cible = static_cast<int>(largeur * TAILLE_CIBLE);

	painter.drawLine(decalage_largeur, static_cast<int>(hauteur * DECALAGE_CIBLE), decalage_largeur, hauteur_cible);
	painter.drawLine(static_cast<int>(largeur * DECALAGE_CIBLE), decalage_hauteur, largeur_cible, decalage_hauteur);

	/* dessine le cercle de couleur */
	auto const dist = static_cast<int>(m_distance * 255.0);
	QConicalGradient degrade(decalage_hauteur, decalage_largeur, 0.0);
	degrade.setColorAt(0.000, QColor::fromHsv(  0, dist, 255));
	degrade.setColorAt(0.125, QColor::fromHsv( 45, dist, 255));
	degrade.setColorAt(0.250, QColor::fromHsv( 90, dist, 255));
	degrade.setColorAt(0.375, QColor::fromHsv(135, dist, 255));
	degrade.setColorAt(0.500, QColor::fromHsv(180, dist, 255));
	degrade.setColorAt(0.625, QColor::fromHsv(225, dist, 255));
	degrade.setColorAt(0.750, QColor::fromHsv(270, dist, 255));
	degrade.setColorAt(0.875, QColor::fromHsv(315, dist, 255));
	degrade.setColorAt(1.000, QColor::fromHsv(359, dist, 255));

	painter.setPen(QPen(QBrush(degrade), 0.03125 * hauteur));

	painter.drawArc(QRectF(DECALAGE_ROUE,
						   DECALAGE_ROUE,
						   DIAMETRE_ROUE,
						   DIAMETRE_ROUE),
					0.0,
					5760.0);

	/* dessine le contour du selecteur */
	stylo.setColor(QColor::fromHsv(m_angle, dist, 255.0));
	//stylo.setColor(QColor(144, 144, 144));
	stylo.setWidthF(3.0);

	painter.setPen(stylo);

	painter.drawEllipse(QRectF((m_controle_x - rayon_controle),
							   (m_controle_y - rayon_controle),
							   diametre_controle,
							   diametre_controle));
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
	m_controle_x = static_cast<double>(x) / static_cast<double>(this->rect().width());
	m_controle_y = static_cast<double>(y) / static_cast<double>(this->rect().height());

	const auto controle_x = m_controle_x - 0.5;
	const auto controle_y = 1.0 - m_controle_y - 0.5;

	m_distance = std::sqrt(controle_x * controle_x + controle_y * controle_y) / RAYON_ROUE;

	auto angle = std::atan2(controle_y, controle_x);

	/* restreint la position du contrôle à l'intérieur du cercle de sélection */
	m_distance = std::min(m_distance, 1.0);
	m_controle_x = 0.5 + RAYON_ROUE * m_distance * std::cos(angle);
	m_controle_y = 0.5 - RAYON_ROUE * m_distance * std::sin(angle);

	if (angle < 0.0) {
		angle = 2.0 * M_PI + angle;
	}

	m_angle = static_cast<int>(angle * 180.0 / M_PI) % 360;
	//		const auto hauteur = size().height();
	//		const auto largeur = size().width();
	//		std::cerr << "Centre : " << hauteur / 2 << ',' << largeur / 2 << '\n';
	//		std::cerr << "Position : " << x << ',' << y << '\n';
	//		std::cerr << "Angle : " << m_angle << '\n';
	//		std::cerr << "Distance : " << m_distance << '\n';
}
