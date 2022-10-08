/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include <string>

namespace geo {

class Maillage;

void charge_fichier_OBJ(Maillage &maillage, std::string const &chemin);

void ecris_fichier_OBJ(Maillage const &maillage, std::string const &chemin);

void charge_fichier_STL(Maillage &maillage, std::string const &chemin);

}  // namespace geo
