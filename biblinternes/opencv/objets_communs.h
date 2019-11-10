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

#include <filesystem>
#include "biblinternes/structures/chaine.hh"

namespace sql {

class Connection;
class Driver;

}  /* namespace sql */

class ObjetsCommuns {
	sql::Driver *m_pilote_sql = nullptr;
	sql::Connection *m_connection_sql = nullptr;

	std::filesystem::path m_chemin_racine = {};

public:
	ObjetsCommuns() = default;

	ObjetsCommuns(const ObjetsCommuns &autre) = default;
	ObjetsCommuns &operator=(const ObjetsCommuns &autre) = default;

	~ObjetsCommuns();

	/**
	 * @brief Initialise le pilote de la base de données et la connection
	 *        principale vers celle-ci.
	 *
	 * @param hote          Le nom de l'hôte de la base de données.
	 * @param usager        Le nom d'utilisateur de la base de données.
	 * @param mot_de_passe  Le mot de passe de la base de données.
	 * @param schema        Le schéma de la base de données à utiliser.
	 */
	void initialise_base_de_donnees(
			const dls::chaine &hote,
			const dls::chaine &usager,
			const dls::chaine &mot_de_passe,
			const dls::chaine &schema);

	/**
	 * @return Un pointer vers la connection de la base de données.
	 */
	sql::Connection *connection();

	/**
	 * Crée les dossiers utilisés par le programme.
	 */
	void initialise_dossiers();

	/**
	 * Retourne le chemin vers le dossier racine du programme, là où le
	 * programme sauvegarde ses données.
	 */
	std::filesystem::path chemin_racine() const;
};
