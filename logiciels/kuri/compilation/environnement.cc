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

kuri::chemin_systeme chemin_fichier_objet_r16(ArchitectureCible architecture_cible)
{
    const kuri::chaine_statique noms_de_base_fichiers[2] = {"r16_tables_x86", "r16_tables_x64"};
    const auto fichier_objet = nom_fichier_objet_pour(
        noms_de_base_fichiers[int(architecture_cible)]);
    return kuri::chemin_systeme::chemin_temporaire(fichier_objet);
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

    /* A FAIRE : généralise les chemins. */
    const auto fichier_objet = nom_bibliothèque_dynamique_pour("lib/x86_64-linux-gnu/libr16");
    const auto chemin_objet = kuri::chemin_systeme::chemin_temporaire(fichier_objet);

    if (kuri::chemin_systeme::existe(chemin_objet)) {
        return true;
    }

    const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
    /* assure l'existence des dossiers parents */
    kuri::chemin_systeme::cree_dossiers(chemin_objet.chemin_parent());

    const auto commande = commande_pour_bibliotheque_dynamique(
        chemin_fichier, chemin_objet, ArchitectureCible::X64);

    return execute_commande(commande);
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

    return execute_commande(commande);
}
