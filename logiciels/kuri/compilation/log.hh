/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <iostream>

struct LogConditionel {
    std::ostream &os;
    bool actif = false;
};

template <typename T>
LogConditionel &operator<<(LogConditionel &log, T valeur)
{
    if (log.actif) {
        log.os << valeur;
    }
    return log;
}
