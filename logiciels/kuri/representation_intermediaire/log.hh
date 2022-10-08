/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <ostream>

extern bool log_actif;

void active_log();

void desactive_log();

template <typename... Ts>
void log(std::ostream &os, Ts... ts)
{
    if (!log_actif) {
        return;
    }

    ((os << ts), ...);
    os << '\n';
}
