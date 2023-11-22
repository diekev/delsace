/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "code_binaire.hh"

#include <iomanip>
#include <iostream>

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/bibliotheque.hh"
#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/ipa.hh"
#include "compilation/log.hh"
#include "compilation/operateurs.hh"

#include "parsage/outils_lexemes.hh"

#include "impression.hh"

kuri::chaine_statique chaine_code_operation(octet_t code_operation)
{
    switch (code_operation) {
#define ENUMERE_CODE_OPERATION_EX(code)                                                           \
    case code:                                                                                    \
        return #code;
        ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
    }

    return "ERREUR";
}

/* ************************************************************************** */

Chunk::~Chunk()
{
    détruit();
}

NoeudExpression const *Chunk::donne_site_pour_adresse(octet_t *adresse) const
{
    assert(adresse >= code && adresse < (code + compte));

    if (m_sites_source.est_vide()) {
        return nullptr;
    }

    auto décalage = static_cast<int>(adresse - code);
    if (décalage < 0 || décalage >= compte) {
        return nullptr;
    }

    for (int i = 0; i < m_sites_source.taille() - 1; i++) {
        if (décalage >= m_sites_source[i].décalage && décalage < m_sites_source[i + 1].décalage) {
            return m_sites_source[i].site;
        }
    }

    return m_sites_source.dernière().site;
}

void Chunk::ajoute_site_source(NoeudExpression const *site)
{
    if (!site) {
        return;
    }

    if (m_sites_source.est_vide()) {
        m_sites_source.ajoute({static_cast<int>(compte), site});
        return;
    }

    if (m_sites_source.dernière().site == site) {
        return;
    }

    m_sites_source.ajoute({static_cast<int>(compte), site});
}

void Chunk::initialise()
{
    code = nullptr;
    compte = 0;
    capacité = 0;
    m_sites_source.efface();
}

void Chunk::détruit()
{
    memoire::deloge_tableau("Chunk::code", code, capacité);
    initialise();
}

void Chunk::rétrécis_capacité_sur_taille()
{
    if (compte == capacité) {
        return;
    }

    memoire::reloge_tableau("Chunk::code", code, capacité, compte);
    capacité = compte;
}

int64_t Chunk::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += capacité;
    résultat += locales.taille_memoire();
    résultat += m_sites_source.taille_memoire();
    return résultat;
}

void Chunk::émets(octet_t o)
{
    agrandis_si_nécessaire(1);
    code[compte] = o;
    compte += 1;
}

void Chunk::agrandis_si_nécessaire(int64_t taille)
{
    auto taille_requise = compte + taille;
    if (capacité < taille_requise) {
        auto nouvelle_capacité = taille_requise < 8 ? 8 : taille_requise;
        if (nouvelle_capacité < capacité * 2) {
            nouvelle_capacité = capacité * 2;
        }
        memoire::reloge_tableau("Chunk::code", code, capacité, nouvelle_capacité);
        capacité = nouvelle_capacité;
    }
}

void Chunk::émets_entête_op(octet_t op, const NoeudExpression *site)
{
    ajoute_site_source(site);

    if (émets_stats_ops) {
        émets(OP_STAT_INSTRUCTION);
    }

    émets(op);
}

void Chunk::émets_logue_instruction(int32_t décalage)
{
    émets_entête_op(OP_LOGUE_INSTRUCTION, nullptr);
    émets(décalage);
}

void Chunk::émets_logue_appel(AtomeFonction const *atome)
{
    émets_entête_op(OP_LOGUE_APPEL, nullptr);
    émets(atome);
}

void Chunk::émets_logue_entrées(AtomeFonction const *atome, unsigned taille_arguments)
{
    émets_entête_op(OP_LOGUE_ENTRÉES, nullptr);
    émets(atome);
    émets(taille_arguments);
}

void Chunk::émets_logue_sorties()
{
    émets_entête_op(OP_LOGUE_SORTIES, nullptr);
}

void Chunk::émets_logue_retour()
{
    émets_entête_op(OP_LOGUE_RETOUR, nullptr);
}

int Chunk::ajoute_locale(InstructionAllocation const *alloc)
{
    auto type = alloc->type->comme_type_pointeur()->type_pointe;

    // XXX - À FAIRE : normalise les entiers constants
    if (type->est_type_entier_constant()) {
        type->taille_octet = 4;
    }
    assert(type->taille_octet);

    auto index = locales.taille();
    locales.ajoute({alloc->ident, alloc->type, taille_allouée});
    taille_allouée += static_cast<int>(type->taille_octet);
    return index;
}

void Chunk::émets_chaine_constante(const NoeudExpression *site,
                                   const void *pointeur_chaine,
                                   int64_t taille_chaine)
{
    émets_entête_op(OP_CHAINE_CONSTANTE, site);
    émets(pointeur_chaine);
    émets(taille_chaine);
}

void Chunk::émets_retour(NoeudExpression const *site)
{
    émets_entête_op(OP_RETOURNE, site);
}

void Chunk::émets_assignation(ContexteGénérationCodeBinaire contexte,
                              NoeudExpression const *site,
                              Type const *type)
{
#if 0  // ndef CMAKE_BUILD_TYPE_PROFILE
    assert_rappel(type->taille_octet, [&]() {
        std::cerr << "Le type est " << chaine_type(type) << '\n';

        auto fonction = contexte.fonction;
        if (fonction) {
            std::cerr << "La fonction est " << nom_humainement_lisible(fonction) << '\n';
            std::cerr << *fonction << '\n';
        }

        erreur::imprime_site(*contexte.espace, site);
    });
#endif

    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_ADRESSAGE_ASSIGNE, site);
        émets(type->taille_octet);
    }

    émets_entête_op(OP_ASSIGNE, site);
    émets(type->taille_octet);
}

void Chunk::émets_assignation_locale(NoeudExpression const *site, int pointeur, Type const *type)
{
#if 0  // ndef CMAKE_BUILD_TYPE_PROFILE
    assert_rappel(type->taille_octet, [&]() {
        std::cerr << "Le type est " << chaine_type(type) << '\n';

        auto fonction = contexte.fonction;
        if (fonction) {
            std::cerr << "La fonction est " << nom_humainement_lisible(fonction) << '\n';
            std::cerr << *fonction << '\n';
        }

        erreur::imprime_site(*contexte.espace, site);
    });
#endif

    émets_entête_op(OP_ASSIGNE_LOCALE, site);
    émets(pointeur);
    émets(type->taille_octet);
}

void Chunk::émets_copie_locale(NoeudExpression const *site,
                               Type const *type,
                               int pointeur_source,
                               int pointeur_destination)
{
    émets_entête_op(OP_COPIE_LOCALE, site);
    émets(type->taille_octet);
    émets(pointeur_source);
    émets(pointeur_destination);
}

void Chunk::émets_charge(NoeudExpression const *site, Type const *type)
{
    assert(type->taille_octet);

    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_ADRESSAGE_CHARGE, site);
        émets(type->taille_octet);
    }

    émets_entête_op(OP_CHARGE, site);
    émets(type->taille_octet);
}

void Chunk::émets_charge_locale(NoeudExpression const *site, int pointeur, Type const *type)
{
    assert(type->taille_octet);
    émets_entête_op(OP_CHARGE_LOCALE, site);
    émets(pointeur);
    émets(type->taille_octet);
}

void Chunk::émets_référence_globale(NoeudExpression const *site, int pointeur)
{
    émets_entête_op(OP_REFERENCE_GLOBALE, site);
    émets(pointeur);
}

void Chunk::émets_référence_locale(NoeudExpression const *site, int pointeur)
{
    émets_entête_op(OP_RÉFÉRENCE_LOCALE, site);
    émets(pointeur);
}

void Chunk::émets_référence_membre(NoeudExpression const *site, unsigned décalage)
{
    émets_entête_op(OP_REFERENCE_MEMBRE, site);
    émets(décalage);
}

void Chunk::émets_référence_membre_locale(const NoeudExpression *site,
                                          int pointeur,
                                          uint32_t décalage)
{
    émets_entête_op(OP_RÉFÉRENCE_MEMBRE_LOCALE, site);
    émets(pointeur);
    émets(décalage);
}

void Chunk::émets_appel(NoeudExpression const *site,
                        AtomeFonction const *fonction,
                        unsigned taille_arguments)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_entête_op(OP_APPEL, site);
    émets(fonction);
    émets(taille_arguments);
}

void Chunk::émets_appel_externe(NoeudExpression const *site,
                                AtomeFonction const *fonction,
                                unsigned taille_arguments,
                                InstructionAppel const *inst_appel)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_entête_op(OP_APPEL_EXTERNE, site);
    émets(fonction);
    émets(taille_arguments);
    émets(inst_appel);
}

void Chunk::émets_appel_compilatrice(const NoeudExpression *site, const AtomeFonction *fonction)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_entête_op(OP_APPEL_COMPILATRICE, site);
    émets(fonction);
}

void Chunk::émets_appel_intrinsèque(NoeudExpression const *site, AtomeFonction const *fonction)
{
    émets_entête_op(OP_APPEL_INTRINSÈQUE, site);
    émets(fonction);
}

void Chunk::émets_appel_pointeur(NoeudExpression const *site,
                                 unsigned taille_arguments,
                                 InstructionAppel const *inst_appel)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(true); /* est pointeur */
    }

    émets_entête_op(OP_APPEL_POINTEUR, site);
    émets(taille_arguments);
    émets(inst_appel);
}

void Chunk::émets_accès_index(NoeudExpression const *site, Type const *type)
{
    assert(type->taille_octet);
    émets_entête_op(OP_ACCEDE_INDEX, site);
    émets(type->taille_octet);
}

void Chunk::émets_branche(NoeudExpression const *site,
                          kuri::tableau<PatchLabel> &patchs_labels,
                          int index)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_BRANCHE, nullptr);
        émets(0);
        patchs_labels.ajoute({index, static_cast<int>(compte - 4)});
    }

    émets_entête_op(OP_BRANCHE, site);
    émets(0);
    patchs_labels.ajoute({index, static_cast<int>(compte - 4)});
}

void Chunk::émets_branche_condition(NoeudExpression const *site,
                                    kuri::tableau<PatchLabel> &patchs_labels,
                                    int index_label_si_vrai,
                                    int index_label_si_faux)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION, nullptr);
        émets(0);
        patchs_labels.ajoute({index_label_si_vrai, static_cast<int>(compte - 4)});
        émets(0);
        patchs_labels.ajoute({index_label_si_faux, static_cast<int>(compte - 4)});
    }

    émets_entête_op(OP_BRANCHE_CONDITION, site);
    émets(0);
    patchs_labels.ajoute({index_label_si_vrai, static_cast<int>(compte - 4)});
    émets(0);
    patchs_labels.ajoute({index_label_si_faux, static_cast<int>(compte - 4)});
}

void Chunk::émets_operation_unaire(NoeudExpression const *site,
                                   OpérateurUnaire::Genre op,
                                   Type const *type)
{
    if (op == OpérateurUnaire::Genre::Complement) {
        if (type->est_type_reel()) {
            émets_entête_op(OP_COMPLEMENT_REEL, site);
        }
        else {
            émets_entête_op(OP_COMPLEMENT_ENTIER, site);
        }
    }
    else if (op == OpérateurUnaire::Genre::Non_Binaire) {
        émets_entête_op(OP_NON_BINAIRE, site);
    }

    if (type->est_type_entier_constant()) {
        émets(4);
    }
    else {
        émets(type->taille_octet);
    }
}

static octet_t converti_op_binaire(OpérateurBinaire::Genre genre)
{
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code)                                            \
    case OpérateurBinaire::Genre::genre:                                                          \
        return op_code;
    switch (genre) {
        ENUMERE_OPERATEURS_BINAIRE
    }
#undef ENUMERE_GENRE_OPBINAIRE_EX
    return static_cast<octet_t>(-1);
}

static std::optional<octet_t> converti_type_transtypage(TypeTranstypage genre)
{
    switch (genre) {
        case TypeTranstypage::BITS:
        case TypeTranstypage::DEFAUT:
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
        {
            break;
        }
        case TypeTranstypage::REEL_VERS_ENTIER:
        {
            return OP_REEL_VERS_ENTIER;
        }
        case TypeTranstypage::ENTIER_VERS_REEL:
        {
            return OP_ENTIER_VERS_REEL;
        }
        case TypeTranstypage::AUGMENTE_REEL:
        {
            return OP_AUGMENTE_REEL;
        }
        case TypeTranstypage::AUGMENTE_NATUREL:
        {
            return OP_AUGMENTE_NATUREL;
        }
        case TypeTranstypage::AUGMENTE_RELATIF:
        {
            return OP_AUGMENTE_RELATIF;
        }
        case TypeTranstypage::DIMINUE_REEL:
        {
            return OP_DIMINUE_REEL;
        }
        case TypeTranstypage::DIMINUE_NATUREL:
        {
            return OP_DIMINUE_NATUREL;
        }
        case TypeTranstypage::DIMINUE_RELATIF:
        {
            return OP_DIMINUE_RELATIF;
        }
    }

    return {};
}

void Chunk::émets_operation_binaire(NoeudExpression const *site,
                                    OpérateurBinaire::Genre op,
                                    Type const *type_gauche,
                                    Type const *type_droite)
{
    auto op_comp = converti_op_binaire(op);
    émets_entête_op(op_comp, site);

    auto taille_octet = std::max(type_gauche->taille_octet, type_droite->taille_octet);
    if (taille_octet == 0) {
        assert(type_gauche->est_type_entier_constant() && type_droite->est_type_entier_constant());
        émets(4);
    }
    else {
        émets(taille_octet);
    }
}

void Chunk::émets_incrémente(const NoeudExpression *site, const Type *type)
{
    auto taille_octet = type->taille_octet;
    if (type->est_type_entier_constant()) {
        taille_octet = 4;
    }
    émets_entête_op(OP_INCRÉMENTE, site);
    émets(taille_octet);
}

void Chunk::émets_incrémente_locale(const NoeudExpression *site, const Type *type, int pointeur)
{
    auto taille_octet = type->taille_octet;
    if (type->est_type_entier_constant()) {
        taille_octet = 4;
    }
    émets_entête_op(OP_INCRÉMENTE_LOCALE, site);
    émets(taille_octet);
    émets(pointeur);
}

void Chunk::émets_décrémente(const NoeudExpression *site, const Type *type)
{
    auto taille_octet = type->taille_octet;
    if (type->est_type_entier_constant()) {
        taille_octet = 4;
    }
    émets_entête_op(OP_DÉCRÉMENTE, site);
    émets(taille_octet);
}

void Chunk::émets_transtype(const NoeudExpression *site,
                            uint8_t op,
                            uint32_t taille_source,
                            uint32_t taille_dest)
{
    émets_entête_op(op, site);
    émets(taille_source);
    émets(taille_dest);
}

void Chunk::émets_rembourrage(uint32_t rembourrage)
{
    émets_entête_op(OP_REMBOURRAGE, nullptr);
    émets(rembourrage);
}

/* ************************************************************************** */

static int64_t instruction_simple(kuri::chaine_statique nom, int64_t décalage, Enchaineuse &os)
{
    os << nom << '\n';
    return décalage + 1;
}

template <typename T>
static int64_t instruction_1d(Chunk const &chunk,
                              kuri::chaine_statique nom,
                              int64_t décalage,
                              Enchaineuse &os)
{
    décalage += 1;
    auto index = *reinterpret_cast<T *>(&chunk.code[décalage]);
    os << nom << ' ' << index << '\n';
    return décalage + static_cast<int64_t>(sizeof(T));
}

template <typename T1, typename T2>
static int64_t instruction_2d(Chunk const &chunk,
                              kuri::chaine_statique nom,
                              int64_t décalage,
                              Enchaineuse &os)
{
    décalage += 1;
    auto v1 = *reinterpret_cast<T1 *>(&chunk.code[décalage]);
    décalage += static_cast<int64_t>(sizeof(T1));
    auto v2 = *reinterpret_cast<T2 *>(&chunk.code[décalage]);
    os << nom << ' ' << v1 << ", " << v2 << "\n";
    return décalage + static_cast<int64_t>(sizeof(T2));
}

template <typename T1, typename T2, typename T3>
static int64_t instruction_3d(Chunk const &chunk,
                              kuri::chaine_statique nom,
                              int64_t décalage,
                              Enchaineuse &os)
{
    décalage += 1;
    auto v1 = *reinterpret_cast<T1 *>(&chunk.code[décalage]);
    décalage += static_cast<int64_t>(sizeof(T1));
    auto v2 = *reinterpret_cast<T2 *>(&chunk.code[décalage]);
    décalage += static_cast<int64_t>(sizeof(T2));
    auto v3 = *reinterpret_cast<T3 *>(&chunk.code[décalage]);
    os << nom << ' ' << v1 << ", " << v2 << ", " << v3 << "\n";
    return décalage + static_cast<int64_t>(sizeof(T3));
}

int64_t désassemble_instruction(Chunk const &chunk, int64_t décalage, Enchaineuse &os)
{
    os << std::setfill('0') << std::setw(4) << décalage << ' ';

    auto instruction = chunk.code[décalage];

    switch (instruction) {
        case OP_LOGUE_RETOUR:
        case OP_LOGUE_SORTIES:
        case OP_RETOURNE:
        case OP_VÉRIFIE_CIBLE_BRANCHE:
        case OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION:
        {
            return instruction_simple(chaine_code_operation(instruction), décalage, os);
        }
        case OP_CONSTANTE:
        {
            décalage += 1;
            auto drapeaux = chunk.code[décalage];
            décalage += 1;
            os << "OP_CONSTANTE " << ' ';

#define LIS_CONSTANTE(type)                                                                       \
    type v = *(reinterpret_cast<type *>(&chunk.code[décalage]));                                  \
    os << v;                                                                                      \
    décalage += (drapeaux >> 3);

            switch (drapeaux) {
                case CONSTANTE_ENTIER_RELATIF | BITS_8:
                {
                    LIS_CONSTANTE(char);
                    os << " z8";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_16:
                {
                    LIS_CONSTANTE(short);
                    os << " z16";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_32:
                {
                    LIS_CONSTANTE(int);
                    os << " z32";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_64:
                {
                    LIS_CONSTANTE(int64_t);
                    os << " z64";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_8:
                {
                    // erreur de compilation pour transtype inutile avec drapeaux stricts
                    os << static_cast<int64_t>(chunk.code[décalage]);
                    décalage += 1;
                    os << " n8";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_16:
                {
                    LIS_CONSTANTE(unsigned short);
                    os << " n16";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_32:
                {
                    LIS_CONSTANTE(uint32_t);
                    os << " n32";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_64:
                {
                    LIS_CONSTANTE(uint64_t);
                    os << " n64";
                    break;
                }
                case CONSTANTE_NOMBRE_REEL | BITS_32:
                {
                    LIS_CONSTANTE(float);
                    os << " r32";
                    break;
                }
                case CONSTANTE_NOMBRE_REEL | BITS_64:
                {
                    LIS_CONSTANTE(double);
                    os << " r64";
                    break;
                }
            }

#undef LIS_CONSTANTE
            os << '\n';
            return décalage;
        }
        case OP_CHAINE_CONSTANTE:
        {
            os << "OP_CHAINE_CONSTANTE\n";
            return décalage + 17;
        }
        case OP_AJOUTE:
        case OP_AJOUTE_REEL:
        case OP_SOUSTRAIT:
        case OP_SOUSTRAIT_REEL:
        case OP_MULTIPLIE:
        case OP_MULTIPLIE_REEL:
        case OP_DIVISE:
        case OP_DIVISE_RELATIF:
        case OP_DIVISE_REEL:
        case OP_RESTE_NATUREL:
        case OP_RESTE_RELATIF:
        case OP_COMP_EGAL:
        case OP_COMP_INEGAL:
        case OP_COMP_INF:
        case OP_COMP_INF_EGAL:
        case OP_COMP_SUP:
        case OP_COMP_SUP_EGAL:
        case OP_COMP_INF_NATUREL:
        case OP_COMP_INF_EGAL_NATUREL:
        case OP_COMP_SUP_NATUREL:
        case OP_COMP_SUP_EGAL_NATUREL:
        case OP_COMP_EGAL_REEL:
        case OP_COMP_INEGAL_REEL:
        case OP_COMP_INF_REEL:
        case OP_COMP_INF_EGAL_REEL:
        case OP_COMP_SUP_REEL:
        case OP_COMP_SUP_EGAL_REEL:
        case OP_ET_BINAIRE:
        case OP_OU_BINAIRE:
        case OP_OU_EXCLUSIF:
        case OP_DEC_GAUCHE:
        case OP_DEC_DROITE_ARITHM:
        case OP_DEC_DROITE_LOGIQUE:
        case OP_BRANCHE:
        case OP_ASSIGNE:
        case OP_CHARGE:
        case OP_REFERENCE_GLOBALE:
        case OP_RÉFÉRENCE_LOCALE:
        case OP_REFERENCE_MEMBRE:
        case OP_ACCEDE_INDEX:
        case OP_APPEL_POINTEUR:
        case OP_COMPLEMENT_REEL:
        case OP_COMPLEMENT_ENTIER:
        case OP_NON_BINAIRE:
        case OP_VÉRIFIE_ADRESSAGE_ASSIGNE:
        case OP_VÉRIFIE_ADRESSAGE_CHARGE:
        case OP_LOGUE_INSTRUCTION:
        case OP_INCRÉMENTE:
        case OP_DÉCRÉMENTE:
        case OP_REMBOURRAGE:
        {
            return instruction_1d<int>(chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_STAT_INSTRUCTION:
        {
            décalage += 1;
            auto op = chunk.code[décalage];
            os << chaine_code_operation(instruction) << ' ' << chaine_code_operation(op) << '\n';
            return décalage + 1;
        }
        case OP_ASSIGNE_LOCALE:
        case OP_CHARGE_LOCALE:
        case OP_BRANCHE_CONDITION:
        case OP_INCRÉMENTE_LOCALE:
        case OP_RÉFÉRENCE_MEMBRE_LOCALE:
        {
            return instruction_2d<int, int>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_VÉRIFIE_CIBLE_APPEL:
        {
            return instruction_2d<int, int64_t>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_APPEL:
        case OP_APPEL_EXTERNE:
        case OP_APPEL_INTRINSÈQUE:
        case OP_APPEL_COMPILATRICE:
        {
            return instruction_3d<void *, int, void *>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_COPIE_LOCALE:
        {
            return instruction_3d<int, int, int>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_AUGMENTE_NATUREL:
        case OP_DIMINUE_NATUREL:
        case OP_AUGMENTE_RELATIF:
        case OP_DIMINUE_RELATIF:
        case OP_AUGMENTE_REEL:
        case OP_DIMINUE_REEL:
        case OP_ENTIER_VERS_REEL:
        case OP_REEL_VERS_ENTIER:
        {
            return instruction_2d<int, int>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_LOGUE_APPEL:
        {
            return instruction_1d<void *>(chunk, chaine_code_operation(instruction), décalage, os);
        }
        case OP_LOGUE_ENTRÉES:
        {
            return instruction_2d<void *, uint32_t>(
                chunk, chaine_code_operation(instruction), décalage, os);
        }
        default:
        {
            os << "Code Opération inconnu : " << instruction << '\n';
            return décalage + 1;
        }
    }
}

static void désassemble(const Chunk &chunk, kuri::chaine_statique nom, Enchaineuse &os)
{
    os << "== " << nom << " ==\n";
    for (auto décalage = int64_t(0); décalage < chunk.compte;) {
        décalage = désassemble_instruction(chunk, décalage, os);
    }
}

void désassemble(const Chunk &chunk, kuri::chaine_statique nom, std::ostream &os)
{
    Enchaineuse enchaineuse;
    désassemble(chunk, nom, enchaineuse);
    os << enchaineuse.chaine();
}

ffi_type *converti_type_ffi(Type const *type)
{
    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        {
            assert_rappel(false, [&]() { dbg() << "Type polymorphique dans la conversion FFI"; });
            return static_cast<ffi_type *>(nullptr);
        }
        case GenreType::TUPLE:
        {
            assert_rappel(false, [&]() { dbg() << "Type tuple dans la conversion FFI"; });
            return static_cast<ffi_type *>(nullptr);
        }
        case GenreType::BOOL:
        case GenreType::OCTET:
        {
            return &ffi_type_uint8;
        }
        case GenreType::CHAINE:
        {
            static ffi_type *types_elements[] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};

            static ffi_type type_ffi_chaine = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_chaine;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            static ffi_type *types_elements[] = {
                &ffi_type_pointer, &ffi_type_sint64, &ffi_type_sint64, nullptr};

            static ffi_type type_ffi_tableau = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_tableau;
        }
        case GenreType::EINI:
        {
            static ffi_type *types_elements[] = {&ffi_type_pointer, &ffi_type_pointer, nullptr};

            static ffi_type type_ffi_eini = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_eini;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            return &ffi_type_sint32;
        }
        case GenreType::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                return &ffi_type_uint8;
            }

            if (type->taille_octet == 2) {
                return &ffi_type_uint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_uint32;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_uint64;
            }

            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                return &ffi_type_sint8;
            }

            if (type->taille_octet == 2) {
                return &ffi_type_sint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_sint32;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_sint64;
            }

            break;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                return &ffi_type_uint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_float;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_double;
            }

            break;
        }
        case GenreType::RIEN:
        {
            return &ffi_type_void;
        }
        case GenreType::POINTEUR:
        case GenreType::REFERENCE:
        case GenreType::FONCTION:
        {
            return &ffi_type_pointer;
        }
        case GenreType::STRUCTURE:
        {
            // non supporté pour le moment, nous devrions uniquement passer des pointeurs
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            return converti_type_ffi(type_opaque->type_opacifie);
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_type_union();

            if (type_union->est_nonsure) {
                return converti_type_ffi(type_union->type_le_plus_grand);
            }

            // non supporté
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            return converti_type_ffi(static_cast<TypeEnum const *>(type)->type_sous_jacent);
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            return &ffi_type_sint64;
        }
        case GenreType::VARIADIQUE:
        case GenreType::TABLEAU_FIXE:
        {
            // ces types là ne sont pas supporté dans FFI
            break;
        }
    }

    return static_cast<ffi_type *>(nullptr);
}

/* ************************************************************************** */

CompilatriceCodeBinaire::CompilatriceCodeBinaire(EspaceDeTravail *espace_,
                                                 MetaProgramme *metaprogramme_)
    : espace(espace_), données_exécutions(&espace_->compilatrice().données_constantes_exécutions),
      métaprogramme(metaprogramme_)
{
    vérifie_adresses = espace->compilatrice().arguments.debogue_execution;
    émets_stats_ops = espace->compilatrice().arguments.émets_stats_ops_exécution;
}

bool CompilatriceCodeBinaire::génère_code(kuri::tableau_statique<AtomeGlobale *> globales,
                                          kuri::tableau_statique<AtomeFonction *> fonctions)
{
    kuri::tableau<AtomeGlobale *> globales_requérant_génération_code;

    POUR (globales) {
        if (it->index != -1) {
            continue;
        }

        if (!ajoute_globale(it)) {
            return false;
        }

        globales_requérant_génération_code.ajoute(it);
    }

    POUR (globales_requérant_génération_code) {
        génère_code_pour_globale(it);
    }

    POUR (fonctions) {
        /* Évite de recréer le code binaire. */
        if (it->données_exécution) {
            continue;
        }

        it->données_exécution = memoire::loge<DonnéesExécutionFonction>(
            "DonnéesExécutionFonction");
        fonction_courante = it->decl;

        if (!génère_code_pour_fonction(it)) {
            return false;
        }

        /* Les fonction d'initialisation de globales n'ont pas de déclarations. */
        if (it->decl) {
            if (it->decl->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_CODE_BINAIRE_FUT_REQUIS)) {
                désassemble(it->données_exécution->chunk, it->nom, std::cerr);
            }
        }
    }

    return true;
}

bool CompilatriceCodeBinaire::génère_code_pour_fonction(AtomeFonction const *fonction)
{
    auto données_exécution = fonction->données_exécution;

    /* Certains AtomeFonction créés par la compilatrice n'ont pas de déclaration. */
    if (fonction->decl && fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        if (fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
            return true;
        }

        auto &donnees_externe = données_exécution->données_externe;
        auto decl = fonction->decl;

        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
            donnees_externe.ptr_fonction = fonction_compilatrice_pour_ident(decl->ident);
        }
        else {
            /* Nous ne pouvons appeler une fonction prenant un pointeur de fonction car le pointeur
             * pourrait être une fonction interne dont l'adresse ne sera pas celle d'une fonction
             * exécutable (pour le système d'exploitation) mais l'adresse de l'AtomeFonction
             * correspondant qui est utilisée dans la machine virtuelle. */
            POUR (decl->params) {
                if (it->type->est_type_fonction()) {
                    espace->rapporte_erreur(fonction->decl,
                                            "Impossible d'appeler dans un métaprogramme une "
                                            "fonction externe utilisant un pointeur de fonction");
                    return false;
                }
            }

            if (!decl->symbole->charge(
                    espace, decl, RaisonRechercheSymbole::EXECUTION_METAPROGRAMME)) {
                return false;
            }

            donnees_externe.ptr_fonction = decl->symbole->donne_adresse_fonction_pour_exécution();
        }

        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE)) {
            /* Les fonctions variadiques doivent être préparées pour chaque appel. */
            return true;
        }

        auto type_fonction = fonction->type->comme_type_fonction();
        donnees_externe.types_entrées.reserve(type_fonction->types_entrees.taille());

        POUR (type_fonction->types_entrees) {
            donnees_externe.types_entrées.ajoute(converti_type_ffi(it));
        }

        auto type_ffi_sortie = converti_type_ffi(type_fonction->type_sortie);
        auto nombre_arguments = static_cast<unsigned>(donnees_externe.types_entrées.taille());
        auto ptr_types_entrees = donnees_externe.types_entrées.donnees();

        auto status = ffi_prep_cif(&donnees_externe.cif,
                                   FFI_DEFAULT_ABI,
                                   nombre_arguments,
                                   type_ffi_sortie,
                                   ptr_types_entrees);

        if (status != FFI_OK) {
            espace->rapporte_erreur(
                decl, "Impossible de préparer l'interface d'appel forrain pour la fonction");
            return false;
        }

        return true;
    }

    auto &chunk = données_exécution->chunk;
    chunk.émets_stats_ops = émets_stats_ops;
    chunk.émets_vérification_branches = vérifie_adresses;

    m_index_locales.redimensionne(fonction->nombre_d_instructions_avec_entrées_sorties());
    numérote_instructions(*fonction);

    POUR (fonction->params_entrees) {
        m_index_locales[it->numero] = chunk.ajoute_locale(it);
    }

    /* crée une variable local pour la valeur de sortie */
    if (fonction->param_sortie) {
        auto alloc = fonction->param_sortie;
        auto type_pointe = alloc->type->comme_type_pointeur()->type_pointe;

        if (!type_pointe->est_type_rien()) {
            m_index_locales[alloc->numero] = chunk.ajoute_locale(alloc);
        }
    }

    POUR (fonction->instructions) {
        if (it->est_alloc()) {
            m_index_locales[it->numero] = chunk.ajoute_locale(it->comme_alloc());
        }
    }

    POUR (fonction->instructions) {
        // génère le code binaire depuis les instructions « racines » (assignation, retour,
        // appel, et controle de flux).
        auto est_inst_racine = dls::outils::est_element(it->genre,
                                                        GenreInstruction::APPEL,
                                                        GenreInstruction::BRANCHE,
                                                        GenreInstruction::BRANCHE_CONDITION,
                                                        GenreInstruction::LABEL,
                                                        GenreInstruction::RETOUR,
                                                        GenreInstruction::STOCKE_MEMOIRE);

        if (!est_inst_racine) {
            continue;
        }

        génère_code_pour_instruction(it, chunk, false);
    }

    POUR (patchs_labels) {
        auto décalage = décalages_labels[it.index_label];
        *reinterpret_cast<int *>(&chunk.code[it.adresse]) = décalage;
    }

    /* Réinitialise à la fin pour ne pas polluer les données pour les autres fonctions. */
    décalages_labels.efface();
    patchs_labels.efface();
    m_index_locales.efface();

    chunk.rétrécis_capacité_sur_taille();

    return true;
}

void CompilatriceCodeBinaire::génère_code_pour_instruction(Instruction const *instruction,
                                                           Chunk &chunk,
                                                           bool pour_operande)
{
    switch (instruction->genre) {
        case GenreInstruction::INVALIDE:
        {
            return;
        }
        case GenreInstruction::LABEL:
        {
            auto label = instruction->comme_label();
            if (décalages_labels.taille() <= label->id) {
                décalages_labels.redimensionne(label->id + 1);
            }

            décalages_labels[label->id] = static_cast<int>(chunk.compte);
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto branche = instruction->comme_branche();
            chunk.émets_branche(branche->site, patchs_labels, branche->label->id);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto branche = instruction->comme_branche_cond();
            génère_code_pour_atome(branche->condition, chunk);
            chunk.émets_branche_condition(branche->site,
                                          patchs_labels,
                                          branche->label_si_vrai->id,
                                          branche->label_si_faux->id);
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            auto alloc = instruction->comme_alloc();
            assert(pour_operande);
            chunk.émets_référence_locale(alloc->site, donne_index_locale(alloc));
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto charge = instruction->comme_charge();

            if (est_allocation(charge->chargee)) {
                auto alloc = charge->chargee->comme_instruction()->comme_alloc();
                chunk.émets_charge_locale(charge->site, donne_index_locale(alloc), charge->type);
                break;
            }

            génère_code_pour_atome(charge->chargee, chunk);
            chunk.émets_charge(charge->site, charge->type);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto stocke = instruction->comme_stocke_mem();

            if (est_stocke_alloc_incrémente(stocke)) {
                auto alloc_destination = static_cast<InstructionAllocation const *>(stocke->ou);
                chunk.émets_incrémente_locale(
                    stocke->site, stocke->valeur->type, donne_index_locale(alloc_destination));
                break;
            }

            if (auto alloc_source = est_stocke_alloc_depuis_charge_alloc(stocke)) {
                auto alloc_destination = static_cast<InstructionAllocation const *>(stocke->ou);
                chunk.émets_copie_locale(stocke->site,
                                         stocke->valeur->type,
                                         donne_index_locale(alloc_source),
                                         donne_index_locale(alloc_destination));
                break;
            }

            génère_code_pour_atome(stocke->valeur, chunk);

            if (est_allocation(stocke->ou)) {
                auto alloc = stocke->ou->comme_instruction()->comme_alloc();
                chunk.émets_assignation_locale(
                    stocke->site, donne_index_locale(alloc), stocke->valeur->type);
                break;
            }

            // l'adresse de la valeur doit être au sommet de la pile lors de l'assignation
            génère_code_pour_atome(stocke->ou, chunk);
            chunk.émets_assignation(contexte(), stocke->site, stocke->valeur->type);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto appel = instruction->comme_appel();

            /* Évite de générer deux fois le code pour les appels : une fois dans la boucle sur les
             * instructions, une fois pour l'opérande. Les fonctions retournant « rien » ne peuvent
             * être opérandes. */
            if (!appel->type->est_type_rien() && !pour_operande) {
                return;
            }

            auto appelee = appel->appele;
            auto taille_arguments = 0u;

            POUR (appel->args) {
                génère_code_pour_atome(it, chunk);

                if (it->type->est_type_entier_constant()) {
                    taille_arguments += 4;
                }
                else {
                    taille_arguments += it->type->taille_octet;
                }
            }

            if (appelee->genre_atome == Atome::Genre::FONCTION) {
                auto atome_appelee = appelee->comme_fonction();

                if (atome_appelee->decl &&
                    atome_appelee->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
                    chunk.émets_appel_intrinsèque(appel->site, atome_appelee);
                }
                else if (atome_appelee->decl && atome_appelee->decl->possède_drapeau(
                                                    DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
                    chunk.émets_appel_compilatrice(appel->site, atome_appelee);
                }
                else if (atome_appelee->est_externe) {
                    chunk.émets_appel_externe(appel->site, atome_appelee, taille_arguments, appel);
                }
                else {
                    chunk.émets_appel(appel->site, atome_appelee, taille_arguments);
                }
            }
            else {
                génère_code_pour_atome(appelee, chunk);
                chunk.émets_appel_pointeur(appel->site, taille_arguments, appel);
            }

            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = instruction->comme_retour();

            if (retour->valeur) {
                génère_code_pour_atome(retour->valeur, chunk);
            }

            chunk.émets_retour(retour->site);
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto transtype = instruction->comme_transtype();
            auto valeur = transtype->valeur;

            génère_code_pour_atome(valeur, chunk);

            auto opt_op_transtype = converti_type_transtypage(transtype->op);
            if (opt_op_transtype.has_value()) {
                auto op_transtype = opt_op_transtype.value();
                if (op_transtype == OP_AUGMENTE_REEL) {
                    chunk.émets_transtype(transtype->site, op_transtype, 4, 8);
                }
                else if (op_transtype == OP_DIMINUE_REEL) {
                    chunk.émets_transtype(transtype->site, op_transtype, 8, 4);
                }
                else {
                    chunk.émets_transtype(transtype->site,
                                          op_transtype,
                                          valeur->type->taille_octet,
                                          transtype->type->taille_octet);
                }
            }

            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto index = instruction->comme_acces_index();
            auto type_pointeur = index->type->comme_type_pointeur();
            génère_code_pour_atome(index->index, chunk);
            génère_code_pour_atome(index->accede, chunk);

            if (index->accede->genre_atome == Atome::Genre::INSTRUCTION) {
                auto accede = index->accede->comme_instruction();
                auto type_accede = accede->type->comme_type_pointeur()->type_pointe;

                // l'accédé est le pointeur vers le pointeur, donc déréférence-le
                if (type_accede->est_type_pointeur()) {
                    chunk.émets_charge(index->site, type_pointeur);
                }
            }

            chunk.émets_accès_index(index->site, type_pointeur->type_pointe);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto membre = instruction->comme_acces_membre();
            auto index_membre = membre->index;

            auto type_compose = static_cast<TypeCompose *>(
                type_dereference_pour(membre->accede->type));

            if (type_compose->est_type_union()) {
                type_compose = type_compose->comme_type_union()->type_structure;
            }

            auto décalage = type_compose->membres[index_membre].decalage;

            if (est_allocation(membre->accede)) {
                auto alloc = membre->accede->comme_instruction()->comme_alloc();
                chunk.émets_référence_membre_locale(
                    membre->site, donne_index_locale(alloc), décalage);
                break;
            }

            génère_code_pour_atome(membre->accede, chunk);
            chunk.émets_référence_membre(membre->site, décalage);

            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto op_unaire = instruction->comme_op_unaire();
            auto type = op_unaire->valeur->type;
            génère_code_pour_atome(op_unaire->valeur, chunk);
            chunk.émets_operation_unaire(op_unaire->site, op_unaire->op, type);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto op_binaire = instruction->comme_op_binaire();
            auto type_gauche = op_binaire->valeur_gauche->type;

            génère_code_pour_atome(op_binaire->valeur_gauche, chunk);

            if (op_binaire->op == OpérateurBinaire::Genre::Addition &&
                est_constante_entière_un(op_binaire->valeur_droite)) {
                chunk.émets_incrémente(op_binaire->site, type_gauche);
                break;
            }
            if (op_binaire->op == OpérateurBinaire::Genre::Soustraction &&
                est_constante_entière_un(op_binaire->valeur_droite)) {
                chunk.émets_décrémente(op_binaire->site, type_gauche);
                break;
            }

            génère_code_pour_atome(op_binaire->valeur_droite, chunk);

            auto type_droite = op_binaire->valeur_droite->type;
            chunk.émets_operation_binaire(
                op_binaire->site, op_binaire->op, type_gauche, type_droite);

            break;
        }
    }
}

void CompilatriceCodeBinaire::génère_code_pour_initialisation_globale(
    AtomeConstante const *constante, int décalage, int ou_patcher) const
{
    unsigned char *donnees = nullptr;
    if (ou_patcher == DONNÉES_GLOBALES) {
        donnees = données_exécutions->données_globales.donnees() + décalage;
    }
    else {
        donnees = données_exécutions->données_constantes.donnees() + décalage;
    }

    switch (constante->genre_atome) {
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = constante->comme_transtype_constant();
            génère_code_pour_initialisation_globale(transtype->valeur, décalage, ou_patcher);
            break;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto indexage = constante->comme_accès_index_constant();
            auto indexée = indexage->accede->comme_globale();

            if (indexée->initialisateur->est_données_constantes()) {
                assert(indexage->index == 0);
                auto tableau = indexée->initialisateur->comme_données_constantes();
                auto données_tableau = tableau->donne_données();
                *reinterpret_cast<const char **>(donnees) = données_tableau.begin();
            }
            else if (indexée->initialisateur->est_constante_tableau()) {
                assert(indexage->index == 0);
                auto globale = données_exécutions->globales[indexée->index];

                auto patch = PatchDonnéesConstantes{};
                patch.destination = {ou_patcher, décalage};
                patch.source = {ADRESSE_GLOBALE, globale.adresse};
                données_exécutions->patchs_données_constantes.ajoute(patch);
            }
            else {
                assert_rappel(false, [&]() {
                    dbg() << "Indexage constant non-implémenté dans le code binaire pour le genre "
                             "d'atome "
                          << indexée->initialisateur->genre_atome;
                });
            }

            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            *reinterpret_cast<uint64_t *>(donnees) = 0;
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            // utilisation du pointeur directement au lieu de l'index car la table de type
            // n'est pas implémentée, et il y a des concurrences critiques entre les
            // métaprogrammes
            auto type = constante->comme_constante_type()->type_de_données;
            *reinterpret_cast<int64_t *>(donnees) = reinterpret_cast<int64_t>(type);
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = constante->comme_taille_de()->type_de_données;
            *reinterpret_cast<uint32_t *>(donnees) = type->taille_octet;
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = constante->comme_constante_réelle();
            auto type = constante->type;

            if (type->taille_octet == 4) {
                *reinterpret_cast<float *>(donnees) = static_cast<float>(constante_réelle->valeur);
            }
            else {
                *reinterpret_cast<double *>(donnees) = constante_réelle->valeur;
            }

            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = constante->comme_constante_entière();
            auto type = type_entier_sous_jacent(constante_entière->type);
            auto valeur_entiere = constante_entière->valeur;

            if (type->est_type_entier_naturel() || type->est_type_enum() ||
                type->est_type_erreur()) {
                if (type->taille_octet == 1) {
                    *donnees = static_cast<unsigned char>(valeur_entiere);
                }
                else if (type->taille_octet == 2) {
                    *reinterpret_cast<unsigned short *>(donnees) = static_cast<unsigned short>(
                        valeur_entiere);
                }
                else if (type->taille_octet == 4) {
                    *reinterpret_cast<uint32_t *>(donnees) = static_cast<uint32_t>(valeur_entiere);
                }
                else if (type->taille_octet == 8) {
                    *reinterpret_cast<uint64_t *>(donnees) = valeur_entiere;
                }
            }
            else if (type->est_type_entier_relatif()) {
                if (type->taille_octet == 1) {
                    *reinterpret_cast<char *>(donnees) = static_cast<char>(valeur_entiere);
                }
                else if (type->taille_octet == 2) {
                    *reinterpret_cast<short *>(donnees) = static_cast<short>(valeur_entiere);
                }
                else if (type->taille_octet == 4) {
                    *reinterpret_cast<int *>(donnees) = static_cast<int>(valeur_entiere);
                }
                else if (type->taille_octet == 8) {
                    *reinterpret_cast<int64_t *>(donnees) = static_cast<int64_t>(valeur_entiere);
                }
            }

            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = constante->comme_constante_booléenne();
            *reinterpret_cast<char *>(donnees) = static_cast<char>(constante_booléenne->valeur);
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = constante->comme_constante_caractère();
            *reinterpret_cast<char *>(donnees) = static_cast<char>(caractère->valeur);
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = constante->comme_constante_structure();
            auto type = structure->type->comme_type_compose();
            auto tableau_valeur = structure->donne_atomes_membres();

            auto index_membre = 0;
            for (auto i = 0; i < type->membres.taille(); ++i) {
                if (type->membres[i].ne_doit_pas_être_dans_code_machine()) {
                    continue;
                }

                /* Les tableaux fixes ont une initialisation nulle. */
                if (tableau_valeur[index_membre] == nullptr) {
                    index_membre += 1;
                    continue;
                }

                auto décalage_membre = type->membres[i].decalage;
                génère_code_pour_initialisation_globale(tableau_valeur[index_membre],
                                                        décalage +
                                                            static_cast<int>(décalage_membre),
                                                        ou_patcher);

                index_membre += 1;
            }

            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau_fixe = constante->comme_constante_tableau();
            auto type_tableau = constante->type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointe;

            auto décalage_élément = décalage;
            POUR (tableau_fixe->donne_atomes_éléments()) {
                génère_code_pour_initialisation_globale(it, décalage_élément, ou_patcher);
                décalage_élément += int(type_élément->taille_octet);
            }
            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            break;
        }
        case Atome::Genre::GLOBALE:
        {
            auto atome_globale = constante->comme_globale();
            auto globale = données_exécutions->globales[atome_globale->index];

            auto patch = PatchDonnéesConstantes{};
            patch.destination = {ou_patcher, décalage};
            patch.source = {ADRESSE_GLOBALE, globale.adresse};
            données_exécutions->patchs_données_constantes.ajoute(patch);

            break;
        }
        case Atome::Genre::FONCTION:
        {
            *reinterpret_cast<AtomeFonction const **>(donnees) = constante->comme_fonction();
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            assert_rappel(false, []() {
                dbg() << "Une instruction se retrouve dans une initialisation de globale\n";
            });
            break;
        }
    }
}

void CompilatriceCodeBinaire::génère_code_pour_atome(Atome const *atome, Chunk &chunk)
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto atome_globale = atome->comme_globale();
            chunk.émets_référence_globale(nullptr, atome_globale->index);
            break;
        }
        case Atome::Genre::FONCTION:
        {
            /* L'adresse pour les pointeurs de fonctions. */
            chunk.émets_constante(reinterpret_cast<int64_t>(atome));
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            génère_code_pour_instruction(atome->comme_instruction(), chunk, true);
            break;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = atome->comme_transtype_constant();
            génère_code_pour_atome(transtype->valeur, chunk);
            break;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto index_constant = atome->comme_accès_index_constant();
            auto type_pointeur = index_constant->type->comme_type_pointeur();
            chunk.émets_constante(index_constant->index);
            génère_code_pour_atome(index_constant->accede, chunk);
            chunk.émets_accès_index(nullptr, type_pointeur->type_pointe);
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            chunk.émets_constante(int64_t(0));
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            // utilisation du pointeur directement au lieu de l'index car la table de type
            // n'est pas implémentée, et il y a des concurrences critiques entre les
            // métaprogrammes
            auto type = atome->comme_constante_type()->type_de_données;
            chunk.émets_constante(reinterpret_cast<int64_t>(type));
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = atome->comme_taille_de()->type_de_données;
            chunk.émets_constante(type->taille_octet);
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto type = constante_réelle->type;

            if (type->taille_octet == 4) {
                chunk.émets_constante(static_cast<float>(constante_réelle->valeur));
            }
            else {
                chunk.émets_constante(constante_réelle->valeur);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            auto valeur_entiere = constante_entière->valeur;
            auto type = type_entier_sous_jacent(constante_entière->type);

            if (type->est_type_entier_naturel()) {
                if (type->taille_octet == 1) {
                    chunk.émets_constante(static_cast<unsigned char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.émets_constante(static_cast<unsigned short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.émets_constante(static_cast<uint32_t>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.émets_constante(valeur_entiere);
                }
            }
            else if (type->est_type_entier_relatif()) {
                if (type->taille_octet == 1) {
                    chunk.émets_constante(static_cast<char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.émets_constante(static_cast<short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.émets_constante(static_cast<int>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.émets_constante(static_cast<int64_t>(valeur_entiere));
                }
            }

            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            chunk.émets_constante(constante_booléenne->valeur);
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            chunk.émets_constante(static_cast<char>(caractère->valeur));
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = atome->comme_constante_structure();
            auto type = atome->type;
            auto tableau_valeur = structure->donne_atomes_membres();

            if (type->est_type_chaine()) {
                if (tableau_valeur[0]->est_constante_nulle()) {
                    /* Valeur nulle pour les chaines initilisées à zéro. */
                    chunk.émets_chaine_constante(/* site */ nullptr, nullptr, 0);
                    return;
                }

                auto acces_index = tableau_valeur[0]->comme_accès_index_constant();
                auto globale_tableau = acces_index->accede->comme_globale();
                auto tableau = globale_tableau->initialisateur->comme_données_constantes();
                auto données = tableau->donne_données();

                chunk.émets_chaine_constante(
                    /* site */ nullptr, données.begin(), données.taille());
                return;
            }

            /* À FAIRE(code binaire) : considère avoir OP_STRUCTURE_CONSTANTE et d'écrire
             * directement les données sans passer par des OP_CONSTANTES pour économiser de la
             * mémoire. Ceci ne sera possible que si les globales ont des adresses stables pour
             * pouvoir garantir que fonctions pointent vers la même globale pour chaque
             * métaprogramme. */
            auto type_composé = static_cast<TypeCompose const *>(type);

            auto index_membre = 0;
            POUR (type_composé->membres) {
                if (it.ne_doit_pas_être_dans_code_machine()) {
                    continue;
                }

                if (tableau_valeur[index_membre] == nullptr) {
                    /* À FAIRE(tableau fixe) : initialisation défaut. */
                    chunk.émets_rembourrage(it.type->taille_octet);
                    continue;
                }

                if (it.rembourrage != 0) {
                    chunk.émets_rembourrage(it.rembourrage);
                }

                génère_code_pour_atome(tableau_valeur[index_membre], chunk);

                index_membre += 1;
            }

            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau = atome->comme_constante_tableau();
            auto éléments = tableau->donne_atomes_éléments();

            POUR (éléments) {
                génère_code_pour_atome(it, chunk);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            break;
        }
    }
}

bool CompilatriceCodeBinaire::ajoute_globale(AtomeGlobale *globale) const
{
    if (globale->index != -1) {
        /* Déjà généré par un autre métaprogramme. */
        return true;
    }

    void *adresse_pour_exécution = nullptr;
    if (globale->est_info_type_de) {
        adresse_pour_exécution = globale->est_info_type_de->info_type;
    }
    else if (globale->decl && globale->decl->symbole) {
        auto decl = globale->decl;
        if (!decl->symbole->charge(
                espace, decl, RaisonRechercheSymbole::EXECUTION_METAPROGRAMME)) {
            return false;
        }

        adresse_pour_exécution = decl->symbole->donne_adresse_objet_pour_exécution();
    }

    auto type_globale = globale->type->comme_type_pointeur()->type_pointe;
    auto index = données_exécutions->ajoute_globale(
        type_globale, globale->ident, adresse_pour_exécution);
    globale->index = index;
    return true;
}

void CompilatriceCodeBinaire::génère_code_pour_globale(AtomeGlobale const *atome_globale) const
{
    auto index = atome_globale->index;

    if (atome_globale->est_constante && !atome_globale->est_info_type_de) {
        auto globale = données_exécutions->globales[index];
        auto initialisateur = atome_globale->initialisateur;
        génère_code_pour_initialisation_globale(initialisateur, globale.adresse, DONNÉES_GLOBALES);
    }
}

int CompilatriceCodeBinaire::donne_index_locale(const InstructionAllocation *alloc) const
{
    return m_index_locales[alloc->numero];
}

ContexteGénérationCodeBinaire CompilatriceCodeBinaire::contexte() const
{
    return {espace, fonction_courante};
}

int64_t DonnéesExécutionFonction::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += chunk.mémoire_utilisée();

    if (!données_externe.types_entrées.est_stocke_dans_classe()) {
        résultat += données_externe.types_entrées.capacite() * taille_de(ffi_type *);
    }

    return résultat;
}
