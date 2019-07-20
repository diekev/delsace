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

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#include "morceaux.hh"

#include "biblinternes/structures/dico_fixe.hh"

static auto paires_mots_cles = dls::cree_dico(
	dls::paire{ dls::vue_chaine("ajourne"), ID_AJOURNE },
	dls::paire{ dls::vue_chaine("auto_incrémente"), ID_AUTO_INCREMENTE },
	dls::paire{ dls::vue_chaine("binaire"), ID_BINAIRE },
	dls::paire{ dls::vue_chaine("bit"), ID_BIT },
	dls::paire{ dls::vue_chaine("cascade"), ID_CASCADE },
	dls::paire{ dls::vue_chaine("chaîne"), ID_CHAINE },
	dls::paire{ dls::vue_chaine("clé"), ID_CLE },
	dls::paire{ dls::vue_chaine("clé_primaire"), ID_CLE_PRIMAIRE },
	dls::paire{ dls::vue_chaine("défaut"), ID_DEFAUT },
	dls::paire{ dls::vue_chaine("entier"), ID_ENTIER },
	dls::paire{ dls::vue_chaine("faux"), ID_FAUX },
	dls::paire{ dls::vue_chaine("nul"), ID_NUL },
	dls::paire{ dls::vue_chaine("octet"), ID_OCTET },
	dls::paire{ dls::vue_chaine("réel"), ID_REEL },
	dls::paire{ dls::vue_chaine("référence"), ID_REFERENCE },
	dls::paire{ dls::vue_chaine("signé"), ID_SIGNE },
	dls::paire{ dls::vue_chaine("supprime"), ID_SUPPRIME },
	dls::paire{ dls::vue_chaine("table"), ID_TABLE },
	dls::paire{ dls::vue_chaine("taille"), ID_TAILLE },
	dls::paire{ dls::vue_chaine("temps"), ID_TEMPS },
	dls::paire{ dls::vue_chaine("temps_courant"), ID_TEMPS_COURANT },
	dls::paire{ dls::vue_chaine("temps_date"), ID_TEMPS_DATE },
	dls::paire{ dls::vue_chaine("texte"), ID_TEXTE },
	dls::paire{ dls::vue_chaine("variable"), ID_VARIABLE },
	dls::paire{ dls::vue_chaine("vrai"), ID_VRAI },
	dls::paire{ dls::vue_chaine("zerofill"), ID_ZEROFILL }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '(', ID_PARENTHESE_OUVRANTE },
	dls::paire{ ')', ID_PARENTHESE_FERMANTE },
	dls::paire{ ',', ID_VIRGULE },
	dls::paire{ '.', ID_POINT },
	dls::paire{ ':', ID_DOUBLE_POINTS },
	dls::paire{ ';', ID_POINT_VIRGULE },
	dls::paire{ '{', ID_ACCOLADE_OUVRANTE },
	dls::paire{ '}', ID_ACCOLADE_FERMANTE }
);

const char *chaine_identifiant(int id)
{
	switch (id) {
		case ID_PARENTHESE_OUVRANTE:
			return "ID_PARENTHESE_OUVRANTE";
		case ID_PARENTHESE_FERMANTE:
			return "ID_PARENTHESE_FERMANTE";
		case ID_VIRGULE:
			return "ID_VIRGULE";
		case ID_POINT:
			return "ID_POINT";
		case ID_DOUBLE_POINTS:
			return "ID_DOUBLE_POINTS";
		case ID_POINT_VIRGULE:
			return "ID_POINT_VIRGULE";
		case ID_ACCOLADE_OUVRANTE:
			return "ID_ACCOLADE_OUVRANTE";
		case ID_ACCOLADE_FERMANTE:
			return "ID_ACCOLADE_FERMANTE";
		case ID_AJOURNE:
			return "ID_AJOURNE";
		case ID_AUTO_INCREMENTE:
			return "ID_AUTO_INCREMENTE";
		case ID_BINAIRE:
			return "ID_BINAIRE";
		case ID_BIT:
			return "ID_BIT";
		case ID_CASCADE:
			return "ID_CASCADE";
		case ID_CHAINE:
			return "ID_CHAINE";
		case ID_CLE:
			return "ID_CLE";
		case ID_CLE_PRIMAIRE:
			return "ID_CLE_PRIMAIRE";
		case ID_DEFAUT:
			return "ID_DEFAUT";
		case ID_ENTIER:
			return "ID_ENTIER";
		case ID_FAUX:
			return "ID_FAUX";
		case ID_NUL:
			return "ID_NUL";
		case ID_OCTET:
			return "ID_OCTET";
		case ID_REEL:
			return "ID_REEL";
		case ID_REFERENCE:
			return "ID_REFERENCE";
		case ID_SIGNE:
			return "ID_SIGNE";
		case ID_SUPPRIME:
			return "ID_SUPPRIME";
		case ID_TABLE:
			return "ID_TABLE";
		case ID_TAILLE:
			return "ID_TAILLE";
		case ID_TEMPS:
			return "ID_TEMPS";
		case ID_TEMPS_COURANT:
			return "ID_TEMPS_COURANT";
		case ID_TEMPS_DATE:
			return "ID_TEMPS_DATE";
		case ID_TEXTE:
			return "ID_TEXTE";
		case ID_VARIABLE:
			return "ID_VARIABLE";
		case ID_VRAI:
			return "ID_VRAI";
		case ID_ZEROFILL:
			return "ID_ZEROFILL";
		case ID_CHAINE_CARACTERE:
			return "ID_CHAINE_CARACTERE";
		case ID_NOMBRE_ENTIER:
			return "ID_NOMBRE_ENTIER";
		case ID_NOMBRE_REEL:
			return "ID_NOMBRE_REEL";
		case ID_NOMBRE_BINAIRE:
			return "ID_NOMBRE_BINAIRE";
		case ID_NOMBRE_OCTAL:
			return "ID_NOMBRE_OCTAL";
		case ID_NOMBRE_HEXADECIMAL:
			return "ID_NOMBRE_HEXADECIMAL";
		case ID_INCONNU:
			return "ID_INCONNU";
	};

	return "ERREUR";
}

static bool tables_caracteres[256] = {};
static int tables_identifiants[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = -1;
	}

    {
	    auto plg = paires_caracteres_speciaux.plage();

	    while (!plg.est_finie()) {
		    tables_caracteres[int(plg.front().premier)] = true;
		    tables_identifiants[int(plg.front().premier)] = plg.front().second;
	   		plg.effronte();
	    }
	}

    {
	    auto plg = paires_mots_cles.plage();

	    while (!plg.est_finie()) {
		    tables_mots_cles[static_cast<unsigned char>(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}
}

bool est_caractere_special(char c, int &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

int id_chaine(const dls::vue_chaine &chaine)
{
	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return ID_CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return ID_CHAINE_CARACTERE;
}
