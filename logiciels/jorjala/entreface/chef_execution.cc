/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "chef_execution.hh"

#include "coeur/jorjala.hh"

#include "tache.h"

ChefExecution::ChefExecution(JJL::Jorjala &jorjala, TaskNotifier *task_notifier)
    : m_jorjala(jorjala), m_task_notifier(task_notifier)
{
}

bool ChefExecution::interrompu() const
{
    return m_jorjala.interrompu();
}

void ChefExecution::indique_progression(float progression)
{
    m_task_notifier->signale_ajournement_progres(progression);
}

void ChefExecution::indique_progression_parallele(float delta)
{
    m_mutex_progression.lock();
    m_progression_parallele += delta;
    indique_progression(m_progression_parallele);
    m_mutex_progression.unlock();
}

void ChefExecution::demarre_evaluation(const char *message)
{
    m_progression_parallele = 0.0f;
    m_nombre_execution += 1;
    m_task_notifier->signale_debut_evaluation(message, m_nombre_execution, m_nombre_a_executer);
}

void ChefExecution::reinitialise()
{
    m_nombre_a_executer = 0;
    m_nombre_execution = 0;
}

void ChefExecution::incremente_compte_a_executer()
{
    m_nombre_a_executer += 1;
}
