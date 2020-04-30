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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "structures.hh"

bool kuri::operator ==(const kuri::chaine &chn1, const kuri::chaine &chn2)
{
	if (chn1.taille != chn2.taille) {
		return false;
	}

	if (chn1.taille == 0) {
		return true;
	}

	for (auto i = 0; i < chn1.taille; ++i) {
		if (chn1[i] != chn2[i]) {
			return false;
		}
	}

	return true;
}

bool kuri::operator !=(const kuri::chaine &chn1, const kuri::chaine &chn2)
{
	return !(chn1 == chn2);
}

std::ostream &kuri::operator<<(std::ostream &os, const kuri::chaine &chn)
{
	POUR (chn) {
		os << it;
	}

	return os;
}

kuri::chaine kuri::copie_chaine(kuri::chaine const &autre)
{
	chaine resultat;
	resultat.taille = autre.taille;
	resultat.pointeur = memoire::loge_tableau<char>("chaine", resultat.taille);

	for (auto i = 0; i < autre.taille; ++i) {
		resultat.pointeur[i] = autre.pointeur[i];
	}

	return resultat;
}

void kuri::detruit_chaine(kuri::chaine &chn)
{
	memoire::deloge_tableau("chaine", chn.pointeur, chn.taille);
}
