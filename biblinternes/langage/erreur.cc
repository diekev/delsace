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

#include "erreur.hh"

#include "biblinternes/structures/flux_chaine.hh"

#include "unicode.hh"

namespace lng::erreur {

void imprime_caractere_vide(dls::flux_chaine &os, const long nombre, const dls::vue_chaine &chaine)
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

void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine)
{
	for (auto i = 0l; i < chaine.taille() - 1;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine, long debut, long fin)
{
	for (auto i = debut; i < fin;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

void imprime_ligne_entre(dls::flux_chaine &os, const dls::vue_chaine &chaine, long debut, long fin)
{
	for (auto i = debut; i < fin; ++i) {
		os << chaine[i];
	}
}

void imprime_tilde(dls::flux_chaine &ss, dls::vue_chaine_compacte chaine)
{
	imprime_tilde(ss, dls::vue_chaine(chaine.pointeur(), chaine.taille()));
}

}  /* namespace lng::erreur */
