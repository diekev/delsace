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
static long trouve_fin_ligne(const char *debut, const char *fin)
{
	long pos = 0;

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

namespace lng {

tampon_source::tampon_source(const char *chaine)
	: m_tampon(chaine)
{
	construit_lignes();
}

tampon_source::tampon_source(dls::chaine chaine) noexcept
	: m_tampon(std::move(chaine))
{
	construit_lignes();
}

tampon_source::tampon_source(const tampon_source &autre)
{
	m_tampon = autre.m_tampon;
	construit_lignes();
}

tampon_source &tampon_source::operator=(const tampon_source &autre)
{
	m_tampon = autre.m_tampon;
	construit_lignes();

	return *this;
}

const char *tampon_source::debut() const noexcept
{
	return &m_tampon[0];
}

const char *tampon_source::fin() const noexcept
{
	/* cppcheck-suppress containerOutOfBoundsIndexExpression */
	return &m_tampon[m_tampon.taille()];
}

dls::vue_chaine tampon_source::operator[](long i) const noexcept
{
	return m_lignes[i];
}

size_t tampon_source::nombre_lignes() const noexcept
{
	return static_cast<size_t>(m_lignes.taille());
}

size_t tampon_source::taille_donnees() const noexcept
{
	return static_cast<size_t>(m_tampon.taille()) * sizeof(char);
}

tampon_source tampon_source::sous_tampon(size_t debut, size_t fin) const
{
	auto pos = m_lignes[static_cast<long>(debut)].begin();
	auto taille = 0l;

	for (auto i = debut; i < fin; ++i) {
		taille += m_lignes[static_cast<long>(i)].taille();
	}

	return tampon_source(dls::chaine(pos, taille));
}

const dls::chaine &tampon_source::chaine() const
{
	return m_tampon;
}

void tampon_source::construit_lignes()
{
	for (auto i = 0l; i < m_tampon.taille();) {
		auto pos = &m_tampon[i];
		auto taille = trouve_fin_ligne(pos, this->fin());

		m_lignes.pousse(dls::vue_chaine{pos, taille});

		i += taille;
	}
}

}  /* namespace lng */
