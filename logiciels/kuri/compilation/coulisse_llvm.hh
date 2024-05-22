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
struct Bibliothèque;
struct DonnéesConstantes;
struct ProgrammeRepreInter;

namespace llvm {
class LLVMContext;
class Module;
class TargetMachine;
}  // namespace llvm

/* ------------------------------------------------------------------------- */
/** \name DonnéesModule
 * \{ */

struct DonnéesModule {
    llvm::LLVMContext *contexte_llvm = nullptr;
    llvm::Module *module = nullptr;

    kuri::chemin_systeme chemin_fichier_objet{};

  private:
    const DonnéesConstantes *m_données_constantes = nullptr;
    kuri::tableau_statique<AtomeGlobale *> m_globales{};
    kuri::tableau_statique<AtomeFonction *> m_fonctions{};

    kuri::ensemble<AtomeFonction const *> m_ensemble_fonctions{};
    kuri::ensemble<AtomeGlobale const *> m_ensemble_globales{};

  public:
    kuri::chaine erreur_fichier_objet{};

    void définis_données_constantes(const DonnéesConstantes *données_constantes);
    const DonnéesConstantes *donne_données_constantes() const;

    void définis_fonctions(kuri::tableau_statique<AtomeFonction *> fonctions);
    kuri::tableau_statique<AtomeFonction *> donne_fonctions() const;
    bool fait_partie_du_module(AtomeFonction const *fonction) const;

    void définis_globales(kuri::tableau_statique<AtomeGlobale *> globales);
    kuri::tableau_statique<AtomeGlobale *> donne_globales() const;
    bool fait_partie_du_module(AtomeGlobale const *globale) const;

  private:
    void ajourne_globales();
};

/** \} */

struct CoulisseLLVM final : public Coulisse {
  private:
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

    int64_t mémoire_utilisée() const override;
};
