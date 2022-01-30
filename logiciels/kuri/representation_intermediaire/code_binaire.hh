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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <iostream>

#include <ffi.h>

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/pile.hh"

#include "structures/table_hachage.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

struct Atome;
struct AtomeConstante;
struct AtomeFonction;
struct AtomeGlobale;
struct Compilatrice;
struct DonneesConstantesExecutions;
struct DonneesExecution;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Instruction;
struct InstructionAppel;
struct MetaProgramme;
struct NoeudBloc;
struct NoeudDeclaration;
struct NoeudDeclarationCorpsFonction;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudInstructionTente;
struct NoeudPour;
struct NoeudStruct;
struct Type;
struct TypeFonction;

using octet_t = unsigned char;

#define ENUMERE_CODES_OPERATION                                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_ACCEDE_INDEX)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE_REEL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_ALLOUE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL)                                                           \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_EXTERNE)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_POINTEUR)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_ASSIGNE)                                                         \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_NATUREL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_REEL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_RELATIF)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_BRANCHE)                                                         \
    ENUMERE_CODE_OPERATION_EX(OP_BRANCHE_CONDITION)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_CHAINE_CONSTANTE)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_CHARGE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_EGAL)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_EGAL_REEL)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INEGAL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INEGAL_REEL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF)                                                        \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF_EGAL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF_EGAL_NATUREL)                                           \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF_EGAL_REEL)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF_NATUREL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_INF_REEL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP)                                                        \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP_EGAL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP_EGAL_NATUREL)                                           \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP_EGAL_REEL)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP_NATUREL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_COMP_SUP_REEL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_COMPLEMENT_ENTIER)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_COMPLEMENT_REEL)                                                 \
    ENUMERE_CODE_OPERATION_EX(OP_CONSTANTE)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_DEC_DROITE_ARITHM)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_DEC_DROITE_LOGIQUE)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_DEC_GAUCHE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_DIMINUE_NATUREL)                                                 \
    ENUMERE_CODE_OPERATION_EX(OP_DIMINUE_REEL)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_DIMINUE_RELATIF)                                                 \
    ENUMERE_CODE_OPERATION_EX(OP_DIVISE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_DIVISE_REEL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_DIVISE_RELATIF)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_ET_BINAIRE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_LABEL)                                                           \
    ENUMERE_CODE_OPERATION_EX(OP_MULTIPLIE)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_MULTIPLIE_REEL)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_NON_BINAIRE)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_OU_BINAIRE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_OU_EXCLUSIF)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_GLOBALE)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_MEMBRE)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_VARIABLE)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_RESTE_NATUREL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_RESTE_RELATIF)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_RETOURNE)                                                        \
    ENUMERE_CODE_OPERATION_EX(OP_SOUSTRAIT)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_SOUSTRAIT_REEL)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_REEL_VERS_ENTIER)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_ENTIER_VERS_REEL)

enum : octet_t {
#define ENUMERE_CODE_OPERATION_EX(code) code,
    ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
};

const char *chaine_code_operation(octet_t code_operation);

enum : octet_t {
    CONSTANTE_ENTIER_NATUREL = 1,
    CONSTANTE_ENTIER_RELATIF = 2,
    CONSTANTE_NOMBRE_REEL = 4,
    BITS_8 = 8,
    BITS_16 = 16,
    BITS_32 = 32,
    BITS_64 = 64,
};

template <typename T>
struct drapeau_pour_constante;

template <>
struct drapeau_pour_constante<bool> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_RELATIF | BITS_8;
};

template <>
struct drapeau_pour_constante<char> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_RELATIF | BITS_8;
};

template <>
struct drapeau_pour_constante<short> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_RELATIF | BITS_16;
};

template <>
struct drapeau_pour_constante<int> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_RELATIF | BITS_32;
};

template <>
struct drapeau_pour_constante<long> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_RELATIF | BITS_64;
};

template <>
struct drapeau_pour_constante<unsigned char> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_NATUREL | BITS_8;
};

template <>
struct drapeau_pour_constante<unsigned short> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_NATUREL | BITS_16;
};

template <>
struct drapeau_pour_constante<unsigned int> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_NATUREL | BITS_32;
};

template <>
struct drapeau_pour_constante<unsigned long> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_NATUREL | BITS_64;
};

template <>
struct drapeau_pour_constante<float> {
    static constexpr octet_t valeur = CONSTANTE_NOMBRE_REEL | BITS_32;
};

template <>
struct drapeau_pour_constante<double> {
    static constexpr octet_t valeur = CONSTANTE_NOMBRE_REEL | BITS_64;
};

struct PatchLabel {
    int index_label;
    int adresse;
};

struct Locale {
    IdentifiantCode *ident = nullptr;
    Type *type = nullptr;
    int adresse = 0;
};

struct Chunk {
    octet_t *code = nullptr;
    long compte = 0;
    long capacite = 0;

    // tient trace de toutes les allocations pour savoir où les variables se trouvent sur la pile
    // d'exécution
    int taille_allouee = 0;

    int nombre_labels = 0;

    kuri::tableau<int, int> decalages_labels{};

    kuri::tableau<Locale, int> locales{};

    ~Chunk();

    void initialise();

    void detruit();

    void emets(octet_t o);

    template <typename T>
    void emets(T v)
    {
        agrandis_si_necessaire(static_cast<long>(sizeof(T)));
        auto ptr = reinterpret_cast<T *>(&code[compte]);
        *ptr = v;
        compte += static_cast<long>(sizeof(T));
    }

    template <typename T>
    void emets_constante(T v)
    {
        emets(OP_CONSTANTE);
        emets(nullptr); /* site */
        emets(drapeau_pour_constante<T>::valeur);
        emets(v);
    }

    void agrandis_si_necessaire(long taille);

    int emets_allocation(NoeudExpression *site, Type *type, IdentifiantCode *ident);
    void emets_assignation(NoeudExpression *site, Type *type);
    void emets_charge(NoeudExpression *site, Type *type);
    void emets_charge_variable(NoeudExpression *site, int pointeur, Type *type);
    void emets_reference_globale(NoeudExpression *site, int pointeur);
    void emets_reference_variable(NoeudExpression *site, int pointeur);
    void emets_reference_membre(NoeudExpression *site, unsigned decalage);
    void emets_appel(NoeudExpression *site,
                     AtomeFonction *fonction,
                     unsigned taille_arguments,
                     InstructionAppel *inst_appel);
    void emets_appel_externe(NoeudExpression *site,
                             AtomeFonction *fonction,
                             unsigned taille_arguments,
                             InstructionAppel *inst_appel);
    void emets_appel_pointeur(NoeudExpression *site,
                              unsigned taille_arguments,
                              InstructionAppel *inst_appel);
    void emets_acces_index(NoeudExpression *site, Type *type);

    void emets_branche(NoeudExpression *site, kuri::tableau<PatchLabel> &patchs_labels, int index);
    void emets_branche_condition(NoeudExpression *site,
                                 kuri::tableau<PatchLabel> &patchs_labels,
                                 int index_label_si_vrai,
                                 int index_label_si_faux);

    void emets_label(NoeudExpression *site, int index);
};

void desassemble(Chunk const &chunk, const char *nom, std::ostream &os);
long desassemble_instruction(Chunk const &chunk, long decalage, std::ostream &os);

struct Globale {
    IdentifiantCode *ident = nullptr;
    Type *type = nullptr;
    int adresse = 0;
    void *adresse_pour_execution = nullptr;
};

// À FAIRE : l'optimisation pour la réutilisation de la mémoire des locales en se basant sur la
// durée de vie de celles-ci ne fonctionne pas
//           il existe des superposition partiells entre certaines variables
//           lors de la dernière investigation, il semberait que les instructions de retours au
//           milieu des fonctions y soient pour quelque chose pour le moment désactive cet
//           optimisation et alloue de l'espace pour toutes les variables au début de chaque
//           fonction.
#undef OPTIMISE_ALLOCS

class ConvertisseuseRI {
    EspaceDeTravail *espace = nullptr;
    DonneesConstantesExecutions *donnees_executions = nullptr;

    /* Le métaprogramme pour lequel nous devons générer du code. Il est là avant pour stocker les
     * adresses des globales qu'il utilise. */
    MetaProgramme *metaprogramme = nullptr;

    /* Patchs pour les labels, puisque nous d'abord générer le code des branches avant de connaître
     * les adresses cibles des sauts, nous utilisons ces patchs pour insérer les adresses au bon
     * endroit à la fin de la génération de code. */
    kuri::tableau<PatchLabel> patchs_labels{};

#ifdef OPTIMISE_ALLOCS
    dls::pile<int> pile_taille{};
    int dernier_decalage_pile = 0;
#endif

  public:
    ConvertisseuseRI(EspaceDeTravail *espace_, MetaProgramme *metaprogramme_);

    COPIE_CONSTRUCT(ConvertisseuseRI);

    bool genere_code(const kuri::tableau<AtomeFonction *> &fonctions);

    bool genere_code_pour_fonction(AtomeFonction *fonction);

  private:
    void genere_code_binaire_pour_instruction(Instruction *instruction,
                                              Chunk &chunk,
                                              bool pour_operande);

    void genere_code_binaire_pour_constante(AtomeConstante *constante, Chunk &chunk);

    void genere_code_binaire_pour_initialisation_globale(AtomeConstante *constante,
                                                         int decalage,
                                                         int ou_patcher);

    void genere_code_binaire_pour_atome(Atome *atome, Chunk &chunk, bool pour_operande);

    int ajoute_globale(AtomeGlobale *globale);
    int genere_code_pour_globale(AtomeGlobale *atome_globale);
};

ffi_type *converti_type_ffi(Type *type);
