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

#include "base_analyseuse.h"

namespace kangao {

class AnalyseuseLogique : public Analyseuse {
public:
	AnalyseuseLogique() = default;

	void lance_analyse(const std::vector<DonneesMorceaux> &identifiants) override;

private:
	void analyse_corps();
	void analyse_entree();
	void analyse_declaration(const int type);
	void analyse_expression(const std::string &nom, const int type);
	void analyse_interface();
	void analyse_logique();
	void analyse_sortie();
	void analyse_relation();
};

}  /* namespace kangao */
