/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <variant>

#include "danjo.h"

struct CheminFichier {
    const char *chemin = nullptr;
    static int tag;
};

struct SourceFichier {
    dls::chaine texte = "";
    static int tag;
};

using ResultatCheminEntreface = std::variant<CheminFichier, SourceFichier>;

void initialise_entreface(danjo::Manipulable *manipulable, const ResultatCheminEntreface &res);

void initialise_entreface(danjo::GestionnaireInterface *gestionnaire,
                          danjo::Manipulable *manipulable,
                          const ResultatCheminEntreface &res);
