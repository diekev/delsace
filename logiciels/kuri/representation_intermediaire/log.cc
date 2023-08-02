/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "log.hh"

bool log_actif = false;

void active_log()
{
    log_actif = true;
}

void desactive_log()
{
    log_actif = false;
}
