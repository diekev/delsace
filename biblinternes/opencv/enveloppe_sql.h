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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <string>

namespace sql {

class Connection;
class ResultSet;
class Statement;

}  /* namespace sql */

/* **************************** ResultatsSQL ******************************** */

class ResultatsSQL {
	sql::ResultSet *m_resultats = nullptr;

public:
	ResultatsSQL() = delete;

	~ResultatsSQL();

	ResultatsSQL(sql::ResultSet *resultats);

	ResultatsSQL(const ResultatsSQL &autre) = delete;
	ResultatsSQL &operator=(const ResultatsSQL &autre) = delete;

	ResultatsSQL(ResultatsSQL &&autre) = default;
	ResultatsSQL &operator=(ResultatsSQL &&autre) = default;

	/**
	 * Va vers la ligne suivante du résultat de la requête. Cette fonction doit
	 * être appelé avant d'accéder à la première ligne du résultat. Retourne
	 * vrai si une ligne suivante existe, faux s'il n'y aucun ou plus de
	 * résultats.
	 */
	bool suivant();

	/**
	 * Retourne la valeur de la colonne dont le nom est passé en paramètre sous
	 * forme d'une std::string. Lance une exception si la colonne n'existe pas.
	 */
	std::string valeur(const std::string &colonne) noexcept(false);
};

/* *************************** DeclarationSQL ******************************* */

class DeclarationSQL {
	sql::Statement *m_declaration = nullptr;

public:
	DeclarationSQL() = delete;

	DeclarationSQL(sql::Connection *connection);

	DeclarationSQL(const DeclarationSQL &autre) = delete;
	DeclarationSQL &operator=(const DeclarationSQL &autre) = delete;

	DeclarationSQL(DeclarationSQL &&autre) = default;
	DeclarationSQL &operator=(DeclarationSQL &&autre) = default;

	~DeclarationSQL();

	/**
	 * Lance la requête passée en paramètre sur la base de données et retourne
	 * un objet `ResultatsSQL` contenant les résultats de la requête. Lance une
	 * exception si la requête échoue.
	 */
	ResultatsSQL requiert(const std::string &requete) noexcept(false);
};
