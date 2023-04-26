/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/patrons_conception/observation.hh"

namespace JJL {
class Jorjala;
}

class BaseEditrice;
class QMouseEvent;
class QWheelEvent;

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice);

void gere_pression_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id);

void gere_mouvement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id);

void gere_relachement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id);

void gere_molette_souris(JJL::Jorjala &jorjala, QWheelEvent *e, const char *id);
