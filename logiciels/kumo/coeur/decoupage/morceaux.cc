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
	dls::paire{ dls::vue_chaine("ajourne"), id_morceau::AJOURNE },
	dls::paire{ dls::vue_chaine("auto_incrémente"), id_morceau::AUTO_INCREMENTE },
	dls::paire{ dls::vue_chaine("binaire"), id_morceau::BINAIRE },
	dls::paire{ dls::vue_chaine("bit"), id_morceau::BIT },
	dls::paire{ dls::vue_chaine("cascade"), id_morceau::CASCADE },
	dls::paire{ dls::vue_chaine("chaîne"), id_morceau::CHAINE },
	dls::paire{ dls::vue_chaine("clé"), id_morceau::CLE },
	dls::paire{ dls::vue_chaine("clé_primaire"), id_morceau::CLE_PRIMAIRE },
	dls::paire{ dls::vue_chaine("défaut"), id_morceau::DEFAUT },
	dls::paire{ dls::vue_chaine("entier"), id_morceau::ENTIER },
	dls::paire{ dls::vue_chaine("faux"), id_morceau::FAUX },
	dls::paire{ dls::vue_chaine("nul"), id_morceau::NUL },
	dls::paire{ dls::vue_chaine("octet"), id_morceau::OCTET },
	dls::paire{ dls::vue_chaine("réel"), id_morceau::REEL },
	dls::paire{ dls::vue_chaine("référence"), id_morceau::REFERENCE },
	dls::paire{ dls::vue_chaine("signé"), id_morceau::SIGNE },
	dls::paire{ dls::vue_chaine("supprime"), id_morceau::SUPPRIME },
	dls::paire{ dls::vue_chaine("table"), id_morceau::TABLE },
	dls::paire{ dls::vue_chaine("taille"), id_morceau::TAILLE },
	dls::paire{ dls::vue_chaine("temps"), id_morceau::TEMPS },
	dls::paire{ dls::vue_chaine("temps_courant"), id_morceau::TEMPS_COURANT },
	dls::paire{ dls::vue_chaine("temps_date"), id_morceau::TEMPS_DATE },
	dls::paire{ dls::vue_chaine("texte"), id_morceau::TEXTE },
	dls::paire{ dls::vue_chaine("variable"), id_morceau::VARIABLE },
	dls::paire{ dls::vue_chaine("vrai"), id_morceau::VRAI },
	dls::paire{ dls::vue_chaine("zerofill"), id_morceau::ZEROFILL }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '(', id_morceau::PARENTHESE_OUVRANTE },
	dls::paire{ ')', id_morceau::PARENTHESE_FERMANTE },
	dls::paire{ ',', id_morceau::VIRGULE },
	dls::paire{ '.', id_morceau::POINT },
	dls::paire{ ':', id_morceau::DOUBLE_POINTS },
	dls::paire{ ';', id_morceau::POINT_VIRGULE },
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
		case id_morceau::POINT:
			return "id_morceau::POINT";
		case id_morceau::DOUBLE_POINTS:
			return "id_morceau::DOUBLE_POINTS";
		case id_morceau::POINT_VIRGULE:
			return "id_morceau::POINT_VIRGULE";
		case id_morceau::ACCOLADE_OUVRANTE:
			return "id_morceau::ACCOLADE_OUVRANTE";
		case id_morceau::ACCOLADE_FERMANTE:
			return "id_morceau::ACCOLADE_FERMANTE";
		case id_morceau::AJOURNE:
			return "id_morceau::AJOURNE";
		case id_morceau::AUTO_INCREMENTE:
			return "id_morceau::AUTO_INCREMENTE";
		case id_morceau::BINAIRE:
			return "id_morceau::BINAIRE";
		case id_morceau::BIT:
			return "id_morceau::BIT";
		case id_morceau::CASCADE:
			return "id_morceau::CASCADE";
		case id_morceau::CHAINE:
			return "id_morceau::CHAINE";
		case id_morceau::CLE:
			return "id_morceau::CLE";
		case id_morceau::CLE_PRIMAIRE:
			return "id_morceau::CLE_PRIMAIRE";
		case id_morceau::DEFAUT:
			return "id_morceau::DEFAUT";
		case id_morceau::ENTIER:
			return "id_morceau::ENTIER";
		case id_morceau::FAUX:
			return "id_morceau::FAUX";
		case id_morceau::NUL:
			return "id_morceau::NUL";
		case id_morceau::OCTET:
			return "id_morceau::OCTET";
		case id_morceau::REEL:
			return "id_morceau::REEL";
		case id_morceau::REFERENCE:
			return "id_morceau::REFERENCE";
		case id_morceau::SIGNE:
			return "id_morceau::SIGNE";
		case id_morceau::SUPPRIME:
			return "id_morceau::SUPPRIME";
		case id_morceau::TABLE:
			return "id_morceau::TABLE";
		case id_morceau::TAILLE:
			return "id_morceau::TAILLE";
		case id_morceau::TEMPS:
			return "id_morceau::TEMPS";
		case id_morceau::TEMPS_COURANT:
			return "id_morceau::TEMPS_COURANT";
		case id_morceau::TEMPS_DATE:
			return "id_morceau::TEMPS_DATE";
		case id_morceau::TEXTE:
			return "id_morceau::TEXTE";
		case id_morceau::VARIABLE:
			return "id_morceau::VARIABLE";
		case id_morceau::VRAI:
			return "id_morceau::VRAI";
		case id_morceau::ZEROFILL:
			return "id_morceau::ZEROFILL";
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
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_mots_cles[i] = false;
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

    {
	    auto plg = paires_mots_cles.plage();

	    while (!plg.est_finie()) {
		    tables_mots_cles[static_cast<unsigned char>(plg.front().premier[0])] = true;
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

id_morceau id_chaine(const dls::vue_chaine &chaine)
{
	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return id_morceau::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return id_morceau::CHAINE_CARACTERE;
}
