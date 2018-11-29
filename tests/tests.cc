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

#include "test_assignation.hh"
#include "test_boucle.hh"
#include "test_decoupage.h"
#include "test_decoupage_nombres.h"
#include "test_expression.h"
#include "test_fonctions.h"
#include "test_modules.hh"
#include "test_retour.hh"
#include "test_structures.hh"
#include "test_tableaux.hh"
#include "test_transtype.hh"
#include "test_types.h"
#include "test_unicode.h"
#include "test_variables.h"

int main()
{
	dls::test_unitaire::Controleuse controleuse;
	controleuse.ajoute_fonction(test_decoupage);
	controleuse.ajoute_fonction(test_decoupage_nombres);
	controleuse.ajoute_fonction(test_unicode);
	controleuse.ajoute_fonction(test_expression);
	controleuse.ajoute_fonction(test_fonctions);
	controleuse.ajoute_fonction(test_types);
	controleuse.ajoute_fonction(test_variables);
	controleuse.ajoute_fonction(test_structures);
	controleuse.ajoute_fonction(test_assignation);
	controleuse.ajoute_fonction(test_retour);
	controleuse.ajoute_fonction(test_boucle);
	controleuse.ajoute_fonction(test_transtype);
	controleuse.ajoute_fonction(test_modules);
	controleuse.ajoute_fonction(test_tableaux);

	controleuse.performe_controles();

	controleuse.imprime_resultat();
}
