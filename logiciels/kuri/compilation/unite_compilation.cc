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

#include "unite_compilation.hh"

const char *chaine_etat_unite(UniteCompilation::Etat etat)
{
#define ENUMERE_ETAT_UNITE_EX(etat) \
	case UniteCompilation::Etat::etat: return #etat;
	switch (etat) {
		ENUMERE_ETATS_UNITE
	}
#undef ENUMERE_ETAT_UNITE_EX

	return "erreur";
}

std::ostream &operator<<(std::ostream &os, UniteCompilation::Etat etat)
{
	os << chaine_etat_unite(etat);
	return os;
}
