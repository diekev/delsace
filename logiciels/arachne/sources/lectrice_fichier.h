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

#include <experimental/filesystem>

namespace arachne {

class lectrice_fichier {
	FILE *m_pointeur = nullptr;

public:
	lectrice_fichier() = default;

	lectrice_fichier(lectrice_fichier const &autre) = default;
	lectrice_fichier &operator=(lectrice_fichier const &autre) = default;

	explicit lectrice_fichier(const std::experimental::filesystem::path &chemin);

	~lectrice_fichier();

	void ouvre(const std::experimental::filesystem::path &chemin);

	size_t taille() const;

	void ferme();

	size_t lis_tampon(char *tampon, size_t taille) const;

	bool est_ouverte() const;
};

}  /* namespace arachne */
