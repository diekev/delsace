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

#include "test_fonctions.h"

#include <cstring>

#include "analyseuse_grammaire.h"
#include "decoupeuse.h"
#include "erreur.h"

void test_fonctions(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction ajouter(a : e32, b : e32) : e32
			{
				retourne a + b;
			}

			fonction principale(compte : e32, arguments : e8) : e32
			{
				soit a = ajouter(5, 8);
				soit b = ajouter(8, 5);
				soit c = ajouter(ajouter(a + b, b), ajouter(b + a, a));
				retourne c != 5;
			}
			)";

	auto tampon = TamponSource(texte);
	auto erreur_lancee = false;

	try {
		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		analyseuse_grammaire analyseuse;
		analyseuse.lance_analyse(decoupeuse.morceaux());
	}
	catch (const erreur::frappe &e) {
		std::cerr << e.message() << '\n';
		erreur_lancee = true;
	}
	catch (const char *e) {
		std::cerr << e << '\n';
		erreur_lancee = true;
	}
	catch (...) {
		erreur_lancee = true;
	}

	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}
