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

#include "validation_semantique.hh"

#include <cassert>

#include "arbre_syntactic.hh"
#include "broyage.hh"
#include "contexte_generation_code.hh"
#include "donnees_type.hh"
#include "erreur.hh"

namespace noeud {

/* ************************************************************************** */


/* ************************************************************************** */

bool est_constant(base *b)
{
	switch (b->type) {
		default:
		case type_noeud::TABLEAU:
		{
			return false;
		}
		case type_noeud::BOOLEEN:
		case type_noeud::CARACTERE:
		case type_noeud::NOMBRE_ENTIER:
		case type_noeud::NOMBRE_REEL:
		case type_noeud::CHAINE_LITTERALE:
		{
			return true;
		}
	}
}

static bool est_assignation_operee(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::MOINS_EGAL:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		{
			return true;
		}
	}
}

/* ************************************************************************** */

void performe_validation_semantique(base *b, ContexteGenerationCode &contexte)
{
}

}  /* namespace noeud */
