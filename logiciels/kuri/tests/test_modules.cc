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

#include "test_modules.hh"

#include <filesystem>

#include "compilation/syntaxeuse.hh"
#include "compilation/assembleuse_arbre.h"
#include "compilation/contexte_generation_code.h"
#include "compilation/coulisse_c.hh"
#include "compilation/lexeuse.hh"
#include "compilation/modules.hh"
#include "compilation/validation_semantique.hh"

static std::pair<bool, bool> retourne_erreur_module_lancee(
		const char *chemin_fichier,
		const bool imprime_message,
		const erreur::type_erreur type,
		const bool genere_code = true)
{
	auto erreur_lancee = false;
	auto type_correcte = false;

	auto dossier_origine = std::filesystem::current_path();

	try {
		auto chemin = std::filesystem::path(chemin_fichier);
		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		auto nom_module = chemin.stem();

		auto contexte_generation = ContexteGenerationCode{};
		auto assembleuse = assembleuse_arbre(contexte_generation);

		auto module = contexte_generation.cree_module("", "");

		std::ostream os(nullptr);
		charge_fichier(os, module, "", nom_module.c_str(), contexte_generation, {});

		if (genere_code) {
			noeud::performe_validation_semantique(assembleuse, contexte_generation);
			noeud::genere_code_C(assembleuse, contexte_generation, "", os);
		}
	}
	catch (const erreur::frappe &e) {
		if (imprime_message) {
			std::cerr << e.message() << '\n';
		}

		erreur_lancee = true;
		type_correcte = type == e.type();
	}

	std::filesystem::current_path(dossier_origine);

	return { erreur_lancee, type_correcte };
}

void test_modules(dls::test_unitaire::Controleuse &controleuse)
{
	CU_DEBUTE_PROPOSITION(
				controleuse,
				"La compilation de module fonctionne si le module est connu.");
	{
		auto const [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_module_correcte.kuri", false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut pas appeler une fonction inconnue d'un module.");
	{
		auto const [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_fonction_inconnue_module.kuri", false, erreur::type_erreur::FONCTION_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut importer un module inconnu.");
	{
		auto const [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_module_inconnu.kuri", false, erreur::type_erreur::MODULE_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(
				controleuse,
				"On ne peut accéder à un module non-importé.");
	{
		auto const [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_utilisation_module_inconnu.kuri", false, erreur::type_erreur::VARIABLE_INCONNUE);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}
