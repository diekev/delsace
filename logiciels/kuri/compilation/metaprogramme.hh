/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

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
// doit être écris dans la zone mémoire ou se trouve le tableau de membres de l'InfoTypeStructure.
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

    int ajoute_globale(Type *type, IdentifiantCode *ident);

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
