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

/* Logique de découpage et de conversion de nombres.
 *
 */

enum class etat_nombre : char {
	POINT,
	EXPONENTIEL,
	DEBUT,
};

static inline constexpr bool est_nombre_binaire(char c)
{
	return (c == '0' || c == '1') || (c == '_');
}

static inline constexpr bool est_nombre_octal(char c)
{
	return (c >= '0' && c <= '7') || (c == '_');
}

static inline constexpr bool est_nombre_hexadecimal(char c)
{
	return est_nombre_decimal(c)
			|| (c >= 'a' && c <= 'f')
			|| (c >= 'A' && c <= 'F')
			 || (c == '_');
}

static auto extrait_nombre_binaire(const char *debut, const char *fin)
{
	auto compte = 0ul;

	while (debut != fin) {
		if (!est_nombre_binaire(*debut)) {
			break;
		}

		++debut;
		++compte;
	}

	return compte;
}

static auto extrait_nombre_octal(const char *debut, const char *fin)
{
	auto compte = 0ul;

	while (debut != fin) {
		if (!est_nombre_octal(*debut)) {
			break;
		}

		++debut;
		++compte;
	}

	return compte;
}

static auto extrait_nombre_hexadecimal(const char *debut, const char *fin)
{
	auto compte = 0ul;

	while (debut != fin) {
		if (!est_nombre_hexadecimal(*debut)) {
			break;
		}

		++debut;
		++compte;
	}

	return compte;
}

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
				throw erreur::frappe("Erreur ! Le nombre contient un point en trop !\n", erreur::type_erreur::DECOUPAGE);
			}

			etat = etat_nombre::POINT;
			id_nombre = id_morceau::NOMBRE_REEL;
		}

		++debut;
		++compte;
	}

	return compte;
}

size_t extrait_nombre(const char *debut, const char *fin, id_morceau &id_nombre)
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

/* ************************************************************************** */

static long converti_chaine_nombre_binaire(const std::string_view &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (size_t i = chaine.size() - 1; i != -1ul; --i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		resultat |= ((int(c) - int('0')) << n);
		n += 1;

		if (n > 64) {
			/* À FAIRE : erreur, surcharge. */
			return std::numeric_limits<long>::max();
		}
	}

	return resultat;
}

static long converti_chaine_nombre_octal(const std::string_view &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (size_t i = chaine.size() - 1; i != -1ul; --i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		resultat |= ((int(c) - int('0')) * static_cast<long>(std::pow(8, n)));
		n += 1;

		if (n > 22 /*|| chaine > "1777777777777777777777"*/) {
			/* À FAIRE : erreur, surcharge. */
			return std::numeric_limits<long>::max();
		}
	}

	return resultat;
}

static long converti_chaine_nombre_hexadecimal(const std::string_view &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (size_t i = chaine.size() - 1; i != -1ul; --i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (est_nombre_decimal(c)) {
			resultat |= ((int(c) - int('0')) * static_cast<long>(std::pow(16, n)));
		}
		else {
			c = static_cast<char>(::tolower(c));
			resultat |= ((int(c) - int('a') + 10) * static_cast<long>(std::pow(16, n)));
		}

		n += 1;

		if (n > 16) {
			/* À FAIRE : erreur, surcharge. */
			return std::numeric_limits<long>::max();
		}
	}

	return resultat;
}

static auto converti_nombre_entier(const std::string_view &chaine)
{
	long valeur = 0l;
	size_t i = 0;

	for (; i < chaine.size(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur = valeur * 10 + static_cast<long>(c - '0');

		if (i > 19 /*|| chaine > "9223372036854775807"*/) {
			/* À FAIRE : erreur, surcharge. */
			return std::numeric_limits<long>::max();
		}
	}

	return valeur;
}

long converti_chaine_nombre_entier(const std::string_view &chaine, id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::NOMBRE_ENTIER:
			return converti_nombre_entier(chaine);
		case id_morceau::NOMBRE_BINAIRE:
			return converti_chaine_nombre_binaire({&chaine[2], chaine.size() - 2});
		case id_morceau::NOMBRE_OCTAL:
			return converti_chaine_nombre_octal({&chaine[2], chaine.size() - 2});
		case id_morceau::NOMBRE_HEXADECIMAL:
			return converti_chaine_nombre_hexadecimal({&chaine[2], chaine.size() - 2});
		default:
			return 0l;
	}
}

static auto converti_nombre_reel(const std::string_view &chaine)
{
	double valeur = 0.0;
	size_t i = 0;

	/* avant point */
	for (; i < chaine.size(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (c == '.') {
			++i;
			break;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur = valeur * 10.0 + static_cast<double>(c - '0');
	}

	/* après point */
	auto dividende = 10.0;

	for (; i < chaine.size(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur += static_cast<double>(c - '0') / dividende;
		dividende *= 10.0;
	}

	return valeur;
}

double converti_chaine_nombre_reel(const std::string_view &chaine, id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::NOMBRE_REEL:
			return converti_nombre_reel(chaine);
		default:
			return 0.0;
	}
}
