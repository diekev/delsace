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

#include <thread>

#include "compilation/compilatrice.hh"
#include "compilation/coulisse_llvm.hh"
#include "compilation/environnement.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/options.hh"
#include "compilation/tacheronne.hh"

#include "parsage/modules.hh"

#include "representation_intermediaire/constructrice_ri.hh"

#include "statistiques/statistiques.hh"

#include "date.hh"

#include "biblinternes/chrono/chronometrage.hh"

#define AVEC_THREADS

void lance_tacheronne(Tacheronne *tacheronne)
{
    tacheronne->gere_tache();
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

static void imprime_fichiers_utilises(std::ostream &os, EspaceDeTravail *espace)
{
    auto fichiers = espace->fichiers.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*fichiers)) {
        os << it.chemin() << "\n";
    }
}

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
        std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement "
                     "'RACINE_KURI' soit définie !\n";
        return 1;
    }

    auto const chemin_fichier = argv[1];

    if (!std::filesystem::exists(chemin_fichier)) {
        std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
        return 1;
    }

    std::ostream &os = std::cout;

    auto debut_compilation = dls::chrono::compte_seconde();
    auto debut_nettoyage = dls::chrono::compte_seconde(false);

    precompile_objet_r16(chemin_racine_kuri);

    auto stats = Statistiques();
    auto compilatrice = Compilatrice{};

    const char *nom_fichier_utilises = nullptr;

    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--tests") == 0) {
                compilatrice.active_tests = true;
            }
            else if (strcmp(argv[i], "--emets_fichiers_utilises") == 0) {
                ++i;
                nom_fichier_utilises = argv[i];
            }
            else {
            }
        }
    }

    {
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

        os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..."
           << std::endl;

        auto module = espace_defaut->trouve_ou_cree_module(
            compilatrice.sys_module, ID::chaine_vide, dossier.c_str());
        compilatrice.ajoute_fichier_a_la_compilation(
            espace_defaut, nom_fichier.c_str(), module, {});

#ifdef AVEC_THREADS
        auto nombre_tacheronnes = std::thread::hardware_concurrency();

        kuri::tableau<Tacheronne *> tacheronnes;
        tacheronnes.reserve(nombre_tacheronnes);

        for (auto i = 0u; i < nombre_tacheronnes; ++i) {
            tacheronnes.ajoute(memoire::loge<Tacheronne>("Tacheronne", compilatrice));
        }

        // pour le moment, une seule tacheronne peut exécuter du code
        tacheronnes[0]->drapeaux = DrapeauxTacheronne::PEUT_EXECUTER;
        tacheronnes[1]->drapeaux &= ~DrapeauxTacheronne::PEUT_EXECUTER;

        for (auto i = 2u; i < nombre_tacheronnes; ++i) {
            tacheronnes[i]->drapeaux = DrapeauxTacheronne(0);
        }

        auto drapeaux = DrapeauxTacheronne::PEUT_LEXER | DrapeauxTacheronne::PEUT_PARSER |
                        DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE;

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

        if (!compilatrice.possede_erreur() && nom_fichier_utilises) {
            std::ofstream fichier_fichiers_utilises(nom_fichier_utilises);

            POUR (*compilatrice.espaces_de_travail.verrou_lecture()) {
                imprime_fichiers_utilises(fichier_fichiers_utilises, it);
            }
        }

        if (!compilatrice.possede_erreur() &&
            compilatrice.espace_de_travail_defaut->options.emets_metriques) {
            POUR (tacheronnes) {
                stats.temps_executable = std::max(stats.temps_executable, it->temps_executable);
                stats.temps_fichier_objet = std::max(stats.temps_fichier_objet,
                                                     it->temps_fichier_objet);
                stats.temps_generation_code = std::max(stats.temps_generation_code,
                                                       it->temps_generation_code);
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

                // std::cerr << "tâcheronne " << it->id << " a dormis pendant " <<
                // it->temps_passe_a_dormir << "ms\n";

#ifdef STATISTIQUES_DETAILLEES
                auto imprime_stats = [](const EntreesStats<EntreeTemps> &entrees) {
                    std::cerr << entrees.nom << " :\n";
                    for (auto &entree : entrees.entrees) {
                        std::cerr << "-- " << entree.nom << " : " << entree.temps << '\n';
                    }
                };
                if ((it->drapeaux & DrapeauxTacheronne::PEUT_TYPER) ==
                    DrapeauxTacheronne::PEUT_TYPER) {
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

    stats.temps_nettoyage = debut_nettoyage.temps();

    if (!compilatrice.possede_erreur() &&
        compilatrice.espace_de_travail_defaut->options.emets_metriques) {
        imprime_stats(stats, debut_compilation);
#ifdef STATISTIQUES_DETAILLEES
        imprime_stats_detaillee(stats);
#endif
    }

#ifdef AVEC_LLVM
    issitialise_llvm();
#endif

    return static_cast<int>(compilatrice.code_erreur());
}
