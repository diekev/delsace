/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "structures/chaine.hh"

struct IdentifiantCode;
struct NoeudDeclarationEnteteFonction;
struct Type;

kuri::chaine broye_nom_simple(kuri::chaine_statique const &nom);

kuri::chaine const &nom_broye_type(Type *type);

kuri::chaine broye_nom_fonction(NoeudDeclarationEnteteFonction *decl,
                                kuri::chaine const &nom_module);

kuri::chaine_statique broye_nom_simple(IdentifiantCode *ident);
