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

#include "assembleuse_objet.hh"

assembleuse_objet::assembleuse_objet()
	: racine(tori::construit_objet(tori::type_objet::DICTIONNAIRE))
{
	empile_objet(racine);
}

assembleuse_objet::ptr_objet assembleuse_objet::cree_objet(const dls::vue_chaine &nom, tori::type_objet type)
{
	auto objet = tori::construit_objet(type);

	if (!objets.est_vide()) {
		auto objet_haut = objets.haut().get();

		if (objet_haut->type == tori::type_objet::DICTIONNAIRE) {
			auto dico = static_cast<tori::ObjetDictionnaire *>(objet_haut);
			dico->insere(nom, objet);
		}
		else {
			auto tabl = static_cast<tori::ObjetTableau *>(objet_haut);
			tabl->pousse(objet);
		}
	}

	return objet;
}

void assembleuse_objet::empile_objet(assembleuse_objet::ptr_objet objet)
{
	assert(objet->type == tori::type_objet::DICTIONNAIRE || objet->type == tori::type_objet::TABLEAU);
	objets.empile(objet);
}

void assembleuse_objet::depile_objet()
{
	objets.depile();
}
