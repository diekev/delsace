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

#include "operatrices_standards.h"

#include <delsace/math/bruit.hh>

#include "../bibliotheques/objets/adaptrice_creation.h"
#include "../bibliotheques/objets/creation.h"

#include "sdk/context.h"
#include "sdk/mesh.h"
#include "sdk/primitive.h"
#include "sdk/prim_points.h"
#include "sdk/segmentprim.h"

#include "sdk/outils/géométrie.h"

#include <random>
#include <sstream>

OperatriceSortie::OperatriceSortie(Noeud *noeud, const Context &contexte)
	: Operatrice(noeud, contexte)
{}

const char *OperatriceSortie::nom_entree(size_t)
{
	return "entrée";
}

void OperatriceSortie::execute(const Context &contexte, double temps)
{
	return;
}

const char *OperatriceSortie::nom()
{
	return "Sortie";
}

/* ************************************************************************** */

void enregistre_operatrices_integres(UsineOperatrice &/*usine*/)
{
	/* Opérateurs géométrie. */
}
