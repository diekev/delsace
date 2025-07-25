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
#include "utilitaires/log.hh"

#include "statistiques/statistiques.hh"

#include "structures/chemin_systeme.hh"

#include "utilitaires/chrono.hh"
#include "utilitaires/unicode.hh"

#ifdef _MSC_VER
#    include <windows.h>
#endif

#if 0
__declspec(noinline) size_t available_stack_size()
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery(&mbi, &mbi, sizeof(mbi));
    return uintptr_t(&mbi) - uintptr_t(mbi.AllocationBase);
}
#endif

/**
 * Fonction de rappel pour les fils d'exécutions.
 */
static void lance_tacheronne(Tacheronne *tacheronne)
{
    while (true) {
        if (tacheronne->gère_tâche()) {
            break;
        }
    }
}

#if 0
static void valide_blocs_modules(EspaceDeTravail const &espace)
{
    POUR_TABLEAU_PAGE (espace.graphe_dépendance->noeuds) {
		if (it.type != TypeNoeudDependance::FONCTION) {
			continue;
		}

		auto noeud = it.noeud_syntaxique;

		auto fichier = espace.fichier(noeud->lexème->fichier);
		auto module = fichier->module;

		auto bloc = noeud->bloc_parent;

		while (bloc->bloc_parent) {
			bloc = bloc->bloc_parent;
		}

		if (module->bloc != bloc) {
            dbg() << "Une fonction n'est pas le bon bloc parent !";
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
    if (!compilatrice.espace_de_travail_defaut->options.émets_métriques) {
        return;
    }

    compilatrice.rassemble_statistiques(stats);

    POUR (tacheronnes) {
        it->rassemble_statistiques(stats);
    }

    stats.ajoute_mémoire_utilisée("RI", stats.stats_ri.totaux.mémoire);
}

static void imprime_stats(Compilatrice const &compilatrice,
                          Statistiques const &stats,
                          kuri::chrono::compte_seconde debut_compilation)
{
    if (!compilatrice.espace_de_travail_defaut->options.émets_métriques) {
        return;
    }

    imprime_stats(stats, debut_compilation);
    compilatrice.gestionnaire_code->imprime_stats();

    if (compilatrice.arguments.stats_détaillées) {
        imprime_stats_détaillées(stats);
    }
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
    kuri::chaine_statique nom = "";
    kuri::chaine_statique nom_court = "";
    kuri::chaine_statique nom_pour_aide = "";
    kuri::chaine_statique description_pour_aide = "";
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

static ActionParsageArgument gère_argument_émets_stats_ops(ParseuseArguments & /*parseuse*/,
                                                           ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_préserve_symbole(ParseuseArguments & /*parseuse*/,
                                                            ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_format_profile(ParseuseArguments &parseuse,
                                                          ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_sans_stats(ParseuseArguments &parseuse,
                                                      ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_sans_traces_d_appel(ParseuseArguments &parseuse,
                                                               ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_coulisse(ParseuseArguments &parseuse,
                                                    ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_stats_détaillées(ParseuseArguments & /*parseuse*/,
                                                            ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_émission_ri(ParseuseArguments & /*parseuse*/,
                                                       ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_émission_code_binaire(ParseuseArguments & /*parseuse*/,
                                                                 ArgumentsCompilatrice &résultat);

static ActionParsageArgument gère_argument_mode_parallèle(ParseuseArguments &parseuse,
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
    {"--stats_ops_exécution",
     "",
     "",
     "Rapporte le nombre de fois que chaque instruction fut exécutée dans les métaprogrammes",
     gère_argument_émets_stats_ops},
    {"--préserve_symboles",
     "",
     "",
     "Indique aux coulisses de préserver les symboles non-globaux",
     gère_argument_préserve_symbole},
    {"--sans_stats",
     "",
     "",
     "N'imprime pas les statistiques à la fin de la compilation",
     gère_argument_sans_stats},
    {"--stats_détaillées",
     "",
     "",
     "Imprime les détails des statistiques",
     gère_argument_stats_détaillées},
    {"--sans_traces_d_appel",
     "",
     "",
     "Ne génère pas de traces d'appel",
     gère_argument_sans_traces_d_appel},
    {"--coulisse",
     "",
     "--coulisse {c|asm|llvm}",
     "Détermine quelle coulisse utilisée pour l'espace par défaut de compilation",
     gère_argument_coulisse},
    {"--émets_ri",
     "",
     "",
     "Imprime la Représentation Intermédiaire des programmes des espaces dans un fichier .ri. Le "
     "nom du fichier dérive du nom de l'espace, avec les espaces remplacées par des tirets-bas. "
     "Par exemple, si un espace s'appelle « Espace 1 », le fichier s'appelera « Espace_1.ri »",
     gère_argument_émission_ri},
    {"--émets_code_binaire",
     "",
     "",
     "Imprime le code binaire, sous forme textuelle, de chaque métaprogramme exécuté. Le code "
     "binaire est émis avant l'exécution desdits métaprogrammes. Le fichier texte est émis comme "
     "un log standard de métaprogramme, dont la racine du nom est « code_binaire.txt ».",
     gère_argument_émission_code_binaire},
    {"", "-j", "-j", "Active la compilation en mode parallèle", gère_argument_mode_parallèle},
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
        auto n = unicode::nombre_octets(&chaine.pointeur()[i]);
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

    Enchaineuse sortie;

    sortie << "Utilisation : kuri [options...] FICHIER\n";
    sortie << "Options :\n";

    POUR (descriptions_arguments) {
        auto nom = donne_nom_pour_aide(it);
        sortie << "\t" << nom;

        auto const taille_nom = calcule_taille_utf8(nom);
        auto const taille_restante = taille_max_nom_pour_aide - taille_nom;
        auto const taille_marge = taille_restante + 2;

        for (int i = 0; i < taille_marge; i++) {
            sortie << ' ';
        }

        sortie << it.description_pour_aide << ".\n";
    }

    info() << sortie.chaine();

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
        dbg() << "Argument manquant après --emets_fichiers_utilises.";
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

static ActionParsageArgument gère_argument_émets_stats_ops(ParseuseArguments & /*parseuse*/,
                                                           ArgumentsCompilatrice &résultat)
{
    résultat.émets_stats_ops_exécution = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_préserve_symbole(ParseuseArguments & /*parseuse*/,
                                                            ArgumentsCompilatrice &résultat)
{
    résultat.préserve_symboles = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_format_profile(ParseuseArguments &parseuse,
                                                          ArgumentsCompilatrice &résultat)
{
    auto arg = parseuse.donne_argument_suivant();
    if (!arg.has_value()) {
        dbg() << "Argument manquant après --format_profile.";
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

    dbg() << "Type de format de profile \"" << arg.value() << "\" inconnu.";
    return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
}

static ActionParsageArgument gère_argument_sans_stats(ParseuseArguments & /*parseuse*/,
                                                      ArgumentsCompilatrice &résultat)
{
    résultat.sans_stats = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_stats_détaillées(ParseuseArguments & /*parseuse*/,
                                                            ArgumentsCompilatrice &résultat)
{
    résultat.stats_détaillées = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_sans_traces_d_appel(ParseuseArguments & /*parseuse*/,
                                                               ArgumentsCompilatrice &résultat)
{
    résultat.sans_traces_d_appel = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_émission_ri(ParseuseArguments & /*parseuse*/,
                                                       ArgumentsCompilatrice &résultat)
{
    résultat.émets_ri = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_émission_code_binaire(ParseuseArguments & /*parseuse*/,
                                                                 ArgumentsCompilatrice &résultat)
{
    résultat.émets_code_binaire = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_mode_parallèle(ParseuseArguments & /*parseuse*/,
                                                          ArgumentsCompilatrice &résultat)
{
    résultat.compile_en_mode_parallèle = true;
    return ActionParsageArgument::CONTINUE;
}

static ActionParsageArgument gère_argument_coulisse(ParseuseArguments &parseuse,
                                                    ArgumentsCompilatrice &résultat)
{
    auto arg = parseuse.donne_argument_suivant();
    if (!arg.has_value()) {
        dbg() << "Argument manquant après --coulisse.";
        return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
    }

    if (arg.value() == "c") {
        résultat.coulisse = TypeCoulisse::C;
        return ActionParsageArgument::CONTINUE;
    }

    if (arg.value() == "asm") {
        résultat.coulisse = TypeCoulisse::ASM;
        return ActionParsageArgument::CONTINUE;
    }

    if (arg.value() == "llvm") {
        résultat.coulisse = TypeCoulisse::LLVM;
        return ActionParsageArgument::CONTINUE;
    }

    dbg() << "Argument « " << arg.value() << " » inconnu après --coulisse.";
    return ActionParsageArgument::ARRÊTE_CAR_ERREUR;
}

static std::optional<ArgumentsCompilatrice> parse_arguments(int argc, char **argv)
{
    if (argc < 2) {
        dbg() << "Utilisation : " << argv[0] << " [options...] FICHIER.";
        return {};
    }

    auto résultat = ArgumentsCompilatrice();
    auto arguments_pour_métaprogrammes = false;

    auto parseuse_arguments = ParseuseArguments(argc, argv, 1);

    while (true) {
        auto arg = parseuse_arguments.donne_argument_suivant();
        if (!arg.has_value()) {
            break;
        }

        if (arguments_pour_métaprogrammes) {
            résultat.arguments_pour_métaprogrammes.ajoute(arg.value());
            continue;
        }

        auto desc = donne_description_pour_arg(arg.value());
        if (!desc.has_value()) {
            if (parseuse_arguments.a_consommé_tous_les_arguments()) {
                /* C'est peut-être le fichier, ce cas est géré en dehors de cet fonction. */
                return résultat;
            }

            dbg() << "Argument '" << arg.value() << "' inconnu. Arrêt de la compilation.";
            return {};
        }

        if (!desc->fonction) {
            dbg() << "Erreur interne : l'argument '" << arg.value()
                  << "' ne peut être géré. Arrêt de la compilation.";
            return {};
        }

        auto action = desc->fonction(parseuse_arguments, résultat);
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

    return résultat;
}

/** \} */

static bool compile_fichier(Compilatrice &compilatrice, kuri::chaine_statique chemin_fichier)
{
    auto debut_compilation = kuri::chrono::compte_seconde();

    /* Compile les objets pour le support des r16 afin d'avoir la bibliothèque r16. */
    if (!precompile_objet_r16(kuri::chaine_statique(compilatrice.racine_kuri))) {
        return false;
    }

    /* Initialise les bibliothèques après avoir généré les objets r16. */
    if (!GestionnaireBibliothèques::initialise_bibliothèques_pour_exécution(compilatrice)) {
        return false;
    }

    /* Crée les tâches pour les données requise de la typeuse. */
    Typeuse::crée_tâches_précompilation(compilatrice);

    /* enregistre le dossier d'origine */
    auto dossier_origine = kuri::chemin_systeme::chemin_courant();

    auto chemin = kuri::chemin_systeme::absolu(chemin_fichier);

    /* Charge d'abord le module basique. */
    auto espace_defaut = compilatrice.espace_de_travail_defaut;
    espace_defaut->options.nom_sortie = chemin.nom_fichier_sans_extension();

    auto dossier = chemin.chemin_parent();
    kuri::chemin_systeme::change_chemin_courant(dossier);

    info() << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'...";

    auto module = compilatrice.sys_module->crée_module_fichier_racine_compilation(dossier, chemin);
    compilatrice.module_racine_compilation = module;
    compilatrice.gestionnaire_code->requiers_chargement(espace_defaut, module->fichiers[0]);

    auto nombre_tacheronnes = std::thread::hardware_concurrency();

    kuri::tableau<Tacheronne *> tacheronnes;
    tacheronnes.réserve(nombre_tacheronnes);

    for (auto i = 0u; i < nombre_tacheronnes; ++i) {
        tacheronnes.ajoute(mémoire::loge<Tacheronne>("Tacheronne", compilatrice));
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

    if (compilatrice.arguments.compile_en_mode_parallèle) {
        kuri::tableau<std::thread *> threads;
        threads.réserve(nombre_tacheronnes);

        POUR (tacheronnes) {
            threads.ajoute(mémoire::loge<std::thread>("std::thread", lance_tacheronne, it));
        }

        POUR (threads) {
            it->join();
            mémoire::deloge("std::thread", it);
        }
    }
    else {
        auto compilation_terminée = false;
        while (!compilation_terminée) {
            POUR (tacheronnes) {
                compilation_terminée |= it->gère_tâche();
            }
        }
    }

    if (compilatrice.chaines_ajoutées_à_la_compilation->nombre_de_chaines()) {
        auto fichier_chaines = std::ofstream(".chaines_ajoutées");
        compilatrice.chaines_ajoutées_à_la_compilation->imprime_dans(fichier_chaines);
    }

    /* restore le dossier d'origine */
    kuri::chemin_systeme::change_chemin_courant(dossier_origine);

    if (compilatrice.possède_erreur()) {
        return false;
    }

    imprime_fichiers_utilises(compilatrice);

    if (compilatrice.arguments.sans_stats == false) {
        auto stats = Statistiques();
        rassemble_statistiques(compilatrice, stats, tacheronnes);

        imprime_stats(compilatrice, stats, debut_compilation);
    }

    info() << "Nettoyage...";

    POUR (tacheronnes) {
        mémoire::deloge("Tacheronne", it);
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
#ifdef _MSC_VER
    std::wstring path(1024, L'\0');
    const DWORD len = GetModuleFileNameW(NULL, &path[0], (DWORD)path.size());
    if (!len) {
        return {};
    }
    path.resize(len);
    std::string string = kuri::vers_utf8(path);
    return kuri::chaine(string.c_str(), long(string.size()));
#else
    /* Tente de déterminer la racine depuis le système. */
    char tampon[1024];
    ssize_t len = readlink("/proc/self/exe", tampon, 1024);
    if (len < 0) {
        return {};
    }
    return kuri::chaine(&tampon[0], len);
#endif
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
        dbg() << "Impossible de déterminer la racine d'exécution de Kuri depuis le système !"
              << "Compilation avortée.";
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
        dbg() << "Impossible de déterminer la racine d'exécution de Kuri depuis l'environnement.";
        dbg() << "Veuillez vous assurer que RACINE_KURI fait partie de l'environnement "
                 "d'exécution et pointe vers une installation valide de Kuri.";
        dbg() << "Compilation avortée.";
        return {};
    }

    racine = kuri::chemin_systeme(racine_env);
    dossier_manquant = dossier_manquant_racine_execution(racine);

    if (dossier_manquant.has_value()) {
        dbg() << "Racine d'exécution de Kuri invalide !";
        dbg() << "Le dossier \"" << dossier_manquant.value() << "\" n'existe pas !";
        dbg() << "Veuillez vérifier que votre installation est correcte.";
        dbg() << "NOTE : le chemin racine utilisé provient de la variable d'environnement « "
                 "RACINE_KURI ».";
        dbg() << "Compilation avortée.";
        return {};
    }

    return kuri::chaine(racine);
}

int main(int argc, char *argv[])
{
    std::ios::sync_with_stdio(false);

#ifdef _MSC_VER
    SetConsoleOutputCP(CP_UTF8);
#endif

    auto const opt_racine_kuri = determine_racine_execution_kuri();
    if (!opt_racine_kuri.has_value()) {
        return 1;
    }

    auto const opt_arguments = parse_arguments(argc, argv);
    if (!opt_arguments.has_value()) {
        return 1;
    }

    auto chemin_fichier = kuri::chemin_systeme(argv[argc - 1]);
    auto extension = chemin_fichier.extension();
    if (extension != ".kuri") {
        if (extension == "") {
            /* Puisque le nom de fichier de sortie par défaut est le nom du fichier qui est
             * compilé, ajoute une extension si nous compilons le compilat.
             * Ceci permet de compiler en ligne de commande quand l'autocomplétion s'arrête avant
             * l'extension, mais que le développeur lança l'exécution de la commande avec de
             * compléter plus. */
            chemin_fichier = chemin_fichier.remplace_extension(".kuri");
        }
        else {
            dbg() << "Le fichier spécifié doit avoir l'extension '.kuri'";
            dbg() << "    L'extension du fichier est '" << extension << "'";
            return 1;
        }
    }

    if (!kuri::chemin_systeme::existe(chemin_fichier)) {
        dbg() << "Impossible d'ouvrir le fichier : " << chemin_fichier;
        return 1;
    }

    auto compilatrice = Compilatrice(opt_racine_kuri.value(), opt_arguments.value());

    if (!compile_fichier(compilatrice, chemin_fichier)) {
        return 1;
    }

#ifdef AVEC_COULISSE_LLVM
    issitialise_llvm();
#endif

    return static_cast<int>(compilatrice.code_erreur());
}
