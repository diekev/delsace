/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "environnement.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "options.hh"

static kuri::chaine chaine_echappee(kuri::chaine_statique chn)
{
    Enchaineuse enchaineuse;

    enchaineuse.pousse_caractere('"');
    for (auto i = 0; i < chn.taille(); i++) {
        auto it = chn.pointeur()[i];
        //        if (it == ' ') {
        //            enchaineuse.pousse_caractere('\\');
        //        }
        enchaineuse.pousse_caractere(it);
    }
    enchaineuse.pousse_caractere('"');

    return enchaineuse.chaine();
}

/* Retourne le nom suffixé de l'extension native pour un fichier objet. */
static kuri::chaine nom_fichier_objet_pour(kuri::chaine_statique nom_base)
{
#ifdef _MSC_VER
    return enchaine(nom_base, ".obj");
#else
    return enchaine(nom_base, ".o");
#endif
}

/* Retourne le nom suffixé de l'extension native pour une bibliothèque dynamique. */
static kuri::chaine nom_bibliothèque_dynamique_pour(kuri::chaine_statique nom_base)
{
#ifdef _MSC_VER
    return enchaine(nom_base, ".dll");
#else
    return enchaine(nom_base, ".so");
#endif
}

/* Crée une commande système pour appeler le compilateur natif afin de créer un fichier objet. */
static kuri::chaine commande_pour_fichier_objet(kuri::chaine_statique nom_entree,
                                                kuri::chaine_statique nom_sortie)
{
    Enchaineuse enchaineuse;
    // enchaineuse << chaine_echappee(COMPILATEUR_CXX_COULISSE_C);
    enchaineuse << "cl";

#ifdef _MSC_VER
    enchaineuse << " /c " << nom_sortie << " " << nom_entree;
#else
    enchaineuse << " -c  -fPIC ";
    enchaineuse << chemin_fichier;
    enchaineuse << " -o ";
    enchaineuse << chemin_objet;
#endif

    /* Nous devons construire une chaine C, donc ajoutons un terminateur nul. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* Crée une commande système pour appeler le compilateur natif afin de créer une bibliothèque
 * dynamique. */
static kuri::chaine commande_pour_bibliotheque_dynamique(kuri::chaine_statique nom_entree,
                                                         kuri::chaine_statique nom_sortie)
{
    Enchaineuse enchaineuse;
    //  enchaineuse << chaine_echappee(COMPILATEUR_CXX_COULISSE_C);
    enchaineuse << "cl";

#ifdef _MSC_VER
    enchaineuse << " /DLL "
                << "/OUT:" << nom_sortie << " " << nom_entree;
#else
    enchaineuse << " -shared -fPIC ";
    enchaineuse << nom_entree;
    enchaineuse << " -o ";
    enchaineuse << nom_sortie;
#endif

    /* Nous devons construire une chaine C, donc ajoutons un terminateur nul. */
    enchaineuse << '\0';

    return enchaineuse.chaine();
}

/* À FAIRE(r16) : il faudra proprement gérer les architectures pour les r16, ou trouver des
 * algorithmes pour supprimer les tables */
bool precompile_objet_r16(const kuri::chemin_systeme &chemin_racine_kuri)
{
    // objet pour la liaison statique de la bibliothèque
    {
        const auto fichier_objet = nom_fichier_objet_pour("r16_tables_x64");
        const auto chemin_objet = kuri::chemin_systeme::chemin_temporaire(fichier_objet);

        if (!kuri::chemin_systeme::existe(chemin_objet)) {
            const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
            const auto commande = commande_pour_fichier_objet(chemin_fichier, chemin_objet);

            std::cout << "Compilation des tables de conversion R16...\n";
            std::cout << "Exécution de la commande " << commande << std::endl;
            const auto err = system(commande.pointeur());
            if (err != 0) {
                std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
                return false;
            }

            std::cout << "Compilation du fichier statique réussie !" << std::endl;
        }
    }

    // objet pour la liaison dynamique de la bibliothèque, pour les métaprogrammes
    {
        /* A FAIRE : chemin bibliothèque pour Windows. */
        const auto fichier_objet = nom_bibliothèque_dynamique_pour("lib/x86_64-linux-gnu/libr16");
        const auto chemin_objet = kuri::chemin_systeme::chemin_temporaire(fichier_objet);

        if (!kuri::chemin_systeme::existe(chemin_objet)) {
            const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
            /* assure l'existence des dossiers parents */
            kuri::chemin_systeme::cree_dossiers(chemin_objet.chemin_parent());

            const auto commande = commande_pour_bibliotheque_dynamique(chemin_fichier,
                                                                       chemin_objet);

            std::cout << "Compilation des tables de conversion R16...\n";
            std::cout << "Exécution de la commande " << commande << std::endl;
            const auto err = system(commande.pointeur());
            if (err != 0) {
                std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
                return false;
            }

            std::cout << "Compilation du fichier dynamique réussie !" << std::endl;
        }
    }

    return true;
}

bool compile_objet_r16(const kuri::chemin_systeme &chemin_racine_kuri,
                       ArchitectureCible architecture_cible)
{
    if (architecture_cible == ArchitectureCible::X64) {
        // nous devrions déjà l'avoir
        return true;
    }

    const auto chemin_objet = kuri::chemin_systeme::chemin_temporaire("r16_tables_x86.o");
    if (kuri::chemin_systeme::existe(chemin_objet)) {
        return true;
    }

    const auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";

    Enchaineuse enchaineuse;
    enchaineuse << COMPILATEUR_CXX_COULISSE_C << " -c -m32 ";
    enchaineuse << chemin_fichier;
    enchaineuse << " -o ";
    enchaineuse << chemin_objet;
    enchaineuse << '\0';

    const auto commande = enchaineuse.chaine();

    std::cout << "Compilation des tables de conversion R16...\n";
    std::cout << "Exécution de la commande " << commande << std::endl;

    const auto err = system(commande.pointeur());

    if (err != 0) {
        std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
        return false;
    }

    std::cout << "Compilation du fichier statique réussie !" << std::endl;
    return true;
}
