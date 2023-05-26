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

#include "unicode.hh"

namespace lng::erreur {

template <typename TypeErreur, typename TypeChaine>
class frappe {
	TypeChaine m_message;
	TypeErreur m_type;

public:
	frappe(const TypeChaine &message, TypeErreur type)
		: m_message(message)
		, m_type(type)
	{}

	frappe(TypeChaine &&message, TypeErreur type)
		: m_message(std::move(message))
		, m_type(type)
	{}

	TypeErreur type() const
	{
		return m_type;
	}

	const TypeChaine &message() const
	{
		return m_message;
	}
};

/* Fonctions communes de formattage d'erreurs. */
template <typename Flux>
void imprime_caractere_vide(Flux &os, const int64_t nombre, const dls::vue_chaine &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0l; i < std::min(nombre, chaine.taille());) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}

		i += lng::decalage_pour_caractere(chaine, i);
	}
}

template <typename Flux>
void imprime_tilde(Flux &os, const dls::vue_chaine &chaine)
{
    for (auto i = int64_t(0); i < chaine.taille() - 1;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

template <typename Flux>
void imprime_tilde(Flux &os, const dls::vue_chaine &chaine, int64_t debut, int64_t fin)
{
	for (auto i = debut; i < fin;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

template <typename Flux>
void imprime_ligne_entre(Flux &os, const dls::vue_chaine &chaine, int64_t debut, int64_t fin)
{
	for (auto i = debut; i < fin; ++i) {
		os << chaine[i];
	}
}

template <typename Flux>
void imprime_tilde(Flux &ss, dls::vue_chaine_compacte chaine)
{
	imprime_tilde(ss, dls::vue_chaine(chaine.pointeur(), chaine.taille()));
}

}  /* namespace lng::erreur */
