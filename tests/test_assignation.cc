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

#include "decoupage/erreur.h"

#include "outils.h"

void test_assignation(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* réassignation : SUCCÈS */
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5;
					a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* réassignation : ÉCHEC */
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit a = 5;
					a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation de pointeur */
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5;
					@a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation de tableaux : À FAIRE : référence */
	{
		const char *texte =
				R"(
				fonction foo(variable a : [2]e32)
				{
					a[0] = 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* assignation de tableaux, échec car mauvais type */
	{
		const char *texte =
				R"(
				fonction foo(variable a : [2]e32)
				{
					a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation de tableaux, échec car invariable */
	{
		const char *texte =
				R"(
				fonction foo(a : [2]e32)
				{
					a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation autre type */
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit variable a = 5.0;
					a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation dans une expression */
	{
		const char *texte =
				R"(
				fonction foo()
				{
					soit a = 5.0;
					soit b = a = 6;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* assignation d'un membre scalaire d'une structure */
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : e32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x de v = 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* assignation d'un membre tableau d'une structure : SUCCÈS */
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : [2]e32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x[0] de v = 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_INVALIDE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* assignation d'un membre tableau d'une structure : ÉCHEC */
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					x : [2]e32;
				}
				fonction foo(variable v : Vecteur3D)
				{
					x de v = 5;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::ASSIGNATION_MAUVAIS_TYPE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}
