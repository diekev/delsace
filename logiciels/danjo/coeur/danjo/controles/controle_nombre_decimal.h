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

class ControleNombreDecimal : public QWidget {
	Q_OBJECT

	float m_valeur = 0.0f;
	float m_min = -std::numeric_limits<float>::max();
	float m_max = std::numeric_limits<float>::max();
	float m_precision = 10.0f;
	float m_inv_precision = 1.0f / m_precision;
	bool m_souris_pressee = false;
	bool m_edition = false;
	bool m_anime = false;
	bool m_temps_exacte = false;
	int m_vieil_x = 0;
	int pad = 0;
	QString m_tampon = "";
	QString m_suffixe = "";
	bool m_premier_changement = false;

public:
	explicit ControleNombreDecimal(QWidget *parent = nullptr);

	void paintEvent(QPaintEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

	void keyPressEvent(QKeyEvent *event) override;

	void marque_anime(bool ouinon, bool temps_exacte);

	void ajourne_plage(float min, float max);

	float valeur() const;

	void valeur(const float v);

	void suffixe(const QString &s);

	float min() const;

	float max() const;

Q_SIGNALS:
	void prevaleur_changee();
	void valeur_changee(float);

public Q_SLOTS:
	void ajourne_valeur(float valeur);
};

