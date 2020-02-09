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

#include "compilation/contexte_generation_code.h"  // pour DonneesModule
#include "compilation/lexeuse.hh"
#include "compilation/modules.hh"

#undef DEBOGUE_MORCEAUX

template <typename I1, typename I2>
bool verifie_lexemes(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
{
	auto const dist1 = std::distance(debut1, fin1);
	auto const dist2 = std::distance(debut2, fin2);

	if (dist1 != dist2) {
#ifdef DEBOGUE_MORCEAUX
		std::cerr << "Les distances ne correspondent pas : "
				  << dist1 << " vs " << dist2 << '\n';
#endif
		return false;
	}

	while (debut1 != fin1 && debut2 != fin2) {
		if ((*debut1).genre != (*debut2).genre) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les identifiants ne correspondent pas : "
					  << chaine_identifiant((*debut1).genre)
					  << " vs "
					  << chaine_identifiant((*debut2).genre) << '\n';
#endif
			return false;
		}

		if ((*debut1).chaine != (*debut2).chaine) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les chaines ne correspondent pas : "
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
« ceci est une chaine française avec espaces »
«ceci est une chaine française sans espaces»
str='a';
str0='\0';
discr nombre {
	0...1_000: imprime(1000);
	11_000...2_0000: imprime(20000);
	sinon:imprime(inconnu);
}
Lexeuse lexeuse(str, str + len);
)";

	const DonneesLexeme donnees_lexemes[] = {
		{ " ceci est une chaine française avec espaces ", GenreLexeme::CHAINE_LITTERALE },
		{ "\n", GenreLexeme::POINT_VIRGULE },
		{ "ceci est une chaine française sans espaces", GenreLexeme::CHAINE_LITTERALE },
		{ "\n", GenreLexeme::POINT_VIRGULE },
		{ "str", GenreLexeme::CHAINE_CARACTERE },
		{ "=", GenreLexeme::EGAL },
		{ "a", GenreLexeme::CARACTERE },
		{ ";", GenreLexeme::POINT_VIRGULE },
		{ "str0", GenreLexeme::CHAINE_CARACTERE },
		{ "=", GenreLexeme::EGAL },
		{ "\\0", GenreLexeme::CARACTERE },
		{ ";", GenreLexeme::POINT_VIRGULE },
		{ "discr", GenreLexeme::DISCR },
		{ "nombre", GenreLexeme::CHAINE_CARACTERE },
		{ "{", GenreLexeme::ACCOLADE_OUVRANTE },
		{ "0", GenreLexeme::NOMBRE_ENTIER },
		{ "...", GenreLexeme::TROIS_POINTS },
		{ "1_000", GenreLexeme::NOMBRE_ENTIER },
		{ ":", GenreLexeme::DOUBLE_POINTS },
		{ "imprime", GenreLexeme::CHAINE_CARACTERE },
		{ "(", GenreLexeme::PARENTHESE_OUVRANTE },
		{ "1000", GenreLexeme::NOMBRE_ENTIER },
		{ ")", GenreLexeme::PARENTHESE_FERMANTE },
		{ ";", GenreLexeme::POINT_VIRGULE },
		{ "11_000", GenreLexeme::NOMBRE_ENTIER },
		{ "...", GenreLexeme::TROIS_POINTS },
		{ "2_0000", GenreLexeme::NOMBRE_ENTIER },
		{ ":", GenreLexeme::DOUBLE_POINTS },
		{ "imprime", GenreLexeme::CHAINE_CARACTERE },
		{ "(", GenreLexeme::PARENTHESE_OUVRANTE },
		{ "20000", GenreLexeme::NOMBRE_ENTIER },
		{ ")", GenreLexeme::PARENTHESE_FERMANTE },
		{ ";", GenreLexeme::POINT_VIRGULE },
		{ "sinon", GenreLexeme::SINON },
		{ ":", GenreLexeme::DOUBLE_POINTS },
		{ "imprime", GenreLexeme::CHAINE_CARACTERE },
		{ "(", GenreLexeme::PARENTHESE_OUVRANTE },
		{ "inconnu", GenreLexeme::CHAINE_CARACTERE },
		{ ")", GenreLexeme::PARENTHESE_FERMANTE },
		{ ";", GenreLexeme::POINT_VIRGULE },
		{ "}", GenreLexeme::ACCOLADE_FERMANTE },
		{ "lexeuse_texte", GenreLexeme::CHAINE_CARACTERE },
		{ "lexeuse", GenreLexeme::CHAINE_CARACTERE },
		{ "(", GenreLexeme::PARENTHESE_OUVRANTE },
		{ "str", GenreLexeme::CHAINE_CARACTERE },
		{ ",", GenreLexeme::VIRGULE },
		{ "str", GenreLexeme::CHAINE_CARACTERE },
		{ "+", GenreLexeme::PLUS },
		{ "len", GenreLexeme::CHAINE_CARACTERE },
		{ ")", GenreLexeme::PARENTHESE_FERMANTE },
		{ ";", GenreLexeme::POINT_VIRGULE }
	};

	auto fichier = Fichier{};
	fichier.tampon = lng::tampon_source(texte);

	Lexeuse lexeuse(&fichier);
	lexeuse.performe_lexage();

	return verifie_lexemes(fichier.lexemes.debut(),
							fichier.lexemes.fin(),
							std::begin(donnees_lexemes),
							std::end(donnees_lexemes));
}

void test_decoupage(dls::test_unitaire::Controleuse &controleuse)
{
	CU_VERIFIE_CONDITION(controleuse, test_decoupage_texte1());
}
