/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <mutex>

namespace JJL {
class Jorjala;
}

class TaskNotifier;

class ChefExecution {
    JJL::Jorjala &m_jorjala;
    TaskNotifier *m_task_notifier;
    float m_progression_parallele = 0.0f;
    std::mutex m_mutex_progression{};

    int m_nombre_a_executer = 0;
    int m_nombre_execution = 0;

  public:
    ChefExecution(JJL::Jorjala &jorjala, TaskNotifier *task_notifier);

    bool interrompu() const;

    void indique_progression(float progression);

    void indique_progression_parallele(float delta);

    void demarre_evaluation(const char *message);

    void reinitialise();

    void incremente_compte_a_executer();
};
