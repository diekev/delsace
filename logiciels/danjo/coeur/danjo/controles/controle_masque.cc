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

#include "controle_masque.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

#include <iostream>

static void construit_table_cerce(CerceBezier &cerce)
{
	const auto res_courbe = 32;
	const auto facteur = 1.0f / res_courbe;

	cerce.table.efface();
	cerce.table.reserve(res_courbe + 1);

	for (auto i = 0; i < cerce.points.taille(); ++i) {
		auto i1 = i;
		auto i2 = i + 1;

		if (i2 == cerce.points.taille()) {
			if (cerce.ferme) {
				i2 = 0;
			}
			else {
				return;
			}
		}

		const auto &p1 = cerce.points[i1];
		const auto &p2 = cerce.points[i2];

		const auto &x1 = p1.co[POINT_CENTRE].x;
		const auto &y1 = p1.co[POINT_CENTRE].y;
		const auto &x_pt2 = p1.co[POINT_CONTROLE2].x;
		const auto &y_pt2 = p1.co[POINT_CONTROLE2].y;
		const auto &x2 = p2.co[POINT_CENTRE].x;
		const auto &y2 = p2.co[POINT_CENTRE].y;
		const auto &x_pt1 = p2.co[POINT_CONTROLE1].x;
		const auto &y_pt1 = p2.co[POINT_CONTROLE1].y;

		cerce.table.pousse(Point{x1, y1});

		for (int j = 1; j <= res_courbe; ++j) {
			const auto fac_i = facteur * static_cast<float>(j);
			const auto mfac_i = 1.0f - fac_i;

			/* centre -> pt2 */
			const auto x_c_pt2 = mfac_i * x1 + fac_i * x_pt2;
			const auto y_c_pt2 = mfac_i * y1 + fac_i * y_pt2;

			/* pt2 -> pt1 */
			const auto x_pt2_pt1 = mfac_i * x_pt2 + fac_i * x_pt1;
			const auto y_pt2_pt1 = mfac_i * y_pt2 + fac_i * y_pt1;

			/* pt1 -> centre */
			const auto x_pt1_c = mfac_i * x_pt1 + fac_i * x2;
			const auto y_pt1_c = mfac_i * y_pt1 + fac_i * y2;

			/* c_pt2 -> pt2_pt1 */
			const auto x_c_pt1 = mfac_i * x_c_pt2 + fac_i * x_pt2_pt1;
			const auto y_c_pt1 = mfac_i * y_c_pt2 + fac_i * y_pt2_pt1;

			/* pt2_pt1 -> pt1_c */
			const auto x_pt2_c = mfac_i * x_pt2_pt1 + fac_i * x_pt1_c;
			const auto y_pt2_c = mfac_i * y_pt2_pt1 + fac_i * y_pt1_c;

			const auto xt2 = mfac_i * x_c_pt1 + fac_i * x_pt2_c;
			const auto yt2 = mfac_i * y_c_pt1 + fac_i * y_pt2_c;

			cerce.table.pousse(Point{xt2, yt2});
		}
	}
}

static void ajoute_point_cerce(CerceBezier &cerce, float x, float y)
{
	PointBezier point;
	point.co[POINT_CONTROLE1].x = x - 0.1f;
	point.co[POINT_CONTROLE1].y = y;
	point.co[POINT_CENTRE].x = x;
	point.co[POINT_CENTRE].y = y;
	point.co[POINT_CONTROLE2].x = x + 0.1f;
	point.co[POINT_CONTROLE2].y = y;

	cerce.points.pousse(point);

	cerce.min.x = std::min(cerce.min.x, x);
	cerce.min.y = std::min(cerce.min.y, y);
	cerce.max.x = std::max(cerce.max.x, x);
	cerce.max.y = std::max(cerce.max.y, y);

	construit_table_cerce(cerce);
}

/* http://okomestudio.net/biboroku/?p=986
 * http://www.ariel.com.au/a/python-point-int-poly.html */
static bool contenu_dans_courbe(const CerceBezier &cerce, float x, float y)
{
	/* vérifie si le point se trouve dans la boîte englobante de la cerce */

	if (x < cerce.min.x || x > cerce.max.x) {
		return false;
	}

	if (y < cerce.min.y || y > cerce.max.y) {
		return false;
	}

	const auto n = cerce.table.taille();
	auto inside = false;
	auto p1x = cerce.table[0].x;
	auto p1y = cerce.table[0].y;

	for (auto i = 1; i < n + 1; ++i) {
		auto p2x = cerce.table[i % n].x;
		auto p2y = cerce.table[i % n].y;

		if (y > std::min(p1y, p2y)) {
			if (y <= std::max(p1y, p2y)) {
				if (x <= std::max(p1x, p2x)) {
					auto xinters = 0.0f;

					if (p1y != p2y) {
						xinters = (y - p1y) * (p2x - p1y) + p1x;
					}

					if (p1x == p2x or x <= xinters) {
						inside = !inside;
					}
				}
			}
		}

		std::swap(p1x, p2x);
		std::swap(p1y, p2y);
	}

	return inside;
}

struct Carreau {
	int x;
	int y;

	int hauteur;
	int largeur;
};

ControleMasque::ControleMasque(QWidget *parent)
	: QWidget(parent)
{
	resize(512, 512);
}

void ControleMasque::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(QBrush(Qt::black));

	painter.drawRect(this->rect());

	if (m_cerce.points.est_vide()) {
		return;
	}

	auto largeur = size().width();
	auto hauteur = size().height();
	auto hauteurf = static_cast<float>(hauteur);
	auto largeurf = static_cast<float>(largeur);

	if (m_cerce.ferme && m_point_courant == nullptr) {
		QApplication::setOverrideCursor(Qt::WaitCursor);

		auto stylo = QPen(Qt::white);
		stylo.setWidthF(1.0);

		painter.setPen(stylo);

#if 1
		auto hauteur_inv = 1.0f / hauteurf;
		auto largeur_inv = 1.0f / largeurf;

		constexpr auto TAILLE_CARREAU = 16;

		dls::tableau<Carreau> carreaux;

		for (int i = 0; i < largeur; i += TAILLE_CARREAU) {
			for (int j = 0; j < hauteur; j += TAILLE_CARREAU) {
				Carreau carreau;
				carreau.x = i;
				carreau.y = j;
				carreau.largeur = std::min(TAILLE_CARREAU, largeur - i);
				carreau.hauteur = std::min(TAILLE_CARREAU, hauteur - j);

				auto fx = static_cast<float>(i) * largeur_inv;
				auto fy = static_cast<float>(j) * hauteur_inv;

				if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
					carreaux.pousse(carreau);
					continue;
				}

				fx = static_cast<float>(i + carreau.largeur) * largeur_inv;
				fy = static_cast<float>(j) * hauteur_inv;

				if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
					carreaux.pousse(carreau);
					continue;
				}

				fx = static_cast<float>(i) * largeur_inv;
				fy = static_cast<float>(j + carreau.hauteur) * hauteur_inv;

				if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
					carreaux.pousse(carreau);
					continue;
				}

				fx = static_cast<float>(i + carreau.largeur) * largeur_inv;
				fy = static_cast<float>(j + carreau.hauteur) * hauteur_inv;

				if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
					carreaux.pousse(carreau);
					continue;
				}
			}
		}

		std::cerr << "Il y a " << carreaux.taille() << " carreaux\n";

		for (const Carreau &carreau : carreaux) {
			for (int i = carreau.x; i < carreau.x + carreau.largeur; ++i) {
				for (int j = carreau.y; j < carreau.y + carreau.hauteur; ++j) {
					auto fx = static_cast<float>(i) * hauteur_inv;
					auto fy = static_cast<float>(j) * largeur_inv;

					if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
						painter.drawPoint(i, j);
					}
				}
			}
		}
#else
		for (int i = 0; i < largeur; ++i) {
			for (int j = 0; j < hauteur; ++j) {
				auto fx = i / static_cast<float>(largeur);
				auto fy = j / static_cast<float>(hauteur);

				if (contenu_dans_courbe(m_cerce, fx, 1.0f - fy)) {
					painter.drawPoint(i, j);
				}
			}
		}
#endif

		QApplication::restoreOverrideCursor();
	}

	/* dessine la courbe : notons que les coordonnées y sont inversées afin que
	 * qu'une coordonnée égale à 0.0 soit dessinée 'en-bas' */

	/* dessine les points */
	auto stylo = QPen(Qt::yellow);
	stylo.setWidthF(5.0);

	painter.setPen(stylo);

	for (const PointBezier &point : m_cerce.points) {
		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE1].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CONTROLE1].y) * hauteurf));

		painter.drawPoint(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

		painter.drawPoint(static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						  static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
	}

	/* dessine les connexions entre points de controles et points centraux */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(1.0);

	painter.setPen(stylo);

	for (const PointBezier &point : m_cerce.points) {
		painter.drawLine(static_cast<int>(point.co[POINT_CONTROLE1].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE1].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf));

		painter.drawLine(static_cast<int>(point.co[POINT_CENTRE].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CENTRE].y) * hauteurf),
						 static_cast<int>(point.co[POINT_CONTROLE2].x * largeurf),
						 static_cast<int>((1.0f - point.co[POINT_CONTROLE2].y) * hauteurf));
	}

	if (m_cerce.points.taille() <= 1) {
		return;
	}

	/* dessine la courbe */
	stylo = QPen(Qt::yellow);
	stylo.setWidthF(2.0);

	painter.setPen(stylo);

	for (auto i = 0; i < m_cerce.table.taille() - 1; ++i) {
		auto p1 = m_cerce.table[i];
		auto p2 = m_cerce.table[i + 1];

		painter.drawLine(static_cast<int>(p1.x * largeurf),
						 static_cast<int>((1.0f - p1.y) * hauteurf),
						 static_cast<int>(p2.x * largeurf),
						 static_cast<int>((1.0f - p2.y) * hauteurf));
	}
}

void ControleMasque::mousePressEvent(QMouseEvent *event)
{
	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	/* fenêtre de 10 pixels */
	const auto &taille_fenetre_x = 10.0f / static_cast<float>(size().width());
	const auto &taille_fenetre_y = 10.0f / static_cast<float>(size().height());

	m_point_courant = nullptr;
	m_type_point = -1;

	for (PointBezier &point : m_cerce.points) {
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

void ControleMasque::mouseMoveEvent(QMouseEvent *event)
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

	construit_table_cerce(m_cerce);

	update();
}

void ControleMasque::mouseReleaseEvent(QMouseEvent */*event*/)
{
	m_point_courant = nullptr;
	m_type_point = -1;
	update();
}

void ControleMasque::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (m_cerce.ferme) {
		return;
	}

	const auto &x = static_cast<float>(event->pos().x()) / static_cast<float>(size().width());
	const auto &y = static_cast<float>(event->pos().y()) / static_cast<float>(size().height());

	ajoute_point_cerce(m_cerce, x, 1.0f - y);

	update();
}

void ControleMasque::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
		case Qt::Key_F:
			m_cerce.ferme = true;
			construit_table_cerce(m_cerce);
			break;
		default:
			break;
	}
	update();
}
