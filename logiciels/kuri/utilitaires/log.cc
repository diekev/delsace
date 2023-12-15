/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "log.hh"

#include <mutex>

#include "structures/chaine_statique.hh"

static Indentation __indente_globale;
static std::mutex __mutex_flux{};

/* ------------------------------------------------------------------------- */
/** \name Logueuse.
 * \{ */

Logueuse::Logueuse(std::ostream &flux_sortie) : os(flux_sortie)
{
    __mutex_flux.lock();
    os << chaine_indentations(__indente_globale.v);
}

Logueuse::~Logueuse()
{
    os << "\n";
    __mutex_flux.unlock();
}

void Logueuse::réinitialise_indentation()
{
    __indente_globale.v = 0;
}

void Logueuse::indente()
{
    __indente_globale.incrémente();
}

void Logueuse::désindente()
{
    __indente_globale.décrémente();
}

const Logueuse &operator<<(const Logueuse &log_debug, Indentation indentation)
{
    log_debug.os << chaine_indentations(indentation.v);
    return log_debug;
}

Logueuse::IncrémenteuseTemporaire::IncrémenteuseTemporaire()
{
    indentation_courante = __indente_globale.v;
    __indente_globale.incrémente();
}

Logueuse::IncrémenteuseTemporaire::~IncrémenteuseTemporaire()
{
    __indente_globale.v = indentation_courante;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Logage d'erreurs, d'informations de débogage.
 * \{ */

Logueuse dbg()
{
    return Logueuse(std::cerr);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Logage d'informations.
 * \{ */

Logueuse info()
{
    return Logueuse(std::cout);
}

/** \} */

kuri::chaine_statique chaine_indentations(int indentations)
{
    static std::string chaine = std::string(1024, '\t');
    return {chaine.c_str(), static_cast<int64_t>(indentations)};
}
