/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "compteuse_cas.hh"

#include <ostream>

void CompteuseCas::ajoute_cas(bool prédicat)
{
    cas_vrai += prédicat;
    cas_totaux += 1;
}

void CompteuseCas::imprime(std::ostream &os)
{
    os << cas_vrai << " / " << cas_totaux << " (" << (cas_totaux - cas_vrai) << ")" << '\n';
}
