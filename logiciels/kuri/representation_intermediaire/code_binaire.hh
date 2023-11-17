/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <ffi.h>  // pour ffi_type qui est un typedef
#include <iosfwd>

#include "biblinternes/outils/definitions.h"

#include "compilation/operateurs.hh"

#include "structures/tableau.hh"

// À FAIRE : l'optimisation pour la réutilisation de la mémoire des locales en se basant sur la
// durée de vie de celles-ci ne fonctionne pas
//           il existe des superposition partiells entre certaines variables
//           lors de la dernière investigation, il semberait que les instructions de retours au
//           milieu des fonctions y soient pour quelque chose pour le moment désactive cet
//           optimisation et alloue de l'espace pour toutes les variables au début de chaque
//           fonction.

struct Atome;
struct AtomeConstante;
struct AtomeFonction;
struct AtomeGlobale;
struct AtomeValeurConstante;
struct DonneesConstantesExecutions;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Instruction;
struct InstructionAppel;
struct MetaProgramme;
struct NoeudDeclarationEnteteFonction;
struct NoeudExpression;
struct Type;

namespace kuri {
struct chaine_statique;
}

struct ContexteGenerationCodeBinaire {
    EspaceDeTravail *espace = nullptr;
    const NoeudDeclarationEnteteFonction *fonction = nullptr;
};

using octet_t = unsigned char;

#define ENUMERE_CODES_OPERATION                                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_ACCEDE_INDEX)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE_REEL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_ALLOUE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL)                                                           \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_EXTERNE)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_COMPILATRICE)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_INTRINSÈQUE)                                               \
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
    ENUMERE_CODE_OPERATION_EX(OP_ENTIER_VERS_REEL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_VERIFIE_ADRESSAGE_CHARGE)                                        \
    ENUMERE_CODE_OPERATION_EX(OP_VERIFIE_ADRESSAGE_ASSIGNE)                                       \
    ENUMERE_CODE_OPERATION_EX(OP_VERIFIE_CIBLE_APPEL)

enum : octet_t {
#define ENUMERE_CODE_OPERATION_EX(code) code,
    ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
};

#define NOMBRE_OP_CODE (OP_VERIFIE_CIBLE_APPEL + 1)

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
struct drapeau_pour_constante<int64_t> {
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
struct drapeau_pour_constante<uint32_t> {
    static constexpr octet_t valeur = CONSTANTE_ENTIER_NATUREL | BITS_32;
};

template <>
struct drapeau_pour_constante<uint64_t> {
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
    Type const *type = nullptr;
    int adresse = 0;
};

struct Chunk {
    octet_t *code = nullptr;
    int64_t compte = 0;
    int64_t capacite = 0;

    // tient trace de toutes les allocations pour savoir où les variables se trouvent sur la pile
    // d'exécution
    int taille_allouee = 0;

    int nombre_labels = 0;

    kuri::tableau<int, int> decalages_labels{};

    kuri::tableau<Locale, int> locales{};

    ~Chunk();

    void initialise();

    void detruit();

  private:
    void emets(octet_t o);

    template <typename T>
    void emets(T v)
    {
        agrandis_si_necessaire(static_cast<int64_t>(sizeof(T)));
        auto ptr = reinterpret_cast<T *>(&code[compte]);
        *ptr = v;
        compte += static_cast<int64_t>(sizeof(T));
    }

    void agrandis_si_necessaire(int64_t taille);

    void émets_entête_op(octet_t op, NoeudExpression const *site);

  public:
    template <typename T>
    void emets_constante(T v)
    {
        émets_entête_op(OP_CONSTANTE, nullptr);
        emets(drapeau_pour_constante<T>::valeur);
        emets(v);
    }

    void émets_chaine_constante(NoeudExpression const *site,
                                void *pointeur_chaine,
                                int64_t taille_chaine);

    void émets_retour(NoeudExpression const *site);

    int emets_allocation(NoeudExpression const *site, Type const *type, IdentifiantCode *ident);
    void emets_assignation(ContexteGenerationCodeBinaire contexte,
                           NoeudExpression const *site,
                           Type const *type,
                           bool ajoute_verification);
    void emets_charge(NoeudExpression const *site, Type const *type, bool ajoute_verification);
    void emets_charge_variable(NoeudExpression const *site,
                               int pointeur,
                               Type const *type,
                               bool ajoute_verification);
    void emets_reference_globale(NoeudExpression const *site, int pointeur);
    void emets_reference_variable(NoeudExpression const *site, int pointeur);
    void emets_reference_membre(NoeudExpression const *site, unsigned decalage);
    void emets_appel(NoeudExpression const *site,
                     AtomeFonction const *fonction,
                     unsigned taille_arguments,
                     InstructionAppel const *inst_appel,
                     bool ajoute_verification);
    void emets_appel_externe(NoeudExpression const *site,
                             AtomeFonction const *fonction,
                             unsigned taille_arguments,
                             InstructionAppel const *inst_appel,
                             bool ajoute_verification);
    void emets_appel_compilatrice(NoeudExpression const *site,
                                  AtomeFonction const *fonction,
                                  bool ajoute_verification);
    void emets_appel_intrinsèque(NoeudExpression const *site, AtomeFonction const *fonction);
    void emets_appel_pointeur(NoeudExpression const *site,
                              unsigned taille_arguments,
                              InstructionAppel const *inst_appel,
                              bool ajoute_verification);
    void emets_acces_index(NoeudExpression const *site, Type const *type);

    void emets_branche(NoeudExpression const *site,
                       kuri::tableau<PatchLabel> &patchs_labels,
                       int index);
    void emets_branche_condition(NoeudExpression const *site,
                                 kuri::tableau<PatchLabel> &patchs_labels,
                                 int index_label_si_vrai,
                                 int index_label_si_faux);

    void emets_label(NoeudExpression const *site, int index);

    void emets_operation_unaire(NoeudExpression const *site,
                                OpérateurUnaire::Genre op,
                                Type const *type);
    void emets_operation_binaire(NoeudExpression const *site,
                                 OpérateurBinaire::Genre op,
                                 Type const *type_gauche,
                                 Type const *type_droite);

    void émets_transtype(NoeudExpression const *site,
                         uint8_t op,
                         uint32_t taille_source,
                         uint32_t taille_dest);
};

void desassemble(Chunk const &chunk, kuri::chaine_statique nom, std::ostream &os);
int64_t desassemble_instruction(Chunk const &chunk, int64_t decalage, std::ostream &os);

struct Globale {
    IdentifiantCode *ident = nullptr;
    Type const *type = nullptr;
    int adresse = 0;
    void *adresse_pour_execution = nullptr;
};

struct DonnéesExécutionFonction {
    Chunk chunk{};

    struct DonneesFonctionExterne {
        kuri::tablet<ffi_type *, 6> types_entrees{};
        ffi_cif cif{};
        void (*ptr_fonction)() = nullptr;
    };

    DonneesFonctionExterne donnees_externe{};

    int64_t mémoire_utilisée() const;
};

class ConvertisseuseRI {
    EspaceDeTravail *espace = nullptr;
    DonneesConstantesExecutions *donnees_executions = nullptr;

    const NoeudDeclarationEnteteFonction *fonction_courante = nullptr;

    /* Le métaprogramme pour lequel nous devons générer du code. Il est là avant pour stocker les
     * adresses des globales qu'il utilise. */
    MetaProgramme *metaprogramme = nullptr;

    /* Patchs pour les labels, puisque nous d'abord générer le code des branches avant de connaître
     * les adresses cibles des sauts, nous utilisons ces patchs pour insérer les adresses au bon
     * endroit à la fin de la génération de code. */
    kuri::tableau<PatchLabel> patchs_labels{};

    bool verifie_adresses = false;

  public:
    ConvertisseuseRI(EspaceDeTravail *espace_, MetaProgramme *metaprogramme_);

    EMPECHE_COPIE(ConvertisseuseRI);

    bool genere_code(const kuri::tableau<AtomeFonction *> &fonctions);

    bool genere_code_pour_fonction(AtomeFonction *fonction);

  private:
    void genere_code_binaire_pour_instruction(Instruction const *instruction,
                                              Chunk &chunk,
                                              bool pour_operande);

    void genere_code_binaire_pour_constante(AtomeConstante *constante, Chunk &chunk);
    void genere_code_binaire_pour_valeur_constante(AtomeValeurConstante const *valeur_constante,
                                                   Chunk &chunk);

    void genere_code_binaire_pour_initialisation_globale(AtomeConstante *constante,
                                                         int decalage,
                                                         int ou_patcher);

    void genere_code_binaire_pour_atome(Atome *atome, Chunk &chunk, bool pour_operande);

    int ajoute_globale(AtomeGlobale *globale);
    int genere_code_pour_globale(AtomeGlobale *atome_globale);

    ContexteGenerationCodeBinaire contexte() const;
};

ffi_type *converti_type_ffi(Type const *type);
