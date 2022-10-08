/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

struct AtomeFonction;
struct ConstructriceRI;
struct EspaceDeTravail;
struct Instruction;

void marque_instructions_utilisees(kuri::tableau<Instruction *, int> &instructions);

void analyse_ri(EspaceDeTravail &espace, AtomeFonction *atome);
