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

#include <filesystem>

#include "biblinternes/structures/chaine.hh"

namespace arachne {

/**
 * La classe gestionnaire s'occupe de la création et suppression des dossiers où
 * se situe les bases de données.
 */
class gestionnaire {
	std::filesystem::path m_chemin_racine;
	std::filesystem::path m_chemin_courant;
	dls::chaine m_base_courante;

public:
	/**
	 * Construit un gestionnaire par défaut. Le chemin racine est désigné comme
	 * étant ~/arachne.
	 */
	gestionnaire();

	/**
	 * Crée le dossier où se trouvera la base de données, le nom duquel étant
	 * égal au nom de la base de données passé en paramètre. Après la création
	 * de la base du dossier, le chemin courant et la base de données courante
	 * seront le chemin du dossier et la base de données nouvellement créés.
	 *
	 * Si une base de données avec le même nom existe déjà, le dossier n'est pas
	 * créé, mais l'état du gestionnaire est changé pour se plavé sur la base
	 * de données existante.
	 */
	void cree_base_donnees(const dls::chaine &nom);

	/**
	 * Supprime le dossier où se trouve la base de données, le nom duquel étant
	 * égal au nom passé en paramètre. Après la suppression du dossier, le
	 * chemin courant et la base de données courante sont réinitialisés avec des
	 * valeurs par défaut.
	 */
	void supprime_base_donnees(const dls::chaine &nom);

	/**
	 * Change la base de données courante pour celle dont le nom est spécifiée
	 * en paramètre. Si aucune base de données avec le nom indiqué n'existe,
	 * l'état du gestionnaire n'est pas modifié.
	 *
	 * Retourne vrai si la base de données courante a été changé.
	 */
	bool change_base_donnees(const dls::chaine &nom);

	/**
	 * Retourne le chemin racine où se trouve les dossiers des bases de données.
	 */
	std::filesystem::path chemin_racine() const;

	/**
	 * Retourne le chemin de la base de données courante. Si aucune base de
	 * données n'est sélectionnée, retourne un chemin vide.
	 */
	std::filesystem::path chemin_courant() const;

	/**
	 * Retourne le nom de la base de données courante. Si aucune base de données
	 * n'est sélectionnée, retourne un nom vide.
	 */
	dls::chaine base_courante() const;
};

}  /* namespace arachne */

