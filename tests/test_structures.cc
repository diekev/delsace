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

#include "test_structures.hh"

#include "decoupage/erreur.h"

#include "outils.h"

static void test_structure_redefinie(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* définition unique */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				structure Vecteur3D {
					x : e32;
					y : e32;
					z : e32;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* redéfinition */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				structure Vecteur2D {
					x : e32;
					y : e32;
					z : e32;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::STRUCTURE_REDEFINIE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_structure_inconnue(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* structure connue */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				structure Vecteur2D {
					x : e32;
					y : e32;
					z : e32;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* structure inconnue dans une autre structure */
	{
		const char *texte =
				R"(
				structure Vecteur3D {
					xy : Vecteur2D;
					z : e32;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* structure inconnue dans en paramètre d'une fonction */
	{
		const char *texte =
				R"(
				fonction foo(v : Vecteur3D) : rien
				{
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::STRUCTURE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_acces_membre(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* membre connu */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne x de v;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	/* membre inconnu */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne z de v;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::MEMBRE_INCONNU);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* structure inconnu */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					y : e32;
				}
				fonction accès_x(v : Vecteur2D) : rien
				{
					retourne x de w;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::VARIABLE_INCONNUE);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_membre_unique(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* membre redéfini */
	{
		const char *texte =
				R"(
				structure Vecteur2D {
					x : e32;
					x : e32;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::MEMBRE_REDEFINI);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

void test_structures(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_structure_redefinie(controleur);
	test_structure_inconnue(controleur);
	test_acces_membre(controleur);
	test_membre_unique(controleur);
}
