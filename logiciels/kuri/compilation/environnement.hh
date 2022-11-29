/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

namespace kuri {
struct chemin_systeme;
}

enum class ArchitectureCible : int;

void precompile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri);

void compile_objet_r16(kuri::chemin_systeme const &chemin_racine_kuri,
                       ArchitectureCible architecture_cible);
