/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include <fstream>
#include <iostream>
#include <thread>

#include "compilation/compilatrice.hh"
#include "compilation/coulisse_llvm.hh"
#include "compilation/environnement.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/tacheronne.hh"

#include "statistiques/statistiques.hh"

#include "biblinternes/chrono/chronometrage.hh"

#include "structures/chemin_systeme.hh"

#define AVEC_THREADS

/**
 * Fonction de rappel pour les fils d'exécutions.
 */
static void lance_tacheronne(Tacheronne *tacheronne)
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

/**
 * Imprime les fichiers utilisés dans le fichier spécifié via la ligne de commande.
 */
static void imprime_fichiers_utilises(Compilatrice &compilatrice)
{
    auto chemin = vers_std_path(compilatrice.arguments.chemin_fichier_utilises);
    if (chemin.empty()) {
        return;
    }

    std::ofstream fichier_sortie(chemin);

    POUR_TABLEAU_PAGE (compilatrice.sys_module->fichiers) {
        fichier_sortie << it.chemin() << "\n";
    }
}

static void rassemble_statistiques(Compilatrice &compilatrice,
                                   Statistiques &stats,
                                   kuri::tableau<Tacheronne *> const &tacheronnes)
{
    if (!compilatrice.espace_de_travail_defaut->options.emets_metriques) {
        return;
    }

    POUR (tacheronnes) {
        stats.temps_executable = std::max(stats.temps_executable, it->temps_executable);
        stats.temps_fichier_objet = std::max(stats.temps_fichier_objet, it->temps_fichier_objet);
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
        if ((it->drapeaux & DrapeauxTacheronne::PEUT_TYPER) == DrapeauxTacheronne::PEUT_TYPER) {
            it->stats_typage.imprime_stats();
        }
#endif
    }

    compilatrice.rassemble_statistiques(stats);

    stats.memoire_ri = stats.stats_ri.totaux.memoire;
}

static void imprime_stats(Compilatrice const &compilatrice,
                          Statistiques const &stats,
                          dls::chrono::compte_seconde debut_compilation)
{
    if (!compilatrice.espace_de_travail_defaut->options.emets_metriques) {
        return;
    }

    imprime_stats(stats, debut_compilation);
    compilatrice.gestionnaire_code->imprime_stats();
#ifdef STATISTIQUES_DETAILLEES
    imprime_stats_detaillee(stats);
#endif
}

static std::optional<ArgumentsCompilatrice> parse_arguments(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Utilisation : " << argv[0] << " FICHIER [options...]\n";
        return {};
    }

    auto resultat = ArgumentsCompilatrice();

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--tests") == 0) {
            resultat.active_tests = true;
        }
        else if (strcmp(argv[i], "--emets_fichiers_utilises") == 0) {
            ++i;
            if (i >= argc) {
                std::cerr << "Argument manquant après --emets_fichiers_utilises\n";
                return {};
            }
            resultat.chemin_fichier_utilises = argv[i];
        }
        else if (strcmp(argv[i], "--profile_exécution") == 0) {
            resultat.profile_metaprogrammes = true;
        }
        else if (strcmp(argv[i], "--débogue_exécution") == 0) {
            resultat.debogue_execution = true;
        }
        else if (strcmp(argv[i], "--format_profile") == 0) {
            ++i;

            if (i >= argc) {
                std::cerr << "Argument manquant après --format_profile\n";
                return {};
            }

            if (strcmp(argv[i], "défaut") == 0 || strcmp(argv[i], "gregg") == 0) {
                resultat.format_rapport_profilage = FormatRapportProfilage::BRENDAN_GREGG;
            }
            else if (strcmp(argv[i], "échantillons_totaux") == 0) {
                resultat.format_rapport_profilage =
                    FormatRapportProfilage::ECHANTILLONS_TOTAL_POUR_FONCTION;
            }
            else {
                std::cerr << "Type de format de profile \"" << argv[i] << "\" inconnu\n";
                return {};
            }
        }
        else {
            std::cerr << "Argument \"" << argv[i] << "\" inconnu\n";
            return {};
        }
    }

    return resultat;
}

static bool compile_fichier(Compilatrice &compilatrice,
                            kuri::chaine_statique chemin_fichier,
                            std::ostream &os)
{
    auto debut_compilation = dls::chrono::compte_seconde();

    /* Compile les objets pour le support des r16 afin d'avoir la bibliothèque r16. */
    if (!precompile_objet_r16(kuri::chaine_statique(compilatrice.racine_kuri))) {
        return false;
    }

    /* Initialise les bibliothèques après avoir généré les objets r16. */
    if (!GestionnaireBibliotheques::initialise_bibliotheques_pour_execution(compilatrice)) {
        return false;
    }

    /* Crée les tâches pour les données requise de la typeuse. */
    Typeuse::crée_tâches_précompilation(compilatrice);

    /* enregistre le dossier d'origine */
    auto dossier_origine = kuri::chemin_systeme::chemin_courant();

    auto chemin = kuri::chemin_systeme::absolu(chemin_fichier);

    auto nom_fichier = chemin.nom_fichier_sans_extension();

    /* Charge d'abord le module basique. */
    auto espace_defaut = compilatrice.espace_de_travail_defaut;

    auto dossier = chemin.chemin_parent();
    kuri::chemin_systeme::change_chemin_courant(dossier);

    os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..."
       << std::endl;

    auto module = compilatrice.trouve_ou_cree_module(ID::chaine_vide, dossier);
    compilatrice.module_racine_compilation = module;
    compilatrice.ajoute_fichier_a_la_compilation(espace_defaut, nom_fichier, module, {});

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

    if (compilatrice.chaines_ajoutees_a_la_compilation->nombre_de_chaines()) {
        auto fichier_chaines = std::ofstream(".chaines_ajoutées");
        compilatrice.chaines_ajoutees_a_la_compilation->imprime_dans(fichier_chaines);
    }

    /* restore le dossier d'origine */
    kuri::chemin_systeme::change_chemin_courant(dossier_origine);

    if (compilatrice.possede_erreur()) {
        return false;
    }

    imprime_fichiers_utilises(compilatrice);

    auto stats = Statistiques();
    rassemble_statistiques(compilatrice, stats, tacheronnes);

    imprime_stats(compilatrice, stats, debut_compilation);

    os << "Nettoyage..." << std::endl;

    POUR (tacheronnes) {
        memoire::deloge("Tacheronne", it);
    }

    return true;
}

/* Détermine si la racine d'exécution est valide : que les dossiers que nous y espérons se trouver
 * s'y trouvent. Si un dossier est manquant, son chemin sera retourné. En cas de racine valide la
 * valeur optionnelle sera elle invalide. */
static std::optional<kuri::chemin_systeme> dossier_manquant_racine_execution(
    kuri::chaine_statique racine)
{
    /* Certains dossiers qui doivent être dans une installation valide de Kuri. */
    const kuri::chaine_statique dossiers[] = {"fichiers", "modules", "modules/Kuri"};

    POUR (dossiers) {
        auto const chemin_dossier = kuri::chemin_systeme(racine) / it;
        /* Si le dossier n'existe pas, le test échouera. */
        if (!kuri::chemin_systeme::est_dossier(chemin_dossier)) {
            return chemin_dossier;
        }
    }

    return {};
}

/* Détermine le chemin racine d'exécution de Kuri. Ceci est nécessaire puisque les modules de la
 * bibliothèque standarde sont stockés à coté de l'exécutable.
 *
 * Pour les développeurs, il est possible de définir la variable d'environnement « RACINE_KURI »
 * pour pointer vers la racine d'installation de Kuri, afin que le dossier de travail puisse être
 * différent de celui de l'installation.
 *
 * https://stackoverflow.com/questions/65548404/finding-path-of-execution-of-c-program
 */
static std::optional<kuri::chaine> determine_racine_execution_kuri()
{
    /* Tente de déterminer la racine depuis le système. */
    char tampon[1024];
    ssize_t len = readlink("/proc/self/exe", tampon, 1024);
    if (len < 0) {
        std::cerr
            << "Impossible de déterminer la racine d'exécution de Kuri depuis le système !\n";
        std::cerr << "Compilation avortée.\n";
        return {};
    }

    /* Ici nous avons le chemin complet vers l'exécutable, pour la racine il nous faut le chemin
     * parent. */
    auto chemin_executable = kuri::chaine(&tampon[0], len);
    auto racine = kuri::chemin_systeme(kuri::chemin_systeme(chemin_executable).chemin_parent());

    /* Vérifie que nous avons tous les dossiers. Si oui, nous sommes sans doute à la bonne adresse.
     */
    auto dossier_manquant = dossier_manquant_racine_execution(racine);
    if (!dossier_manquant) {
        return kuri::chaine(racine);
    }

    /* Essayons alors la variable d'environnement. */
    auto racine_env = getenv("RACINE_KURI");
    if (racine_env == nullptr) {
        std::cerr
            << "Impossible de déterminer la racine d'exécution de Kuri depuis l'environnement !\n";
        std::cerr << "Veuillez vous assurer que RACINE_KURI fait partie de l'environnement "
                     "d'exécution et pointe vers une installation valide de Kuri.\n";
        std::cerr << "Compilation avortée.\n";
        return {};
    }

    racine = kuri::chemin_systeme(racine_env);
    dossier_manquant = dossier_manquant_racine_execution(racine);

    if (dossier_manquant.has_value()) {
        std::cerr << "Racine d'exécution de Kuri invalide !\n";
        std::cerr << "Le dossier \"" << dossier_manquant.value() << "\" n'existe pas !\n";
        std::cerr << "Veuillez vérifier que votre installation est correcte.\n";
        std::cerr << "NOTE : le chemin racine utilisé provient de la variable d'environnement « "
                     "RACINE_KURI ».\n";
        std::cerr << "Compilation avortée.\n";
        return {};
    }

    return kuri::chaine(racine);
}

int main(int argc, char *argv[])
{
    std::ios::sync_with_stdio(false);

    auto const opt_racine_kuri = determine_racine_execution_kuri();
    if (!opt_racine_kuri.has_value()) {
        return 1;
    }

    auto const opt_arguments = parse_arguments(argc, argv);
    if (!opt_arguments.has_value()) {
        return 1;
    }

    auto const chemin_fichier = argv[1];
    if (!kuri::chemin_systeme::existe(chemin_fichier)) {
        std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
        return 1;
    }

    std::ostream &os = std::cout;

    auto compilatrice = Compilatrice(opt_racine_kuri.value(), opt_arguments.value());

    if (!compile_fichier(compilatrice, chemin_fichier, os)) {
        return 1;
    }

#ifdef AVEC_LLVM
    issitialise_llvm();
#endif

    return static_cast<int>(compilatrice.code_erreur());
}
