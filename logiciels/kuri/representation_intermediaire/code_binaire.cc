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
#include "compilation/operateurs.hh"

#include "parsage/outils_lexemes.hh"

const char *chaine_code_operation(octet_t code_operation)
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
    detruit();
}

void Chunk::initialise()
{
    code = nullptr;
    compte = 0;
    capacite = 0;
}

void Chunk::detruit()
{
    memoire::deloge_tableau("Chunk::code", code, capacite);
    initialise();
}

void Chunk::emets(octet_t o)
{
    agrandis_si_necessaire(1);
    code[compte] = o;
    compte += 1;
}

void Chunk::agrandis_si_necessaire(int64_t taille)
{
    if (capacite < compte + taille) {
        auto nouvelle_capacite = capacite < 8 ? 8 : capacite * 2;
        memoire::reloge_tableau("Chunk::code", code, capacite, nouvelle_capacite);
        capacite = nouvelle_capacite;
    }
}

int Chunk::emets_allocation(NoeudExpression const *site, Type const *type, IdentifiantCode *ident)
{
    // XXX - À FAIRE : normalise les entiers constants
    if (type->est_type_entier_constant()) {
        const_cast<Type *>(type)->taille_octet = 4;
    }
    assert(type->taille_octet);
    emets(OP_ALLOUE);
    emets(site);
    emets(type);
    emets(ident);

    auto decalage = taille_allouee;
    taille_allouee += static_cast<int>(type->taille_octet);
    return decalage;
}

void Chunk::emets_assignation(ContexteGenerationCodeBinaire contexte,
                              NoeudExpression const *site,
                              Type const *type,
                              bool ajoute_verification)
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

    if (ajoute_verification) {
        emets(OP_VERIFIE_ADRESSAGE_ASSIGNE);
        emets(site);
        emets(type->taille_octet);
    }

    emets(OP_ASSIGNE);
    emets(site);
    emets(type->taille_octet);
}

void Chunk::emets_charge(NoeudExpression const *site, Type const *type, bool ajoute_verification)
{
    assert(type->taille_octet);

    if (ajoute_verification) {
        emets(OP_VERIFIE_ADRESSAGE_CHARGE);
        emets(site);
        emets(type->taille_octet);
    }

    emets(OP_CHARGE);
    emets(site);
    emets(type->taille_octet);
}

void Chunk::emets_charge_variable(NoeudExpression const *site,
                                  int pointeur,
                                  Type const *type,
                                  bool ajoute_verification)
{
    assert(type->taille_octet);
    emets_reference_variable(site, pointeur);
    emets_charge(site, type, ajoute_verification);
}

void Chunk::emets_reference_globale(NoeudExpression const *site, int pointeur)
{
    emets(OP_REFERENCE_GLOBALE);
    emets(site);
    emets(pointeur);
}

void Chunk::emets_reference_variable(NoeudExpression const *site, int pointeur)
{
    emets(OP_REFERENCE_VARIABLE);
    emets(site);
    emets(pointeur);
}

void Chunk::emets_reference_membre(NoeudExpression const *site, unsigned decalage)
{
    emets(OP_REFERENCE_MEMBRE);
    emets(site);
    emets(decalage);
}

void Chunk::emets_appel(NoeudExpression const *site,
                        AtomeFonction const *fonction,
                        unsigned taille_arguments,
                        InstructionAppel const *inst_appel,
                        bool ajoute_verification)
{
    if (ajoute_verification) {
        emets(OP_VERIFIE_CIBLE_APPEL);
        emets(site);
        emets(false); /* est pointeur */
        emets(fonction);
    }

    emets(OP_APPEL);
    emets(site);
    emets(fonction);
    emets(taille_arguments);
    emets(inst_appel);
}

void Chunk::emets_appel_externe(NoeudExpression const *site,
                                AtomeFonction const *fonction,
                                unsigned taille_arguments,
                                InstructionAppel const *inst_appel,
                                bool ajoute_verification)
{
    if (ajoute_verification) {
        emets(OP_VERIFIE_CIBLE_APPEL);
        emets(site);
        emets(false); /* est pointeur */
        emets(fonction);
    }

    emets(OP_APPEL_EXTERNE);
    emets(site);
    emets(fonction);
    emets(taille_arguments);
    emets(inst_appel);
}

void Chunk::emets_appel_compilatrice(const NoeudExpression *site,
                                     const AtomeFonction *fonction,
                                     bool ajoute_verification)
{
    if (ajoute_verification) {
        emets(OP_VERIFIE_CIBLE_APPEL);
        emets(site);
        emets(false); /* est pointeur */
        emets(fonction);
    }

    emets(OP_APPEL_COMPILATRICE);
    emets(site);
    emets(fonction);
}

void Chunk::emets_appel_intrinsèque(NoeudExpression const *site, AtomeFonction const *fonction)
{
    emets(OP_APPEL_INTRINSÈQUE);
    emets(site);
    emets(fonction);
}

void Chunk::emets_appel_pointeur(NoeudExpression const *site,
                                 unsigned taille_arguments,
                                 InstructionAppel const *inst_appel,
                                 bool ajoute_verification)
{
    if (ajoute_verification) {
        emets(OP_VERIFIE_CIBLE_APPEL);
        emets(site);
        emets(true); /* est pointeur */
    }

    emets(OP_APPEL_POINTEUR);
    emets(site);
    emets(taille_arguments);
    emets(inst_appel);
}

void Chunk::emets_acces_index(NoeudExpression const *site, Type const *type)
{
    assert(type->taille_octet);
    emets(OP_ACCEDE_INDEX);
    emets(site);
    emets(type->taille_octet);
}

void Chunk::emets_branche(NoeudExpression const *site,
                          kuri::tableau<PatchLabel> &patchs_labels,
                          int index)
{
    emets(OP_BRANCHE);
    emets(site);
    emets(0);

    auto patch = PatchLabel();
    patch.index_label = index;
    patch.adresse = static_cast<int>(compte - 4);
    patchs_labels.ajoute(patch);
}

void Chunk::emets_branche_condition(NoeudExpression const *site,
                                    kuri::tableau<PatchLabel> &patchs_labels,
                                    int index_label_si_vrai,
                                    int index_label_si_faux)
{
    emets(OP_BRANCHE_CONDITION);
    emets(site);
    emets(0);
    emets(0);

    auto patch = PatchLabel();
    patch.index_label = index_label_si_vrai;
    patch.adresse = static_cast<int>(compte - 8);
    patchs_labels.ajoute(patch);

    patch = PatchLabel();
    patch.index_label = index_label_si_faux;
    patch.adresse = static_cast<int>(compte - 4);
    patchs_labels.ajoute(patch);
}

void Chunk::emets_label(NoeudExpression const *site, int index)
{
    if (decalages_labels.taille() <= index) {
        decalages_labels.redimensionne(index + 1);
    }

    decalages_labels[index] = static_cast<int>(compte);
    emets(OP_LABEL);
    emets(site);
    emets(index);
}

void Chunk::emets_operation_unaire(NoeudExpression const *site,
                                   OperateurUnaire::Genre op,
                                   Type const *type)
{
    if (op == OperateurUnaire::Genre::Complement) {
        if (type->est_type_reel()) {
            emets(OP_COMPLEMENT_REEL);
            emets(site);
        }
        else {
            emets(OP_COMPLEMENT_ENTIER);
            emets(site);
        }
    }
    else if (op == OperateurUnaire::Genre::Non_Binaire) {
        emets(OP_NON_BINAIRE);
        emets(site);
    }

    if (type->est_type_entier_constant()) {
        emets(4);
    }
    else {
        emets(type->taille_octet);
    }
}

static octet_t converti_op_binaire(OperateurBinaire::Genre genre)
{
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code)                                            \
    case OperateurBinaire::Genre::genre:                                                          \
        return op_code;
    switch (genre) {
        ENUMERE_OPERATEURS_BINAIRE
    }
#undef ENUMERE_GENRE_OPBINAIRE_EX
    return static_cast<octet_t>(-1);
}

void Chunk::emets_operation_binaire(NoeudExpression const *site,
                                    OperateurBinaire::Genre op,
                                    Type const *type_gauche,
                                    Type const *type_droite)
{
    auto op_comp = converti_op_binaire(op);
    emets(op_comp);
    emets(site);

    auto taille_octet = std::max(type_gauche->taille_octet, type_droite->taille_octet);
    if (taille_octet == 0) {
        assert(type_gauche->est_type_entier_constant() && type_droite->est_type_entier_constant());
        emets(4);
    }
    else {
        emets(taille_octet);
    }
}

/* ************************************************************************** */

static int64_t instruction_simple(const char *nom, int64_t decalage, std::ostream &os)
{
    os << nom << '\n';
    return decalage + 1;
}

template <typename T>
static int64_t instruction_1d(Chunk const &chunk,
                              const char *nom,
                              int64_t decalage,
                              std::ostream &os)
{
    decalage += 1;
    auto index = *reinterpret_cast<T *>(&chunk.code[decalage]);
    os << nom << ' ' << index << '\n';
    return decalage + static_cast<int64_t>(sizeof(T));
}

template <typename T1, typename T2>
static int64_t instruction_2d(Chunk const &chunk,
                              const char *nom,
                              int64_t decalage,
                              std::ostream &os)
{
    decalage += 1;
    auto v1 = *reinterpret_cast<T1 *>(&chunk.code[decalage]);
    decalage += static_cast<int64_t>(sizeof(T1));
    auto v2 = *reinterpret_cast<T2 *>(&chunk.code[decalage]);
    os << nom << ' ' << v1 << ", " << v2 << "\n";
    return decalage + static_cast<int64_t>(sizeof(T2));
}

template <typename T1, typename T2, typename T3>
static int64_t instruction_3d(Chunk const &chunk,
                              const char *nom,
                              int64_t decalage,
                              std::ostream &os)
{
    decalage += 1;
    auto v1 = *reinterpret_cast<T1 *>(&chunk.code[decalage]);
    decalage += static_cast<int64_t>(sizeof(T1));
    auto v2 = *reinterpret_cast<T2 *>(&chunk.code[decalage]);
    decalage += static_cast<int64_t>(sizeof(T2));
    auto v3 = *reinterpret_cast<T3 *>(&chunk.code[decalage]);
    os << nom << ' ' << v1 << ", " << v2 << ", " << v3 << "\n";
    return decalage + static_cast<int64_t>(sizeof(T3));
}

int64_t desassemble_instruction(Chunk const &chunk, int64_t decalage, std::ostream &os)
{
    os << std::setfill('0') << std::setw(4) << decalage << ' ';

    auto instruction = chunk.code[decalage];
    /* ignore le site */
    decalage += 8;

    switch (instruction) {
        case OP_RETOURNE:
        {
            return instruction_simple("OP_RETOURNE", decalage, os);
        }
        case OP_CONSTANTE:
        {
            decalage += 1;
            auto drapeaux = chunk.code[decalage];
            decalage += 1;
            os << "OP_CONSTANTE " << ' ';

#define LIS_CONSTANTE(type)                                                                       \
    type v = *(reinterpret_cast<type *>(&chunk.code[decalage]));                                  \
    os << v;                                                                                      \
    decalage += (drapeaux >> 3);

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
                    os << static_cast<int64_t>(chunk.code[decalage]);
                    decalage += 1;
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
            return decalage;
        }
        case OP_CHAINE_CONSTANTE:
        {
            os << "OP_CHAINE_CONSTANTE\n";
            return decalage + 17;
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
        case OP_LABEL:
        case OP_BRANCHE:
        case OP_ASSIGNE:
        case OP_CHARGE:
        case OP_REFERENCE_GLOBALE:
        case OP_REFERENCE_VARIABLE:
        case OP_REFERENCE_MEMBRE:
        case OP_ACCEDE_INDEX:
        case OP_APPEL_POINTEUR:
        case OP_COMPLEMENT_REEL:
        case OP_COMPLEMENT_ENTIER:
        case OP_NON_BINAIRE:
        case OP_VERIFIE_ADRESSAGE_ASSIGNE:
        case OP_VERIFIE_ADRESSAGE_CHARGE:
        {
            return instruction_1d<int>(chunk, chaine_code_operation(instruction), decalage, os);
        }
        case OP_BRANCHE_CONDITION:
        {
            return instruction_2d<int, int>(
                chunk, chaine_code_operation(instruction), decalage, os);
        }
        case OP_VERIFIE_CIBLE_APPEL:
        {
            return instruction_2d<int, int64_t>(
                chunk, chaine_code_operation(instruction), decalage, os);
        }
        case OP_ALLOUE:
        {
            decalage += 1;
            auto v1 = *reinterpret_cast<Type **>(&chunk.code[decalage]);
            decalage += static_cast<int64_t>(sizeof(Type *));
            auto v2 = *reinterpret_cast<IdentifiantCode **>(&chunk.code[decalage]);
            os << chaine_code_operation(instruction) << ' ' << chaine_type(v1) << ", " << v2
               << "\n";
            return decalage + static_cast<int64_t>(sizeof(IdentifiantCode *));
        }
        case OP_APPEL:
        case OP_APPEL_EXTERNE:
        case OP_APPEL_INTRINSÈQUE:
        case OP_APPEL_COMPILATRICE:
        {
            return instruction_3d<void *, int, void *>(
                chunk, chaine_code_operation(instruction), decalage, os);
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
                chunk, chaine_code_operation(instruction), decalage, os);
        }
        default:
        {
            os << "Code Opération inconnu : " << instruction << '\n';
            return decalage + 1;
        }
    }
}

void desassemble(const Chunk &chunk, kuri::chaine_statique nom, std::ostream &os)
{
    os << "== " << nom << " ==\n";
    for (auto decalage = int64_t(0); decalage < chunk.compte;) {
        decalage = desassemble_instruction(chunk, decalage, os);
    }
}

ffi_type *converti_type_ffi(Type const *type)
{
    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        {
            assert_rappel(false,
                          [&]() { std::cerr << "Type polymorphique dans la conversion FFI\n"; });
            return static_cast<ffi_type *>(nullptr);
        }
        case GenreType::TUPLE:
        {
            assert_rappel(false, [&]() { std::cerr << "Type tuple dans la conversion FFI\n"; });
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

ConvertisseuseRI::ConvertisseuseRI(EspaceDeTravail *espace_, MetaProgramme *metaprogramme_)
    : espace(espace_), donnees_executions(&espace_->compilatrice().donnees_constantes_executions),
      metaprogramme(metaprogramme_)
{
    verifie_adresses = espace->compilatrice().arguments.debogue_execution;
}

bool ConvertisseuseRI::genere_code(const kuri::tableau<AtomeFonction *> &fonctions)
{
    POUR (fonctions) {
        /* Évite de recréer le code binaire. */
        if (it->données_exécution) {
            continue;
        }

        it->données_exécution = memoire::loge<DonnéesExécutionFonction>(
            "DonnéesExécutionFonction");
        fonction_courante = it->decl;

        if (!genere_code_pour_fonction(it)) {
            return false;
        }
    }

    return true;
}

bool ConvertisseuseRI::genere_code_pour_fonction(AtomeFonction *fonction)
{
    auto données_exécution = fonction->données_exécution;

    /* Certains AtomeFonction créés par la compilatrice n'ont pas de déclaration. */
    if (fonction->decl && fonction->decl->possede_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        if (fonction->decl->possede_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
            return true;
        }

        auto &donnees_externe = données_exécution->donnees_externe;
        auto decl = fonction->decl;

        if (decl->possede_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
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

            donnees_externe.ptr_fonction = decl->symbole->adresse_pour_execution();
        }

        if (decl->possede_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE)) {
            /* Les fonctions variadiques doivent être préparées pour chaque appel. */
            return true;
        }

        auto type_fonction = fonction->type->comme_type_fonction();
        donnees_externe.types_entrees.reserve(type_fonction->types_entrees.taille());

        POUR (type_fonction->types_entrees) {
            donnees_externe.types_entrees.ajoute(converti_type_ffi(it));
        }

        auto type_ffi_sortie = converti_type_ffi(type_fonction->type_sortie);
        auto nombre_arguments = static_cast<unsigned>(donnees_externe.types_entrees.taille());
        auto ptr_types_entrees = donnees_externe.types_entrees.donnees();

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

    POUR (fonction->params_entrees) {
        auto alloc = it->comme_instruction()->comme_alloc();
        auto type_pointe = alloc->type->comme_type_pointeur()->type_pointe;
        auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
        alloc->index_locale = chunk.locales.taille();
        chunk.locales.ajoute({alloc->ident, alloc->type, adresse});
    }

    /* crée une variable local pour la valeur de sortie */
    if (fonction->param_sortie) {
        auto param = fonction->param_sortie;
        auto alloc = param->comme_instruction()->comme_alloc();
        auto type_pointe = alloc->type->comme_type_pointeur()->type_pointe;

        if (!type_pointe->est_type_rien()) {
            auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
            alloc->index_locale = chunk.locales.taille();
            chunk.locales.ajoute({alloc->ident, alloc->type, adresse});
        }
    }

#ifndef OPTIMISE_ALLOCS
    POUR (fonction->instructions) {
        if (it->est_alloc()) {
            auto alloc = it->comme_alloc();
            auto type_pointe = alloc->type->comme_type_pointeur()->type_pointe;
            auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
            alloc->index_locale = chunk.locales.taille();
            chunk.locales.ajoute({alloc->ident, alloc->type, adresse});
        }
    }
#endif

    POUR (fonction->instructions) {
        // génère le code binaire depuis les instructions « racines » (assignation, retour,
        // allocation, appel, et controle de flux).
        auto est_inst_racine = dls::outils::est_element(it->genre,
                                                        Instruction::Genre::ALLOCATION,
                                                        Instruction::Genre::APPEL,
                                                        Instruction::Genre::BRANCHE,
                                                        Instruction::Genre::BRANCHE_CONDITION,
                                                        Instruction::Genre::LABEL,
                                                        Instruction::Genre::RETOUR,
                                                        Instruction::Genre::STOCKE_MEMOIRE);

        if (!est_inst_racine) {
            continue;
        }

        genere_code_binaire_pour_instruction(it, chunk, false);
    }

    POUR (patchs_labels) {
        auto decalage = chunk.decalages_labels[it.index_label];
        *reinterpret_cast<int *>(&chunk.code[it.adresse]) = decalage;
    }

    /* Réinitialise à la fin pour ne pas polluer les données pour les autres fonctions. */
    patchs_labels.efface();

    // desassemble(chunk, fonction->nom.c_str(), std::cerr);
    return true;
}

void ConvertisseuseRI::genere_code_binaire_pour_instruction(Instruction const *instruction,
                                                            Chunk &chunk,
                                                            bool pour_operande)
{
    switch (instruction->genre) {
        case Instruction::Genre::INVALIDE:
        {
            return;
        }
        case Instruction::Genre::LABEL:
        {
            auto label = instruction->comme_label();
            chunk.emets_label(label->site, label->id);
            break;
        }
        case Instruction::Genre::BRANCHE:
        {
            auto branche = instruction->comme_branche();
            chunk.emets_branche(branche->site, patchs_labels, branche->label->id);
            break;
        }
        case Instruction::Genre::BRANCHE_CONDITION:
        {
            auto branche = instruction->comme_branche_cond();
            genere_code_binaire_pour_atome(branche->condition, chunk, true);
            chunk.emets_branche_condition(branche->site,
                                          patchs_labels,
                                          branche->label_si_vrai->id,
                                          branche->label_si_faux->id);
            break;
        }
        case Instruction::Genre::ALLOCATION:
        {
            auto alloc = instruction->comme_alloc();

            if (pour_operande) {
                chunk.emets_reference_variable(alloc->site, alloc->index_locale);
            }
#ifdef OPTIMISE_ALLOCS
            else {
                if (alloc->decalage_pile > dernier_decalage_pile) {
                    dernier_decalage_pile = alloc->decalage_pile;
                    pile_taille.empile(chunk.taille_allouee);
                }
                else if (alloc->decalage_pile < dernier_decalage_pile) {
                    dernier_decalage_pile = alloc->decalage_pile;
                    chunk.taille_allouee = pile_taille.depile();
                }

                auto type_pointe = alloc->type->comme_type_pointeur()->type_pointe;
                auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
                alloc->index_locale = static_cast<int>(chunk.locales.taille);
                chunk.locales.ajoute({alloc->ident, alloc->type, adresse});
            }
#endif

            break;
        }
        case Instruction::Genre::CHARGE_MEMOIRE:
        {
            auto charge = instruction->comme_charge();
            genere_code_binaire_pour_atome(charge->chargee, chunk, true);
            chunk.emets_charge(charge->site, charge->type, verifie_adresses);
            break;
        }
        case Instruction::Genre::STOCKE_MEMOIRE:
        {
            auto stocke = instruction->comme_stocke_mem();
            genere_code_binaire_pour_atome(stocke->valeur, chunk, true);
            // l'adresse de la valeur doit être au sommet de la pile lors de l'assignation
            genere_code_binaire_pour_atome(stocke->ou, chunk, true);
            chunk.emets_assignation(
                contexte(), stocke->site, stocke->valeur->type, verifie_adresses);
            break;
        }
        case Instruction::Genre::APPEL:
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
                genere_code_binaire_pour_atome(it, chunk, true);

                if (it->type->est_type_entier_constant()) {
                    taille_arguments += 4;
                }
                else {
                    taille_arguments += it->type->taille_octet;
                }
            }

            if (appelee->genre_atome == Atome::Genre::FONCTION) {
                auto atome_appelee = static_cast<AtomeFonction *>(appelee);

                if (atome_appelee->decl &&
                    atome_appelee->decl->possede_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
                    chunk.emets_appel_intrinsèque(appel->site, atome_appelee);
                }
                else if (atome_appelee->decl && atome_appelee->decl->possede_drapeau(
                                                    DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
                    chunk.emets_appel_compilatrice(appel->site, atome_appelee, verifie_adresses);
                }
                else if (atome_appelee->est_externe) {
                    chunk.emets_appel_externe(
                        appel->site, atome_appelee, taille_arguments, appel, verifie_adresses);
                }
                else {
                    chunk.emets_appel(
                        appel->site, atome_appelee, taille_arguments, appel, verifie_adresses);
                }
            }
            else {
                genere_code_binaire_pour_atome(appelee, chunk, true);
                chunk.emets_appel_pointeur(appel->site, taille_arguments, appel, verifie_adresses);
            }

            break;
        }
        case Instruction::Genre::RETOUR:
        {
            auto retour = instruction->comme_retour();

            if (retour->valeur) {
                genere_code_binaire_pour_atome(retour->valeur, chunk, true);
            }

            chunk.emets(OP_RETOURNE);
            chunk.emets(retour->site);
            break;
        }
        case Instruction::Genre::TRANSTYPE:
        {
            auto transtype = instruction->comme_transtype();
            auto valeur = transtype->valeur;

            genere_code_binaire_pour_atome(valeur, chunk, true);

            switch (transtype->op) {
                case TypeTranstypage::BITS:
                case TypeTranstypage::DEFAUT:
                case TypeTranstypage::POINTEUR_VERS_ENTIER:
                case TypeTranstypage::ENTIER_VERS_POINTEUR:
                {
                    break;
                }
                case TypeTranstypage::REEL_VERS_ENTIER:
                {
                    chunk.emets(OP_REEL_VERS_ENTIER);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
                case TypeTranstypage::ENTIER_VERS_REEL:
                {
                    chunk.emets(OP_ENTIER_VERS_REEL);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
                case TypeTranstypage::AUGMENTE_REEL:
                {
                    chunk.emets(OP_AUGMENTE_REEL);
                    chunk.emets(transtype->site);
                    chunk.emets(4);
                    chunk.emets(8);
                    break;
                }
                case TypeTranstypage::AUGMENTE_NATUREL:
                {
                    chunk.emets(OP_AUGMENTE_NATUREL);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
                case TypeTranstypage::AUGMENTE_RELATIF:
                {
                    chunk.emets(OP_AUGMENTE_RELATIF);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
                case TypeTranstypage::DIMINUE_REEL:
                {
                    chunk.emets(OP_DIMINUE_REEL);
                    chunk.emets(transtype->site);
                    chunk.emets(8);
                    chunk.emets(4);
                    break;
                }
                case TypeTranstypage::DIMINUE_NATUREL:
                {
                    chunk.emets(OP_DIMINUE_NATUREL);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
                case TypeTranstypage::DIMINUE_RELATIF:
                {
                    chunk.emets(OP_DIMINUE_RELATIF);
                    chunk.emets(transtype->site);
                    chunk.emets(valeur->type->taille_octet);
                    chunk.emets(transtype->type->taille_octet);
                    break;
                }
            }

            break;
        }
        case Instruction::Genre::ACCEDE_INDEX:
        {
            auto index = instruction->comme_acces_index();
            auto type_pointeur = index->type->comme_type_pointeur();
            genere_code_binaire_pour_atome(index->index, chunk, true);
            genere_code_binaire_pour_atome(index->accede, chunk, true);

            if (index->accede->genre_atome == Atome::Genre::INSTRUCTION) {
                auto accede = index->accede->comme_instruction();
                auto type_accede = accede->type->comme_type_pointeur()->type_pointe;

                // l'accédé est le pointeur vers le pointeur, donc déréférence-le
                if (type_accede->est_type_pointeur()) {
                    chunk.emets_charge(index->site, type_pointeur, verifie_adresses);
                }
            }

            chunk.emets_acces_index(index->site, type_pointeur->type_pointe);
            break;
        }
        case Instruction::Genre::ACCEDE_MEMBRE:
        {
            auto membre = instruction->comme_acces_membre();
            auto index_membre = static_cast<int>(
                static_cast<AtomeValeurConstante *>(membre->index)->valeur.valeur_entiere);

            auto type_compose = static_cast<TypeCompose *>(
                type_dereference_pour(membre->accede->type));

            if (type_compose->est_type_union()) {
                type_compose = type_compose->comme_type_union()->type_structure;
            }

            auto decalage = type_compose->membres[index_membre].decalage;
            genere_code_binaire_pour_atome(membre->accede, chunk, true);
            chunk.emets_reference_membre(membre->site, decalage);

            break;
        }
        case Instruction::Genre::OPERATION_UNAIRE:
        {
            auto op_unaire = instruction->comme_op_unaire();
            auto type = op_unaire->valeur->type;
            genere_code_binaire_pour_atome(op_unaire->valeur, chunk, true);
            chunk.emets_operation_unaire(op_unaire->site, op_unaire->op, type);
            break;
        }
        case Instruction::Genre::OPERATION_BINAIRE:
        {
            auto op_binaire = instruction->comme_op_binaire();

            genere_code_binaire_pour_atome(op_binaire->valeur_gauche, chunk, true);
            genere_code_binaire_pour_atome(op_binaire->valeur_droite, chunk, true);

            auto type_gauche = op_binaire->valeur_gauche->type;
            auto type_droite = op_binaire->valeur_droite->type;
            chunk.emets_operation_binaire(
                op_binaire->site, op_binaire->op, type_gauche, type_droite);

            break;
        }
    }
}

static Type const *type_entier_sous_jacent(Typeuse &typeuse, Type const *type)
{
    if (type->est_type_entier_constant()) {
        return TypeBase::Z32;
    }

    if (type->est_type_enum()) {
        return type->comme_type_enum()->type_sous_jacent;
    }

    if (type->est_type_erreur()) {
        return type->comme_type_erreur()->type_sous_jacent;
    }

    if (type->est_type_type_de_donnees()) {
        return TypeBase::Z64;
    }

    if (type->est_type_octet()) {
        return TypeBase::N8;
    }

    return type;
}

void ConvertisseuseRI::genere_code_binaire_pour_constante(AtomeConstante *constante, Chunk &chunk)
{
    switch (constante->genre) {
        case AtomeConstante::Genre::VALEUR:
        {
            auto valeur_constante = static_cast<AtomeValeurConstante const *>(constante);
            genere_code_binaire_pour_valeur_constante(valeur_constante, chunk);
            break;
        }
        case AtomeConstante::Genre::GLOBALE:
        {
            genere_code_binaire_pour_atome(constante, chunk, true);
            break;
        }
        case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = static_cast<TranstypeConstant const *>(constante);
            genere_code_binaire_pour_constante(transtype->valeur, chunk);
            break;
        }
        case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
        {
            auto op_unaire = static_cast<OpUnaireConstant const *>(constante);
            genere_code_binaire_pour_constante(op_unaire->operande, chunk);
            chunk.emets_operation_unaire(nullptr, op_unaire->op, op_unaire->type);
            break;
        }
        case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
        {
            auto op_binaire = static_cast<OpBinaireConstant const *>(constante);
            genere_code_binaire_pour_constante(op_binaire->operande_gauche, chunk);
            genere_code_binaire_pour_constante(op_binaire->operande_droite, chunk);
            chunk.emets_operation_binaire(nullptr,
                                          op_binaire->op,
                                          op_binaire->operande_gauche->type,
                                          op_binaire->operande_droite->type);
            break;
        }
        case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
        {
            auto index_constant = static_cast<AccedeIndexConstant const *>(constante);
            auto type_pointeur = index_constant->type->comme_type_pointeur();
            genere_code_binaire_pour_constante(index_constant->index, chunk);
            genere_code_binaire_pour_constante(index_constant->accede, chunk);
            chunk.emets_acces_index(nullptr, type_pointeur->type_pointe);
            break;
        }
    }
}

void ConvertisseuseRI::genere_code_binaire_pour_valeur_constante(
    AtomeValeurConstante const *valeur_constante, Chunk &chunk)
{
    switch (valeur_constante->valeur.genre) {
        case AtomeValeurConstante::Valeur::Genre::NULLE:
        {
            chunk.emets_constante(int64_t(0));
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::TYPE:
        {
            // utilisation du pointeur directement au lieu de l'index car la table de type
            // n'est pas implémentée, et il y a des concurrences critiques entre les
            // métaprogrammes
            chunk.emets_constante(reinterpret_cast<int64_t>(valeur_constante->valeur.type));
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::TAILLE_DE:
        {
            chunk.emets_constante(valeur_constante->valeur.type->taille_octet);
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::ENTIERE:
        {
            auto valeur_entiere = valeur_constante->valeur.valeur_entiere;
            auto type = type_entier_sous_jacent(espace->compilatrice().typeuse,
                                                valeur_constante->type);

            if (type->est_type_entier_naturel()) {
                if (type->taille_octet == 1) {
                    chunk.emets_constante(static_cast<unsigned char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.emets_constante(static_cast<unsigned short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.emets_constante(static_cast<uint32_t>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.emets_constante(valeur_entiere);
                }
            }
            else if (type->est_type_entier_relatif()) {
                if (type->taille_octet == 1) {
                    chunk.emets_constante(static_cast<char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.emets_constante(static_cast<short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.emets_constante(static_cast<int>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.emets_constante(static_cast<int64_t>(valeur_entiere));
                }
            }
            else if (type->est_type_reel()) {
                if (type->taille_octet == 4) {
                    chunk.emets_constante(static_cast<float>(valeur_entiere));
                }
                else {
                    chunk.emets_constante(static_cast<double>(valeur_entiere));
                }
            }

            break;
        }
        case AtomeValeurConstante::Valeur::Genre::REELLE:
        {
            auto valeur_reele = valeur_constante->valeur.valeur_reelle;
            auto type = valeur_constante->type;

            if (type->taille_octet == 4) {
                chunk.emets_constante(static_cast<float>(valeur_reele));
            }
            else {
                chunk.emets_constante(valeur_reele);
            }

            break;
        }
        case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
        {
            AtomeConstante **pointeur = valeur_constante->valeur.valeur_tableau.pointeur;
            const int64_t taille = valeur_constante->valeur.valeur_tableau.taille;

            for (auto i = 0; i < taille; i++) {
                genere_code_binaire_pour_constante(pointeur[i], chunk);
            }

            break;
        }
        case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
        {
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
        {
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
        {
            auto valeur_bool = valeur_constante->valeur.valeur_booleenne;
            chunk.emets_constante(valeur_bool);
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::CARACTERE:
        {
            auto valeur_caractere = valeur_constante->valeur.valeur_entiere;
            chunk.emets_constante(static_cast<char>(valeur_caractere));
            break;
        }
        case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
        {
            auto type = valeur_constante->type;
            auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

            if (type->est_type_chaine()) {
                if (tableau_valeur[0]->genre == AtomeConstante::Genre::VALEUR) {
                    // valeur nulle pour les chaines initilisées à zéro
                    chunk.emets(OP_CHAINE_CONSTANTE);
                    chunk.emets(nullptr); /* site */
                    chunk.emets(nullptr);
                    chunk.emets(0l);
                }
                else {
                    auto acces_index = static_cast<AccedeIndexConstant *>(tableau_valeur[0]);
                    auto globale_tableau = static_cast<AtomeGlobale *>(acces_index->accede);

                    auto tableau = static_cast<AtomeValeurConstante *>(
                        globale_tableau->initialisateur);

                    auto pointeur_chaine = tableau->valeur.valeur_tdc.pointeur;
                    auto taille_chaine = tableau->valeur.valeur_tdc.taille;

                    chunk.emets(OP_CHAINE_CONSTANTE);
                    chunk.emets(nullptr); /* site */
                    chunk.emets(pointeur_chaine);
                    chunk.emets(taille_chaine);

                    // reférence globale, tableau
                    // accède index
                    // --> pointeur de la chaine
                }

                return;
            }

            auto type_compose = static_cast<TypeCompose const *>(type);

            auto index_membre = 0;
            POUR (type_compose->membres) {
                if (it.ne_doit_pas_être_dans_code_machine()) {
                    continue;
                }

                if (tableau_valeur[index_membre] != nullptr) {
                    // À FAIRE(tableau fixe) : initialisation défaut
                    genere_code_binaire_pour_atome(tableau_valeur[index_membre], chunk, true);
                }

                index_membre += 1;
            }

            break;
        }
    }
}

void ConvertisseuseRI::genere_code_binaire_pour_initialisation_globale(AtomeConstante *constante,
                                                                       int decalage,
                                                                       int ou_patcher)
{
    switch (constante->genre) {
        case AtomeConstante::Genre::VALEUR:
        {
            auto valeur_constante = static_cast<AtomeValeurConstante *>(constante);
            unsigned char *donnees = nullptr;

            if (ou_patcher == DONNEES_GLOBALES) {
                donnees = donnees_executions->donnees_globales.donnees() + decalage;
            }
            else {
                donnees = donnees_executions->donnees_constantes.donnees() + decalage;
            }

            switch (valeur_constante->valeur.genre) {
                case AtomeValeurConstante::Valeur::Genre::NULLE:
                {
                    *reinterpret_cast<uint64_t *>(donnees) = 0;
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::TYPE:
                {
                    // utilisation du pointeur directement au lieu de l'index car la table de type
                    // n'est pas implémentée, et il y a des concurrences critiques entre les
                    // métaprogrammes
                    *reinterpret_cast<int64_t *>(donnees) = reinterpret_cast<int64_t>(
                        valeur_constante->valeur.type);
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::TAILLE_DE:
                {
                    *reinterpret_cast<uint32_t *>(
                        donnees) = valeur_constante->valeur.type->taille_octet;
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                {
                    auto valeur_entiere = valeur_constante->valeur.valeur_entiere;
                    auto type = constante->type;

                    if (type->est_type_entier_naturel() || type->est_type_enum() ||
                        type->est_type_erreur()) {
                        if (type->taille_octet == 1) {
                            *donnees = static_cast<unsigned char>(valeur_entiere);
                        }
                        else if (type->taille_octet == 2) {
                            *reinterpret_cast<unsigned short *>(
                                donnees) = static_cast<unsigned short>(valeur_entiere);
                        }
                        else if (type->taille_octet == 4) {
                            *reinterpret_cast<uint32_t *>(donnees) = static_cast<uint32_t>(
                                valeur_entiere);
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
                            *reinterpret_cast<short *>(donnees) = static_cast<short>(
                                valeur_entiere);
                        }
                        else if (type->taille_octet == 4) {
                            *reinterpret_cast<int *>(donnees) = static_cast<int>(valeur_entiere);
                        }
                        else if (type->taille_octet == 8) {
                            *reinterpret_cast<int64_t *>(donnees) = static_cast<int64_t>(
                                valeur_entiere);
                        }
                    }
                    else if (type->est_type_entier_constant()) {
                        *reinterpret_cast<int *>(donnees) = static_cast<int>(valeur_entiere);
                    }

                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::REELLE:
                {
                    auto valeur_reele = valeur_constante->valeur.valeur_reelle;
                    auto type = constante->type;

                    if (type->taille_octet == 4) {
                        *reinterpret_cast<float *>(donnees) = static_cast<float>(valeur_reele);
                    }
                    else {
                        *reinterpret_cast<double *>(donnees) = valeur_reele;
                    }

                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                {
                    assert_rappel(false, [&]() {
                        std::cerr << "Les valeurs de globales de type tableau fixe ne sont pas "
                                     "générées dans le code binaire pour le moment.\n";
                        std::cerr << "Le type est " << chaine_type(constante->type) << '\n';
                    });
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                {
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                {
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                {
                    auto valeur_bool = valeur_constante->valeur.valeur_booleenne;
                    *reinterpret_cast<char *>(donnees) = static_cast<char>(valeur_bool);
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                {
                    break;
                }
                case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                {
                    auto type = static_cast<TypeCompose const *>(constante->type);
                    auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

                    auto index_membre = 0;
                    for (auto i = 0; i < type->membres.taille(); ++i) {
                        if (type->membres[i].ne_doit_pas_être_dans_code_machine()) {
                            continue;
                        }

                        // les tableaux fixes ont une initialisation nulle
                        if (tableau_valeur[index_membre] == nullptr) {
                            index_membre += 1;
                            continue;
                        }

                        // std::cerr << "Ajout du code pour le membre : " << type->membres[i].nom
                        // << '\n';

                        auto type_membre = type->membres[i].type;

                        auto decalage_membre = type->membres[i].decalage;

                        if (type_membre->est_type_chaine()) {
                            auto valeur_chaine = static_cast<AtomeValeurConstante *>(
                                tableau_valeur[index_membre]);
                            auto acces_index = static_cast<AccedeIndexConstant *>(
                                valeur_chaine->valeur.valeur_structure.pointeur[0]);
                            auto globale_tableau = static_cast<AtomeGlobale *>(
                                acces_index->accede);

                            auto tableau = static_cast<AtomeValeurConstante *>(
                                globale_tableau->initialisateur);

                            auto pointeur_chaine = tableau->valeur.valeur_tdc.pointeur;
                            auto taille_chaine = tableau->valeur.valeur_tdc.taille;

                            auto donnees_ = donnees_executions->donnees_globales.donnees() +
                                            decalage + static_cast<int>(decalage_membre);
                            *reinterpret_cast<char **>(donnees_) = pointeur_chaine;
                            *reinterpret_cast<int64_t *>(donnees_ + 8) = taille_chaine;
                        }
                        else if (type_membre->est_type_tableau_dynamique()) {
                            auto valeur_tableau = static_cast<AtomeValeurConstante *>(
                                tableau_valeur[index_membre]);
                            auto acces_index = static_cast<AccedeIndexConstant *>(
                                valeur_tableau->valeur.valeur_structure.pointeur[0]);
                            auto globale_tableau = static_cast<AtomeGlobale *>(
                                acces_index->accede);

                            auto tableau = static_cast<AtomeValeurConstante *>(
                                globale_tableau->initialisateur);

                            auto pointeur = tableau->valeur.valeur_tableau.pointeur;
                            auto taille = tableau->valeur.valeur_tableau.taille;

                            auto type_tableau = tableau->type->comme_type_tableau_fixe();
                            auto type_pointe = type_tableau->type_pointe;
                            auto decalage_valeur = donnees_executions->donnees_constantes.taille();
                            auto adresse_tableau = decalage_valeur;

                            donnees_executions->donnees_constantes.redimensionne(
                                donnees_executions->donnees_constantes.taille() +
                                static_cast<int>(type_pointe->taille_octet) *
                                    type_tableau->taille);

                            for (auto j = 0; j < taille; ++j) {
                                auto pointeur_valeur = pointeur[j];
                                genere_code_binaire_pour_initialisation_globale(
                                    pointeur_valeur, decalage_valeur, DONNEES_CONSTANTES);
                                decalage_valeur += static_cast<int>(type_pointe->taille_octet);
                            }

                            auto patch = PatchDonneesConstantes{};
                            patch.ou = DONNEES_GLOBALES;
                            patch.quoi = ADRESSE_CONSTANTE;
                            patch.decalage_ou = decalage + static_cast<int>(decalage_membre);
                            patch.decalage_quoi = adresse_tableau;

                            donnees_executions->patchs_donnees_constantes.ajoute(patch);

                            auto donnees_ = donnees_executions->donnees_globales.donnees() +
                                            decalage + static_cast<int>(decalage_membre);
                            *reinterpret_cast<int64_t *>(donnees_ + 8) = taille;
                        }
                        else {
                            genere_code_binaire_pour_initialisation_globale(
                                tableau_valeur[index_membre],
                                decalage + static_cast<int>(decalage_membre),
                                ou_patcher);
                        }

                        index_membre += 1;
                    }

                    break;
                }
            }

            break;
        }
        case AtomeConstante::Genre::GLOBALE:
        {
            auto atome_globale = static_cast<AtomeGlobale *>(constante);
            auto index_globale = genere_code_pour_globale(atome_globale);
            auto globale = donnees_executions->globales[index_globale];

            auto patch = PatchDonneesConstantes{};
            patch.ou = ou_patcher;
            patch.quoi = ADRESSE_GLOBALE;
            patch.decalage_ou = decalage;
            patch.decalage_quoi = globale.adresse;

            donnees_executions->patchs_donnees_constantes.ajoute(patch);

            break;
        }
        case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = static_cast<TranstypeConstant *>(constante);
            genere_code_binaire_pour_initialisation_globale(
                transtype->valeur, decalage, ou_patcher);
            break;
        }
        case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
        {
            // À FAIRE
            // assert_rappel(false, []() { std::cerr << "Les opérations unaires constantes ne sont
            // pas implémentées dans le code binaire\n"; });
            break;
        }
        case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
        {
            // À FAIRE
            // assert_rappel(false, []() { std::cerr << "Les opérations binaires constantes ne sont
            // pas implémentées dans le code binaire\n"; });
            break;
        }
        case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
        {
            // À FAIRE
            // assert_rappel(false, []() { std::cerr << "Les indexages constants ne sont pas
            // implémentés dans le code binaire\n"; });
            break;
        }
    }
}

void ConvertisseuseRI::genere_code_binaire_pour_atome(Atome *atome,
                                                      Chunk &chunk,
                                                      bool pour_operande)
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto atome_globale = static_cast<AtomeGlobale *>(atome);
            auto index_globale = genere_code_pour_globale(atome_globale);
            chunk.emets_reference_globale(nullptr, index_globale);
            break;
        }
        case Atome::Genre::FONCTION:
        {
            // l'adresse pour les pointeurs de fonctions
            if (pour_operande) {
                chunk.emets_constante(reinterpret_cast<int64_t>(atome));
            }

            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            genere_code_binaire_pour_instruction(atome->comme_instruction(), chunk, pour_operande);
            break;
        }
        case Atome::Genre::CONSTANTE:
        {
            genere_code_binaire_pour_constante(static_cast<AtomeConstante *>(atome), chunk);
            break;
        }
    }
}

int ConvertisseuseRI::ajoute_globale(AtomeGlobale *globale)
{
    assert(globale->index == -1);
    auto type_globale = globale->type->comme_type_pointeur()->type_pointe;
    auto index = donnees_executions->ajoute_globale(
        type_globale, globale->ident, globale->est_info_type_de);
    globale->index = index;
    return index;
}

int ConvertisseuseRI::genere_code_pour_globale(AtomeGlobale *atome_globale)
{
    auto index = atome_globale->index;

    if (index != -1) {
        /* Un index de -1 indique que le code ne fut pas encore généré. */
        return index;
    }

    index = ajoute_globale(atome_globale);

    if (atome_globale->est_constante && !atome_globale->est_info_type_de) {
        auto globale = donnees_executions->globales[index];
        auto initialisateur = atome_globale->initialisateur;
        genere_code_binaire_pour_initialisation_globale(
            initialisateur, globale.adresse, DONNEES_GLOBALES);
    }

    return index;
}

ContexteGenerationCodeBinaire ConvertisseuseRI::contexte() const
{
    return {espace, fonction_courante};
}

int64_t DonnéesExécutionFonction::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += chunk.capacite;
    résultat += chunk.locales.taille_memoire();
    résultat += chunk.decalages_labels.taille_memoire();

    if (!donnees_externe.types_entrees.est_stocke_dans_classe()) {
        résultat += donnees_externe.types_entrees.capacite() * taille_de(ffi_type *);
    }

    return résultat;
}
