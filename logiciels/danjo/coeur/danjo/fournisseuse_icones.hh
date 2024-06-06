/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <QIcon>
#include <optional>

namespace danjo {

enum class IcônePourBouton : int {
    AJOUTE,
    AJOUTE_IMAGE_CLÉ,
    CHOISIR_FICHIER,
    DÉPLACE_EN_HAUT,
    DÉPLACE_EN_BAS,
    ÉCHELLE_VALEUR,
    LISTE_CHAINE,
    RAFRAICHIS_TEXTE,
    SUPPRIME,
};

enum class ÉtatIcône {
    ACTIF,
    INACTIF,
};

class FournisseuseIcône {
  public:
    virtual ~FournisseuseIcône() = default;

    virtual std::optional<QIcon> icone_pour_bouton(const IcônePourBouton pour_bouton,
                                                   const ÉtatIcône état) = 0;
    virtual std::optional<QIcon> icone_pour_identifiant(std::string const &identifiant,
                                                        const ÉtatIcône état) = 0;
};

FournisseuseIcône &donne_fournisseuse_icone();

void définis_fournisseuse_icone(FournisseuseIcône &fournisseuse);

}  // namespace danjo
