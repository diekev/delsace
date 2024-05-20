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
#define NOUVELLE_LIGNE "\n"

#define VERIFIE_NON_ATTEINT assert(false)

/* clang-format off */
#define OPERANDE1 AssembleuseASM::Opérande{}
#define OPERANDE2 AssembleuseASM::Opérande{}, AssembleuseASM::Opérande{}
/* clang-format on */

/* ------------------------------------------------------------------------- */
/** \name ABI x64 pour passer des paramètres
 *  https://refspecs.linuxfoundation.org/elf/x86_64-abi-0.99.pdf section 3.2.3
 * \{ */

enum class ClasseArgument : uint8_t {
    INTEGER,
    SSE,
    SSEUP,
    X87,
    X87UP,
    COMPLEX_X87,
    NO_CLASS,
    MEMORY,
};

/* La taille de chaque type doit être alignée sur 8 octets. */
static uint32_t donne_taille_alignée(Type const *type)
{
    return (type->taille_octet + 7u) & ~7u;
}

static ClasseArgument détermine_classe_argument_aggrégé(TypeCompose const *type)
{
    auto const taille = donne_taille_alignée(type);
    /* 1. If the size of an object is larger than four eightbytes, or it contains unaligned fields,
     * it has class MEMORY. */
    // À FAIRE : champs non-aligné.
    if (taille > (8 * 4)) {
        return ClasseArgument::MEMORY;
    }

    // À FAIRE : termine ça.
    assert(false);
    return ClasseArgument::NO_CLASS;
}

static ClasseArgument donne_classe_argument(Type const *type)
{
    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            return ClasseArgument::NO_CLASS;
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
            return ClasseArgument::INTEGER;
        }
        case GenreNoeud::RÉEL:
        {
            /* @Incomplet : __m128, __m256, etc. */
            return ClasseArgument::SSE;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto const type_opaque = type->comme_type_opaque();
            return donne_classe_argument(type_opaque->type_opacifié);
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::TUPLE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::DÉCLARATION_UNION:
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::TYPE_TRANCHE:
        {
            return détermine_classe_argument_aggrégé(type->comme_type_composé());
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    return ClasseArgument::NO_CLASS;
}

struct ClasseArgumentAppel {
    kuri::tableau<ClasseArgument> entrées{};
    ClasseArgument sorties{};
};

static ClasseArgumentAppel détermine_classes_arguments(TypeFonction const *type)
{
    auto résultat = ClasseArgumentAppel{};

    POUR (type->types_entrées) {
        résultat.entrées.ajoute(donne_classe_argument(it));
    }

    résultat.sorties = donne_classe_argument(type->type_sortie);

    return résultat;
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

struct AssembleuseASM {
  private:
    Enchaineuse &m_sortie;

  public:
    enum class TypeOpérande {
        REGISTRE,
        IMMÉDIATE8,
        IMMÉDIATE16,
        IMMÉDIATE32,
        IMMÉDIATE64,
        MÉMOIRE,
        FONCTION,
    };

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
    };

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
        }

#undef APPARIE_REGISTRE

        return "registre_invalide";
    }

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
            {
                return false;
            }
        }
        return false;
    }

    struct Mémoire {
        Registre registre{};
        int32_t décalage = 0;
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

        Opérande(){};

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

        if (dst.type == AssembleuseASM::TypeOpérande::MÉMOIRE) {
            assert(src.type != AssembleuseASM::TypeOpérande::MÉMOIRE);
        }

        m_sortie << TABULATION << "mov ";
        imprime_opérande(dst, taille);
        m_sortie << ", ";
        imprime_opérande(src, taille);

        m_sortie << NOUVELLE_LIGNE;
    }

    void lea(Opérande dst, Opérande src)
    {
        assert(src.type == TypeOpérande::MÉMOIRE);
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

    void mul(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "mul", taille_octet);
    }

    void imul(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "imul", taille_octet);
    }

    void div(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "div", taille_octet);
    }

    void idiv(Opérande dst, Opérande src, uint32_t taille_octet)
    {
        génère_code_opération_binaire(dst, src, "idiv", taille_octet);
    }

    void addss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "addss" << NOUVELLE_LIGNE;
    }

    void addsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "addsd" << NOUVELLE_LIGNE;
    }

    void mulss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "mulss" << NOUVELLE_LIGNE;
    }

    void mulsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "mulsd" << NOUVELLE_LIGNE;
    }

    void subss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "subss" << NOUVELLE_LIGNE;
    }

    void subsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "subsd" << NOUVELLE_LIGNE;
    }

    void divss(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "divss" << NOUVELLE_LIGNE;
    }

    void divsd(Opérande dst, Opérande src)
    {
        m_sortie << TABULATION << "divsd" << NOUVELLE_LIGNE;
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
        imprime_opérande(dst, taille_octet);
        m_sortie << ", ";
        imprime_opérande(src, taille_octet);
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
                auto const registre = opérande.mémoire.registre;
                auto const décalage = opérande.mémoire.décalage;
                m_sortie << "[" << chaine_pour_registre(registre, 8);
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
    bool registres[16] = {false};

  public:
    GestionnaireRegistres()
    {
        réinitialise();
    }

    AssembleuseASM::Registre donne_registre_inoccupé()
    {
        auto index_registre = 0;

        POUR (registres) {
            if (!it) {
                it = true;
                break;
            }

            index_registre += 1;
        }

        return static_cast<AssembleuseASM::Registre>(index_registre);
    }

    bool registre_est_occupé(AssembleuseASM::Registre registre) const
    {
        return registres[static_cast<int>(registre)];
    }

    void marque_registre_occupé(AssembleuseASM::Registre registre)
    {
        registres[static_cast<int>(registre)] = true;
    }

    void marque_registre_inoccupé(AssembleuseASM::Registre registre)
    {
        assert(registre != AssembleuseASM::Registre::RSP);
        registres[static_cast<int>(registre)] = false;
    }

    void réinitialise()
    {
        POUR (registres) {
            it = false;
        }

        marque_registre_occupé(AssembleuseASM::Registre::RSP);
    }
};

struct GénératriceCodeASM {
  private:
    kuri::tableau<AssembleuseASM::Opérande> table_valeurs{};
    kuri::table_hachage<Atome const *, kuri::chaine> table_globales{"Valeurs globales ASM"};
    AtomeFonction const *m_fonction_courante = nullptr;
    Enchaineuse enchaineuse_tmp{};
    Enchaineuse stockage_chn{};

    GestionnaireRegistres registres{};
    uint32_t taille_allouée = 0;

  public:
    AssembleuseASM::Opérande génère_code_pour_atome(Atome *atome,
                                                    AssembleuseASM &assembleuse,
                                                    const UtilisationAtome utilisation);

    void génère_code_pour_instruction(Instruction const *inst,
                                      AssembleuseASM &assembleuse,
                                      const UtilisationAtome utilisation);

    void génère_code_pour_opération_binaire(const InstructionOpBinaire *inst_bin,
                                            AssembleuseASM &assembleuse,
                                            const UtilisationAtome utilisation);

    void génère_code(kuri::tableau_statique<AtomeGlobale *> globales,
                     kuri::tableau_statique<AtomeFonction *> fonctions,
                     Enchaineuse &os);

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

    AssembleuseASM::Mémoire donne_adresse_stack();

    void génère_code_pour_fonction(const AtomeFonction *it,
                                   AssembleuseASM &assembleuse,
                                   Enchaineuse &os);

    void génère_code_pour_branche_condition(const InstructionBrancheCondition *inst_branche,
                                            AssembleuseASM &assembleuse);
};

AssembleuseASM::Opérande GénératriceCodeASM::génère_code_pour_atome(
    Atome *atome, AssembleuseASM &assembleuse, const UtilisationAtome utilisation)
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
            VERIFIE_NON_ATTEINT;
            return {};
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
            VERIFIE_NON_ATTEINT;
            return {};  // "";
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
            VERIFIE_NON_ATTEINT;
            return {};
        }
    }

    return {};
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

            /* Il faut faire de la place sur la pile. */
            auto type_alloué = inst->comme_alloc()->donne_type_alloué();

            if ((taille_allouée % type_alloué->alignement) != 0) {
                taille_allouée += (taille_allouée % type_alloué->alignement);
            }

            auto taille_requise = type_alloué->taille_octet;
            taille_allouée += taille_requise;
            auto adresse = donne_adresse_stack();

            table_valeurs[inst->numero] = AssembleuseASM::Mémoire{adresse};
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto appel = inst->comme_appel();

            /* Évite de générer deux fois le code pour les appels : une fois dans la boucle sur les
             * instructions, une fois pour l'opérande. Les fonctions retournant « rien » ne peuvent
             * être opérandes. */
            if (!appel->type->est_type_rien() &&
                ((utilisation & UtilisationAtome::POUR_OPÉRANDE) == UtilisationAtome::AUCUNE)) {
                return;
            }

            auto atome_appelée = appel->appelé;

            auto classes_args = détermine_classes_arguments(
                atome_appelée->type->comme_type_fonction());
            const AssembleuseASM::Registre registres_disponibles[6] = {
                AssembleuseASM::Registre::RDI,
                AssembleuseASM::Registre::RSI,
                AssembleuseASM::Registre::RDX,
                AssembleuseASM::Registre::RCX,
                AssembleuseASM::Registre::R8,
                AssembleuseASM::Registre::R9,
            };

            auto pointeur_registre = registres_disponibles;

            /* À FAIRE: chargement des paramètres dans les registres */
            POUR_INDEX (appel->args) {
                assert(it->est_instruction() && it->comme_instruction()->est_charge());
                auto chargement = it->comme_instruction()->comme_charge();
                auto source = chargement->chargée;
                auto adresse_source = génère_code_pour_atome(
                    source, assembleuse, UtilisationAtome::AUCUNE);
                assert(adresse_source.type == AssembleuseASM::TypeOpérande::MÉMOIRE);

                auto classe = classes_args.entrées[index_it];
                assert(classe == ClasseArgument::INTEGER);

                auto registre = *pointeur_registre++;
                assembleuse.mov(registre, adresse_source, it->type->taille_octet);

                // génère_code_pour_atome(it, assembleuse, false);
            }

            auto appelée = génère_code_pour_atome(
                appel->appelé, assembleuse, UtilisationAtome::AUCUNE);
            /* À FAIRE : appel pointeur. */
            assert(appelée.type == AssembleuseASM::TypeOpérande::FONCTION);

            /* Préserve note pile. */
            assembleuse.sub(
                AssembleuseASM::Registre::RSP, AssembleuseASM::Immédiate64{taille_allouée}, 8);

            assembleuse.call(appelée);

            /* Restaure note pile. */
            assembleuse.add(
                AssembleuseASM::Registre::RSP, AssembleuseASM::Immédiate64{taille_allouée}, 8);

            auto type_retour = appel->type;
            if (!type_retour->est_type_rien()) {
                /* À FAIRE : structures. */
                assert(type_retour->taille_octet <= 8);
                /* La valeur de retour est dans RAX. */
                table_valeurs[inst->numero] = AssembleuseASM::Registre::RAX;
                registres.réinitialise();
                registres.marque_registre_occupé(AssembleuseASM::Registre::RAX);
            }

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
            auto inst_charge = inst->comme_charge();

            /* À FAIRE: charge vers où? */
            /* À FAIRE: movss/movsd pour les réels. */
            /* À FAIRE: vérifie la taille de la structure. */
            auto src = génère_code_pour_atome(
                inst_charge->chargée, assembleuse, UtilisationAtome::AUCUNE);

            auto registre = registres.donne_registre_inoccupé();

            assembleuse.mov(registre, src, inst_charge->type->taille_octet);

            /* Déréférencement de pointeur. */
            AssembleuseASM::Opérande résultat = registre;
            if (inst_charge->type->est_type_pointeur()) {
                résultat = AssembleuseASM::Mémoire{registre, 0};
            }

            table_valeurs[inst->numero] = résultat;
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();

            auto dest = génère_code_pour_atome(
                inst_stocke->destination, assembleuse, UtilisationAtome::AUCUNE);
            auto src = génère_code_pour_atome(
                inst_stocke->source, assembleuse, UtilisationAtome::AUCUNE);

            auto type_stocké = inst_stocke->source->type;

            if (src.type == AssembleuseASM::TypeOpérande::MÉMOIRE) {
                /* Stockage d'une adresse. */
                auto registre = registres.donne_registre_inoccupé();
                assembleuse.lea(registre, src);
                assembleuse.mov(dest, registre, type_stocké->taille_octet);
                table_valeurs[inst->numero] = dest;
                registres.marque_registre_inoccupé(registre);
                return;
            }

            if (src.type == AssembleuseASM::TypeOpérande::REGISTRE) {
                registres.marque_registre_inoccupé(src.registre);
            }

            /* À FAIRE: met où? */
            /* À FAIRE: movss/movsd pour les réels. */
            assembleuse.mov(dest, src, type_stocké->taille_octet);
            table_valeurs[inst->numero] = dest;
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
                    VERIFIE_NON_ATTEINT;
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
            auto inst_retour = inst->comme_retour();

            if (inst_retour->valeur != nullptr) {
                auto valeur = génère_code_pour_atome(
                    inst_retour->valeur, assembleuse, UtilisationAtome::AUCUNE);

                if (valeur.type != AssembleuseASM::TypeOpérande::REGISTRE) {
                    // À FAIRE mov dans rax
                }
                else {
                    if (valeur.registre != AssembleuseASM::Registre::RAX) {
                        // À FAIRE mov dans rax
                    }
                }
            }

            restaure_registres_appel(assembleuse);
            assembleuse.ret();

            /* À FAIRE : marque plus de registres inoccupés ? */
            registres.marque_registre_inoccupé(AssembleuseASM::Registre::RAX);
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            /* À FAIRE: [ptr + décalage] */
            VERIFIE_NON_ATTEINT;
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto const accès = inst->comme_acces_membre();
            auto const accédé = accès->accédé;

            auto const valeur_accédé = génère_code_pour_atome(
                accédé, assembleuse, UtilisationAtome::AUCUNE);
            assert(valeur_accédé.type == AssembleuseASM::TypeOpérande::MÉMOIRE);

            auto const &membre = accès->donne_membre_accédé();

            auto registre = valeur_accédé.mémoire.registre;
            auto décalage = valeur_accédé.mémoire.décalage + int32_t(membre.decalage);
            auto résultat = AssembleuseASM::Mémoire{registre, décalage};

            table_valeurs[accès->numero] = résultat;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            /* À FAIRE: les types de transtypage */
            auto transtype = inst->comme_transtype();
            auto valeur = génère_code_pour_atome(
                transtype->valeur, assembleuse, UtilisationAtome::AUCUNE);
            table_valeurs[inst->numero] = valeur;

            if (!transtype->valeur->type->est_type_entier_constant()) {
                VERIFIE_NON_ATTEINT;
            }

            break;
        }
        case GenreInstruction::INATTEIGNABLE:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case GenreInstruction::SÉLECTION:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
    }
}

void GénératriceCodeASM::génère_code_pour_opération_binaire(InstructionOpBinaire const *inst_bin,
                                                            AssembleuseASM &assembleuse,
                                                            UtilisationAtome const utilisation)
{

    auto opérande_droite = génère_code_pour_atome(
        inst_bin->valeur_droite, assembleuse, UtilisationAtome::AUCUNE);
    auto opérande_gauche = génère_code_pour_atome(
        inst_bin->valeur_gauche, assembleuse, UtilisationAtome::AUCUNE);

#define GENERE_CODE_INST_ENTIER(nom_inst)                                                         \
    auto registre_résultat = opérande_gauche;                                                     \
    if (opérande_gauche.type != AssembleuseASM::TypeOpérande::REGISTRE) {                         \
        registre_résultat = registres.donne_registre_inoccupé();                                  \
        assembleuse.mov(                                                                          \
            registre_résultat, opérande_gauche, inst_bin->valeur_gauche->type->taille_octet);     \
    }                                                                                             \
    assembleuse.nom_inst(registre_résultat, opérande_droite, inst_bin->type->taille_octet);       \
    table_valeurs[inst_bin->numero] = registre_résultat;                                          \
    registres.réinitialise();                                                                     \
    registres.marque_registre_occupé(registre_résultat.registre)

#define GENERE_CODE_INST_DECALAGE_BIT(nom_inst)                                                   \
    std::optional<AssembleuseASM::Registre> registre_sauvegarde_rcx;                              \
    if (!assembleuse.est_immédiate(opérande_droite.type)) {                                       \
        if (registres.registre_est_occupé(AssembleuseASM::Registre::RCX)) {                       \
            registre_sauvegarde_rcx = registres.donne_registre_inoccupé();                        \
            assembleuse.mov(registre_sauvegarde_rcx.value(), AssembleuseASM::Registre::RCX, 8);   \
        }                                                                                         \
        assembleuse.mov(AssembleuseASM::Registre::RCX, opérande_droite, 1);                       \
        opérande_droite = AssembleuseASM::Registre::RCX;                                          \
    }                                                                                             \
    else {                                                                                        \
        opérande_droite = AssembleuseASM::donne_immédiate8(opérande_droite);                      \
    }                                                                                             \
    assembleuse.nom_inst(opérande_gauche, opérande_droite, inst_bin->type->taille_octet);         \
    table_valeurs[inst_bin->numero] = opérande_gauche;                                            \
    if (registre_sauvegarde_rcx.has_value()) {                                                    \
        assembleuse.mov(AssembleuseASM::Registre::RCX, registre_sauvegarde_rcx.value(), 8);       \
    }                                                                                             \
    registres.réinitialise();                                                                     \
    registres.marque_registre_occupé(opérande_gauche.registre)

    /* Les comparaisons sont générées en testant la valeur et utilise cmov pour mettre en place un
     * 0 ou un 1 dans le registre résultat. Ceci est le comportement désiré pour assigner depuis la
     * comparaison (a := b == c), mais pour les branches nous devrions plutôt émettre une
     * instruction idoine de branche au lieu de cmovxx. */
    auto génère_code_comparaison_entier = [&](auto &&gen_code) {
        auto const type_gauche = inst_bin->valeur_gauche->type;

        if (AssembleuseASM::est_immédiate(opérande_gauche.type)) {
            auto registre = registres.donne_registre_inoccupé();
            assembleuse.mov(registre, opérande_gauche, type_gauche->taille_octet);
            opérande_gauche = registre;
        }

        if ((utilisation & UtilisationAtome::POUR_BRANCHE_CONDITION) != UtilisationAtome::AUCUNE) {
            assembleuse.cmp(opérande_gauche, opérande_droite, type_gauche->taille_octet);
            return;
        }

        /* Xor avant la comparaison afin de ne pas modifier les drapeaux du CPU. */
        auto registre_résultat = registres.donne_registre_inoccupé();
        assembleuse.xor_(registre_résultat, registre_résultat, 8);

        /* Mis en place de la valeur si vrai avant la comparaison afin de ne pas modifier les
         * drapeaux du CPU. */
        auto registre = registres.donne_registre_inoccupé();
        assembleuse.mov(registre, AssembleuseASM::Immédiate64{1}, 8);

        assembleuse.cmp(opérande_gauche, opérande_droite, type_gauche->taille_octet);
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
            // À FAIRE : addsd pour des r64
            assembleuse.addss(OPERANDE2);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Soustraction:
        {
            GENERE_CODE_INST_ENTIER(sub);
            break;
        }
        case OpérateurBinaire::Genre::Soustraction_Reel:
        {
            // À FAIRE : subss pour des r64
            assembleuse.subss(OPERANDE2);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Multiplication:
        {
            if (inst_bin->type->est_type_entier_relatif()) {
                GENERE_CODE_INST_ENTIER(imul);
            }
            else {
                GENERE_CODE_INST_ENTIER(mul);
            }
            break;
        }
        case OpérateurBinaire::Genre::Multiplication_Reel:
        {
            // À FAIRE : mulsd pour des r64
            assembleuse.mulss(OPERANDE2);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Division_Naturel:
        {
            // À FAIRE
            GENERE_CODE_INST_ENTIER(div);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Division_Relatif:
        {
            // À FAIRE
            GENERE_CODE_INST_ENTIER(idiv);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Division_Reel:
        {
            // À FAIRE : divsd pour des r64
            assembleuse.divss(OPERANDE2);
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        {
            VERIFIE_NON_ATTEINT;
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
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        {
            VERIFIE_NON_ATTEINT;
            break;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            VERIFIE_NON_ATTEINT;
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
}

void GénératriceCodeASM::génère_code_pour_branche_condition(
    const InstructionBrancheCondition *inst_branche, AssembleuseASM &assembleuse)
{
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
            {
                génère_code_branche(&AssembleuseASM::jump_si_égal,
                                    &AssembleuseASM::jump_si_inégal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inegal:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inégal,
                                    &AssembleuseASM::jump_si_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inf:
            case OpérateurBinaire::Genre::Comp_Inf_Nat:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inférieur,
                                    &AssembleuseASM::jump_si_supérieur_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Inf_Egal:
            case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
            {
                génère_code_branche(&AssembleuseASM::jump_si_inférieur_égal,
                                    &AssembleuseASM::jump_si_supérieur);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Sup:
            case OpérateurBinaire::Genre::Comp_Sup_Nat:
            {
                génère_code_branche(&AssembleuseASM::jump_si_supérieur,
                                    &AssembleuseASM::jump_si_inférieur_égal);
                return;
            }
            case OpérateurBinaire::Genre::Comp_Sup_Egal:
            case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
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
            if (!it->est_appel()) {
                continue;
            }

            auto appel = it->comme_appel();
            if (appel->appelé->est_fonction()) {
                fonctions_à_visiter.empile(appel->appelé->comme_fonction());
            }
        }
    }

    return résultat;
}

void GénératriceCodeASM::génère_code(kuri::tableau_statique<AtomeGlobale *> globales,
                                     kuri::tableau_statique<AtomeFonction *> fonctions,
                                     Enchaineuse &os)
{
    auto fonctions_à_compiler = donne_fonctions_à_compiler(fonctions);

    os << "section .text\n";

    /* Prodéclaration des fonctions. */
    POUR (fonctions_à_compiler) {
        if (it->est_externe) {
            os << "extern " << it->nom << "\n";
        }
        else {
            os << "global " << it->nom << "\n";
        }
    }

    os << "\n";

    /* Définition des fonctions. */
    auto assembleuse = AssembleuseASM(os);

    POUR (fonctions_à_compiler) {
        if (it->est_externe) {
            continue;
        }

        génère_code_pour_fonction(it, assembleuse, os);
    }

    // Fonction de test.
    os << "global _start\n";
    os << "_start:\n";

    assembleuse.call(AssembleuseASM::Fonction{"principale"});
    /* Déplace le résultat de princpale dans RDI pour exit. */
    assembleuse.mov(AssembleuseASM::Registre::RDI, AssembleuseASM::Registre::RAX, 4);

    /* 60 = exit */
    assembleuse.mov(AssembleuseASM::Registre::RAX, AssembleuseASM::Immédiate32{60}, 4);
    assembleuse.syscall();
}

void GénératriceCodeASM::génère_code_pour_fonction(AtomeFonction const *fonction,
                                                   AssembleuseASM &assembleuse,
                                                   Enchaineuse &os)
{
    fonction->numérote_instructions();

    os << fonction->nom << ":\n";
    définis_fonction_courante(fonction);

    /* À FAIRE : comprend ce qu'il y a à RSP pour qu'un décalage de 8 soit toujours requis. */
    taille_allouée = 8;

    sauvegarde_registres_appel(assembleuse);

    auto classes_args = détermine_classes_arguments(fonction->type->comme_type_fonction());
    const AssembleuseASM::Registre registres_disponibles[6] = {
        AssembleuseASM::Registre::RDI,
        AssembleuseASM::Registre::RSI,
        AssembleuseASM::Registre::RDX,
        AssembleuseASM::Registre::RCX,
        AssembleuseASM::Registre::R8,
        AssembleuseASM::Registre::R9,
    };

    auto pointeur_registre = registres_disponibles;

    POUR_INDEX (fonction->params_entrée) {
        auto type_alloué = it->donne_type_alloué();
        taille_allouée += type_alloué->taille_octet;  // XXX : alignement
        auto adresse = donne_adresse_stack();
        table_valeurs[it->numero] = adresse;

        auto classe = classes_args.entrées[index_it];
        assert(classe == ClasseArgument::INTEGER);

        auto registre = *pointeur_registre++;
        assembleuse.mov(adresse, registre, type_alloué->taille_octet);
    }

    if (!fonction->param_sortie->type->est_type_rien()) {
        /* À FAIRE : multiple types de retour. */
        auto type_pointeur = fonction->param_sortie->type->comme_type_pointeur();
        taille_allouée += type_pointeur->type_pointé->taille_octet;
        table_valeurs[fonction->param_sortie->numero] = donne_adresse_stack();
    }

    POUR (fonction->instructions) {
        if (!instruction_est_racine(it) && !it->est_alloc()) {
            continue;
        }

        // imprime_inst_en_commentaire(os, inst);
        génère_code_pour_instruction(it, assembleuse, UtilisationAtome::AUCUNE);
    }

    m_fonction_courante = nullptr;
    os << "\n\n";
}

/* http://www.uclibc.org/docs/psABI-x86_64.pdf */
void GénératriceCodeASM::sauvegarde_registres_appel(AssembleuseASM &assembleuse)
{
    /* r12, r13, r14, r15, rbx, rsp, rbp */
    assembleuse.push(AssembleuseASM::Registre::R12);
    assembleuse.push(AssembleuseASM::Registre::R13);
    assembleuse.push(AssembleuseASM::Registre::R14);
    assembleuse.push(AssembleuseASM::Registre::R15);
    assembleuse.push(AssembleuseASM::Registre::RBX);
    assembleuse.push(AssembleuseASM::Registre::RSP);
    assembleuse.push(AssembleuseASM::Registre::RBP);
}

void GénératriceCodeASM::restaure_registres_appel(AssembleuseASM &assembleuse)
{
    assembleuse.pop(AssembleuseASM::Registre::RBP);
    assembleuse.pop(AssembleuseASM::Registre::RSP);
    assembleuse.pop(AssembleuseASM::Registre::RBX);
    assembleuse.pop(AssembleuseASM::Registre::R15);
    assembleuse.pop(AssembleuseASM::Registre::R14);
    assembleuse.pop(AssembleuseASM::Registre::R13);
    assembleuse.pop(AssembleuseASM::Registre::R12);
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

AssembleuseASM::Mémoire GénératriceCodeASM::donne_adresse_stack()
{
    return {AssembleuseASM::Registre::RSP, -int32_t(taille_allouée)};
}

void GénératriceCodeASM::imprime_inst_en_commentaire(Enchaineuse &os, Instruction const *inst)
{
    os << "  ; ";
    os << imprime_instruction(inst);
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
    génératrice.génère_code(repr_inter_programme.donne_globales(),
                            repr_inter_programme.donne_fonctions(),
                            enchaineuse);

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
    auto commande = "ld -o a.out /tmp/compilation_kuri_asm.o";
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
