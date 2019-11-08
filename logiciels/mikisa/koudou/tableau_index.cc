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

#include "tableau_index.hh"

namespace kdo {

tableau_index::tableau_index(int oct)
	: octets(oct)
{}

int tableau_index::octets_pour_taille(long taille)
{
	if (taille <= 256) {
		return 0;
	}

	if (taille <= 65536) {
		return 1;
	}

	return 2;
}

void tableau_index::pousse(int v)
{
	switch (octets) {
		case 0:
		{
			pousse_impl(static_cast<unsigned char>(v));
			break;
		}
		case 1:
		{
			pousse_impl(static_cast<unsigned short>(v));
			break;
		}
		case 2:
		{
			pousse_impl(v);
			break;
		}
	}
}

void tableau_index::pousse_impl(unsigned char v)
{
	assert(octets == 0);
	donnees.pousse(v);
	assert((*this)[taille] >= 0 && (*this)[taille] < 256);
	taille += 1;
}

void tableau_index::pousse_impl(unsigned short v)
{
	assert(octets == 1);
	auto curseur = donnees.taille();
	donnees.redimensionne(curseur + 2);
	*reinterpret_cast<unsigned short *>(&donnees[curseur]) = v;
	assert((*this)[curseur / 2] >= 0 && (*this)[curseur / 2] < 65536);
	taille += 1;
}

void tableau_index::pousse_impl(int v)
{
	assert(octets == 2);
	auto curseur = donnees.taille();
	donnees.redimensionne(curseur + 4);
	*reinterpret_cast<int *>(&donnees[curseur]) = v;
	assert((*this)[curseur / 4] >= 0);
	taille += 1;
}

/* ****************************************************************** */

int tableau_index_comprime::operator[](long idx) const
{
	if (donnees.taille() == 1) {
		return donnees[0].second;
	}

	for (auto i = 0; i < donnees.taille() - 1; ++i) {
		if (donnees[i].first >= idx && donnees[i + 1].first < idx) {
			return donnees[i].second;
		}
	}

	return donnees.back().second;
}

void tableau_index_comprime::pousse(int decalage, int valeur)
{
	donnees.pousse({ decalage, valeur });
}

/* ****************************************************************** */

/**
 * Comprime un tableau_index en utilisant une variation de l'algorithme
 * de codage par plages.
 *
 * Avec l'algorithme normal, la séquence 0000001111224444466666777
 * devrait donner 6:0;4:1;1:2;5:4;5:6;3:7.
 *
 * Or dans notre cas nous n'avons pas besoin de reconstituer la séquence
 * originale : nous voulons simplement savoir à quel index correspond
 * une valeur. Pour ce faire nous stockons non pas le nombre de valeur,
 * mais les décalages depuis le début de la séquence originale où les
 * valeurs commencent. Ainsi nous pouvons utiliser une recherche binaire
 * ou un autre algorithme simple pour trouver ce que nous cherchons.
 *
 * La séquence devient alors O:O;6:1;10:2;12:4;17:6;22:7.
 *
 * Voir https://fr.wikipedia.org/wiki/Run-length_encoding pour la
 * théorie du codage par plages.
 */
tableau_index_comprime comprimes_tableau_index(tableau_index const &entree)
{
	auto sortie = tableau_index_comprime();
	auto decalage = 0;

	for (auto i = 0; i < entree.taille; ++i) {
		auto valeur = entree[i];

		for (auto j = i + 1; j < entree.taille; ++j, ++i) {
			if (entree[j] != valeur) {
				break;
			}
		}

		sortie.pousse(decalage, valeur);
		decalage = i;
	}

	return sortie;
}

}  /* namespace kdo */
