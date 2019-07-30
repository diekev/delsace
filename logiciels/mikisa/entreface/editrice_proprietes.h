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

#include "base_editrice.h"

class QGridLayout;
class QHBoxLayout;
class QScrollArea;

class EditriceProprietes : public BaseEditrice {
	QWidget *m_widget;
	QWidget *m_conteneur_avertissements;
	QWidget *m_conteneur_disposition;
	QScrollArea *m_scroll;
	QVBoxLayout *m_disposition_widget;

public:
	explicit EditriceProprietes(Mikisa &mikisa, QWidget *parent = nullptr);

	EditriceProprietes(EditriceProprietes const &) = default;
	EditriceProprietes &operator=(EditriceProprietes const &) = default;

	void ajourne_etat(int evenement) override;

	void ajourne_manipulable() override;

	void obtiens_liste(dls::chaine const &attache, dls::tableau<dls::chaine> &chaines) override;

	void onglet_dossier_change(int index) override;

	void reinitialise_entreface(bool creation_avert);
};
