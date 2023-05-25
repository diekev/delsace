/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "environnement.hh"

#include <set>

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "bibliotheque.hh"
#include "coulisse.hh"  // Pour nom_sortie_resultat_final.
#include "options.hh"

/* Pour Linux, nous préfixons avec "lib", sauf si nous avons un chemin. */
static kuri::chaine_statique préfixe_lib_pour_linux(kuri::chaine_statique nom_base)
{
    for (int i = 0; i < nom_base.taille(); i++) {
        if (nom_base.pointeur()[i] == '/') {
            return "";
        }
    }

    return "lib";
}

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
    auto préfixe = préfixe_lib_pour_linux(nom_base);
    return enchaine(préfixe, nom_base, ".so");
}

kuri::chemin_systeme chemin_bibliothèque_dynamique_temporaire_pour(kuri::chaine_statique nom_base)
{
    return kuri::chemin_systeme::chemin_temporaire(nom_bibliothèque_dynamique_pour(nom_base));
}

kuri::chaine nom_bibliothèque_statique_pour(kuri::chaine_statique nom_base)
{
    auto préfixe = préfixe_lib_pour_linux(nom_base);
    return enchaine(préfixe, nom_base, ".a");
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

static void ajoute_options_pour_niveau_options(TableauOptions &résultat,
                                               OptionsDeCompilation const &options)
{
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
}

static TableauOptions options_pour_fichier_objet(OptionsDeCompilation const &options)
{
    TableauOptions résultat;

    résultat.ajoute("-c");

    if (options.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        /* Un fichier objet pour une bibliothèque dynamique doit compiler du code indépendant de la
         * position. */
        résultat.ajoute("-fPIC");
    }

    ajoute_options_pour_niveau_options(résultat, options);

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

static TableauOptions options_pour_liaison(OptionsDeCompilation const &options)
{
    TableauOptions résultat;

    if (options.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        résultat.ajoute("-shared");
        résultat.ajoute("-fPIC");
    }

    ajoute_options_pour_niveau_options(résultat, options);

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

static kuri::chaine commande_pour_fichier_objet_impl(OptionsDeCompilation const &options,
                                                     kuri::chaine_statique compilateur,
                                                     kuri::chaine_statique fichier_entrée,
                                                     kuri::chaine_statique fichier_sortie)
{
    auto options_compilateur = options_pour_fichier_objet(options);

    Enchaineuse enchaineuse;
    enchaineuse << compilateur << " ";

    POUR (options_compilateur) {
        enchaineuse << it << " ";
    }

    enchaineuse << fichier_entrée << " -o " << fichier_sortie;

    /* Terminateur nul afin de pouvoir passer la commande à #system. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

kuri::chaine commande_pour_fichier_objet(OptionsDeCompilation const &options,
                                         kuri::chaine_statique fichier_entrée,
                                         kuri::chaine_statique fichier_sortie)
{
    return commande_pour_fichier_objet_impl(
        options, COMPILATEUR_C_COULISSE_C, fichier_entrée, fichier_sortie);
}

kuri::chaine commande_pour_liaison(OptionsDeCompilation const &options,
                                   kuri::tableau_statique<kuri::chaine_statique> fichiers_entrée,
                                   kuri::tableau_statique<Bibliotheque *> bibliotheques)
{
    auto options_compilateur = options_pour_liaison(options);

    Enchaineuse enchaineuse;
    enchaineuse << COMPILATEUR_CXX_COULISSE_C << " ";

    POUR (options_compilateur) {
        enchaineuse << it << " ";
    }

    POUR (fichiers_entrée) {
        enchaineuse << it << " ";
    }

    /* Ajoute le fichier objet pour les r16. */
    enchaineuse << chemin_fichier_objet_r16(options.architecture) << " ";

    auto chemins_utilises = std::set<kuri::chemin_systeme>();

    POUR (bibliotheques) {
        if (it->nom == "r16") {
            continue;
        }

        auto chemin_parent = it->chemin_de_base(options);
        if (chemin_parent.taille() == 0) {
            continue;
        }

        if (chemins_utilises.find(chemin_parent) != chemins_utilises.end()) {
            continue;
        }

        if (it->chemin_dynamique(options)) {
            enchaineuse << " -Wl,-rpath=" << chemin_parent;
        }

        enchaineuse << " -L" << chemin_parent;
        chemins_utilises.insert(chemin_parent);
    }

    /* À FAIRE(bibliothèques) : permet la liaison statique.
     * Les deux formes de commandes suivant résultent en des erreurs de liaison :
     * -Wl,-Bshared -llib1 -lib2 -Wl,-Bstatic -lc -llib3
     * (et une version où la liaison de chaque bibliothèque est spécifiée)
     * -Wl,-Bshared -llib1 -Wl,-Bshared -lib2 -Wl,-Bstatic -lc -Wl,-Bstatic -llib3
     */
    POUR (bibliotheques) {
        if (it->nom == "r16") {
            continue;
        }

        enchaineuse << " -l" << it->nom_pour_liaison(options);
    }

    /* Ajout d'une liaison dynamique pour dire à ld de chercher les symboles des bibliothèques
     * propres à GCC dans des bibliothèques dynamiques (car aucune version statique n'existe).
     */
    enchaineuse << " -Wl,-Bdynamic";

    enchaineuse << " -o " << nom_sortie_resultat_final(options);

    /* Terminateur nul afin de pouvoir passer la commande à #system. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* Crée une commande système pour appeler le compilateur natif afin de créer un fichier objet. */
static kuri::chaine commande_pour_fichier_objet_r16(OptionsDeCompilation const &options,
                                                    kuri::chaine_statique nom_entree,
                                                    kuri::chaine_statique nom_sortie)
{
    return commande_pour_fichier_objet_impl(
        options, COMPILATEUR_CXX_COULISSE_C, nom_entree, nom_sortie);
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

    const auto fichier_objet = nom_bibliothèque_dynamique_pour("r16");
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

    OptionsDeCompilation options;
    options.architecture = architecture_cible;
    /* À FAIRE : pour -fPIC... */
    options.resultat = ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE;

    const auto commande = commande_pour_fichier_objet_r16(options, chemin_fichier, chemin_objet);

    if (!execute_commande(commande)) {
        return false;
    }

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        std::cerr << "Le fichier compilé « " << chemin_objet << " » n'existe pas !\n";
        return false;
    }

    return true;
}
