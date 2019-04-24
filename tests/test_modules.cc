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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/Module.h>
#pragma GCC diagnostic pop

#include <filesystem>

#include "analyseuse_grammaire.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"
#include "modules.hh"

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

		std::ostream os(nullptr);
		charge_module(os, "", nom_module, contexte_generation, {}, true);

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
				"fichiers_tests/test_utilisation_module_inconnu.kuri", false, erreur::type_erreur::MODULE_INCONNU);

		CU_VERIFIE_CONDITION(controleuse, erreur_lancee == true);
		CU_VERIFIE_CONDITION(controleuse, type_correcte == true);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}
