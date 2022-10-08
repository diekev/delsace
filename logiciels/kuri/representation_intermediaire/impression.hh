/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <ostream>

#include "structures/tableau.hh"

struct Atome;
struct AtomeFonction;
struct Instruction;

void imprime_information_atome(Atome const *atome, std::ostream &os);

void imprime_atome(Atome const *atome, std::ostream &os);

int numerote_instructions(AtomeFonction const &fonction);

void imprime_fonction(AtomeFonction const *atome_fonc,
                      std::ostream &os,
                      bool inclus_nombre_utilisations = false,
                      bool surligne_inutilisees = false,
                      std::function<void(Instruction const &, std::ostream &)> rappel = nullptr);
void imprime_instruction(Instruction const *inst, std::ostream &os);
void imprime_instructions(
    kuri::tableau<Instruction *, int> const &instructions,
    std::ostream &os,
    bool inclus_nombre_utilisations = false,
    bool surligne_inutilisees = false,
    std::function<void(Instruction const &, std::ostream &)> rappel = nullptr);
