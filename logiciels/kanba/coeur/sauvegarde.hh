/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

namespace dls {
class chaine;
}

namespace KNB {

struct Kanba;

void écris_projet(Kanba &kanba, dls::chaine const &chemin_projet);

bool lis_projet(Kanba &kanba, dls::chaine const &chemin_projet);

}  // namespace KNB
