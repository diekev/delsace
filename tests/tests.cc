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

#include "tests.hh"

//#include "test_uri.hh"

#include "math/tests_math.hh"
#include "math/tests_matrice.hh"
#include "math/tests_quaternion.hh"
#include "math/tests_vecteur.hh"

int main()
{
	dls::test_unitaire::Controleuse controleuse;
	//controleuse.ajoute_fonction(test_uri);
	controleuse.ajoute_fonction(tests_matrice);
	controleuse.ajoute_fonction(tests_quaternion);
	controleuse.ajoute_fonction(tests_vecteur);
	controleuse.ajoute_fonction(test_matrice);
	controleuse.ajoute_fonction(test_math);
	controleuse.ajoute_fonction(test_statistique);
	controleuse.ajoute_fonction(test_nombre_decimaux);
	controleuse.ajoute_fonction(test_pystring);
	controleuse.ajoute_fonction(test_date);
	controleuse.ajoute_fonction(test_temperature);
	controleuse.ajoute_fonction(test_pointeur_marque);
	controleuse.ajoute_fonction(test_tableau_compact);

	controleuse.performe_controles();

	controleuse.imprime_resultat();
}
