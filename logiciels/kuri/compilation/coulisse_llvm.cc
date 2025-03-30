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
#include <llvm/MC/TargetRegistry.h>
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
#include "intrinseques.hh"
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

#undef COMPILE_EN_PLUSIEURS_MODULE

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

static llvm::GlobalValue::LinkageTypes donne_liaison_fonction(DonnéesModule const &module,
                                                              AtomeFonction const *fonction)
{
    if (fonction->est_externe) {
        return llvm::GlobalValue::ExternalLinkage;
    }

    /* Ces fonctions sont appelées depuis les fichiers de point d'entrées C. */
    if (dls::outils::est_element(fonction->decl->ident,
                                 ID::__point_d_entree_dynamique,
                                 ID::__point_d_entree_systeme,
                                 ID::__point_de_sortie_dynamique)) {
        return llvm::GlobalValue::ExternalLinkage;
    }

#ifndef COMPILE_EN_PLUSIEURS_MODULE
    static_cast<void>(module);
    return llvm::GlobalValue::InternalLinkage;
#else
    /* La fonction ne fait pas partie du module. Nous avons une définition ailleurs. */
    if (!module.fait_partie_du_module(fonction)) {
        return llvm::GlobalValue::AvailableExternallyLinkage;
    }

    return llvm::GlobalValue::InternalLinkage;
#endif
}

static llvm::GlobalValue::LinkageTypes donne_liaison_globale(DonnéesModule const &module,
                                                             AtomeGlobale const *globale)
{
    if (globale->est_externe) {
        return llvm::GlobalValue::ExternalLinkage;
    }

#ifndef COMPILE_EN_PLUSIEURS_MODULE
    static_cast<void>(module);
    return llvm::GlobalValue::InternalLinkage;
#else
    /* La globale ne fait pas partie du module. Nous avons une définition ailleurs. */
    if (!module.fait_partie_du_module(globale)) {
        return llvm::GlobalValue::AvailableExternallyLinkage;
    }

    return llvm::GlobalValue::InternalLinkage;
#endif
}

/* Retourne vrai si la valeur globale doit être considérer comme locale pour l'exécutable ou la
 * bibliothèque.
 * Logique partiellement reprise de Clang :
 * https://clang.llvm.org/doxygen/CodeGenModule_8cpp_source.html */
static bool doit_être_considérée_dso_local(llvm::GlobalValue *valeur)
{
    if (valeur->hasLocalLinkage()) {
        return true;
    }

    if (!valeur->hasDefaultVisibility() && !valeur->hasExternalWeakLinkage()) {
        return true;
    }

    /* DLLImport marque explicitement la globale comme étant externe. */
    if (valeur->hasDLLImportStorageClass()) {
        return false;
    }

    /* Une définition ne peut être preemptée d'un exécutable. */
    if (!valeur->isDeclarationForLinker()) {
        return true;
    }

    return false;
}

static void définis_dllimport_dllexport(llvm::GlobalValue *valeur, VisibilitéSymbole visibilité)
{
    if (visibilité == VisibilitéSymbole::EXPORTÉ) {
        valeur->setDLLStorageClass(llvm::GlobalValue::DLLExportStorageClass);
    }
}

static void définis_visibilité(llvm::GlobalValue *valeur, VisibilitéSymbole visibilité)
{
    if (valeur->hasLocalLinkage()) {
        valeur->setVisibility(llvm::GlobalValue::DefaultVisibility);
        return;
    }

    if (visibilité == VisibilitéSymbole::INTERNE) {
        valeur->setVisibility(llvm::GlobalValue::HiddenVisibility);
    }
}

static void définis_les_propriétés_globales(llvm::GlobalValue *valeur,
                                            AtomeFonction const *fonction)
{
    définis_dllimport_dllexport(valeur, donne_visibilité_fonction(fonction));
    définis_visibilité(valeur, donne_visibilité_fonction(fonction));
    valeur->setDSOLocal(doit_être_considérée_dso_local(valeur));
}

static void définis_les_propriétés_globales(llvm::GlobalValue *valeur, AtomeGlobale const *globale)
{
    définis_dllimport_dllexport(valeur, globale->donne_visibilité_symbole());
    définis_visibilité(valeur, globale->donne_visibilité_symbole());
    valeur->setDSOLocal(doit_être_considérée_dso_local(valeur));
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

/* ------------------------------------------------------------------------- */
/** \name DonnéesModule
 * \{ */

void DonnéesModule::définis_données_constantes(const DonnéesConstantes *données_constantes)
{
    m_données_constantes = données_constantes;
    ajourne_globales();
}

const DonnéesConstantes *DonnéesModule::donne_données_constantes() const
{
    return m_données_constantes;
}

void DonnéesModule::définis_fonctions(kuri::tableau_statique<AtomeFonction *> fonctions)
{
    m_fonctions = fonctions;
    m_ensemble_fonctions.efface();
    POUR (m_fonctions) {
        m_ensemble_fonctions.insère(it);
    }
}

kuri::tableau_statique<AtomeFonction *> DonnéesModule::donne_fonctions() const
{
    return m_fonctions;
}

bool DonnéesModule::fait_partie_du_module(AtomeFonction const *fonction) const
{
    return m_ensemble_fonctions.possède(fonction);
}

void DonnéesModule::définis_globales(kuri::tableau_statique<AtomeGlobale *> globales)
{
    m_globales = globales;
    ajourne_globales();
}

kuri::tableau_statique<AtomeGlobale *> DonnéesModule::donne_globales() const
{
    return m_globales;
}

bool DonnéesModule::fait_partie_du_module(AtomeGlobale const *globale) const
{
    return m_ensemble_globales.possède(globale);
}

void DonnéesModule::ajourne_globales()
{
    m_ensemble_globales.efface();
    POUR (m_globales) {
        m_ensemble_globales.insère(it);
    }
    if (m_données_constantes) {
        POUR (m_données_constantes->tableaux_constants) {
            m_ensemble_globales.insère(it.globale);
        }
    }
}

/** \} */

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

    void génère_code_pour_appel(InstructionAppel const *inst_appel);

    void génère_code_pour_appel_intrinsèque(InstructionAppel const *inst_appel,
                                            DonnéesSymboleExterne const *données_externe);

    void génère_code_pour_fonction(const AtomeFonction *atome_fonc);

    void génère_code_pour_constructeur_global(const AtomeFonction *atome_fonc,
                                              kuri::chaine_statique nom_globale);

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
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            /* Convertis vers void(*)(), comme en C. */
            auto type_sortie_llvm = llvm::Type::getVoidTy(m_contexte_llvm);
            auto type_fonction = llvm::FunctionType::get(type_sortie_llvm, false);
            return type_fonction->getPointerTo();
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
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            type_llvm = llvm::Type::getInt64Ty(m_contexte_llvm);
            break;
        }
        case GenreNoeud::RÉEL:
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
        case GenreNoeud::RÉFÉRENCE:
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
        case GenreNoeud::DÉCLARATION_UNION:
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
        case GenreNoeud::DÉCLARATION_STRUCTURE:
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
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            type_llvm = convertis_type_llvm(type_enum->type_sous_jacent);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            type_llvm = convertis_type_llvm(type_opaque->type_opacifié);
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
    paramètres.reserve(static_cast<size_t>(type->types_entrées.taille()));

    auto est_variadique = false;

    POUR (type->types_entrées) {
        if (it->est_type_variadique()) {
            auto type_variadique = it->comme_type_variadique();
            /* Type variadique externe. */
            if (type_variadique->type_pointé == nullptr) {
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
            auto type = structure->type->comme_type_composé();
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
            génère_code_pour_appel(inst->comme_appel());
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
                case OpérateurUnaire::Genre::Négation:
                {
                    if (inst_un->type->est_type_réel()) {
                        valeur = m_builder.CreateFNeg(valeur);
                    }
                    else {
                        valeur = m_builder.CreateNeg(valeur);
                    }
                    break;
                }
                case OpérateurUnaire::Genre::Négation_Binaire:
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
                                                       inst_accédée->type->est_type_référence())) {
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
        case GenreInstruction::INATTEIGNABLE:
        {
            m_builder.CreateUnreachable();
            break;
        }
        case GenreInstruction::SÉLECTION:
        {
            auto sélection = inst->comme_sélection();
            auto const condition = génère_code_pour_atome(sélection->condition, false);
            auto const si_vrai = génère_code_pour_atome(sélection->si_vrai, false);
            auto const si_faux = génère_code_pour_atome(sélection->si_faux, false);
            auto const résultat = m_builder.CreateSelect(condition, si_vrai, si_faux);
            définis_valeur_instruction(inst, résultat);
            break;
        }
    }
}

void GénératriceCodeLLVM::génère_code_pour_appel(InstructionAppel const *inst_appel)
{
    if (auto données_externe = inst_appel->est_appel_intrinsèque()) {
        génère_code_pour_appel_intrinsèque(inst_appel, données_externe);
        return;
    }

    auto arguments = std::vector<llvm::Value *>();
    POUR (inst_appel->args) {
        arguments.push_back(génère_code_pour_atome(it, false));
    }

    auto valeur_fonction = génère_code_pour_atome(inst_appel->appelé, false);
    assert_rappel(!adresse_est_nulle(valeur_fonction), [&]() {
        dbg() << erreur::imprime_site(m_espace, inst_appel->site) << '\n'
              << imprime_atome(inst_appel->appelé);
    });

    auto type_fonction = convertis_type_llvm(inst_appel->appelé->type)->getPointerElementType();

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

    définis_valeur_instruction(inst_appel, résultat);
}

llvm::AtomicOrdering donne_valeur_pour_ordre_mémoire(Atome const *arg)
{
    if (arg->est_constante_entière()) {
        auto valeur = OrdreMémoire(arg->comme_constante_entière()->valeur);

        switch (valeur) {
            default:
            {
                return llvm::AtomicOrdering::SequentiallyConsistent;
            }
            case OrdreMémoire::RELAXÉ:
            {
                return llvm::AtomicOrdering::Monotonic;
            }
            case OrdreMémoire::CONSOMME:
            {
                /* LLVM n'a pas de "consomme", utilise donc la contrainte la plus forte. */
                return llvm::AtomicOrdering::SequentiallyConsistent;
            }
            case OrdreMémoire::ACQUIÈRE:
            {
                return llvm::AtomicOrdering::Acquire;
            }
            case OrdreMémoire::RELÂCHE:
            {
                return llvm::AtomicOrdering::Release;
            }
            case OrdreMémoire::ACQUIÈRE_RELÂCHE:
            {
                return llvm::AtomicOrdering::AcquireRelease;
            }
            case OrdreMémoire::SEQ_CST:
            {
                return llvm::AtomicOrdering::SequentiallyConsistent;
            }
        }
    }

    return llvm::AtomicOrdering::SequentiallyConsistent;
}

void GénératriceCodeLLVM::génère_code_pour_appel_intrinsèque(
    InstructionAppel const *inst_appel, DonnéesSymboleExterne const *données_externe)
{
    auto opt_genre_intrinsèque = donne_genre_intrinsèque_pour_identifiant(
        données_externe->ident_énum_intrinsèque);
    assert(opt_genre_intrinsèque.has_value());
    auto genre_intrinsèque = opt_genre_intrinsèque.value();

    llvm::Value *valeur_retour = nullptr;

    switch (genre_intrinsèque) {
        case GenreIntrinsèque::RÉINITIALISE_TAMPON_INSTRUCTION:
        {
            auto arg1 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg2 = génère_code_pour_atome(inst_appel->args[1], false);
            m_builder.CreateBinaryIntrinsic(llvm::Intrinsic::clear_cache, arg1, arg2);
            break;
        }
        case GenreIntrinsèque::PRÉCHARGE:
        {
            llvm::SmallVector<llvm::Type *, 4> types;
            llvm::SmallVector<llvm::Value *, 4> args;

            POUR (inst_appel->args) {
                auto arg = génère_code_pour_atome(it, false);
                types.push_back(arg->getType());
                args.push_back(arg);
            }

            /* LLVM a un dernier argument pour le type de cache : instruction (0) ou données (1).
             * Par défaut nous suivons la signature de GCC, et utilisons le cache de données. */
            auto type_i32 = llvm::Type::getInt32Ty(m_contexte_llvm);
            auto type_cache = llvm::ConstantInt::get(type_i32, 1);

            types.push_back(type_i32);
            args.push_back(type_cache);

            m_builder.CreateIntrinsic(llvm::Intrinsic::prefetch, types, args);
            break;
        }
        case GenreIntrinsèque::PRÉDIT:
        {
            auto arg1 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg2 = génère_code_pour_atome(inst_appel->args[1], false);
            valeur_retour = m_builder.CreateBinaryIntrinsic(llvm::Intrinsic::expect, arg1, arg2);
            break;
        }
        case GenreIntrinsèque::PRÉDIT_AVEC_PROBABILITÉ:
        {
            llvm::SmallVector<llvm::Type *, 3> types;
            llvm::SmallVector<llvm::Value *, 3> args;

            POUR (inst_appel->args) {
                auto arg = génère_code_pour_atome(it, false);
                types.push_back(arg->getType());
                args.push_back(arg);
            }

            valeur_retour = m_builder.CreateIntrinsic(
                llvm::Intrinsic::expect_with_probability, types, args);
            break;
        }
        case GenreIntrinsèque::PIÈGE:
        case GenreIntrinsèque::NONATTEIGNABLE:
        {
            /* LLVM n'a pas d'intrinsèque pour nonatteignable, utilisons "trap". */
            m_builder.CreateIntrinsic(llvm::Intrinsic::trap, {}, {});
            break;
        }
        case GenreIntrinsèque::TROUVE_PREMIER_ACTIF_32:
        case GenreIntrinsèque::TROUVE_PREMIER_ACTIF_64:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            auto zero_est_poison = llvm::ConstantInt::get(llvm::Type::getInt1Ty(m_contexte_llvm),
                                                          0);
            valeur_retour = m_builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::cttz, arg, zero_est_poison);
            valeur_retour = m_builder.CreateAdd(
                valeur_retour, llvm::ConstantInt::get(valeur_retour->getType(), 1));

            auto arg_est_zéro = m_builder.CreateICmpEQ(arg,
                                                       llvm::ConstantInt::get(arg->getType(), 0));
            valeur_retour = m_builder.CreateSelect(arg_est_zéro, arg, valeur_retour);
            break;
        }
        case GenreIntrinsèque::COMPTE_ZÉROS_EN_TÊTE_32:
        case GenreIntrinsèque::COMPTE_ZÉROS_EN_TÊTE_64:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            auto zero_est_poison = llvm::ConstantInt::get(llvm::Type::getInt1Ty(m_contexte_llvm),
                                                          1);
            valeur_retour = m_builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::ctlz, arg, zero_est_poison);
            break;
        }
        case GenreIntrinsèque::COMPTE_ZÉROS_EN_FIN_32:
        case GenreIntrinsèque::COMPTE_ZÉROS_EN_FIN_64:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            auto zero_est_poison = llvm::ConstantInt::get(llvm::Type::getInt1Ty(m_contexte_llvm),
                                                          1);
            valeur_retour = m_builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::cttz, arg, zero_est_poison);
            break;
        }
        case GenreIntrinsèque::COMPTE_REDONDANCE_BIT_SIGNE_32:
        case GenreIntrinsèque::COMPTE_REDONDANCE_BIT_SIGNE_64:
        {
            /* Inverse les bits puis compte le nombre de zéro en tête. */
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            arg = m_builder.CreateNot(arg);
            auto zero_est_poison = llvm::ConstantInt::get(llvm::Type::getInt1Ty(m_contexte_llvm),
                                                          1);
            valeur_retour = m_builder.CreateBinaryIntrinsic(
                llvm::Intrinsic::ctlz, arg, zero_est_poison);
            break;
        }
        case GenreIntrinsèque::COMPTE_BITS_ACTIFS_32:
        case GenreIntrinsèque::COMPTE_BITS_ACTIFS_64:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            valeur_retour = m_builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, arg);
            break;
        }
        case GenreIntrinsèque::PARITÉ_BITS_32:
        case GenreIntrinsèque::PARITÉ_BITS_64:
        {
            /* La parité est le nombre de bits actif modulo 2. */
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            valeur_retour = m_builder.CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, arg);
            valeur_retour = m_builder.CreateURem(
                valeur_retour, llvm::ConstantInt::get(valeur_retour->getType(), 2));
            break;
        }
        case GenreIntrinsèque::COMMUTE_OCTETS_16:
        case GenreIntrinsèque::COMMUTE_OCTETS_32:
        case GenreIntrinsèque::COMMUTE_OCTETS_64:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            valeur_retour = m_builder.CreateUnaryIntrinsic(llvm::Intrinsic::bswap, arg);
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_BARRIÈRE_FIL:
        {
            auto arg = inst_appel->args[0];
            m_builder.CreateFence(donne_valeur_pour_ordre_mémoire(arg), llvm::SyncScope::System);
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_BARRIÈRE_SIGNAL:
        {
            auto arg = inst_appel->args[0];
            m_builder.CreateFence(donne_valeur_pour_ordre_mémoire(arg),
                                  llvm::SyncScope::SingleThread);
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_TOUJOURS_SANS_VERROU:
        case GenreIntrinsèque::ATOMIQUE_EST_SANS_VERROU:
        {
            /* À FAIRE(LLVM) : __atomic_always_lock_free, __atomic_is_lock_free */
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_CHARGE_BOOL:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_OCTET:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_N8:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_N16:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_N32:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_N64:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_Z8:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_Z16:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_Z32:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_Z64:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_PTR:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_CHARGE_ADRESSE_FONCTION:
        {
            auto arg = génère_code_pour_atome(inst_appel->args[0], false);
            auto load = m_builder.CreateLoad(arg->getType(), arg);
            load->setAtomic(donne_valeur_pour_ordre_mémoire(inst_appel->args[1]));
            valeur_retour = load;
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_STOCKE_BOOL:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_OCTET:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_N8:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_N16:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_N32:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_N64:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_Z8:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_Z16:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_Z32:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_Z64:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_PTR:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_STOCKE_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto store = m_builder.CreateStore(arg1, arg0);
            store->setAtomic(donne_valeur_pour_ordre_mémoire(inst_appel->args[2]));
            valeur_retour = store;
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_BOOL:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_OCTET:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_N8:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_N16:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_N32:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_N64:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_Z8:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_Z16:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_Z32:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_Z64:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_PTR:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_ÉCHANGE_ADRESSE_FONCTION:
        {
            /* À FAIRE(llvm) : __atomic_exchange. */
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_BOOL:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_OCTET:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_N8:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_N16:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_N32:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_N64:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_Z8:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_Z16:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_Z32:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_Z64:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_PTR:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_COMPARE_ÉCHANGE_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = génère_code_pour_atome(inst_appel->args[2], false);
            auto arg3 = inst_appel->args[3];
            auto arg4 = inst_appel->args[4];
            auto arg5 = inst_appel->args[5];
            auto compare_échange = m_builder.CreateAtomicCmpXchg(
                arg0,
                arg1,
                arg2,
                donne_valeur_pour_ordre_mémoire(arg4),
                donne_valeur_pour_ordre_mémoire(arg5));
            compare_échange->setWeak(arg3->comme_constante_booléenne()->valeur);
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_AJT_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::Add, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_SST_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::Sub, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_ET_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::And, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OU_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::Or, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_OUX_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::Xor, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_OCTET:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_N8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_N16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_N32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_N64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_Z8:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_Z16:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_Z32:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_Z64:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_PTR:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_TYPE_DE_DONNÉES:
        case GenreIntrinsèque::ATOMIQUE_DONNE_PUIS_NET_ADRESSE_FONCTION:
        {
            auto arg0 = génère_code_pour_atome(inst_appel->args[0], false);
            auto arg1 = génère_code_pour_atome(inst_appel->args[1], false);
            auto arg2 = inst_appel->args[0];
            valeur_retour = m_builder.CreateAtomicRMW(
                llvm::AtomicRMWInst::Nand, arg0, arg1, donne_valeur_pour_ordre_mémoire(arg2));
            break;
        }
        case GenreIntrinsèque::EST_ADRESSE_DONNÉES_CONSTANTES:
        {
            break;
        }
    }

    if (valeur_retour) {
        définis_valeur_instruction(inst_appel, valeur_retour);
    }
}

template <typename T>
static llvm::ArrayRef<T> donne_tableau_typé(const AtomeConstanteDonnéesConstantes *constante,
                                            int taille_données)
{
    auto const données = constante->donne_données();
    auto pointeur_données = reinterpret_cast<T const *>(données.begin());
    return {pointeur_données, size_t(taille_données)};
}

llvm::Value *GénératriceCodeLLVM::génère_valeur_données_constantes(
    const AtomeConstanteDonnéesConstantes *constante)
{
    auto const type_tableau = constante->type->comme_type_tableau_fixe();
    auto const taille_tableau = type_tableau->taille;
    auto const type_élément = type_tableau->type_pointé;

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

    if (type_élément->est_type_réel()) {
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
    auto liaison = donne_liaison_fonction(données_module, fonction);

    auto résultat = llvm::Function::Create(type_llvm, liaison, nom, m_module);

    définis_les_propriétés_globales(résultat, fonction);

    auto decl = fonction->decl;
    if (decl) {
        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
            résultat->addParamAttr(0, llvm::Attribute::AttrKind::NonNull);
        }

        if (decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_HORSLIGNE)) {
            résultat->addFnAttr(llvm::Attribute::AttrKind::NoInline);
        }
        else if (decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_ENLIGNE)) {
            résultat->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
        }

        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_SANSRETOUR)) {
            résultat->addFnAttr(llvm::Attribute::AttrKind::NoReturn);
        }
    }
    else if (fonction->enligne) {
        résultat->addFnAttr(llvm::Attribute::AttrKind::AlwaysInline);
    }

    return résultat;
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
    auto liaison = donne_liaison_globale(données_module, globale);
    auto résultat = new llvm::GlobalVariable(
        *m_module, type_llvm, globale->est_constante, liaison, nullptr, nom_globale);

    définis_les_propriétés_globales(résultat, globale);

    résultat->setAlignment(llvm::Align(type->alignement));
    table_globales.insère(globale, résultat);
    return résultat;
}

void GénératriceCodeLLVM::génère_code()
{
    if (données_module.donne_données_constantes()) {
        auto données_constantes = données_module.donne_données_constantes();

        POUR (données_constantes->tableaux_constants) {
            auto valeur_globale = it.globale;
            auto valeur_initialisateur = static_cast<llvm::Constant *>(
                génère_code_pour_atome(valeur_globale->initialisateur, true));

            auto globale = donne_ou_crée_déclaration_globale(valeur_globale);
            globale->setInitializer(valeur_initialisateur);
        }
    }

    POUR (données_module.donne_globales()) {
        // Logueuse::réinitialise_indentation();
        // dbg() << "Génère code pour globale (" << it << ") " << it->ident << ' '
        //       << chaine_type(it->type);
        auto valeur_globale = it;

        auto globale = donne_ou_crée_déclaration_globale(valeur_globale);
        if (valeur_globale->est_constante) {
            assert(valeur_globale->initialisateur);
            auto valeur_initialisateur = static_cast<llvm::Constant *>(
                génère_code_pour_atome(valeur_globale->initialisateur, true));
            globale->setInitializer(valeur_initialisateur);
        }
        else {
            globale->setInitializer(llvm::ConstantAggregateZero::get(globale->getType()));
        }
    }

    AtomeFonction *point_d_entrée_dynamique = nullptr;
    AtomeFonction *point_de_sortie_dynamique = nullptr;

    POUR (données_module.donne_fonctions()) {
        m_nombre_fonctions_compilées++;
        // dbg() << "[" << m_nombre_fonctions_compilées << " / " <<
        // données_module.globales.taille()
        //       << "] :\n"
        //       << imprime_fonction(it);
        génère_code_pour_fonction(it);

        if (it->decl) {
            if (it->decl->ident == ID::__point_d_entree_dynamique) {
                point_d_entrée_dynamique = it;
            }
            else if (it->decl->ident == ID::__point_de_sortie_dynamique) {
                point_de_sortie_dynamique = it;
            }
        }
    }

    génère_code_pour_constructeur_global(point_d_entrée_dynamique, "llvm.global_ctors");
    génère_code_pour_constructeur_global(point_de_sortie_dynamique, "llvm.global_dtors");
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
                param->comme_déclaration_variable()->atome->comme_instruction());
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

void GénératriceCodeLLVM::génère_code_pour_constructeur_global(const AtomeFonction *atome_fonc,
                                                               kuri::chaine_statique nom_globale)
{
    if (!atome_fonc) {
        return;
    }

    auto espace_adressage = m_module->getDataLayout().getProgramAddressSpace();
    auto type_void = llvm::Type::getVoidTy(m_contexte_llvm);
    auto type_i8 = llvm::Type::getInt8Ty(m_contexte_llvm);
    auto type_void_ptr = type_i8->getPointerTo(espace_adressage);
    auto type_int32 = llvm::Type::getInt32Ty(m_contexte_llvm);

    /* Le type de la fonction de constrution est void()*. */
    llvm::FunctionType *CtorFTy = llvm::FunctionType::get(type_void, false);
    llvm::Type *CtorPFTy = llvm::PointerType::get(CtorFTy, espace_adressage);

    /* Le type d'une entrée dans la liste, { i32, void ()*, i8* }. */
    llvm::StructType *CtorStructTy = llvm::StructType::get(type_int32, CtorPFTy, type_void_ptr);

    /* Construiction des tableaux de constructeurs/desctructeurs. */
    std::vector<llvm::Constant *> tableau_constructeurs;
    tableau_constructeurs.reserve(1);

    auto tableau_membre = std::vector<llvm::Constant *>();
    tableau_membre.push_back(llvm::ConstantInt::get(type_int32, 0));
    tableau_membre.push_back(donne_ou_crée_déclaration_fonction(atome_fonc));
    /* Données associées. Nous n'en avons aucune. */
    tableau_membre.push_back(llvm::ConstantPointerNull::get(type_void_ptr));

    auto constructeur = llvm::ConstantStruct::get(CtorStructTy, tableau_membre);

    tableau_constructeurs.push_back(constructeur);

    auto type_tableau = llvm::ArrayType::get(CtorStructTy, tableau_constructeurs.size());
    auto init_constructeurs = llvm::ConstantArray::get(type_tableau, tableau_constructeurs);

    auto nom_globale_llvm = vers_string_ref(nom_globale);
    auto liaison = llvm::GlobalValue::AppendingLinkage;
    auto résultat = new llvm::GlobalVariable(
        *m_module, type_tableau, false, liaison, init_constructeurs, nom_globale_llvm);

    /* Le lieur LTO n'a pas l'air d'apprécier que nous renseignons un alignement
     * sur des variables liées avec "appending". Donc n'en renseignons aucun. */
    résultat->setAlignment(llvm::MaybeAlign());
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
#    define DEBOGUE_IR
#endif

#ifdef DEBOGUE_IR
/* Chemin du fichier de code binaire LLVM généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_bc_llvm(int64_t index)
{
    return kuri::chemin_systeme::chemin_temporaire(enchaine("kuri", index, ".bc"));
}

/* Chemin du fichier de code LLVM généré par la coulisse. */
static kuri::chemin_systeme chemin_fichier_ll_llvm(int64_t index)
{
    return kuri::chemin_systeme::chemin_temporaire(enchaine("kuri", index, ".ll"));
}

static kuri::chaine_statique donne_assembleur_llvm()
{
    return LLVM_ASSEMBLEUR;
}

static std::optional<ErreurCommandeExterne> valide_llvm_ir(llvm::Module &module, int64_t index)
{
    auto const fichier_ll = chemin_fichier_ll_llvm(index);
    auto const fichier_bc = chemin_fichier_bc_llvm(index);

    std::error_code ec;
    llvm::raw_fd_ostream dest(vers_string_ref(fichier_ll), ec, llvm::sys::fs::OF_None);
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

static bool est_intrinsèque_supportée_par_llvm(IdentifiantCode const *ident)
{
    return ident != ID::intrinsèque_est_adresse_données_constantes &&
           ident != ID::atomique_échange && ident != ID::atomique_toujours_sans_verrou &&
           ident != ID::atomique_est_sans_verrou;
}

std::optional<ErreurCoulisse> CoulisseLLVM::génère_code_impl(const ArgsGénérationCode &args)
{
    if (!initialise_llvm()) {
        return ErreurCoulisse{"Impossible d'intialiser LLVM."};
    }

    auto &espace = *args.espace;
    auto &repr_inter = *args.ri_programme;

    POUR (repr_inter.donne_fonctions()) {
        if (!it->est_intrinsèque()) {
            continue;
        }

        if (!est_intrinsèque_supportée_par_llvm(it->decl->ident)) {
            auto message = enchaine("Utilisation d'une intrinsèque non-implémentée pour LLVM : ",
                                    it->decl->ident->nom,
                                    ".");
            return ErreurCoulisse{message};
        }
    }

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

    auto CPU = "generic";
    auto feature = "";
    auto options_cible = llvm::TargetOptions{};
    auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
    m_machine_cible = cible->createTargetMachine(triplet_cible, CPU, feature, options_cible, RM);

    crée_modules(repr_inter, triplet_cible);

    POUR (m_modules) {
        auto generatrice = GénératriceCodeLLVM(espace, *it);
        generatrice.génère_code();
    }

#ifdef DEBOGUE_IR
    POUR_INDEX (m_modules) {
        auto opt_erreur_validation = valide_llvm_ir(*it->module, index_it);
        if (opt_erreur_validation.has_value()) {
            auto erreur_validation = opt_erreur_validation.value();
            auto message_erreur = enchaine("Erreur lors de la validation du code LLVM.\n",
                                           "La commande a retourné :\n\n",
                                           erreur_validation.message);
            return ErreurCoulisse{message_erreur};
        }
    }
#endif

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
    if (options.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE) {
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
        vers_string_ref(module->chemin_fichier_objet), ec, llvm::sys::fs::OF_None);

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
        module->définis_données_constantes(opt_données_constantes.value());
    }
    module->définis_globales(repr_inter.donne_globales());

#ifndef COMPILE_EN_PLUSIEURS_MODULE
    module->définis_fonctions(repr_inter.donne_fonctions());

    /* À FAIRE : pour la compilation en plusieurs fichiers il faudra proprement
     * gérer les liaisons des globales, ainsi que leur donner des noms uniques. */
#else
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

        // dbg() << "Nombre instructions : " << nombre_instructions;

        auto pointeur_fonction = fonctions.begin() + index_première_fonction;
        auto taille = i - index_première_fonction + 1;

        auto fonctions_du_modules = kuri::tableau_statique<AtomeFonction *>(pointeur_fonction,
                                                                            taille);

        module = crée_un_module("Fonction", triplet_cible);
        module->définis_fonctions(fonctions_du_modules);

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
    résultat += m_modules.taille();

    POUR (m_modules) {
        résultat += taille_de(llvm::LLVMContext);
        résultat += taille_de(llvm::Module);
        résultat += it->chemin_fichier_objet.taille();
        résultat += it->erreur_fichier_objet.taille();
    }

    return résultat;
}
