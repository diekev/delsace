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

#include "tests_arachne.hh"

#include "../biblinternes/systeme_fichier/utilitaires.h"

#include "../logiciels/arachne/sources/gestionnaire.h"

void test_creation_base_donnees(dls::test_unitaire::Controleuse &controlleur)
{
	arachne::gestionnaire gestionnaire;
	gestionnaire.cree_base_donnees("test_creation1");
	gestionnaire.cree_base_donnees("test_creation2");

	const auto chemin_test1 = gestionnaire.chemin_racine() / "test_creation1";
	const auto chemin_test2 = gestionnaire.chemin_racine() / "test_creation2";

	CU_VERIFIE_CONDITION(controlleur, std::experimental::filesystem::exists(chemin_test1));
	CU_VERIFIE_CONDITION(controlleur, std::experimental::filesystem::exists(chemin_test2));
	CU_VERIFIE_CONDITION(controlleur, gestionnaire.base_courante() == "test_creation2");

	gestionnaire.supprime_base_donnees("test_creation1");
	gestionnaire.supprime_base_donnees("test_creation2");

	CU_VERIFIE_CONDITION(controlleur, !std::experimental::filesystem::exists(chemin_test1));
	CU_VERIFIE_CONDITION(controlleur, !std::experimental::filesystem::exists(chemin_test2));
	CU_VERIFIE_CONDITION(controlleur, gestionnaire.base_courante() == "");

	gestionnaire.cree_base_donnees("test_creation1");

	CU_VERIFIE_CONDITION(controlleur, std::experimental::filesystem::exists(chemin_test1));
	CU_VERIFIE_CONDITION(controlleur, gestionnaire.base_courante() == "test_creation1");

	gestionnaire.supprime_base_donnees("test_creation1");
}
