/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "log.hh"

static Indentation __indente_globale;

LogDebug dbg()
{
    return {};
}

LogDebug::LogDebug()
{
    for (auto i = 0; i < __indente_globale.v; ++i) {
        os << ' ' << ' ';
    }
}

LogDebug::~LogDebug()
{
    os << "\n";
}

void LogDebug::réinitialise_indentation()
{
    __indente_globale.v = 0;
}

void LogDebug::indente()
{
    __indente_globale.incrémente();
}

void LogDebug::désindente()
{
    __indente_globale.décrémente();
}

const LogDebug &operator<<(const LogDebug &log_debug, Indentation indentation)
{
    for (auto i = 0; i < indentation.v; ++i) {
        log_debug.os << ' ';
    }

    return log_debug;
}

LogDebug::IncrémenteuseTemporaire::IncrémenteuseTemporaire()
{
    indentation_courante = __indente_globale.v;
    __indente_globale.incrémente();
}

LogDebug::IncrémenteuseTemporaire::~IncrémenteuseTemporaire()
{
    __indente_globale.v = indentation_courante;
}
