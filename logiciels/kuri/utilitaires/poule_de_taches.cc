/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "poule_de_taches.hh"

#include <unistd.h>

namespace kuri {

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesEnSérie
 * \{ */

void PouleDeTâchesEnSérie::ajoute_tâche(std::function<void()> &&tâche)
{
    m_tâches.ajoute(tâche);
}

void PouleDeTâchesEnSérie::attends_sur_tâches()
{
    POUR (m_tâches) {
        it();
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesSousProcessus
 * \{ */

void PouleDeTâchesSousProcessus::ajoute_tâche(std::function<void()> &&tâche)
{
    auto child_pid = fork();
    if (child_pid == 0) {
        tâche();
        exit(0);
    }

    m_enfants.ajoute(child_pid);
}

void PouleDeTâchesSousProcessus::attends_sur_tâches()
{
    POUR (m_enfants) {
        int etat;
        if (waitpid(it, &etat, 0) != it) {
            continue;
        }

        if (!WIFEXITED(etat)) {
            continue;
        }

        if (WEXITSTATUS(etat) != EXIT_SUCCESS) {
            continue;
        }
    }
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

void PouleDeTâchesMoultFils::attends_sur_tâches()
{
    POUR (m_threads) {
        it->join();
        delete it;
    }
}

/** \} */

}  // namespace kuri
