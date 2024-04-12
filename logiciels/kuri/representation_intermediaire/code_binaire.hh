/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <ffi.h>  // pour ffi_type qui est un typedef
#include <iosfwd>

#include "compilation/operateurs.hh"

#include "structures/tableau.hh"

#include "utilitaires/macros.hh"

struct AdresseDonnéesExécution;
struct Atome;
struct AtomeConstante;
struct AtomeFonction;
struct AtomeGlobale;
struct DonnéesConstantesExécutions;
struct Enchaineuse;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Instruction;
struct InstructionAllocation;
struct InstructionAppel;
struct MetaProgramme;
struct NoeudDéclarationEntêteFonction;
struct NoeudExpression;
struct NoeudDéclarationType;
struct ProgrammeRepreInter;
using Type = NoeudDéclarationType;

namespace kuri {
struct chaine_statique;
}

struct ContexteGénérationCodeBinaire {
    EspaceDeTravail *espace = nullptr;
    const NoeudDéclarationEntêteFonction *fonction = nullptr;
};

using octet_t = unsigned char;

#define ENUMERE_CODES_OPERATION                                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_ACCEDE_INDEX)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_INCRÉMENTE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_INCRÉMENTE_LOCALE)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_AJOUTE_REEL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL)                                                           \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_EXTERNE)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_COMPILATRICE)                                              \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_INTRINSÈQUE)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_APPEL_POINTEUR)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_ASSIGNE)                                                         \
    ENUMERE_CODE_OPERATION_EX(OP_ASSIGNE_LOCALE)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_INIT_LOCALE_ZÉRO)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_COPIE_LOCALE)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_NATUREL)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_REEL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_AUGMENTE_RELATIF)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_BRANCHE)                                                         \
    ENUMERE_CODE_OPERATION_EX(OP_BRANCHE_CONDITION)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_BRANCHE_SI_ZÉRO)                                                 \
    ENUMERE_CODE_OPERATION_EX(OP_STRUCTURE_CONSTANTE)                                             \
    ENUMERE_CODE_OPERATION_EX(OP_CHARGE)                                                          \
    ENUMERE_CODE_OPERATION_EX(OP_CHARGE_LOCALE)                                                   \
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
    ENUMERE_CODE_OPERATION_EX(OP_MULTIPLIE)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_MULTIPLIE_REEL)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_NON_BINAIRE)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_OU_BINAIRE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_OU_EXCLUSIF)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_GLOBALE)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_GLOBALE_EXTERNE)                                       \
    ENUMERE_CODE_OPERATION_EX(OP_REFERENCE_MEMBRE)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_RÉFÉRENCE_MEMBRE_LOCALE)                                         \
    ENUMERE_CODE_OPERATION_EX(OP_RÉFÉRENCE_LOCALE)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_RESTE_NATUREL)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_RESTE_RELATIF)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_RETOURNE)                                                        \
    ENUMERE_CODE_OPERATION_EX(OP_SOUSTRAIT)                                                       \
    ENUMERE_CODE_OPERATION_EX(OP_DÉCRÉMENTE)                                                      \
    ENUMERE_CODE_OPERATION_EX(OP_SOUSTRAIT_REEL)                                                  \
    ENUMERE_CODE_OPERATION_EX(OP_REEL_VERS_NATUREL)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_REEL_VERS_RELATIF)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_NATUREL_VERS_REEL)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_RELATIF_VERS_REEL)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_REMBOURRAGE)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_VÉRIFIE_ADRESSAGE_CHARGE)                                        \
    ENUMERE_CODE_OPERATION_EX(OP_VÉRIFIE_ADRESSAGE_ASSIGNE)                                       \
    ENUMERE_CODE_OPERATION_EX(OP_VÉRIFIE_CIBLE_APPEL)                                             \
    ENUMERE_CODE_OPERATION_EX(OP_VÉRIFIE_CIBLE_BRANCHE)                                           \
    ENUMERE_CODE_OPERATION_EX(OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION)                                 \
    ENUMERE_CODE_OPERATION_EX(OP_STAT_INSTRUCTION)                                                \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_INSTRUCTION)                                               \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_VALEURS_LOCALES)                                           \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_APPEL)                                                     \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_ENTRÉES)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_SORTIES)                                                   \
    ENUMERE_CODE_OPERATION_EX(OP_LOGUE_RETOUR)                                                    \
    ENUMERE_CODE_OPERATION_EX(OP_NOTIFIE_EMPILAGE_VALEUR)                                         \
    ENUMERE_CODE_OPERATION_EX(OP_NOTIFIE_DÉPILAGE_VALEUR)                                         \
    ENUMERE_CODE_OPERATION_EX(OP_PROFILE_DÉBUTE_APPEL)                                            \
    ENUMERE_CODE_OPERATION_EX(OP_PROFILE_TERMINE_APPEL)                                           \
    ENUMERE_CODE_OPERATION_EX(OP_INATTEIGNABLE)

enum : octet_t {
#define ENUMERE_CODE_OPERATION_EX(code) code,
    ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
};

#define NOMBRE_OP_CODE (OP_LOGUE_RETOUR + 1)

kuri::chaine_statique chaine_code_operation(octet_t code_operation);

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
    int64_t capacité = 0;

    bool émets_stats_ops = false;
    bool émets_vérification_branches = false;
    bool émets_notifications_empilage = false;
    bool émets_profilage = false;

    // tient trace de toutes les allocations pour savoir où les variables se trouvent sur la pile
    // d'exécution
    int taille_allouée = 0;

    kuri::tableau<Locale, int> locales{};

  private:
    struct SiteSource {
        /* Le décalage dans les instructions où se trouve le site. */
        int décalage = 0;
        NoeudExpression const *site = nullptr;
    };

    kuri::tableau<SiteSource, int> m_sites_source{};

  public:
    ~Chunk();

    NoeudExpression const *donne_site_pour_adresse(octet_t *adresse) const;

    void initialise();

    void détruit();

    int64_t mémoire_utilisée() const;

  private:
    void émets(octet_t o);

    template <typename T>
    void émets(T v)
    {
        agrandis_si_nécessaire(static_cast<int64_t>(sizeof(T)));
        auto ptr = reinterpret_cast<T *>(&code[compte]);
        *ptr = v;
        compte += static_cast<int64_t>(sizeof(T));
    }

    void agrandis_si_nécessaire(int64_t taille);

    void émets_entête_op(octet_t op, NoeudExpression const *site);

    void émets_logue_instruction(int32_t décalage);
    void émets_logue_appel(AtomeFonction const *atome);
    void émets_logue_entrées(AtomeFonction const *atome, unsigned taille_arguments);
    void émets_logue_sorties();
    void émets_logue_retour();

    void émets_notifie_empilage(NoeudExpression const *site, uint32_t taille);
    void émets_notifie_dépilage(NoeudExpression const *site, uint32_t taille);

    void émets_dépilage_paramètres_appel(NoeudExpression const *site,
                                         InstructionAppel const *inst);
    void émets_empilage_retour_appel(NoeudExpression const *site, InstructionAppel const *inst);

    void émets_profile_débute_appel();
    void émets_profile_termine_appel();

    void ajoute_site_source(NoeudExpression const *site);

  public:
    [[nodiscard]] int ajoute_locale(InstructionAllocation const *alloc);

    template <typename T>
    void émets_constante(T v)
    {
        émets_entête_op(OP_CONSTANTE, nullptr);
        émets(drapeau_pour_constante<T>::valeur);
        émets(v);
        émets_notifie_empilage(nullptr, sizeof(T));
    }

    int émets_structure_constante(uint32_t taille_structure);

    void émets_retour(NoeudExpression const *site, Atome const *valeur);

    void émets_assignation(ContexteGénérationCodeBinaire contexte,
                           NoeudExpression const *site,
                           Type const *type);
    void émets_assignation_locale(NoeudExpression const *site, int pointeur, Type const *type);
    void émets_init_locale_zéro(const NoeudExpression *site, int pointeur, Type const *type);
    void émets_copie_locale(const NoeudExpression *site,
                            const Type *type,
                            int pointeur_source,
                            int pointeur_destination);
    void émets_charge(NoeudExpression const *site, Type const *type);
    void émets_charge_locale(NoeudExpression const *site, int pointeur, Type const *type);
    void émets_référence_globale(NoeudExpression const *site, int pointeur);
    void émets_référence_globale_externe(const NoeudExpression *site, const void *adresse);
    void émets_référence_locale(NoeudExpression const *site, int pointeur);
    void émets_référence_membre(NoeudExpression const *site, unsigned decalage);
    void émets_référence_membre_locale(NoeudExpression const *site,
                                       int pointeur,
                                       uint32_t décalage);
    void émets_appel(NoeudExpression const *site,
                     AtomeFonction const *fonction,
                     const InstructionAppel *inst_appel,
                     unsigned taille_arguments);
    void émets_appel_externe(NoeudExpression const *site,
                             AtomeFonction const *fonction,
                             unsigned taille_arguments,
                             InstructionAppel const *inst_appel);
    void émets_appel_compilatrice(NoeudExpression const *site,
                                  AtomeFonction const *fonction,
                                  InstructionAppel const *inst_appel);
    void émets_appel_intrinsèque(NoeudExpression const *site,
                                 AtomeFonction const *fonction,
                                 InstructionAppel const *inst_appel);
    void émets_appel_pointeur(NoeudExpression const *site,
                              unsigned taille_arguments,
                              InstructionAppel const *inst_appel);
    void émets_accès_index(NoeudExpression const *site, Type const *type);

    void émets_branche(NoeudExpression const *site,
                       kuri::tableau<PatchLabel> &patchs_labels,
                       int index);
    void émets_branche_condition(NoeudExpression const *site,
                                 kuri::tableau<PatchLabel> &patchs_labels,
                                 int index_label_si_vrai,
                                 int index_label_si_faux);

    void émets_branche_si_zéro(NoeudExpression const *site,
                               kuri::tableau<PatchLabel> &patchs_labels,
                               uint32_t taille_opérande,
                               int index_label_si_vrai,
                               int index_label_si_faux);

    void émets_operation_unaire(NoeudExpression const *site,
                                OpérateurUnaire::Genre op,
                                Type const *type);
    void émets_operation_binaire(NoeudExpression const *site,
                                 OpérateurBinaire::Genre op,
                                 Type const *type_résultat,
                                 Type const *type_gauche,
                                 Type const *type_droite);

    void émets_incrémente(NoeudExpression const *site, Type const *type);
    void émets_incrémente_locale(const NoeudExpression *site, const Type *type, int pointeur);
    void émets_décrémente(NoeudExpression const *site, Type const *type);

    void émets_transtype(NoeudExpression const *site,
                         uint8_t op,
                         uint32_t taille_source,
                         uint32_t taille_dest);

    void émets_inatteignable(NoeudExpression const *site);

    void émets_rembourrage(uint32_t rembourrage);
    void rétrécis_capacité_sur_taille();
};

[[nodiscard]] kuri::chaine désassemble(Chunk const &chunk, kuri::chaine_statique nom);
int64_t désassemble_instruction(Chunk const &chunk, int64_t decalage, Enchaineuse &os);

struct Globale {
    IdentifiantCode *ident = nullptr;
    Type const *type = nullptr;
    int adresse = 0;
    void *adresse_pour_exécution = nullptr;
};

struct DonnéesExécutionFonction {
    Chunk chunk{};

    struct DonnéesFonctionExterne {
        kuri::tablet<ffi_type *, 6> types_entrées{};
        ffi_cif cif{};
        void (*ptr_fonction)() = nullptr;
    };

    DonnéesFonctionExterne données_externe{};

    int64_t mémoire_utilisée() const;
};

class CompilatriceCodeBinaire {
    EspaceDeTravail *espace = nullptr;
    DonnéesConstantesExécutions *données_exécutions = nullptr;

    const NoeudDéclarationEntêteFonction *fonction_courante = nullptr;
    AtomeFonction *m_atome_fonction_courante = nullptr;

    /* Le métaprogramme pour lequel nous devons générer du code. Il est là avant pour stocker les
     * adresses des globales qu'il utilise. */
    MetaProgramme *métaprogramme = nullptr;

    /* Patchs pour les labels, puisque nous d'abord générer le code des branches avant de connaître
     * les adresses cibles des sauts, nous utilisons ces patchs pour insérer les adresses au bon
     * endroit à la fin de la génération de code. */
    kuri::tableau<int, int> décalages_labels{};
    kuri::tableau<PatchLabel> patchs_labels{};

    /* Pour stocker les index des locales. */
    kuri::tableau<int> m_index_locales{};

    bool vérifie_adresses = false;
    bool émets_stats_ops = false;
    bool notifie_empilage = false;
    bool émets_profilage = false;

  public:
    CompilatriceCodeBinaire(EspaceDeTravail *espace_, MetaProgramme *metaprogramme_);

    EMPECHE_COPIE(CompilatriceCodeBinaire);

    bool génère_code(const ProgrammeRepreInter &repr_inter);

  private:
    bool génère_code_pour_fonction(AtomeFonction const *fonction);

    void génère_code_pour_instruction(Instruction const *instruction,
                                      Chunk &chunk,
                                      bool pour_operande);
    void génère_code_atome_constant(const AtomeConstante *atome,
                                    const AdresseDonnéesExécution &adressage_destination,
                                    octet_t *destination,
                                    int décalage) const;

    void génère_code_pour_atome(Atome const *atome, Chunk &chunk);

    bool ajoute_globale(AtomeGlobale *globale) const;
    void génère_code_pour_globale(AtomeGlobale const *atome_globale) const;

    int donne_index_locale(InstructionAllocation const *alloc) const;

    ContexteGénérationCodeBinaire contexte() const;

    void ajoute_réadressage_pour_globale(const Globale &globale,
                                         const AdresseDonnéesExécution &adressage_destination,
                                         int décalage) const;
};

ffi_type *converti_type_ffi(Type const *type);
