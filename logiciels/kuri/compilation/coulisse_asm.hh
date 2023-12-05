/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

struct CoulisseASM final : public Coulisse {
  private:
    bool génère_code_impl(ArgsGénérationCode const &args) override;

    bool crée_fichier_objet_impl(ArgsCréationFichiersObjets const &args) override;

    bool crée_exécutable_impl(ArgsLiaisonObjets const &args) override;
};
