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

#include "erreur.hh"

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/langage/unicode.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "morceaux.hh"

namespace erreur {

frappe::frappe(const char *message, int type)
	: m_message(message)
	, m_type(type)
{}

int frappe::type() const
{
	return m_type;
}

const char *frappe::message() const
{
	return m_message.c_str();
}

static void imprime_caractere_vide(dls::flux_chaine &os, const long nombre, const dls::vue_chaine &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0; i < nombre; i += lng::nombre_octets(&chaine[i])) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}
	}
}

static void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine)
{
	for (auto i = 0; i < chaine.taille() - 1; i += lng::nombre_octets(&chaine[i])) {
		os << '~';
	}
}

void lance_erreur(const dls::chaine &quoi, lng::tampon_source const &tampon, const DonneesMorceaux &morceau, int type)
{
	const auto ligne = static_cast<long>(morceau.ligne_pos >> 32);
	const auto pos_mot = static_cast<long>(morceau.ligne_pos & 0xffffffff);
	const auto identifiant = morceau.identifiant;
	const auto &chaine = morceau.chaine;

	auto ligne_courante = tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

}
