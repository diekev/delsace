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

#include "tampon_source.hh"

/* ************************************************************************** */

/**
 * Retourne la position de la fin de la prochaine ligne (caractère '\\n') dans
 * la chaîne délimitée par 'debut' et 'fin'.
 */
static size_t trouve_fin_ligne(const char *debut, const char *fin)
{
	size_t pos = 0;

	while (debut != fin) {
		++pos;

		if (*debut == '\n') {
			break;
		}

		++debut;
	}

	return pos;
}

/* ************************************************************************** */

TamponSource::TamponSource(const char *chaine)
	: m_tampon(chaine)
{
	construit_lignes();
}

TamponSource::TamponSource(std::string chaine) noexcept
	: m_tampon(std::move(chaine))
{
	construit_lignes();
}

const char *TamponSource::debut() const noexcept
{
	return &m_tampon[0];
}

const char *TamponSource::fin() const noexcept
{
	return &m_tampon[m_tampon.size()];
}

std::string_view TamponSource::operator[](size_t i) const noexcept
{
	return m_lignes[i];
}

size_t TamponSource::nombre_lignes() const noexcept
{
	return m_lignes.size();
}

size_t TamponSource::taille_donnees() const noexcept
{
	return m_tampon.size() * sizeof(std::string_view::value_type);
}

void TamponSource::construit_lignes()
{
	for (size_t i = 0; i < m_tampon.size();) {
		auto d = &m_tampon[i];
		auto taille = trouve_fin_ligne(&m_tampon[i], &m_tampon[m_tampon.size()]);

		m_lignes.push_back(std::string_view{d, taille});

		i += taille;
	}
}
