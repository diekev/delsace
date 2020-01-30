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
#include "compilation/decoupeuse.h"
#include "compilation/modules.hh"

#undef DEBOGUE_MORCEAUX

template <typename I1, typename I2>
bool verifie_morceaux(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
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
soit str='a';
soit str0='\0';
discr nombre {
	0...1_000: imprime(1000);
	11_000...2_0000: imprime(20000);
	sinon:imprime(inconnu);
}
decoupeuse_texte decoupeuse(str, str + len);
)";

	const DonneesLexeme donnees_morceaux[] = {
		{ " ceci est une chaine française avec espaces ", TypeLexeme::CHAINE_LITTERALE },
		{ "\n", TypeLexeme::POINT_VIRGULE },
		{ "ceci est une chaine française sans espaces", TypeLexeme::CHAINE_LITTERALE },
		{ "\n", TypeLexeme::POINT_VIRGULE },
		{ "soit", TypeLexeme::SOIT },
		{ "str", TypeLexeme::CHAINE_CARACTERE },
		{ "=", TypeLexeme::EGAL },
		{ "a", TypeLexeme::CARACTERE },
		{ ";", TypeLexeme::POINT_VIRGULE },
		{ "soit", TypeLexeme::SOIT },
		{ "str0", TypeLexeme::CHAINE_CARACTERE },
		{ "=", TypeLexeme::EGAL },
		{ "\\0", TypeLexeme::CARACTERE },
		{ ";", TypeLexeme::POINT_VIRGULE },
		{ "discr", TypeLexeme::DISCR },
		{ "nombre", TypeLexeme::CHAINE_CARACTERE },
		{ "{", TypeLexeme::ACCOLADE_OUVRANTE },
		{ "0", TypeLexeme::NOMBRE_ENTIER },
		{ "...", TypeLexeme::TROIS_POINTS },
		{ "1_000", TypeLexeme::NOMBRE_ENTIER },
		{ ":", TypeLexeme::DOUBLE_POINTS },
		{ "imprime", TypeLexeme::CHAINE_CARACTERE },
		{ "(", TypeLexeme::PARENTHESE_OUVRANTE },
		{ "1000", TypeLexeme::NOMBRE_ENTIER },
		{ ")", TypeLexeme::PARENTHESE_FERMANTE },
		{ ";", TypeLexeme::POINT_VIRGULE },
		{ "11_000", TypeLexeme::NOMBRE_ENTIER },
		{ "...", TypeLexeme::TROIS_POINTS },
		{ "2_0000", TypeLexeme::NOMBRE_ENTIER },
		{ ":", TypeLexeme::DOUBLE_POINTS },
		{ "imprime", TypeLexeme::CHAINE_CARACTERE },
		{ "(", TypeLexeme::PARENTHESE_OUVRANTE },
		{ "20000", TypeLexeme::NOMBRE_ENTIER },
		{ ")", TypeLexeme::PARENTHESE_FERMANTE },
		{ ";", TypeLexeme::POINT_VIRGULE },
		{ "sinon", TypeLexeme::SINON },
		{ ":", TypeLexeme::DOUBLE_POINTS },
		{ "imprime", TypeLexeme::CHAINE_CARACTERE },
		{ "(", TypeLexeme::PARENTHESE_OUVRANTE },
		{ "inconnu", TypeLexeme::CHAINE_CARACTERE },
		{ ")", TypeLexeme::PARENTHESE_FERMANTE },
		{ ";", TypeLexeme::POINT_VIRGULE },
		{ "}", TypeLexeme::ACCOLADE_FERMANTE },
		{ "decoupeuse_texte", TypeLexeme::CHAINE_CARACTERE },
		{ "decoupeuse", TypeLexeme::CHAINE_CARACTERE },
		{ "(", TypeLexeme::PARENTHESE_OUVRANTE },
		{ "str", TypeLexeme::CHAINE_CARACTERE },
		{ ",", TypeLexeme::VIRGULE },
		{ "str", TypeLexeme::CHAINE_CARACTERE },
		{ "+", TypeLexeme::PLUS },
		{ "len", TypeLexeme::CHAINE_CARACTERE },
		{ ")", TypeLexeme::PARENTHESE_FERMANTE },
		{ ";", TypeLexeme::POINT_VIRGULE }
	};

	auto fichier = Fichier{};
	fichier.tampon = lng::tampon_source(texte);

	decoupeuse_texte decoupeuse(&fichier);
	decoupeuse.genere_morceaux();

	return verifie_morceaux(fichier.morceaux.debut(),
							fichier.morceaux.fin(),
							std::begin(donnees_morceaux),
							std::end(donnees_morceaux));
}

void test_decoupage(dls::test_unitaire::Controleuse &controleuse)
{
	CU_VERIFIE_CONDITION(controleuse, test_decoupage_texte1());
}
