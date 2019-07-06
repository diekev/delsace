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

#include <map>

static std::map<std::string_view, id_morceau> paires_mots_cles = {
	{ "arrête", id_morceau::ARRETE },
	{ "associe", id_morceau::ASSOCIE },
	{ "bool", id_morceau::BOOL },
	{ "boucle", id_morceau::BOUCLE },
	{ "continue", id_morceau::CONTINUE },
	{ "dans", id_morceau::DANS },
	{ "de", id_morceau::DE },
	{ "diffère", id_morceau::DIFFERE },
	{ "déc", id_morceau::DEC },
	{ "ent", id_morceau::ENT },
	{ "faux", id_morceau::FAUX },
	{ "hors", id_morceau::HORS },
	{ "mat3", id_morceau::MAT3 },
	{ "mat4", id_morceau::MAT4 },
	{ "nul", id_morceau::NUL },
	{ "pour", id_morceau::POUR },
	{ "renvoie", id_morceau::RENVOIE },
	{ "retiens", id_morceau::RETIENS },
	{ "sansarrêt", id_morceau::SANSARRET },
	{ "si", id_morceau::SI },
	{ "sinon", id_morceau::SINON },
	{ "vec2", id_morceau::VEC2 },
	{ "vec3", id_morceau::VEC3 },
	{ "vec4", id_morceau::VEC4 },
	{ "vrai", id_morceau::VRAI },
};

static std::map<std::string_view, id_morceau> paires_caracteres_double = {
	{ "!=", id_morceau::DIFFERENCE },
	{ "&&", id_morceau::ESP_ESP },
	{ "*=", id_morceau::FOIS_EGAL },
	{ "+=", id_morceau::PLUS_EGAL },
	{ "-=", id_morceau::MOINS_EGAL },
	{ "/=", id_morceau::DIVISE_EGAL },
	{ "<<", id_morceau::DECALAGE_GAUCHE },
	{ "<=", id_morceau::INFERIEUR_EGAL },
	{ "==", id_morceau::EGALITE },
	{ ">=", id_morceau::SUPERIEUR_EGAL },
	{ ">>", id_morceau::DECALAGE_DROITE },
	{ "||", id_morceau::BARRE_BARRE },
};

static std::map<char, id_morceau> paires_caracteres_speciaux = {
	{ '!', id_morceau::EXCLAMATION },
	{ '"', id_morceau::GUILLEMET },
	{ '#', id_morceau::DIESE },
	{ '$', id_morceau::DOLLAR },
	{ '%', id_morceau::POURCENT },
	{ '&', id_morceau::ESPERLUETTE },
	{ '\'', id_morceau::APOSTROPHE },
	{ '(', id_morceau::PARENTHESE_OUVRANTE },
	{ ')', id_morceau::PARENTHESE_FERMANTE },
	{ '*', id_morceau::FOIS },
	{ '+', id_morceau::PLUS },
	{ ',', id_morceau::VIRGULE },
	{ '-', id_morceau::MOINS },
	{ '.', id_morceau::POINT },
	{ '/', id_morceau::DIVISE },
	{ ':', id_morceau::DOUBLE_POINTS },
	{ ';', id_morceau::POINT_VIRGULE },
	{ '<', id_morceau::INFERIEUR },
	{ '=', id_morceau::EGAL },
	{ '>', id_morceau::SUPERIEUR },
	{ '@', id_morceau::AROBASE },
	{ '[', id_morceau::CROCHET_OUVRANT },
	{ ']', id_morceau::CROCHET_FERMANT },
	{ '^', id_morceau::CHAPEAU },
	{ '{', id_morceau::ACCOLADE_OUVRANTE },
	{ '|', id_morceau::BARRE },
	{ '}', id_morceau::ACCOLADE_FERMANTE },
	{ '~', id_morceau::TILDE },
};

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

	for (const auto &iter : paires_caracteres_speciaux) {
		tables_caracteres[int(iter.first)] = true;
		tables_identifiants[int(iter.first)] = iter.second;
	}

	for (const auto &iter : paires_caracteres_double) {
		tables_caracteres_double[int(iter.first[0])] = true;
	}

	for (const auto &iter : paires_mots_cles) {
		tables_mots_cles[static_cast<unsigned char>(iter.first[0])] = true;
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

id_morceau id_caractere_double(const std::string_view &chaine)
{
	if (!tables_caracteres_double[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_caracteres_double.find(chaine);

	if (iterateur != paires_caracteres_double.end()) {
		return (*iterateur).second;
	}

	return id_morceau::INCONNU;
}

id_morceau id_chaine(const std::string_view &chaine)
{
	if (chaine.size() == 1 || chaine.size() > TAILLE_MAX_MOT_CLE) {
		return id_morceau::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return id_morceau::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.find(chaine);

	if (iterateur != paires_mots_cles.end()) {
		return (*iterateur).second;
	}

	return id_morceau::CHAINE_CARACTERE;
}
