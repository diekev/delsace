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

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QMainWindow>
#pragma GCC diagnostic pop

class Mikisa;

class FenetrePrincipale : public QMainWindow {
	Q_OBJECT

	Mikisa &m_mikisa;

public:
	explicit FenetrePrincipale(Mikisa &mikisa, QWidget *parent = nullptr);

	FenetrePrincipale(FenetrePrincipale const &) = default;
	FenetrePrincipale &operator=(FenetrePrincipale const &) = default;

public Q_SLOTS:
	void image_traitee();
	void mis_a_jour_menu_fichier_recent();

private:
	QDockWidget *ajoute_dock(const QString &nom, int type, int aire, QDockWidget *premier = nullptr);
	void genere_barre_menu();
	void charge_reglages();
	void ecrit_reglages() const;
	void closeEvent(QCloseEvent *) override;
};
