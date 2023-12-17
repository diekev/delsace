/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

#include "bloc_basique.hh"

struct Atome;
struct AtomeFonction;
struct ConstructriceRI;
struct EspaceDeTravail;
struct Instruction;

/* Structure pour contenir les différentes structures utilisées pour analyser la RI afin de
 * récupérer la mémoire entre différents appels.
 * Cette structure n'est pas sûre vis-à-vis du moultfilage.
 */
struct ContexteAnalyseRI {
  private:
    FonctionEtBlocs fonction_et_blocs{};
    VisiteuseBlocs visiteuse{fonction_et_blocs};

  public:
    void analyse_ri(EspaceDeTravail &espace, ConstructriceRI &constructrice, AtomeFonction *atome);

  private:
    /* Réinitialise les différentes structures. */
    void reinitialise();
};

void marque_instructions_utilisées(kuri::tableau<Instruction *, int> &instructions);

AtomeConstante *évalue_opérateur_binaire(InstructionOpBinaire const *inst,
                                         ConstructriceRI &constructrice);

Atome *peut_remplacer_instruction_binaire_par_opérande(InstructionOpBinaire const *op_binaire);
