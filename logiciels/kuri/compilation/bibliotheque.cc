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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "bibliotheque.hh"

#include "arbre_syntaxique.hh"

Bibliotheque *GestionnaireBibliotheque::trouve_bibliotheque(NoeudExpression *site)
{
	POUR_TABLEAU_PAGE (bibliotheques) {
		if (it.ident == site->ident) {
			return &it;
		}
	}

	return nullptr;
}

Bibliotheque *GestionnaireBibliotheque::cree_bibliotheque(NoeudExpression *site)
{
	auto bibliotheque = trouve_bibliotheque(site);

	if (bibliotheque) {
		// À FAIRE: erreur
		return bibliotheque;
	}

	bibliotheque = bibliotheques.ajoute_element();
	bibliotheque->site = site;
	bibliotheque->ident = site->ident;
	return bibliotheque;
}

void GestionnaireBibliotheque::rassemble_statistiques(Statistiques &stats)
{
	// À FAIRE
}
