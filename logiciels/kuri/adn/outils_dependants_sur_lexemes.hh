/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

namespace kuri {
struct chaine;
}  // namespace kuri

struct SiteSource;

void imprime_erreur(SiteSource site, kuri::chaine message);
