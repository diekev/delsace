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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "assembleuse_logique.h"
#include "base_analyseuse.h"

namespace danjo {

class Manipulable;

class AnalyseuseLogique : public base_analyseuse {
    AssembleuseLogique m_assembleuse{};
    Manipulable *m_manipulable{};
    bool m_initialise_manipulable{};
    char pad[7];

  public:
    explicit AnalyseuseLogique(Manipulable *manipulable,
                               lng::tampon_source const &tampon,
                               dls::tableau<DonneesMorceaux> &identifiants,
                               bool initialise_manipulable = false);

    AnalyseuseLogique(AnalyseuseLogique const &) = default;
    AnalyseuseLogique &operator=(AnalyseuseLogique const &) = default;

    void lance_analyse(std::ostream &) override;

  private:
    void analyse_corps();
    void analyse_entree();
    void analyse_declaration(const int type);
    void analyse_expression(const dls::chaine &nom, const int type);
    void analyse_entreface();
    void analyse_logique();
    void analyse_sortie();
    void analyse_relation();
};

} /* namespace danjo */
