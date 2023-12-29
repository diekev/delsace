/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/chaine.hh"

/**
 * Ces structures sont les mêmes que celles définies par le langage (tableaux
 * via « [..]TYPE », et chaine via « chaine ») ; elles sont donc la même
 * définition que celles du langage. Elles sont utilisées pour pouvoir passer
 * des messages sainement entre la compilatrice et les métaprogrammes. Par
 * sainement, on entend que l'interface binaire de l'application doit être la
 * même.
 */

namespace kuri {

/* Structure pour passer les lexèmes aux métaprogrammes, via compilatrice_lèxe_fichier
 */
struct Lexeme {
    int genre = 0;
    chaine_statique texte{};
};

}  // namespace kuri
