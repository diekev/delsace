/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "log.hh"

#include <mutex>

#include "structures/chaine_statique.hh"

static Indentation __indente_globale;
static std::mutex __mutex_flux{};

LogDebug dbg()
{
    return {};
}

LogDebug::LogDebug()
{
    __mutex_flux.lock();
    os << chaine_indentations(__indente_globale.v);
}

LogDebug::~LogDebug()
{
    os << "\n";
    __mutex_flux.unlock();
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
    log_debug.os << chaine_indentations(indentation.v);
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

kuri::chaine_statique chaine_indentations(int indentations)
{
    static std::string chaine = std::string(1024, '\t');
    return {chaine.c_str(), static_cast<int64_t>(indentations)};
}

bool log_actif = false;

void active_log()
{
    log_actif = true;
}

void desactive_log()
{
    log_actif = false;
}
