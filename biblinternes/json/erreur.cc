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
#include "biblinternes/structures/flux_chaine.hh"

#include "morceaux.hh"

namespace json {
namespace erreur {

void lance_erreur(const dls::chaine &quoi, lng::tampon_source const &tampon, const DonneesMorceau &morceau, int type)
{
	const auto ligne = static_cast<long>(morceau.ligne_pos >> 32);
	const auto pos_mot = static_cast<long>(morceau.ligne_pos & 0xffffffff);
	const auto identifiant = morceau.identifiant;
	const auto &chaine = morceau.chaine;

	auto ligne_courante = tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

}  /* namespace erreur */
}  /* namespace json */
