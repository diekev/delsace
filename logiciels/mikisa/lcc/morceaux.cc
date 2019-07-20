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

static auto paires_mots_cles = dls::cree_dico(
	dls::paire{ dls::vue_chaine("arrête"), id_morceau::ARRETE },
	dls::paire{ dls::vue_chaine("associe"), id_morceau::ASSOCIE },
	dls::paire{ dls::vue_chaine("bool"), id_morceau::BOOL },
	dls::paire{ dls::vue_chaine("boucle"), id_morceau::BOUCLE },
	dls::paire{ dls::vue_chaine("continue"), id_morceau::CONTINUE },
	dls::paire{ dls::vue_chaine("dans"), id_morceau::DANS },
	dls::paire{ dls::vue_chaine("de"), id_morceau::DE },
	dls::paire{ dls::vue_chaine("diffère"), id_morceau::DIFFERE },
	dls::paire{ dls::vue_chaine("déc"), id_morceau::DEC },
	dls::paire{ dls::vue_chaine("ent"), id_morceau::ENT },
	dls::paire{ dls::vue_chaine("faux"), id_morceau::FAUX },
	dls::paire{ dls::vue_chaine("hors"), id_morceau::HORS },
	dls::paire{ dls::vue_chaine("mat3"), id_morceau::MAT3 },
	dls::paire{ dls::vue_chaine("mat4"), id_morceau::MAT4 },
	dls::paire{ dls::vue_chaine("nul"), id_morceau::NUL },
	dls::paire{ dls::vue_chaine("pour"), id_morceau::POUR },
	dls::paire{ dls::vue_chaine("renvoie"), id_morceau::RENVOIE },
	dls::paire{ dls::vue_chaine("retiens"), id_morceau::RETIENS },
	dls::paire{ dls::vue_chaine("sansarrêt"), id_morceau::SANSARRET },
	dls::paire{ dls::vue_chaine("si"), id_morceau::SI },
	dls::paire{ dls::vue_chaine("sinon"), id_morceau::SINON },
	dls::paire{ dls::vue_chaine("vec2"), id_morceau::VEC2 },
	dls::paire{ dls::vue_chaine("vec3"), id_morceau::VEC3 },
	dls::paire{ dls::vue_chaine("vec4"), id_morceau::VEC4 },
	dls::paire{ dls::vue_chaine("vrai"), id_morceau::VRAI }
);

static auto paires_caracteres_double = dls::cree_dico(
	dls::paire{ dls::vue_chaine("!="), id_morceau::DIFFERENCE },
	dls::paire{ dls::vue_chaine("&&"), id_morceau::ESP_ESP },
	dls::paire{ dls::vue_chaine("*="), id_morceau::FOIS_EGAL },
	dls::paire{ dls::vue_chaine("+="), id_morceau::PLUS_EGAL },
	dls::paire{ dls::vue_chaine("-="), id_morceau::MOINS_EGAL },
	dls::paire{ dls::vue_chaine("/="), id_morceau::DIVISE_EGAL },
	dls::paire{ dls::vue_chaine("<<"), id_morceau::DECALAGE_GAUCHE },
	dls::paire{ dls::vue_chaine("<="), id_morceau::INFERIEUR_EGAL },
	dls::paire{ dls::vue_chaine("=="), id_morceau::EGALITE },
	dls::paire{ dls::vue_chaine(">="), id_morceau::SUPERIEUR_EGAL },
	dls::paire{ dls::vue_chaine(">>"), id_morceau::DECALAGE_DROITE },
	dls::paire{ dls::vue_chaine("||"), id_morceau::BARRE_BARRE }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '!', id_morceau::EXCLAMATION },
	dls::paire{ '"', id_morceau::GUILLEMET },
	dls::paire{ '#', id_morceau::DIESE },
	dls::paire{ '$', id_morceau::DOLLAR },
	dls::paire{ '%', id_morceau::POURCENT },
	dls::paire{ '&', id_morceau::ESPERLUETTE },
	dls::paire{ '\'', id_morceau::APOSTROPHE },
	dls::paire{ '(', id_morceau::PARENTHESE_OUVRANTE },
	dls::paire{ ')', id_morceau::PARENTHESE_FERMANTE },
	dls::paire{ '*', id_morceau::FOIS },
	dls::paire{ '+', id_morceau::PLUS },
	dls::paire{ ',', id_morceau::VIRGULE },
	dls::paire{ '-', id_morceau::MOINS },
	dls::paire{ '.', id_morceau::POINT },
	dls::paire{ '/', id_morceau::DIVISE },
	dls::paire{ ':', id_morceau::DOUBLE_POINTS },
	dls::paire{ ';', id_morceau::POINT_VIRGULE },
	dls::paire{ '<', id_morceau::INFERIEUR },
	dls::paire{ '=', id_morceau::EGAL },
	dls::paire{ '>', id_morceau::SUPERIEUR },
	dls::paire{ '@', id_morceau::AROBASE },
	dls::paire{ '[', id_morceau::CROCHET_OUVRANT },
	dls::paire{ ']', id_morceau::CROCHET_FERMANT },
	dls::paire{ '^', id_morceau::CHAPEAU },
	dls::paire{ '{', id_morceau::ACCOLADE_OUVRANTE },
	dls::paire{ '|', id_morceau::BARRE },
	dls::paire{ '}', id_morceau::ACCOLADE_FERMANTE },
	dls::paire{ '~', id_morceau::TILDE }
);

const char *chaine_identifiant(id_morceau id)
{
	switch (id) {
		case id_morceau::EXCLAMATION:
			return "id_morceau::EXCLAMATION";
		case id_morceau::GUILLEMET:
			return "id_morceau::GUILLEMET";
		case id_morceau::DIESE:
			return "id_morceau::DIESE";
		case id_morceau::DOLLAR:
			return "id_morceau::DOLLAR";
		case id_morceau::POURCENT:
			return "id_morceau::POURCENT";
		case id_morceau::ESPERLUETTE:
			return "id_morceau::ESPERLUETTE";
		case id_morceau::APOSTROPHE:
			return "id_morceau::APOSTROPHE";
		case id_morceau::PARENTHESE_OUVRANTE:
			return "id_morceau::PARENTHESE_OUVRANTE";
		case id_morceau::PARENTHESE_FERMANTE:
			return "id_morceau::PARENTHESE_FERMANTE";
		case id_morceau::FOIS:
			return "id_morceau::FOIS";
		case id_morceau::PLUS:
			return "id_morceau::PLUS";
		case id_morceau::VIRGULE:
			return "id_morceau::VIRGULE";
		case id_morceau::MOINS:
			return "id_morceau::MOINS";
		case id_morceau::POINT:
			return "id_morceau::POINT";
		case id_morceau::DIVISE:
			return "id_morceau::DIVISE";
		case id_morceau::DOUBLE_POINTS:
			return "id_morceau::DOUBLE_POINTS";
		case id_morceau::POINT_VIRGULE:
			return "id_morceau::POINT_VIRGULE";
		case id_morceau::INFERIEUR:
			return "id_morceau::INFERIEUR";
		case id_morceau::EGAL:
			return "id_morceau::EGAL";
		case id_morceau::SUPERIEUR:
			return "id_morceau::SUPERIEUR";
		case id_morceau::AROBASE:
			return "id_morceau::AROBASE";
		case id_morceau::CROCHET_OUVRANT:
			return "id_morceau::CROCHET_OUVRANT";
		case id_morceau::CROCHET_FERMANT:
			return "id_morceau::CROCHET_FERMANT";
		case id_morceau::CHAPEAU:
			return "id_morceau::CHAPEAU";
		case id_morceau::ACCOLADE_OUVRANTE:
			return "id_morceau::ACCOLADE_OUVRANTE";
		case id_morceau::BARRE:
			return "id_morceau::BARRE";
		case id_morceau::ACCOLADE_FERMANTE:
			return "id_morceau::ACCOLADE_FERMANTE";
		case id_morceau::TILDE:
			return "id_morceau::TILDE";
		case id_morceau::DIFFERENCE:
			return "id_morceau::DIFFERENCE";
		case id_morceau::ESP_ESP:
			return "id_morceau::ESP_ESP";
		case id_morceau::FOIS_EGAL:
			return "id_morceau::FOIS_EGAL";
		case id_morceau::PLUS_EGAL:
			return "id_morceau::PLUS_EGAL";
		case id_morceau::MOINS_EGAL:
			return "id_morceau::MOINS_EGAL";
		case id_morceau::DIVISE_EGAL:
			return "id_morceau::DIVISE_EGAL";
		case id_morceau::DECALAGE_GAUCHE:
			return "id_morceau::DECALAGE_GAUCHE";
		case id_morceau::INFERIEUR_EGAL:
			return "id_morceau::INFERIEUR_EGAL";
		case id_morceau::EGALITE:
			return "id_morceau::EGALITE";
		case id_morceau::SUPERIEUR_EGAL:
			return "id_morceau::SUPERIEUR_EGAL";
		case id_morceau::DECALAGE_DROITE:
			return "id_morceau::DECALAGE_DROITE";
		case id_morceau::BARRE_BARRE:
			return "id_morceau::BARRE_BARRE";
		case id_morceau::ARRETE:
			return "id_morceau::ARRETE";
		case id_morceau::ASSOCIE:
			return "id_morceau::ASSOCIE";
		case id_morceau::BOOL:
			return "id_morceau::BOOL";
		case id_morceau::BOUCLE:
			return "id_morceau::BOUCLE";
		case id_morceau::CONTINUE:
			return "id_morceau::CONTINUE";
		case id_morceau::DANS:
			return "id_morceau::DANS";
		case id_morceau::DE:
			return "id_morceau::DE";
		case id_morceau::DIFFERE:
			return "id_morceau::DIFFERE";
		case id_morceau::DEC:
			return "id_morceau::DEC";
		case id_morceau::ENT:
			return "id_morceau::ENT";
		case id_morceau::FAUX:
			return "id_morceau::FAUX";
		case id_morceau::HORS:
			return "id_morceau::HORS";
		case id_morceau::MAT3:
			return "id_morceau::MAT3";
		case id_morceau::MAT4:
			return "id_morceau::MAT4";
		case id_morceau::NUL:
			return "id_morceau::NUL";
		case id_morceau::POUR:
			return "id_morceau::POUR";
		case id_morceau::RENVOIE:
			return "id_morceau::RENVOIE";
		case id_morceau::RETIENS:
			return "id_morceau::RETIENS";
		case id_morceau::SANSARRET:
			return "id_morceau::SANSARRET";
		case id_morceau::SI:
			return "id_morceau::SI";
		case id_morceau::SINON:
			return "id_morceau::SINON";
		case id_morceau::VEC2:
			return "id_morceau::VEC2";
		case id_morceau::VEC3:
			return "id_morceau::VEC3";
		case id_morceau::VEC4:
			return "id_morceau::VEC4";
		case id_morceau::VRAI:
			return "id_morceau::VRAI";
		case id_morceau::NOMBRE_REEL:
			return "id_morceau::NOMBRE_REEL";
		case id_morceau::NOMBRE_ENTIER:
			return "id_morceau::NOMBRE_ENTIER";
		case id_morceau::NOMBRE_HEXADECIMAL:
			return "id_morceau::NOMBRE_HEXADECIMAL";
		case id_morceau::NOMBRE_OCTAL:
			return "id_morceau::NOMBRE_OCTAL";
		case id_morceau::NOMBRE_BINAIRE:
			return "id_morceau::NOMBRE_BINAIRE";
		case id_morceau::PLUS_UNAIRE:
			return "id_morceau::PLUS_UNAIRE";
		case id_morceau::MOINS_UNAIRE:
			return "id_morceau::MOINS_UNAIRE";
		case id_morceau::TROIS_POINTS:
			return "id_morceau::TROIS_POINTS";
		case id_morceau::CHAINE_CARACTERE:
			return "id_morceau::CHAINE_CARACTERE";
		case id_morceau::CHAINE_LITTERALE:
			return "id_morceau::CHAINE_LITTERALE";
		case id_morceau::CARACTERE:
			return "id_morceau::CARACTERE";
		case id_morceau::TABLEAU:
			return "id_morceau::TABLEAU";
		case id_morceau::INCONNU:
			return "id_morceau::INCONNU";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 10;

static bool tables_caracteres[256] = {};
static id_morceau tables_identifiants[256] = {};
static bool tables_caracteres_double[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_caracteres_double[i] = false;
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
	    auto plg = paires_caracteres_double.plage();

	    while (!plg.est_finie()) {
		    tables_caracteres_double[int(plg.front().premier[0])] = true;
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

id_morceau id_caractere_double(const dls::vue_chaine &chaine)
{
	if (!tables_caracteres_double[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_caracteres_double.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return id_morceau::INCONNU;
}

id_morceau id_chaine(const dls::vue_chaine &chaine)
{
	if (chaine.taille() == 1 || chaine.taille() > TAILLE_MAX_MOT_CLE) {
		return id_morceau::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return id_morceau::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return id_morceau::CHAINE_CARACTERE;
}
