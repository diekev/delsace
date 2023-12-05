/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <functional>
#include <sys/wait.h>
#include <thread>

#include "structures/tableau.hh"
#include "structures/tablet.hh"

namespace kuri {

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesEnSérie
 *  Classe de base pour créer des poules de tâches.
 * \{ */

struct PouleDeTâches {
  public:
    virtual ~PouleDeTâches() = default;

    virtual void ajoute_tâche(std::function<void()> &&tâche) = 0;

    /* Attends que toutes les tâches sont finies. */
    virtual void attends_sur_tâches() = 0;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesEnSérie
 *  Exécutes des tâches les unes après les autres.
 * \{ */

struct PouleDeTâchesEnSérie final : public PouleDeTâches {
  private:
    kuri::tableau<std::function<void()>> m_tâches{};

  public:
    void ajoute_tâche(std::function<void()> &&tâche) override;

    void attends_sur_tâches() override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesSousProcessus
 *  Exécute des tâches dans des sous-processus.
 * \{ */

struct PouleDeTâchesSousProcessus final : public PouleDeTâches {
  private:
    kuri::tablet<pid_t, 16> m_enfants;

  public:
    void ajoute_tâche(std::function<void()> &&tâche) override;

    void attends_sur_tâches() override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PouleDeTâchesMoultFils
 *  Exécute des tâches dans des fils d'exécutions.
 * \{ */

struct PouleDeTâchesMoultFils final : public PouleDeTâches {
  private:
    kuri::tablet<std::thread *, 16> m_threads;

  public:
    void ajoute_tâche(std::function<void()> &&tâche) override;

    void attends_sur_tâches() override;
};

/** \} */

}  // namespace kuri
