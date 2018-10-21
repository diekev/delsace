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

#include "test_types.h"

#include "erreur.h"
#include "outils.h"

static void test_inference_type_echec(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* assignation de 'rien' */
	{
		const char *texte =
				R"(
				fonction ne_retourne_rien() : rien
				{
					retourne;
				}

				fonction principale(compte : n32, arguments : n8) : n32
				{
					soit a = ne_retourne_rien();
					retourne 0;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::ASSIGNATION_RIEN);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
	/* impossibilité de définir */
	{
		const char *texte =
				R"(
				fonction ne_retourne_rien()
				{
					retourne;
				}

				fonction principale(compte : n32, arguments : n8) : n32
				{
					soit a = ne_retourne_rien();
					retourne 0;
				}
				)";

		const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::TYPE_INCONNU);
		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
	}
}

static void test_inference_type_succes(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	const char *texte =
			R"(
			fonction ajouter(x : n32) : n32
			{
				retourne x + 2;
			}

			fonction ajouter_r64(x : r64) : r64
			{
				retourne x + 2.0;
			}

			fonction principale(compte : n32, arguments : n8) : n32
			{
				soit a = ajouter(9);
				soit b = ajouter(a);
				soit x = 9.0;
				soit y = ajouter_r64(x);
				retourne 0;
			}
			)";

	const auto erreur_lancee = retourne_erreur_lancee(texte, false, erreur::type_erreur::AUCUNE_ERREUR);
	CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
}

void test_types(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_inference_type_echec(controleur);
	test_inference_type_succes(controleur);
}
