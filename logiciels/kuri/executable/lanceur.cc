/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include <fstream>
#include <iostream>
#include <thread>

#include "compilation/compilatrice.hh"
#include "compilation/coulisse_llvm.hh"
#include "compilation/environnement.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/gestionnaire_code.hh"
#include "compilation/tacheronne.hh"

#include "statistiques/statistiques.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/langage/unicode.hh"

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

/* ------------------------------------------------------------------------- */
/** \name Parsage des arguments de compilation.
 * \{ */

class ParseuseArguments {
  private:
    int m_argc = 0;
    char **m_argv = nullptr;
    int m_index = 0;

  public:
    ParseuseArguments(int argc, char **argv, int index)
        : m_argc(argc), m_argv(argv), m_index(index)
    {
    }

    ParseuseArguments(ParseuseArguments const &) = delete;
    ParseuseArguments &operator=(ParseuseArguments const &) = delete;

    bool a_consommé_tous_les_arguments() const
    {
        return m_index >= m_argc;
    }

    std::optional<kuri::chaine_statique> donne_argument_suivant()
    {
        if (a_consommé_tous_les_arguments()) {
            return {};
        }

        return kuri::chaine_statique(m_argv[m_index++]);
    }
};

enum class ActionParsageArgument : uint8_t {
    CONTINUE,
    ARRÊTE_POUR_AIDE,
    ARRÊTE_CAR_ERREUR,
    DÉBUTE_LISTE_ARGUMENTS_MÉTAPROGRAMMES,
};

using TypeFonctionGestionArgument = ActionParsageArgument (*)(ParseuseArguments &,
                                                              ArgumentsCompilatrice &);

struct DescriptionArgumentCompilation {
    kuri::chaine nom = "";
    kuri::chaine nom_court = "";
    kuri::chaine nom_pour_aide = "";
    kuri::chaine description_pour_aide = "";
    TypeFonctionGestionArgument fonction{};
};

static ActionParsageArgument gère_argument_aide(ParseuseArguments & /*parseuse*/,
                                                ArgumentsCompilatrice & /*résultat*/);

static ActionParsageArgument gère_argument_pour_métaprogrammes(
    ParseuseArguments & /*parseuse*/, ArgumentsCompilatrice & /*résultat*/);

static ActionParsageArgument gère_argument_emets_fichiers_utilises(
    ParseuseArguments &parseuse, ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_tests(ParseuseArguments & /*parseuse*/,
                                                 ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_profile_exécution(ParseuseArguments & /*parseuse*/,
                                                             ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_débogue_exécution(ParseuseArguments & /*parseuse*/,
                                                             ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_format_profile(ParseuseArguments &parseuse,
                                                          ArgumentsCompilatrice &résultat);

static DescriptionArgumentCompilation descriptions_arguments[] = {
    {"--aide", "-a", "--aide, -a", "Imprime cette aide", gère_argument_aide},
    {"--",
     "",
     "",
     "Débute la liste des arguments pour les métaprogrammes",
     gère_argument_pour_métaprogrammes},
    {"--tests",
     "-t",
     "--tests, -t",
     "Active la compilation et l'exécution des directives #test",
     gère_argument_tests},
    {"--emets_fichiers_utilises",
     "",
     "--emets_fichiers_utilises [FICHIER]",
     "Imprime, à la fin de la compilation, la liste des fichiers utilisés dans le fichier "
     "spécifié",
     gère_argument_emets_fichiers_utilises},
    {"--profile_exécution",
     "",
     "",
     "Active le profilage des métaprogrammes. Émets un fichier pouvant être lu avec "
     "speedscope.app",
     gère_argument_profile_exécution},
    {"--format_profile",
     "",
     "--format_profile {défaut|gregg|échantillons_totaux}",
     "Définit le format du fichier de profilage",
     gère_argument_format_profile},
    {"--débogue_exécution",
     "",
     "",
     "Ajoute des instructions de débogage aux métaprogrammes afin de pouvoir détecter des "
     "erreurs",
     gère_argument_débogue_exécution},
};

static std::optional<DescriptionArgumentCompilation> donne_description_pour_arg(
    kuri::chaine_statique nom)
{
    POUR (descriptions_arguments) {
        if (it.nom == nom || it.nom_court == nom) {
            return it;
        }
    }

    return {};
}

static kuri::chaine_statique donne_nom_pour_aide(DescriptionArgumentCompilation const &desc)
{
    if (desc.nom_pour_aide.taille() > 0) {
        return desc.nom_pour_aide;
    }

    return desc.nom;
}

static int calcule_taille_utf8(kuri::chaine_statique chaine)
{
    int résultat = 0;
    for (auto i = 0l; i < chaine.taille();) {
        auto n = lng::nombre_octets(&chaine.pointeur()[i]);
        résultat += 1;
        i += n;
    }
    return résultat;
}

static ActionParsageArgument gère_argument_aide(ParseuseArguments & /*parseuse*/,
                                                ArgumentsCompilatrice & /*résultat*/)
{
    int taille_max_nom_pour_aide = 0;
    POUR (descriptions_arguments) {
        auto nom = donne_nom_pour_aide(it);
        auto const taille = calcule_taille_utf8(nom);
        if (taille > taille_max_nom_pour_aide) {
            taille_max_nom_pour_aide = taille;
        }
    }

    std::cout << "Utilisation : kuri [options...] FICHIER\n";
    std::cout << "Options :\n";

    POUR (descriptions_arguments) {
        auto nom = donne_nom_pour_aide(it);
        std::cout << "\t" << nom;

        auto const taille_nom = calcule_taille_utf8(nom);
        auto const taille_restante = taille_max_nom_pour_aide - taille_nom;
        auto const taille_marge = taille_restante + 2;

        for (int i = 0; i < taille_marge; i++) {
            std::cout << ' ';
        }

        std::cout << it.description_pour_aide << ".\n";
    }

    return ActionParsageArgument::ARRÊTE_POUR_AIDE;
}

static ActionParsageArgument gère_argument_pour_métaprogrammes(
    ParseuseArguments & /*parseuse*/, ArgumentsCompilatrice & /*résultat*/)
{
    return ActionParsageArgument::DÉBUTE_LISTE_ARGUMENTS_MÉTAPROGRAMMES;
}

static ActionParsageArgument gère_argument_emets_fichiers_utilises(ParseuseArguments &parseuse,
                                                                   ArgumentsCompilatrice &résultat)
{
    auto arg = parseuse.donne_argument_suivant();
    if (!arg.has_value()) {
        std::cerr << "Argument manquant après --emets_fichiers_utilises\n";
        return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
    }
    résultat.chemin_fichier_utilises = arg.value();
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_tests(ParseuseArguments & /*parseuse*/,
                                                 ArgumentsCompilatrice &résultat)
{
    résultat.active_tests = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_profile_exécution(ParseuseArguments & /*parseuse*/,
                                                             ArgumentsCompilatrice &résultat)
{
    résultat.profile_metaprogrammes = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_débogue_exécution(ParseuseArguments & /*parseuse*/,
                                                             ArgumentsCompilatrice &résultat)
{
    résultat.debogue_execution = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_format_profile(ParseuseArguments &parseuse,
                                                          ArgumentsCompilatrice &résultat)
{
    auto arg = parseuse.donne_argument_suivant();
    if (!arg.has_value()) {
        std::cerr << "Argument manquant après --format_profile\n";
        return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
    }

    if (arg.value() == "défaut" || arg.value() == "gregg") {
        résultat.format_rapport_profilage = FormatRapportProfilage::BRENDAN_GREGG;
        return ActionParsageArgument::CONTINUE;
    }

    if (arg.value() == "échantillons_totaux") {
        résultat.format_rapport_profilage =
            FormatRapportProfilage::ECHANTILLONS_TOTAL_POUR_FONCTION;
        return ActionParsageArgument::CONTINUE;
    }

    std::cerr << "Type de format de profile \"" << arg.value() << "\" inconnu\n";
    return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
}

static std::optional<ArgumentsCompilatrice> parse_arguments(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Utilisation : " << argv[0] << " [options...] FICHIER\n";
        return {};
    }

    auto resultat = ArgumentsCompilatrice();
    auto arguments_pour_métaprogrammes = false;

    auto parseuse_arguments = ParseuseArguments(argc, argv, 1);

    while (true) {
        auto arg = parseuse_arguments.donne_argument_suivant();
        if (!arg.has_value()) {
            break;
        }

        if (arguments_pour_métaprogrammes) {
            resultat.arguments_pour_métaprogrammes.ajoute(arg.value());
            continue;
        }

        auto desc = donne_description_pour_arg(arg.value());
        if (!desc.has_value()) {
            if (parseuse_arguments.a_consommé_tous_les_arguments()) {
                /* C'est peut-être le fichier, ce cas est géré en dehors de cet fonction. */
                return resultat;
            }

            std::cerr << "Argument '" << arg.value() << "' inconnu. Arrêt de la compilation.\n";
            return {};
        }

        if (!desc->fonction) {
            std::cerr << "Erreur interne : l'argument '" << arg.value()
                      << "' ne peut être géré. Arrêt de la compilation.\n";
            return {};
        }

        auto action = desc->fonction(parseuse_arguments, resultat);
        switch (action) {
            case ActionParsageArgument::CONTINUE:
            {
                break;
            }
            case ActionParsageArgument::ARRÊTE_POUR_AIDE:
            {
                /* L'aide a été imprimée. */
                exit(0);
            }
            case ActionParsageArgument::ARRÊTE_CAR_ERREUR:
            {
                return {};
            }
            case ActionParsageArgument::DÉBUTE_LISTE_ARGUMENTS_MÉTAPROGRAMMES:
            {
                arguments_pour_métaprogrammes = true;
                break;
            }
        }
    }

    return resultat;
}

/** \} */

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

    auto gestionnaire = compilatrice.gestionnaire_code;
    gestionnaire->démarre_boucle_compilation();

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

/* Tente de déterminer la racine depuis le système. */
static std::optional<kuri::chaine> détermine_chemin_exécutable()
{
    /* Tente de déterminer la racine depuis le système. */
    char tampon[1024];
    ssize_t len = readlink("/proc/self/exe", tampon, 1024);
    if (len < 0) {
        return {};
    }
    return kuri::chaine(&tampon[0], len);
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
    auto opt_chemin_executable = détermine_chemin_exécutable();
    if (!opt_chemin_executable.has_value()) {
        std::cerr
            << "Impossible de déterminer la racine d'exécution de Kuri depuis le système !\n";
        std::cerr << "Compilation avortée.\n";
        return {};
    }

    /* Ici nous avons le chemin complet vers l'exécutable, pour la racine il nous faut le chemin
     * parent. */
    auto chemin_executable = opt_chemin_executable.value();
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

    auto const chemin_fichier = argv[argc - 1];
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
