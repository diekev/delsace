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

#include "lectrice_fichier.h"

namespace arachne {

lectrice_fichier::lectrice_fichier(const std::experimental::filesystem::path &chemin)
	: lectrice_fichier()
{
	ouvre(chemin);
}

lectrice_fichier::~lectrice_fichier()
{
	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
	}
}

void lectrice_fichier::ouvre(const std::experimental::filesystem::path &chemin)
{
	auto pointeur = std::fopen(chemin.c_str(), "r");

	if (pointeur == nullptr) {
		std::perror("Ne peut pas ouvrir le fichier");
		return;
	}

	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
	}

	m_pointeur = pointeur;
}

size_t lectrice_fichier::taille() const
{
	std::fseek(m_pointeur, 0ul, SEEK_END);
	auto t = std::ftell(m_pointeur);
	std::fseek(m_pointeur, 0ul, SEEK_SET);
	return static_cast<size_t>(t);
}

void lectrice_fichier::ferme()
{
	if (m_pointeur != nullptr) {
		std::fclose(m_pointeur);
		m_pointeur = nullptr;
	}
}

size_t lectrice_fichier::lis_tampon(char *tampon, size_t taille) const
{
	return std::fread(tampon, sizeof(char), taille, m_pointeur);
}

bool lectrice_fichier::est_ouverte() const
{
	return m_pointeur != nullptr;
}

}  /* namespace arachne */
