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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <filesystem>

#include "biblinternes/structures/chaine.hh"

namespace filesystem = std::filesystem;

class Noeud;
class Graphe;

dls::chaine id_dot_pour_noeud(Noeud const *noeud, bool apostrophes = true);
dls::chaine chaine_dot_pour_graphe(Graphe const &graphe);

class ImprimeuseGraphe {
    Graphe *m_graph;

  public:
    explicit ImprimeuseGraphe(Graphe *graph);

    void operator()(filesystem::path const &path);
};
