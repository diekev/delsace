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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
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
#include <QWidget>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/observation.hh"

namespace kdo {
class Koudou;
}
class QFrame;
class QHBoxLayout;
class QLineEdit;
class QVBoxLayout;

class BaseEditrice : public QWidget, public Observatrice {
	Q_OBJECT

protected:
	kdo::Koudou *m_koudou;
	QFrame *m_cadre;
	QVBoxLayout *m_agencement;
	QHBoxLayout *m_agencement_principal;

public:
	explicit BaseEditrice(kdo::Koudou &koudou, QWidget *parent = nullptr);

	BaseEditrice(BaseEditrice const &) = default;
	BaseEditrice &operator=(BaseEditrice const &) = default;

	void actif(bool yesno);
	void rend_actif();

	void mousePressEvent(QMouseEvent *e) override;
};
