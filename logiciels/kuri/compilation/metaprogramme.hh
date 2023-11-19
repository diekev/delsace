/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "representation_intermediaire/code_binaire.hh"

#include "structures/ensemble.hh"

struct DonnéesExécution;
struct Fichier;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDirectiveExecute;
struct NoeudStruct;
struct Programme;
struct Statistiques;
struct UniteCompilation;

enum {
    DONNÉES_CONSTANTES,
    DONNÉES_GLOBALES,
};

enum {
    ADRESSE_CONSTANTE,
    ADRESSE_GLOBALE,
};

struct AdresseDonnéesExécution {
    int type{};
    int décalage{};
};

// Ces patchs sont utilisés pour écrire au bon endroit les adresses des constantes ou des globales
// dans les données d'exécution des métaprogrammes. Par exemple, les pointeurs des infos types des
// membres des structures sont écris dans un tableau constant, et le pointeur du tableau constant
// doit être écris dans la zone mémoire où se trouve le tableau de membres de l'InfoTypeStructure.
struct PatchDonnéesConstantes {
    AdresseDonnéesExécution destination{};
    AdresseDonnéesExécution source{};
};

std::ostream &operator<<(std::ostream &os, PatchDonnéesConstantes const &patch);

struct DonnéesConstantesExécutions {
    kuri::tableau<Globale, int> globales{};
    kuri::tableau<unsigned char, int> données_globales{};
    kuri::tableau<unsigned char, int> données_constantes{};
    kuri::tableau<PatchDonnéesConstantes, int> patchs_données_constantes{};

    int ajoute_globale(Type *type, IdentifiantCode *ident, const Type *pour_info_type);

    void rassemble_statistiques(Statistiques &stats) const;
};

/* ------------------------------------------------------------------------- */
/** \name ComportementMétaprogramme
 * Drapeaux pour savoir ce qu'un métaprogramme fera lors de son exécution.
 * Les drapaux dérivent des fonctions ajoutées au programme du métaprogramme.
 * \{ */

enum class ComportementMétaprogramme : uint32_t {
    /* Le métaprogramme possède des appels à #compilatrice_commence_interception. */
    COMMENCE_INTERCEPTION = (1 << 0),
    /* Le métaprogramme possède des appels à #compilatrice_termine_interception. */
    TERMINE_INTERCEPTION = (1 << 1),
    /* Le métaprogramme va ajouter du code à la compilation. */
    AJOUTE_CODE = (1 << 2),

    REQUIERS_MESSAGE = COMMENCE_INTERCEPTION | TERMINE_INTERCEPTION,
};
DEFINIS_OPERATEURS_DRAPEAU(ComportementMétaprogramme)

/** \} */

struct MetaProgramme {
    enum class RésultatExécution : int {
        ERREUR,
        SUCCÈS,
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

    RésultatExécution resultat{};

    DonnéesExécution *données_exécution = nullptr;

    Programme *programme = nullptr;

    /* Pour les exécutions. */
    kuri::tableau<unsigned char, int> données_globales{};
    kuri::tableau<unsigned char, int> données_constantes{};

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

    ComportementMétaprogramme comportement{};

    ~MetaProgramme();

    bool ajoutera_du_code() const
    {
        return (comportement & ComportementMétaprogramme::AJOUTE_CODE) !=
               static_cast<ComportementMétaprogramme>(0);
    }

    bool écoutera_les_messages() const
    {
        return (comportement & ComportementMétaprogramme::REQUIERS_MESSAGE) !=
               static_cast<ComportementMétaprogramme>(0);
    }
};
