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

#include "autrice_fichier.h"

namespace arachne {

autrice_fichier::autrice_fichier(const std::filesystem::path &chemin)
	: autrice_fichier()
{
	ouvre(chemin);
}

autrice_fichier::~autrice_fichier()
{
	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
	}
}

void autrice_fichier::ouvre(const std::filesystem::path &chemin)
{
	auto pointeur = std::fopen(chemin.c_str(), "w");

	if (pointeur == nullptr) {
		std::perror("Ne peut pas ouvrir le fichier");
		return;
	}

	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
	}

	m_pointeur = pointeur;
}

void autrice_fichier::ferme()
{
	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
		m_pointeur = nullptr;
	}
}

long autrice_fichier::ecrit_tampon(const char *tampon, long taille) const
{
	return static_cast<long>(std::fwrite(tampon, sizeof(char), static_cast<size_t>(taille), m_pointeur));
}

bool autrice_fichier::est_ouverte() const
{
	return m_pointeur != nullptr;
}

}  /* namespace arachne */
