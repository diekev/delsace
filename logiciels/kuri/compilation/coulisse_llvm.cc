/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "coulisse_llvm.hh"

#include <iostream>

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wclass-memaccess"
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/InitializePasses.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "structures/chemin_systeme.hh"
#include "structures/table_hachage.hh"

#include "compilatrice.hh"
#include "environnement.hh"
#include "espace_de_travail.hh"
#include "log.hh"
#include "programme.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/instructions.hh"

inline bool adresse_est_nulle(void *adresse)
{
    /* 0xbebebebebebebebe peut être utilisé par les débogueurs. */
    return adresse == nullptr || adresse == reinterpret_cast<void *>(0xbebebebebebebebe);
}

/* ************************************************************************** */

/* À FAIRE(typage) : l'index des membres utilisé pour les accès à des membres est l'index absolu
 * dans le tableau de membres du type. Hors ceci contient également les membres constants. Avoir de
 * tels index pour les accès est plus ou moins correcte pour la coulisse C, mais pour LLVM, dont
 * les types n'ont pas de membres constants, les index sont faux et peuvent pointer endehors du
 * type donc nous devons les corriger afin d'accéder au membre correspondant du type LLVM.
 * Nous pourrions stocker les membres constants à la fin du tableau de membre afin de ne pas avoir
 * à se soucier de ce genre de choses. */
static unsigned index_reel_pour_membre(TypeCompose const &type, unsigned index)
{
    auto index_reel = 0u;

    POUR (type.membres) {
        if (index == 0) {
            break;
        }

        index -= 1;

        if (it.ne_doit_pas_être_dans_code_machine()) {
            continue;
        }

        index_reel += 1;
    }

    return index_reel;
}

/* ************************************************************************** */

static const LogDebug &operator<<(const LogDebug &log_debug, const llvm::Value &llvm_value)
{
    llvm::errs() << llvm_value;
    return log_debug;
}

static const LogDebug &operator<<(const LogDebug &log_debug, const llvm::Constant &llvm_value)
{
    llvm::errs() << llvm_value;
    return log_debug;
}

static const LogDebug &operator<<(const LogDebug &log_debug,
                                  const llvm::GlobalVariable &llvm_value)
{
    llvm::errs() << llvm_value;
    return log_debug;
}

/* ************************************************************************** */

auto vers_std_string(kuri::chaine const &chn)
{
    return std::string(chn.pointeur(), static_cast<size_t>(chn.taille()));
}

auto vers_std_string(dls::vue_chaine_compacte const &chn)
{
    return std::string(chn.pointeur(), static_cast<size_t>(chn.taille()));
}

static auto inst_llvm_depuis_operateur(OpérateurBinaire::Genre genre)
{
    using Genre = OpérateurBinaire::Genre;

    switch (genre) {
        case Genre::Addition:
            return llvm::Instruction::Add;
        case Genre::Addition_Reel:
            return llvm::Instruction::FAdd;
        case Genre::Soustraction:
            return llvm::Instruction::Sub;
        case Genre::Soustraction_Reel:
            return llvm::Instruction::FSub;
        case Genre::Multiplication:
            return llvm::Instruction::Mul;
        case Genre::Multiplication_Reel:
            return llvm::Instruction::FMul;
        case Genre::Division_Naturel:
            return llvm::Instruction::UDiv;
        case Genre::Division_Reel:
            return llvm::Instruction::FDiv;
        case Genre::Division_Relatif:
            return llvm::Instruction::SDiv;
        case Genre::Reste_Naturel:
            return llvm::Instruction::URem;
        case Genre::Reste_Relatif:
            return llvm::Instruction::SRem;
        case Genre::Et_Binaire:
            return llvm::Instruction::And;
        case Genre::Ou_Binaire:
            return llvm::Instruction::Or;
        case Genre::Ou_Exclusif:
            return llvm::Instruction::Xor;
        case Genre::Dec_Gauche:
            return llvm::Instruction::Shl;
        case Genre::Dec_Droite_Arithm:
            return llvm::Instruction::AShr;
        case Genre::Dec_Droite_Logique:
            return llvm::Instruction::LShr;
        case Genre::Invalide:
        case Genre::Comp_Egal:
        case Genre::Comp_Inegal:
        case Genre::Comp_Inf:
        case Genre::Comp_Inf_Egal:
        case Genre::Comp_Sup:
        case Genre::Comp_Sup_Egal:
        case Genre::Comp_Inf_Nat:
        case Genre::Comp_Inf_Egal_Nat:
        case Genre::Comp_Sup_Nat:
        case Genre::Comp_Sup_Egal_Nat:
        case Genre::Comp_Egal_Reel:
        case Genre::Comp_Inegal_Reel:
        case Genre::Comp_Inf_Reel:
        case Genre::Comp_Inf_Egal_Reel:
        case Genre::Comp_Sup_Reel:
        case Genre::Comp_Sup_Egal_Reel:
        case Genre::Indexage:
            break;
    }

    return static_cast<llvm::Instruction::BinaryOps>(0);
}

static auto cmp_llvm_depuis_operateur(OpérateurBinaire::Genre genre)
{
    using Genre = OpérateurBinaire::Genre;

    switch (genre) {
        case Genre::Comp_Egal:
            return llvm::CmpInst::Predicate::ICMP_EQ;
        case Genre::Comp_Inegal:
            return llvm::CmpInst::Predicate::ICMP_NE;
        case Genre::Comp_Inf:
            return llvm::CmpInst::Predicate::ICMP_SLT;
        case Genre::Comp_Inf_Egal:
            return llvm::CmpInst::Predicate::ICMP_SLE;
        case Genre::Comp_Sup:
            return llvm::CmpInst::Predicate::ICMP_SGT;
        case Genre::Comp_Sup_Egal:
            return llvm::CmpInst::Predicate::ICMP_SGE;
        case Genre::Comp_Inf_Nat:
            return llvm::CmpInst::Predicate::ICMP_ULT;
        case Genre::Comp_Inf_Egal_Nat:
            return llvm::CmpInst::Predicate::ICMP_ULE;
        case Genre::Comp_Sup_Nat:
            return llvm::CmpInst::Predicate::ICMP_UGT;
        case Genre::Comp_Sup_Egal_Nat:
            return llvm::CmpInst::Predicate::ICMP_UGE;
        case Genre::Comp_Egal_Reel:
            return llvm::CmpInst::Predicate::FCMP_OEQ;
        case Genre::Comp_Inegal_Reel:
            return llvm::CmpInst::Predicate::FCMP_ONE;
        case Genre::Comp_Inf_Reel:
            return llvm::CmpInst::Predicate::FCMP_OLT;
        case Genre::Comp_Inf_Egal_Reel:
            return llvm::CmpInst::Predicate::FCMP_OLE;
        case Genre::Comp_Sup_Reel:
            return llvm::CmpInst::Predicate::FCMP_OGT;
        case Genre::Comp_Sup_Egal_Reel:
            return llvm::CmpInst::Predicate::FCMP_OGE;
        case Genre::Invalide:
        case Genre::Addition:
        case Genre::Addition_Reel:
        case Genre::Soustraction:
        case Genre::Soustraction_Reel:
        case Genre::Multiplication:
        case Genre::Multiplication_Reel:
        case Genre::Division_Naturel:
        case Genre::Division_Reel:
        case Genre::Division_Relatif:
        case Genre::Reste_Naturel:
        case Genre::Reste_Relatif:
        case Genre::Et_Binaire:
        case Genre::Ou_Binaire:
        case Genre::Ou_Exclusif:
        case Genre::Dec_Gauche:
        case Genre::Dec_Droite_Arithm:
        case Genre::Dec_Droite_Logique:
        case Genre::Indexage:
            break;
    }

    return static_cast<llvm::CmpInst::Predicate>(0);
}

static auto convertis_type_transtypage(TypeTranstypage const transtypage,
                                       Type const *type_de,
                                       Type const *type_vers)
{
    using CastOps = llvm::Instruction::CastOps;
    switch (transtypage) {
        case TypeTranstypage::AUGMENTE_NATUREL:
            return CastOps::ZExt;
        case TypeTranstypage::AUGMENTE_RELATIF:
            return CastOps::SExt;
        case TypeTranstypage::AUGMENTE_REEL:
            return CastOps::FPExt;
        case TypeTranstypage::DIMINUE_NATUREL:
        case TypeTranstypage::DIMINUE_RELATIF:
            return CastOps::Trunc;
        case TypeTranstypage::DIMINUE_REEL:
            return CastOps::FPTrunc;
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
            return CastOps::PtrToInt;
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
            return CastOps::IntToPtr;
        case TypeTranstypage::REEL_VERS_ENTIER:
            if (type_vers->est_type_entier_naturel()) {
                return CastOps::FPToUI;
            }
            return CastOps::FPToSI;
        case TypeTranstypage::ENTIER_VERS_REEL:
            if (type_de->est_type_entier_naturel()) {
                return CastOps::UIToFP;
            }
            return CastOps::SIToFP;
        case TypeTranstypage::DEFAUT:
        case TypeTranstypage::BITS:
            return CastOps::BitCast;
    }

    return static_cast<CastOps>(0);
}

/* ************************************************************************** */

struct GeneratriceCodeLLVM {
    kuri::table_hachage<Atome const *, llvm::Value *> table_valeurs{"Table valeurs locales LLVM"};
    kuri::table_hachage<InstructionLabel const *, llvm::BasicBlock *> table_blocs{
        "Table labels LLVM"};
    kuri::table_hachage<Atome const *, llvm::GlobalVariable *> table_globales{
        "Table valeurs globales LLVM"};
    kuri::table_hachage<Type const *, llvm::Type *> table_types{"Table types LLVM"};
    kuri::table_hachage<kuri::chaine, llvm::Constant *> valeurs_chaines_globales{
        "Table chaines LLVM"};
    EspaceDeTravail &m_espace;

    llvm::Function *m_fonction_courante = nullptr;
    llvm::LLVMContext m_contexte_llvm{};
    llvm::Module *m_module = nullptr;
    llvm::IRBuilder<> m_builder;
    llvm::legacy::FunctionPassManager *manager_fonctions = nullptr;

    GeneratriceCodeLLVM(EspaceDeTravail &espace);

    GeneratriceCodeLLVM(GeneratriceCodeLLVM const &) = delete;
    GeneratriceCodeLLVM &operator=(const GeneratriceCodeLLVM &) = delete;

    llvm::Type *converti_type_llvm(Type const *type);

    llvm::FunctionType *converti_type_fonction(TypeFonction const *type, bool est_externe);

    llvm::Value *genere_code_pour_atome(Atome *atome, bool pour_globale);

    void genere_code_pour_instruction(Instruction const *inst);

    void genere_code(const ProgrammeRepreInter &repr_inter);

    llvm::Constant *valeur_pour_chaine(const kuri::chaine &chaine, int64_t taille_chaine);
};

GeneratriceCodeLLVM::GeneratriceCodeLLVM(EspaceDeTravail &espace)
    : m_espace(espace), m_builder(m_contexte_llvm)
{
}

llvm::Type *GeneratriceCodeLLVM::converti_type_llvm(Type const *type)
{
    auto type_llvm = table_types.valeur_ou(type, nullptr);

    if (type_llvm != nullptr) {
        /* Note: normalement les types des pointeurs vers les fonctions doivent
         * être des pointeurs, mais les types fonctions sont partagés entre les
         * fonctions et les variables, donc un type venant d'une fonction n'aura
         * pas le pointeur. Ajoutons-le. */
        if (type->est_type_fonction() && !type_llvm->isPointerTy()) {
            return llvm::PointerType::get(type_llvm, 0);
        }

        return type_llvm;
    }

    if (type == nullptr) {
        return nullptr;
    }

    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        {
            type_llvm = nullptr;
            table_types.insère(type, type_llvm);
            break;
        }
        case GenreType::TUPLE:
        {
            auto tuple = type->comme_type_tuple();

            std::vector<llvm::Type *> types_membres;
            types_membres.reserve(static_cast<size_t>(tuple->membres.taille()));
            POUR (tuple->membres) {
                types_membres.push_back(converti_type_llvm(it.type));
            }

            type_llvm = llvm::StructType::create(m_contexte_llvm, types_membres, "tuple", false);
            break;
        }
        case GenreType::FONCTION:
        {
            auto type_fonc = type->comme_type_fonction();

            std::vector<llvm::Type *> parametres;
            POUR (type_fonc->types_entrees) {
                auto type_llvm_it = converti_type_llvm(it);
                parametres.push_back(type_llvm_it);
            }

            auto type_retour = converti_type_llvm(type_fonc->type_sortie);

            type_llvm = llvm::FunctionType::get(type_retour, parametres, false);

            type_llvm = llvm::PointerType::get(type_llvm, 0);
            break;
        }
        case GenreType::EINI:
        {
            /* type = structure { *z8, *InfoType } */
            auto type_info_type = m_espace.compilatrice().typeuse.type_info_type_;

            std::vector<llvm::Type *> types_membres(2ul);
            types_membres[0] = llvm::Type::getInt8PtrTy(m_contexte_llvm);
            types_membres[1] = converti_type_llvm(type_info_type)->getPointerTo();

            type_llvm = llvm::StructType::create(
                m_contexte_llvm, types_membres, "struct.eini", false);

            break;
        }
        case GenreType::CHAINE:
        {
            /* type = structure { *z8, z64 } */
            std::vector<llvm::Type *> types_membres(2ul);
            types_membres[0] = llvm::Type::getInt8PtrTy(m_contexte_llvm);
            types_membres[1] = llvm::Type::getInt64Ty(m_contexte_llvm);

            type_llvm = llvm::StructType::create(
                m_contexte_llvm, types_membres, "struct.chaine", false);

            break;
        }
        case GenreType::RIEN:
        {
            type_llvm = llvm::Type::getVoidTy(m_contexte_llvm);
            break;
        }
        case GenreType::BOOL:
        {
            type_llvm = llvm::Type::getInt1Ty(m_contexte_llvm);
            break;
        }
        case GenreType::OCTET:
        {
            type_llvm = llvm::Type::getInt8Ty(m_contexte_llvm);
            break;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            type_llvm = llvm::Type::getInt32Ty(m_contexte_llvm);
            break;
        }
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                type_llvm = llvm::Type::getInt8Ty(m_contexte_llvm);
            }
            else if (type->taille_octet == 2) {
                type_llvm = llvm::Type::getInt16Ty(m_contexte_llvm);
            }
            else if (type->taille_octet == 4) {
                type_llvm = llvm::Type::getInt32Ty(m_contexte_llvm);
            }
            else if (type->taille_octet == 8) {
                type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
            }

            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
            break;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                type_llvm = llvm::Type::getInt16Ty(m_contexte_llvm);
            }
            else if (type->taille_octet == 4) {
                type_llvm = llvm::Type::getFloatTy(m_contexte_llvm);
            }
            else if (type->taille_octet == 8) {
                type_llvm = llvm::Type::getDoubleTy(m_contexte_llvm);
            }

            break;
        }
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        {
            auto type_deref = type_dereference_pour(type);

            // Les pointeurs vers rien (void) ne sont pas valides avec LLVM
            // @Incomplet : LLVM n'a pas de pointeur nul
            if (!type_deref || type_deref->est_type_rien()) {
                type_deref = TypeBase::Z8;
            }

            auto type_deref_llvm = converti_type_llvm(type_deref);
            type_llvm = llvm::PointerType::get(type_deref_llvm, 0);
            break;
        }
        case GenreType::UNION:
        {
            auto type_struct = type->comme_type_union();
            auto decl_struct = type_struct->decl;
            auto nom_nonsur = enchaine("union_nonsure.", type_struct->nom->nom);
            auto nom = enchaine("union.", type_struct->nom->nom);

            // création d'une structure ne contenant que le membre le plus grand
            auto type_le_plus_grand = type_struct->type_le_plus_grand;

            auto type_max_llvm = converti_type_llvm(type_le_plus_grand);
            auto type_union = llvm::StructType::create(
                m_contexte_llvm, {type_max_llvm}, vers_std_string(nom_nonsur));

            if (!decl_struct->est_nonsure) {
                // création d'une structure contenant l'union et une valeur discriminante
                type_union = llvm::StructType::create(
                    m_contexte_llvm,
                    {type_union, llvm::Type::getInt32Ty(m_contexte_llvm)},
                    vers_std_string(nom));
            }

            type_llvm = type_union;
            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();
            auto nom = enchaine("struct.", type_struct->nom->nom);

            /* Pour les structures récursives, il faut créer un type
             * opaque, dont le corps sera renseigné à la fin */
            auto type_opaque = llvm::StructType::create(m_contexte_llvm, vers_std_string(nom));
            table_types.insère(type, type_opaque);

            std::vector<llvm::Type *> types_membres;
            types_membres.reserve(static_cast<size_t>(type_struct->membres.taille()));

            POUR (type_struct->membres) {
                if (it.ne_doit_pas_être_dans_code_machine()) {
                    continue;
                }
                types_membres.push_back(converti_type_llvm(it.type));
            }

            auto est_compacte = false;
            if (type_struct->decl && type_struct->decl->est_compacte) {
                est_compacte = true;
            }

            type_opaque->setBody(types_membres, est_compacte);

            /* retourne directement puisque le type a déjà été ajouté à la table de types */
            return type_opaque;
        }
        case GenreType::VARIADIQUE:
        {
            auto type_var = type->comme_type_variadique();

            // Utilise le type de tableau dynamique afin que le code IR LLVM
            // soit correcte (pointe vers le même type)
            if (type_var->type_pointe != nullptr) {
                auto type_tabl = m_espace.compilatrice().typeuse.type_tableau_dynamique(
                    type_var->type_pointe);
                type_llvm = converti_type_llvm(type_tabl);
            }

            break;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            auto type_deref_llvm = converti_type_llvm(type_dereference_pour(type));

            /* type = structure { *type, n64, n64 } */
            std::vector<llvm::Type *> types_membres(3ul);
            types_membres[0] = llvm::PointerType::get(type_deref_llvm, 0);
            types_membres[1] = llvm::Type::getInt64Ty(m_contexte_llvm);
            types_membres[2] = llvm::Type::getInt64Ty(m_contexte_llvm);

            type_llvm = llvm::StructType::create(
                m_contexte_llvm, types_membres, "struct.tableau", false);
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_deref_llvm = converti_type_llvm(type_dereference_pour(type));
            auto const taille = type->comme_type_tableau_fixe()->taille;

            type_llvm = llvm::ArrayType::get(type_deref_llvm, static_cast<uint64_t>(taille));
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            type_llvm = converti_type_llvm(type_enum->type_sous_jacent);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            type_llvm = converti_type_llvm(type_opaque->type_opacifie);
            break;
        }
    }

    table_types.insère(type, type_llvm);
    return type_llvm;
}

llvm::FunctionType *GeneratriceCodeLLVM::converti_type_fonction(TypeFonction const *type,
                                                                bool est_externe)
{
    std::vector<llvm::Type *> parametres;
    parametres.reserve(static_cast<size_t>(type->types_entrees.taille()));

    auto est_variadique = false;

    POUR (type->types_entrees) {
        if (it->est_type_variadique()) {
            est_variadique = true;

            /* les arguments variadiques sont transformés en un tableau */
            if (!est_externe) {
                auto type_var = it->comme_type_variadique();
                auto type_tabl = m_espace.compilatrice().typeuse.type_tableau_dynamique(
                    type_var->type_pointe);
                parametres.push_back(converti_type_llvm(type_tabl));
            }

            break;
        }

        parametres.push_back(converti_type_llvm(it));
    }

    auto type_sortie_llvm = converti_type_llvm(type->type_sortie);
    assert(type_sortie_llvm);

    return llvm::FunctionType::get(type_sortie_llvm, parametres, est_variadique && est_externe);
}

llvm::Value *GeneratriceCodeLLVM::genere_code_pour_atome(Atome *atome, bool pour_globale)
{
    // auto incrémentation_temp = LogDebug::IncrémenteuseTemporaire();
    // dbg() << __func__ << ", atome: " << static_cast<int>(atome->genre_atome);

    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            // dbg() << "FONCTION";
            auto atome_fonc = static_cast<AtomeFonction const *>(atome);
            return m_module->getFunction(vers_std_string(atome_fonc->nom));
        }
        case Atome::Genre::CONSTANTE:
        {
            auto atome_const = static_cast<AtomeConstante const *>(atome);
            auto type_llvm = converti_type_llvm(atome_const->type);

            switch (atome_const->genre) {
                case AtomeConstante::Genre::GLOBALE:
                {
                    auto valeur_globale = static_cast<AtomeGlobale const *>(atome);

                    if (valeur_globale->ident) {
                        auto valeur_ = table_globales.valeur_ou(valeur_globale, nullptr);
                        // dbg() << "CONSTANTE GLOBALE: " << *valeur_;
                        return valeur_;
                    }

                    auto valeur_ = table_valeurs.valeur_ou(valeur_globale, nullptr);
                    // dbg() << "CONSTANTE GLOBALE: " << *valeur_;
                    return valeur_;
                }
                case AtomeConstante::Genre::FONCTION:
                {
                    auto fonction = static_cast<AtomeFonction const *>(atome_const);
                    return m_module->getFunction(vers_std_string(fonction->nom));
                }
                case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
                {
                    auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
                    auto valeur = genere_code_pour_atome(transtype_const->valeur, pour_globale);
                    auto valeur_ = llvm::ConstantExpr::getBitCast(
                        llvm::cast<llvm::Constant>(valeur), type_llvm);
                    // dbg() << "TRANSTYPE_CONSTANT: " << *valeur_;
                    return valeur_;
                }
                case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
                {
                    auto acces = static_cast<AccedeIndexConstant const *>(atome_const);
                    auto index = genere_code_pour_atome(acces->index, pour_globale);
                    assert(index);
                    auto accede = genere_code_pour_atome(acces->accede, pour_globale);
                    assert(accede);

                    if (acces->accede->est_globale()) {
                        auto globale = static_cast<AtomeGlobale *>(acces->accede);

                        auto init = genere_code_pour_atome(globale->initialisateur, pour_globale);
                        assert(init);
                        auto globale_llvm = table_globales.valeur_ou(globale, nullptr);

                        accede = globale_llvm;

                        globale_llvm->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
                        globale_llvm->setInitializer(llvm::cast<llvm::Constant>(init));
                    }

                    auto index_array = std::vector<llvm::Value *>();
                    auto type_accede = acces->accede->type;
                    if (type_accede->comme_type_pointeur()->type_pointe->est_type_pointeur()) {
                        index_array.resize(1);
                        index_array[0] = index;
                    }
                    else {
                        index_array.resize(2);
                        index_array[0] = llvm::ConstantInt::get(
                            llvm::Type::getInt32Ty(m_contexte_llvm), 0);
                        index_array[1] = index;
                    }

                    assert(type_llvm);

                    // dbg() << "ACCES_INDEX_CONSTANT: index=" << *index << ", accede=" << *accede;

                    return llvm::ConstantExpr::getInBoundsGetElementPtr(
                        nullptr, llvm::cast<llvm::Constant>(accede), index_array);
                }
                case AtomeConstante::Genre::VALEUR:
                {
                    // dbg() << "VALEUR";
                    auto valeur_const = static_cast<AtomeValeurConstante const *>(atome);

                    switch (valeur_const->valeur.genre) {
                        case AtomeValeurConstante::Valeur::Genre::NULLE:
                        {
                            return llvm::ConstantPointerNull::get(static_cast<llvm::PointerType *>(
                                converti_type_llvm(valeur_const->type)));
                        }
                        case AtomeValeurConstante::Valeur::Genre::REELLE:
                        {
                            if (atome_const->type->taille_octet == 2) {
                                return llvm::ConstantInt::get(
                                    llvm::Type::getInt16Ty(m_contexte_llvm),
                                    static_cast<unsigned>(valeur_const->valeur.valeur_reelle));
                            }

                            return llvm::ConstantFP::get(type_llvm,
                                                         valeur_const->valeur.valeur_reelle);
                        }
                        case AtomeValeurConstante::Valeur::Genre::TYPE:
                        {
                            return llvm::ConstantInt::get(
                                type_llvm, valeur_const->valeur.type->index_dans_table_types);
                        }
                        case AtomeValeurConstante::Valeur::Genre::TAILLE_DE:
                        {
                            return llvm::ConstantInt::get(type_llvm,
                                                          valeur_const->valeur.type->taille_octet);
                        }
                        case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                        {
                            // À FAIRE : la RI peut modifier le type mais pas le genre de l'atome.
                            if (atome->type->est_type_reel()) {
                                auto valeur_reelle = static_cast<double>(
                                    valeur_const->valeur.valeur_entiere);
                                return llvm::ConstantFP::get(type_llvm, valeur_reelle);
                            }

                            return llvm::ConstantInt::get(type_llvm,
                                                          valeur_const->valeur.valeur_entiere);
                        }
                        case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                        {
                            return llvm::ConstantInt::get(type_llvm,
                                                          valeur_const->valeur.valeur_booleenne);
                        }
                        case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                        {
                            return llvm::ConstantInt::get(type_llvm,
                                                          valeur_const->valeur.valeur_entiere);
                        }
                        case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                        {
                            return nullptr;
                        }
                        case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                        {
                            auto type = static_cast<TypeCompose const *>(atome->type);
                            auto tableau_valeur = valeur_const->valeur.valeur_structure.pointeur;

                            auto tableau_membre = std::vector<llvm::Constant *>();

                            auto index_membre = 0;
                            for (auto i = 0; i < type->membres.taille(); ++i) {
                                if (type->membres[i].ne_doit_pas_être_dans_code_machine()) {
                                    continue;
                                }

                                auto valeur = static_cast<llvm::Constant *>(nullptr);

                                // les tableaux fixes ont une initialisation nulle
                                if (tableau_valeur[index_membre] == nullptr) {
                                    auto type_llvm_valeur = converti_type_llvm(
                                        type->membres[i].type);
                                    valeur = llvm::ConstantAggregateZero::get(type_llvm_valeur);
                                }
                                else {
                                    // dbg() << "Génère code pour le membre " <<
                                    // type->membres[i].nom->nom;
                                    valeur = llvm::cast<llvm::Constant>(genere_code_pour_atome(
                                        tableau_valeur[index_membre], pour_globale));
                                }

                                tableau_membre.push_back(valeur);
                                index_membre += 1;
                            }

                            return llvm::ConstantStruct::get(
                                llvm::cast<llvm::StructType>(converti_type_llvm(type)),
                                tableau_membre);
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                        {
                            auto pointeur_tableau = valeur_const->valeur.valeur_tableau.pointeur;
                            auto taille_tableau = valeur_const->valeur.valeur_tableau.taille;

                            std::vector<llvm::Constant *> valeurs;
                            valeurs.reserve(static_cast<size_t>(taille_tableau));

                            for (auto i = 0; i < taille_tableau; ++i) {
                                auto valeur = genere_code_pour_atome(pointeur_tableau[i],
                                                                     pour_globale);
                                valeurs.push_back(llvm::cast<llvm::Constant>(valeur));
                            }

                            auto resultat = llvm::ConstantArray::get(
                                llvm::cast<llvm::ArrayType>(type_llvm), valeurs);
                            // dbg() << "TABLEAU_FIXE : " << *resultat;
                            return resultat;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                        {
                            auto pointeur_donnnees = valeur_const->valeur.valeur_tdc.pointeur;
                            auto taille_donnees = valeur_const->valeur.valeur_tdc.taille;

                            std::vector<unsigned char> donnees;
                            donnees.resize(static_cast<size_t>(taille_donnees));

                            for (auto i = 0; i < taille_donnees; ++i) {
                                donnees[static_cast<size_t>(i)] = static_cast<unsigned char>(
                                    pointeur_donnnees[i]);
                            }

                            auto valeur_ = llvm::ConstantDataArray::get(m_contexte_llvm, donnees);
                            // dbg() << "TABLEAU_DONNEES_CONSTANTES: " << *valeur_;
                            return valeur_;
                        }
                    }
                }
            }

            return nullptr;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = atome->comme_instruction();
            return table_valeurs.valeur_ou(inst, nullptr);
        }
        case Atome::Genre::GLOBALE:
        {
            auto valeur = table_globales.valeur_ou(atome, nullptr);
            // dbg() << "GLOBALE: " << *valeur;
            return valeur;
        }
    }

    return nullptr;
}

void GeneratriceCodeLLVM::genere_code_pour_instruction(const Instruction *inst)
{
    // auto incrémentation_temp = LogDebug::IncrémenteuseTemporaire();
    // dbg() << __func__;

    switch (inst->genre) {
        case GenreInstruction::INVALIDE:
        {
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            auto type_pointeur = inst->type->comme_type_pointeur();
            auto type_llvm = converti_type_llvm(type_pointeur->type_pointe);
            auto alloca = m_builder.CreateAlloca(type_llvm, 0u);
            alloca->setAlignment(llvm::Align(type_pointeur->type_pointe->alignement));
            table_valeurs.insère(inst, alloca);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            auto arguments = std::vector<llvm::Value *>();

            POUR (inst_appel->args) {
                arguments.push_back(genere_code_pour_atome(it, false));
            }

            auto valeur_fonction = genere_code_pour_atome(inst_appel->appele, false);
            assert_rappel(!adresse_est_nulle(valeur_fonction), [&]() {
                std::cerr << erreur::imprime_site(m_espace, inst_appel->site);
                imprime_atome(inst_appel->appele, std::cerr);
            });
            table_valeurs.insère(
                inst,
                m_builder.CreateCall(llvm::FunctionCallee(static_cast<llvm::FunctionType *>(
                                                              valeur_fonction->getType()),
                                                          valeur_fonction),
                                     arguments));

            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            m_builder.CreateBr(table_blocs.valeur_ou(inst_branche->label, nullptr));
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            auto condition = genere_code_pour_atome(inst_branche->condition, false);
            auto bloc_si_vrai = table_blocs.valeur_ou(inst_branche->label_si_vrai, nullptr);
            auto bloc_si_faux = table_blocs.valeur_ou(inst_branche->label_si_faux, nullptr);
            m_builder.CreateCondBr(condition, bloc_si_vrai, bloc_si_faux);
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargee;
            auto valeur = genere_code_pour_atome(charge, false);
            assert(valeur != nullptr);

            auto load = m_builder.CreateLoad(valeur, "");
            load->setAlignment(llvm::Align(charge->type->alignement));
            table_valeurs.insère(inst_charge, load);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto valeur = genere_code_pour_atome(inst_stocke->valeur, false);
            auto ou = inst_stocke->ou;
            auto valeur_ou = genere_code_pour_atome(ou, false);

            assert_rappel(!adresse_est_nulle(valeur_ou), [&]() {
                std::cerr << erreur::imprime_site(m_espace, inst_stocke->site);
                imprime_atome(inst_stocke, std::cerr);
                std::cerr << '\n';
                imprime_information_atome(inst_stocke->ou, std::cerr);
                std::cerr << '\n';
            });
            assert_rappel(!adresse_est_nulle(valeur), [&]() {
                std::cerr << erreur::imprime_site(m_espace, inst_stocke->site);
                imprime_atome(inst_stocke, std::cerr);
                std::cerr << '\n';
                imprime_information_atome(inst_stocke->valeur, std::cerr);
                std::cerr << '\n';
            });

            m_builder.CreateStore(valeur, valeur_ou);

            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            auto bloc = llvm::BasicBlock::Create(m_contexte_llvm, "", m_fonction_courante);
            table_blocs.insère(inst_label, bloc);
            // m_builder.SetInsertPoint(bloc);
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();
            auto valeur = genere_code_pour_atome(inst_un->valeur, false);
            auto type = inst_un->valeur->type;

            switch (inst_un->op) {
                case OpérateurUnaire::Genre::Positif:
                {
                    valeur = m_builder.CreateLoad(valeur, "");
                    break;
                }
                case OpérateurUnaire::Genre::Invalide:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Complement:
                {
                    auto type_llvm = converti_type_llvm(inst_un->valeur->type);
                    if (est_type_entier(type)) {
                        auto zero = llvm::ConstantInt::get(
                            type_llvm, 0, type->est_type_entier_relatif());
                        valeur = m_builder.CreateSub(zero, valeur);
                    }
                    else {
                        auto zero = llvm::ConstantFP::get(type_llvm, 0.0);
                        valeur = m_builder.CreateFSub(zero, valeur);
                    }
                    break;
                }
                case OpérateurUnaire::Genre::Non_Binaire:
                {
                    auto type_llvm = converti_type_llvm(inst_un->valeur->type);
                    valeur = m_builder.CreateXor(
                        valeur,
                        llvm::ConstantInt::get(type_llvm, 0, type->est_type_entier_relatif()));
                    break;
                }
                case OpérateurUnaire::Genre::Non_Logique:
                {
                    auto valeur2 = m_builder.getInt1(0);
                    valeur = m_builder.CreateICmpEQ(valeur, valeur2);
                    valeur = m_builder.CreateXor(valeur, m_builder.getInt1(1));
                    break;
                }
                case OpérateurUnaire::Genre::Prise_Adresse:
                {
                    break;
                }
            }

            table_valeurs.insère(inst, valeur);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            auto valeur_gauche = genere_code_pour_atome(inst_bin->valeur_gauche, false);
            auto valeur_droite = genere_code_pour_atome(inst_bin->valeur_droite, false);

            if (inst_bin->op >= OpérateurBinaire::Genre::Comp_Egal &&
                inst_bin->op <= OpérateurBinaire::Genre::Comp_Sup_Egal_Nat) {
                auto cmp = cmp_llvm_depuis_operateur(inst_bin->op);

                assert_rappel(inst_bin->valeur_droite->type == inst_bin->valeur_gauche->type,
                              [&]() {
                                  std::cerr << erreur::imprime_site(m_espace, inst_bin->site);
                                  std::cerr << "Type à gauche "
                                            << chaine_type(inst_bin->valeur_gauche->type) << '\n';
                                  std::cerr << "Type à droite "
                                            << chaine_type(inst_bin->valeur_droite->type) << '\n';
                                  imprime_instruction(inst_bin, std::cerr);
                                  std::cerr << '\n';
                              });

                assert_rappel(valeur_gauche->getType() == valeur_droite->getType(), [&]() {
                    std::cerr << erreur::imprime_site(m_espace, inst_bin->site);
                    llvm::errs() << "Type à gauche LLVM " << *valeur_gauche->getType() << '\n';
                    llvm::errs() << "Type à droite LLVM " << *valeur_droite->getType() << '\n';
                    std::cerr << "Type à gauche " << chaine_type(inst_bin->valeur_gauche->type)
                              << '\n';
                    std::cerr << "Type à droite " << chaine_type(inst_bin->valeur_droite->type)
                              << '\n';
                    imprime_instruction(inst_bin, std::cerr);
                    std::cerr << '\n';
                });

                table_valeurs.insère(inst,
                                     m_builder.CreateICmp(cmp, valeur_gauche, valeur_droite));
            }
            else if (inst_bin->op >= OpérateurBinaire::Genre::Comp_Egal_Reel &&
                     inst_bin->op <= OpérateurBinaire::Genre::Comp_Sup_Egal_Reel) {
                auto cmp = cmp_llvm_depuis_operateur(inst_bin->op);
                table_valeurs.insère(inst,
                                     m_builder.CreateFCmp(cmp, valeur_gauche, valeur_droite));
            }
            else {
                auto inst_llvm = inst_llvm_depuis_operateur(inst_bin->op);
                table_valeurs.insère(
                    inst, m_builder.CreateBinOp(inst_llvm, valeur_gauche, valeur_droite));
            }

            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                auto valeur_retour = genere_code_pour_atome(atome, false);
                m_builder.CreateRet(valeur_retour);
            }
            else {
                m_builder.CreateRet(nullptr);
            }
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto inst_acces = inst->comme_acces_index();
            auto valeur_accede = genere_code_pour_atome(inst_acces->accede, false);
            auto valeur_index = genere_code_pour_atome(inst_acces->index, false);

            auto type_accede = inst_acces->accede->type;
            if (type_accede->comme_type_pointeur()->type_pointe->est_type_pointeur()) {
                auto index = std::vector<llvm::Value *>(1);
                index[0] = valeur_index;

                // À FAIRE : ceci est sans doute faux
                auto xx = m_builder.CreateGEP(valeur_accede, index);
                table_valeurs.insère(inst, m_builder.CreateLoad(xx));
            }
            else {
                auto index = std::vector<llvm::Value *>(2);
                index[0] = m_builder.getInt32(0);
                index[1] = valeur_index;
                table_valeurs.insère(inst, m_builder.CreateGEP(valeur_accede, index));
            }

            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto inst_acces = inst->comme_acces_membre();

            auto accede = inst_acces->accede;
            auto valeur_accede = genere_code_pour_atome(accede, false);

            auto index_membre =
                static_cast<AtomeValeurConstante *>(inst_acces->index)->valeur.valeur_entiere;

            auto type_pointe = accede->type->comme_type_pointeur()->type_pointe;

            auto index_reel = index_reel_pour_membre(
                *accede->type->comme_type_pointeur()->type_pointe->comme_type_compose(),
                static_cast<uint32_t>(index_membre));

            if (!type_pointe->est_type_pointeur()) {
                auto index = std::vector<llvm::Value *>(2);
                index[0] = m_builder.getInt32(0);
                index[1] = m_builder.getInt32(index_reel);

                auto valeur_membre = m_builder.CreateInBoundsGEP(valeur_accede, index);
                table_valeurs.insère(inst, valeur_membre);
            }
            else {
                valeur_accede = table_globales.valeur_ou(accede, nullptr);

                auto index = std::vector<llvm::Value *>(1);
                index[0] = m_builder.getInt32(index_reel);

                table_valeurs.insère(inst, m_builder.CreateGEP(valeur_accede, index));
            }

            // À FAIRE : type union

            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto const inst_transtype = inst->comme_transtype();
            auto const valeur = genere_code_pour_atome(inst_transtype->valeur, false);
            auto const type_de = inst_transtype->valeur->type;
            auto const type_vers = inst_transtype->type;
            auto const type_llvm = converti_type_llvm(type_vers);
            auto const cast_op = convertis_type_transtypage(
                inst_transtype->op, type_de, type_vers);
            auto const resultat = m_builder.CreateCast(cast_op, valeur, type_llvm);
            table_valeurs.insère(inst, resultat);
            assert_rappel(!adresse_est_nulle(resultat), [&]() {
                std::cerr << erreur::imprime_site(m_espace, inst_transtype->site);
                imprime_atome(inst_transtype->valeur, std::cerr);
            });
            break;
        }
    }
}

void GeneratriceCodeLLVM::genere_code(const ProgrammeRepreInter &repr_inter)
{
    POUR (repr_inter.globales) {
        // LogDebug::réinitialise_indentation();
        // dbg() << "Prédéclare globale " << it.ident << ' ' << chaine_type(it.type);
        auto valeur_globale = it;

        auto type = valeur_globale->type->comme_type_pointeur()->type_pointe;
        auto type_llvm = converti_type_llvm(type);

        auto nom_globale = llvm::StringRef();

        if (valeur_globale->ident) {
            nom_globale = llvm::StringRef(
                valeur_globale->ident->nom.pointeur(),
                static_cast<size_t>(valeur_globale->ident->nom.taille()));
        }

        auto globale = new llvm::GlobalVariable(*m_module,
                                                type_llvm,
                                                valeur_globale->est_constante,
                                                valeur_globale->est_externe ?
                                                    llvm::GlobalValue::ExternalLinkage :
                                                    llvm::GlobalValue::InternalLinkage,
                                                nullptr,
                                                nom_globale);

        globale->setAlignment(llvm::Align(type->alignement));
        table_globales.insère(valeur_globale, globale);
    }

    POUR (repr_inter.globales) {
        // LogDebug::réinitialise_indentation();
        // dbg() << "Génère code pour globale " << it.ident << ' ' << chaine_type(it.type);
        auto valeur_globale = it;

        auto globale = table_globales.valeur_ou(valeur_globale, nullptr);
        if (valeur_globale->initialisateur) {
            auto valeur_initialisateur = static_cast<llvm::Constant *>(
                genere_code_pour_atome(valeur_globale->initialisateur, true));
            globale->setInitializer(valeur_initialisateur);
        }
        else {
            globale->setInitializer(llvm::ConstantAggregateZero::get(globale->getType()));
        }
    }

    POUR (repr_inter.fonctions) {
        auto atome_fonc = it;

        auto type_fonction = atome_fonc->type->comme_type_fonction();
        auto type_llvm = converti_type_fonction(type_fonction,
                                                atome_fonc->instructions.taille() == 0);

        llvm::Function::Create(type_llvm,
                               llvm::Function::ExternalLinkage,
                               vers_std_string(atome_fonc->nom),
                               m_module);
    }

    // auto index_fonction = 0l;
    POUR (repr_inter.fonctions) {
        auto atome_fonc = it;
        table_valeurs.efface();
        table_blocs.efface();

        // index_fonction++;

        if (atome_fonc->est_externe) {
            continue;
        }

        // dbg() << "Génère code pour : " << atome_fonc->nom;

        auto fonction = m_module->getFunction(vers_std_string(atome_fonc->nom));

        m_fonction_courante = fonction;
        table_valeurs.efface();
        table_blocs.efface();

        /* génère d'abord tous les blocs depuis les labels */
        for (auto inst : atome_fonc->instructions) {
            if (inst->genre != GenreInstruction::LABEL) {
                continue;
            }

            genere_code_pour_instruction(inst);
        }

        auto bloc_entree = table_blocs.valeur_ou(atome_fonc->instructions[0]->comme_label(),
                                                 nullptr);
        m_builder.SetInsertPoint(bloc_entree);

        auto valeurs_args = fonction->arg_begin();

        for (auto &param : atome_fonc->params_entrees) {
            auto const &nom_argument = param->ident->nom;

            auto valeur = &(*valeurs_args++);
            valeur->setName(vers_std_string(nom_argument));

            auto type = param->type->comme_type_pointeur()->type_pointe;
            auto type_llvm = converti_type_llvm(type);

            auto alloc = m_builder.CreateAlloca(type_llvm, 0u);
            alloc->setAlignment(llvm::Align(type->alignement));

            auto store = m_builder.CreateStore(valeur, alloc);
            store->setAlignment(llvm::Align(type->alignement));

            table_valeurs.insère(param, alloc);
        }

        /* crée une variable local pour la valeur de sortie */
        auto type_fonction = atome_fonc->type->comme_type_fonction();
        if (!type_fonction->type_sortie->est_type_rien()) {
            auto param = atome_fonc->param_sortie;
            auto type_pointeur = param->type->comme_type_pointeur();
            auto type_pointe = type_pointeur->type_pointe;
            auto type_llvm = converti_type_llvm(type_pointe);
            auto alloca = m_builder.CreateAlloca(type_llvm, 0u);
            alloca->setAlignment(llvm::Align(type_pointeur->type_pointe->alignement));
            table_valeurs.insère(param, alloca);
        }

        /* Génère le code pour les accès de membres des retours multiples. */
        if (atome_fonc->decl && atome_fonc->decl->params_sorties.taille() > 1) {
            for (auto &param : atome_fonc->decl->params_sorties) {
                genere_code_pour_instruction(
                    param->comme_declaration_variable()->atome->comme_instruction());
            }
        }

        //        std::cerr << "Fonction " << index_fonction << " / " <<
        //        repr_inter.fonctions.taille()
        //                  << '\n';
        // imprime_fonction(atome_fonc, std::cerr);
        for (auto inst : atome_fonc->instructions) {
            if (inst->genre == GenreInstruction::LABEL) {
                auto bloc = table_blocs.valeur_ou(inst->comme_label(), nullptr);
                m_builder.SetInsertPoint(bloc);
                continue;
            }

            genere_code_pour_instruction(inst);
        }

        if (manager_fonctions) {
            manager_fonctions->run(*m_fonction_courante);
        }

        m_fonction_courante = nullptr;
    }
}

llvm::Constant *GeneratriceCodeLLVM::valeur_pour_chaine(const kuri::chaine &chaine,
                                                        int64_t taille_chaine)
{
    auto iter = valeurs_chaines_globales.valeur_ou(chaine, nullptr);
    if (iter) {
        return iter;
    }

    // @.chn [N x i8] c"...0"
    auto type_tableau = m_espace.compilatrice().typeuse.type_tableau_fixe(
        TypeBase::Z8, static_cast<int>(taille_chaine + 1));

    auto constante = llvm::ConstantDataArray::getString(m_contexte_llvm, vers_std_string(chaine));

    auto tableu_chaine_c = new llvm::GlobalVariable(*m_module,
                                                    converti_type_llvm(type_tableau),
                                                    true,
                                                    llvm::GlobalValue::PrivateLinkage,
                                                    constante,
                                                    ".chn");

    // prend le pointeur vers le premier élément.
    auto idx = std::vector<llvm::Constant *>{
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_contexte_llvm), 0),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(m_contexte_llvm), 0)};

    auto pointeur_chaine_c = llvm::ConstantExpr::getGetElementPtr(
        converti_type_llvm(type_tableau), tableu_chaine_c, idx, true);

    auto valeur_taille = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(m_contexte_llvm), static_cast<uint64_t>(chaine.taille()), false);

    auto type_chaine = converti_type_llvm(TypeBase::CHAINE);

    auto struct_chaine = llvm::ConstantStruct::get(
        static_cast<llvm::StructType *>(type_chaine), pointeur_chaine_c, valeur_taille);

    valeurs_chaines_globales.insère(chaine, struct_chaine);

    return struct_chaine;
}

bool initialise_llvm()
{
    if (llvm::InitializeNativeTarget() || llvm::InitializeNativeTargetAsmParser() ||
        llvm::InitializeNativeTargetAsmPrinter()) {
        std::cerr << "Ne peut pas initialiser LLVM !\n";
        return false;
    }

    /* Initialise les passes. */
    auto &registre = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(registre);
    llvm::initializeScalarOpts(registre);
    llvm::initializeObjCARCOpts(registre);
    llvm::initializeVectorization(registre);
    llvm::initializeIPO(registre);
    llvm::initializeAnalysis(registre);
    llvm::initializeTransformUtils(registre);
    llvm::initializeInstCombine(registre);
    llvm::initializeInstrumentation(registre);
    llvm::initializeTarget(registre);

    /* Pour les passes de transformation de code, seuls celles d'IR à IR sont
     * supportées. */
    llvm::initializeCodeGenPreparePass(registre);
    llvm::initializeAtomicExpandPass(registre);
    llvm::initializeWinEHPreparePass(registre);
    llvm::initializeDwarfEHPrepareLegacyPassPass(registre);
    llvm::initializeSjLjEHPreparePass(registre);
    llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registre);
    llvm::initializeGlobalMergePass(registre);
    llvm::initializeInterleavedAccessPass(registre);
    llvm::initializeUnreachableBlockElimLegacyPassPass(registre);

    return true;
}

void issitialise_llvm()
{
    llvm::llvm_shutdown();
}

/**
 * Ajoute les passes d'optimisation au ménageur en fonction du niveau
 * d'optimisation.
 */
static void ajoute_passes(llvm::legacy::FunctionPassManager &manager_fonctions,
                          uint32_t niveau_optimisation,
                          uint32_t niveau_taille)
{
    llvm::PassManagerBuilder builder;
    builder.OptLevel = niveau_optimisation;
    builder.SizeLevel = niveau_taille;
    builder.DisableUnrollLoops = (niveau_optimisation == 0);

    /* Pour plus d'informations sur les vectoriseurs, suivre le lien :
     * http://llvm.org/docs/Vectorizers.html */
    builder.LoopVectorize = (niveau_optimisation > 1 && niveau_taille < 2);
    builder.SLPVectorize = (niveau_optimisation > 1 && niveau_taille < 2);

    builder.populateFunctionPassManager(manager_fonctions);
}

/**
 * Initialise le ménageur de passes fonctions du contexte selon le niveau
 * d'optimisation.
 */
static void initialise_optimisation(NiveauOptimisation optimisation, GeneratriceCodeLLVM &contexte)
{
    if (contexte.manager_fonctions == nullptr) {
        contexte.manager_fonctions = new llvm::legacy::FunctionPassManager(contexte.m_module);
    }

    switch (optimisation) {
        case NiveauOptimisation::AUCUN:
            break;
        case NiveauOptimisation::O0:
            ajoute_passes(*contexte.manager_fonctions, 0, 0);
            break;
        case NiveauOptimisation::O1:
            ajoute_passes(*contexte.manager_fonctions, 1, 0);
            break;
        case NiveauOptimisation::O2:
            ajoute_passes(*contexte.manager_fonctions, 2, 0);
            break;
        case NiveauOptimisation::Os:
            ajoute_passes(*contexte.manager_fonctions, 2, 1);
            break;
        case NiveauOptimisation::Oz:
            ajoute_passes(*contexte.manager_fonctions, 2, 2);
            break;
        case NiveauOptimisation::O3:
            ajoute_passes(*contexte.manager_fonctions, 3, 0);
            break;
    }
}

/* Chemin du fichier objet généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_objet_llvm()
{
    return chemin_fichier_objet_temporaire_pour("kuri");
}

/* Chemin du fichier objet "execution_kuri" généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_objet_execution_llvm()
{
    return chemin_fichier_objet_temporaire_pour("execution_kuri");
}

/* Chemin du fichier de code binaire LLVM généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_bc_llvm()
{
    return kuri::chemin_systeme::chemin_temporaire("kuri.bc");
}

/* Chemin du fichier de code LLVM généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_ll_llvm()
{
    return kuri::chemin_systeme::chemin_temporaire("kuri.ll");
}

/* Chemin du fichier ".s" généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_s_llvm()
{
    return kuri::chemin_systeme::chemin_temporaire("kuri.s");
}

static llvm::StringRef vers_string_ref(kuri::chaine_statique chaine)
{
    return llvm::StringRef(chaine.pointeur(), size_t(chaine.taille()));
}

static bool ecris_fichier_objet(llvm::TargetMachine *machine_cible, llvm::Module &module)
{
#if 1
    auto chemin_sortie = chemin_fichier_objet_llvm();
    std::error_code ec;

    llvm::raw_fd_ostream dest(
        llvm::StringRef(chemin_sortie.pointeur(), size_t(chemin_sortie.taille())),
        ec,
        llvm::sys::fs::F_None);

    if (ec) {
        std::cerr << "Ne put pas ouvrir le fichier '" << chemin_sortie << "'\n";
        return false;
    }

    llvm::legacy::PassManager pass;
    auto type_fichier = llvm::CGFT_ObjectFile;

    if (machine_cible->addPassesToEmitFile(pass, dest, nullptr, type_fichier)) {
        std::cerr << "La machine cible ne peut pas émettre ce type de fichier\n";
        return false;
    }

    pass.run(module);
    dest.flush();
#else
    auto const fichier_ll = chemin_fichier_ll_llvm();
    auto const fichier_bc = chemin_fichier_bc_llvm();
    auto const fichier_s = chemin_fichier_s_llvm();

    // https://stackoverflow.com/questions/1419139/llvm-linking-problem?rq=1
    std::error_code ec;
    llvm::raw_fd_ostream dest(vers_string_ref(fichier_ll), ec, llvm::sys::fs::F_None);
    module.print(dest, nullptr);

    /* Génère le fichier de code binaire depuis le fichier de RI LLVM. */
    auto commande = enchaine("llvm-as ", fichier_ll, " -o ", fichier_bc, "\0");
    if (system(commande.pointeur()) != 0) {
        return false;
    }

    /* Génère le fichier d'instruction assembly depuis le fichier de code binaire. */
    commande = enchaine("llvm-as ", fichier_bc, " -o ", fichier_s, "\0");
    if (system(commande.pointeur()) != 0) {
        return false;
    }
#endif
    return true;
}

#ifndef NDEBUG
static bool valide_llvm_ir(llvm::Module &module)
{
    auto const fichier_ll = chemin_fichier_ll_llvm();
    auto const fichier_bc = chemin_fichier_bc_llvm();

    std::error_code ec;
    llvm::raw_fd_ostream dest(vers_string_ref(fichier_ll), ec, llvm::sys::fs::F_None);
    module.print(dest, nullptr);

    /* Génère le fichier de code binaire depuis le fichier de RI LLVM, ce qui vérifiera que la RI
     * est correcte. */
    auto commande = enchaine("llvm-as ", fichier_ll, " -o ", fichier_bc, "\0");
    return system(commande.pointeur()) == 0;
}
#endif

static bool crée_executable(EspaceDeTravail const &espace,
                            const kuri::chaine &dest,
                            const kuri::chemin_systeme &racine_kuri)
{
    /* Compile le fichier objet qui appelera 'fonction principale'. */
    auto chemin_execution = chemin_fichier_objet_execution_llvm();
    if (!kuri::chemin_systeme::existe(chemin_execution)) {
        auto const &chemin_execution_S = racine_kuri / "fichiers/execution_kuri.S";

        Enchaineuse ss;
        ss << "as -o " << chemin_execution;
        ss << chemin_execution_S;
        ss << '\0';

        const auto err = system(ss.chaine().pointeur());

        if (err != 0) {
            std::cerr << "Ne peut pas créer " << chemin_execution << " !\n";
            return false;
        }
    }

    auto chemin_objet = chemin_fichier_objet_llvm();

    if (!kuri::chemin_systeme::existe(chemin_objet)) {
        std::cerr << "Le fichier objet n'a pas été émis !\n Utiliser la commande -o !\n";
        return false;
    }

    Enchaineuse ss;
#if 1
    ss << "gcc ";
    ss << racine_kuri / "fichiers/point_d_entree.c";
    ss << chemin_fichier_objet_r16(espace.options.architecture) << " ";

#else
    ss << "ld ";
    /* ce qui chargera le programme */
    ss << "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ";
    ss << "-m elf_x86_64 ";
    ss << "--hash-style=gnu ";
    ss << "-lc ";
    ss << chemin_execution << " ";
#endif

    ss << " " << chemin_objet << " ";
    ss << "-o " << dest;
    ss << '\0';

    auto commande = ss.chaine();

    std::cout << "Exécution de la commande : " << commande << '\n';

    const auto err = system(commande.pointeur());

    if (err != 0) {
        std::cerr << "Ne peut pas créer l'executable !\n";
        return false;
    }

    return true;
}

CoulisseLLVM::~CoulisseLLVM()
{
    delete m_module;
    delete m_machine_cible;
}

bool CoulisseLLVM::génère_code_impl(Compilatrice & /*compilatrice*/,
                                    EspaceDeTravail &espace,
                                    Programme *programme,
                                    CompilatriceRI &compilatrice_ri,
                                    Broyeuse &)
{
    if (!initialise_llvm()) {
        return false;
    }

    auto const triplet_cible = llvm::sys::getDefaultTargetTriple();

    auto erreur = std::string{""};
    auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

    if (!cible) {
        std::cerr << erreur << '\n';
        return false;
    }

    auto repr_inter = représentation_intermédiaire_programme(espace, compilatrice_ri, *programme);
    if (!repr_inter.has_value()) {
        return false;
    }

    auto CPU = "generic";
    auto feature = "";
    auto options_cible = llvm::TargetOptions{};
    auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    m_machine_cible = cible->createTargetMachine(triplet_cible, CPU, feature, options_cible, RM);

    auto generatrice = GeneratriceCodeLLVM(espace);

    m_module = new llvm::Module("Module", generatrice.m_contexte_llvm);
    m_module->setDataLayout(m_machine_cible->createDataLayout());
    m_module->setTargetTriple(triplet_cible);

    generatrice.m_module = m_module;

    initialise_optimisation(espace.options.niveau_optimisation, generatrice);

    generatrice.genere_code(*repr_inter);

    delete generatrice.manager_fonctions;

#ifndef NDEBUG
    if (!valide_llvm_ir(*m_module)) {
        espace.rapporte_erreur_sans_site("Erreur interne, impossible de générer le code LLVM.");
        return false;
    }
#endif

    return true;
}

bool CoulisseLLVM::crée_fichier_objet_impl(Compilatrice & /*compilatrice*/,
                                           EspaceDeTravail &espace,
                                           Programme *programme,
                                           CompilatriceRI & /*constructrice_ri*/,
                                           Broyeuse &)
{
    if (espace.options.resultat != ResultatCompilation::EXECUTABLE) {
        return true;
    }

    if (!ecris_fichier_objet(m_machine_cible, *m_module)) {
        return false;
    }

    return true;
}

bool CoulisseLLVM::crée_exécutable_impl(Compilatrice &compilatrice,
                                        EspaceDeTravail &espace,
                                        Programme * /*programme*/)
{
    if (!::crée_executable(
            espace, nom_sortie_resultat_final(espace.options), compilatrice.racine_kuri)) {
        return false;
    }

    return true;
}
