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

#include "enveloppe_sql.h"

#include <cppconn/connection.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

/* **************************** ResultatsSQL ******************************** */

ResultatsSQL::~ResultatsSQL()
{
	delete m_resultats;
}

ResultatsSQL::ResultatsSQL(sql::ResultSet *resultats)
	: m_resultats(resultats)
{}

bool ResultatsSQL::suivant()
{
	return m_resultats->next();
}

std::string ResultatsSQL::valeur(const std::string &colonne) noexcept(false)
{
	return m_resultats->getString(colonne);
}

/* *************************** DeclarationSQL ******************************* */

DeclarationSQL::DeclarationSQL(sql::Connection *connection)
	: m_declaration(connection->createStatement())
{}

DeclarationSQL::~DeclarationSQL()
{
	delete m_declaration;
}

ResultatsSQL DeclarationSQL::requiert(const std::string &requete) noexcept(false)
{
	auto resultats = m_declaration->executeQuery(requete);

	return ResultatsSQL(resultats);
}
