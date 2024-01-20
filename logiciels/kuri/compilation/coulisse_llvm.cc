/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "coulisse_llvm.hh"

#include "biblinternes/outils/conditions.h"

#include <iostream>

#include "utilitaires/poule_de_taches.hh"

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

#include "arbre_syntaxique/cas_genre_noeud.hh"

#include "structures/chemin_systeme.hh"
#include "structures/table_hachage.hh"

#include "compilatrice.hh"
#include "environnement.hh"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "utilitaires/log.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/instructions.hh"

inline bool adresse_est_nulle(void *adresse)
{
    /* 0xbebebebebebebebe peut être utilisé par les débogueurs. */
    return adresse == nullptr || adresse == reinterpret_cast<void *>(0xbebebebebebebebe);
}

/* ------------------------------------------------------------------------- */
/** \name Utilitaires locaux.
 * \{ */

static VisibilitéSymbole donne_visibilité_fonction(AtomeFonction const *fonction)
{
    if (!fonction->decl) {
        return VisibilitéSymbole::INTERNE;
    }

    if (fonction->est_externe) {
        return VisibilitéSymbole::EXPORTÉ;
    }

    if (dls::outils::est_element(fonction->decl->ident,
                                 ID::__point_d_entree_dynamique,
                                 ID::__point_d_entree_systeme,
                                 ID::__point_de_sortie_dynamique)) {
        return VisibilitéSymbole::EXPORTÉ;
    }

    return fonction->decl->visibilité_symbole;
}

static llvm::GlobalValue::LinkageTypes convertis_visibilité_symbole(
    VisibilitéSymbole const visibilité)
{
    switch (visibilité) {
        case VisibilitéSymbole::EXPORTÉ:
        {
            return llvm::GlobalValue::ExternalLinkage;
        }
        case VisibilitéSymbole::INTERNE:
        {
            return llvm::GlobalValue::InternalLinkage;
        }
    }
    return llvm::GlobalValue::InternalLinkage;
}

static llvm::GlobalValue::LinkageTypes donne_liaison_fonction(AtomeFonction const *fonction)
{
    auto visibilité = donne_visibilité_fonction(fonction);
    return convertis_visibilité_symbole(visibilité);
}

/** \} */

/* ************************************************************************** */

static const Logueuse &operator<<(const Logueuse &log_debug, const llvm::Value &llvm_value)
{
    std::string str;
    llvm::raw_string_ostream ss(str);
    ss << llvm_value;
    return log_debug << str;
}

static const Logueuse &operator<<(const Logueuse &log_debug, const llvm::Constant &llvm_value)
{
    std::string str;
    llvm::raw_string_ostream ss(str);
    ss << llvm_value;
    return log_debug << str;
}

static const Logueuse &operator<<(const Logueuse &log_debug,
                                  const llvm::GlobalVariable &llvm_value)
{
    std::string str;
    llvm::raw_string_ostream ss(str);
    ss << llvm_value;
    return log_debug << str;
}

static const Logueuse &operator<<(const Logueuse &log_debug, const llvm::Type &llvm_value)
{
    std::string str;
    llvm::raw_string_ostream ss(str);
    ss << llvm_value;
    return log_debug << str;
}

/* ************************************************************************** */

static llvm::StringRef vers_string_ref(kuri::chaine_statique chaine)
{
    return llvm::StringRef(chaine.pointeur(), size_t(chaine.taille()));
}

static llvm::StringRef vers_string_ref(IdentifiantCode const *ident)
{
    if (!ident) {
        return {};
    }
    return vers_string_ref(ident->nom);
}

static auto inst_llvm_depuis_opérateur(OpérateurBinaire::Genre genre)
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

static auto cmp_llvm_depuis_opérateur(OpérateurBinaire::Genre genre)
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
        case TypeTranstypage::AUGMENTE_NATUREL_VERS_RELATIF:
            return CastOps::ZExt;
        case TypeTranstypage::AUGMENTE_RELATIF:
        case TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL:
            return CastOps::SExt;
        case TypeTranstypage::AUGMENTE_REEL:
            return CastOps::FPExt;
        case TypeTranstypage::DIMINUE_NATUREL:
        case TypeTranstypage::DIMINUE_RELATIF:
        case TypeTranstypage::DIMINUE_NATUREL_VERS_RELATIF:
        case TypeTranstypage::DIMINUE_RELATIF_VERS_NATUREL:
            return CastOps::Trunc;
        case TypeTranstypage::DIMINUE_REEL:
            return CastOps::FPTrunc;
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
            return CastOps::PtrToInt;
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
            return CastOps::IntToPtr;
        case TypeTranstypage::REEL_VERS_ENTIER_RELATIF:
            return CastOps::FPToSI;
        case TypeTranstypage::REEL_VERS_ENTIER_NATUREL:
            return CastOps::FPToUI;
        case TypeTranstypage::ENTIER_RELATIF_VERS_REEL:
            return CastOps::SIToFP;
        case TypeTranstypage::ENTIER_NATUREL_VERS_REEL:
            return CastOps::UIToFP;
        case TypeTranstypage::BITS:
            if (type_de->est_type_bool() && est_type_entier(type_vers)) {
                /* LLVM représente les bool avec i1, nous devons transtyper en augmentant la taille
                 * du type. */
                return CastOps::ZExt;
            }
            return CastOps::BitCast;
    }

    return static_cast<CastOps>(0);
}

/* ************************************************************************** */

struct GénératriceCodeLLVM {
  private:
    kuri::tableau<llvm::Value *> table_valeurs{};
    kuri::tableau<llvm::BasicBlock *> table_blocs{};
    kuri::table_hachage<Atome const *, llvm::GlobalVariable *> table_globales{
        "Table valeurs globales LLVM"};
    kuri::table_hachage<Type const *, llvm::Type *> table_types{"Table types LLVM"};
    EspaceDeTravail &m_espace;

    DonnéesModule &données_module;

    llvm::Function *m_fonction_courante = nullptr;
    llvm::Module *m_module = nullptr;
    llvm::LLVMContext &m_contexte_llvm;
    llvm::IRBuilder<> m_builder;
    llvm::legacy::FunctionPassManager *manager_fonctions = nullptr;

    /* Pour le débogage. */
    int m_nombre_fonctions_compilées = 0;

  public:
    GénératriceCodeLLVM(EspaceDeTravail &espace, DonnéesModule &module);

    GénératriceCodeLLVM(GénératriceCodeLLVM const &) = delete;
    GénératriceCodeLLVM &operator=(const GénératriceCodeLLVM &) = delete;

    ~GénératriceCodeLLVM();

    void génère_code();

  private:
    void initialise_optimisation(NiveauOptimisation optimisation);

    llvm::Type *convertis_type_llvm(Type const *type);

    llvm::FunctionType *convertis_type_fonction(TypeFonction const *type);

    llvm::StructType *convertis_type_composé(TypeCompose const *type, kuri::chaine_statique nom);

    llvm::Value *génère_code_pour_atome(Atome const *atome, bool pour_globale);

    void génère_code_pour_instruction(Instruction const *inst);

    void génère_code_pour_fonction(const AtomeFonction *atome_fonc);

    llvm::AllocaInst *crée_allocation(InstructionAllocation const *alloc);

    llvm::Value *génère_valeur_données_constantes(
        const AtomeConstanteDonnéesConstantes *constante);

    void définis_valeur_instruction(Instruction const *inst, llvm::Value *valeur);

    llvm::Function *donne_ou_crée_déclaration_fonction(AtomeFonction const *fonction);
    llvm::GlobalVariable *donne_ou_crée_déclaration_globale(AtomeGlobale const *globale);
};

GénératriceCodeLLVM::GénératriceCodeLLVM(EspaceDeTravail &espace, DonnéesModule &module)
    : m_espace(espace), données_module(module), m_module(module.module),
      m_contexte_llvm(m_module->getContext()), m_builder(m_contexte_llvm)
{
    initialise_optimisation(espace.options.niveau_optimisation);
}

GénératriceCodeLLVM::~GénératriceCodeLLVM()
{
    delete manager_fonctions;
}

/**
 * Ajoute les passes d'optimisation au manageur en fonction du niveau
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
 * Initialise le manageur de passes fonctions du contexte selon le niveau
 * d'optimisation.
 */
void GénératriceCodeLLVM::initialise_optimisation(NiveauOptimisation optimisation)
{
    if (manager_fonctions == nullptr) {
        manager_fonctions = new llvm::legacy::FunctionPassManager(m_module);
    }

    switch (optimisation) {
        case NiveauOptimisation::AUCUN:
            break;
        case NiveauOptimisation::O0:
            ajoute_passes(*manager_fonctions, 0, 0);
            break;
        case NiveauOptimisation::O1:
            ajoute_passes(*manager_fonctions, 1, 0);
            break;
        case NiveauOptimisation::O2:
            ajoute_passes(*manager_fonctions, 2, 0);
            break;
        case NiveauOptimisation::Os:
            ajoute_passes(*manager_fonctions, 2, 1);
            break;
        case NiveauOptimisation::Oz:
            ajoute_passes(*manager_fonctions, 2, 2);
            break;
        case NiveauOptimisation::O3:
            ajoute_passes(*manager_fonctions, 3, 0);
            break;
    }
}

llvm::Type *GénératriceCodeLLVM::convertis_type_llvm(Type const *type)
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
        case GenreNoeud::POLYMORPHIQUE:
        {
            type_llvm = nullptr;
            table_types.insère(type, type_llvm);
            break;
        }
        case GenreNoeud::TUPLE:
        {
            return convertis_type_composé(type->comme_type_tuple(), "tuple");
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonc = type->comme_type_fonction();
            type_llvm = convertis_type_fonction(type_fonc);
            type_llvm = llvm::PointerType::get(type_llvm, 0);
            break;
        }
        case GenreNoeud::EINI:
        {
            return convertis_type_composé(type->comme_type_eini(), "eini");
        }
        case GenreNoeud::CHAINE:
        {
            return convertis_type_composé(type->comme_type_chaine(), "chaine");
        }
        case GenreNoeud::RIEN:
        {
            type_llvm = llvm::Type::getVoidTy(m_contexte_llvm);
            break;
        }
        case GenreNoeud::BOOL:
        {
            type_llvm = llvm::Type::getInt1Ty(m_contexte_llvm);
            break;
        }
        case GenreNoeud::OCTET:
        {
            type_llvm = llvm::Type::getInt8Ty(m_contexte_llvm);
            break;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            type_llvm = llvm::Type::getInt32Ty(m_contexte_llvm);
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
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
        case GenreNoeud::TYPE_DE_DONNEES:
        {
            type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
            break;
        }
        case GenreNoeud::REEL:
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
        case GenreNoeud::REFERENCE:
        case GenreNoeud::POINTEUR:
        {
            auto type_deref = type_déréférencé_pour(type);

            // Les pointeurs vers rien (void) ne sont pas valides avec LLVM
            // @Incomplet : LLVM n'a pas de pointeur nul
            if (!type_deref || type_deref->est_type_rien()) {
                type_deref = TypeBase::Z8;
            }

            auto type_deref_llvm = convertis_type_llvm(type_deref);
            type_llvm = llvm::PointerType::get(type_deref_llvm, 0);
            break;
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_struct = type->comme_type_union();

            if (type_struct->type_structure) {
                type_llvm = convertis_type_llvm(type_struct->type_structure);
                break;
            }

            assert(type_struct->est_nonsure);

            auto nom_nonsur = enchaine("union_nonsure.", type_struct->ident->nom);

            // création d'une structure ne contenant que le membre le plus grand
            auto type_le_plus_grand = type_struct->type_le_plus_grand;

            auto type_max_llvm = convertis_type_llvm(type_le_plus_grand);
            type_llvm = llvm::StructType::create(
                m_contexte_llvm, {type_max_llvm}, vers_string_ref(nom_nonsur));
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            return convertis_type_composé(type->comme_type_structure(), "struct");
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_var = type->comme_type_variadique();
            /* Utilisons le type tranche afin que le code IR LLVM utilise le type final. */
            if (type_var->type_tranche != nullptr) {
                type_llvm = convertis_type_llvm(type_var->type_tranche);
            }
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            return convertis_type_composé(type->comme_type_tableau_dynamique(), "tableau");
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            return convertis_type_composé(type->comme_type_tranche(), "tranche");
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_deref_llvm = convertis_type_llvm(type_déréférencé_pour(type));
            auto const taille = type->comme_type_tableau_fixe()->taille;

            type_llvm = llvm::ArrayType::get(type_deref_llvm, static_cast<uint64_t>(taille));
            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            type_llvm = convertis_type_llvm(type_enum->type_sous_jacent);
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            type_llvm = convertis_type_llvm(type_opaque->type_opacifie);
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    table_types.insère(type, type_llvm);
    return type_llvm;
}

llvm::FunctionType *GénératriceCodeLLVM::convertis_type_fonction(TypeFonction const *type)
{
    std::vector<llvm::Type *> paramètres;
    paramètres.reserve(static_cast<size_t>(type->types_entrees.taille()));

    auto est_variadique = false;

    POUR (type->types_entrees) {
        if (it->est_type_variadique()) {
            auto type_variadique = it->comme_type_variadique();
            /* Type variadique externe. */
            if (type_variadique->type_pointe == nullptr) {
                est_variadique = true;
                break;
            }
        }

        paramètres.push_back(convertis_type_llvm(it));
    }

    auto type_sortie_llvm = convertis_type_llvm(type->type_sortie);
    assert(type_sortie_llvm);

    return llvm::FunctionType::get(type_sortie_llvm, paramètres, est_variadique);
}

llvm::StructType *GénératriceCodeLLVM::convertis_type_composé(TypeCompose const *type,
                                                              kuri::chaine_statique classe)
{
    auto nom = type->ident ? enchaine(classe, ".", type->ident->nom) : kuri::chaine(classe);

    /* Pour les structures récursives, il faut créer un type
     * opaque, dont le corps sera renseigné à la fin */
    auto type_opaque = llvm::StructType::create(m_contexte_llvm, vers_string_ref(nom));
    table_types.insère(type, type_opaque);

    llvm::SmallVector<llvm::Type *, 6> types_membres;
    types_membres.reserve(static_cast<size_t>(type->membres.taille()));

    POUR (type->donne_membres_pour_code_machine()) {
        types_membres.push_back(convertis_type_llvm(it.type));
    }

    auto est_compacte = false;
    if (type->est_type_structure()) {
        auto type_structure = type->comme_type_structure();
        est_compacte = type_structure->est_compacte;
    }

    type_opaque->setBody(types_membres, est_compacte);

    return type_opaque;
}

llvm::Value *GénératriceCodeLLVM::génère_code_pour_atome(Atome const *atome, bool pour_globale)
{
    // auto incrémentation_temp = Logueuse::IncrémenteuseTemporaire();
    // dbg() << __func__ << ", atome: " << static_cast<int>(atome->genre_atome);

    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            // dbg() << "FONCTION";
            return donne_ou_crée_déclaration_fonction(atome->comme_fonction());
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype_const = atome->comme_transtype_constant();
            auto valeur = génère_code_pour_atome(transtype_const->valeur, pour_globale);
            auto valeur_ = llvm::ConstantExpr::getBitCast(llvm::cast<llvm::Constant>(valeur),
                                                          convertis_type_llvm(atome->type));
            // dbg() << "TRANSTYPE_CONSTANT: " << *valeur_;
            return valeur_;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto acces = atome->comme_accès_index_constant();
            auto index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(m_contexte_llvm),
                                                uint64_t(acces->index));
            assert(index);
            auto accede = génère_code_pour_atome(acces->accédé, pour_globale);
            assert_rappel(accede, [&]() {
                dbg() << "L'accédé est de genre " << acces->accédé->genre_atome << " ("
                      << acces->accédé << ")\n"
                      << imprime_information_atome(acces->accédé);
            });

            auto index_array = llvm::SmallVector<llvm::Value *>();
            auto type_accede = acces->donne_type_accédé();
            if (!type_accede->est_type_pointeur()) {
                auto type_z32 = llvm::Type::getInt32Ty(m_contexte_llvm);
                index_array.push_back(llvm::ConstantInt::get(type_z32, 0));
            }
            index_array.push_back(index);

            // dbg() << "ACCES_INDEX_CONSTANT: index=" << *index << ", accede=" << *accede;

            return llvm::ConstantExpr::getInBoundsGetElementPtr(
                nullptr, llvm::cast<llvm::Constant>(accede), index_array);
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            return llvm::ConstantPointerNull::get(
                static_cast<llvm::PointerType *>(convertis_type_llvm(atome->type)));
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            auto type = atome->comme_constante_type()->type_de_données;
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type),
                                          type->index_dans_table_types);
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = atome->comme_taille_de()->type_de_données;
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type), type->taille_octet);
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            auto type = atome->comme_index_table_type()->type_de_données;
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type),
                                          type->index_dans_table_types);
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto type = constante_réelle->type;
            if (type->taille_octet == 2) {
                return llvm::ConstantInt::get(llvm::Type::getInt16Ty(m_contexte_llvm),
                                              static_cast<unsigned>(constante_réelle->valeur));
            }

            return llvm::ConstantFP::get(convertis_type_llvm(atome->type),
                                         constante_réelle->valeur);
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type),
                                          constante_entière->valeur);
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type),
                                          constante_booléenne->valeur);
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            return llvm::ConstantInt::get(convertis_type_llvm(atome->type), caractère->valeur);
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = atome->comme_constante_structure();
            auto type = structure->type->comme_type_compose();
            auto tableau_valeur = structure->donne_atomes_membres();

            auto tableau_membre = std::vector<llvm::Constant *>();

            POUR_INDEX (type->donne_membres_pour_code_machine()) {
                static_cast<void>(it);
                // dbg() << "Génère code pour le membre " << it.nom->nom;
                auto valeur = llvm::cast<llvm::Constant>(
                    génère_code_pour_atome(tableau_valeur[index_it], pour_globale));

                tableau_membre.push_back(valeur);
            }

            return llvm::ConstantStruct::get(
                llvm::cast<llvm::StructType>(convertis_type_llvm(type)), tableau_membre);
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau = atome->comme_constante_tableau();
            auto éléments = tableau->donne_atomes_éléments();

            std::vector<llvm::Constant *> valeurs;
            valeurs.reserve(static_cast<size_t>(éléments.taille()));

            POUR (éléments) {
                auto valeur = génère_code_pour_atome(it, pour_globale);
                valeurs.push_back(llvm::cast<llvm::Constant>(valeur));
            }

            auto type_llvm = convertis_type_llvm(atome->type);
            auto résultat = llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(type_llvm),
                                                     valeurs);
            // dbg() << "TABLEAU_FIXE : " << *résultat;
            return résultat;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            auto constante = atome->comme_données_constantes();
            auto valeur_ = génère_valeur_données_constantes(constante);
            // dbg() << "TABLEAU_DONNEES_CONSTANTES: " << *valeur_;
            return valeur_;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto const init_tableau = atome->comme_initialisation_tableau();
            auto const type_tableau = init_tableau->type->comme_type_tableau_fixe();
            auto type_llvm = convertis_type_llvm(type_tableau);

            std::vector<llvm::Constant *> valeurs(size_t(type_tableau->taille));

            auto valeur = génère_code_pour_atome(init_tableau->valeur, pour_globale);
            POUR (valeurs) {
                it = llvm::cast<llvm::Constant>(valeur);
            }

            return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(type_llvm), valeurs);
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = atome->comme_instruction();
            return table_valeurs[inst->numero];
        }
        case Atome::Genre::GLOBALE:
        {
            auto valeur = donne_ou_crée_déclaration_globale(atome->comme_globale());
            // dbg() << "GLOBALE: " << *valeur;
            return valeur;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            assert(false);
            return nullptr;
        }
    }

    return nullptr;
}

void GénératriceCodeLLVM::génère_code_pour_instruction(const Instruction *inst)
{
    // auto incrémentation_temp = Logueuse::IncrémenteuseTemporaire();
    // dbg() << __func__;

    switch (inst->genre) {
        case GenreInstruction::ALLOCATION:
        {
            auto alloc = inst->comme_alloc();
            crée_allocation(alloc);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            auto arguments = std::vector<llvm::Value *>();

            POUR (inst_appel->args) {
                arguments.push_back(génère_code_pour_atome(it, false));
            }

            auto valeur_fonction = génère_code_pour_atome(inst_appel->appelé, false);
            assert_rappel(!adresse_est_nulle(valeur_fonction), [&]() {
                dbg() << erreur::imprime_site(m_espace, inst_appel->site) << '\n'
                      << imprime_atome(inst_appel->appelé);
            });

            auto type_fonction =
                convertis_type_llvm(inst_appel->appelé->type)->getPointerElementType();

            auto callee = llvm::FunctionCallee(llvm::cast<llvm::FunctionType>(type_fonction),
                                               valeur_fonction);

            auto call_inst = m_builder.CreateCall(callee, arguments);

            llvm::Value *résultat = call_inst;

            if (!inst_appel->type->est_type_rien()) {
                /* Crée une temporaire sinon la valeur sera du type fonction... */
                auto type_retour = convertis_type_llvm(inst_appel->type);
                auto alloca = m_builder.CreateAlloca(type_retour, 0u);
                m_builder.CreateAlignedStore(
                    résultat, alloca, llvm::MaybeAlign(inst_appel->type->alignement));
                résultat = m_builder.CreateLoad(type_retour, alloca);
            }

            définis_valeur_instruction(inst, résultat);
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            m_builder.CreateBr(table_blocs[inst_branche->label->id]);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            auto condition = génère_code_pour_atome(inst_branche->condition, false);
            auto bloc_si_vrai = table_blocs[inst_branche->label_si_vrai->id];
            auto bloc_si_faux = table_blocs[inst_branche->label_si_faux->id];
            m_builder.CreateCondBr(condition, bloc_si_vrai, bloc_si_faux);
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargée;
            auto valeur = génère_code_pour_atome(charge, false);
            assert(valeur != nullptr);

            auto load = m_builder.CreateLoad(valeur, "");
            load->setAlignment(llvm::Align(charge->type->alignement));
            définis_valeur_instruction(inst, load);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto valeur = génère_code_pour_atome(inst_stocke->source, false);
            auto ou = inst_stocke->destination;
            auto valeur_ou = génère_code_pour_atome(ou, false);

            assert_rappel(!adresse_est_nulle(valeur_ou), [&]() {
                dbg() << erreur::imprime_site(m_espace, inst_stocke->site) << '\n'
                      << imprime_atome(inst_stocke) << '\n'
                      << imprime_information_atome(inst_stocke->destination);
            });
            assert_rappel(!adresse_est_nulle(valeur), [&]() {
                dbg() << erreur::imprime_site(m_espace, inst_stocke->site) << '\n'
                      << imprime_atome(inst_stocke) << '\n'
                      << imprime_information_atome(inst_stocke->source);
            });

            /* Crash si l'alignement n'est pas renseigné. */
            auto alignement = llvm::MaybeAlign(inst_stocke->source->type->alignement);
            m_builder.CreateAlignedStore(valeur, valeur_ou, alignement);
            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            auto bloc = llvm::BasicBlock::Create(m_contexte_llvm, "", m_fonction_courante);
            if (table_blocs.taille() <= inst_label->id) {
                table_blocs.redimensionne(inst_label->id + 1);
            }
            table_blocs[inst_label->id] = bloc;
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();
            auto valeur = génère_code_pour_atome(inst_un->valeur, false);

            switch (inst_un->op) {
                case OpérateurUnaire::Genre::Positif:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Invalide:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Complement:
                {
                    if (inst_un->type->est_type_reel()) {
                        valeur = m_builder.CreateFNeg(valeur);
                    }
                    else {
                        valeur = m_builder.CreateNeg(valeur);
                    }
                    break;
                }
                case OpérateurUnaire::Genre::Non_Binaire:
                {
                    valeur = m_builder.CreateNot(valeur);
                    break;
                }
            }

            définis_valeur_instruction(inst, valeur);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            auto gauche = inst_bin->valeur_gauche;
            auto droite = inst_bin->valeur_droite;
            auto valeur_gauche = génère_code_pour_atome(gauche, false);
            auto valeur_droite = génère_code_pour_atome(droite, false);

            llvm::Value *valeur = nullptr;

            if (inst_bin->op >= OpérateurBinaire::Genre::Comp_Egal &&
                inst_bin->op <= OpérateurBinaire::Genre::Comp_Sup_Egal_Nat) {
                auto cmp = cmp_llvm_depuis_opérateur(inst_bin->op);

                assert_rappel(valeur_gauche->getType() == valeur_droite->getType(), [&]() {
                    dbg() << erreur::imprime_site(m_espace, inst_bin->site) << '\n'
                          << "Type à gauche LLVM " << *valeur_gauche->getType() << '\n'
                          << "Type à droite LLVM " << *valeur_droite->getType() << '\n'
                          << "Type à gauche " << chaine_type(inst_bin->valeur_gauche->type) << '\n'
                          << "Type à droite " << chaine_type(inst_bin->valeur_droite->type) << '\n'
                          << imprime_instruction(inst_bin);
                });

                valeur = m_builder.CreateICmp(cmp, valeur_gauche, valeur_droite);
            }
            else if (inst_bin->op >= OpérateurBinaire::Genre::Comp_Egal_Reel &&
                     inst_bin->op <= OpérateurBinaire::Genre::Comp_Sup_Egal_Reel) {
                auto cmp = cmp_llvm_depuis_opérateur(inst_bin->op);
                valeur = m_builder.CreateFCmp(cmp, valeur_gauche, valeur_droite);
            }
            else {
                auto inst_llvm = inst_llvm_depuis_opérateur(inst_bin->op);
                valeur = m_builder.CreateBinOp(inst_llvm, valeur_gauche, valeur_droite);
            }

            définis_valeur_instruction(inst, valeur);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                auto valeur_retour = génère_code_pour_atome(atome, false);
                m_builder.CreateRet(valeur_retour);
            }
            else {
                m_builder.CreateRet(nullptr);
            }
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto inst_accès = inst->comme_acces_index();
            auto valeur_accédée = génère_code_pour_atome(inst_accès->accédé, false);
            auto valeur_index = génère_code_pour_atome(inst_accès->index, false);

            llvm::SmallVector<llvm::Value *, 2> liste_index;

            auto accédé = inst_accès->accédé;
            if (accédé->est_instruction()) {
                // dbg() << accédé->comme_instruction()->genre << " " << *valeur_accede->getType();

                auto type_accédé = inst_accès->donne_type_accédé();
                if (type_accédé->est_type_pointeur()) {
                    /* L'accédé est le pointeur vers le pointeur, donc déréférence-le. */
                    valeur_accédée = m_builder.CreateLoad(
                        valeur_accédée->getType()->getPointerElementType(), valeur_accédée);
                }
                else {
                    /* Tableau fixe ou autre ; accède d'abord l'adresse de base. */
                    liste_index.push_back(m_builder.getInt32(0));
                }
            }
            else {
                // dbg() << inst_acces->accédé->genre_atome << '\n';
                /* Tableau fixe ou autre ; accède d'abord l'adresse de base. */
                liste_index.push_back(m_builder.getInt32(0));
            }

            liste_index.push_back(valeur_index);

            auto valeur = m_builder.CreateInBoundsGEP(valeur_accédée, liste_index);
            // dbg() << "--> " << *valeur->getType();
            définis_valeur_instruction(inst, valeur);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto inst_accès = inst->comme_acces_membre();

            auto accédé = inst_accès->accédé;
            auto valeur_accédée = génère_code_pour_atome(accédé, false);

            auto index_membre = uint32_t(inst_accès->index);

            llvm::SmallVector<llvm::Value *, 2> liste_index;

            if (inst_accès->accédé->est_instruction()) {
                auto inst_accédée = inst_accès->accédé->comme_instruction();
                auto type_accédé = inst_accès->donne_type_accédé();

                if (inst_accédée->est_alloc()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (inst_accédée->est_charge()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (inst_accédée->est_acces_membre()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (type_accédé->est_type_pointeur()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (inst_accédée->est_appel() && (inst_accédée->type->est_type_pointeur() ||
                                                       inst_accédée->type->est_type_reference())) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (inst_accédée->est_acces_index()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
                else if (inst_accédée->est_transtype()) {
                    liste_index.push_back(m_builder.getInt32(0));
                }
            }
            else if (inst_accès->accédé->est_globale()) {
                liste_index.push_back(m_builder.getInt32(0));
            }
            else {
                assert(false);
            }

            liste_index.push_back(m_builder.getInt32(index_membre));

            auto résultat = m_builder.CreateInBoundsGEP(valeur_accédée, liste_index);

            définis_valeur_instruction(inst, résultat);
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto const inst_transtype = inst->comme_transtype();
            auto const valeur = génère_code_pour_atome(inst_transtype->valeur, false);
            auto const type_de = inst_transtype->valeur->type;
            auto const type_vers = inst_transtype->type;
            auto const type_llvm = convertis_type_llvm(type_vers);
            auto const cast_op = convertis_type_transtypage(
                inst_transtype->op, type_de, type_vers);
            auto const résultat = m_builder.CreateCast(cast_op, valeur, type_llvm);
            définis_valeur_instruction(inst, résultat);
            break;
        }
    }
}

template <typename T>
static std::vector<T> donne_tableau_typé(const AtomeConstanteDonnéesConstantes *constante,
                                         int taille_données)
{
    auto const données = constante->donne_données();
    auto pointeur_données = reinterpret_cast<T const *>(données.begin());

    std::vector<T> résultat;
    résultat.resize(size_t(taille_données));

    for (int i = 0; i < taille_données; i++) {
        résultat[size_t(i)] = *pointeur_données++;
    }

    return résultat;
}

llvm::Value *GénératriceCodeLLVM::génère_valeur_données_constantes(
    const AtomeConstanteDonnéesConstantes *constante)
{
    auto const type_tableau = constante->type->comme_type_tableau_fixe();
    auto const taille_tableau = type_tableau->taille;
    auto const type_élément = type_tableau->type_pointe;

    if (type_élément->est_type_entier_relatif() || type_élément->est_type_entier_constant()) {
        if (type_élément->taille_octet == 1) {
            auto données = donne_tableau_typé<int8_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        if (type_élément->taille_octet == 2) {
            auto données = donne_tableau_typé<int16_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        if (type_élément->taille_octet == 4 || type_élément->est_type_entier_constant()) {
            auto données = donne_tableau_typé<int32_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        auto données = donne_tableau_typé<int64_t>(constante, taille_tableau);
        return llvm::ConstantDataArray::get(m_contexte_llvm, données);
    }

    if (type_élément->est_type_entier_naturel()) {
        if (type_élément->taille_octet == 1) {
            auto données = donne_tableau_typé<uint8_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        if (type_élément->taille_octet == 2) {
            auto données = donne_tableau_typé<uint16_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        if (type_élément->taille_octet == 4 || type_élément->est_type_entier_constant()) {
            auto données = donne_tableau_typé<uint32_t>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        auto données = donne_tableau_typé<uint64_t>(constante, taille_tableau);
        return llvm::ConstantDataArray::get(m_contexte_llvm, données);
    }

    if (type_élément->est_type_reel()) {
        if (type_élément->taille_octet == 2) {
            assert_rappel(false, []() { dbg() << "Type r16 dans les données constantes."; });
        }

        if (type_élément->taille_octet == 4) {
            auto données = donne_tableau_typé<float>(constante, taille_tableau);
            return llvm::ConstantDataArray::get(m_contexte_llvm, données);
        }

        auto données = donne_tableau_typé<double>(constante, taille_tableau);
        return llvm::ConstantDataArray::get(m_contexte_llvm, données);
    }

    assert_rappel(false, [&]() {
        dbg() << "Type non pris en charge dans les données constantes : "
              << chaine_type(type_élément);
    });
    return nullptr;
}

void GénératriceCodeLLVM::définis_valeur_instruction(Instruction const *inst, llvm::Value *valeur)
{
    assert_rappel(!adresse_est_nulle(valeur),
                  [&]() { dbg() << erreur::imprime_site(m_espace, inst->site); });

    assert_rappel(valeur->getType() == convertis_type_llvm(inst->type), [&]() {
        dbg() << "Le type de l'instruction est " << chaine_type(inst->type) << '\n'
              << "Le type LLVM est " << *valeur->getType() << '\n'
              << "Le type espéré serait " << *convertis_type_llvm(inst->type) << '\n'
              << imprime_arbre_instruction(inst) << '\n'
              << imprime_commentaire_instruction(inst) << '\n'
              << erreur::imprime_site(m_espace, inst->site);
    });

    table_valeurs[inst->numero] = valeur;
}

llvm::Function *GénératriceCodeLLVM::donne_ou_crée_déclaration_fonction(
    const AtomeFonction *fonction)
{
    auto nom = vers_string_ref(fonction->nom);

    if (fonction->decl && fonction->decl->ident == ID::__principale) {
        nom = "principale";
    }

    auto existante = m_module->getFunction(nom);
    if (existante) {
        return existante;
    }

    auto type_fonction = fonction->type->comme_type_fonction();
    auto type_llvm = convertis_type_fonction(type_fonction);
    // auto liaison = donne_liaison_fonction(fonction);
    auto liaison = llvm::GlobalValue::ExternalLinkage;

    return llvm::Function::Create(type_llvm, liaison, nom, m_module);
}

llvm::GlobalVariable *GénératriceCodeLLVM::donne_ou_crée_déclaration_globale(
    const AtomeGlobale *globale)
{
    auto existante = table_globales.valeur_ou(globale, nullptr);
    if (existante) {
        return existante;
    }

    auto type = globale->donne_type_alloué();
    auto type_llvm = convertis_type_llvm(type);
    auto nom_globale = vers_string_ref(globale->ident);
    auto liaison = globale->est_externe ? llvm::GlobalValue::ExternalLinkage :
                                          llvm::GlobalValue::InternalLinkage;
    auto résultat = new llvm::GlobalVariable(
        *m_module, type_llvm, globale->est_constante, liaison, nullptr, nom_globale);

    résultat->setAlignment(llvm::Align(type->alignement));
    table_globales.insère(globale, résultat);
    return résultat;
}

void GénératriceCodeLLVM::génère_code()
{
    if (données_module.données_constantes) {
        auto données_constantes = données_module.données_constantes;

        POUR (données_constantes->tableaux_constants) {
            auto valeur_globale = it.globale;
            auto valeur_initialisateur = static_cast<llvm::Constant *>(
                génère_code_pour_atome(valeur_globale->initialisateur, true));

            auto globale = donne_ou_crée_déclaration_globale(valeur_globale);
            globale->setInitializer(valeur_initialisateur);
        }
    }

    POUR (données_module.globales) {
        // Logueuse::réinitialise_indentation();
        // dbg() << "Génère code pour globale (" << it << ") " << it->ident << ' '
        //       << chaine_type(it->type);
        auto valeur_globale = it;

        auto globale = donne_ou_crée_déclaration_globale(valeur_globale);
        if (valeur_globale->initialisateur) {
            auto valeur_initialisateur = static_cast<llvm::Constant *>(
                génère_code_pour_atome(valeur_globale->initialisateur, true));
            globale->setInitializer(valeur_initialisateur);
        }
        else {
            globale->setInitializer(llvm::ConstantAggregateZero::get(globale->getType()));
        }
    }

    POUR (données_module.fonctions) {
        m_nombre_fonctions_compilées++;
        // dbg() << "[" << m_nombre_fonctions_compilées << " / " <<
        // données_module.globales.taille()
        //       << "] :\n"
        //       << imprime_fonction(it);
        génère_code_pour_fonction(it);
    }
}

void GénératriceCodeLLVM::génère_code_pour_fonction(AtomeFonction const *atome_fonc)
{
    if (atome_fonc->est_externe) {
        return;
    }

    auto fonction = donne_ou_crée_déclaration_fonction(atome_fonc);

    m_fonction_courante = fonction;

    table_valeurs.redimensionne(atome_fonc->numérote_instructions());
    table_blocs.efface();

#ifndef NDEBUG
    POUR (table_valeurs) {
        it = nullptr;
    }
#endif

    table_blocs.efface();

    /* génère d'abord tous les blocs depuis les labels */
    for (auto inst : atome_fonc->instructions) {
        if (inst->genre != GenreInstruction::LABEL) {
            continue;
        }

        génère_code_pour_instruction(inst);
    }

    auto bloc_entree = table_blocs[atome_fonc->instructions[0]->comme_label()->id];
    m_builder.SetInsertPoint(bloc_entree);

    auto valeurs_args = fonction->arg_begin();

    for (auto &param : atome_fonc->params_entrée) {
        auto valeur = &(*valeurs_args++);
        valeur->setName(vers_string_ref(param->ident));

        auto alloc = crée_allocation(param);

        auto store = m_builder.CreateStore(valeur, alloc);
        store->setAlignment(alloc->getAlign());

        définis_valeur_instruction(param, alloc);
    }

    /* crée une variable local pour la valeur de sortie */
    auto type_fonction = atome_fonc->type->comme_type_fonction();
    if (!type_fonction->type_sortie->est_type_rien()) {
        auto param = atome_fonc->param_sortie;
        crée_allocation(param);
    }

    /* Génère le code pour les accès de membres des retours multiples. */
    if (atome_fonc->decl && atome_fonc->decl->params_sorties.taille() > 1) {
        for (auto &param : atome_fonc->decl->params_sorties) {
            génère_code_pour_instruction(
                param->comme_declaration_variable()->atome->comme_instruction());
        }
    }

    for (auto inst : atome_fonc->instructions) {
        // dbg() << imprime_instruction(inst);

        if (inst->genre == GenreInstruction::LABEL) {
            auto bloc = table_blocs[inst->comme_label()->id];
            m_builder.SetInsertPoint(bloc);
            continue;
        }

        génère_code_pour_instruction(inst);
    }

    if (manager_fonctions) {
        manager_fonctions->run(*m_fonction_courante);
    }

    m_fonction_courante = nullptr;
}

llvm::AllocaInst *GénératriceCodeLLVM::crée_allocation(const InstructionAllocation *alloc)
{
    auto type_alloué = alloc->donne_type_alloué();
    if (type_alloué->est_type_entier_constant()) {
        type_alloué = TypeBase::Z32;
    }
    assert_rappel(type_alloué->alignement, [&]() { dbg() << chaine_type(type_alloué); });

    auto type_llvm = convertis_type_llvm(type_alloué);
    auto alloca = m_builder.CreateAlloca(type_llvm, 0u);
    alloca->setAlignment(llvm::Align(type_alloué->alignement));
    alloca->setName(vers_string_ref(alloc->ident));

    définis_valeur_instruction(alloc, alloca);
    return alloca;
}

bool initialise_llvm()
{
    if (llvm::InitializeNativeTarget() || llvm::InitializeNativeTargetAsmParser() ||
        llvm::InitializeNativeTargetAsmPrinter()) {
        dbg() << "Ne peut pas initialiser LLVM !";
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

/* Chemin du fichier objet généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_objet_llvm(int index)
{
    auto nom_de_base = enchaine("kuri", index);
    return chemin_fichier_objet_temporaire_pour(nom_de_base);
}

#ifndef NDEBUG
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

static kuri::chaine_statique donne_assembleur_llvm()
{
    return "llvm-as-12";
}

static std::optional<ErreurCommandeExterne> valide_llvm_ir(llvm::Module &module)
{
    auto const fichier_ll = chemin_fichier_ll_llvm();
    auto const fichier_bc = chemin_fichier_bc_llvm();

    std::error_code ec;
    llvm::raw_fd_ostream dest(vers_string_ref(fichier_ll), ec, llvm::sys::fs::F_None);
    module.print(dest, nullptr);

    /* Génère le fichier de code binaire depuis le fichier de RI LLVM, ce qui vérifiera que la RI
     * est correcte. */
    auto commande = enchaine(donne_assembleur_llvm(), " ", fichier_ll, " -o ", fichier_bc, '\0');
    return exécute_commande_externe_erreur(commande);
}
#endif

CoulisseLLVM::~CoulisseLLVM()
{
    POUR (m_modules) {
        delete it->module;
        delete it->contexte_llvm;
        memoire::deloge("DonnéesModule", it);
    }
    delete m_machine_cible;
}

std::optional<ErreurCoulisse> CoulisseLLVM::génère_code_impl(const ArgsGénérationCode &args)
{
    if (!initialise_llvm()) {
        return ErreurCoulisse{"Impossible d'intialiser LLVM."};
    }

    auto &compilatrice_ri = *args.compilatrice_ri;
    auto &espace = *args.espace;
    auto const &programme = *args.programme;

    auto const triplet_cible = llvm::sys::getDefaultTargetTriple();

    auto erreur = std::string{""};
    auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

    if (!cible) {
        auto message_erreur = enchaine("Erreur lors la recherche de la cible selon le triplet '",
                                       triplet_cible,
                                       "'.\n",
                                       "LLVM dis '",
                                       erreur,
                                       "'.");

        return ErreurCoulisse{message_erreur};
    }

    auto repr_inter = représentation_intermédiaire_programme(espace, compilatrice_ri, programme);
    if (!repr_inter.has_value()) {
        return ErreurCoulisse{"Impossible d'obtenir la représentation intermédiaire du programme"};
    }

    auto CPU = "generic";
    auto feature = "";
    auto options_cible = llvm::TargetOptions{};
    auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    m_machine_cible = cible->createTargetMachine(triplet_cible, CPU, feature, options_cible, RM);

    crée_modules(*repr_inter, triplet_cible);

    POUR (m_modules) {
        auto generatrice = GénératriceCodeLLVM(espace, *it);
        generatrice.génère_code();
    }

#ifndef NDEBUG
    POUR (m_modules) {
        auto opt_erreur_validation = valide_llvm_ir(*it->module);
        if (opt_erreur_validation.has_value()) {
            auto erreur_validation = opt_erreur_validation.value();
            auto message_erreur = enchaine("Erreur lors de la validation du code LLVM.\n",
                                           "La commande a retourné :\n\n",
                                           erreur_validation.message);
            return ErreurCoulisse{message_erreur};
        }
    }
#endif

    m_bibliothèques = repr_inter->donne_bibliothèques_utilisées();

    return {};
}

std::optional<ErreurCoulisse> CoulisseLLVM::crée_fichier_objet_impl(
    const ArgsCréationFichiersObjets & /*args*/)
{
#ifndef NDEBUG
    auto poule_de_tâches = kuri::PouleDeTâchesEnSérie{};
#else
#    if 0
    auto poule_de_tâches = kuri::PouleDeTâchesMoultFils{};
#    else
    auto poule_de_tâches = kuri::PouleDeTâchesSousProcessus{};
#    endif
#endif

    POUR (m_modules) {
        poule_de_tâches.ajoute_tâche([&]() { crée_fichier_objet(it); });
    }

    poule_de_tâches.attends_sur_tâches();

    POUR (m_modules) {
        if (it->erreur_fichier_objet.taille() == 0) {
            continue;
        }

        return ErreurCoulisse{it->erreur_fichier_objet};
    }

    return {};
}

static kuri::chaine_statique donne_fichier_point_d_entree(OptionsDeCompilation const &options)
{
    if (options.résultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        return "fichiers/point_d_entree_dynamique.c";
    }

    return "fichiers/point_d_entree.c";
}

std::optional<ErreurCoulisse> CoulisseLLVM::crée_exécutable_impl(const ArgsLiaisonObjets &args)
{
    auto &compilatrice = *args.compilatrice;
    auto &espace = *args.espace;

    kuri::tablet<kuri::chaine_statique, 16> fichiers_objet;
    auto fichier_point_d_entrée_c = compilatrice.racine_kuri /
                                    donne_fichier_point_d_entree(espace.options);
    fichiers_objet.ajoute(fichier_point_d_entrée_c);

    POUR (m_modules) {
        fichiers_objet.ajoute(it->chemin_fichier_objet);
    }

    auto commande = commande_pour_liaison(espace.options, fichiers_objet, m_bibliothèques);

    if (!exécute_commande_externe(commande)) {
        return ErreurCoulisse{"Ne peut pas créer l'executable !"};
    }

    return {};
}

void CoulisseLLVM::crée_fichier_objet(DonnéesModule *module)
{
    std::error_code ec;
    llvm::raw_fd_ostream dest(
        vers_string_ref(module->chemin_fichier_objet), ec, llvm::sys::fs::F_None);

    if (ec) {
        module->erreur_fichier_objet = enchaine(
            "Ne peut pas ouvrir le fichier '", module->chemin_fichier_objet, "'");
        return;
    }

    llvm::legacy::PassManager pass;
    auto type_fichier = llvm::CGFT_ObjectFile;

    if (m_machine_cible->addPassesToEmitFile(pass, dest, nullptr, type_fichier)) {
        module->erreur_fichier_objet = "La machine cible ne peut pas émettre ce type de fichier";
        return;
    }

    pass.run(*module->module);
    dest.flush();
    if (!kuri::chemin_systeme::existe(module->chemin_fichier_objet)) {
        module->erreur_fichier_objet = enchaine(
            "Le fichier '", module->chemin_fichier_objet, "' ne fut pas écrit.");
        return;
    }
}

void CoulisseLLVM::crée_modules(const ProgrammeRepreInter &repr_inter,
                                const std::string &triplet_cible)
{
    /* Crée un module pour les globales. */
    auto module = crée_un_module("Globales", triplet_cible);

    auto opt_données_constantes = repr_inter.donne_données_constantes();
    if (opt_données_constantes.has_value()) {
        module->données_constantes = opt_données_constantes.value();
    }
    module->globales = repr_inter.donne_globales();
    module->fonctions = repr_inter.donne_fonctions();

    /* À FAIRE : pour la compilation en plusieurs fichiers il faudra proprement
     * gérer les liaisons des globales, ainsi que leurs données des noms uniques. */
#if 0
    /* Crée des modules pour les fonctions. */
    constexpr int nombre_instructions_par_module = 10000;
    int nombre_instructions = 0;
    int index_première_fonction = 0;
    auto fonctions = repr_inter.donne_fonctions();
    for (int i = 0; i < fonctions.taille(); i++) {
        auto fonction = fonctions[i];
        nombre_instructions += fonction->nombre_d_instructions_avec_entrées_sorties();

        if (nombre_instructions < nombre_instructions_par_module && i != fonctions.taille() - 1) {
            continue;
        }

        auto pointeur_fonction = fonctions.begin() + index_première_fonction;
        auto taille = i - index_première_fonction + 1;

        auto fonctions_du_modules = kuri::tableau_statique<AtomeFonction *>(pointeur_fonction,
                                                                            taille);

        module = crée_un_module("Fonction", triplet_cible);
        module->fonctions = fonctions_du_modules;

        nombre_instructions = 0;
        index_première_fonction = i + 1;
    }
#endif
}

DonnéesModule *CoulisseLLVM::crée_un_module(kuri::chaine_statique nom,
                                            const std::string &triplet_cible)
{
    auto résultat = memoire::loge<DonnéesModule>("DonnéesModule");
    résultat->contexte_llvm = new llvm::LLVMContext;

    auto nom_module = enchaine(nom, m_modules.taille());

    résultat->module = new llvm::Module(vers_string_ref(nom_module), *résultat->contexte_llvm);
    résultat->module->setDataLayout(m_machine_cible->createDataLayout());
    résultat->module->setTargetTriple(triplet_cible);

    résultat->chemin_fichier_objet = chemin_fichier_objet_llvm(int32_t(m_modules.taille()));

    m_modules.ajoute(résultat);

    return résultat;
}

int64_t CoulisseLLVM::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += m_bibliothèques.taille();
    résultat += m_modules.taille();

    POUR (m_modules) {
        résultat += taille_de(llvm::LLVMContext);
        résultat += taille_de(llvm::Module);
        résultat += it->chemin_fichier_objet.taille();
        résultat += it->erreur_fichier_objet.taille();
    }

    return résultat;
}
