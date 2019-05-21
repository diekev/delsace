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

#include "test_tableaux.hh"

#include "erreur.h"
#include "outils.h"

void test_tableaux(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut passer un tableau fixe à une fonction attendant un tableau fixe.");
	{
		const char *texte =
				R"(
				fonc passe_tableau_fixe(tabl : [3]z32) : rien
				{
				}
				fonc foo() : rien
				{
					dyn tabl : [3]z32;
					passe_tableau_fixe(tabl);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas passer un tableau dynamic à une fonction attendant un tableau fixe.");
	{
		const char *texte =
				R"(
				fonc passe_tableau_fixe(tabl : [3]z32) : rien
				{
				}
				fonc foo() : rien
				{
					dyn tabl : []z32;
					passe_tableau_fixe(tabl);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas passer un tableau dynamic à une fonction attendant un pointeur.");
	{
		const char *texte =
				R"(
				fonc passe_pointeur(tabl : *z32) : rien
				{
				}
				fonc foo() : rien
				{
					dyn tabl : []z32;
					nonsûr {
						passe_pointeur(tabl);
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::TYPE_ARGUMENT);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut passer un tableau fixe à une fonction attendant un tableau dynamic.");
	{
		const char *texte =
				R"(
				fonc passe_tableau_dynamic(tabl : []z32) : rien
				{
				}
				fonc foo() : rien
				{
					dyn tabl : [3]z32;
					passe_tableau_dynamic(tabl);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut passer un tableau dynamic à une fonction attendant un tableau dynamic.");
	{
		const char *texte =
				R"(
				fonc passe_tableau_dynamic(tabl : []z32) : rien
				{
				}
				fonc foo() : rien
				{
					dyn tabl : []z32;
					passe_tableau_dynamic(tabl);
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut accèder à la propriété 'taille' d'un tableau dynamic.");
	{
		const char *texte =
				R"(
				fonc foo() : rien
				{
					dyn tabl : []z32;
					soit taille = taille de tabl;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut accèder à la propriété 'taille' d'un tableau fixe.");
	{
		const char *texte =
				R"(
				fonc foo() : rien
				{
					dyn tabl : [3]z32;
					soit taille = taille de tabl;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut modifier les propriétés d'un tableau dans un bloc 'nonsûr'.");
	{
		const char *texte =
				R"(
				fonc foo() : rien
				{
					dyn tabl : [3]z32;

					nonsûr {
						taille de tabl = 3;
					}
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

#ifdef NONSUR
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas modifier les propriétés d'un tableau hors d'un bloc 'nonsûr'.");
	{
		const char *texte =
				R"(
				fonc foo() : rien
				{
					dyn tabl : [3]z32;
					taille de tabl = 3;
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::ASSIGNATION_INVALIDE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On peut construire un tableau dans une expression.");
	{
		const char *texte =
				R"(
				fonc foo() : rien
				{
					dyn tabl = [1, 2, 3];
				}
				)";

		auto const [erreur_lancee, type_correcte] = retourne_erreur_lancee(
				texte, false, erreur::type_erreur::AUCUNE_ERREUR, false);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);
#endif
}
