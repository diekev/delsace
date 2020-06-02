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

#include "biblinternes/structures/chaine.hh"

namespace dls {
struct flux_chaine;
}

namespace lng::erreur {

template <typename type_erreur>
class frappe {
	dls::chaine m_message;
	type_erreur m_type;

public:
	frappe(const char *message, type_erreur type)
		: m_message(message)
		, m_type(type)
	{}

	type_erreur type() const
	{
		return m_type;
	}

	const char *message() const
	{
		return m_message.c_str();
	}
};

/* Fonctions communes de formattage d'erreurs. */

void imprime_caractere_vide(dls::flux_chaine &os, const long nombre, const dls::vue_chaine &chaine);

void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine);

void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine, long debut, long fin);

void imprime_tilde(dls::flux_chaine &ss, dls::vue_chaine_compacte chaine);

void imprime_ligne_entre(
		dls::flux_chaine &os,
		const dls::vue_chaine &chaine,
		long debut,
		long fin);

}  /* namespace lng::erreur */
