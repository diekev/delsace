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

#undef DEBOGUE_MORCEAUX

template <typename I1, typename I2>
bool verifie_morceaux(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
{
	const auto dist1 = std::distance(debut1, fin1);
	const auto dist2 = std::distance(debut2, fin2);

	if (dist1 != dist2) {
#ifdef DEBOGUE_MORCEAUX
		std::cerr << "Les distances ne correspondent pas : "
				  << dist1 << " vs " << dist2 << '\n';
#endif
		return false;
	}

	while (debut1 != fin1 && debut2 != fin2) {
		if ((*debut1).identifiant != (*debut2).identifiant) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les identifiants ne correspondent pas : "
					  << chaine_identifiant((*debut1).identifiant)
					  << " vs "
					  << chaine_identifiant((*debut2).identifiant) << '\n';
#endif
			return false;
		}

		if ((*debut1).chaine != (*debut2).chaine) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les chaînes ne correspondent pas : "
					  << (*debut1).chaine
					  << " vs "
					  << (*debut2).chaine << '\n';
#endif
			return false;
		}

		++debut1;
		++debut2;
	}

	return true;
}

bool test_decoupage_texte1()
{
	const char *texte =
R"(# Ceci est un commentaire
soit str='a';
soit str0='\0';
associe nombre {
	0...1_000: imprime(1000);
	11_000...2_0000: imprime(20000);
	sinon:imprime(inconnu);
}
decoupeuse_texte decoupeuse(str, str + len);
)";

	const DonneesMorceaux donnees_morceaux[] = {
		{ "soit", 0ul, ID_SOIT },
		{ "str", 0ul, ID_CHAINE_CARACTERE },
		{ "=", 0ul, ID_EGAL },
		{ "a", 0ul, ID_CARACTERE },
		{ ";", 0ul, ID_POINT_VIRGULE },
		{ "soit", 0ul, ID_SOIT },
		{ "str0", 0ul, ID_CHAINE_CARACTERE },
		{ "=", 0ul, ID_EGAL },
		{ "\\0", 0ul, ID_CARACTERE },
		{ ";", 0ul, ID_POINT_VIRGULE },
		{ "associe", 0ul, ID_ASSOCIE },
		{ "nombre", 0ul, ID_CHAINE_CARACTERE },
		{ "{", 0ul, ID_ACCOLADE_OUVRANTE },
		{ "0", 0ul, ID_NOMBRE_ENTIER },
		{ "...", 0ul, ID_TROIS_POINTS },
		{ "1_000", 0ul, ID_NOMBRE_ENTIER },
		{ ":", 0ul, ID_DOUBLE_POINTS },
		{ "imprime", 0ul, ID_CHAINE_CARACTERE },
		{ "(", 0ul, ID_PARENTHESE_OUVRANTE },
		{ "1000", 0ul, ID_NOMBRE_ENTIER },
		{ ")", 0ul, ID_PARENTHESE_FERMANTE },
		{ ";", 0ul, ID_POINT_VIRGULE },
		{ "11_000", 0ul, ID_NOMBRE_ENTIER },
		{ "...", 0ul, ID_TROIS_POINTS },
		{ "2_0000", 0ul, ID_NOMBRE_ENTIER },
		{ ":", 0ul, ID_DOUBLE_POINTS },
		{ "imprime", 0ul, ID_CHAINE_CARACTERE },
		{ "(", 0ul, ID_PARENTHESE_OUVRANTE },
		{ "20000", 0ul, ID_NOMBRE_ENTIER },
		{ ")", 0ul, ID_PARENTHESE_FERMANTE },
		{ ";", 0ul, ID_POINT_VIRGULE },
		{ "sinon", 0ul, ID_SINON },
		{ ":", 0ul, ID_DOUBLE_POINTS },
		{ "imprime", 0ul, ID_CHAINE_CARACTERE },
		{ "(", 0ul, ID_PARENTHESE_OUVRANTE },
		{ "inconnu", 0ul, ID_CHAINE_CARACTERE },
		{ ")", 0ul, ID_PARENTHESE_FERMANTE },
		{ ";", 0ul, ID_POINT_VIRGULE },
		{ "}", 0ul, ID_ACCOLADE_FERMANTE },
		{ "decoupeuse_texte", 0ul, ID_CHAINE_CARACTERE },
		{ "decoupeuse", 0ul, ID_CHAINE_CARACTERE },
		{ "(", 0ul, ID_PARENTHESE_OUVRANTE },
		{ "str", 0ul, ID_CHAINE_CARACTERE },
		{ ",", 0ul, ID_VIRGULE },
		{ "str", 0ul, ID_CHAINE_CARACTERE },
		{ "+", 0ul, ID_PLUS },
		{ "len", 0ul, ID_CHAINE_CARACTERE },
		{ ")", 0ul, ID_PARENTHESE_FERMANTE },
		{ ";", 0ul, ID_POINT_VIRGULE }
	};

	auto tampon = TamponSource(texte);

	decoupeuse_texte decoupeuse(tampon);
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
