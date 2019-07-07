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

#include "tampon_source.h"

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

TamponSource::TamponSource(const char *chaine)
	: m_tampon(chaine)
{
	construit_lignes();
}

TamponSource::TamponSource(dls::chaine chaine) noexcept
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
	return &m_tampon[m_tampon.taille()];
}

dls::vue_chaine TamponSource::operator[](long i) const noexcept
{
	return m_lignes[i];
}

size_t TamponSource::nombre_lignes() const noexcept
{
	return static_cast<size_t>(m_lignes.taille());
}

size_t TamponSource::taille_donnees() const noexcept
{
	return static_cast<size_t>(m_tampon.taille()) * sizeof(char);
}

void TamponSource::construit_lignes()
{
	for (auto i = 0l; i < m_tampon.taille();) {
		auto const d = &m_tampon[i];
		auto const t = trouve_fin_ligne(&m_tampon[i], &m_tampon[m_tampon.taille()]);

		m_lignes.pousse(dls::vue_chaine{d, t});

		i += t;
	}
}
