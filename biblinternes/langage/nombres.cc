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

#include "nombres.hh"

#include <cmath>
#include <cctype>

namespace lng {

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

/* ************************************************************************** */

size_t extrait_nombre_binaire(const char *debut, const char *fin)
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

size_t extrait_nombre_octal(const char *debut, const char *fin)
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

size_t extrait_nombre_hexadecimal(const char *debut, const char *fin)
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

/* ************************************************************************** */

long converti_chaine_nombre_binaire(dls::vue_chaine const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_chaine_nombre_octal(dls::vue_chaine const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_chaine_nombre_hexadecimal(dls::vue_chaine const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_nombre_entier(dls::vue_chaine const &chaine)
{
	long valeur = 0l;
	auto i = 0l;

	for (; i < chaine.taille(); ++i) {
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

double converti_nombre_reel(dls::vue_chaine const &chaine)
{
	double valeur = 0.0;
	auto i = 0l;

	/* avant point */
	for (; i < chaine.taille(); ++i) {
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

	for (; i < chaine.taille(); ++i) {
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


/* ************************************************************************** */

long converti_chaine_nombre_binaire(dls::vue_chaine_compacte const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_chaine_nombre_octal(dls::vue_chaine_compacte const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_chaine_nombre_hexadecimal(dls::vue_chaine_compacte const &chaine)
{
	auto resultat = 0l;
	auto n = 0;

	for (auto i = chaine.taille() - 1; i != -1l; --i) {
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

long converti_nombre_entier(dls::vue_chaine_compacte const &chaine)
{
	long valeur = 0l;
	auto i = 0l;

	for (; i < chaine.taille(); ++i) {
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

double converti_nombre_reel(dls::vue_chaine_compacte const &chaine)
{
	double valeur = 0.0;
	auto i = 0l;

	/* avant point */
	for (; i < chaine.taille(); ++i) {
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

	for (; i < chaine.taille(); ++i) {
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

}  /* namespace lng */
