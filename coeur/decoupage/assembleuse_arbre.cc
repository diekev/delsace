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

#include "assembleuse_arbre.h"

#include "arbre_syntactic.h"

void assembleuse_arbre::ajoute_noeud(int type, const std::string &chaine)
{
	auto noeud = cree_noeud(type, chaine);

	if (!m_noeuds.empty()) {
		m_noeuds.top()->ajoute_noeud(noeud);
	}

	m_noeuds.push(noeud);
}

Noeud *assembleuse_arbre::cree_noeud(int type, const std::string &chaine)
{
	switch (type) {
		case NOEUD_RACINE:
			return new NoeudRacine(chaine);
		case NOEUD_APPEL_FONCTION:
			return new NoeudAppelFonction(chaine);
		case NOEUD_DECLARATION_FONCTION:
			return new NoeudDeclarationFonction(chaine);
		case NOEUD_EXPRESSION:
			return new NoeudExpression(chaine);
		case NOEUD_ASSIGNATION_VARIABLE:
			return new NoeudAssignationVariable(chaine);
		case NOEUD_VARIABLE:
			return new NoeudVariable(chaine);
		case NOEUD_NOMBRE_ENTIER:
			return new NoeudNombreEntier(chaine);
		case NOEUD_NOMBRE_REEL:
			return new NoeudNombreReel(chaine);
		case NOEUD_OPERATION:
			return new NoeudOperation(chaine);
		case NOEUD_RETOUR:
			return new NoeudRetour(chaine);
	}

	return nullptr;
}

void assembleuse_arbre::sors_noeud(int type)
{
	m_noeuds.pop();
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	m_noeuds.top()->imprime_code(os);
}
