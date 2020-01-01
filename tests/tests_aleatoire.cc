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

#include <iostream>

#include "biblinternes/tests/test_aleatoire.hh"

#include "biblinternes/base64/base64.hh"
#include "biblinternes/sha256/sha256.hh"

int test_aleatoire_base64_encode(const u_char *donnees, size_t taille)
{
	base64::encode(donnees, taille);
	base64::encode_pour_url(donnees, taille);

	return 0;
}

int test_aleatoire_base64_decode(const u_char *donnees, long taille)
{
	dls::chaine chaine(reinterpret_cast<const char *>(donnees), taille);
	base64::decode(chaine);
	base64::decode_pour_url(chaine);

	return 0;
}

int test_aleatoire_sha256(const u_char *donnees, long taille)
{
	dls::chaine chaine(reinterpret_cast<const char *>(donnees), taille);
	sha256::empreinte(chaine);

	return 0;
}

int main()
{
	dls::test_aleatoire::Testeuse testeur;
	testeur.ajoute_tests("base64_encode", nullptr, test_aleatoire_base64_encode);
	testeur.ajoute_tests("base64_decode", nullptr, test_aleatoire_base64_decode);
	testeur.ajoute_tests("sha256", nullptr, test_aleatoire_sha256);

	return testeur.performe_tests(std::cerr);
}
