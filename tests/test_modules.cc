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

#include <llvm/IR/Module.h>

#include <filesystem>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"
#include "modules.hh"

static std::pair<bool, bool> retourne_erreur_module_lancee(
		const char *chemin_fichier,
		const bool imprime_message,
		const erreur::type_erreur type,
		const bool genere_code = false)
{
	auto erreur_lancee = false;
	auto type_correcte = false;

	auto dossier_origine = std::filesystem::current_path();

	try {
		auto chemin = std::filesystem::path(chemin_fichier);
		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		auto nom_module = chemin.stem();

		auto assembleuse = assembleuse_arbre();
		auto contexte_generation = ContexteGenerationCode{};
		contexte_generation.assembleuse = &assembleuse;

		std::ostream os(nullptr);
		charge_module(os, nom_module, contexte_generation, {}, true);

		if (genere_code) {
			auto module_llvm = llvm::Module("test", contexte_generation.contexte);
			contexte_generation.module_llvm = &module_llvm;

			assembleuse.genere_code_llvm(contexte_generation);
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

void test_modules(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_DEBUTE_PROPOSITION(
				controleur,
				"La compilation de module fonctionne si le module est connu.");
	{
		const auto [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_module_correcte.kuri", false, erreur::type_erreur::AUCUNE_ERREUR);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == false);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut pas appeler une fonction inconnue d'un module.");
	{
		const auto [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_fonction_inconnue_module.kuri", false, erreur::type_erreur::FONCTION_INCONNUE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut importer un module inconnu.");
	{
		const auto [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_module_inconnu.kuri", false, erreur::type_erreur::MODULE_INCONNU);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);

	/* À FAIRE : normalement l'erreur doit être de type MODULE_INCONNU, mais une
	 * erreur de type FONCTION_INCONNUE est lancée lors de l'analyse car on ne
	 * trouve pas la fonction : il faut retarder le lancement d'erreur inconnue
	 * pour entrevenir lors de l'analyse sémantique. */
	CU_DEBUTE_PROPOSITION(
				controleur,
				"On ne peut accéder à un module non-importé.");
	{
		const auto [erreur_lancee, type_correcte] = retourne_erreur_module_lancee(
				"fichiers_tests/test_utilisation_module_inconnu.kuri", false, erreur::type_erreur::FONCTION_INCONNUE);

		CU_VERIFIE_CONDITION(controleur, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleur, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleur);
}
