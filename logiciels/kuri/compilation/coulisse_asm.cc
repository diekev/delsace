/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse_asm.hh"

#include <fstream>
#include <iostream>

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/instructions.hh"

#include "structures/enchaineuse.hh"
#include "structures/pile.hh"
#include "structures/table_hachage.hh"

#include "utilitaires/log.hh"

#include "broyage.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "typage.hh"

#define TABULATION "  "
#define TABULATION2 "    "
#define TABULATION3 "      "
#define NOUVELLE_LIGNE "\n"

#define VERIFIE_NON_ATTEINT assert(false)

/* clang-format off */
#define OPERANDE1 AssembleuseASM::Opérande{}
#define OPERANDE2 AssembleuseASM::Opérande{}, AssembleuseASM::Opérande{}
/* clang-format on */

/* ------------------------------------------------------------------------- */
/** \name Utilitaires
 * \{ */

inline bool est_adresse_globale(Atome const *atome)
{
    return atome->est_fonction() || atome->est_globale();
}

inline bool est_adresse_locale(Atome const *atome)
{
    if (!atome->est_instruction()) {
        return false;
    }

    auto inst = atome->comme_instruction();

    if (inst->est_alloc() || inst->est_acces_membre() || inst->est_acces_index()) {
        return true;
    }

    return false;
}

inline bool est_adresse(Atome const *atome)
{
    return est_adresse_globale(atome) || est_adresse_locale(atome);
}

static Atome const *donne_source_charge_ou_atome(Atome const *atome)
{
    if (atome->est_instruction() && atome->comme_instruction()->est_charge()) {
        return atome->comme_instruction()->comme_charge()->chargée;
    }
    return atome;
}

inline bool est_accès_index(Atome const *atome)
{
    return atome->est_instruction() && atome->comme_instruction()->est_acces_index();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Registres x64
 * \{ */

enum class Registre {
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,

    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,
};

#define NOMBRE_REGISTRES_ENTIER 16
#define NOMBRE_REGISTRES_REEL 8
#define NOMBRE_REGISTRES (NOMBRE_REGISTRES_ENTIER + NOMBRE_REGISTRES_REEL)

static kuri::chaine_statique chaine_pour_registre(Registre registre, uint32_t taille_octet)
{
#define APPARIE_REGISTRE(type, nom1, nom2, nom4, nom8)                                            \
    case Registre::type:                                                                          \
    {                                                                                             \
        if (taille_octet == 1)                                                                    \
            return nom1;                                                                          \
        if (taille_octet == 2)                                                                    \
            return nom2;                                                                          \
        if (taille_octet == 4)                                                                    \
            return nom4;                                                                          \
        return nom8;                                                                              \
    }

    switch (registre) {
        APPARIE_REGISTRE(RAX, "al", "ax", "eax", "rax")
        APPARIE_REGISTRE(RBX, "bl", "bx", "ebx", "rbx")
        APPARIE_REGISTRE(RCX, "cl", "cx", "ecx", "rcx")
        APPARIE_REGISTRE(RDX, "dl", "dx", "edx", "rdx")
        APPARIE_REGISTRE(RSI, "sil", "si", "esi", "rsi")
        APPARIE_REGISTRE(RDI, "dil", "di", "edi", "rdi")
        APPARIE_REGISTRE(RBP, "bpl", "bp", "ebp", "rbp")
        APPARIE_REGISTRE(RSP, "spl", "sp", "esp", "rsp")
        APPARIE_REGISTRE(R8, "r8b", "r8w", "r8d", "r8")
        APPARIE_REGISTRE(R9, "r9b", "r9w", "r9d", "r9")
        APPARIE_REGISTRE(R10, "r10b", "r10w", "r10d", "r10")
        APPARIE_REGISTRE(R11, "r11b", "r11w", "r11d", "r11")
        APPARIE_REGISTRE(R12, "r12b", "r12w", "r12d", "r12")
        APPARIE_REGISTRE(R13, "r13b", "r13w", "r13d", "r13")
        APPARIE_REGISTRE(R14, "r14b", "r14W", "r14d", "r14")
        APPARIE_REGISTRE(R15, "r15b", "r15w", "r15d", "r15")

        APPARIE_REGISTRE(XMM0, "xmm0", "xmm0", "xmm0", "xmm0")
        APPARIE_REGISTRE(XMM1, "xmm1", "xmm1", "xmm1", "xmm1")
        APPARIE_REGISTRE(XMM2, "xmm2", "xmm2", "xmm2", "xmm2")
        APPARIE_REGISTRE(XMM3, "xmm3", "xmm3", "xmm3", "xmm3")
        APPARIE_REGISTRE(XMM4, "xmm4", "xmm4", "xmm4", "xmm4")
        APPARIE_REGISTRE(XMM5, "xmm5", "xmm5", "xmm5", "xmm5")
        APPARIE_REGISTRE(XMM6, "xmm6", "xmm6", "xmm6", "xmm6")
        APPARIE_REGISTRE(XMM7, "xmm7", "xmm7", "xmm7", "xmm7")
    }

#undef APPARIE_REGISTRE

    return "registre_invalide";
}

static std::ostream &operator<<(std::ostream &os, Registre reg)
{
    return os << chaine_pour_registre(reg, 8);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ABI x64 pour passer des paramètres
 *  https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.99.pdf section 3.2.3
 * \{ */

enum class ClasseArgument : uint8_t {
    NO_CLASS,
    INTEGER,
    SSE,
    SSEUP,
    X87,
    X87UP,
    COMPLEX_X87,
    MEMORY,
};

static std::ostream &operator<<(std::ostream &os, ClasseArgument arg)
{
#define IMPRIME_CAS(x)                                                                            \
    case ClasseArgument::x:                                                                       \
        os << #x;                                                                                 \
        break
    switch (arg) {
        IMPRIME_CAS(NO_CLASS);
        IMPRIME_CAS(INTEGER);
        IMPRIME_CAS(SSE);
        IMPRIME_CAS(SSEUP);
        IMPRIME_CAS(X87);
        IMPRIME_CAS(X87UP);
        IMPRIME_CAS(COMPLEX_X87);
        IMPRIME_CAS(MEMORY);
    }
#undef IMPRIME_CAS
    return os;
}

/* La taille de chaque type doit être alignée sur 8 octets. */
static uint32_t donne_taille_alignée(Type const *type)
{
    return (type->taille_octet + 7u) & ~7u;
}

struct Huitoctet {
    ClasseArgument classe = ClasseArgument::NO_CLASS;
};

static void donne_classe_argument(Type const *type, kuri::tablet<Huitoctet, 4> &résultat);

class ConstructriceHuitoctets {
    struct DonnéesHuitoctets {
        /* Index dans le tableau de types. */
        uint32_t premier_type_inclusif = 0;
        uint32_t dernier_type_exclusif = 0;
        uint32_t taille_types = 0;
    };

    kuri::tablet<Huitoctet, 4> résultat{};
    kuri::tablet<DonnéesHuitoctets, 4> données{};
    kuri::tableau<Type const *> types{};
    kuri::tableau<ClasseArgument> classes_pour_types{};
    uint32_t index_huitoctet = 0;

  public:
    static void construit_huitoctets(Type const *type, kuri::tablet<Huitoctet, 4> &résultat)
    {
        /* Le résultat doit être prédimensionné. */
        assert(résultat.taille() != 0);

        ConstructriceHuitoctets constructrice;
        constructrice.résultat = résultat;
        constructrice.données.redimensionne(résultat.taille());

        constructrice.construit_huitoctets_récursif(type);
        constructrice.assigne_classes_huitoctets();

        résultat = constructrice.résultat;
    }

  private:
    void construit_huitoctets_récursif(Type const *type);

    void assigne_classes_huitoctets();

    void ajoute_type(Type const *type)
    {
        assert_rappel(type->taille_octet <= 8,
                      [&]() { dbg() << "Le type est " << chaine_type(type); });

        if (données[index_huitoctet].taille_types == 8 ||
            (données[index_huitoctet].taille_types + type->taille_octet > 8)) {
            index_huitoctet += 1;
            données[index_huitoctet].premier_type_inclusif = uint32_t(types.taille());
        }

        données[index_huitoctet].dernier_type_exclusif = uint32_t(types.taille() + 1);
        données[index_huitoctet].taille_types += type->taille_octet;
        types.ajoute(type);

        kuri::tablet<Huitoctet, 4> tampon_classe;
        donne_classe_argument(type, tampon_classe);
        assert(tampon_classe.taille() == 1);
        classes_pour_types.ajoute(tampon_classe[0].classe);
    }

    void ajoute_rembourrage(uint32_t rembourrage)
    {
        assert((index_huitoctet * 8) + données[index_huitoctet].taille_types + rembourrage <= 32);

        while (rembourrage > 0) {
            auto à_rembourrer = std::min(8 - données[index_huitoctet].taille_types, rembourrage);

            données[index_huitoctet].taille_types += à_rembourrer;

            if (données[index_huitoctet].taille_types == 8) {
                index_huitoctet += 1;
            }

            rembourrage -= à_rembourrer;
        }
    }
};

void ConstructriceHuitoctets::construit_huitoctets_récursif(Type const *type)
{
    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            return;
        }
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        case GenreNoeud::RÉEL:
        {
            ajoute_type(type);
            return;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto const type_opaque = type->comme_type_opaque();
            construit_huitoctets_récursif(type_opaque->type_opacifié);
            return;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TUPLE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_composé = type->comme_type_composé();

            auto décalage = uint32_t(0);

            POUR (type_composé->donne_membres_pour_code_machine()) {
                if (it.decalage != décalage) {
                    auto rembourrage = it.decalage - décalage;
                    ajoute_rembourrage(rembourrage);
                    décalage += rembourrage;
                }

                construit_huitoctets_récursif(it.type);
                décalage += it.type->taille_octet;
            }

            if (type->taille_octet != décalage) {
                auto rembourrage = type->taille_octet - décalage;
                ajoute_rembourrage(rembourrage);
            }
            return;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();
            if (type_union->est_nonsure) {
                construit_huitoctets_récursif(type_union->type_le_plus_grand);
                return;
            }
            construit_huitoctets_récursif(type_union->type_structure);
            return;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto const type_tableau_fixe = type->comme_type_tableau_fixe();
            for (int i = 0; i < type_tableau_fixe->taille; i++) {
                construit_huitoctets_récursif(type_tableau_fixe->type_pointé);
            }
            return;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }
}

void ConstructriceHuitoctets::assigne_classes_huitoctets()
{
    POUR_INDEX (données) {
        auto classe_huitoctet = classes_pour_types[it.premier_type_inclusif];
        auto classe1 = classe_huitoctet;

        for (auto i = it.premier_type_inclusif + 1; i < it.dernier_type_exclusif; i++) {
            auto classe2 = classes_pour_types[i];

            // (a) If both classes are equal, this is the resulting class.
            if (classe1 == classe2) {
                classe_huitoctet = classe1;
            }
            // (b) If one of the classes is NO_CLASS, the resulting class is the other class.
            else if (classe1 == ClasseArgument::NO_CLASS) {
                classe_huitoctet = classe2;
            }
            else if (classe2 == ClasseArgument::NO_CLASS) {
                classe_huitoctet = classe1;
            }
            // (c) If one of the classes is MEMORY, the result is the MEMORY class.
            else if (classe1 == ClasseArgument::MEMORY || classe2 == ClasseArgument::MEMORY) {
                classe_huitoctet = ClasseArgument::MEMORY;
            }
            // (d) If one of the classes is INTEGER, the result is the INTEGER.
            else if (classe1 == ClasseArgument::INTEGER || classe2 == ClasseArgument::INTEGER) {
                classe_huitoctet = ClasseArgument::INTEGER;
            }
            // (e) If one of the classes is X87, X87UP, COMPLEX_X87 class, MEMORY is used as class.
            else if (classe1 == ClasseArgument::X87 || classe2 == ClasseArgument::X87 ||
                     classe1 == ClasseArgument::X87UP || classe2 == ClasseArgument::X87UP ||
                     classe1 == ClasseArgument::COMPLEX_X87 ||
                     classe2 == ClasseArgument::COMPLEX_X87) {
                classe_huitoctet = ClasseArgument::MEMORY;
            }
            // (f) Otherwise class SSE is used.
            else {
                classe_huitoctet = ClasseArgument::SSE;
            }

            classe1 = classe_huitoctet;
        }

        résultat[index_it].classe = classe_huitoctet;
    }
}

static void détermine_classe_argument_aggrégé(TypeCompose const *type,
                                              kuri::tablet<Huitoctet, 4> &résultat)
{
    auto const taille = donne_taille_alignée(type);
    auto nombre_huitoctets = (taille + 7) / 8;
    dbg() << chaine_type(type) << " taille octet " << type->taille_octet << " taille alignée "
          << taille;
    dbg() << "Nombre de huitoctets pour " << chaine_type(type) << " : " << nombre_huitoctets;

    /* 1. If the size of an object is larger than four eightbytes, or it contains unaligned fields,
     *    it has class MEMORY. */
    // À FAIRE : champs non-aligné.
    if (nombre_huitoctets > 4) {
        résultat.ajoute(Huitoctet{ClasseArgument::MEMORY});
        return;
    }

    /* 3. If the size of the aggregate exceeds a single eightbyte, each is classified separately.
     *    Each eightbyte gets initialized to class NO_CLASS. */
    auto huitoctets = kuri::tablet<Huitoctet, 4>();
    for (auto i = 0; i < nombre_huitoctets; i++) {
        huitoctets.ajoute({ClasseArgument::NO_CLASS});
    }

    // 4. Each field of an object is classified recursively so that always two fields are
    //    considered. The resulting class is calculated according to the classes of the
    //    fields in the eightbyte:
    ConstructriceHuitoctets::construit_huitoctets(type, huitoctets);

    // 5. Then a post merger cleanup is done:
    auto classe_précédente = ClasseArgument::NO_CLASS;

    POUR (huitoctets) {
        // (a) If one of the classes is MEMORY, the whole argument is passed in memory.
        if (it.classe == ClasseArgument::MEMORY) {
            résultat.ajoute(Huitoctet{ClasseArgument::MEMORY});
            return;
        }

        // (b) If X87UP is not preceded by X87, the whole argument is passed in memory.
        if (it.classe == ClasseArgument::X87UP && classe_précédente != ClasseArgument::X87) {
            résultat.ajoute(Huitoctet{ClasseArgument::MEMORY});
            return;
        }

        classe_précédente = it.classe;
    }

    // (c) If the size of the aggregate exceeds two eightbytes and the first eightbyte isn’t
    //     SSE or any other eightbyte isn’t SSEUP, the whole argument is passed in memory.
    if (huitoctets.taille() > 2) {
        if (huitoctets[0].classe != ClasseArgument::SSE) {
            résultat.ajoute(Huitoctet{ClasseArgument::MEMORY});
            return;
        }

        auto sseup_rencontré = false;
        for (int i = 1; i < huitoctets.taille(); i++) {
            if (huitoctets[i].classe == ClasseArgument::SSEUP) {
                sseup_rencontré = true;
                break;
            }
        }

        if (!sseup_rencontré) {
            résultat.ajoute(Huitoctet{ClasseArgument::MEMORY});
            return;
        }
    }

    // (d) If SSEUP is not preceded by SSE or SSEUP, it is converted to SSE.
    classe_précédente = ClasseArgument::NO_CLASS;
    POUR (huitoctets) {
        if (it.classe == ClasseArgument::SSEUP && (classe_précédente != ClasseArgument::SSE &&
                                                   classe_précédente != ClasseArgument::SSEUP)) {
            it.classe = ClasseArgument::SSE;
        }

        classe_précédente = it.classe;
    }

    dbg() << "Classement des huitoctets :";
    POUR (huitoctets) {
        dbg() << "-- " << it.classe;
        résultat.ajoute(it);
    }
}

static void donne_classe_argument(Type const *type, kuri::tablet<Huitoctet, 4> &résultat)
{
    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            résultat.ajoute({ClasseArgument::NO_CLASS});
            return;
        }
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            résultat.ajoute({ClasseArgument::INTEGER});
            return;
        }
        case GenreNoeud::RÉEL:
        {
            /* @Incomplet : __m128, __m256, etc. */
            résultat.ajoute({ClasseArgument::SSE});
            return;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto const type_opaque = type->comme_type_opaque();
            donne_classe_argument(type_opaque->type_opacifié, résultat);
            return;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::TUPLE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::TYPE_TRANCHE:
        {
            // À FAIRE : types variadiques externes
            return détermine_classe_argument_aggrégé(type->comme_type_composé(), résultat);
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();
            if (type_union->est_nonsure) {
                donne_classe_argument(type_union->type_le_plus_grand, résultat);
                return;
            }
            return détermine_classe_argument_aggrégé(type_union->type_structure, résultat);
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }
}

struct ArgumentPassé {
    uint32_t premier_huitoctet_inclusif = 0;
    uint32_t dernier_huitoctet_exclusif = 0;
    bool est_en_mémoire = false;
};

struct RegistreHuitoctet {
    Registre registre;
};

struct ClassementArgument {
    kuri::tableau<Huitoctet> huitoctets{};
    kuri::tableau<RegistreHuitoctet> registres_huitoctets{};
    kuri::tableau<ArgumentPassé> arguments{};
    ArgumentPassé sortie{};
};

class AllocatriceRegistreArgument {
    kuri::tablet<Registre, 6> registres_integer{};
    kuri::tablet<Registre, 8> registres_sse{};

    int premier_registre_integer = 0;
    int premier_registre_sse = 0;

    kuri::tablet<Registre, 6> registres_integer_retour{};
    kuri::tablet<Registre, 8> registres_sse_retour{};

    int premier_registre_integer_retour = 0;
    int premier_registre_sse_retour = 0;

    int ancien_premier_registre_integer = 0;
    int ancien_premier_registre_sse = 0;

  public:
    AllocatriceRegistreArgument()
    {
        registres_integer.ajoute(Registre::RDI);
        registres_integer.ajoute(Registre::RSI);
        registres_integer.ajoute(Registre::RDX);
        registres_integer.ajoute(Registre::RCX);
        registres_integer.ajoute(Registre::R8);
        registres_integer.ajoute(Registre::R9);

        registres_sse.ajoute(Registre::XMM0);
        registres_sse.ajoute(Registre::XMM1);
        registres_sse.ajoute(Registre::XMM2);
        registres_sse.ajoute(Registre::XMM3);
        registres_sse.ajoute(Registre::XMM4);
        registres_sse.ajoute(Registre::XMM5);
        registres_sse.ajoute(Registre::XMM6);
        registres_sse.ajoute(Registre::XMM7);

        registres_integer_retour.ajoute(Registre::RAX);
        registres_integer_retour.ajoute(Registre::RDX);

        registres_sse_retour.ajoute(Registre::XMM0);
        registres_sse_retour.ajoute(Registre::XMM1);
    }

    void enregistre_état()
    {
        ancien_premier_registre_integer = premier_registre_integer;
        ancien_premier_registre_sse = premier_registre_sse;
    }

    void restaure_état()
    {
        premier_registre_integer = ancien_premier_registre_integer;
        premier_registre_sse = ancien_premier_registre_sse;
    }

    bool peut_passer_integer() const
    {
        return premier_registre_integer < registres_integer.taille();
    }

    bool peut_passer_sse() const
    {
        return premier_registre_sse < registres_sse.taille();
    }

    Registre donne_registre_integer()
    {
        auto résultat = registres_integer[premier_registre_integer];
        premier_registre_integer += 1;
        return résultat;
    }

    Registre donne_registre_sse()
    {
        auto résultat = registres_sse[premier_registre_sse];
        premier_registre_sse += 1;
        return résultat;
    }

    Registre donne_dernier_registre_sse()
    {
        return registres_sse[premier_registre_sse - 1];
    }

    Registre donne_registre_integer_retour()
    {
        auto résultat = registres_integer_retour[premier_registre_integer_retour];
        premier_registre_integer_retour += 1;
        return résultat;
    }

    Registre donne_registre_sse_retour()
    {
        auto résultat = registres_sse_retour[premier_registre_sse_retour];
        premier_registre_sse_retour += 1;
        return résultat;
    }

    Registre donne_dernier_registre_sse_retour()
    {
        return registres_sse_retour[premier_registre_sse_retour - 1];
    }
};

class ClassifieuseArgument {
    kuri::table_hachage<TypeFonction const *, int> m_table_classement_arguments{
        "Table classement arguments"};
    kuri::tableau<ClassementArgument> m_classements_arguments{};

    /* Indexe m_rangée_huitoctets_types. */
    kuri::table_hachage<Type const *, int> m_table_huitoctets_types{"Table huitoctets types"};

    struct RangéeHuitoctetsType {
        int index_premier_inclusif = 0;
        int index_dernier_exclusif = 0;
    };

    kuri::tableau<RangéeHuitoctetsType> m_rangées_huitoctets_types{};
    kuri::tableau<Huitoctet> m_huitoctets_types{};

  public:
    ClassementArgument donne_classement_arguments(TypeFonction const *type)
    {
        auto index = m_table_classement_arguments.valeur_ou(type, -1);
        if (index != -1) {
            return m_classements_arguments[index];
        }

        auto classement = donne_classement_arguments_impl(type);
        m_classements_arguments.ajoute(classement);
        m_table_classement_arguments.insère(type, int32_t(m_classements_arguments.taille() - 1));
        return classement;
    }

  private:
    ClassementArgument donne_classement_arguments_impl(TypeFonction const *type);
    ClassementArgument détermine_classes_arguments(TypeFonction const *type);
    kuri::tablet<Huitoctet, 4> donne_classe_argument(Type const *type);
};

ClassementArgument ClassifieuseArgument::donne_classement_arguments_impl(
    TypeFonction const *type_fonction)
{
    auto classement = détermine_classes_arguments(type_fonction);

    auto allocatrice_registre = AllocatriceRegistreArgument();

    classement.registres_huitoctets.redimensionne(classement.huitoctets.taille());

    if (!type_fonction->type_sortie->est_type_rien()) {
        for (auto i = classement.sortie.premier_huitoctet_inclusif;
             i < classement.sortie.dernier_huitoctet_exclusif;
             i++) {
            // 1. Classify the return type with the classification algorithm.
            auto classe = classement.huitoctets[i].classe;

            // 2. If the type has class MEMORY, then the caller provides space for the return
            //    value and passes the address of this storage in %rdi as if it were the first
            //    argument to the function. In effect, this address becomes a “hidden” first
            //    argument. This storage must not overlap any data visible to the callee
            //    through other names than this argument. On return %rax will contain the
            //    address that has been passed in by the caller in %rdi.
            if (classe == ClasseArgument::MEMORY) {
                classement.sortie.est_en_mémoire = true;
                /* réserve %rdi pour l'adresse retour. */
                allocatrice_registre.donne_registre_integer();
            }
            // 3. If the class is INTEGER, the next available register of the sequence %rax,
            //    %rdx is used.
            else if (classe == ClasseArgument::INTEGER) {
                classement.registres_huitoctets[i].registre =
                    allocatrice_registre.donne_registre_integer_retour();
            }
            // 4. If the class is SSE, the next available vector register of the sequence
            //    %xmm0, %xmm1 is used.
            else if (classe == ClasseArgument::SSE) {
                classement.registres_huitoctets[i].registre =
                    allocatrice_registre.donne_registre_sse_retour();
            }
            // 5. If the class is SSEUP, the eightbyte is returned in the next available
            //    eightbyte chunk of the last used vector register.
            else {
                VERIFIE_NON_ATTEINT;
            }
            // 6. If the class is X87, the value is returned on the X87 stack in %st0 as 80-bit
            //    x87 number.

            // 7. If the class is X87UP, the value is returned together with the previous X87
            //    value in %st0.

            // 8. If the class is COMPLEX_X87, the real part of the value is returned in
            //    %st0 and the imaginary part in %st1.
        }
    }

    POUR (classement.arguments) {
        allocatrice_registre.enregistre_état();

        for (auto i = it.premier_huitoctet_inclusif; i < it.dernier_huitoctet_exclusif; i++) {
            auto classe = classement.huitoctets[i].classe;

            // 1. If the class is MEMORY, pass the argument on the stack.
            if (classe == ClasseArgument::MEMORY) {
                it.est_en_mémoire = true;
            }
            // 2. If the class is INTEGER, the next available register of the sequence %rdi, %rsi,
            //    %rdx, %rcx, %r8 and %r9 is used.
            else if (classe == ClasseArgument::INTEGER) {
                if (!allocatrice_registre.peut_passer_integer()) {
                    it.est_en_mémoire = true;
                    allocatrice_registre.restaure_état();
                    break;
                }

                classement.registres_huitoctets[i].registre =
                    allocatrice_registre.donne_registre_integer();
            }
            // 3. If the class is SSE, the next available vector register is used, the registers
            //    are taken in the order from %xmm0 to %xmm7.
            else if (classe == ClasseArgument::SSE) {
                if (!allocatrice_registre.peut_passer_sse()) {
                    it.est_en_mémoire = true;
                    allocatrice_registre.restaure_état();
                    break;
                }

                classement.registres_huitoctets[i].registre =
                    allocatrice_registre.donne_registre_sse();
            }
            // 4. If the class is SSEUP, the eightbyte is passed in the next available eightbyte
            //    chunk of the last used vector register.
            else if (classe == ClasseArgument::SSEUP) {
                // À FAIRE : next available eightbytes
                classement.registres_huitoctets[i].registre =
                    allocatrice_registre.donne_dernier_registre_sse();
            }
            // 5. If the class is X87, X87UP or COMPLEX_X87, it is passed in memory.
            else {
                it.est_en_mémoire = true;
            }
        }
    }

    return classement;
}

ClassementArgument ClassifieuseArgument::détermine_classes_arguments(TypeFonction const *type)
{
    auto résultat = ClassementArgument{};

    POUR (type->types_entrées) {
        auto argument = ArgumentPassé{};
        argument.premier_huitoctet_inclusif = uint32_t(résultat.huitoctets.taille());

        auto tampon_huitoctets = donne_classe_argument(it);
        for (auto huitoctet : tampon_huitoctets) {
            résultat.huitoctets.ajoute(huitoctet);
        }

        argument.dernier_huitoctet_exclusif = uint32_t(résultat.huitoctets.taille());
        résultat.arguments.ajoute(argument);
    }

    résultat.sortie.premier_huitoctet_inclusif = uint32_t(résultat.huitoctets.taille());
    auto tampon_huitoctets = donne_classe_argument(type->type_sortie);
    for (auto huitoctet : tampon_huitoctets) {
        résultat.huitoctets.ajoute(huitoctet);
    }
    résultat.sortie.dernier_huitoctet_exclusif = uint32_t(résultat.huitoctets.taille());

    return résultat;
}

kuri::tablet<Huitoctet, 4> ClassifieuseArgument::donne_classe_argument(Type const *type)
{
    kuri::tablet<Huitoctet, 4> résultat;

    auto index = m_table_huitoctets_types.valeur_ou(type, -1);
    if (index == -1) {
        ::donne_classe_argument(type, résultat);

        auto rangée = RangéeHuitoctetsType{};
        rangée.index_premier_inclusif = int32_t(m_huitoctets_types.taille());
        rangée.index_dernier_exclusif = rangée.index_premier_inclusif + int32_t(résultat.taille());

        POUR (résultat) {
            m_huitoctets_types.ajoute(it);
        }

        m_rangées_huitoctets_types.ajoute(rangée);

        index = int32_t(m_rangées_huitoctets_types.taille() - 1);
        m_table_huitoctets_types.insère(type, index);
        return résultat;
    }

    auto rangée = m_rangées_huitoctets_types[index];

    for (auto i = rangée.index_premier_inclusif; i < rangée.index_dernier_exclusif; i++) {
        résultat.ajoute(m_huitoctets_types[i]);
    }

    return résultat;
}

void classifie_arguments(AtomeFonction const *fonction)
{
    dbg() << "------------------------------------------";

    auto classifieuse = ClassifieuseArgument{};
    auto classement = classifieuse.donne_classement_arguments(
        fonction->type->comme_type_fonction());

    auto index_arg = 0;
    POUR (classement.arguments) {
        dbg() << fonction->params_entrée[index_arg++]->ident->nom;

        for (auto i = it.premier_huitoctet_inclusif; i < it.dernier_huitoctet_exclusif; i++) {
            auto registre = classement.registres_huitoctets[i].registre;
            dbg() << " => " << classement.huitoctets[i].classe << " ("
                  << chaine_pour_registre(registre, 8) << ")";
        }
    }
}

/** \} */

/*

  Pour réduire la taille du code :

  préfére imm8, imm16, imm32 si le nombre le permet pour mov

  mov reg, 0 => xor reg, reg
  cmp reg, 0 => test reg, reg (pour les sauts : saute_si_zéro, saute_si_non_zéro)
  add reg, 1 => inc reg
  sub reg, 1 => dec reg

*/

static kuri::chaine_statique donne_chaine_taille_opérande(uint32_t taille)
{
    if (taille == 1) {
        return "byte";
    }
    if (taille == 2) {
        return "word";
    }
    if (taille == 4) {
        return "dword";
    }
    return "qword";
}

enum class TypeOpérande {
    REGISTRE,
    IMMÉDIATE8,
    IMMÉDIATE16,
    IMMÉDIATE32,
    IMMÉDIATE64,
    MÉMOIRE,
    FONCTION,
    GLOBALE,
};

static std::ostream &operator<<(std::ostream &os, TypeOpérande type)
{
#define IMPRIME_CAS(x)                                                                            \
    case TypeOpérande::x:                                                                         \
        os << #x;                                                                                 \
        break
    switch (type) {
        IMPRIME_CAS(REGISTRE);
        IMPRIME_CAS(IMMÉDIATE8);
        IMPRIME_CAS(IMMÉDIATE16);
        IMPRIME_CAS(IMMÉDIATE32);
        IMPRIME_CAS(IMMÉDIATE64);
        IMPRIME_CAS(MÉMOIRE);
        IMPRIME_CAS(FONCTION);
        IMPRIME_CAS(GLOBALE);
    }
#undef IMPRIME_CAS
    return os;
}

struct AssembleuseASM {
  private:
    Enchaineuse &m_sortie;

  public:
    struct Immédiate8 {
        uint8_t valeur;
    };

    struct Immédiate16 {
        uint16_t valeur;
    };

    struct Immédiate32 {
        uint32_t valeur;
    };

    struct Immédiate64 {
        uint64_t valeur;
    };

    struct Fonction {
        kuri::chaine_statique valeur;
    };

    struct Globale {
        kuri::chaine_statique valeur;
    };

    static bool est_immédiate(TypeOpérande type)
    {
        switch (type) {
            case TypeOpérande::IMMÉDIATE8:
            case TypeOpérande::IMMÉDIATE16:
            case TypeOpérande::IMMÉDIATE32:
            case TypeOpérande::IMMÉDIATE64:
            {
                return true;
            }
            case TypeOpérande::REGISTRE:
            case TypeOpérande::MÉMOIRE:
            case TypeOpérande::FONCTION:
            case TypeOpérande::GLOBALE:
            {
                return false;
            }
        }
        return false;
    }

    struct Mémoire {
        /* Un registre ou le nom d'une globale. */
        kuri::chaine_statique adresse{};
        int32_t décalage = 0;
        bool est_globale = false;

        Mémoire() = default;

        Mémoire(Registre registre, int32_t décalage_ = 0)
            : adresse(chaine_pour_registre(registre, 8)), décalage(décalage_)
        {
        }

        Mémoire(kuri::chaine_statique globale) : adresse(globale), est_globale(true)
        {
        }
    };

    struct Opérande {
        TypeOpérande type{};
        Registre registre{};
        Immédiate8 immédiate8{};
        Immédiate16 immédiate16{};
        Immédiate32 immédiate32{};
        Immédiate64 immédiate64{};
        Mémoire mémoire{};
        Fonction fonction{};
        Globale globale{};

        Opérande() {};

        Opérande(Mémoire mém) : type(TypeOpérande::MÉMOIRE), mémoire(mém)
        {
        }

        Opérande(Immédiate8 imm) : type(TypeOpérande::IMMÉDIATE8), immédiate8(imm)
        {
        }

        Opérande(Immédiate16 imm) : type(TypeOpérande::IMMÉDIATE16), immédiate16(imm)
        {
        }

        Opérande(Immédiate32 imm) : type(TypeOpérande::IMMÉDIATE32), immédiate32(imm)
        {
        }

        Opérande(Immédiate64 imm) : type(TypeOpérande::IMMÉDIATE64), immédiate64(imm)
        {
        }

        Opérande(Registre reg) : type(TypeOpérande::REGISTRE), registre(reg)
        {
        }

        Opérande(Fonction fonct) : type(TypeOpérande::FONCTION), fonction(fonct)
        {
        }

        Opérande(Globale glob) : type(TypeOpérande::GLOBALE), globale(glob)
        {
        }

        bool est_mémoire() const
        {
            return type == TypeOpérande::MÉMOIRE && mémoire.est_globale == false;
        }
    };

    static bool est_registre(Opérande src, Registre reg)
    {
        return src.type == TypeOpérande::REGISTRE && src.registre == reg;
    }

    static Opérande donne_immédiate8(Opérande opérande)
    {
        assert(est_immédiate(opérande.type));
        if (opérande.type == TypeOpérande::IMMÉDIATE16) {
            return Immédiate8{static_cast<uint8_t>(opérande.immédiate16.valeur)};
        }
        if (opérande.type == TypeOpérande::IMMÉDIATE32) {
            return Immédiate8{static_cast<uint8_t>(opérande.immédiate32.valeur)};
        }
        if (opérande.type == TypeOpérande::IMMÉDIATE64) {
            return Immédiate8{static_cast<uint8_t>(opérande.immédiate64.valeur)};
        }
        return opérande;
    }

    AssembleuseASM(Enchaineuse &sortie) : m_sortie(sortie)
    {
    }

    void mov(Opérande dst, Opérande src, uint32_t taille)
    {
        assert(!est_immédiate(dst.type));
        assert(taille <= 8);
        assert(!dst.est_mémoire() || !src.est_mémoire());

        m_sortie << TABULATION << "mov ";
        if (dst.type == TypeOpérande::MÉMOIRE) {
            m_sortie << donne_chaine_taille_opérande(taille) << " ";
        }
        imprime_opérande(dst, taille);
        m_sortie << ", ";
        imprime_opérande(src, taille);

        m_sortie << NOUVELLE_LIGNE;
    }

    void mov_ah(Opérande dst)
    {
        assert(!est_immédiate(dst.type));

        m_sortie << TABULATION << "mov ";
        if (dst.type == TypeOpérande::MÉMOIRE) {
            m_sortie << donne_chaine_taille_opérande(1) << " ";
        }
        imprime_opérande(dst, 1);
        m_sortie << ", ah";

        m_sortie << NOUVELLE_LIGNE;
    }

    void movsx(Opérande dst, uint32_t taille_dst, Opérande src, uint32_t taille_src)
    {
        assert(!est_immédiate(dst.type) && !est_immédiate(src.type));
        assert(taille_dst <= 8 && taille_src <= 8);
        assert(!dst.est_mémoire() || !src.est_mémoire());

        m_sortie << TABULATION << "movsx ";
        if (dst.type == TypeOpérande::MÉMOIRE) {
            m_sortie << donne_chaine_taille_opérande(taille_dst) << " ";
        }
        imprime_opérande(dst, taille_dst);
        m_sortie << ", ";
        imprime_opérande(src, taille_src);

        m_sortie << NOUVELLE_LIGNE;
    }

    void movzx(Opérande dst, uint32_t taille_dst, Opérande src, uint32_t taille_src)
    {
        assert(!est_immédiate(dst.type) && !est_immédiate(src.type));
        assert(taille_dst <= 8 && taille_src <= 8);
        assert(!dst.est_mémoire() || !src.est_mémoire());

        m_sortie << TABULATION << "movzx ";
        if (dst.type == TypeOpérande::MÉMOIRE) {
            m_sortie << donne_chaine_taille_opérande(taille_dst) << " ";
        }
        imprime_opérande(dst, taille_dst);
        m_sortie << ", ";
        imprime_opérande(src, taille_src);

        m_sortie << NOUVELLE_LIGNE;
    }

    void movss(Opérande dst, Opérande src)
    {
        assert(!est_immédiate(dst.type));
        assert(dst.type != TypeOpérande::MÉMOIRE || src.type != TypeOpérande::MÉMOIRE);

        m_sortie << TABULATION << "movss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);

        m_sortie << NOUVELLE_LIGNE;
    }

    void movsd(Opérande dst, Opérande src)
    {
        assert(!est_immédiate(dst.type));
        assert(dst.type != TypeOpérande::MÉMOIRE || src.type != TypeOpérande::MÉMOIRE);

        m_sortie << TABULATION << "movsd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);

        m_sortie << NOUVELLE_LIGNE;
    }

    void lea(Opérande dst, Opérande src)
    {
        assert_rappel(src.type == TypeOpérande::MÉMOIRE,
                      [&]() { dbg() << "La source est de type " << src.type; });
        assert(dst.type == TypeOpérande::REGISTRE);

        m_sortie << TABULATION << "lea ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);

        m_sortie << NOUVELLE_LIGNE;
    }

    void call(Opérande src)
    {
        m_sortie << TABULATION << "call ";
        imprime_opérande(src);
        m_sortie << NOUVELLE_LIGNE;
    }

    void jump(int id_label)
    {
        m_sortie << TABULATION << "jmp .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_zéro(int id_label)
    {
        m_sortie << TABULATION << "jz .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_non_zéro(int id_label)
    {
        m_sortie << TABULATION << "jnz .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_égal(int id_label)
    {
        m_sortie << TABULATION << "je .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_inégal(int id_label)
    {
        m_sortie << TABULATION << "jne .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_inférieur(int id_label)
    {
        m_sortie << TABULATION << "jl .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_inférieur_égal(int id_label)
    {
        m_sortie << TABULATION << "jle .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_supérieur(int id_label)
    {
        m_sortie << TABULATION << "jg .label" << id_label << NOUVELLE_LIGNE;
    }

    void jump_si_supérieur_égal(int id_label)
    {
        m_sortie << TABULATION << "jge .label" << id_label << NOUVELLE_LIGNE;
    }

    void label(int id)
    {
        m_sortie << ".label" << id << ":" << NOUVELLE_LIGNE;
    }

    void add(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "add", taille_octet);
    }

    void sub(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "sub", taille_octet);
    }

    void mul(Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "mul ";
        m_sortie << donne_chaine_taille_opérande(taille_octet) << " ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void imul(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "imul", taille_octet);
    }

    void imul(Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "imul ";
        m_sortie << donne_chaine_taille_opérande(taille_octet) << " ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void div(Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "div ";
        m_sortie << donne_chaine_taille_opérande(taille_octet) << " ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void idiv(Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "idiv ";
        m_sortie << donne_chaine_taille_opérande(taille_octet) << " ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void neg(Opérande dst, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "neg ";
        imprime_opérande(dst, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void addss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "addss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);
        m_sortie << NOUVELLE_LIGNE;
    }

    void addsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "addsd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);
        m_sortie << NOUVELLE_LIGNE;
    }

    void mulss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "mulss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);
        m_sortie << NOUVELLE_LIGNE;
    }

    void mulsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "mulsd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);
        m_sortie << NOUVELLE_LIGNE;
    }

    void subss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "subss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);
        m_sortie << NOUVELLE_LIGNE;
    }

    void subsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "subsd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);
        m_sortie << NOUVELLE_LIGNE;
    }

    void divss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "divss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);
        m_sortie << NOUVELLE_LIGNE;
    }

    void divsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "divsd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);
        m_sortie << NOUVELLE_LIGNE;
    }

    void xorps(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "xorps ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);
        m_sortie << NOUVELLE_LIGNE;
    }

    void and_(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "and", taille_octet);
    }

    void or_(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "or", taille_octet);
    }

    void xor_(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "xor", taille_octet);
    }

    void not_(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "not" << NOUVELLE_LIGNE;
    }

    void shl(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_décalage_bits(dst, src, "shl", taille_octet);
    }

    void shr(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_décalage_bits(dst, src, "shr", taille_octet);
    }

    void sar(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_décalage_bits(dst, src, "sar", taille_octet);
    }

    void cmp(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "cmp ";
        if (dst.type == TypeOpérande::MÉMOIRE) {
            m_sortie << donne_chaine_taille_opérande(taille_octet) << " ";
        }
        imprime_opérande(dst, taille_octet);
        m_sortie << ", ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void ucomiss(Opérande dst, Opérande src)
    {
        assert(dst.type == TypeOpérande::REGISTRE);
        assert(!est_immédiate(src.type));

        m_sortie << TABULATION << "ucomiss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 4);

        m_sortie << NOUVELLE_LIGNE;
    }

    void ucomisd(Opérande dst, Opérande src)
    {
        assert(dst.type == TypeOpérande::REGISTRE);
        assert(!est_immédiate(src.type));

        m_sortie << TABULATION << "ucomisd ";
        imprime_opérande(dst, 8);
        m_sortie << ", ";
        imprime_opérande(src, 8);

        m_sortie << NOUVELLE_LIGNE;
    }

    void cmove(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmove", 8);
    }

    void cmovne(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmovne", 8);
    }

    void cmovl(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmovl", 8);
    }

    void cmovle(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmovle", 8);
    }

    void cmovg(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmovg", 8);
    }

    void cmovge(Opérande dst, Opérande src)
    {
        génère_code_opération_binaire(dst, src, "cmovge", 8);
    }

    void cvttss2si(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        m_sortie << TABULATION << "cvttss2si ";
        imprime_opérande(dst, taille_octet);
        m_sortie << ", ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void cvtsd2ss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "cvtsd2ss ";
        imprime_opérande(dst, 4);
        m_sortie << ", ";
        imprime_opérande(src, 8);
        m_sortie << NOUVELLE_LIGNE;
    }

    void test(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "test ";
        imprime_opérande(dst);
        m_sortie << ", ";
        imprime_opérande(src);
        m_sortie << NOUVELLE_LIGNE;
    }

    void push(Opérande src)
    {
        m_sortie << TABULATION << "push ";
        imprime_opérande(src);
        m_sortie << NOUVELLE_LIGNE;
    }

    void pop(Opérande dst)
    {
        m_sortie << TABULATION << "pop ";
        imprime_opérande(dst);
        m_sortie << NOUVELLE_LIGNE;
    }

    void ret()
    {
        m_sortie << TABULATION << "ret" << NOUVELLE_LIGNE;
    }

    void syscall()
    {
        m_sortie << TABULATION << "syscall" << NOUVELLE_LIGNE;
    }

    void ud2()
    {
        m_sortie << TABULATION << "ud2" << NOUVELLE_LIGNE;
    }

    void cbw()
    {
        m_sortie << TABULATION << "cbw" << NOUVELLE_LIGNE;
    }

    void cwd()
    {
        m_sortie << TABULATION << "cwd" << NOUVELLE_LIGNE;
    }

    void cdq()
    {
        m_sortie << TABULATION << "cdq" << NOUVELLE_LIGNE;
    }

    void cqo()
    {
        m_sortie << TABULATION << "cqo" << NOUVELLE_LIGNE;
    }

  private:
    void imprime_opérande(Opérande opérande, uint32_t taille_octet = 8)
    {
        switch (opérande.type) {
            case TypeOpérande::IMMÉDIATE8:
            {
                m_sortie << "byte " << uint32_t(opérande.immédiate8.valeur);
                return;
            }
            case TypeOpérande::IMMÉDIATE16:
            {
                m_sortie << "word " << uint32_t(opérande.immédiate16.valeur);
                return;
            }
            case TypeOpérande::IMMÉDIATE32:
            {
                m_sortie << "dword " << opérande.immédiate32.valeur;
                return;
            }
            case TypeOpérande::IMMÉDIATE64:
            {
                m_sortie << "qword " << opérande.immédiate64.valeur;
                return;
            }
            case TypeOpérande::MÉMOIRE:
            {
                auto const adresse = opérande.mémoire.adresse;
                assert(adresse.taille() != 0);
                auto const décalage = opérande.mémoire.décalage;
                m_sortie << "[" << adresse;
                if (décalage < 0) {
                    m_sortie << " - ";
                }
                else {
                    m_sortie << " + ";
                }
                m_sortie << abs(décalage) << "]";
                return;
            }
            case TypeOpérande::REGISTRE:
            {
                m_sortie << chaine_pour_registre(opérande.registre, taille_octet);
                return;
            }
            case TypeOpérande::FONCTION:
            {
                m_sortie << opérande.fonction.valeur;
                return;
            }
            case TypeOpérande::GLOBALE:
            {
                m_sortie << "[" << opérande.globale.valeur << "]";
                return;
            }
        }

        m_sortie << "opérande_invalide";
    }

    void génère_code_opération_binaire(Opérande dst,
                                       Opérande src,
                                       kuri::chaine_statique nom_op,
                                       uint32_t taille_octet)
    {
        assert(!est_immédiate(dst.type));
        assert(!(dst.type == TypeOpérande::MÉMOIRE && src.type == TypeOpérande::MÉMOIRE));

        m_sortie << TABULATION << nom_op << " ";
        imprime_opérande(dst, taille_octet);
        m_sortie << ", ";
        imprime_opérande(src, taille_octet);
        m_sortie << NOUVELLE_LIGNE;
    }

    void génère_code_décalage_bits(Opérande dst,
                                   Opérande src,
                                   kuri::chaine_statique nom_op,
                                   uint32_t taille_octet)
    {
        assert(!est_immédiate(dst.type));
        assert(est_immédiate(src.type) || est_registre(src, Registre::RCX));
        m_sortie << TABULATION << nom_op << " ";
        imprime_opérande(dst, taille_octet);
        m_sortie << ", ";
        imprime_opérande(src, 1);
        m_sortie << NOUVELLE_LIGNE;
    }
};

/* Tient trace des registres pour éviter de surécrire dans un registre. */
struct GestionnaireRegistres {
  private:
    bool registres[NOMBRE_REGISTRES] = {false};

  public:
    GestionnaireRegistres()
    {
        réinitialise();
    }

    Registre donne_registre_inoccupé(Type const *type)
    {
        if (type == TypeBase::R32 || type == TypeBase::R64) {
            return donne_registre_réel_inoccupé();
        }

        return donne_registre_entier_inoccupé();
    }

    Registre donne_registre_entier_inoccupé()
    {
        return donne_registre_inoccupé(0, NOMBRE_REGISTRES_ENTIER);
    }

    Registre donne_registre_réel_inoccupé()
    {
        return donne_registre_inoccupé(NOMBRE_REGISTRES_ENTIER, NOMBRE_REGISTRES);
    }

  private:
    Registre donne_registre_inoccupé(int index_début, int index_fin)
    {
        for (auto i = index_début; i < index_fin; i++) {
            if (registres[i]) {
                continue;
            }

            registres[i] = true;
            return static_cast<Registre>(i);
        }

        assert(false);
    }

  public:
    bool registre_est_occupé(Registre registre) const
    {
        return registres[static_cast<int>(registre)];
    }

    void marque_registre_occupé(Registre registre)
    {
        registres[static_cast<int>(registre)] = true;
    }

    void marque_registre_inoccupé(Registre registre)
    {
        assert(registre != Registre::RSP);
        registres[static_cast<int>(registre)] = false;
    }

    [[nodiscard]] std::array<bool, NOMBRE_REGISTRES> sauvegarde_état() const
    {
        std::array<bool, NOMBRE_REGISTRES> résultat;
        POUR_INDEX (registres) {
            résultat[size_t(index_it)] = it;
        }
        return résultat;
    }

    void restaure_état(std::array<bool, NOMBRE_REGISTRES> sauvegarde)
    {
        POUR_INDEX (sauvegarde) {
            registres[index_it] = it;
        }
    }

    void réinitialise()
    {
        POUR (registres) {
            it = false;
        }

        marque_registre_occupé(Registre::RSP);
    }
};

class SauveRegistres {
    GestionnaireRegistres &m_registres;
    std::array<bool, NOMBRE_REGISTRES> sauvegarde{};

  public:
    SauveRegistres(GestionnaireRegistres &registres) : m_registres(registres)
    {
        sauvegarde = m_registres.sauvegarde_état();
    }

    ~SauveRegistres()
    {
        m_registres.restaure_état(sauvegarde);
    }
};

#define SAUVEGARDE_REGISTRES(x) SauveRegistres sauve_registres(x)

struct GénératriceCodeASM {
  private:
    kuri::tableau<AssembleuseASM::Opérande> table_valeurs{};
    kuri::table_hachage<Atome const *, kuri::chaine> table_globales{"Valeurs globales ASM"};
    AtomeFonction const *m_fonction_courante = nullptr;
    ClassementArgument m_classement_fonction_courante{};

    /* Si la valeur de retour doit être retournée en mémoire. */
    AssembleuseASM::Mémoire m_adresse_retour{};

    Enchaineuse enchaineuse_tmp{};
    Enchaineuse stockage_chn{};

    GestionnaireRegistres registres{};
    uint32_t taille_allouée = 0;

    Broyeuse broyeuse{};
    ClassifieuseArgument m_classifieuse{};

  public:
    AssembleuseASM::Opérande génère_code_pour_atome(Atome const *atome,
                                                    AssembleuseASM &assembleuse,
                                                    const UtilisationAtome utilisation);

    AssembleuseASM::Opérande génère_code_pour_atome_opérande(Atome const *atome,
                                                             AssembleuseASM &assembleuse,
                                                             const UtilisationAtome utilisation);

    void génère_code_pour_initialisation_globale(Atome const *initialisateur,
                                                 Enchaineuse &enchaineuse,
                                                 int profondeur);

    void génère_code_pour_instruction(Instruction const *inst,
                                      AssembleuseASM &assembleuse,
                                      const UtilisationAtome utilisation);

    void génère_code_pour_appel(const InstructionAppel *appel,
                                AssembleuseASM &assembleuse,
                                UtilisationAtome const utilisation);

    template <bool est_relatif, bool retourne_reste>
    void génère_code_pour_division(AssembleuseASM &assembleuse,
                                   AssembleuseASM::Opérande dst,
                                   AssembleuseASM::Opérande gauche,
                                   AssembleuseASM::Opérande droite,
                                   Type const *type);

    void génère_code_pour_opération_binaire(const InstructionOpBinaire *inst_bin,
                                            AssembleuseASM &assembleuse,
                                            const UtilisationAtome utilisation);

    void génère_code(ProgrammeRepreInter const &repr_inter_programme, Enchaineuse &os);

    /* Sauvegarde/restaure les registres devant être préservés à travers un appel.
     * @Long-terme : ne préserve que les registres que nous modifions. */
    void sauvegarde_registres_appel(AssembleuseASM &assembleuse);
    void restaure_registres_appel(AssembleuseASM &assembleuse);

  private:
    template <typename... Ts>
    kuri::chaine_statique enchaine(Ts &&...ts)
    {
        enchaineuse_tmp.réinitialise();
        ((enchaineuse_tmp << ts), ...);
        return stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());
    }

    void imprime_inst_en_commentaire(Enchaineuse &os, const Instruction *inst);
    void définis_fonction_courante(const AtomeFonction *fonction);

    AssembleuseASM::Mémoire alloue_variable(Type const *type_alloué);
    AssembleuseASM::Mémoire donne_adresse_stack();

    void génère_code_pour_fonction(const AtomeFonction *it,
                                   AssembleuseASM &assembleuse,
                                   Enchaineuse &os);

    void génère_code_pour_retourne(const InstructionRetour *inst_retour,
                                   AssembleuseASM &assembleuse);

    void génère_code_pour_accès_index(InstructionAccèdeIndex const *accès,
                                      AssembleuseASM &assembleuse);

    void génère_code_pour_transtype(InstructionTranstype const *transtype,
                                    AssembleuseASM &assembleuse);

    void génère_code_pour_branche_condition(const InstructionBrancheCondition *inst_branche,
                                            AssembleuseASM &assembleuse);

    void génère_code_pour_charge_mémoire(InstructionChargeMem const *inst_charge,
                                         AssembleuseASM &assembleuse,
                                         UtilisationAtome utilisation);

    void génère_code_pour_stocke_mémoire(InstructionStockeMem const *inst_stocke,
                                         AssembleuseASM &assembleuse,
                                         UtilisationAtome utilisation);

    void copie(AssembleuseASM::Opérande dest,
               AssembleuseASM::Opérande src,
               uint32_t taille_octet,
               AssembleuseASM &assembleuse);
};

AssembleuseASM::Opérande GénératriceCodeASM::génère_code_pour_atome(
    Atome const *atome, AssembleuseASM &assembleuse, const UtilisationAtome utilisation)
{
    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            auto atome_fonc = atome->comme_fonction();
            return AssembleuseASM::Fonction{atome_fonc->nom};
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            VERIFIE_NON_ATTEINT;
            return {};
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            VERIFIE_NON_ATTEINT;
            return {};
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            return AssembleuseASM::Immédiate64{0};
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            VERIFIE_NON_ATTEINT;
            return {};
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            VERIFIE_NON_ATTEINT;
            return {};
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            VERIFIE_NON_ATTEINT;
            return {};
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto const type = constante_réelle->type;
            if (type->taille_octet == 2) {
                return AssembleuseASM::Immédiate16{
                    static_cast<uint16_t>(constante_réelle->valeur)};
            }
            if (type->taille_octet == 4) {
                auto valeur_float = float(constante_réelle->valeur);
                auto bits = *reinterpret_cast<uint32_t *>(&valeur_float);
                return AssembleuseASM::Immédiate32{bits};
            }
            auto bits = *reinterpret_cast<const uint64_t *>(&constante_réelle->valeur);
            return AssembleuseASM::Immédiate64{bits};
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            auto const type = constante_entière->type;
            if (type->taille_octet == 1) {
                return AssembleuseASM::Immédiate8{static_cast<uint8_t>(constante_entière->valeur)};
            }
            if (type->taille_octet == 2) {
                return AssembleuseASM::Immédiate16{
                    static_cast<uint16_t>(constante_entière->valeur)};
            }
            if (type->taille_octet == 4) {
                return AssembleuseASM::Immédiate32{
                    static_cast<uint32_t>(constante_entière->valeur)};
            }
            return AssembleuseASM::Immédiate64{constante_entière->valeur};
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            return AssembleuseASM::Immédiate8{constante_booléenne->valeur};
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            return AssembleuseASM::Immédiate8{static_cast<uint8_t>(caractère->valeur)};
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto const structure = atome->comme_constante_structure();
            auto const type = structure->type->comme_type_composé();
            auto const tableau_valeur = structure->donne_atomes_membres();

            auto résultat = alloue_variable(type);

            POUR_INDEX (type->donne_membres_pour_code_machine()) {
                auto dst = résultat;
                dst.décalage = int32_t(it.decalage);

                auto valeur = génère_code_pour_atome(
                    tableau_valeur[index_it], assembleuse, UtilisationAtome::AUCUNE);

                if (AssembleuseASM::est_immédiate(valeur.type)) {
                    assembleuse.mov(dst, valeur, it.type->taille_octet);
                }
                else {
                    copie(dst, valeur, it.type->taille_octet, assembleuse);
                }
            }

            return résultat;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            VERIFIE_NON_ATTEINT;
            return {};  // "";
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            VERIFIE_NON_ATTEINT;
            return {};  // "";
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            return {};  // "";
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            return {};  // "";
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = atome->comme_instruction();
            génère_code_pour_instruction(
                inst, assembleuse, utilisation | UtilisationAtome::POUR_OPÉRANDE);
            return table_valeurs[inst->numero];
        }
        case Atome::Genre::GLOBALE:
        {
            auto globale = atome->comme_globale();
            return AssembleuseASM::Mémoire{globale->ident->nom_broye};
        }
    }

    return {};
}

AssembleuseASM::Opérande GénératriceCodeASM::génère_code_pour_atome_opérande(
    Atome const *opérande, AssembleuseASM &assembleuse, const UtilisationAtome utilisation)
{
    auto atome = donne_source_charge_ou_atome(opérande);

    auto valeur_atome = génère_code_pour_atome(atome, assembleuse, utilisation);

    if (est_accès_index(atome)) {
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, valeur_atome, 8);
        valeur_atome = AssembleuseASM::Mémoire(registre);
    }

    return valeur_atome;
}

bool est_initialisateur_supporté(Atome const *atome)
{
    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        case Atome::Genre::TRANSTYPE_CONSTANT:
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        case Atome::Genre::CONSTANTE_TYPE:
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        case Atome::Genre::CONSTANTE_RÉELLE:
        case Atome::Genre::CONSTANTE_STRUCTURE:
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        case Atome::Genre::INITIALISATION_TABLEAU:
        case Atome::Genre::NON_INITIALISATION:
        case Atome::Genre::INSTRUCTION:
        case Atome::Genre::GLOBALE:
        {
            return false;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        case Atome::Genre::CONSTANTE_NULLE:
        {
            return true;
        }
    }
    return false;
}

static kuri::chaine_statique chaine_indentations_espace(int indentations)
{
    static std::string chaine = std::string(1024, ' ');
    return {chaine.c_str(), static_cast<int64_t>(indentations * 4)};
}

void GénératriceCodeASM::génère_code_pour_initialisation_globale(Atome const *initialisateur,
                                                                 Enchaineuse &enchaineuse,
                                                                 int profondeur)
{
    switch (initialisateur->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            auto atome_fonc = initialisateur->comme_fonction();
            enchaineuse << chaine_indentations_espace(profondeur) << "dq " << atome_fonc->nom
                        << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = initialisateur->comme_transtype_constant();
            génère_code_pour_initialisation_globale(transtype->valeur, enchaineuse, profondeur);
            return;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto index_constant = initialisateur->comme_accès_index_constant();
            assert(index_constant->index == 0);
            génère_code_pour_initialisation_globale(
                index_constant->accédé, enchaineuse, profondeur);
            return;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            enchaineuse << chaine_indentations_espace(profondeur) << "dq 0" << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            auto type = initialisateur->comme_index_table_type()->type_de_données;
            enchaineuse << chaine_indentations_espace(profondeur) << "dd "
                        << type->index_dans_table_types << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = initialisateur->comme_taille_de()->type_de_données;
            enchaineuse << chaine_indentations_espace(profondeur) << "dd " << type->taille_octet
                        << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = initialisateur->comme_constante_entière();
            auto const type = constante_entière->type;
            enchaineuse << chaine_indentations_espace(profondeur);
            if (type->taille_octet == 1) {
                enchaineuse << "db " << uint8_t(constante_entière->valeur) << NOUVELLE_LIGNE;
                return;
            }
            if (type->taille_octet == 2) {
                enchaineuse << "dw " << uint16_t(constante_entière->valeur) << NOUVELLE_LIGNE;
                return;
            }
            if (type->taille_octet == 4) {
                enchaineuse << "dd " << uint32_t(constante_entière->valeur) << NOUVELLE_LIGNE;
                return;
            }
            enchaineuse << "dq " << constante_entière->valeur << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = initialisateur->comme_constante_booléenne();
            enchaineuse << chaine_indentations_espace(profondeur) << "db "
                        << uint8_t(constante_booléenne->valeur) << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = initialisateur->comme_constante_caractère();
            enchaineuse << chaine_indentations_espace(profondeur) << "db "
                        << uint8_t(caractère->valeur) << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = initialisateur->comme_constante_structure();
            auto type = structure->type->comme_type_composé();
            auto tableau_valeur = structure->donne_atomes_membres();
            auto nom_structure = chaine_type(type);
            nom_structure = broyeuse.broye_nom_simple(nom_structure);

            if (type->est_type_tranche()) {
                nom_structure = "tranche";
            }

            enchaineuse << chaine_indentations_espace(profondeur) << "istruc " << nom_structure
                        << NOUVELLE_LIGNE;

            auto décalage = uint32_t(0);
            auto nombre_rembourrage = 0;

            POUR_INDEX (type->donne_membres_pour_code_machine()) {
                if (it.decalage != décalage) {
                    auto rembourrage = it.decalage - décalage;
                    enchaineuse << chaine_indentations_espace(profondeur + 1) << "at "
                                << nom_structure << ".rembourrage" << nombre_rembourrage;

                    auto virgule = ", db ";
                    for (int i = 0; i < rembourrage; i++) {
                        enchaineuse << virgule << "0";
                        virgule = ", ";
                    }

                    enchaineuse << NOUVELLE_LIGNE;

                    décalage += rembourrage;
                    nombre_rembourrage++;
                }

                enchaineuse << chaine_indentations_espace(profondeur + 1) << "at " << nom_structure
                            << ".";

                if (it.nom == ID::chaine_vide) {
                    enchaineuse << "membre_invisible";
                }
                else {
                    enchaineuse << broyeuse.broye_nom_simple(it.nom);
                }

                enchaineuse << NOUVELLE_LIGNE;
                génère_code_pour_initialisation_globale(
                    tableau_valeur[index_it], enchaineuse, profondeur + 2);

                décalage += it.type->taille_octet;
            }

            if (type->taille_octet != décalage) {
                auto rembourrage = type->taille_octet - décalage;
                enchaineuse << chaine_indentations_espace(profondeur + 1) << "at " << nom_structure
                            << ".rembourrage" << nombre_rembourrage;

                auto virgule = ", db ";
                for (int i = 0; i < rembourrage; i++) {
                    enchaineuse << virgule << "0";
                    virgule = ", ";
                }

                enchaineuse << NOUVELLE_LIGNE;
            }

            enchaineuse << chaine_indentations_espace(profondeur) << "iend" << NOUVELLE_LIGNE;
            return;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau = initialisateur->comme_constante_tableau();
            auto éléments = tableau->donne_atomes_éléments();

            POUR (éléments) {
                génère_code_pour_initialisation_globale(it, enchaineuse, profondeur + 1);
            }

            return;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::INSTRUCTION:
        {
            VERIFIE_NON_ATTEINT;
            return;
        }
        case Atome::Genre::GLOBALE:
        {
            enchaineuse << chaine_indentations_espace(profondeur) << "dq "
                        << table_globales.valeur_ou(initialisateur, "") << NOUVELLE_LIGNE;
            return;
        }
    }
}

void GénératriceCodeASM::génère_code_pour_instruction(const Instruction *inst,
                                                      AssembleuseASM &assembleuse,
                                                      UtilisationAtome const utilisation)
{
    switch (inst->genre) {
        case GenreInstruction::ALLOCATION:
        {
            if ((utilisation & UtilisationAtome::POUR_OPÉRANDE) != UtilisationAtome::AUCUNE) {
                return;
            }

            auto type_alloué = inst->comme_alloc()->donne_type_alloué();
            table_valeurs[inst->numero] = alloue_variable(type_alloué);
            break;
        }
        case GenreInstruction::APPEL:
        {
            génère_code_pour_appel(inst->comme_appel(), assembleuse, utilisation);
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();

            /* N'émettons pas de saut si la cible est la prochaine instruction. */
            if (inst_branche->label->numero == inst->numero + 1) {
                return;
            }

            assembleuse.jump(inst_branche->label->id);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto const inst_branche = inst->comme_branche_cond();
            génère_code_pour_branche_condition(inst_branche, assembleuse);
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            génère_code_pour_charge_mémoire(inst->comme_charge(), assembleuse, utilisation);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            génère_code_pour_stocke_mémoire(inst->comme_stocke_mem(), assembleuse, utilisation);
            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            if (inst_label->id == 0) {
                /* Ce label est inutile. */
                return;
            }
            assembleuse.label(inst_label->id);
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();

            switch (inst_un->op) {
                case OpérateurUnaire::Genre::Positif:
                {
                    génère_code_pour_atome(inst_un->valeur, assembleuse, UtilisationAtome::AUCUNE);
                    VERIFIE_NON_ATTEINT;
                    break;
                }
                case OpérateurUnaire::Genre::Invalide:
                {
                    VERIFIE_NON_ATTEINT;
                    break;
                }
                case OpérateurUnaire::Genre::Négation:
                {
                    auto valeur = génère_code_pour_atome_opérande(
                        inst_un->valeur, assembleuse, UtilisationAtome::AUCUNE);
                    auto dest = alloue_variable(inst_un->type);
                    table_valeurs[inst->numero] = dest;

                    if (est_type_entier(inst_un->type)) {
                        auto registre = registres.donne_registre_entier_inoccupé();
                        assembleuse.mov(registre, valeur, inst_un->type->taille_octet);
                        assembleuse.neg(registre, inst_un->type->taille_octet);
                        assembleuse.mov(dest, registre, inst_un->type->taille_octet);
                        registres.marque_registre_inoccupé(registre);
                    }
                    else if (inst_un->type == TypeBase::R32) {
                        assembleuse.movss(Registre::XMM0, valeur);
                        assembleuse.movss(Registre::XMM1,
                                          AssembleuseASM::Immédiate64{uint64_t(-2147483648)});
                        assembleuse.xorps(Registre::XMM0, Registre::XMM1);
                        assembleuse.movss(dest, Registre::XMM0);
                    }
                    else {
                        VERIFIE_NON_ATTEINT;
                    }

                    break;
                }
                case OpérateurUnaire::Genre::Négation_Binaire:
                {
                    assembleuse.not_(OPERANDE2);
                    VERIFIE_NON_ATTEINT;
                    break;
                }
            }

            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            génère_code_pour_opération_binaire(inst_bin, assembleuse, utilisation);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            génère_code_pour_retourne(inst->comme_retour(), assembleuse);
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            génère_code_pour_accès_index(inst->comme_acces_index(), assembleuse);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto const accès = inst->comme_acces_membre();
            auto const accédé = accès->accédé;

            auto const valeur_accédé = génère_code_pour_atome(
                accédé, assembleuse, UtilisationAtome::AUCUNE);
            assert_rappel(valeur_accédé.type == TypeOpérande::MÉMOIRE,
                          [&]() { dbg() << "La valeur est de type " << valeur_accédé.type; });

            auto const &membre = accès->donne_membre_accédé();

            auto résultat = valeur_accédé.mémoire;
            résultat.décalage += int32_t(membre.decalage);
            table_valeurs[accès->numero] = résultat;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            génère_code_pour_transtype(inst->comme_transtype(), assembleuse);
            break;
        }
        case GenreInstruction::INATTEIGNABLE:
        {
            assembleuse.ud2();
            break;
        }
        case GenreInstruction::SÉLECTION:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
    }
}

void GénératriceCodeASM::génère_code_pour_appel(const InstructionAppel *appel,
                                                AssembleuseASM &assembleuse,
                                                UtilisationAtome const utilisation)
{
    /* Évite de générer deux fois le code pour les appels : une fois dans la boucle sur les
     * instructions, une fois pour l'opérande. Les fonctions retournant « rien » ne peuvent
     * être opérandes. */
    if (!appel->type->est_type_rien() &&
        ((utilisation & UtilisationAtome::POUR_OPÉRANDE) == UtilisationAtome::AUCUNE)) {
        return;
    }

    SAUVEGARDE_REGISTRES(registres);

    auto classement = m_classifieuse.donne_classement_arguments(
        appel->appelé->type->comme_type_fonction());

    auto type_retour = appel->type;
    auto adresse_retour = AssembleuseASM::Mémoire{};
    if (!type_retour->est_type_rien()) {
        adresse_retour = alloue_variable(type_retour);
        table_valeurs[appel->numero] = adresse_retour;
    }

    POUR_INDEX (appel->args) {
        auto classement_arg = classement.arguments[index_it];
        if (classement_arg.est_en_mémoire) {
            continue;
        }

        if (est_adresse_locale(it)) {
            assert(classement_arg.premier_huitoctet_inclusif ==
                   classement_arg.dernier_huitoctet_exclusif - 1);
            auto registre = classement
                                .registres_huitoctets[classement_arg.premier_huitoctet_inclusif]
                                .registre;
            // Nous prenons l'adresse d'une variable.
            auto adresse_source = génère_code_pour_atome(
                it, assembleuse, UtilisationAtome::AUCUNE);
            assembleuse.lea(registre, adresse_source);
            registres.marque_registre_occupé(registre);
            continue;
        }

        if (est_adresse_globale(it)) {
            assert(classement_arg.premier_huitoctet_inclusif ==
                   classement_arg.dernier_huitoctet_exclusif - 1);
            auto registre = classement
                                .registres_huitoctets[classement_arg.premier_huitoctet_inclusif]
                                .registre;
            auto adresse_source = génère_code_pour_atome(
                it, assembleuse, UtilisationAtome::AUCUNE);
            assembleuse.mov(registre, adresse_source, 8);
            registres.marque_registre_occupé(registre);
            continue;
        }

        auto atome_argument = donne_source_charge_ou_atome(it);
        auto adresse_source = génère_code_pour_atome(
            atome_argument, assembleuse, UtilisationAtome::AUCUNE);

        if (AssembleuseASM::est_immédiate(adresse_source.type)) {
            assert(classement_arg.premier_huitoctet_inclusif ==
                   classement_arg.dernier_huitoctet_exclusif - 1);
            auto registre = classement
                                .registres_huitoctets[classement_arg.premier_huitoctet_inclusif]
                                .registre;

            if (it->type->est_type_réel()) {
                assert(it->type == TypeBase::R32);
                assembleuse.movss(registre, adresse_source);
            }
            else {
                assembleuse.mov(registre, adresse_source, it->type->taille_octet);
            }

            registres.marque_registre_occupé(registre);
            continue;
        }

        auto taille_en_octet = it->type->taille_octet;

        for (auto i = classement_arg.premier_huitoctet_inclusif;
             i < classement_arg.dernier_huitoctet_exclusif;
             i++) {
            auto huitoctet = classement.huitoctets[i];
            auto classe = huitoctet.classe;

            auto registre = classement.registres_huitoctets[i].registre;

            auto taille_à_copier = taille_en_octet;
            if (taille_à_copier > 8) {
                taille_à_copier = 8;
                taille_en_octet -= 8;
            }

            registres.marque_registre_occupé(registre);
            if (classe == ClasseArgument::SSE) {
                if (it->type == TypeBase::R32) {
                    assembleuse.movss(registre, adresse_source);
                }
                else {
                    assert(it->type == TypeBase::R64);
                    assembleuse.movsd(registre, adresse_source);
                }
            }
            else {
                assert(classe == ClasseArgument::INTEGER);
                assembleuse.mov(registre, adresse_source, taille_à_copier);
            }

            adresse_source.mémoire.décalage += int32_t(taille_à_copier);
        }
    }

    auto atome_appelée = donne_source_charge_ou_atome(appel->appelé);

    auto appelée = génère_code_pour_atome(atome_appelée, assembleuse, UtilisationAtome::AUCUNE);

    if (classement.sortie.est_en_mémoire) {
        /* Charge l'adresse dans %rdi. */
        assembleuse.lea(Registre::RDI, adresse_retour);
        registres.marque_registre_occupé(Registre::RDI);
    }

    if (appelée.type == TypeOpérande::MÉMOIRE) {
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, appelée, 8);
        appelée = registre;
    }

    struct OpérandeTaillée {
        AssembleuseASM::Opérande opérande{};
        Type const *type = 0;
    };

    kuri::tablet<OpérandeTaillée, 6> arguments_mémoire;
    auto sauvegarde_taille_allouée = taille_allouée;

    POUR_INDEX (appel->args) {
        auto classement_arg = classement.arguments[index_it];
        if (!classement_arg.est_en_mémoire) {
            continue;
        }

        if (est_adresse_locale(it)) {
            assert(classement_arg.premier_huitoctet_inclusif ==
                   classement_arg.dernier_huitoctet_exclusif - 1);
            // Nous prenons l'adresse d'une variable.
            auto adresse_source = génère_code_pour_atome(
                it, assembleuse, UtilisationAtome::AUCUNE);
            auto tmp = alloue_variable(TypeBase::PTR_RIEN);
            auto registre = registres.donne_registre_entier_inoccupé();
            assembleuse.lea(registre, adresse_source);
            assembleuse.mov(tmp, registre, 8);
            arguments_mémoire.ajoute({tmp, TypeBase::PTR_RIEN});
            registres.marque_registre_inoccupé(registre);
            continue;
        }

        if (est_adresse_globale(it)) {
            assert(classement_arg.premier_huitoctet_inclusif ==
                   classement_arg.dernier_huitoctet_exclusif - 1);
            auto adresse_source = génère_code_pour_atome(
                it, assembleuse, UtilisationAtome::AUCUNE);
            arguments_mémoire.ajoute({adresse_source, TypeBase::PTR_RIEN});
            continue;
        }

        auto atome_argument = donne_source_charge_ou_atome(it);
        auto adresse_source = génère_code_pour_atome(
            atome_argument, assembleuse, UtilisationAtome::AUCUNE);
        arguments_mémoire.ajoute({adresse_source, it->type});
    }

    for (auto i = arguments_mémoire.taille() - 1; i >= 0; i--) {
        auto arg = arguments_mémoire[i];
        auto tmp = alloue_variable(arg.type);
        copie(tmp, arg.opérande, arg.type->taille_octet, assembleuse);
    }

    /* Préserve note pile. */
    assembleuse.sub(Registre::RSP, AssembleuseASM::Immédiate64{taille_allouée}, 8);

    assembleuse.call(appelée);

    /* Restaure note pile. */
    assembleuse.add(Registre::RSP, AssembleuseASM::Immédiate64{taille_allouée}, 8);

    if (!type_retour->est_type_rien() && !classement.sortie.est_en_mémoire) {
        auto taille_en_octet = type_retour->taille_octet;

        for (auto i = classement.sortie.premier_huitoctet_inclusif;
             i < classement.sortie.dernier_huitoctet_exclusif;
             i++) {
            auto huitoctet = classement.huitoctets[i];
            auto classe = huitoctet.classe;

            auto registre = classement.registres_huitoctets[i].registre;

            auto taille_à_copier = taille_en_octet;
            if (taille_à_copier > 8) {
                taille_à_copier = 8;
                taille_en_octet -= 8;
            }

            if (classe == ClasseArgument::SSE) {
                if (type_retour == TypeBase::R32) {
                    assembleuse.movss(adresse_retour, registre);
                }
                else {
                    assert(type_retour == TypeBase::R64);
                    assembleuse.movsd(adresse_retour, registre);
                }
            }
            else {
                assert(classe == ClasseArgument::INTEGER);
                assembleuse.mov(adresse_retour, registre, taille_à_copier);
            }

            adresse_retour.décalage += int32_t(taille_à_copier);
        }
    }

    taille_allouée = sauvegarde_taille_allouée;
}

template <bool est_relatif, bool retourne_reste>
void GénératriceCodeASM::génère_code_pour_division(AssembleuseASM &assembleuse,
                                                   AssembleuseASM::Opérande dst,
                                                   AssembleuseASM::Opérande gauche,
                                                   AssembleuseASM::Opérande droite,
                                                   Type const *type)
{
    auto const taille_octet = type->taille_octet;

    auto sauvegarde_eax = std::optional<AssembleuseASM::Mémoire>();
    auto sauvegarde_edx = std::optional<AssembleuseASM::Mémoire>();

    if (registres.registre_est_occupé(Registre::RAX)) {
        auto tmp = alloue_variable(TypeBase::N64);
        assembleuse.mov(tmp, Registre::RAX, 8);
        sauvegarde_eax = tmp;
    }

    if (registres.registre_est_occupé(Registre::RDX)) {
        auto tmp = alloue_variable(TypeBase::N64);
        assembleuse.mov(tmp, Registre::RDX, 8);
        sauvegarde_edx = tmp;
    }

    registres.marque_registre_occupé(Registre::RAX);
    registres.marque_registre_occupé(Registre::RDX);

    auto registre_droite = std::optional<Registre>();
    if (AssembleuseASM::est_immédiate(droite.type)) {
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, droite, taille_octet);
        registre_droite = registre;
        droite = registre;
    }

    assembleuse.mov(Registre::RAX, gauche, taille_octet);

    if (est_relatif) {
        if (taille_octet == 8) {
            assembleuse.cqo();
        }
        else if (taille_octet == 4) {
            assembleuse.cdq();
        }
        else if (taille_octet == 2) {
            assembleuse.cwd();
        }
        else {
            assembleuse.cbw();
        }
    }
    else {
        assembleuse.mov(Registre::RDX, AssembleuseASM::Immédiate64{0}, 8);
    }

    if (est_relatif) {
        assembleuse.idiv(droite, taille_octet);
    }
    else {
        assembleuse.div(droite, taille_octet);
    }

    if (retourne_reste) {
        if (taille_octet == 1) {
            assembleuse.mov_ah(dst);
        }
        else {
            assembleuse.mov(dst, Registre::RDX, taille_octet);
        }
    }
    else {
        assembleuse.mov(dst, Registre::RAX, taille_octet);
    }

    if (sauvegarde_edx.has_value()) {
        assembleuse.mov(Registre::RDX, sauvegarde_edx.value(), 8);
        taille_allouée -= 8;
    }
    else {
        registres.marque_registre_inoccupé(Registre::RDX);
    }

    if (sauvegarde_eax.has_value()) {
        assembleuse.mov(Registre::RAX, sauvegarde_eax.value(), 8);
        taille_allouée -= 8;
    }
    else {
        registres.marque_registre_inoccupé(Registre::RAX);
    }

    if (registre_droite.has_value()) {
        registres.marque_registre_inoccupé(registre_droite.value());
    }
}

void GénératriceCodeASM::génère_code_pour_opération_binaire(InstructionOpBinaire const *inst_bin,
                                                            AssembleuseASM &assembleuse,
                                                            UtilisationAtome const utilisation)
{
    auto sauvegarde = registres.sauvegarde_état();

    auto dest = alloue_variable(inst_bin->type);
    table_valeurs[inst_bin->numero] = dest;

    auto opérande_droite = génère_code_pour_atome_opérande(
        inst_bin->valeur_droite, assembleuse, UtilisationAtome::AUCUNE);
    auto opérande_gauche = génère_code_pour_atome_opérande(
        inst_bin->valeur_gauche, assembleuse, UtilisationAtome::AUCUNE);

#define GENERE_CODE_INST_ENTIER(nom_inst)                                                         \
    auto registre_résultat = registres.donne_registre_entier_inoccupé();                          \
    assembleuse.mov(                                                                              \
        registre_résultat, opérande_gauche, inst_bin->valeur_gauche->type->taille_octet);         \
    assembleuse.nom_inst(registre_résultat, opérande_droite, inst_bin->type->taille_octet);       \
    assembleuse.mov(dest, registre_résultat, inst_bin->type->taille_octet);

#define GENERE_CODE_INST_R32(nom_inst)                                                            \
    assembleuse.movss(Registre::XMM0, opérande_gauche);                                           \
    assembleuse.movss(Registre::XMM1, opérande_droite);                                           \
    assembleuse.nom_inst(Registre::XMM0, Registre::XMM1);                                         \
    assembleuse.movss(dest, Registre::XMM0);

#define GENERE_CODE_INST_R64(nom_inst)                                                            \
    assembleuse.movsd(Registre::XMM0, opérande_gauche);                                           \
    assembleuse.movsd(Registre::XMM1, opérande_droite);                                           \
    assembleuse.nom_inst(Registre::XMM0, Registre::XMM1);                                         \
    assembleuse.movsd(dest, Registre::XMM0);

#define GENERE_CODE_INST_DECALAGE_BIT(nom_inst)                                                   \
    std::optional<Registre> registre_sauvegarde_rcx;                                              \
    if (!assembleuse.est_immédiate(opérande_droite.type)) {                                       \
        if (registres.registre_est_occupé(Registre::RCX)) {                                       \
            registre_sauvegarde_rcx = registres.donne_registre_entier_inoccupé();                 \
            assembleuse.mov(registre_sauvegarde_rcx.value(), Registre::RCX, 8);                   \
        }                                                                                         \
        assembleuse.mov(Registre::RCX, opérande_droite, 1);                                       \
        opérande_droite = Registre::RCX;                                                          \
    }                                                                                             \
    else {                                                                                        \
        opérande_droite = AssembleuseASM::donne_immédiate8(opérande_droite);                      \
    }                                                                                             \
    GENERE_CODE_INST_ENTIER(nom_inst)                                                             \
    if (registre_sauvegarde_rcx.has_value()) {                                                    \
        assembleuse.mov(Registre::RCX, registre_sauvegarde_rcx.value(), 8);                       \
    }

    /* Les comparaisons sont générées en testant la valeur et utilise cmov pour mettre en place un
     * 0 ou un 1 dans le registre résultat. Ceci est le comportement désiré pour assigner depuis la
     * comparaison (a := b == c), mais pour les branches nous devrions plutôt émettre une
     * instruction idoine de branche au lieu de cmovxx. */
    auto génère_code_comparaison_entier = [&](auto &&gen_code) {
        auto const type_gauche = inst_bin->valeur_gauche->type;

        if (AssembleuseASM::est_immédiate(opérande_gauche.type)) {
            auto registre = registres.donne_registre_entier_inoccupé();
            assembleuse.mov(registre, opérande_gauche, type_gauche->taille_octet);
            opérande_gauche = registre;
        }

        if ((utilisation & UtilisationAtome::POUR_BRANCHE_CONDITION) != UtilisationAtome::AUCUNE) {
            assembleuse.cmp(opérande_gauche, opérande_droite, type_gauche->taille_octet);
            return;
        }

        /* Xor avant la comparaison afin de ne pas modifier les drapeaux du CPU. */
        auto registre_résultat = registres.donne_registre_entier_inoccupé();
        assembleuse.xor_(registre_résultat, registre_résultat, 8);

        /* Mis en place de la valeur si vrai avant la comparaison afin de ne pas modifier les
         * drapeaux du CPU. */
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, AssembleuseASM::Immédiate64{1}, 8);

        assembleuse.cmp(opérande_gauche, opérande_droite, type_gauche->taille_octet);
        gen_code(registre_résultat, registre);

        table_valeurs[inst_bin->numero] = registre_résultat;
        registres.réinitialise();
        registres.marque_registre_occupé(registre_résultat);
    };

    auto génère_code_comparaison_réel = [&](auto &&gen_code) {
        auto const type_gauche = inst_bin->valeur_gauche->type;

        if (type_gauche == TypeBase::R32) {
            assembleuse.movss(Registre::XMM0, opérande_gauche);
            assembleuse.movss(Registre::XMM1, opérande_droite);
        }
        else {
            assert(type_gauche == TypeBase::R64);
            assembleuse.movsd(Registre::XMM0, opérande_gauche);
            assembleuse.movsd(Registre::XMM1, opérande_droite);
        }

        if ((utilisation & UtilisationAtome::POUR_BRANCHE_CONDITION) != UtilisationAtome::AUCUNE) {
            if (type_gauche == TypeBase::R32) {
                assembleuse.ucomiss(Registre::XMM0, Registre::XMM1);
            }
            else {
                assert(type_gauche == TypeBase::R64);
                assembleuse.ucomisd(Registre::XMM0, Registre::XMM1);
            }
            return;
        }

        /* Xor avant la comparaison afin de ne pas modifier les drapeaux du CPU. */
        auto registre_résultat = registres.donne_registre_entier_inoccupé();
        assembleuse.xor_(registre_résultat, registre_résultat, 8);

        /* Mis en place de la valeur si vrai avant la comparaison afin de ne pas modifier les
         * drapeaux du CPU. */
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, AssembleuseASM::Immédiate64{1}, 8);

        if (type_gauche == TypeBase::R32) {
            assembleuse.ucomiss(Registre::XMM0, Registre::XMM1);
        }
        else {
            assert(type_gauche == TypeBase::R64);
            assembleuse.ucomisd(Registre::XMM0, Registre::XMM1);
        }
        gen_code(registre_résultat, registre);

        table_valeurs[inst_bin->numero] = registre_résultat;
        registres.réinitialise();
        registres.marque_registre_occupé(registre_résultat);
    };

    switch (inst_bin->op) {
        case OpérateurBinaire::Genre::Addition:
        {
            GENERE_CODE_INST_ENTIER(add);
            break;
        }
        case OpérateurBinaire::Genre::Addition_Reel:
        {
            if (inst_bin->type == TypeBase::R32) {
                GENERE_CODE_INST_R32(addss);
            }
            else {
                assert(inst_bin->type == TypeBase::R64);
                GENERE_CODE_INST_R64(addsd);
            }
            break;
        }
        case OpérateurBinaire::Genre::Soustraction:
        {
            GENERE_CODE_INST_ENTIER(sub);
            break;
        }
        case OpérateurBinaire::Genre::Soustraction_Reel:
        {
            if (inst_bin->type == TypeBase::R32) {
                GENERE_CODE_INST_R32(subss);
            }
            else {
                assert(inst_bin->type == TypeBase::R64);
                GENERE_CODE_INST_R64(subsd);
            }
            break;
        }
        case OpérateurBinaire::Genre::Multiplication:
        {
            if (inst_bin->type->est_type_entier_relatif()) {
                if (inst_bin->type->taille_octet == 1) {
                    auto sauvegarde_eax = std::optional<AssembleuseASM::Mémoire>();

                    if (registres.registre_est_occupé(Registre::RAX)) {
                        auto tmp = alloue_variable(TypeBase::N64);
                        assembleuse.mov(tmp, Registre::RAX, 8);
                        sauvegarde_eax = tmp;
                    }

                    assembleuse.mov(Registre::RAX, opérande_gauche, inst_bin->type->taille_octet);
                    assembleuse.cbw();
                    assembleuse.imul(opérande_droite, inst_bin->type->taille_octet);
                    assembleuse.mov(dest, Registre::RAX, inst_bin->type->taille_octet);

                    if (sauvegarde_eax.has_value()) {
                        assembleuse.mov(Registre::RAX, sauvegarde_eax.value(), 8);
                        taille_allouée -= 8;
                    }
                    else {
                        registres.marque_registre_inoccupé(Registre::RAX);
                    }
                }
                else {
                    GENERE_CODE_INST_ENTIER(imul);
                }
            }
            else {
                auto sauvegarde_eax = std::optional<AssembleuseASM::Mémoire>();

                if (registres.registre_est_occupé(Registre::RAX)) {
                    auto tmp = alloue_variable(TypeBase::N64);
                    assembleuse.mov(tmp, Registre::RAX, 8);
                    sauvegarde_eax = tmp;
                }

                assembleuse.mov(Registre::RAX, opérande_gauche, inst_bin->type->taille_octet);
                assembleuse.mul(opérande_droite, inst_bin->type->taille_octet);
                assembleuse.mov(dest, Registre::RAX, inst_bin->type->taille_octet);

                if (sauvegarde_eax.has_value()) {
                    assembleuse.mov(Registre::RAX, sauvegarde_eax.value(), 8);
                    taille_allouée -= 8;
                }
                else {
                    registres.marque_registre_inoccupé(Registre::RAX);
                }
            }
            break;
        }
        case OpérateurBinaire::Genre::Multiplication_Reel:
        {
            if (inst_bin->type == TypeBase::R32) {
                GENERE_CODE_INST_R32(mulss);
            }
            else {
                assert(inst_bin->type == TypeBase::R64);
                GENERE_CODE_INST_R64(mulsd);
            }
            break;
        }
        case OpérateurBinaire::Genre::Division_Naturel:
        {
            génère_code_pour_division<false, false>(
                assembleuse, dest, opérande_gauche, opérande_droite, inst_bin->type);
            break;
        }
        case OpérateurBinaire::Genre::Division_Relatif:
        {
            génère_code_pour_division<true, false>(
                assembleuse, dest, opérande_gauche, opérande_droite, inst_bin->type);
            break;
        }
        case OpérateurBinaire::Genre::Division_Reel:
        {
            if (inst_bin->type == TypeBase::R32) {
                GENERE_CODE_INST_R32(divss);
            }
            else {
                assert(inst_bin->type == TypeBase::R64);
                GENERE_CODE_INST_R64(divsd);
            }
            break;
        }
        case OpérateurBinaire::Genre::Reste_Naturel:
        {
            génère_code_pour_division<false, true>(
                assembleuse, dest, opérande_gauche, opérande_droite, inst_bin->type);
            break;
        }
        case OpérateurBinaire::Genre::Reste_Relatif:
        {
            génère_code_pour_division<true, true>(
                assembleuse, dest, opérande_gauche, opérande_droite, inst_bin->type);
            break;
        }
        case OpérateurBinaire::Genre::Comp_Egal:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmove(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inegal:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovne(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovl(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovle(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovg(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        {
            génère_code_comparaison_entier(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovge(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmove(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovne(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovl(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovle(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovg(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            génère_code_comparaison_réel(
                [&](AssembleuseASM::Opérande gauche, AssembleuseASM::Opérande droite) {
                    assembleuse.cmovge(gauche, droite);
                });
            break;
        }
        case OpérateurBinaire::Genre::Et_Binaire:
        {
            GENERE_CODE_INST_ENTIER(and_);
            break;
        }
        case OpérateurBinaire::Genre::Ou_Binaire:
        {
            GENERE_CODE_INST_ENTIER(or_);
            break;
        }
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            GENERE_CODE_INST_ENTIER(xor_);
            break;
        }
        case OpérateurBinaire::Genre::Dec_Gauche:
        {
            // À FAIRE(langage) : décalage gauche arithmétique
            GENERE_CODE_INST_DECALAGE_BIT(shl);
            break;
        }
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        {
            GENERE_CODE_INST_DECALAGE_BIT(sar);
            break;
        }
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        {
            GENERE_CODE_INST_DECALAGE_BIT(shr);
            break;
        }
        case OpérateurBinaire::Genre::Indexage:
        case OpérateurBinaire::Genre::Invalide:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
    }

#undef GENERE_CODE_INST_DECALAGE_BIT
#undef GENERE_CODE_INST_ENTIER

    registres.restaure_état(sauvegarde);
}

void GénératriceCodeASM::génère_code_pour_retourne(const InstructionRetour *inst_retour,
                                                   AssembleuseASM &assembleuse)
{
    SAUVEGARDE_REGISTRES(registres);

    if (inst_retour->valeur != nullptr) {
        auto atome_source = donne_source_charge_ou_atome(inst_retour->valeur);

        auto valeur = génère_code_pour_atome(atome_source, assembleuse, UtilisationAtome::AUCUNE);
        assert(valeur.type == TypeOpérande::MÉMOIRE);

        auto sortie = m_classement_fonction_courante.sortie;

        auto const type_retour = inst_retour->valeur->type;
        auto taille_en_octet = type_retour->taille_octet;
        if (sortie.est_en_mémoire) {
            assert(valeur.type == TypeOpérande::MÉMOIRE);

            assembleuse.mov(Registre::RAX, m_adresse_retour, 8);

            auto adresse_retour = AssembleuseASM::Mémoire(Registre::RAX);

            registres.marque_registre_occupé(Registre::RAX);
            auto registre_tmp = registres.donne_registre_entier_inoccupé();

            auto taille_à_copier = int32_t(taille_en_octet);
            while (taille_à_copier > 0) {
                auto taille = taille_à_copier;
                if (taille > 8) {
                    taille = 8;
                }
                taille_à_copier -= taille;

                assembleuse.mov(registre_tmp, valeur, uint32_t(taille));
                assembleuse.mov(adresse_retour, registre_tmp, uint32_t(taille));

                adresse_retour.décalage += taille;
                valeur.mémoire.décalage += taille;
            }
        }
        else {
            for (auto i = sortie.premier_huitoctet_inclusif; i < sortie.dernier_huitoctet_exclusif;
                 i++) {

                auto huitoctet = m_classement_fonction_courante.huitoctets[i];
                auto classe = huitoctet.classe;

                auto registre = m_classement_fonction_courante.registres_huitoctets[i].registre;

                auto taille_à_copier = taille_en_octet;
                if (taille_à_copier > 8) {
                    taille_à_copier = 8;
                    taille_en_octet -= 8;
                }

                registres.marque_registre_occupé(registre);

                if (classe == ClasseArgument::SSE) {
                    if (type_retour == TypeBase::R32) {
                        assembleuse.movss(registre, valeur);
                    }
                    else {
                        assert(type_retour == TypeBase::R64);
                        assembleuse.movsd(registre, valeur);
                    }
                }
                else {
                    assert(classe == ClasseArgument::INTEGER);
                    assembleuse.mov(registre, valeur, taille_à_copier);
                }

                valeur.mémoire.décalage += int32_t(taille_à_copier);
            }
        }
    }

    restaure_registres_appel(assembleuse);
    assembleuse.ret();
}

void GénératriceCodeASM::génère_code_pour_accès_index(InstructionAccèdeIndex const *accès,
                                                      AssembleuseASM &assembleuse)
{
    SAUVEGARDE_REGISTRES(registres);

    auto const accédé = accès->accédé;
    auto type_pointeur = accès->type->comme_type_pointeur();

    auto const valeur_accédé = génère_code_pour_atome(
        accédé, assembleuse, UtilisationAtome::AUCUNE);
    assert(valeur_accédé.type == TypeOpérande::MÉMOIRE);

    auto registre1 = registres.donne_registre_entier_inoccupé();
    auto registre2 = registres.donne_registre_entier_inoccupé();

    /* Charge l'adresse. */
    assembleuse.lea(registre1, valeur_accédé);

    /* Corrige l'index pour prendre en compte la taille du type. */
    auto const atome_index = donne_source_charge_ou_atome(accès->index);
    auto const type_accédé = type_pointeur->type_pointé;

    if (atome_index->est_constante_entière()) {
        auto const constante = atome_index->comme_constante_entière();
        auto décalage = constante->valeur * type_accédé->taille_octet;
        if (décalage != 0) {
            assembleuse.add(registre1, AssembleuseASM::Immédiate64{décalage}, 8);
        }
    }
    else {
        auto const valeur_index = génère_code_pour_atome(
            atome_index, assembleuse, UtilisationAtome::AUCUNE);
        assembleuse.mov(registre2, valeur_index, accès->index->type->taille_octet);
        assembleuse.imul(registre2, AssembleuseASM::Immédiate32{type_accédé->taille_octet}, 4);

        /* Corrige l'adresse. */
        assembleuse.add(registre1, registre2, 8);
    }

    auto résultat = alloue_variable(TypeBase::PTR_RIEN);
    assembleuse.mov(résultat, registre1, 8);

    table_valeurs[accès->numero] = résultat;
}

void GénératriceCodeASM::génère_code_pour_transtype(InstructionTranstype const *transtype,
                                                    AssembleuseASM &assembleuse)
{
    SAUVEGARDE_REGISTRES(registres);

    auto const type_de = transtype->valeur->type;
    auto const type_vers = transtype->type;
    auto valeur = génère_code_pour_atome(transtype->valeur, assembleuse, UtilisationAtome::AUCUNE);

    switch (transtype->op) {
        case TypeTranstypage::BITS:
        {
            break;
        }
        case TypeTranstypage::AUGMENTE_NATUREL:
        {
            auto registre = registres.donne_registre_entier_inoccupé();
            auto dst = alloue_variable(type_vers);
            assembleuse.movzx(registre, type_vers->taille_octet, valeur, type_de->taille_octet);
            assembleuse.mov(dst, registre, type_vers->taille_octet);
            valeur = dst;
            break;
        }
        case TypeTranstypage::AUGMENTE_RELATIF:
        {
            auto registre = registres.donne_registre_entier_inoccupé();
            auto dst = alloue_variable(type_vers);
            assembleuse.movsx(registre, type_vers->taille_octet, valeur, type_de->taille_octet);
            assembleuse.mov(dst, registre, type_vers->taille_octet);
            valeur = dst;
            break;
        }
        case TypeTranstypage::AUGMENTE_REEL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::DIMINUE_NATUREL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::DIMINUE_RELATIF:
        {
            auto registre = registres.donne_registre_entier_inoccupé();
            auto dst = alloue_variable(type_vers);
            assembleuse.mov(registre, valeur, type_de->taille_octet);
            assembleuse.mov(dst, registre, type_vers->taille_octet);
            valeur = dst;
            break;
        }
        case TypeTranstypage::DIMINUE_REEL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::AUGMENTE_NATUREL_VERS_RELATIF:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::DIMINUE_NATUREL_VERS_RELATIF:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::DIMINUE_RELATIF_VERS_NATUREL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
        {
            if (type_vers->taille_octet != 8) {
                VERIFIE_NON_ATTEINT;
            }
            break;
        }
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
        {
            if (type_de->taille_octet != 8) {
                auto registre = registres.donne_registre_entier_inoccupé();
                auto dst = alloue_variable(type_vers);
                assembleuse.movzx(
                    registre, type_vers->taille_octet, valeur, type_de->taille_octet);
                assembleuse.mov(dst, registre, type_vers->taille_octet);
                valeur = dst;
            }
            break;
        }
        case TypeTranstypage::REEL_VERS_ENTIER_RELATIF:
        {
            if (type_de->taille_octet != 4 || type_vers->taille_octet != 4) {
                VERIFIE_NON_ATTEINT;
            }

            auto registre = registres.donne_registre_entier_inoccupé();
            auto dst = alloue_variable(type_vers);

            /* À FAIRE : convertis directement les constantes réelles vers des entiers constants.
             */
            if (assembleuse.est_immédiate(valeur.type)) {
                auto tmp = alloue_variable(type_de);
                assembleuse.mov(tmp, valeur, type_de->taille_octet);
                valeur = tmp;
            }

            assembleuse.cvttss2si(registre, valeur, type_vers->taille_octet);
            assembleuse.mov(dst, registre, type_vers->taille_octet);

            valeur = dst;
            break;
        }
        case TypeTranstypage::REEL_VERS_ENTIER_NATUREL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::ENTIER_RELATIF_VERS_REEL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case TypeTranstypage::ENTIER_NATUREL_VERS_REEL:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
    }

    table_valeurs[transtype->numero] = valeur;
}

void GénératriceCodeASM::génère_code_pour_branche_condition(
    const InstructionBrancheCondition *inst_branche, AssembleuseASM &assembleuse)
{
    SAUVEGARDE_REGISTRES(registres);

    auto const prédicat = inst_branche->condition;
    auto const si_vrai = inst_branche->label_si_vrai;
    auto const si_faux = inst_branche->label_si_faux;

    auto const condition = génère_code_pour_atome(
        prédicat, assembleuse, UtilisationAtome::POUR_BRANCHE_CONDITION);

    auto génère_code_branche = [&](auto &&méthode_si_vrai, auto &&méthode_si_faux) {
        /* Ne générons qu'un seul saut si possible. */
        if (si_vrai->numero == inst_branche->numero + 1) {
            (assembleuse.*méthode_si_faux)(si_faux->id);
        }
        else if (si_faux->numero == inst_branche->numero + 1) {
            (assembleuse.*méthode_si_vrai)(si_vrai->id);
        }
        else {
            (assembleuse.*méthode_si_vrai)(si_vrai->id);
            assembleuse.jump(si_faux->id);
        }
    };

    if (est_instruction_comparaison(prédicat)) {
        auto op = prédicat->comme_instruction()->comme_op_binaire()->op;

        switch (op) {
            case OpérateurBinaire::Genre::Comp_Egal:
            case OpérateurBinaire::Genre::Comp_Egal_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_égal,
                                    &AssembleuseASM::jump_si_inégal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inegal:
            case OpérateurBinaire::Genre::Comp_Inegal_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inégal,
                                    &AssembleuseASM::jump_si_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inf:
            case OpérateurBinaire::Genre::Comp_Inf_Nat:
            case OpérateurBinaire::Genre::Comp_Inf_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inférieur,
                                    &AssembleuseASM::jump_si_supérieur_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inf_Egal:
            case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
            case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inférieur_égal,
                                    &AssembleuseASM::jump_si_supérieur);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Sup:
            case OpérateurBinaire::Genre::Comp_Sup_Nat:
            case OpérateurBinaire::Genre::Comp_Sup_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_supérieur,
                                    &AssembleuseASM::jump_si_inférieur_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Sup_Egal:
            case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
            case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
            {
                génère_code_branche(&AssembleuseASM::jump_si_supérieur_égal,
                                    &AssembleuseASM::jump_si_inférieur);
                return;
            }
            default:
            {
                assert(false);
                break;
            }
        }
    }

    assembleuse.test(condition, condition);

    /* Ne générons qu'un seul saut si possible. */
    if (si_vrai->numero == inst_branche->numero + 1) {
        assembleuse.jump_si_zéro(inst_branche->label_si_faux->id);
    }
    else if (si_faux->numero == inst_branche->numero + 1) {
        assembleuse.jump_si_non_zéro(inst_branche->label_si_vrai->id);
    }
    else {
        assembleuse.jump_si_zéro(inst_branche->label_si_faux->id);
        assembleuse.jump(inst_branche->label_si_vrai->id);
    }
}

void GénératriceCodeASM::génère_code_pour_charge_mémoire(InstructionChargeMem const *inst_charge,
                                                         AssembleuseASM &assembleuse,
                                                         UtilisationAtome utilisation)
{
    /* À FAIRE: charge vers où? */
    /* À FAIRE: movss/movsd pour les réels. */
    /* À FAIRE: vérifie la taille de la structure. */
    auto src = génère_code_pour_atome(inst_charge->chargée, assembleuse, UtilisationAtome::AUCUNE);

    auto registre = registres.donne_registre_entier_inoccupé();

    assembleuse.mov(registre, src, inst_charge->type->taille_octet);

    /* Déréférencement de pointeur. */
    AssembleuseASM::Opérande résultat = registre;
    if (inst_charge->type->est_type_pointeur()) {
        résultat = AssembleuseASM::Mémoire(registre);
    }

    table_valeurs[inst_charge->numero] = résultat;
}

void GénératriceCodeASM::génère_code_pour_stocke_mémoire(InstructionStockeMem const *inst_stocke,
                                                         AssembleuseASM &assembleuse,
                                                         UtilisationAtome utilisation)
{
    SAUVEGARDE_REGISTRES(registres);

    auto dest = génère_code_pour_atome(
        inst_stocke->destination, assembleuse, UtilisationAtome::AUCUNE);

    auto type_stocké = inst_stocke->source->type;

    if (est_accès_index(inst_stocke->destination)) {
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, dest, 8);
        dest = AssembleuseASM::Mémoire(registre);
    }

    if (est_adresse(inst_stocke->source)) {
        /* Stockage d'une adresse. */
        auto src = génère_code_pour_atome(
            inst_stocke->source, assembleuse, UtilisationAtome::AUCUNE);

        if (est_adresse_globale(inst_stocke->source)) {
            assembleuse.mov(dest, src, type_stocké->taille_octet);
            return;
        }

        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.lea(registre, src);
        assembleuse.mov(dest, registre, type_stocké->taille_octet);
        return;
    }

    auto const atome_source = donne_source_charge_ou_atome(inst_stocke->source);
    auto src = génère_code_pour_atome(atome_source, assembleuse, UtilisationAtome::AUCUNE);

    if (est_accès_index(atome_source)) {
        auto registre = registres.donne_registre_entier_inoccupé();
        assembleuse.mov(registre, src, 8);
        src = AssembleuseASM::Mémoire(registre);
    }

    copie(dest, src, type_stocké->taille_octet, assembleuse);
}

void GénératriceCodeASM::copie(AssembleuseASM::Opérande dest,
                               AssembleuseASM::Opérande src,
                               uint32_t taille_octet,
                               AssembleuseASM &assembleuse)
{
    /* À FAIRE: movss/movsd pour les réels. */
    auto registre_tmp = registres.donne_registre_entier_inoccupé();

    if (taille_octet <= 8) {
        if (assembleuse.est_immédiate(src.type)) {
            assembleuse.mov(dest, src, taille_octet);
        }
        else {
            assembleuse.mov(registre_tmp, src, taille_octet);
            assembleuse.mov(dest, registre_tmp, taille_octet);
        }
    }
    else {
        assert(src.type == TypeOpérande::MÉMOIRE);

        auto taille_à_copier = int32_t(taille_octet);
        while (taille_à_copier > 0) {
            auto taille = taille_à_copier;
            if (taille > 8) {
                taille = 8;
            }

            assembleuse.mov(registre_tmp, src, uint32_t(taille));
            assembleuse.mov(dest, registre_tmp, uint32_t(taille));
            taille_à_copier -= taille;
            dest.mémoire.décalage += taille;
            src.mémoire.décalage += taille;
        }
    }
}

static kuri::tableau<AtomeFonction *> donne_fonctions_à_compiler(
    kuri::tableau_statique<AtomeFonction *> fonctions)
{
    AtomeFonction *fonction_principale = nullptr;

    POUR (fonctions) {
        if (it->decl && it->decl->ident == ID::principale) {
            fonction_principale = it;
            break;
        }
    }

    if (!fonction_principale) {
        return {};
    }

    kuri::pile<AtomeFonction *> fonctions_à_visiter;
    kuri::ensemble<AtomeFonction *> fonctions_visitées;

    kuri::tableau<AtomeFonction *> résultat;

    fonctions_à_visiter.empile(fonction_principale);

    while (!fonctions_à_visiter.est_vide()) {
        auto fonction = fonctions_à_visiter.depile();

        if (fonctions_visitées.possède(fonction)) {
            continue;
        }

        résultat.ajoute(fonction);
        fonctions_visitées.insère(fonction);

        POUR (fonction->instructions) {
            if (it->est_appel()) {
                auto appel = it->comme_appel();
                if (appel->appelé->est_fonction()) {
                    fonctions_à_visiter.empile(appel->appelé->comme_fonction());
                }
                continue;
            }

            if (it->est_stocke_mem()) {
                auto stocke = it->comme_stocke_mem();
                if (stocke->source->est_fonction()) {
                    fonctions_à_visiter.empile(stocke->source->comme_fonction());
                }
                continue;
            }
        }
    }

    return résultat;
}

static void déclare_structure(TypeCompose const *type,
                              Broyeuse &broyeuse,
                              Enchaineuse &enchaineuse)
{
    auto nom_type = chaine_type(type);

    nom_type = broyeuse.broye_nom_simple(nom_type);

    enchaineuse << "struc " << nom_type << NOUVELLE_LIGNE;

    auto décalage = uint32_t(0);
    auto nombre_rembourrage = 0;

    POUR (type->donne_membres_pour_code_machine()) {
        auto nom_membre = broyeuse.broye_nom_simple(it.nom);

        if (it.decalage != décalage) {
            auto rembourrage = it.decalage - décalage;

            enchaineuse << TABULATION << ".rembourrage" << nombre_rembourrage << " resb "
                        << rembourrage << NOUVELLE_LIGNE;

            décalage += rembourrage;
            nombre_rembourrage++;
        }

        enchaineuse << TABULATION << "." << nom_membre << " resb " << it.type->taille_octet
                    << NOUVELLE_LIGNE;

        décalage += it.type->taille_octet;
    }

    if (type->taille_octet != décalage) {
        auto rembourrage = type->taille_octet - décalage;
        enchaineuse << TABULATION << ".rembourrage" << nombre_rembourrage << " resb "
                    << rembourrage << NOUVELLE_LIGNE;
    }

    enchaineuse << "endstruc" << NOUVELLE_LIGNE;
}

void GénératriceCodeASM::génère_code(ProgrammeRepreInter const &repr_inter_programme,
                                     Enchaineuse &os)
{
    /* Déclaration des types. */
    kuri::rassembleuse<TypeCompose const *> types_pour_globales;
    auto visiteuse_type = VisiteuseType{};

    POUR (repr_inter_programme.donne_globales()) {
        if (!it->est_constante) {
            continue;
        }

        table_globales.insère(it, broyeuse.broye_nom_simple(it->ident));

        visiteuse_type.visite_type(const_cast<Type *>(it->donne_type_alloué()), [&](Type *type) {
            if (type->est_type_structure() || type->est_type_chaine()) {
                types_pour_globales.insère(type->comme_type_composé());
            }
        });
    }

    os << "struc " << "tranche" << NOUVELLE_LIGNE;
    os << TABULATION << ".pointeur resq 1" << NOUVELLE_LIGNE;
    os << TABULATION << ".taille resq 1" << NOUVELLE_LIGNE;
    os << "endstruc" << NOUVELLE_LIGNE;
    POUR (types_pour_globales.donne_éléments()) {
        déclare_structure(it, broyeuse, os);
    }

    /* Prodéclaration des fonctions. */
    auto fonctions = repr_inter_programme.donne_fonctions();
    auto fonctions_à_compiler = donne_fonctions_à_compiler(fonctions);

    POUR (fonctions) {
        if (it->est_externe) {
            os << "extern " << it->nom << "\n";
        }
        else {
            os << "global " << it->nom << "\n";
        }
    }

    auto opt_données_constantes = repr_inter_programme.donne_données_constantes();
    if (opt_données_constantes.has_value()) {
        os << "section .data" << NOUVELLE_LIGNE;

        auto données_constantes = opt_données_constantes.value();

        os << TABULATION << TABULATION << "align " << données_constantes->alignement_désiré
           << NOUVELLE_LIGNE;
        os << TABULATION << "DC:" << NOUVELLE_LIGNE << TABULATION << TABULATION << "db ";

        auto virgule = " ";
        auto compteur = 0;
        POUR (données_constantes->tableaux_constants) {
            auto tableau = it.tableau->donne_données();

            for (auto i = 0; i < it.rembourrage; ++i) {
                compteur++;
                if ((compteur % 20) == 0) {
                    os << NOUVELLE_LIGNE << TABULATION << TABULATION << "db ";
                }
                else {
                    os << virgule;
                }
                os << "0x0";
                virgule = ", ";
            }

            POUR_NOMME (octet, tableau) {
                compteur++;
                if ((compteur % 20) == 0) {
                    os << NOUVELLE_LIGNE << TABULATION << TABULATION << "db ";
                }
                else {
                    os << virgule;
                }
                os << "0x";
                os << dls::num::char_depuis_hex((octet & 0xf0) >> 4);
                os << dls::num::char_depuis_hex(octet & 0x0f);
                virgule = ", ";
            }
        }

        os << NOUVELLE_LIGNE << NOUVELLE_LIGNE;

        POUR (données_constantes->tableaux_constants) {
            auto nom_globale = enchaine("DC + ", it.décalage_dans_données_constantes);
            table_globales.insère(it.globale, nom_globale);
        }
    }

    auto globales = repr_inter_programme.donne_globales();

    POUR (globales) {
        if (it->est_externe || !it->est_constante) {
            continue;
        }

        auto nom = broyeuse.broye_nom_simple(it->ident);
        os << TABULATION << nom << ":" << NOUVELLE_LIGNE;
        génère_code_pour_initialisation_globale(it->initialisateur, os, 1);
    }

    os << "section .bss\n";

    POUR (globales) {
        if (it->est_externe || it->est_constante) {
            continue;
        }

        auto nom = broyeuse.broye_nom_simple(it->ident);
        os << TABULATION << nom << ": resb " << it->donne_type_alloué()->taille_octet
           << NOUVELLE_LIGNE;
    }

    os << NOUVELLE_LIGNE;
    os << "section .text\n";

    POUR (globales) {
        if (it->est_externe) {
            os << "extern " << it->ident->nom << "\n";
        }
    }

    os << "\n";

    /* Définition des fonctions. */
    auto assembleuse = AssembleuseASM(os);

    POUR_INDEX (fonctions_à_compiler) {
        if (it->est_externe) {
            continue;
        }

        dbg() << "[" << index_it << " / " << fonctions_à_compiler.taille() << "] "
              << "Compilation de " << it->nom;
        génère_code_pour_fonction(it, assembleuse, os);
    }

    // Fonction de test.
    os << "global main\n";
    os << "main:\n";

    assembleuse.call(AssembleuseASM::Fonction{"principale"});
    assembleuse.ret();
}

void GénératriceCodeASM::génère_code_pour_fonction(AtomeFonction const *fonction,
                                                   AssembleuseASM &assembleuse,
                                                   Enchaineuse &os)
{
    fonction->numérote_instructions();

    os << fonction->nom << ":\n";
    définis_fonction_courante(fonction);

    /* Décale de 8 car l'adresse de l'instruction de retour se trouve à RSP. */
    taille_allouée = 8;

    sauvegarde_registres_appel(assembleuse);

    auto classement = m_classifieuse.donne_classement_arguments(
        fonction->type->comme_type_fonction());
    m_classement_fonction_courante = classement;

    if (classement.sortie.est_en_mémoire) {
        m_adresse_retour = alloue_variable(TypeBase::PTR_RIEN);
        assembleuse.mov(m_adresse_retour, Registre::RDI, 8);
    }

    /* L'appel a poussé 1 huitoctet sur la pile, et nous avons pousser 7 huitoctets
     * tsupplémentaires, donc nous devons décaler de 8 huitoctets %rsp afin de savoir où se trouve
     * les arguments. */
    auto décalage_argument_mémoire = 8 * 8;

    POUR_INDEX (fonction->params_entrée) {
        auto classement_arg = classement.arguments[index_it];
        auto type_alloué = it->donne_type_alloué();

        if (classement_arg.est_en_mémoire) {
            table_valeurs[it->numero] = AssembleuseASM::Mémoire(Registre::RSP,
                                                                décalage_argument_mémoire);
            décalage_argument_mémoire += int32_t(type_alloué->taille_octet);
            continue;
        }

        auto adresse = alloue_variable(type_alloué);
        table_valeurs[it->numero] = adresse;

        auto taille_en_octet = type_alloué->taille_octet;

        for (auto i = classement_arg.premier_huitoctet_inclusif;
             i < classement_arg.dernier_huitoctet_exclusif;
             i++) {
            auto huitoctet = classement.huitoctets[i];
            auto classe = huitoctet.classe;

            auto registre = classement.registres_huitoctets[i].registre;

            auto taille_à_copier = taille_en_octet;
            if (taille_à_copier > 8) {
                taille_à_copier = 8;
                taille_en_octet -= 8;
            }

            if (classe == ClasseArgument::SSE) {
                if (taille_à_copier == 4) {
                    assembleuse.movss(adresse, registre);
                }
                else {
                    assert(taille_à_copier == 8);
                    assembleuse.movsd(adresse, registre);
                }
            }
            else {
                assert(classe == ClasseArgument::INTEGER);
                assembleuse.mov(adresse, registre, taille_à_copier);
            }

            adresse.décalage += int32_t(taille_à_copier);
        }
    }

    auto type_fonction = fonction->type->comme_type_fonction();
    if (!type_fonction->type_sortie->est_type_rien()) {
        auto type_alloué = type_fonction->type_sortie;
        table_valeurs[fonction->param_sortie->numero] = alloue_variable(type_alloué);
    }

    POUR (fonction->instructions) {
        if (!instruction_est_racine(it) && !it->est_alloc()) {
            continue;
        }

        // imprime_inst_en_commentaire(os, it);
        génère_code_pour_instruction(it, assembleuse, UtilisationAtome::AUCUNE);
    }

    m_fonction_courante = nullptr;
    os << "\n\n";
}

/* http://www.uclibc.org/docs/psABI-x86_64.pdf */
void GénératriceCodeASM::sauvegarde_registres_appel(AssembleuseASM &assembleuse)
{
    /* r12, r13, r14, r15, rbx, rsp, rbp */
    assembleuse.push(Registre::R12);
    assembleuse.push(Registre::R13);
    assembleuse.push(Registre::R14);
    assembleuse.push(Registre::R15);
    assembleuse.push(Registre::RBX);
    assembleuse.push(Registre::RSP);
    assembleuse.push(Registre::RBP);
}

void GénératriceCodeASM::restaure_registres_appel(AssembleuseASM &assembleuse)
{
    assembleuse.pop(Registre::RBP);
    assembleuse.pop(Registre::RSP);
    assembleuse.pop(Registre::RBX);
    assembleuse.pop(Registre::R15);
    assembleuse.pop(Registre::R14);
    assembleuse.pop(Registre::R13);
    assembleuse.pop(Registre::R12);
}

void GénératriceCodeASM::définis_fonction_courante(AtomeFonction const *fonction)
{
    m_fonction_courante = fonction;
    taille_allouée = 0;
    table_valeurs.redimensionne(fonction->params_entrée.taille() + 1 +
                                fonction->instructions.taille());

    auto valeur_défaut = AssembleuseASM::Opérande(AssembleuseASM::Mémoire{});
    POUR (table_valeurs) {
        it = valeur_défaut;
    }
}

AssembleuseASM::Mémoire GénératriceCodeASM::alloue_variable(Type const *type_alloué)
{
    if ((taille_allouée % type_alloué->alignement) != 0) {
        taille_allouée += (taille_allouée % type_alloué->alignement);
    }

    auto taille_requise = type_alloué->taille_octet;
    taille_allouée += taille_requise;
    return donne_adresse_stack();
}

AssembleuseASM::Mémoire GénératriceCodeASM::donne_adresse_stack()
{
    return AssembleuseASM::Mémoire(Registre::RSP, -int32_t(taille_allouée));
}

void GénératriceCodeASM::imprime_inst_en_commentaire(Enchaineuse &os, Instruction const *inst)
{
    os << "  ; " << imprime_instruction(inst) << NOUVELLE_LIGNE;
}

std::optional<ErreurCoulisse> CoulisseASM::génère_code_impl(const ArgsGénérationCode & /*args*/)
{
    return {};
}

std::optional<ErreurCoulisse> CoulisseASM::crée_fichier_objet_impl(
    const ArgsCréationFichiersObjets &args)
{
    Enchaineuse enchaineuse;

    auto &repr_inter_programme = *args.ri_programme;

    // génère_code_debut_fichier(enchaineuse, compilatrice.racine_kuri);

    auto génératrice = GénératriceCodeASM{};
    génératrice.génère_code(repr_inter_programme, enchaineuse);

    std::ofstream of;
    of.open("/tmp/compilation_kuri_asm.asm");
    enchaineuse.imprime_dans_flux(of);
    of.close();

    auto commande = "nasm -f elf64 /tmp/compilation_kuri_asm.asm -o /tmp/compilation_kuri_asm.o";

    if (system(commande) != 0) {
        return ErreurCoulisse{"Impossible de générer les fichiers objets."};
    }

    return {};
}

std::optional<ErreurCoulisse> CoulisseASM::crée_exécutable_impl(const ArgsLiaisonObjets &args)
{
    auto commande = "gcc -no-pie -lc -o a.out /tmp/compilation_kuri_asm.o";
    if (system(commande) != 0) {
        return ErreurCoulisse{"Impossible de lier le fichier objet."};
    }

    auto résultat_exécution = system("./a.out");
    dbg() << "=================================================";
    dbg() << "Le programme a retourné :";
    dbg() << "     " << WEXITSTATUS(résultat_exécution);
    dbg() << "=================================================";
    return {};
}

int64_t CoulisseASM::mémoire_utilisée() const
{
    return 0;
}
