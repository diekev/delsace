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

#include "test_decoupage.h"

#include <filesystem>
#include <fstream>
#include <sys/wait.h>
#include <unistd.h>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "compilation/assembleuse_arbre.h"
#include "compilation/compilatrice.hh"
#include "compilation/modules.hh"
#include "compilation/tacheronne.hh"

#include "compilation/erreur.h"

struct Test {
	const char *cas = "";
	const char *source = "";
	erreur::type_erreur resultat_attendu = erreur::type_erreur::AUCUNE_ERREUR;
};

static Test tests_unitaires[] = {
	// À FAIRE : désactivation de ces tests pour le moment car la fonctionnalité est brisée depuis le réusinage de l'arbre
//	{
//		"",
//		"fichiers/test_module_correcte.kuri",
//		erreur::type_erreur::AUCUNE_ERREUR
//	},
//	{
//		"",
//		"fichiers/test_fonction_inconnue_module.kuri",
//		erreur::type_erreur::FONCTION_INCONNUE
//	},
	{
		"",
		"fichiers/test_lexage_nombre_correcte.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_lexage_nombre_erreur.kuri",
		erreur::type_erreur::LEXAGE
	},
	{
		"",
		"fichiers/test_module_inconnu.kuri",
		erreur::type_erreur::MODULE_INCONNU
	},
	{
		"",
		"fichiers/test_utilisation_module_inconnu.kuri",
		erreur::type_erreur::VARIABLE_INCONNUE
	},
	{
		"",
		"fichiers/test_type_retour_rien.kuri",
		erreur::type_erreur::ASSIGNATION_RIEN
	},
	{
		"",
		"fichiers/test_assignation_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_assignation_invalide.kuri",
		erreur::type_erreur::ASSIGNATION_INVALIDE
	},
	{
		"",
		"fichiers/test_assignation_types_differents.kuri",
		erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE
	},
	{
		"",
		"fichiers/test_erreur_syntaxage.kuri",
		erreur::type_erreur::SYNTAXAGE
	},
	{
		"",
		"fichiers/test_boucle_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_boucle_erreur_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_boucle_erreur_controle_invalide.kuri",
		erreur::type_erreur::CONTROLE_INVALIDE
	},
	{
		"",
		"fichiers/test_boucle_erreur_variable_inconnue.kuri",
		erreur::type_erreur::VARIABLE_INCONNUE
	},
	{
		"",
		"fichiers/test_appel_fonction_erreur_nombre_argument.kuri",
		erreur::type_erreur::NOMBRE_ARGUMENT
	},
	{
		"",
		"fichiers/test_appel_fonction_erreur_fonction_inconnue.kuri",
		erreur::type_erreur::FONCTION_INCONNUE
	},
	{
		"",
		"fichiers/test_appel_fonction_erreur_argument_inconnu.kuri",
		erreur::type_erreur::ARGUMENT_INCONNU
	},
	{
		"",
		"fichiers/test_appel_fonction_erreur_type_argument.kuri",
		erreur::type_erreur::TYPE_ARGUMENT
	},
	{
		"",
		"fichiers/test_appel_fonction_erreur_argument_redefini.kuri",
		erreur::type_erreur::ARGUMENT_REDEFINI
	},
	{
		"",
		"fichiers/test_declaration_fonctin_erreur_normale.kuri",
		erreur::type_erreur::NORMAL
	},
	{
		"",
		"fichiers/test_expression_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_condition_controle_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_condition_controle_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_expression_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_operateurs_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_operateurs_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_fonction_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_fonction_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_structure_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_structure_membre_inconnu.kuri",
		erreur::type_erreur::MEMBRE_INCONNU
	},
	{
		"",
		"fichiers/test_structure_redefinie.kuri",
		erreur::type_erreur::VARIABLE_REDEFINIE // À FAIRE : considère plutôt SYMBOLE_REDIFINI, et fusionne tous les tests
	},
	{
		"",
		"fichiers/test_type_inconnu.kuri",
		erreur::type_erreur::TYPE_INCONNU
	},
	{
		"",
		"fichiers/test_structure_variable_inconnue.kuri",
		erreur::type_erreur::VARIABLE_INCONNUE
	},
	{
		"",
		"fichiers/test_structure_types_differents.kuri",
		erreur::type_erreur::TYPE_DIFFERENTS
	},
	{
		"",
		"fichiers/test_structure_membre_redefini.kuri",
		erreur::type_erreur::MEMBRE_REDEFINI
	},
	{
		"",
		"fichiers/test_tableau_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_tableau_type_argument.kuri",
		erreur::type_erreur::TYPE_ARGUMENT
	},
	{
		"",
		"fichiers/test_transtypage_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_transtypage_assignation_mauvais_type.kuri",
		erreur::type_erreur::ASSIGNATION_MAUVAIS_TYPE
	},
	{
		"",
		"fichiers/test_variable_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
	{
		"",
		"fichiers/test_variable_inconnue.kuri",
		erreur::type_erreur::VARIABLE_INCONNUE
	},
	{
		"",
		"fichiers/test_variable_redefinie.kuri",
		erreur::type_erreur::VARIABLE_REDEFINIE
	},
	{
		"",
		"fichiers/test_variable_redefinie.kuri",
		erreur::type_erreur::VARIABLE_REDEFINIE
	},
	{
		"",
		"fichiers/test_appel_fonction_aucune_erreur.kuri",
		erreur::type_erreur::AUCUNE_ERREUR
	},
};

static erreur::type_erreur lance_test(lng::tampon_source &tampon)
{
	auto chemin_courant = std::filesystem::current_path();
	std::filesystem::current_path("/opt/bin/kuri/fichiers_tests/fichiers/");

	auto compilatrice = Compilatrice{};
	compilatrice.racine_kuri = getenv("RACINE_KURI");

	auto espace = compilatrice.demarre_un_espace_de_travail({}, "");

	/* Charge d'abord le module basique, car nous en avons besoin pour le type ContexteProgramme. */
	compilatrice.importe_module(espace, "Kuri", {});

	/* Ne nomme pas le module, car c'est le module racine. */
	auto module = espace->cree_module("", "");
	auto fichier = espace->cree_fichier("", "", compilatrice.importe_kuri);
	fichier->tampon = tampon;
	fichier->module = module;

	compilatrice.ordonnanceuse->cree_tache_pour_lexage(espace, fichier);

	try {
		auto tacheronne = Tacheronne(compilatrice);
		tacheronne.gere_tache();
	}
	catch (const erreur::frappe &e) {
		std::filesystem::current_path(chemin_courant);
		return e.type();
	}

	std::filesystem::current_path(chemin_courant);
	return erreur::type_erreur::AUCUNE_ERREUR;
}

static auto decoupe_tampon(lng::tampon_source const &tampon)
{
	dls::tableau<lng::tampon_source> resultat;

	auto debut_cas = 0ul;
	auto fin_cas = 1ul;

	for (auto i = 0ul; i < tampon.nombre_lignes(); ++i) {
		auto ligne = dls::chaine(tampon[static_cast<long>(i)]);

		if (ligne.sous_chaine(0, 8) == "// Cas :") {
			debut_cas = i + 1;
		}

		if (ligne.sous_chaine(0, 8) == "// -----") {
			fin_cas = i;

			if (debut_cas < fin_cas) {
				auto sous_tampon = tampon.sous_tampon(debut_cas, fin_cas);
				resultat.pousse(sous_tampon);
			}
		}
	}

	fin_cas = static_cast<size_t>(tampon.nombre_lignes());

	if (debut_cas < fin_cas) {
		auto sous_tampon = tampon.sous_tampon(debut_cas, fin_cas);
		resultat.pousse(sous_tampon);
	}

	return resultat;
}

enum {
	ECHEC_CAR_CRASH,
	ECHEC_POUR_RAISON_INCONNUE,
	ECHEC_CAR_MAUVAIS_CODE_ERREUR,
	ECHEC_CAR_BOUCLE_INFINIE,
};

struct ResultatTest {
	dls::chaine fichier_origine{};
	dls::chaine chemin_fichier{};
	int raison_echec{};
	erreur::type_erreur erreur_attendue{};
	erreur::type_erreur erreur_recue{};
};

static auto ecris_fichier_tmp(dls::chaine const &source, int index)
{
	auto chemin_fichier_tmp = "/tmp/echec_test" + dls::vers_chaine(index) + ".kuri";

	std::ofstream of;
	of.open(chemin_fichier_tmp.c_str());
	of.write(source.c_str(), source.taille());

	return chemin_fichier_tmp;
}

int main()
{
	auto test_passes = 0;
	auto test_echoues = 0;

	auto resultats_tests = dls::tableau<ResultatTest>();

	POUR (tests_unitaires) {
		auto chemin = std::filesystem::path("fichiers_tests/") / it.source;

		if (std::filesystem::exists(chemin)) {
			auto compilatrice = Compilatrice{};
			compilatrice.importe_kuri = false;
			auto espace = compilatrice.demarre_un_espace_de_travail({}, "");

			auto contenu_fichier = charge_fichier(chemin.c_str(), *espace, {});
			auto tampon = lng::tampon_source(contenu_fichier);

			if (tampon.nombre_lignes() == 0) {
				//std::cerr << "Le fichier " << chemin << " est vide\n";
				continue;
			}

			auto cas = decoupe_tampon(tampon);

			for (auto &c : cas) {
				auto pid = fork();

				if (pid == 0) {
					auto res = lance_test(c);
					return static_cast<int>(res);
				}
				else if (pid > 0) {
					auto debut = dls::chrono::compte_seconde();

					while (true) {
						int status;
						pid_t result = waitpid(pid, &status, WNOHANG);

						if (result == 0) {
							/* L'enfant est toujours en vie, continue. */
						}
						else if (result == -1) {
							auto rt = ResultatTest();
							rt.raison_echec = ECHEC_POUR_RAISON_INCONNUE;
							rt.fichier_origine = it.source;
							rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

							resultats_tests.pousse(rt);

							test_echoues += 1;
							break;
						}
						else {
							if (!WIFEXITED(status)) {
								auto rt = ResultatTest();
								rt.raison_echec = ECHEC_CAR_CRASH;
								rt.fichier_origine = it.source;
								rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

								resultats_tests.pousse(rt);

								test_echoues += 1;
							}
							else {
								if (WEXITSTATUS(status) == static_cast<int>(it.resultat_attendu)) {
									test_passes += 1;
								}
								else {
									auto rt = ResultatTest();
									rt.erreur_recue = static_cast<erreur::type_erreur>(WEXITSTATUS(status));
									rt.erreur_attendue = it.resultat_attendu;
									rt.raison_echec = ECHEC_CAR_MAUVAIS_CODE_ERREUR;
									rt.fichier_origine = it.source;
									rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

									resultats_tests.pousse(rt);

									test_echoues += 1;
								}
							}

							break;
						}

						auto temps = debut.temps();

						if (temps > 25.0) {
							kill(pid, SIGKILL);

							auto rt = ResultatTest();
							rt.raison_echec = ECHEC_CAR_BOUCLE_INFINIE;
							rt.fichier_origine = it.source;
							rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

							resultats_tests.pousse(rt);

							test_echoues += 1;
							break;
						}
					}
				}
			}

			std::cout << '.' << std::flush;
		}
		else {
			continue;
		}
	}

	std::cout << '\n';

	POUR (resultats_tests) {
		std::cout << "----------------------------------------\n";

		switch (it.raison_echec) {
			case ECHEC_CAR_CRASH:
			{
				std::cout << "Test échoué à cause d'un crash\n";
				break;
			}
			case ECHEC_CAR_BOUCLE_INFINIE:
			{
				std::cout << "Test échoué à cause d'une boucle infinie\n";
				break;
			}
			case ECHEC_CAR_MAUVAIS_CODE_ERREUR:
			{
				std::cout << "Test échoué à cause d'un mauvais code erreur\n";
				std::cout << "-- attendu : " << erreur::chaine_erreur(it.erreur_attendue) << '\n';
				std::cout << "-- reçu    : " << erreur::chaine_erreur(it.erreur_recue) << '\n';
				break;
			}
			case ECHEC_POUR_RAISON_INCONNUE:
			{
				std::cout << "Test échoué pour une raison inconnue\n";
				break;
			}
		}

		std::cout << "Fichier d'origine : " << it.fichier_origine << '\n';
		std::cout << "Fichier source    : " << it.chemin_fichier << '\n';
	}

	std::cout << '\n';
	std::cout << "SUCCES (" << test_passes << ")\n";
	std::cout << "ÉCHECS (" << test_echoues << ")\n";
}
