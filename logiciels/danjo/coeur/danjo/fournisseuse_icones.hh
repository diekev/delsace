/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <QIcon>
#include <optional>

namespace danjo {

enum class ÉtatIcône {
    ACTIF,
    INACTIF,
};

class FournisseuseIcône {
  public:
    virtual ~FournisseuseIcône() = default;

    virtual std::optional<QIcon> icone_pour_bouton_animation(ÉtatIcône état) = 0;
    virtual std::optional<QIcon> icone_pour_echelle_valeur(ÉtatIcône état) = 0;
    virtual std::optional<QIcon> icone_pour_identifiant(std::string const &identifiant,
                                                        ÉtatIcône état) = 0;
};

FournisseuseIcône &donne_fournisseuse_icone();

void définit_fournisseuse_icone(FournisseuseIcône &fournisseuse);

}  // namespace danjo
