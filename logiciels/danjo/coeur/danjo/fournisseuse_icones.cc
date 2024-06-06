/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "fournisseuse_icones.hh"

namespace danjo {

class FournisseuseIcôneDefaut final : public FournisseuseIcône {
  public:
    std::optional<QIcon> icone_pour_bouton(const IcônePourBouton, ÉtatIcône /*état*/) override
    {
        return {};
    }

    std::optional<QIcon> icone_pour_identifiant(std::string const & /*identifiant*/,
                                                ÉtatIcône /*état*/) override
    {
        return {};
    }
};

static FournisseuseIcôneDefaut __fournisseuse_defaut = {};
static FournisseuseIcône *__fournisseuse = &__fournisseuse_defaut;

FournisseuseIcône &donne_fournisseuse_icone()
{
    return *__fournisseuse;
}

void définis_fournisseuse_icone(FournisseuseIcône &fournisseuse)
{
    __fournisseuse = &fournisseuse;
}

}  // namespace danjo
