/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "coeur/jorjala.hh"

#include <mutex>

class QString;
class TaskNotifier;

class ChefExecution final : public JJL::ChefExecution {
    JJL::Jorjala &m_jorjala;
    TaskNotifier *m_task_notifier;
    float m_progression_parallele = 0.0f;
    std::mutex m_mutex_progression{};

    int m_nombre_a_executer = 0;
    int m_nombre_execution = 0;

  public:
    ChefExecution(JJL::Jorjala &jorjala, TaskNotifier *task_notifier);

    void rappel_démarre_évaluation(JJL::Chaine param1) override;

    void rappel_rapporte_progression(float param1) override;

    void rappel_rapporte_progression_parallèle(float param1) override;

    bool rappel_doit_interrompre() override;

    bool interrompu() const;

    void indique_progression(float progression);

    void indique_progression_parallele(float delta);

    void demarre_evaluation(const QString &message);

    void reinitialise();

    void incremente_compte_a_executer();
};
