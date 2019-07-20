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

#include "controle_spectre_couleur.h"

#include <QMouseEvent>
#include <QPainter>

/* ************************************************************************** */

static constexpr auto DEBUT_SPECTRE = 380.0;
static constexpr auto FIN_SPECTRE = 700.0;
static constexpr auto TAILLE_SPECTRE = FIN_SPECTRE - DEBUT_SPECTRE;

constexpr auto position_spectre(const double spectre)
{
	return (spectre - DEBUT_SPECTRE) / TAILLE_SPECTRE;
}

/* ************************************************************************** */

ControleSpectreCouleur::ControleSpectreCouleur(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 256);

	ajoute_point_courbe(m_courbe, 0.0f, 0.0f);
	ajoute_point_courbe(m_courbe, 0.5f, 0.0f);
	ajoute_point_courbe(m_courbe, 1.0f, 0.0f);
}

void ControleSpectreCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QLinearGradient degrade(QPoint(0, 0), QPoint(size().width(), 0));
	/* couleurs extraites de https://fr.wikipedia.org/wiki/Spectre_visible */
	degrade.setColorAt(position_spectre(DEBUT_SPECTRE), QColor(0, 0, 0));
	degrade.setColorAt(position_spectre(445.0), QColor(39, 0, 91));
	degrade.setColorAt(position_spectre(455.0), QColor(42, 0, 123));
	degrade.setColorAt(position_spectre(470.0), QColor(0, 47, 131));
	degrade.setColorAt(position_spectre(480.0), QColor(0, 71, 105));
	degrade.setColorAt(position_spectre(485.0), QColor(0, 81, 98));
	degrade.setColorAt(position_spectre(500.0), QColor(0, 114, 95));
	degrade.setColorAt(position_spectre(525.0), QColor(0, 175, 108));
	degrade.setColorAt(position_spectre(555.0), QColor(89, 192, 0));
	degrade.setColorAt(position_spectre(574.0), QColor(202, 179, 0));
	degrade.setColorAt(position_spectre(577.0), QColor(210, 169, 0));
	degrade.setColorAt(position_spectre(582.0), QColor(215, 147, 0));
	degrade.setColorAt(position_spectre(586.0), QColor(222, 132, 0));
	degrade.setColorAt(position_spectre(590.0), QColor(231, 119, 0));
	degrade.setColorAt(position_spectre(600.0), QColor(245, 80, 0));
	degrade.setColorAt(position_spectre(615.0), QColor(234, 0, 33));
	degrade.setColorAt(position_spectre(650.0), QColor(122, 0, 34));
	degrade.setColorAt(position_spectre(FIN_SPECTRE), QColor(0, 0, 0));

	painter.setBrush(QBrush(degrade));

	painter.drawRect(this->rect());

	/* dessine la courbe : notons que les coordonnées y sont inversées afin que
	 * qu'une coordonnée égale à 0.0 soit dessinée 'en-bas' */

	/* dessine les points */
	auto stylo = QPen(Qt::yellow);
	stylo.setWidthF(5.0);

	painter.setPen(stylo);

	auto largeur = size().width();
	auto hauteur = size().height();
	auto const hauteurf = static_cast<float>(hauteur);
	auto const largeurf = static_cast<float>(largeur);

	for (const PointBezier &point : m_courbe.points) {
		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE1].x),
						  static_cast<int>(1.0f - point.co[POINT_CONTROLE1].y));

		painter.drawPoint(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
	}

	/* dessine les connexions entre points de controles et points centraux */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(1.0);

	painter.setPen(stylo);

	for (const PointBezier &point : m_courbe.points) {
		painter.drawLine(static_cast<int>(point.co[POINT_CONTROLE1].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE1].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

		painter.drawLine(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
	}

	/* dessine la courbe */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(2.0);

	painter.setPen(stylo);

	auto p1 = m_courbe.extension_min.co[POINT_CENTRE];
	auto p2 = m_courbe.table[0];

	painter.drawLine(static_cast<int>(p1.x * largeurf),
					 static_cast<int>((1.0f - p1.y) * hauteurf),
					 static_cast<int>(p2.x * largeurf),
					 static_cast<int>((1.0f - p2.y) * hauteurf));

	for (auto i = 0; i < m_courbe.table.taille() - 1; ++i) {
		p1 = m_courbe.table[i];
		p2 = m_courbe.table[i + 1];

		painter.drawLine(static_cast<int>(p1.x * largeurf),
						 static_cast<int>((1.0f - p1.y) * hauteurf),
						 static_cast<int>(p2.x * largeurf),
						 static_cast<int>((1.0f - p2.y) * hauteurf));
	}

	p1 = m_courbe.table[m_courbe.table.taille() - 1];
	p2 = m_courbe.extension_max.co[POINT_CENTRE];

	painter.drawLine(static_cast<int>(p1.x * largeurf),
					 static_cast<int>((1.0f - p1.y) * hauteurf),
					 static_cast<int>(p2.x * largeurf),
					 static_cast<int>((1.0f - p2.y) * hauteurf));
}

void ControleSpectreCouleur::mousePressEvent(QMouseEvent *event)
{
	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	/* fenêtre de 10 pixels */
	const auto &taille_fenetre_x = 10.0f / static_cast<float>(size().width());
	const auto &taille_fenetre_y = 10.0f / static_cast<float>(size().height());

	m_point_courant = nullptr;
	m_type_point = -1;

	for (PointBezier &point : m_courbe.points) {
		for (int i = POINT_CONTROLE1; i < NOMBRE_POINT; ++i) {
			auto dist_x = point.co[i].x - x;

			if (std::abs(dist_x) > taille_fenetre_x) {
				continue;
			}

			auto dist_y = point.co[i].y - (1.0f - y);

			if (std::abs(dist_y) < taille_fenetre_y) {
				m_point_courant = &point;
				m_type_point = i;
			}
		}
	}
}

void ControleSpectreCouleur::mouseMoveEvent(QMouseEvent *event)
{
	if (m_point_courant == nullptr) {
		return;
	}

	auto x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	auto y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	x = std::max(0.0f, std::min(1.0f, x));
	y = std::max(0.0f, std::min(1.0f, 1.0f - y));

	const auto delta_x = x - m_point_courant->co[m_type_point].x;
	const auto delta_y = y - m_point_courant->co[m_type_point].y;

	m_point_courant->co[m_type_point].x += delta_x;
	m_point_courant->co[m_type_point].y += delta_y;

	if (m_type_point == POINT_CENTRE) {
		m_point_courant->co[POINT_CONTROLE1].x += delta_x;
		m_point_courant->co[POINT_CONTROLE1].y += delta_y;
		m_point_courant->co[POINT_CONTROLE2].x += delta_x;
		m_point_courant->co[POINT_CONTROLE2].y += delta_y;
	}
	else {
		if (m_point_courant->type_controle == CONTROLE_CONTRAINT) {
			if (m_type_point == POINT_CONTROLE1) {
				m_point_courant->co[POINT_CONTROLE2].x -= delta_x;
				m_point_courant->co[POINT_CONTROLE2].y -= delta_y;
			}
			else if (m_type_point == POINT_CONTROLE2) {
				m_point_courant->co[POINT_CONTROLE1].x -= delta_x;
				m_point_courant->co[POINT_CONTROLE1].y -= delta_y;
			}
		}
	}

	if (m_point_courant == &m_courbe.points[0]) {
		m_courbe.extension_min.co[POINT_CENTRE].y = m_point_courant->co[POINT_CENTRE].y;
	}
	else if (m_point_courant == &m_courbe.points[m_courbe.points.taille() - 1]) {
		m_courbe.extension_max.co[POINT_CENTRE].y = m_point_courant->co[POINT_CENTRE].y;
	}

	construit_table_courbe(m_courbe);

	update();
}

void ControleSpectreCouleur::mouseReleaseEvent(QMouseEvent */*event*/)
{
	m_point_courant = nullptr;
	m_type_point = -1;
}

void ControleSpectreCouleur::mouseDoubleClickEvent(QMouseEvent *event)
{
	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	ajoute_point_courbe(m_courbe, x, 1.0f - y);

	update();
}
