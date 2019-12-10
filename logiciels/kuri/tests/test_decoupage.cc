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
associe nombre {
	0...1_000: imprime(1000);
	11_000...2_0000: imprime(20000);
	sinon:imprime(inconnu);
}
decoupeuse_texte decoupeuse(str, str + len);
)";

	const DonneesMorceau donnees_morceaux[] = {
		{ " ceci est une chaine française avec espaces ", id_morceau::CHAINE_LITTERALE },
		{ "\n", id_morceau::POINT_VIRGULE },
		{ "ceci est une chaine française sans espaces", id_morceau::CHAINE_LITTERALE },
		{ "\n", id_morceau::POINT_VIRGULE },
		{ "soit", id_morceau::SOIT },
		{ "str", id_morceau::CHAINE_CARACTERE },
		{ "=", id_morceau::EGAL },
		{ "a", id_morceau::CARACTERE },
		{ ";", id_morceau::POINT_VIRGULE },
		{ "soit", id_morceau::SOIT },
		{ "str0", id_morceau::CHAINE_CARACTERE },
		{ "=", id_morceau::EGAL },
		{ "\\0", id_morceau::CARACTERE },
		{ ";", id_morceau::POINT_VIRGULE },
		{ "associe", id_morceau::ASSOCIE },
		{ "nombre", id_morceau::CHAINE_CARACTERE },
		{ "{", id_morceau::ACCOLADE_OUVRANTE },
		{ "0", id_morceau::NOMBRE_ENTIER },
		{ "...", id_morceau::TROIS_POINTS },
		{ "1_000", id_morceau::NOMBRE_ENTIER },
		{ ":", id_morceau::DOUBLE_POINTS },
		{ "imprime", id_morceau::CHAINE_CARACTERE },
		{ "(", id_morceau::PARENTHESE_OUVRANTE },
		{ "1000", id_morceau::NOMBRE_ENTIER },
		{ ")", id_morceau::PARENTHESE_FERMANTE },
		{ ";", id_morceau::POINT_VIRGULE },
		{ "11_000", id_morceau::NOMBRE_ENTIER },
		{ "...", id_morceau::TROIS_POINTS },
		{ "2_0000", id_morceau::NOMBRE_ENTIER },
		{ ":", id_morceau::DOUBLE_POINTS },
		{ "imprime", id_morceau::CHAINE_CARACTERE },
		{ "(", id_morceau::PARENTHESE_OUVRANTE },
		{ "20000", id_morceau::NOMBRE_ENTIER },
		{ ")", id_morceau::PARENTHESE_FERMANTE },
		{ ";", id_morceau::POINT_VIRGULE },
		{ "sinon", id_morceau::SINON },
		{ ":", id_morceau::DOUBLE_POINTS },
		{ "imprime", id_morceau::CHAINE_CARACTERE },
		{ "(", id_morceau::PARENTHESE_OUVRANTE },
		{ "inconnu", id_morceau::CHAINE_CARACTERE },
		{ ")", id_morceau::PARENTHESE_FERMANTE },
		{ ";", id_morceau::POINT_VIRGULE },
		{ "}", id_morceau::ACCOLADE_FERMANTE },
		{ "decoupeuse_texte", id_morceau::CHAINE_CARACTERE },
		{ "decoupeuse", id_morceau::CHAINE_CARACTERE },
		{ "(", id_morceau::PARENTHESE_OUVRANTE },
		{ "str", id_morceau::CHAINE_CARACTERE },
		{ ",", id_morceau::VIRGULE },
		{ "str", id_morceau::CHAINE_CARACTERE },
		{ "+", id_morceau::PLUS },
		{ "len", id_morceau::CHAINE_CARACTERE },
		{ ")", id_morceau::PARENTHESE_FERMANTE },
		{ ";", id_morceau::POINT_VIRGULE }
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
