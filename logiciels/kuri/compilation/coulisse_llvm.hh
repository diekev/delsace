/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/tableau.hh"

bool initialise_llvm();
void issitialise_llvm();

struct Bibliotheque;

namespace llvm {
class LLVMContext;
class Module;
class TargetMachine;
}  // namespace llvm

struct CoulisseLLVM final : public Coulisse {
  private:
    kuri::tableau<Bibliotheque *> m_bibliothèques{};
    llvm::LLVMContext *m_contexte_llvm = nullptr;
    llvm::Module *m_module = nullptr;
    llvm::TargetMachine *m_machine_cible = nullptr;

  public:
    ~CoulisseLLVM();

  private:
    bool génère_code_impl(ArgsGénérationCode const &args) override;

    bool crée_fichier_objet_impl(ArgsCréationFichiersObjets const &args) override;

    bool crée_exécutable_impl(ArgsLiaisonObjets const &args) override;
};
