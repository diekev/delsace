/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/chemin_systeme.hh"
#include "structures/tableau.hh"

bool initialise_llvm();
void issitialise_llvm();

struct AtomeGlobale;
struct AtomeFonction;
struct Bibliotheque;
struct DonnéesConstantes;
struct ProgrammeRepreInter;

namespace llvm {
class LLVMContext;
class Module;
class TargetMachine;
}  // namespace llvm

struct DonnéesModule {
    llvm::LLVMContext *contexte_llvm = nullptr;
    llvm::Module *module = nullptr;

    kuri::chemin_systeme chemin_fichier_objet{};

    const DonnéesConstantes *données_constantes = nullptr;
    kuri::tableau_statique<AtomeGlobale *> globales{};
    kuri::tableau_statique<AtomeFonction *> fonctions{};

    kuri::chaine erreur_fichier_objet{};
};

struct CoulisseLLVM final : public Coulisse {
  private:
    kuri::tableau<Bibliotheque *> m_bibliothèques{};
    llvm::TargetMachine *m_machine_cible = nullptr;

    kuri::tableau<DonnéesModule *> m_modules{};

  public:
    ~CoulisseLLVM();

  private:
    std::optional<ErreurCoulisse> génère_code_impl(ArgsGénérationCode const &args) override;

    std::optional<ErreurCoulisse> crée_fichier_objet_impl(
        ArgsCréationFichiersObjets const &args) override;

    std::optional<ErreurCoulisse> crée_exécutable_impl(ArgsLiaisonObjets const &args) override;

    void crée_fichier_objet(DonnéesModule *module);

    void crée_modules(ProgrammeRepreInter const &repr_inter, const std::string &triplet_cible);

    DonnéesModule *crée_un_module(kuri::chaine_statique nom, const std::string &triplet_cible);
};
