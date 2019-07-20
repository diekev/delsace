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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "mainwindow.h"

#include <random>

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QStack>

#include <cmath>
#include <iostream>

#include "biblinternes/outils/constantes.h"

double distance_carre(double x0, double y0, double x1, double y1)
{
	const auto dx = x0 - x1;
	const auto dy = y0 - y1;

	return (dx * dx + dy * dy);
}

class Grille2D {
	size_t m_arriere_plan = -1ul;
	size_t m_res_x = 0;
	size_t m_res_y = 0;

	double m_taille_cellule = 0.0;
	double m_rayon = 0.0;
	double m_rayon_carre = 0.0;
	double m_taille_x = 0.0;
	double m_taille_y = 0.0;

	std::vector<QPointF> m_points{};

public:
	Grille2D() = default;

	Grille2D(double r, double taille_x = 1.0, double taille_y = 1.0)
	{
		m_rayon = r;
		m_rayon_carre = r * r;
		m_taille_x = taille_x;
		m_taille_y = taille_y;
		m_taille_cellule = r / std::sqrt(2.0);
		m_res_x = static_cast<size_t>(std::ceil(taille_x / m_taille_cellule));
		m_res_y = static_cast<size_t>(std::ceil(taille_y / m_taille_cellule));
		m_points.resize(m_res_x * m_res_y, QPointF(constantes<double>::INFINITE, constantes<double>::INFINITE));

		std::cerr << "Pour un rayon de " << r
				  << ", la résolution de la grille est de "
				  << m_res_x << ", " << m_res_y << ", pour un maximum de "
				  << m_res_x * m_res_y << " points\n";
	}

	double taille_cellule() const
	{
		return m_taille_cellule;
	}

	size_t res_x() const
	{
		return m_res_x;
	}

	size_t res_y() const
	{
		return m_res_y;
	}

	bool insert_distance(const double px, const double py)
	{
		auto const index_x = static_cast<size_t>(std::floor(px / m_taille_cellule));
		auto const index_y = static_cast<size_t>(std::floor(py / m_taille_cellule));

		if (index_x >= m_res_x) {
			return false;
		}

		if (index_y >= m_res_y) {
			return false;
		}

		const auto index = index_x + index_y * m_res_x;

		if (m_points[index].x() != constantes<double>::INFINITE) {
			return false;
		}

		const auto min_x = std::max(0ul, index_x - 2);
		const auto min_y = std::max(0ul, index_y - 2);
		const auto max_x = std::min(index_x + 2, m_res_x - 1);
		const auto max_y = std::min(index_y + 2, m_res_y - 1);

		for (size_t y = min_y; y <= max_y; ++y) {
			for (size_t x = min_x; x <= max_x; ++x) {
				const auto index1 = x + y * m_res_x;

				if (m_points[index1].x() != constantes<double>::INFINITE) {
					continue;
				}

				const auto &p = m_points[index1];

				if (distance_carre(p.x(), p.y(), px, py) < m_rayon_carre) {
					return false;
				}
			}
		}

		m_points[index] = QPointF(px, py);

		return true;
	}

	size_t nombre_points() const
	{
		return m_points.size();
	}

	const std::vector<QPointF> &points() const
	{
		return m_points;
	}
};

class WidgetTriangleDelaunay final : public QWidget {
	QVector<QPointF> m_points{};
	Grille2D m_grille_points{};

public:
	explicit WidgetTriangleDelaunay(QWidget *parent = nullptr)
		: QWidget(parent)
	{
		this->resize(512, 512);
		genere_points(128, 128);
	}

	void genere_points(const double px, const double py)
	{
		const auto min_distance = 0.1 * 512.0;

		std::uniform_real_distribution<double> dist_r(0.0, min_distance);
		std::uniform_real_distribution<double> dist_TAU(0.0, constantes<double>::TAU);
		std::mt19937 rng(19937);

#if 0
		std::uniform_real_distribution<float> dist(0.0, 1.0);
		for (int i = 0; i < 100; ++i) {
			m_points.push_back(QPointF(dist(rng), dist(rng)));
		}
#else
		auto x0 = QPointF(px, py);
		QStack<QPointF> liste_active;

		m_grille_points = Grille2D(min_distance, 512, 512);

		if (m_grille_points.insert_distance(x0.x(), x0.y())) {
			liste_active.push_back(x0);
		}

		const auto k = 30;

		while (!liste_active.empty()) {
			const auto &point = liste_active.top();
			liste_active.pop();

			for (int i = 0; i < k; ++i) {
				auto x = point.x();
				auto y = point.y();
				const auto radius = dist_r(rng) + min_distance;
				const auto angle = dist_TAU(rng);

				x += std::cos(angle) * radius;
				y += std::sin(angle) * radius;

				if (m_grille_points.insert_distance(x, y)) {
					liste_active.push(QPointF(x, y));
				}
			}
		}

		std::cerr << "Nombre de points générés " << m_grille_points.nombre_points() << '\n';
#endif
	}

	void paintEvent(QPaintEvent */*event*/) override
	{
		QPainter peintre(this);

		/* dessine la grille */
		peintre.setPen(QPen(QBrush(Qt::gray), 1.0));

		for (size_t i = 1; i < m_grille_points.res_x(); ++i) {
			auto x = static_cast<double>(i) * m_grille_points.taille_cellule() * this->size().width();

			peintre.drawLine(static_cast<int>(x), 0, static_cast<int>(x), this->rect().height());
		}

		for (size_t i = 1; i < m_grille_points.res_y(); ++i) {
			auto y = static_cast<double>(i) * m_grille_points.taille_cellule() * this->size().height();

			peintre.drawLine(0, static_cast<int>(y), this->size().width(), static_cast<int>(y));
		}

		/* dessine les points */

		peintre.setPen(QPen(QBrush(Qt::black), 2.0));

		for (const auto &point : m_grille_points.points()) {
			auto x = point.x();
			auto y = point.y();
			peintre.drawPoint(static_cast<int>(x), static_cast<int>(y));
		}
	}

	void mousePressEvent(QMouseEvent *event) override
	{
		genere_points(event->pos().x(),
					  event->pos().y());
		update();
	}
};

// http://page.mi.fu-berlin.de/faniry/files/faniry_aims.pdf
#if 0
struct Triangle {
	QPointF p0;
	QPointF p1;
	QPointF p2;

	bool contiens(const QPointF &pr)
	{
		return false;
	}
};

#include <QMatrix4x4>
#include <QVector>

/**
 * Retourne vrai si les points du triangles sont orientés dans les sens
 * antihoraire.
 */
bool sens_antihoraire(const Triangle &triangle)
{
	return ((triangle.p1.x() - triangle.p0.x()) * (triangle.p2.y() - triangle.p0.y())
			- (triangle.p2.x() - triangle.p0.x()) * (triangle.p1.y() - triangle.p0.y())) > 0.0;
}

/**
 * Retourne vrai si Pr est contenu dans le cercle circonscrit au triangle.
 */
bool encercle(const Triangle &triangle, const QPointF &Pr)
{
	QMatrix4x4 mat(
				triangle.p0.x(), triangle.p0.y(), triangle.p0.x() * triangle.p0.x() + triangle.p0.y() * triangle.p0.y(), 1.0,
				triangle.p1.x(), triangle.p1.y(), triangle.p1.x() * triangle.p1.x() + triangle.p1.y() * triangle.p1.y(), 1.0,
				triangle.p2.x(), triangle.p2.y(), triangle.p2.x() * triangle.p2.x() + triangle.p2.y() * triangle.p2.y(), 1.0,
				Pr.x(), Pr.y(), Pr.x() * Pr.x() + Pr.y() * Pr.y(), 1.0);

	return mat.determinant() > 0;
}

void bascule(const Triangle &delta, const Triangle &delta_adj, const QPointF &Pr, QVector<Triangle> &DP)
{

}

void valide_cote(const Triangle &delta, const QPointF &Pr, QVector<Triangle> &DP)
{
	// soit delta_adj le triangle opposée à Pr et adjacent à delta
	Triangle delta_adj;

	if (encercle(delta_adj, Pr)) {
		// nous devons faire un basculement de côté
		bascule(delta, delta_adj, Pr, DP);

		// soit delta' et delta" les deux nouveaux triangles, légalisons les récursivement
		Triangle deltap, deltas;
		valide_cote(deltap, Pr, DP);
		valide_cote(deltas, Pr, DP);
	}
}

QVector<Triangle> triangule_delaunay(const QVector<QPointF> &points)
{
	// initialise la triangulation D(P) pour être omega1 omega2 omega3
	auto DP = QVector<Triangle>();

	/* OMEGA1, OMEGA2, et OMEGA3 sont choisis pour être en dehors de l'ensemble
	 * de points, il faut donc calculé le maximum des coordonnées de points. */
	const auto max = 1.0f;
	const auto OMEGA1 = QPointF( 3.0 * max,  0.0);
	const auto OMEGA2 = QPointF( 0.0,        3.0 * max);
	const auto OMEGA3 = QPointF(-3.0 * max, -3.0 * max);

	Triangle OMEGA;
	OMEGA.p0 = OMEGA1;
	OMEGA.p1 = OMEGA2;
	OMEGA.p2 = OMEGA3;
	DP.push_back(OMEGA);

	// fais une permutation aléatoire des points

	for (size_t r = 0; r < points.size(); ++r) {
		// prendre le point Pr qui n'est pas déjà sélectionné
		const auto Pr = points[r];

		// trouve un triangle DELTA qui contient Pr
		Triangle DELTA;

		// si Pr est à l'intérieur de DELTA
		if (DELTA.contiens(Pr)) {
			// divise DELTA en D1, D2, D3
			Triangle DELTA1, DELTA2, DELTA3;

			valide_cote(DELTA1, Pr, DP);
			valide_cote(DELTA2, Pr, DP);
			valide_cote(DELTA3, Pr, DP);
		}
		else {
			// Pr est sur un cote c de DELTA

			// soit DELTA' le triangle partageant le coté c avec DELTA
			// divise DELTA' en DELTA'1, DELTA'2 et DELTA en DELTA1, DELTA2
			Triangle DELTAP1, DELTAP2;
			Triangle DELTA1, DELTA2;

			valide_cote(DELTAP1, Pr, DP);
			valide_cote(DELTAP2, Pr, DP);
			valide_cote(DELTA1, Pr, DP);
			valide_cote(DELTA2, Pr, DP);
		}
	}

	// supprime tous les triangles contenant omega1, omega2, omega3 de D(P)

	return DP;
}
#endif

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QApplication::setApplicationName("Infographie");

	WidgetTriangleDelaunay w;
	w.show();

	return a.exec();
}
