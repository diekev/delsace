/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <string>

namespace kuri {
struct chaine;
struct chaine_statique;
}  // namespace kuri

kuri::chaine supprime_accents(kuri::chaine_statique avec_accent);
bool remplace(std::string &std_string, std::string_view motif, std::string_view remplacement);

void inclus_systeme(std::ostream &os, kuri::chaine_statique fichier);
void inclus(std::ostream &os, kuri::chaine_statique fichier);

void prodeclare_struct(std::ostream &os, kuri::chaine_statique nom);
void prodeclare_struct_espace(std::ostream &os,
                              kuri::chaine_statique nom,
                              kuri::chaine_statique espace,
                              kuri::chaine_statique param_gabarit);

bool remplace_si_different(kuri::chaine_statique nom_source, kuri::chaine_statique nom_dest);
