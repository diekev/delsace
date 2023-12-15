/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <iostream>

namespace kuri {
struct chaine_statique;
}

/* ------------------------------------------------------------------------- */
/** \name Indentation.
 * Représente un niveau d'indentation pour le logage.
 * \{ */

struct Indentation {
    int v = 0;

    void incrémente()
    {
        v += 1;
    }

    void décrémente()
    {
        v -= 1;
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Logueuse.
 *
 * Enveloppe autour de std::ostream pour écrire de manière synchrone dans les
 * flux standards. Un mutex est verrouillé lors de la création d'une logueuse,
 * et déverrouillé lors de sa desctruction.
 *
 * Le même mutex est utilisé pour tous les flux afin de garantir que les données
 * écrites par une logueuse ne soient pas polluées par une autre.
 * \{ */

class Logueuse {
    std::ostream &os;

    template <typename T>
    friend const Logueuse &operator<<(const Logueuse &log_debug, T valeur);

    friend const Logueuse &operator<<(const Logueuse &log_debug, Indentation indentation);

  public:
    explicit Logueuse(std::ostream &flux_sortie);
    ~Logueuse();

    static void réinitialise_indentation();
    static void indente();
    static void désindente();

    struct IncrémenteuseTemporaire {
      private:
        int indentation_courante = 0;

      public:
        IncrémenteuseTemporaire();
        ~IncrémenteuseTemporaire();
    };
};

const Logueuse &operator<<(const Logueuse &log_debug, Indentation indentation);

template <typename T>
const Logueuse &operator<<(const Logueuse &log_debug, T valeur)
{
    log_debug.os << valeur;
    return log_debug;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Logage d'erreurs, d'informations de débogage. Écris vers stderr.
 * \{ */

Logueuse dbg();

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Logage d'informations. Écris vers stdout.
 * \{ */

Logueuse info();

/** \} */

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

kuri::chaine_statique chaine_indentations(int indentations);

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
