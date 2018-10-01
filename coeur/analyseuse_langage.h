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

#include "analyseur.h"

#include <unordered_map>

#include "postfix.h"

namespace langage {

struct DonneesVariables {
	std::string valeur{};

	DonneesVariables() = default;
};

using Expression = std::vector<Variable>;

struct DonneesFonction {
	std::unordered_map<std::string, DonneesVariables> variables_locales;
	std::vector<Expression> expressions;
	Expression expression_retour;
};

struct DonneesScript {
	std::unordered_map<std::string, DonneesVariables> variables;
	std::unordered_map<std::string, DonneesFonction> fonctions;
};

class AnalyseuseLangage : public Analyseuse {
	DonneesScript m_donnees_script;
	std::unordered_map<std::string, double> m_chronometres;

public:
	void lance_analyse(const std::vector<DonneesMorceaux> &morceaux) override;

private:
	void analyse_declaration();
	void analyse_imprime();
	void analyse_variable();
	void analyse_chronometre();
	void analyse_fonction();
	void analyse_expression(DonneesFonction &donnees_fonction);
};

}  /* namespace langage */
