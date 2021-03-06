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

#include "danjo/conteneur_controles.h"

#include "biblinternes/patrons_conception/observation.hh"

class Silvatheque;
class QFrame;
class QHBoxLayout;
class QLineEdit;
class QVBoxLayout;

class BaseEditrice : public danjo::ConteneurControles, public Observatrice {
	Q_OBJECT

protected:
	Silvatheque *m_silvatheque;
	QFrame *m_cadre;
	QVBoxLayout *m_agencement;
	QHBoxLayout *m_agencement_principal{};

public:
	explicit BaseEditrice(Silvatheque &silvatheque, QWidget *parent = nullptr);

	BaseEditrice(BaseEditrice const &autre) = default;
	BaseEditrice &operator=(BaseEditrice const &autre) = default;

	void actif(bool yesno);
	void rend_actif();

	void mousePressEvent(QMouseEvent *e) override;
};
