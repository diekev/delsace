/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

struct Typeuse;

struct CoulisseASM final : public Coulisse {
  private:
    std::optional<ErreurCoulisse> génère_code_impl(ArgsGénérationCode const &args) override;

    std::optional<ErreurCoulisse> crée_fichier_objet_impl(
        ArgsCréationFichiersObjets const &args) override;

    std::optional<ErreurCoulisse> crée_exécutable_impl(ArgsLiaisonObjets const &args) override;

    int64_t mémoire_utilisée() const override;
};

struct AtomeFonction;
void classifie_arguments(Typeuse &typeuse, AtomeFonction const *fonction);
