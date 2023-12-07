/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

struct AtomeFonction;
struct ConstructriceRI;
struct EspaceDeTravail;

void convertis_ssa(EspaceDeTravail &espace,
                   AtomeFonction *fonction,
                   ConstructriceRI &constructrice);
