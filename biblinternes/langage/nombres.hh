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

/**
 * Petite bibliothèque pour parser des nombres.
 */

#include "biblinternes/structures/vue_chaine.hh"

namespace lng {


/**
 * Retourne vrai si le caractère spécifié est un nombre décimal, c'est-à-dire
 * entre 0 et 9.
 */
constexpr bool est_nombre_decimal(char c)
{
	return (c >= '0') && (c <= '9');
}

/* ************************************************************************** */

size_t extrait_nombre_binaire(const char *debut, const char *fin);

size_t extrait_nombre_octal(const char *debut, const char *fin);

size_t extrait_nombre_hexadecimal(const char *debut, const char *fin);

/* ************************************************************************** */

long converti_chaine_nombre_binaire(dls::vue_chaine const &chaine);

long converti_chaine_nombre_octal(dls::vue_chaine const &chaine);

long converti_chaine_nombre_hexadecimal(dls::vue_chaine const &chaine);

long converti_nombre_entier(dls::vue_chaine const &chaine);

double converti_nombre_reel(dls::vue_chaine const &chaine);

/* ************************************************************************** */

template <typename id_morceau>
struct decoupeuse_nombre {
	enum class etat_nombre : char {
		POINT,
		EXPONENTIEL,
		DEBUT,
	};

	static auto extrait_nombre_decimal(const char *debut, const char *fin, id_morceau &id_nombre)
	{
		auto compte = 0ul;
		auto etat = etat_nombre::DEBUT;
		id_nombre = id_morceau::NOMBRE_ENTIER;

		while (debut != fin) {
			if (!est_nombre_decimal(*debut) && (*debut != '_') && *debut != '.') {
				break;
			}

			if (*debut == '.') {
				if ((*(debut + 1) == '.') && (*(debut + 2) == '.')) {
					break;
				}

				if (etat == etat_nombre::POINT) {
					/* À FAIRE : erreur ? */
					break;
				}

				etat = etat_nombre::POINT;
				id_nombre = id_morceau::NOMBRE_REEL;
			}

			++debut;
			++compte;
		}

		return compte;
	}

	/**
	 * Extrait un nombre depuis une chaine de caractère spécifiée par 'debut' et
	 * 'fin'. La chaine est stockée dans 'chaine' et son identifiant dans
	 * 'id_nombre'.
	 *
	 * Retourne le nombre de caractère de la chaine [debut, fin] qui a été consommé.
	 */
	static size_t extrait_nombre(const char *debut, const char *fin, id_morceau &id_nombre)
	{
		if (*debut == '0' && (*(debut + 1) == 'b' || *(debut + 1) == 'B')) {
			id_nombre = id_morceau::NOMBRE_BINAIRE;
			debut += 2;
			return extrait_nombre_binaire(debut, fin) + 2;
		}

		if (*debut == '0' && (*(debut + 1) == 'o' || *(debut + 1) == 'O')) {
			id_nombre = id_morceau::NOMBRE_OCTAL;
			debut += 2;
			return extrait_nombre_octal(debut, fin) + 2;
		}

		if (*debut == '0' && (*(debut + 1) == 'x' || *(debut + 1) == 'X')) {
			id_nombre = id_morceau::NOMBRE_HEXADECIMAL;
			debut += 2;
			return extrait_nombre_hexadecimal(debut, fin) + 2;
		}

		return extrait_nombre_decimal(debut, fin, id_nombre);
	}

	/**
	 * Converti une chaine de caractère en un nombre entier de type 'long'. Si la
	 * chaine de caractère représente un nombre qui ne peut être représenté par un
	 * entier de type 'long' (64-bit), la valeur maximale 0xffffffff est retournée.
	 */
	static long converti_chaine_nombre_entier(dls::vue_chaine const &chaine, id_morceau identifiant)
	{
		switch (identifiant) {
			case id_morceau::NOMBRE_ENTIER:
				return converti_nombre_entier(chaine);
			case id_morceau::NOMBRE_BINAIRE:
				return converti_chaine_nombre_binaire({&chaine[2], chaine.taille() - 2});
			case id_morceau::NOMBRE_OCTAL:
				return converti_chaine_nombre_octal({&chaine[2], chaine.taille() - 2});
			case id_morceau::NOMBRE_HEXADECIMAL:
				return converti_chaine_nombre_hexadecimal({&chaine[2], chaine.taille() - 2});
			default:
				return 0l;
		}
	}

	/**
	 * Converti une chaine de caractère en un nombre réel de type 'double'.
	 */
	static double converti_chaine_nombre_reel(dls::vue_chaine const &chaine, id_morceau identifiant)
	{
		switch (identifiant) {
			case id_morceau::NOMBRE_REEL:
				return converti_nombre_reel(chaine);
			default:
				return 0.0;
		}
	}
};

}  /* namespace lng */
