/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

bool initialise_llvm();
void issitialise_llvm();

namespace llvm {
class Module;
class TargetMachine;
}  // namespace llvm

struct CoulisseLLVM final : public Coulisse {
  private:
    llvm::Module *m_module = nullptr;
    llvm::TargetMachine *m_machine_cible = nullptr;

  public:
    ~CoulisseLLVM();

  private:
    bool génère_code_impl(Compilatrice &compilatrice,
                          EspaceDeTravail &espace,
                          Programme const *programme,
                          CompilatriceRI &compilatrice_ri,
                          Broyeuse &) override;

    bool crée_fichier_objet_impl(Compilatrice &compilatrice,
                                 EspaceDeTravail &espace,
                                 Programme const *programme,
                                 CompilatriceRI &compilatrice_ri) override;

    bool crée_exécutable_impl(Compilatrice &compilatrice,
                              EspaceDeTravail &espace,
                              Programme const *programme) override;
};
