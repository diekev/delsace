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

#include "objets_communs.h"

#include <cppconn/driver.h>
#include <girafeenfeu/systeme_fichier/utilitaires.h>

ObjetsCommuns::~ObjetsCommuns()
{
	delete m_connection_sql;
}

void ObjetsCommuns::initialise_base_de_donnees(
		const std::string &hote,
		const std::string &usager,
		const std::string &mot_de_passe,
		const std::string &schema)
{
	/* Création d'une connection. */
	m_pilote_sql = get_driver_instance();
	m_connection_sql = m_pilote_sql->connect(hote, usager, mot_de_passe);

	m_connection_sql->setSchema(schema);
}

sql::Connection *ObjetsCommuns::connection()
{
	return m_connection_sql;
}

void ObjetsCommuns::initialise_dossiers()
{
	auto chemin_maison = systeme_fichier::chemin_repertoire_maison();
	auto chemin_girafe = chemin_maison / "girafeenfeu";
	auto chemin_daburu = chemin_girafe / "daburu";
	auto chemin_images = chemin_daburu / "images";
	auto chemin_modeles = chemin_daburu / "modèles";
	auto chemin_cvs = chemin_daburu / "cvs";

	std::experimental::filesystem::create_directory(chemin_girafe);
	std::experimental::filesystem::create_directory(chemin_daburu);
	std::experimental::filesystem::create_directory(chemin_modeles);
	std::experimental::filesystem::create_directory(chemin_images);
	std::experimental::filesystem::create_directory(chemin_cvs);
	std::experimental::filesystem::create_directory(chemin_images / "entrainement");
	std::experimental::filesystem::create_directory(chemin_images / "test");

	m_chemin_racine = chemin_daburu;
}

std::experimental::filesystem::path ObjetsCommuns::chemin_racine() const
{
	return m_chemin_racine;
}
