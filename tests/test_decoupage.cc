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

#include "test_decoupage.h"

#include <cstring>

#include "decoupage/decoupeuse.h"

template <typename I1, typename I2>
bool verifie_morceaux(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
{
	const auto dist1 = std::distance(debut1, fin1);
	const auto dist2 = std::distance(debut2, fin2);

	if (dist1 != dist2) {
		return false;
	}

	while (debut1 != fin1 && debut2 != fin2) {
		if ((*debut1).identifiant != (*debut2).identifiant) {
			return false;
		}

		if ((*debut1).chaine != (*debut2).chaine) {
			return false;
		}

		++debut1;
		++debut2;
	}

	return true;
}

bool test_decoupage_texte1()
{
	const char *texte = "# Ceci est un commentaire \n"
						"soit str='a';\n"
						"associe nombre {\n"
						"	0...10: imprime(10);\n"
						"	11...20: imprime(20);\n"
						"	sinon:imprime(inconnu);\n"
						"}\n"
						"decoupeuse_texte decoupeuse(str, str + len);\n";

	const DonneesMorceaux donnees_morceaux[] = {
		{ "soit", IDENTIFIANT_SOIT },
		{ "str", IDENTIFIANT_CHAINE_CARACTERE },
		{ "=", IDENTIFIANT_EGAL },
		{ "a", IDENTIFIANT_CARACTERE },
		{ ";", IDENTIFIANT_POINT_VIRGULE },
		{ "associe", IDENTIFIANT_ASSOCIE },
		{ "nombre", IDENTIFIANT_CHAINE_CARACTERE },
		{ "{", IDENTIFIANT_ACCOLADE_OUVRANTE },
		{ "0", IDENTIFIANT_NOMBRE },
		{ "...", IDENTIFIANT_TROIS_POINT },
		{ "10", IDENTIFIANT_NOMBRE },
		{ ":", IDENTIFIANT_DOUBLE_POINT },
		{ "imprime", IDENTIFIANT_IMPRIME },
		{ "(", IDENTIFIANT_PARENTHESE_OUVRANTE },
		{ "10", IDENTIFIANT_NOMBRE },
		{ ")", IDENTIFIANT_PARENTHESE_FERMANTE },
		{ ";", IDENTIFIANT_POINT_VIRGULE },
		{ "11", IDENTIFIANT_NOMBRE },
		{ "...", IDENTIFIANT_TROIS_POINT },
		{ "20", IDENTIFIANT_NOMBRE },
		{ ":", IDENTIFIANT_DOUBLE_POINT },
		{ "imprime", IDENTIFIANT_IMPRIME },
		{ "(", IDENTIFIANT_PARENTHESE_OUVRANTE },
		{ "20", IDENTIFIANT_NOMBRE },
		{ ")", IDENTIFIANT_PARENTHESE_FERMANTE },
		{ ";", IDENTIFIANT_POINT_VIRGULE },
		{ "sinon", IDENTIFIANT_SINON },
		{ ":", IDENTIFIANT_DOUBLE_POINT },
		{ "imprime", IDENTIFIANT_IMPRIME },
		{ "(", IDENTIFIANT_PARENTHESE_OUVRANTE },
		{ "inconnu", IDENTIFIANT_CHAINE_CARACTERE },
		{ ")", IDENTIFIANT_PARENTHESE_FERMANTE },
		{ ";", IDENTIFIANT_POINT_VIRGULE },
		{ "}", IDENTIFIANT_ACCOLADE_FERMANTE },
		{ "decoupeuse_texte", IDENTIFIANT_CHAINE_CARACTERE },
		{ "decoupeuse", IDENTIFIANT_CHAINE_CARACTERE },
		{ "(", IDENTIFIANT_PARENTHESE_OUVRANTE },
		{ "str,", IDENTIFIANT_CHAINE_CARACTERE },
		{ "str", IDENTIFIANT_CHAINE_CARACTERE },
		{ "+", IDENTIFIANT_PLUS },
		{ "len", IDENTIFIANT_CHAINE_CARACTERE },
		{ ")", IDENTIFIANT_PARENTHESE_FERMANTE },
		{ ";", IDENTIFIANT_POINT_VIRGULE }
	};

	const size_t len = std::strlen(texte);

	decoupeuse_texte decoupeuse(texte, texte + len);
	decoupeuse.genere_morceaux();

	return verifie_morceaux(decoupeuse.begin(),
							decoupeuse.end(),
							std::begin(donnees_morceaux),
							std::end(donnees_morceaux));
}

void test_decoupage(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_VERIFIE_CONDITION(controleur, test_decoupage_texte1());
}
