/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

struct CompilatriceRI;
struct EspaceDeTravail;
struct AtomeFonction;

void optimise_code(EspaceDeTravail &espace,
                   CompilatriceRI &constructrice,
                   AtomeFonction *atome_fonc);
