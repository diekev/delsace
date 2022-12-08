/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "environnement.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "options.hh"

kuri::chaine nom_fichier_objet_pour(kuri::chaine_statique nom_base)
{
    return enchaine(nom_base, ".o");
}

kuri::chemin_systeme chemin_fichier_objet_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_fichier_objet_pour(nom_base));
}

kuri::chaine nom_bibliothèque_dynamique_pour(kuri::chaine_statique nom_base)
{
    return enchaine(nom_base, ".so");
}

kuri::chemin_systeme chemin_bibliothèque_dynamique_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_bibliothèque_dynamique_pour(nom_base));
}

kuri::chaine nom_bibliothèque_statique_pour(kuri::chaine_statique nom_base)
{
    return enchaine(nom_base, ".a");
}

kuri::chemin_systeme chemin_bibliothèque_statique_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_bibliothèque_statique_pour(nom_base));
}

kuri::chaine nom_executable_pour(kuri::chaine_statique nom_base)
{
    if (nom_base == "") {
        /* Utilise "a.out" par convention. */
        return "a.out";
    }
    return nom_base;
}

kuri::chemin_systeme chemin_executable_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_executable_pour(nom_base));
}

kuri::chemin_systeme suffixe_chemin_module_pour_bibliotheque(ArchitectureCible architecture_cible)
{
    const kuri::chaine_statique suffixes[2] = {
        "lib/i386-linux-gnu",
        "lib/x86_64-linux-gnu",
    };

    return suffixes[static_cast<int>(architecture_cible)];
}

kuri::chemin_systeme chemin_de_base_pour_bibliothèque_r16(ArchitectureCible architecture_cible)
{
    const auto suffixe = suffixe_chemin_module_pour_bibliotheque(architecture_cible);
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

static TableauOptions options_pour_fichier_objet(OptionsDeCompilation const &options)
{
    TableauOptions résultat;

    résultat.ajoute("-c");

    if (options.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        /* Un fichier objet pour une bibliothèque dynamique doit compiler du code indépendant de la
         * position. */
        résultat.ajoute("-fPIC");
    }

    switch (options.compilation_pour) {
        case CompilationPour::PRODUCTION:
        {
            résultat.ajoute(chaine_pour_niveau_optimisation(options.niveau_optimisation));
            break;
        }
        case CompilationPour::DEBOGAGE:
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

    /* Désactivation des erreurs concernant le manque de "const" quand
     * on passe des variables générés temporairement par la coulisse à
     * des fonctions qui dont les paramètres ne sont pas constants. */
    résultat.ajoute("-Wno-discarded-qualifiers");
    /* Désactivation des avertissements de passage d'une variable au
     * lieu d'une chaine littérale à printf et al. */
    résultat.ajoute("-Wno-format-security");

    if (!options.protege_pile) {
        résultat.ajoute("-fno-stack-protector");
    }

    if (options.architecture == ArchitectureCible::X86) {
        résultat.ajoute("-m32");
    }

    return résultat;
}

kuri::chaine commande_pour_fichier_objet(OptionsDeCompilation const &options,
                                         kuri::chaine_statique fichier_entrée,
                                         kuri::chaine_statique fichier_sortie)
{
    auto options_compilateur = options_pour_fichier_objet(options);

    Enchaineuse enchaineuse;
    enchaineuse << COMPILATEUR_C_COULISSE_C << " ";

    POUR (options_compilateur) {
        enchaineuse << it << " ";
    }

    enchaineuse << fichier_entrée << " -o " << fichier_sortie;

    /* Terminateur nul afin de pouvoir passer la commande à #system. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* Crée une commande système pour appeler le compilateur natif afin de créer un fichier objet. */
static kuri::chaine commande_pour_fichier_objet(kuri::chaine_statique nom_entree,
                                                kuri::chaine_statique nom_sortie,
                                                ArchitectureCible architecture_cible)
{
    Enchaineuse enchaineuse;
    enchaineuse << COMPILATEUR_CXX_COULISSE_C;
    enchaineuse << " -c -fPIC ";

    if (architecture_cible == ArchitectureCible::X86) {
        enchaineuse << " -m32 ";
    }

    enchaineuse << nom_entree;
    enchaineuse << " -o ";
    enchaineuse << nom_sortie;

    /* Nous devons construire une chaine C, donc ajoutons un terminateur nul. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* Crée une commande système pour appeler le compilateur natif afin de créer une bibliothèque
 * dynamique. */
static kuri::chaine commande_pour_bibliotheque_dynamique(kuri::chaine_statique nom_entree,
                                                         kuri::chaine_statique nom_sortie,
                                                         ArchitectureCible architecture_cible)
{
    Enchaineuse enchaineuse;
    enchaineuse << COMPILATEUR_CXX_COULISSE_C;
    enchaineuse << " -shared -fPIC ";

    if (architecture_cible == ArchitectureCible::X86) {
        enchaineuse << " -m32 ";
    }

    enchaineuse << nom_entree;
    enchaineuse << " -o ";
    enchaineuse << nom_sortie;

    /* Nous devons construire une chaine C, donc ajoutons un terminateur nul. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

static bool execute_commande(kuri::chaine const &commande)
{
    std::cout << "Compilation des tables de conversion R16...\n";
    std::cout << "Exécution de la commande " << commande << std::endl;

    const auto err = system(commande.pointeur());
    if (err != 0) {
        std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
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

    const auto fichier_objet = nom_bibliothèque_dynamique_pour("libr16");
    const auto chemin_objet = chemin_de_base_pour_bibliothèque_r16(ArchitectureCible::X64) /
                              fichier_objet;

    if (kuri::chemin_systeme::existe(chemin_objet)) {
        return true;
    }

    const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
    /* assure l'existence des dossiers parents */
    kuri::chemin_systeme::cree_dossiers(chemin_objet.chemin_parent());

    const auto commande = commande_pour_bibliotheque_dynamique(
        chemin_fichier, chemin_objet, ArchitectureCible::X64);

    if (!execute_commande(commande)) {
        return false;
    }

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        std::cerr << "Le fichier compilé « " << chemin_objet << " » n'existe pas !\n";
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

    const auto commande = commande_pour_fichier_objet(
        chemin_fichier, chemin_objet, architecture_cible);

    if (!execute_commande(commande)) {
        return false;
    }

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        std::cerr << "Le fichier compilé « " << chemin_objet << " » n'existe pas !\n";
        return false;
    }

    return true;
}
