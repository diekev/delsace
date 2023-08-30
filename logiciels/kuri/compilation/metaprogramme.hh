/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "representation_intermediaire/code_binaire.hh"

#include "structures/ensemble.hh"

struct DonneesExecution;
struct Fichier;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDirectiveExecute;
struct NoeudStruct;
struct Programme;
struct Statistiques;
struct UniteCompilation;

enum {
    DONNEES_CONSTANTES,
    DONNEES_GLOBALES,
};

enum {
    ADRESSE_CONSTANTE,
    ADRESSE_GLOBALE,
};

// Ces patchs sont utilisés pour écrire au bon endroit les adresses des constantes ou des globales
// dans les données d'exécution des métaprogrammes. Par exemple, les pointeurs des infos types des
// membres des structures sont écris dans un tableau constant, et le pointeur du tableau constant
// doit être écris dans la zone mémoire où se trouve le tableau de membres de l'InfoTypeStructure.
struct PatchDonneesConstantes {
    int ou;
    int quoi;
    int decalage_ou;
    int decalage_quoi;
};

std::ostream &operator<<(std::ostream &os, PatchDonneesConstantes const &patch);

struct DonneesConstantesExecutions {
    kuri::tableau<Globale, int> globales{};
    kuri::tableau<unsigned char, int> donnees_globales{};
    kuri::tableau<unsigned char, int> donnees_constantes{};
    kuri::tableau<PatchDonneesConstantes, int> patchs_donnees_constantes{};

    int ajoute_globale(Type *type, IdentifiantCode *ident, const Type *pour_info_type);

    void rassemble_statistiques(Statistiques &stats) const;
};

struct MetaProgramme {
    enum class ResultatExecution : int {
        NON_INITIALISE,
        ERREUR,
        SUCCES,
    };

    /* non-nul pour les directives d'exécutions (exécute, corps texte, etc.) */
    NoeudDirectiveExecute *directive = nullptr;

    /* non-nuls pour les corps-textes */
    NoeudBloc *corps_texte = nullptr;
    NoeudDeclarationEnteteFonction *corps_texte_pour_fonction = nullptr;
    NoeudStruct *corps_texte_pour_structure = nullptr;
    Fichier *fichier = nullptr;

    /* la fonction qui sera exécutée */
    NoeudDeclarationEnteteFonction *fonction = nullptr;

    UniteCompilation *unite = nullptr;

    bool fut_execute = false;
    bool a_rapporté_une_erreur = false;

    ResultatExecution resultat{};

    DonneesExecution *donnees_execution = nullptr;

    Programme *programme = nullptr;

    /* Pour les exécutions. */
    kuri::tableau<unsigned char, int> donnees_globales{};
    kuri::tableau<unsigned char, int> donnees_constantes{};

    /* Ensemble de toutes les fonctions potentiellement appelable lors de l'exécution du
     * métaprogramme. Ceci est utilisé pour chaque instruction d'appel afin de vérifier que
     * l'adresse de la fonction est connue et correspond à une adresse d'une fonction du programme
     * du métaprogramme.
     *
     * L'idée est similaire que celle du garde de controle de flux de Microsoft Windows :
     * https://msrc-blog.microsoft.com/2020/08/17/control-flow-guard-for-clang-llvm-and-rust/
     *
     * À FAIRE : cibles des branches.
     */
    kuri::ensemble<AtomeFonction *> cibles_appels{};
};
