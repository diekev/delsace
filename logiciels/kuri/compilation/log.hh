/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <iostream>

namespace kuri {
struct chaine_statique;
}

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

struct LogInfo {
    std::ostream &os = std::cout;

    LogInfo();
    ~LogInfo();
};

LogInfo info();

const LogInfo &operator<<(const LogInfo &log_info, Indentation indentation);

template <typename T>
const LogInfo &operator<<(const LogInfo &log_info, T valeur)
{
    log_info.os << valeur;
    return log_info;
}

struct LogDebug {
    std::ostream &os = std::cerr;

    LogDebug();
    ~LogDebug();

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

LogDebug dbg();

const LogDebug &operator<<(const LogDebug &log_debug, Indentation indentation);

template <typename T>
const LogDebug &operator<<(const LogDebug &log_debug, T valeur)
{
    log_debug.os << valeur;
    return log_debug;
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
