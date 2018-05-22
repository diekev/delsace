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

#include "interne/decoupeuse.h"

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

		if ((*debut1).contenu != (*debut2).contenu) {
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
		{ danjo::IDENTIFIANT_DISPOSITION, 0, 0, "disposition", "" },
		{ danjo::IDENTIFIANT_CHAINE_LITTERALE, 0, 0, "Taille Image", "" },
		{ danjo::IDENTIFIANT_ACCOLADE_OUVRANTE, 0, 0, "{", "" },
		{ danjo::IDENTIFIANT_LIGNE, 0, 0, "ligne", "" },
		{ danjo::IDENTIFIANT_ACCOLADE_OUVRANTE, 0, 0, "{", "" },
		{ danjo::IDENTIFIANT_ETIQUETTE, 0, 0, "étiquette", "" },
		{ danjo::IDENTIFIANT_PARENTHESE_OUVRANTE, 0, 0, "(", "" },
		{ danjo::IDENTIFIANT_VALEUR, 0, 0, "valeur", "" },
		{ danjo::IDENTIFIANT_EGAL, 0, 0, "=", "" },
		{ danjo::IDENTIFIANT_CHAINE_LITTERALE, 0, 0, "Dimension X", "" },
		{ danjo::IDENTIFIANT_PARENTHESE_FERMANTE, 0, 0, ")", "" },
		{ danjo::IDENTIFIANT_ENTIER, 0, 0, "entier", "" },
		{ danjo::IDENTIFIANT_PARENTHESE_OUVRANTE, 0, 0, "(", "" },
		{ danjo::IDENTIFIANT_VALEUR, 0, 0, "valeur", "" },
		{ danjo::IDENTIFIANT_EGAL, 0, 0, "=", "" },
		{ danjo::IDENTIFIANT_NOMBRE, 0, 0, "1920", "" },
		{ danjo::IDENTIFIANT_POINT_VIRGULE, 0, 0, ";", "" },
		{ danjo::IDENTIFIANT_MIN, 0, 0, "min", "" },
		{ danjo::IDENTIFIANT_EGAL, 0, 0, "=", "" },
		{ danjo::IDENTIFIANT_NOMBRE, 0, 0, "4", "" },
		{ danjo::IDENTIFIANT_POINT_VIRGULE, 0, 0, ";", "" },
		{ danjo::IDENTIFIANT_MAX, 0, 0, "max", "" },
		{ danjo::IDENTIFIANT_EGAL, 0, 0, "=", "" },
		{ danjo::IDENTIFIANT_NOMBRE, 0, 0, "8196", "" },
		{ danjo::IDENTIFIANT_POINT_VIRGULE, 0, 0, ";", "" },
		{ danjo::IDENTIFIANT_ATTACHE, 0, 0, "attache", "" },
		{ danjo::IDENTIFIANT_EGAL, 0, 0, "=", "" },
		{ danjo::IDENTIFIANT_CHAINE_CARACTERE, 0, 0, "dimension_x", "" },
		{ danjo::IDENTIFIANT_PARENTHESE_FERMANTE, 0, 0, ")", "" },
		{ danjo::IDENTIFIANT_ACCOLADE_FERMANTE, 0, 0, "}", "" },
		{ danjo::IDENTIFIANT_ACCOLADE_FERMANTE, 0, 0, "}", "" },
	};

	danjo::Decoupeuse decoupeuse(texte);
	decoupeuse.decoupe();

	return verifie_morceaux(decoupeuse.begin(),
							decoupeuse.end(),
							std::begin(donnees_morceaux),
							std::end(donnees_morceaux));
}

void test_decoupage(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	CU_VERIFIE_CONDITION(controleur, test_decoupage_texte1());
}
