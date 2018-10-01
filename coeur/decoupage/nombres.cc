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

#include "nombres.h"

#include <cmath>

#include "erreur.h"
#include "morceaux.h"

/* Logique de découpage et de conversion de nombres.
 *
 */

enum {
	ETAT_NOMBRE_POINT,
	ETAT_NOMBRE_EXPONENTIEL,
	ETAT_NOMBRE_DEBUT,
};

bool est_nombre_decimal(char c)
{
	return (c >= '0') && (c <= '9');
}

static inline bool est_nombre_binaire(char c)
{
	return (c == '0' || c == '1');
}

static inline bool est_nombre_octal(char c)
{
	return (c >= '0' && c <= '7');
}

static inline bool est_nombre_hexadecimal(char c)
{
	return est_nombre_decimal(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int extrait_nombre_binaire(const char *debut, const char *fin, std::string &chaine)
{
	int compte = 0;
	chaine.clear();

	while (debut != fin) {
		if (*debut != '_') {
			if (!est_nombre_binaire(*debut)) {
				break;
			}

			chaine.push_back(*debut);
		}

		++debut;
		++compte;
	}

	return compte;
}

static int extrait_nombre_octal(const char *debut, const char *fin, std::string &chaine)
{
	int compte = 0;
	chaine.clear();

	while (debut != fin) {
		if (*debut != '_') {
			if (!est_nombre_octal(*debut)) {
				break;
			}

			chaine.push_back(*debut);
		}

		++debut;
		++compte;
	}

	return compte;
}

static int extrait_nombre_hexadecimal(const char *debut, const char *fin, std::string &chaine)
{
	int compte = 0;
	chaine.clear();

	while (debut != fin) {
		if (*debut != '_') {
			if (!est_nombre_hexadecimal(*debut)) {
				break;
			}

			chaine.push_back(*debut);
		}

		++debut;
		++compte;
	}

	return compte;
}

static int extrait_nombre_decimal(const char *debut, const char *fin, std::string &chaine, int &id_nombre)
{
	int compte = 0;
	int etat = ETAT_NOMBRE_DEBUT;
	id_nombre = ID_NOMBRE_ENTIER;

	while (debut != fin) {
		if (*debut != '_') {
			if (!est_nombre_decimal(*debut) && *debut != '.') {
				break;
			}

			if (*debut == '.') {
				if ((*(debut + 1) == '.') && (*(debut + 2) == '.')) {
					break;
				}

				if (etat == ETAT_NOMBRE_POINT) {
					throw erreur::frappe("Erreur ! Le nombre contient un point en trop !\n");
				}

				etat = ETAT_NOMBRE_POINT;
				id_nombre = ID_NOMBRE_REEL;
			}

			chaine.push_back(*debut);
		}

		++debut;
		++compte;
	}

	return compte;
}

int extrait_nombre(const char *debut, const char *fin, std::string &chaine, int &id_nombre)
{
	if (*debut == '0' && (*(debut + 1) == 'b' || *(debut + 1) == 'B')) {
		id_nombre = ID_NOMBRE_BINAIRE;
		debut += 2;
		return extrait_nombre_binaire(debut, fin, chaine) + 2;
	}

	if (*debut == '0' && (*(debut + 1) == 'o' || *(debut + 1) == 'O')) {
		id_nombre = ID_NOMBRE_OCTAL;
		debut += 2;
		return extrait_nombre_octal(debut, fin, chaine) + 2;
	}

	if (*debut == '0' && (*(debut + 1) == 'x' || *(debut + 1) == 'X')) {
		id_nombre = ID_NOMBRE_HEXADECIMAL;
		debut += 2;
		return extrait_nombre_hexadecimal(debut, fin, chaine) + 2;
	}

	return extrait_nombre_decimal(debut, fin, chaine, id_nombre);
}

/* ************************************************************************** */

static long converti_chaine_nombre_binaire(const std::string &chaine)
{
	if (chaine.length() > 64) {
		/* À FAIRE : erreur, surcharge. */
		return std::numeric_limits<long>::max();
	}

	auto resultat = 0l;
	auto n = chaine.length() - 1;

	for (auto c : chaine) {
		resultat |= ((int(c) - int('0')) << n);
		n -= 1;
	}

	return resultat;
}

static long converti_chaine_nombre_octal(const std::string &chaine)
{
	if (chaine.length() > 22 || chaine > "1777777777777777777777") {
		/* À FAIRE : erreur, surcharge. */
		return std::numeric_limits<long>::max();
	}

	auto resultat = 0l;
	auto n = chaine.length() - 1;

	for (auto c : chaine) {
		resultat |= ((int(c) - int('0')) * static_cast<long>(std::pow(8, n)));
		n -= 1;
	}

	return resultat;
}

static long converti_chaine_nombre_hexadecimal(const std::string &chaine)
{
	if (chaine.length() > 16) {
		/* À FAIRE : erreur, surcharge. */
		return std::numeric_limits<long>::max();
	}

	auto resultat = 0l;
	auto n = chaine.length() - 1;

	for (auto c : chaine) {
		if (est_nombre_decimal(c)) {
			resultat |= ((int(c) - int('0')) * static_cast<long>(std::pow(16, n)));
		}
		else {
			c = static_cast<char>(::tolower(c));
			resultat |= ((int(c) - int('a') + 10) * static_cast<long>(std::pow(16, n)));
		}

		n -= 1;
	}

	return resultat;
}

long converti_chaine_nombre_entier(const std::string &chaine, int identifiant)
{
	switch (identifiant) {
		case ID_NOMBRE_ENTIER:
			if (chaine.length() > 19 || chaine > "9223372036854775807") {
				/* À FAIRE : erreur, surcharge. */
				return std::numeric_limits<long>::max();
			}

			return std::atol(chaine.c_str());
		case ID_NOMBRE_BINAIRE:
			return converti_chaine_nombre_binaire(chaine);
		case ID_NOMBRE_OCTAL:
			return converti_chaine_nombre_octal(chaine);
		case ID_NOMBRE_HEXADECIMAL:
			return converti_chaine_nombre_hexadecimal(chaine);
		default:
			return 0l;
	}
}

double converti_chaine_nombre_reel(const std::string &chaine, int identifiant)
{
	switch (identifiant) {
		case ID_NOMBRE_REEL:
			return std::atof(chaine.c_str());
		default:
			return 0.0;
	}
}
