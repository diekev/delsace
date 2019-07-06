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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QProgressBar;
class QPushButton;

struct Mikisa;

class BarreDeProgres : public QWidget {
	Q_OBJECT

	Mikisa &m_mikisa;

	QProgressBar *m_barre_progres;
	QLabel *m_label;
	QPushButton *m_bouton_stop;

	QHBoxLayout *m_disposition;

public:
	explicit BarreDeProgres(Mikisa &mikisa, QWidget *parent = nullptr);

	BarreDeProgres(BarreDeProgres const &) = default;
	BarreDeProgres &operator=(BarreDeProgres const &) = default;

	void ajourne_valeur(int valeur);

	void ajourne_message(const char *message);

private Q_SLOTS:
	void signal_stop();
};
