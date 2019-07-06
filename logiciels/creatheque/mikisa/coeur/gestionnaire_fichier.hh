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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <any>
#include <fstream>
#include <mutex>
#include <unordered_map>

using type_fonction_lecture = void(*)(std::istream &, std::any const&);
using type_fonction_ecriture = void(*)(std::ostream &, std::any const&);
using type_fonction_lecture_chemin = void(*)(const char *, std::any const&);
using type_fonction_ecriture_chemin = void(*)(const char *, std::any const&);

struct PoigneeFichier {
private:
	std::mutex m_mutex{};
	std::string m_chemin{};

public:
	explicit PoigneeFichier(std::string const &chemin);

	void lecture(type_fonction_lecture &&fonction, std::any const &donnees);

	/* certaines bibliothèques ne supportent pas les flux C++ par défaut, donc
	 * passe le chemin à la fonction de rappel */
	void lecture_chemin(type_fonction_lecture_chemin &&fonction, std::any const &donnees);

	void ecriture(type_fonction_ecriture &&fonction, std::any const &donnees);

	/* certaines bibliothèques ne supportent pas les flux C++ par défaut, donc
	 * passe le chemin à la fonction de rappel */
	void ecriture_chemin(type_fonction_lecture_chemin &&fonction, std::any const &donnees);
};

class GestionnaireFichier {
	std::unordered_map<std::string, PoigneeFichier *> m_table{};

public:
	~GestionnaireFichier();

	PoigneeFichier *poignee_fichier(std::string const &chemin);
};
