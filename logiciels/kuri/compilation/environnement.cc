/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "environnement.hh"

#include <set>

#include "parsage/modules.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "bibliotheque.hh"
#include "coulisse.hh"  // Pour nom_sortie_résultat_final.
#include "options.hh"
#include "utilitaires/log.hh"

#ifndef _MSC_VER
/* Pour Linux, nous préfixons avec "lib", sauf si nous avons un chemin. */
static kuri::chaine_statique préfixe_lib_pour_linux(kuri::chaine_statique nom_base,
                                                    bool utilise_préfixe)
{
    if (!utilise_préfixe) {
        return "";
    }

    for (int i = 0; i < nom_base.taille(); i++) {
        if (nom_base.pointeur()[i] == '/') {
            return "";
        }
    }

    return "lib";
}
#endif

kuri::chaine nom_fichier_objet_pour(kuri::chaine_statique nom_base)
{
#ifdef _MSC_VER
    return enchaine(nom_base, ".obj");
#else
    return enchaine(nom_base, ".o");
#endif
}

kuri::chemin_systeme chemin_fichier_objet_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_fichier_objet_pour(nom_base));
}

kuri::chaine nom_bibliothèque_dynamique_pour(kuri::chaine_statique nom_base, bool utilise_préfixe)
{
#ifdef _MSC_VER
    return enchaine(nom_base, ".dll");
#else
    auto préfixe = préfixe_lib_pour_linux(nom_base, utilise_préfixe);
    return enchaine(préfixe, nom_base, ".so");
#endif
}

kuri::chemin_systeme chemin_bibliothèque_dynamique_temporaire_pour(kuri::chaine_statique nom_base,
                                                                   bool utilise_préfixe)
{
    return kuri::chemin_systeme::chemin_temporaire(
        nom_bibliothèque_dynamique_pour(nom_base, utilise_préfixe));
}

kuri::chaine nom_bibliothèque_statique_pour(kuri::chaine_statique nom_base, bool utilise_préfixe)
{
#ifdef _MSC_VER
    return enchaine(nom_base, ".lib");
#else
    auto préfixe = préfixe_lib_pour_linux(nom_base, utilise_préfixe);
    return enchaine(préfixe, nom_base, ".a");
#endif
}

kuri::chemin_systeme chemin_bibliothèque_statique_temporaire_pour(kuri::chaine_statique nom_base,
                                                                  bool utilise_préfixe)
{
    return kuri::chemin_systeme::chemin_temporaire(
        nom_bibliothèque_statique_pour(nom_base, utilise_préfixe));
}

kuri::chaine nom_executable_pour(kuri::chaine_statique nom_base)
{
#ifdef _MSC_VER
    if (nom_base == "") {
        /* Utilise "a.exe", en référence au "a.out" de Unix.
         * À FAIRE : utilise du nom du fichier principal. */
        return "a.exe";
    }

    auto chemin = kuri::chemin_systeme(nom_base);
    /* Garantis que le nom de fichier possède l'extension ".exe". */
    chemin = chemin.remplace_extension(".exe");
    return kuri::chaine(chemin);
#else
    if (nom_base == "") {
        /* Utilise "a.out" par convention. */
        return "a.out";
    }
    return nom_base;
#endif
}

kuri::chemin_systeme chemin_executable_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_executable_pour(nom_base));
}

kuri::chemin_systeme suffixe_chemin_module_pour_bibliothèque(ArchitectureCible architecture_cible)
{
#ifdef _MSC_VER
    const kuri::chaine_statique suffixes[2] = {
        "lib/i386-windows",
        "lib/x86_64-windows",
    };
#else
    const kuri::chaine_statique suffixes[2] = {
        "lib/i386-linux-gnu",
        "lib/x86_64-linux-gnu",
    };
#endif

    return suffixes[static_cast<int>(architecture_cible)];
}

kuri::chemin_systeme chemin_de_base_pour_bibliothèque_r16(ArchitectureCible architecture_cible)
{
    const auto suffixe = suffixe_chemin_module_pour_bibliothèque(architecture_cible);
    return kuri::chemin_systeme::chemin_temporaire(suffixe);
}

kuri::chemin_systeme chemin_fichier_objet_r16(ArchitectureCible architecture_cible)
{
    const kuri::chaine_statique noms_de_base_fichiers[2] = {"r16_tables_x86", "r16_tables_x64"};
    const auto fichier_objet = nom_fichier_objet_pour(
        noms_de_base_fichiers[int(architecture_cible)]);
    return kuri::chemin_systeme::chemin_temporaire(fichier_objet);
}

static kuri::chaine_statique chaine_pour_niveau_optimisation(NiveauOptimisation niveau)
{
    switch (niveau) {
        case NiveauOptimisation::AUCUN:
        case NiveauOptimisation::O0:
        {
            return "-O0 ";
        }
        case NiveauOptimisation::O1:
        {
            return "-O1 ";
        }
        case NiveauOptimisation::O2:
        {
            return "-O2 ";
        }
        case NiveauOptimisation::Os:
        {
            return "-Os ";
        }
        /* Oz est spécifique à LLVM, prend O3 car c'est le plus élevé le
         * plus proche. */
        case NiveauOptimisation::Oz:
        case NiveauOptimisation::O3:
        {
            return "-O3 ";
        }
    }

    return "";
}

using TableauOptions = kuri::tablet<kuri::chaine_statique, 16>;

static void ajoute_options_pour_niveau_options(TableauOptions &résultat,
                                               OptionsDeCompilation const &options)
{
    switch (options.compilation_pour) {
        case CompilationPour::PRODUCTION:
        {
            résultat.ajoute(chaine_pour_niveau_optimisation(options.niveau_optimisation));
            break;
        }
        case CompilationPour::DÉBOGAGE:
        {
            résultat.ajoute("-g");
            résultat.ajoute("-Og");

            if (options.utilise_asan) {
                résultat.ajoute("-fsanitize=address");
            }

            break;
        }
        case CompilationPour::PROFILAGE:
        {
            résultat.ajoute("-pg");
            break;
        }
    }
}

static kuri::chaine_statique donne_compilateur_c()
{
#ifdef _MSC_VER
    return "cl";
#else
    return COMPILATEUR_C_COULISSE_C;
#endif
}

static kuri::chaine_statique donne_compilateur_cpp()
{
#ifdef _MSC_VER
    return "cl";
#else
    return COMPILATEUR_CXX_COULISSE_C;
#endif
}

static kuri::chaine_statique donne_lieur()
{
#ifdef _MSC_VER
    return "link";
#else
    return COMPILATEUR_CXX_COULISSE_C;
#endif
}

/* Pour les options d'avertissements et d'erreurs de GCC, voir :
 * https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html */
static TableauOptions options_pour_fichier_objet(kuri::chaine_statique compilateur,
                                                 OptionsDeCompilation const &options)
{
    TableauOptions résultat;

#ifdef _MSC_VER
    résultat.ajoute("/c");
#else
    résultat.ajoute("-c");

    if (options.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE ||
        options.code_indépendent_de_position) {
        /* Un fichier objet pour une bibliothèque dynamique doit compiler du code indépendant de la
         * position. */
        résultat.ajoute("-fPIC");
    }

    ajoute_options_pour_niveau_options(résultat, options);

    /* Désactivation des erreurs concernant le manque de "const" quand
     * on passe des variables générés temporairement par la coulisse à
     * des fonctions qui dont les paramètres ne sont pas constants. */
    if (compilateur == donne_compilateur_c()) {
        résultat.ajoute("-Wno-discarded-qualifiers");
    }
    /* Désactivation des avertissements de passage d'une variable au
     * lieu d'une chaine littérale à printf et al. */
    résultat.ajoute("-Wno-format-security");

    /* Les unions peuvent être perçues comme non-initialisées malgré l'assignation via un pointeur.
     */
    résultat.ajoute("-Wno-error=uninitialized");
    résultat.ajoute("-Wmissing-declarations");

    /* Arrête après une seule erreur. */
    résultat.ajoute("-Wfatal-errors");

    /* Nous devons gérer ce cas nous-même. C'est alors une erreur si la coulisse C se retrouve avec
     * un paramètre inutilisé. */
    résultat.ajoute("-Werror=unused-parameter");

    résultat.ajoute("-Wno-error=unused-but-set-variable");
    /* Peut arriver pour char*. */
    résultat.ajoute("-Wno-error=pointer-sign");
    /* Peut arriver dans les fonctions d'initialisation. */
    résultat.ajoute("-Wno-error=address-of-packed-member");
    /* Peut arriver pour l'assignation des unions. */
    résultat.ajoute("-Wno-error=strict-aliasing");
    /* Peut arriver pour les valeurs de retours si la fonction ne retourne jamais. */
    résultat.ajoute("-Wno-error=unused-variable");
    /* Peut arriver si une variable déclarée avec --- est conditionnellement assignée (mais le code
     * est toujours correcte) (cf. parse_ipv6). */
    résultat.ajoute("-Wno-error=maybe-uninitialized");

    résultat.ajoute("-Wall");
    résultat.ajoute("-Wpedantic");
    résultat.ajoute("-Wextra");
    résultat.ajoute("-Winit-self");
    résultat.ajoute("-Werror");

    /* Évite de faire du travail inutile. */
    résultat.ajoute("-Wno-misleading-indentation");

    if (!options.protège_pile) {
        résultat.ajoute("-fno-stack-protector");
    }

    if (options.architecture == ArchitectureCible::X86) {
        résultat.ajoute("-m32");
    }
#endif

    return résultat;
}

static TableauOptions options_pour_liaison(kuri::chaine_statique compilateur,
                                           OptionsDeCompilation const &options)
{
    TableauOptions résultat;

#ifndef _MSC_VER
    if (options.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE) {
        résultat.ajoute("-shared");
        résultat.ajoute("-fPIC");
    }

    ajoute_options_pour_niveau_options(résultat, options);

    /* Désactivation des erreurs concernant le manque de "const" quand
     * on passe des variables générés temporairement par la coulisse à
     * des fonctions qui dont les paramètres ne sont pas constants. */
    if (compilateur == donne_compilateur_c()) {
        résultat.ajoute("-Wno-discarded-qualifiers");
    }
    /* Désactivation des avertissements de passage d'une variable au
     * lieu d'une chaine littérale à printf et al. */
    résultat.ajoute("-Wno-format-security");

    if (!options.protège_pile) {
        résultat.ajoute("-fno-stack-protector");
    }

    if (options.architecture == ArchitectureCible::X86) {
        résultat.ajoute("-m32");
    }
#endif

    return résultat;
}

static kuri::chaine commande_pour_fichier_objet_impl(OptionsDeCompilation const &options,
                                                     kuri::chaine_statique compilateur,
                                                     kuri::chaine_statique fichier_entrée,
                                                     kuri::chaine_statique fichier_sortie)
{
    auto options_compilateur = options_pour_fichier_objet(compilateur, options);

    Enchaineuse enchaineuse;
    enchaineuse << compilateur << " ";

    POUR (options_compilateur) {
        enchaineuse << it << " ";
    }

#ifdef _MSC_VER
    /* Utilisation de C11 pour _Alignas, _Thread_Local, etc. */
    enchaineuse << "/std:c11 ";
    /* Chutis l'avertissement sur const manquant pour les assignements. */
    enchaineuse << "/wd4090 ";
    /* NOTE : le nom de sortie doit être collé à "/Fo" */
    enchaineuse << "\"" << fichier_entrée << "\"" << " /Fo" << '"' << fichier_sortie << '"';
#else
    enchaineuse << "\"" << fichier_entrée << "\" -o \"" << fichier_sortie << "\"";
#endif

    /* Terminateur nul afin de pouvoir passer la commande à #system. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

kuri::chaine commande_pour_fichier_objet(OptionsDeCompilation const &options,
                                         kuri::chaine_statique fichier_entrée,
                                         kuri::chaine_statique fichier_sortie)
{
    return commande_pour_fichier_objet_impl(
        options, donne_compilateur_c(), fichier_entrée, fichier_sortie);
}

static TypeLiaison donne_type_liaison_pour_bibliothèque(
    OptionsDeCompilation const &options,
    BibliothèquesUtilisées const &bibliothèques,
    Bibliothèque const &bibliohthèque)
{
    if (options.type_liaison == TypeLiaison::STATIQUE &&
        bibliothèques.peut_lier_statiquement(&bibliohthèque)) {
        return TypeLiaison::STATIQUE;
    }

    return TypeLiaison::DYNAMIQUE;
}

static kuri::chaine_statique donne_commande_pour_type_liaison(TypeLiaison const type_liaison)
{
    if (type_liaison == TypeLiaison::STATIQUE) {
        return "-Wl,-Bstatic";
    }
    return "-Wl,-Bdynamic";
}

kuri::chaine commande_pour_liaison(OptionsDeCompilation const &options,
                                   kuri::tableau_statique<kuri::chaine_statique> fichiers_entrée,
                                   BibliothèquesUtilisées const &bibliothèques)
{
    auto compilateur = donne_lieur();
    auto options_compilateur = options_pour_liaison(compilateur, options);

    Enchaineuse enchaineuse;
    enchaineuse << compilateur << " ";

#ifdef _MSC_VER
    enchaineuse << "/machine:X64 ";

    POUR (fichiers_entrée) {
        enchaineuse << "\"" << it << "\" ";
    }

    enchaineuse << chemin_fichier_objet_r16(options.architecture) << " ";

    enchaineuse << " /OUT:\"" << nom_sortie_résultat_final(options) << "\" ";

    POUR (bibliothèques.donne_tableau()) {
        if (it->nom == "r16") {
            continue;
        }

        auto chemin_parent = it->chemin_de_base(options);
        if (chemin_parent.taille() != 0) {
            enchaineuse << "/LIBPATH:\"" << chemin_parent << "\" ";
        }

        auto const liaison = donne_type_liaison_pour_bibliothèque(options, bibliothèques, *it);
        enchaineuse << it->nom;

        if (liaison == TypeLiaison::STATIQUE) {
            enchaineuse << ".lib ";
        }
        else {
            enchaineuse << ".dll ";
        }
    }
#else
    POUR (options_compilateur) {
        enchaineuse << it << " ";
    }

    if (options.coulisse == TypeCoulisse::ASM) {
        enchaineuse << "-no-pie" << " ";
    }

    POUR (fichiers_entrée) {
        enchaineuse << "\"" << it << "\" ";
    }

    /* Ajoute le fichier objet pour les r16. */
    enchaineuse << chemin_fichier_objet_r16(options.architecture) << " ";

    /* Détermine d'abord les chemins pour rpath. Il est possible que deux bibliothèques du même
     * dossier soient liées différement. Sans ça, si la statique précèdait la dynamique, rpath ne
     * serait pas mis en place dans la boucle suivante. */
    auto chemin_requiers_rpath = std::set<kuri::chemin_systeme>();
    POUR (bibliothèques.donne_tableau()) {
        if (it->nom == "r16") {
            continue;
        }

        auto chemin_parent = it->chemin_de_base(options);
        if (chemin_parent.taille() == 0) {
            continue;
        }

        if (it->chemin_dynamique(options)) {
            chemin_requiers_rpath.insert(chemin_parent);
        }
    }

    auto chemins_utilisés = std::set<kuri::chemin_systeme>();
    POUR (bibliothèques.donne_tableau()) {
        if (it->nom == "r16") {
            continue;
        }

        auto chemin_parent = it->chemin_de_base(options);
        if (chemin_parent.taille() == 0) {
            continue;
        }

        if (chemins_utilisés.find(chemin_parent) != chemins_utilisés.end()) {
            continue;
        }

        if (chemin_requiers_rpath.find(chemin_parent) != chemin_requiers_rpath.end()) {
            enchaineuse << " -Wl,-rpath=" << chemin_parent;
        }

        enchaineuse << " -L" << chemin_parent;
        chemins_utilisés.insert(chemin_parent);
    }

    POUR (bibliothèques.donne_tableau()) {
        if (it->nom == "r16") {
            continue;
        }

        auto const liaison = donne_type_liaison_pour_bibliothèque(options, bibliothèques, *it);
        enchaineuse << " " << donne_commande_pour_type_liaison(liaison);
        enchaineuse << " -l" << it->nom_pour_liaison(options);
    }

    /* Ajout d'une liaison dynamique pour dire à ld de chercher les symboles des bibliothèques
     * propres à GCC dans des bibliothèques dynamiques (car aucune version statique n'existe).
     */
    enchaineuse << " -Wl,-Bdynamic";

    enchaineuse << " -o " << nom_sortie_résultat_final(options);
#endif
    /* Terminateur nul afin de pouvoir passer la commande à #system. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* Crée une commande système pour appeler le compilateur natif afin de créer un fichier objet. */
static kuri::chaine commande_pour_fichier_objet_r16(OptionsDeCompilation const &options,
                                                    kuri::chaine_statique nom_entrée,
                                                    kuri::chaine_statique nom_sortie)
{
    return commande_pour_fichier_objet_impl(
        options, donne_compilateur_cpp(), nom_entrée, nom_sortie);
}

/* Crée une commande système pour appeler le compilateur natif afin de créer une bibliothèque
 * dynamique. */
static kuri::chaine commande_pour_bibliothèque_dynamique(kuri::chaine_statique nom_entrée,
                                                         kuri::chaine_statique nom_sortie,
                                                         ArchitectureCible architecture_cible)
{
    Enchaineuse enchaineuse;
    enchaineuse << donne_compilateur_cpp();

#ifdef _MSC_VER
    enchaineuse << " /D_USRDLL /D_WINDLL " << "\"" << nom_entrée << "\""
                << " /link /DLL /OUT:" << nom_sortie;
#else
    enchaineuse << " -shared -fPIC ";

    if (architecture_cible == ArchitectureCible::X86) {
        enchaineuse << " -m32 ";
    }

    enchaineuse << nom_entrée;
    enchaineuse << " -o ";
    enchaineuse << nom_sortie;
#endif

    /* Nous devons construire une chaine C, donc ajoutons un terminateur nul. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

static bool exécute_commande(kuri::chaine_statique commande)
{
    info() << "Compilation des tables de conversion R16...";

    if (!exécute_commande_externe(commande)) {
        dbg() << "Impossible de compiler les tables de conversion R16 !";
        return false;
    }

    return true;
}

/* À FAIRE(r16) : il faudra proprement gérer les architectures pour les r16, ou trouver des
 * algorithmes pour supprimer les tables */
bool precompile_objet_r16(const kuri::chemin_systeme &chemin_racine_kuri)
{
    /* Objet pour la liaison statique de la bibliothèque. */
    if (!compile_objet_r16(chemin_racine_kuri, ArchitectureCible::X64)) {
        return false;
    }

    /* Objet pour la liaison statique de la bibliothèque. */

    const auto fichier_objet = nom_bibliothèque_dynamique_pour("r16", true);
    const auto chemin_objet = chemin_de_base_pour_bibliothèque_r16(ArchitectureCible::X64) /
                              fichier_objet;

    if (kuri::chemin_systeme::existe(chemin_objet)) {
        return true;
    }

    const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
    /* assure l'existence des dossiers parents */
    kuri::chemin_systeme::crée_dossiers(chemin_objet.chemin_parent());

    const auto commande = commande_pour_bibliothèque_dynamique(
        chemin_fichier, chemin_objet, ArchitectureCible::X64);

    if (!exécute_commande(commande)) {
        return false;
    }

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        dbg() << "Le fichier compilé « " << chemin_objet << " » n'existe pas !";
        return false;
    }

    return true;
}

bool compile_objet_r16(const kuri::chemin_systeme &chemin_racine_kuri,
                       ArchitectureCible architecture_cible)
{
    const auto chemin_objet = chemin_fichier_objet_r16(architecture_cible);

    if (kuri::chemin_systeme::existe(chemin_objet)) {
        return true;
    }

    const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";

    OptionsDeCompilation options;
    options.architecture = architecture_cible;
    options.résultat = RésultatCompilation::FICHIER_OBJET;
    options.code_indépendent_de_position = true;

    const auto commande = commande_pour_fichier_objet_r16(options, chemin_fichier, chemin_objet);

    if (!exécute_commande(commande)) {
        return false;
    }

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        dbg() << "Le fichier compilé « " << chemin_objet << " » n'existe pas !";
        return false;
    }

    return true;
}

kuri::chaine donne_contenu_fichier_erreur(kuri::chaine_statique chemin)
{
    /* Lis le fichier d'erreur. */
    auto texte = charge_contenu_fichier(chemin);
    return kuri::chaine(&texte[0], texte.taille());
}

bool exécute_commande_externe_erreur(kuri::chaine_statique commande,
                                     kuri::chaine_statique chemin_fichier_erreur,
                                     bool verbeux)
{
    assert(commande.taille() != 0 && commande.pointeur()[commande.taille() - 1] == '\0');

    /* N'imprime pas le caractère nul. */
    auto commande_sans_caractère_nul = commande.sous_chaine(0, commande.taille() - 1);

    if (verbeux) {
        info() << "Exécution de la commande '" << commande_sans_caractère_nul << "'...";
    }

    auto nouvelle_commande = enchaine(
        commande_sans_caractère_nul, " 2> \"", chemin_fichier_erreur, "\"", '\0');

    auto const err = system(nouvelle_commande.pointeur());
    if (err == 0) {
        static_cast<void>(kuri::chemin_systeme::supprime(chemin_fichier_erreur));
        /* Succès. */
        return true;
    }

    return false;
}

std::optional<ErreurCommandeExterne> exécute_commande_externe_erreur(
    kuri::chaine_statique commande, bool verbeux)
{
    auto chemin_fichier_erreur = kuri::chemin_systeme::chemin_temporaire("erreur_commande.txt");
    std::optional<ErreurCommandeExterne> résultat;

    if (!exécute_commande_externe_erreur(commande, chemin_fichier_erreur, verbeux)) {
        résultat = ErreurCommandeExterne{donne_contenu_fichier_erreur(chemin_fichier_erreur)};
        static_cast<void>(kuri::chemin_systeme::supprime(chemin_fichier_erreur));
    }

    return résultat;
}

bool exécute_commande_externe(kuri::chaine_statique commande)
{
    assert(commande.taille() != 0 && commande.pointeur()[commande.taille() - 1] == '\0');
    /* N'imprime pas le caractère nul. */
    info() << "Exécution de la commande '" << commande.sous_chaine(0, commande.taille() - 1)
           << "'...";

    const auto err = system(commande.pointeur());
    if (err != 0) {
        return false;
    }
    return true;
}
