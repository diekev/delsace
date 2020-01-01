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

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#include "morceaux.hh"

#include "biblinternes/structures/dico_fixe.hh"

namespace json {

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '(', id_morceau::PARENTHESE_OUVRANTE },
	dls::paire{ ')', id_morceau::PARENTHESE_FERMANTE },
	dls::paire{ ',', id_morceau::VIRGULE },
	dls::paire{ ':', id_morceau::DOUBLE_POINTS },
	dls::paire{ '[', id_morceau::CROCHET_OUVRANT },
	dls::paire{ ']', id_morceau::CROCHET_FERMANT },
	dls::paire{ '{', id_morceau::ACCOLADE_OUVRANTE },
	dls::paire{ '}', id_morceau::ACCOLADE_FERMANTE }
);

const char *chaine_identifiant(id_morceau id)
{
	switch (id) {
		case id_morceau::PARENTHESE_OUVRANTE:
			return "id_morceau::PARENTHESE_OUVRANTE";
		case id_morceau::PARENTHESE_FERMANTE:
			return "id_morceau::PARENTHESE_FERMANTE";
		case id_morceau::VIRGULE:
			return "id_morceau::VIRGULE";
		case id_morceau::DOUBLE_POINTS:
			return "id_morceau::DOUBLE_POINTS";
		case id_morceau::CROCHET_OUVRANT:
			return "id_morceau::CROCHET_OUVRANT";
		case id_morceau::CROCHET_FERMANT:
			return "id_morceau::CROCHET_FERMANT";
		case id_morceau::ACCOLADE_OUVRANTE:
			return "id_morceau::ACCOLADE_OUVRANTE";
		case id_morceau::ACCOLADE_FERMANTE:
			return "id_morceau::ACCOLADE_FERMANTE";
		case id_morceau::CHAINE_CARACTERE:
			return "id_morceau::CHAINE_CARACTERE";
		case id_morceau::NOMBRE_ENTIER:
			return "id_morceau::NOMBRE_ENTIER";
		case id_morceau::NOMBRE_REEL:
			return "id_morceau::NOMBRE_REEL";
		case id_morceau::NOMBRE_BINAIRE:
			return "id_morceau::NOMBRE_BINAIRE";
		case id_morceau::NOMBRE_OCTAL:
			return "id_morceau::NOMBRE_OCTAL";
		case id_morceau::NOMBRE_HEXADECIMAL:
			return "id_morceau::NOMBRE_HEXADECIMAL";
		case id_morceau::INCONNU:
			return "id_morceau::INCONNU";
	};

	return "ERREUR";
}

static bool tables_caracteres[256] = {};
static id_morceau tables_identifiants[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_identifiants[i] = id_morceau::INCONNU;
	}

    {
	    auto plg = paires_caracteres_speciaux.plage();

	    while (!plg.est_finie()) {
		    tables_caracteres[int(plg.front().premier)] = true;
		    tables_identifiants[int(plg.front().premier)] = plg.front().second;
	   		plg.effronte();
	    }
	}
}

bool est_caractere_special(char c, id_morceau &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

}  /* namespace json */
