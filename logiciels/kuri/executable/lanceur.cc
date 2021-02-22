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

#include <filesystem>
#include <fstream>
#include <iostream>

#include "biblexternes/iprof/prof.h"

#include <thread>

#include "compilation/compilatrice.hh"
#include "compilation/coulisse_llvm.hh"
#include "compilation/environnement.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/options.hh"
#include "compilation/statistiques.hh"
#include "compilation/tacheronne.hh"

#include "representation_intermediaire/constructrice_ri.hh"

#include "date.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#define AVEC_THREADS

static void imprime_stats(
		Statistiques const &stats,
		dls::chrono::compte_seconde debut_compilation)
{
	auto const temps_total = debut_compilation.temps();

	auto const temps_tampons = stats.temps_tampons;
	auto const temps_chargement = stats.temps_chargement;
	auto const temps_typage = stats.temps_typage;
	auto const temps_parsage = stats.temps_parsage;
	auto const temps_lexage = stats.temps_lexage;

	auto const temps_scene = temps_tampons
							 + temps_lexage
							 + temps_parsage
							 + temps_chargement
							 + temps_typage
							 + stats.temps_metaprogrammes
							 + stats.temps_ri;

	auto const temps_coulisse = stats.temps_generation_code
								+ stats.temps_fichier_objet
								+ stats.temps_executable;

	auto const temps_aggrege = temps_scene + temps_coulisse + stats.temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return (x * 100.0 / total);
	};

	auto const mem_totale = stats.stats_fichiers.totaux.memoire_tampons
							+ stats.stats_fichiers.totaux.memoire_lexemes
							+ stats.stats_arbre.totaux.memoire
							+ stats.memoire_compilatrice
							+ stats.stats_graphe_dependance.totaux.memoire
							+ stats.stats_types.totaux.memoire
							+ stats.stats_operateurs.totaux.memoire
							+ stats.memoire_ri;

	auto memoire_consommee = memoire::consommee();

	auto const nombre_lignes = stats.stats_fichiers.totaux.nombre_lignes;
	auto const lignes_double = static_cast<double>(nombre_lignes);
	auto const debit_lignes = static_cast<int>(lignes_double / temps_aggrege);
	auto const debit_lignes_scene = static_cast<int>(lignes_double / (temps_scene - stats.temps_metaprogrammes));
	auto const debit_lignes_coulisse = static_cast<int>(lignes_double / temps_coulisse);
	auto const debit_seconde = static_cast<int>(static_cast<double>(memoire_consommee) / temps_aggrege);

	auto tableau = Tableau({ "Nom", "Valeur", "Unité", "Pourcentage" });
	tableau.alignement(1, Alignement::DROITE);
	tableau.alignement(3, Alignement::DROITE);

	tableau.ajoute_ligne({ "Temps total", formatte_nombre(temps_total * 1000.0), "ms" });
	tableau.ajoute_ligne({ "Temps aggrégé", formatte_nombre(temps_aggrege * 1000.0), "ms" });
	tableau.ajoute_ligne({ "Nombre de modules", formatte_nombre(stats.nombre_modules), "" });
	tableau.ajoute_ligne({ "Nombre de lignes", formatte_nombre(nombre_lignes), "" });
	tableau.ajoute_ligne({ "Lignes / seconde", formatte_nombre(debit_lignes), "" });
	tableau.ajoute_ligne({ "Lignes / seconde (scène)", formatte_nombre(debit_lignes_scene), "" });
	tableau.ajoute_ligne({ "Lignes / seconde (coulisse)", formatte_nombre(debit_lignes_coulisse), "" });
	tableau.ajoute_ligne({ "Débit par seconde", formatte_nombre(debit_seconde), "o/s" });

	tableau.ajoute_ligne({ "Arbre Syntaxique", "", "" });
	tableau.ajoute_ligne({ "- Nombre Identifiants", formatte_nombre(stats.nombre_identifiants), "" });
	tableau.ajoute_ligne({ "- Nombre Lexèmes", formatte_nombre(stats.stats_fichiers.totaux.nombre_lexemes), "" });
	tableau.ajoute_ligne({ "- Nombre Noeuds", formatte_nombre(stats.stats_arbre.totaux.compte), "" });
	tableau.ajoute_ligne({ "- Nombre Noeuds Déps", formatte_nombre(stats.stats_graphe_dependance.totaux.compte), "" });
	tableau.ajoute_ligne({ "- Nombre Opérateurs", formatte_nombre(stats.stats_operateurs.totaux.compte), "" });
	tableau.ajoute_ligne({ "- Nombre Types", formatte_nombre(stats.stats_types.totaux.compte), "" });

	tableau.ajoute_ligne({ "Mémoire", "", "" });
	tableau.ajoute_ligne({ "- Suivie", formatte_nombre(mem_totale), "o" });
	tableau.ajoute_ligne({ "- Effective", formatte_nombre(memoire_consommee), "o" });
	tableau.ajoute_ligne({ "- Arbre", formatte_nombre(stats.stats_arbre.totaux.memoire), "o" });
	tableau.ajoute_ligne({ "- Compilatrice", formatte_nombre(stats.memoire_compilatrice), "o" });
	tableau.ajoute_ligne({ "- Graphe", formatte_nombre(stats.stats_graphe_dependance.totaux.memoire), "o" });
	tableau.ajoute_ligne({ "- Lexèmes", formatte_nombre(stats.stats_fichiers.totaux.memoire_lexemes), "o" });
	tableau.ajoute_ligne({ "- MV", formatte_nombre(stats.memoire_mv), "o" });
	tableau.ajoute_ligne({ "- Opérateurs", formatte_nombre(stats.stats_operateurs.totaux.memoire), "o" });
	tableau.ajoute_ligne({ "- RI", formatte_nombre(stats.memoire_ri), "o" });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(stats.stats_fichiers.totaux.memoire_tampons), "o" });
	tableau.ajoute_ligne({ "- Types", formatte_nombre(stats.stats_types.totaux.memoire), "o" });
	tableau.ajoute_ligne({ "Nombre allocations", formatte_nombre(memoire::nombre_allocations()), "" });
	tableau.ajoute_ligne({ "Nombre métaprogrammes", formatte_nombre(stats.nombre_metaprogrammes_executes), "" });

	tableau.ajoute_ligne({ "Temps Scène", formatte_nombre(temps_scene * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_scene, temps_total)) });
	tableau.ajoute_ligne({ "- Chargement", formatte_nombre(temps_chargement * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_chargement, temps_scene)) });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(temps_tampons * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_tampons, temps_scene)) });
	tableau.ajoute_ligne({ "- Lexage", formatte_nombre(temps_lexage * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_lexage, temps_scene)) });
	tableau.ajoute_ligne({ "- Métaprogrammes", formatte_nombre(stats.temps_metaprogrammes * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_parsage, temps_scene)) });
	tableau.ajoute_ligne({ "- Syntaxage", formatte_nombre(temps_parsage * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_parsage, temps_scene)) });
	tableau.ajoute_ligne({ "- Typage", formatte_nombre(temps_typage * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_typage, temps_scene)) });
	tableau.ajoute_ligne({ "- RI", formatte_nombre(stats.temps_ri * 1000.0), "ms", formatte_nombre(calc_pourcentage(stats.temps_ri, temps_scene)) });

	tableau.ajoute_ligne({ "Temps Coulisse", formatte_nombre(temps_coulisse * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_coulisse, temps_total)) });
	tableau.ajoute_ligne({ "- Génération Code", formatte_nombre(stats.temps_generation_code * 1000.0), "ms", formatte_nombre(calc_pourcentage(stats.temps_generation_code, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Fichier Objet", formatte_nombre(stats.temps_fichier_objet * 1000.0), "ms", formatte_nombre(calc_pourcentage(stats.temps_fichier_objet, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Exécutable", formatte_nombre(stats.temps_executable * 1000.0), "ms", formatte_nombre(calc_pourcentage(stats.temps_executable, temps_coulisse)) });

	tableau.ajoute_ligne({ "Temps Nettoyage", formatte_nombre(stats.temps_nettoyage * 1000.0), "ms" });

	imprime_tableau(tableau);

	return;
}

void lance_tacheronne(Tacheronne *tacheronne)
{
	try {
		tacheronne->gere_tache();
	}
	catch (const erreur::frappe &e) {
		std::cerr << e.message() << '\n';
		tacheronne->compilatrice.possede_erreur = true;
	}
}

static void imprime_stats_tableau(EntreesStats<EntreeNombreMemoire> &stats)
{
	std::sort(stats.entrees.begin(), stats.entrees.end(), [](const EntreeNombreMemoire &a, const EntreeNombreMemoire &b) { return a.memoire > b.memoire; });

	auto tableau = Tableau({ "Nom", "Compte", "Mémoire" });
	tableau.alignement(1, Alignement::DROITE);
	tableau.alignement(2, Alignement::DROITE);

	POUR (stats.entrees) {
		tableau.ajoute_ligne({ dls::chaine(it.nom), formatte_nombre(it.compte), formatte_nombre(it.memoire) });
	}

	tableau.ajoute_ligne({ "", formatte_nombre(stats.totaux.compte), formatte_nombre(stats.totaux.memoire) });

	imprime_tableau(tableau);
}

static void imprime_stats_fichier(EntreesStats<EntreeFichier> &stats)
{
	std::sort(stats.entrees.begin(), stats.entrees.end(), [](const EntreeFichier &a, const EntreeFichier &b) { return a.nombre_lignes > b.nombre_lignes; });

	auto tableau = Tableau({ "Nom", "Lignes", "Mémoire", "Lexèmes", "Mémoire Lexèmes" });
	tableau.alignement(1, Alignement::DROITE);
	tableau.alignement(2, Alignement::DROITE);
	tableau.alignement(3, Alignement::DROITE);
	tableau.alignement(4, Alignement::DROITE);

	POUR (stats.entrees) {
		tableau.ajoute_ligne({ dls::chaine(it.nom), formatte_nombre(it.nombre_lignes), formatte_nombre(it.memoire_tampons), formatte_nombre(it.nombre_lexemes), formatte_nombre(it.memoire_lexemes) });
	}

	tableau.ajoute_ligne({ "", formatte_nombre(stats.totaux.nombre_lignes), formatte_nombre(stats.totaux.memoire_tampons), formatte_nombre(stats.totaux.nombre_lexemes), formatte_nombre(stats.totaux.memoire_lexemes) });

	imprime_tableau(tableau);
}

static void imprime_stats_tableaux(EntreesStats<EntreeTaille> &stats)
{
	std::sort(stats.entrees.begin(), stats.entrees.end(), [](const EntreeTaille &a, const EntreeTaille &b) { return a.taille > b.taille; });

	auto tableau = Tableau({ "Nom", "Taille Maximum"});
	tableau.alignement(1, Alignement::DROITE);

	POUR (stats.entrees) {
		tableau.ajoute_ligne({ dls::chaine(it.nom), formatte_nombre(it.taille) });
	}

	imprime_tableau(tableau);
}

static void imprime_stats_detaillee(Statistiques &stats)
{
	std::cout << "Arbre Syntaxique :\n";
	imprime_stats_tableau(stats.stats_arbre);
	std::cout << "Graphe Dépendance :\n";
	imprime_stats_tableau(stats.stats_graphe_dependance);
	std::cout << "RI :\n";
	imprime_stats_tableau(stats.stats_ri);
	std::cout << "Operateurs :\n";
	imprime_stats_tableau(stats.stats_operateurs);
	std::cout << "Types :\n";
	imprime_stats_tableau(stats.stats_types);
	std::cout << "Fichiers :\n";
	imprime_stats_fichier(stats.stats_fichiers);
	std::cout << "Tableaux :\n";
	imprime_stats_tableaux(stats.stats_tableaux);
}

#if 0
static void valide_blocs_modules(EspaceDeTravail const &espace)
{
	POUR_TABLEAU_PAGE (espace.graphe_dependance->noeuds) {
		if (it.type != TypeNoeudDependance::FONCTION) {
			continue;
		}

		auto noeud = it.noeud_syntaxique;

		auto fichier = espace.fichier(noeud->lexeme->fichier);
		auto module = fichier->module;

		auto bloc = noeud->bloc_parent;

		while (bloc->bloc_parent) {
			bloc = bloc->bloc_parent;
		}

		if (module->bloc != bloc) {
			std::cerr << "Une fonction n'est pas le bon bloc parent !\n";
		}
	}
}

static void valide_blocs_modules(Compilatrice &compilatrice)
{
	POUR_TABLEAU_PAGE ((*compilatrice.espaces_de_travail.verrou_lecture())) {
		valide_blocs_modules(it);
	}
}
#endif

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER [--tests]\n";
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	auto const chemin_fichier = argv[1];

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	std::ostream &os = std::cout;

	auto resultat = 0;
	auto debut_compilation   = dls::chrono::compte_seconde();
	auto debut_nettoyage     = dls::chrono::compte_seconde(false);

	precompile_objet_r16(chemin_racine_kuri);

	auto stats = Statistiques();
	auto compilatrice = Compilatrice{};

	if (argc == 3) {
		if (strcmp(argv[2], "--tests") == 0) {
			compilatrice.active_tests = true;
		}
	}

	try {
		/* enregistre le dossier d'origine */
		auto dossier_origine = std::filesystem::current_path();

		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto nom_fichier = chemin.stem();

		compilatrice.racine_kuri = chemin_racine_kuri;

		/* Charge d'abord le module basique. */
		auto espace_defaut = compilatrice.demarre_un_espace_de_travail({}, "Espace 1");
		compilatrice.espace_de_travail_defaut = espace_defaut;

		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;

		auto module = espace_defaut->trouve_ou_cree_module(compilatrice.sys_module, ID::chaine_vide, dossier.c_str());
		compilatrice.ajoute_fichier_a_la_compilation(espace_defaut, nom_fichier.c_str(), module, {});

#ifdef AVEC_THREADS
		auto nombre_tacheronnes = std::thread::hardware_concurrency();

		kuri::tableau<Tacheronne *> tacheronnes;
		tacheronnes.reserve(nombre_tacheronnes);

		for (auto i = 0u; i < nombre_tacheronnes; ++i) {
			tacheronnes.ajoute(memoire::loge<Tacheronne>("Tacheronne", compilatrice));
		}

		// pour le moment, une seule tacheronne peut exécuter du code
		tacheronnes[0]->drapeaux  =  DrapeauxTacheronne::PEUT_EXECUTER;
		tacheronnes[1]->drapeaux &= ~DrapeauxTacheronne::PEUT_EXECUTER;

		for (auto i = 2u; i < nombre_tacheronnes; ++i) {
			tacheronnes[i]->drapeaux = DrapeauxTacheronne(0);
		}

		auto drapeaux = DrapeauxTacheronne::PEUT_LEXER | DrapeauxTacheronne::PEUT_PARSER;

		for (auto i = 0u; i < nombre_tacheronnes; ++i) {
			tacheronnes[i]->drapeaux |= drapeaux;
		}

		kuri::tableau<std::thread *> threads;
		threads.reserve(nombre_tacheronnes);

		POUR (tacheronnes) {
			threads.ajoute(memoire::loge<std::thread>("std::thread", lance_tacheronne, it));
		}

		POUR (threads) {
			it->join();
			memoire::deloge("std::thread", it);
		}
#else
		auto tacheronne = Tacheronne(compilatrice);
		auto tacheronne_mp = Tacheronne(compilatrice);

		tacheronne.drapeaux &= ~DrapeauxTacheronne::PEUT_EXECUTER;
		tacheronne_mp.drapeaux = DrapeauxTacheronne::PEUT_EXECUTER;

		lance_tacheronne(&tacheronne);
		lance_tacheronne(&tacheronne_mp);
#endif

		if (compilatrice.chaines_ajoutees_a_la_compilation->taille()) {
			auto fichier_chaines = std::ofstream(".chaines_ajoutées");

			auto d = hui_systeme();

			fichier_chaines << "Fichier créé le " << d.jour << "/" << d.mois << "/" << d.annee
							<< " à " << d.heure << ':' << d.minute << ':' << d.seconde << "\n\n";

			POUR (*compilatrice.chaines_ajoutees_a_la_compilation.verrou_lecture()) {
				fichier_chaines << it;
				fichier_chaines << '\n';
			}
		}

		/* restore le dossier d'origine */
		std::filesystem::current_path(dossier_origine);

		if (!compilatrice.possede_erreur && compilatrice.espace_de_travail_defaut->options.emets_metriques) {
			POUR (tacheronnes) {
				stats.temps_executable = std::max(stats.temps_executable, it->temps_executable);
				stats.temps_fichier_objet = std::max(stats.temps_fichier_objet, it->temps_fichier_objet);
				stats.temps_generation_code = std::max(stats.temps_generation_code, it->temps_generation_code);
				stats.temps_ri = std::max(stats.temps_ri, it->constructrice_ri.temps_generation);
				stats.temps_lexage = std::max(stats.temps_lexage, it->temps_lexage);
				stats.temps_parsage = std::max(stats.temps_parsage, it->temps_parsage);
				stats.temps_typage = std::max(stats.temps_typage, it->temps_validation);
				stats.temps_scene = std::max(stats.temps_scene, it->temps_scene);
				stats.temps_chargement = std::max(stats.temps_chargement, it->temps_chargement);
				stats.temps_tampons = std::max(stats.temps_tampons, it->temps_tampons);

				it->constructrice_ri.rassemble_statistiques(stats);
				it->allocatrice_noeud.rassemble_statistiques(stats);

				it->mv.rassemble_statistiques(stats);

				//std::cerr << "tâcheronne " << it->id << " a dormis pendant " << it->temps_passe_a_dormir << "ms\n";

#ifdef STATISTIQUES_DETAILLEES
				auto imprime_stats = [](const EntreesStats<EntreeTemps> &entrees) {
					std::cerr << entrees.nom << " :\n";
					for (auto &entree : entrees.entrees) {
						std::cerr << "-- " << entree.nom << " : " << entree.temps << '\n';
					}
				};
				if ((it->drapeaux & DrapeauxTacheronne::PEUT_TYPER) == DrapeauxTacheronne::PEUT_TYPER) {
					imprime_stats(it->stats_typage.validation_decl);
					imprime_stats(it->stats_typage.validation_appel);
					imprime_stats(it->stats_typage.ref_decl);
					imprime_stats(it->stats_typage.operateurs_unaire);
					imprime_stats(it->stats_typage.operateurs_binaire);
					imprime_stats(it->stats_typage.fonctions);
					imprime_stats(it->stats_typage.enumerations);
					imprime_stats(it->stats_typage.structures);
					imprime_stats(it->stats_typage.assignations);
				}
#endif
			}

			compilatrice.rassemble_statistiques(stats);

			stats.memoire_ri = stats.stats_ri.totaux.memoire;
		}

		POUR (tacheronnes) {
			memoire::deloge("Tacheronne", it);
		}

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = dls::chrono::compte_seconde();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
		compilatrice.possede_erreur = true;
		resultat = static_cast<int>(erreur_frappe.type());
	}

	stats.temps_nettoyage = debut_nettoyage.temps();

	if (!compilatrice.possede_erreur && compilatrice.espace_de_travail_defaut->options.emets_metriques) {
		imprime_stats(stats, debut_compilation);
#ifdef STATISTIQUES_DETAILLEES
		imprime_stats_detaillee(stats);
#endif
	}

#ifdef AVEC_LLVM
	issitialise_llvm();
#endif

	return resultat;
}
