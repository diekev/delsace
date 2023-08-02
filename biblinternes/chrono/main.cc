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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "chronometrage.hh"
#include "chronometre_de_portee.hh"
#include "pointeur_chronometre.hh"

#include "biblinternes/outils/numerique.hh"

#include <iostream>

/* ************************************************************************** */

class Dormeur {
public:
	void dors_trois_secondes()
	{
		dls::chrono::dors_millisecondes(3000);
	}

	void dors_une_seconde()
	{
		dls::chrono::dors_millisecondes(1000);
	}
};

static void test_pointeur_chrono(std::ostream &os)
{
	CHRONOMETRE_PORTEE(__func__, os);

	Dormeur dormeur;

	dls::chrono::pointeur_chronometre<Dormeur> chrono_dormeur(&dormeur, os);

	IMPRIME_PUIS_EXECUTE(os, chrono_dormeur->dors_trois_secondes());
	IMPRIME_PUIS_EXECUTE(os, chrono_dormeur->dors_une_seconde());
}

/* ************************************************************************** */

static void test_char_vers_int(std::ostream &os, const char *nombre)
{
	using namespace dls::chrono;

	auto char_vers_int = [=]()
	{
		const char *n = nombre;

		static unsigned table[] = {
			100000000UL,
			10000000UL,
			1000000UL,
			100000UL,
			10000UL,
			1000UL,
			100UL,
			10UL,
			1UL,
		};

		unsigned resultat = 0;
		unsigned i = 0;

		for (; *n != '\0'; ++n) {
			resultat += table[i++] * static_cast<unsigned>(*n - '0');
		}

		return resultat;
	};

	os << "Nombre : " << char_vers_int() << '\n';

	os << "Temps d'exécution : "
	   << chronometre_boucle(1000000, false, char_vers_int) << '\n';

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, char_vers_int) << '\n';
}

/* ************************************************************************** */

static void test_compte_chiffre(std::ostream &os, uint64_t nombre)
{
	using namespace dls::chrono;

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, dls::num::nombre_chiffre_base_10, nombre)
	   << '\n';

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, dls::num::nombre_chiffre_base_10_opt, nombre)
	   << '\n';

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, dls::num::nombre_chiffre_base_10_pro, nombre)
	   << '\n';
}

/* ************************************************************************** */

static void test_nombre_vers_ascii(std::ostream &os, uint64_t nombre)
{
	using namespace dls::chrono;

	char tampon[32] = { 0 };

	auto nombre_vers_ascii = [&](uint64_t valeur)
	{
		auto dst = tampon;
		auto debut = dst;

		do {
			*dst++ = static_cast<char>('0' + (valeur % 10));
			valeur /= 10;
		} while (valeur != 0);

		const auto resultat = static_cast<uint32_t>(dst - debut);

		for (dst--; dst > debut; debut++, dst--) {
			std::iter_swap(dst, debut);
		}

		return resultat;
	};

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, nombre_vers_ascii, nombre)
	   << '\n';

	auto nombre_vers_ascii_opt = [&](uint64_t v)
	{
		char *dst = tampon;

		const uint32_t resultat = dls::num::nombre_chiffre_base_10_opt(v);
		uint32_t pos = resultat - 1;

		while (v >= 10) {
			auto const q = v / 10;
			auto const r = static_cast<uint32_t>(v % 10);

			dst[pos--] = static_cast<char>('0' + r);
			v = q;
		}

		/* Le dernier chiffre est aisément géré. */
		*dst = static_cast<char>(static_cast<uint32_t>(v) + '0');

		return resultat;
	};

	os << "Temps d'exécution : "
	   << chronometre_boucle_epoque(1000000, 1000, false, nombre_vers_ascii_opt, nombre)
	   << '\n';
}

/* ************************************************************************** */

int main(int argc, char **argv)
{
	const uint64_t nombre = (argc > 1) ? 94653182745978U : 94658789654321U;
	std::ostream &os = std::cerr;

	static constexpr auto test = 0;

	switch (test) {
		case 0:
			test_pointeur_chrono(os);
			break;
		case 1:
			test_char_vers_int(os, argv[1]);
			break;
		case 2:
			test_compte_chiffre(os, nombre);
			break;
		case 3:
			test_nombre_vers_ascii(os, nombre);
			break;
	}

	return 0;
}
