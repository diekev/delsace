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

#include "erreur.h"

#include <sstream>

#include "morceaux.h"
#include "tampon_source.h"
#include "unicode.h"

namespace erreur {

frappe::frappe(const char *message)
	: m_message(message)
{}

const char *frappe::message() const
{
	return m_message.c_str();
}

void lance_erreur(const std::string &quoi, const TamponSource &tampon, const DonneesMorceaux &morceau)
{
	const auto ligne = morceau.ligne;
	const auto pos_mot = morceau.pos;
	const auto identifiant = morceau.identifiant;
	const auto &chaine = morceau.chaine;

	auto ligne_courante = tampon[ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < pos_mot; i += static_cast<size_t>(nombre_octets(&ligne_courante[i]))) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}
	}

	ss << '^';

	for (size_t i = 0; i < chaine.size() - 1; ++i) {
		ss << '~';
	}

	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.str().c_str());
}

}
