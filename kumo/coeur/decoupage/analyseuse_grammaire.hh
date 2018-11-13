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

#include "analyseuse.hh"

#include <experimental/any>
#include <vector>

struct Colonne {
	std::string nom = "";
	size_t type{};
	int taille = 0;
	int octet = 4;
	bool cle_primaire = false;
	bool auto_inc = false;
	bool variable = false;
	bool signee = true;
	bool peut_etre_nulle = true;
	bool a_valeur_defaut = false;
	size_t id_valeur_defaut = 0;
	std::experimental::any defaut;
	std::string table = "";
	std::string ref = "";
	size_t suppression = 0;
	int ajournement = 0;
};

struct Table {
	std::vector<Colonne> colonnes{};
	std::string_view nom{};
};

struct Schema {
	std::vector<Table> tables{};
	std::string_view nom{};
};

class analyseuse_grammaire : public Analyseuse {
	Schema m_schema{};

public:
	analyseuse_grammaire(const std::vector<DonneesMorceaux> &identifiants, const TamponSource &tampon);

	void lance_analyse() override;

	const Schema *schema() const;

private:
	void analyse_schema();
	void analyse_declaration_table();
	void analyse_declaration_colonne(Table &table);
	void analyse_propriete_colonne(Colonne &colonne);

	bool requiers_type(size_t identifiant);
	bool requiers_propriete(size_t identifiant);
	bool requiers_valeur(size_t identifiant);
};
