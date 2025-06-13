/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "macros.hh"

#include <iostream>

void imprime_message_assert(const char *fichier,
                            int ligne,
                            const char *fonction,
                            const char *texte_ligne)
{
    std::cerr << fichier << ':' << ligne << ": " << fonction << '\n';
    std::cerr << "ÉCHEC DE L'ASSERTION : " << texte_ligne << '\n';
}
