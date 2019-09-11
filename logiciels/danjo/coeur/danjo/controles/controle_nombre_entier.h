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

#pragma once

#include <QWidget>

class ControleNombreEntier : public QWidget {
	Q_OBJECT

	int m_valeur = 1;
	int m_min = std::numeric_limits<int>::min();
	int m_max = std::numeric_limits<int>::max();
	bool m_souris_pressee = false;
	bool m_edition = false;
	int m_vieil_x = 0;
	QString m_tampon = "";
	QString m_suffixe = "";
	bool m_anime = false;
	bool m_temps_exact = false;
	bool m_premier_changement = true;

public:
	explicit ControleNombreEntier(QWidget *parent = nullptr);

	void paintEvent(QPaintEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

	void keyPressEvent(QKeyEvent *event);

	void ajourne_plage(int min, int max);

	void valeur(const int v);

	void suffixe(const QString &s);

	int min() const;

	int max() const;

	void marque_anime(bool ouinon, bool temps_exacte);

Q_SIGNALS:
	void prevaleur_changee();
	void valeur_changee(int);

public Q_SLOTS:
	void ajourne_valeur(int valeur);
};
