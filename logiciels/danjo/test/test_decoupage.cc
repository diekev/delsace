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
#include "biblinternes/langage/tampon_source.hh"

#include "compilation/decoupeuse.h"

#undef DEBOGUE_MORCEAUX

template <typename I1, typename I2>
bool verifie_morceaux(I1 debut1, I1 fin1, I2 debut2, I2 fin2)
{
	const auto dist1 = std::distance(debut1, fin1);
	const auto dist2 = std::distance(debut2, fin2);

	if (dist1 != dist2) {
#ifdef DEBOGUE_MORCEAUX
		std::cerr << "Les distances ne sont pas égales : "
				  << dist1
				  << " != "
				  << dist2
				  << '\n';
#endif
		return false;
	}

	while (debut1 != fin1 && debut2 != fin2) {
		if ((*debut1).identifiant != (*debut2).identifiant) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les identifiants ne sont pas égaux : "
					  << danjo::chaine_identifiant((*debut1).identifiant)
					  << " != "
					  << danjo::chaine_identifiant((*debut2).identifiant)
					  << '\n';
#endif
			return false;
		}

		if ((*debut1).chaine != (*debut2).chaine) {
#ifdef DEBOGUE_MORCEAUX
			std::cerr << "Les chaînes ne sont pas égales : "
					  << (*debut1).chaine
					  << " != "
					  << (*debut2).chaine
					  << '\n';
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
	const char *texte = "disposition \"Taille Image\" {\n"
						"	ligne {\n"
						"		étiquette(valeur=\"Dimension X\")\n"
						"		entier(valeur=1920; min=4; max=8196; attache=dimension_x)\n"
						"	}\n"
						"}\n";

	const danjo::DonneesMorceaux donnees_morceaux[] = {
		{ "disposition", 0, danjo::id_morceau::DISPOSITION },
		{ "Taille Image", 0, danjo::id_morceau::CHAINE_LITTERALE },
		{ "{", 0, danjo::id_morceau::ACCOLADE_OUVRANTE },
		{ "ligne", 0, danjo::id_morceau::LIGNE },
		{ "{", 0, danjo::id_morceau::ACCOLADE_OUVRANTE },
		{ "étiquette", 0, danjo::id_morceau::ETIQUETTE },
		{ "(", 0, danjo::id_morceau::PARENTHESE_OUVRANTE },
		{ "valeur", 0, danjo::id_morceau::VALEUR },
		{ "=", 0, danjo::id_morceau::EGAL },
		{ "Dimension X", 0, danjo::id_morceau::CHAINE_LITTERALE },
		{ ")", 0, danjo::id_morceau::PARENTHESE_FERMANTE },
		{ "entier", 0, danjo::id_morceau::ENTIER },
		{ "(", 0, danjo::id_morceau::PARENTHESE_OUVRANTE },
		{ "valeur", 0, danjo::id_morceau::VALEUR },
		{ "=", 0, danjo::id_morceau::EGAL },
		{ "1920", 0, danjo::id_morceau::NOMBRE },
		{ ";", 0, danjo::id_morceau::POINT_VIRGULE },
		{ "min", 0, danjo::id_morceau::MIN },
		{ "=", 0, danjo::id_morceau::EGAL },
		{ "4", 0, danjo::id_morceau::NOMBRE },
		{ ";", 0, danjo::id_morceau::POINT_VIRGULE },
		{ "max", 0, danjo::id_morceau::MAX },
		{ "=", 0, danjo::id_morceau::EGAL },
		{ "8196", 0, danjo::id_morceau::NOMBRE },
		{ ";", 0, danjo::id_morceau::POINT_VIRGULE },
		{ "attache", 0, danjo::id_morceau::ATTACHE },
		{ "=", 0, danjo::id_morceau::EGAL },
		{ "dimension_x", 0, danjo::id_morceau::CHAINE_CARACTERE },
		{ ")", 0, danjo::id_morceau::PARENTHESE_FERMANTE },
		{ "}", 0, danjo::id_morceau::ACCOLADE_FERMANTE },
		{ "}", 0, danjo::id_morceau::ACCOLADE_FERMANTE },
	};

	auto tampon = lng::tampon_source(texte);
	auto decoupeuse = danjo::Decoupeuse(tampon);
	decoupeuse.decoupe();

	return verifie_morceaux(decoupeuse.morceaux().debut(),
							decoupeuse.morceaux().fin(),
							std::begin(donnees_morceaux),
							std::end(donnees_morceaux));
}

void test_decoupage(dls::test_unitaire::Controleuse &controleuse)
{
	CU_VERIFIE_CONDITION(controleuse, test_decoupage_texte1());
}
