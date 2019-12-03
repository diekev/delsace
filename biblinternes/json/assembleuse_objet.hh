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

#include "biblinternes/structures/pile.hh"
#include "biblinternes/tori/objet.hh"

struct assembleuse_objet {
	using ptr_objet = std::shared_ptr<tori::Objet>;

	ptr_objet racine{};

	dls::pile<ptr_objet> objets{};

	assembleuse_objet();

	ptr_objet cree_objet(dls::vue_chaine const &nom, tori::type_objet type);

	void empile_objet(ptr_objet objet);

	void depile_objet();
};
