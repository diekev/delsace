/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "poule_de_taches.hh"

#ifndef _MSC_VER
#    include <unistd.h>
#endif

namespace kuri {

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesEnSérie
 * \{ */

void PouleDeTâchesEnSérie::ajoute_tâche(std::function<void()> &&tâche)
{
    m_tâches.ajoute(tâche);
}

bool PouleDeTâchesEnSérie::attends_sur_tâches()
{
    POUR (m_tâches) {
        it();
    }
    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesSousProcessus
 * \{ */

void PouleDeTâchesSousProcessus::ajoute_tâche(std::function<void()> &&tâche)
{
#ifdef _MSC_VER
    tâche();
#else
    auto child_pid = fork();
    if (child_pid == 0) {
        tâche();
        exit(0);
    }

    m_enfants.ajoute(child_pid);
#endif
}

bool PouleDeTâchesSousProcessus::attends_sur_tâches()
{
    auto résultat = true;

#ifndef _MSC_VER
    POUR (m_enfants) {
        int etat;
        if (waitpid(it, &etat, 0) != it) {
            continue;
        }

        if (!WIFEXITED(etat)) {
            résultat = false;
            continue;
        }

        if (WEXITSTATUS(etat) != EXIT_SUCCESS) {
            résultat = false;
            continue;
        }
    }
#endif

    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesMoultFils
 * \{ */

void PouleDeTâchesMoultFils::ajoute_tâche(std::function<void()> &&tâche)
{
    auto thread = new std::thread(tâche);
    m_threads.ajoute(thread);
}

bool PouleDeTâchesMoultFils::attends_sur_tâches()
{
    POUR (m_threads) {
        it->join();
        delete it;
    }
    return true;
}

/** \} */

}  // namespace kuri
