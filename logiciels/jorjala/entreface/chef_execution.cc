/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "chef_execution.hh"

#include "coeur/jorjala.hh"

#include "tache.h"

ChefExecution::ChefExecution(JJL::Jorjala &jorjala, TaskNotifier *task_notifier)
    : JJL::ChefExécution(), m_jorjala(jorjala), m_task_notifier(task_notifier)
{
}

void ChefExecution::rappel_démarre_évaluation(JJL::Chaine param1)
{
    demarre_evaluation(param1.vers_std_string().c_str());
}

void ChefExecution::rappel_rapporte_progression(float param1)
{
    indique_progression(param1);
}

void ChefExecution::rappel_rapporte_progression_parallèle(float param1)
{
    indique_progression_parallele(param1);
}

bool ChefExecution::rappel_doit_interrompre()
{
    return interrompu();
}

bool ChefExecution::interrompu() const
{
    return m_jorjala.donne_interrompu();
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

void ChefExecution::demarre_evaluation(const QString &message)
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
