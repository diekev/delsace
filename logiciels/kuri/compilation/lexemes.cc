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
 
#include "lexemes.hh"

#include "biblinternes/structures/dico_fixe.hh"

static auto paires_mots_cles = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("arrête"), GenreLexeme::ARRETE },
	dls::paire{ dls::vue_chaine_compacte("bool"), GenreLexeme::BOOL },
	dls::paire{ dls::vue_chaine_compacte("boucle"), GenreLexeme::BOUCLE },
	dls::paire{ dls::vue_chaine_compacte("chaine"), GenreLexeme::CHAINE },
	dls::paire{ dls::vue_chaine_compacte("charge"), GenreLexeme::CHARGE },
	dls::paire{ dls::vue_chaine_compacte("continue"), GenreLexeme::CONTINUE },
	dls::paire{ dls::vue_chaine_compacte("corout"), GenreLexeme::COROUT },
	dls::paire{ dls::vue_chaine_compacte("dans"), GenreLexeme::DANS },
	dls::paire{ dls::vue_chaine_compacte("diffère"), GenreLexeme::DIFFERE },
	dls::paire{ dls::vue_chaine_compacte("discr"), GenreLexeme::DISCR },
	dls::paire{ dls::vue_chaine_compacte("dyn"), GenreLexeme::DYN },
	dls::paire{ dls::vue_chaine_compacte("déloge"), GenreLexeme::DELOGE },
	dls::paire{ dls::vue_chaine_compacte("eini"), GenreLexeme::EINI },
	dls::paire{ dls::vue_chaine_compacte("eini_erreur"), GenreLexeme::EINI_ERREUR },
	dls::paire{ dls::vue_chaine_compacte("empl"), GenreLexeme::EMPL },
	dls::paire{ dls::vue_chaine_compacte("erreur"), GenreLexeme::ERREUR },
	dls::paire{ dls::vue_chaine_compacte("externe"), GenreLexeme::EXTERNE },
	dls::paire{ dls::vue_chaine_compacte("faux"), GenreLexeme::FAUX },
	dls::paire{ dls::vue_chaine_compacte("fonc"), GenreLexeme::FONC },
	dls::paire{ dls::vue_chaine_compacte("garde"), GenreLexeme::GARDE },
	dls::paire{ dls::vue_chaine_compacte("importe"), GenreLexeme::IMPORTE },
	dls::paire{ dls::vue_chaine_compacte("info_de"), GenreLexeme::INFO_DE },
	dls::paire{ dls::vue_chaine_compacte("loge"), GenreLexeme::LOGE },
	dls::paire{ dls::vue_chaine_compacte("mémoire"), GenreLexeme::MEMOIRE },
	dls::paire{ dls::vue_chaine_compacte("n16"), GenreLexeme::N16 },
	dls::paire{ dls::vue_chaine_compacte("n32"), GenreLexeme::N32 },
	dls::paire{ dls::vue_chaine_compacte("n64"), GenreLexeme::N64 },
	dls::paire{ dls::vue_chaine_compacte("n8"), GenreLexeme::N8 },
	dls::paire{ dls::vue_chaine_compacte("nonatteignable"), GenreLexeme::NONATTEIGNABLE },
	dls::paire{ dls::vue_chaine_compacte("nonsûr"), GenreLexeme::NONSUR },
	dls::paire{ dls::vue_chaine_compacte("nul"), GenreLexeme::NUL },
	dls::paire{ dls::vue_chaine_compacte("octet"), GenreLexeme::OCTET },
	dls::paire{ dls::vue_chaine_compacte("opérateur"), GenreLexeme::OPERATEUR },
	dls::paire{ dls::vue_chaine_compacte("piège"), GenreLexeme::PIEGE },
	dls::paire{ dls::vue_chaine_compacte("pour"), GenreLexeme::POUR },
	dls::paire{ dls::vue_chaine_compacte("pousse_contexte"), GenreLexeme::POUSSE_CONTEXTE },
	dls::paire{ dls::vue_chaine_compacte("r16"), GenreLexeme::R16 },
	dls::paire{ dls::vue_chaine_compacte("r32"), GenreLexeme::R32 },
	dls::paire{ dls::vue_chaine_compacte("r64"), GenreLexeme::R64 },
	dls::paire{ dls::vue_chaine_compacte("reloge"), GenreLexeme::RELOGE },
	dls::paire{ dls::vue_chaine_compacte("retiens"), GenreLexeme::RETIENS },
	dls::paire{ dls::vue_chaine_compacte("retourne"), GenreLexeme::RETOURNE },
	dls::paire{ dls::vue_chaine_compacte("rien"), GenreLexeme::RIEN },
	dls::paire{ dls::vue_chaine_compacte("répète"), GenreLexeme::REPETE },
	dls::paire{ dls::vue_chaine_compacte("sansarrêt"), GenreLexeme::SANSARRET },
	dls::paire{ dls::vue_chaine_compacte("saufsi"), GenreLexeme::SAUFSI },
	dls::paire{ dls::vue_chaine_compacte("si"), GenreLexeme::SI },
	dls::paire{ dls::vue_chaine_compacte("sinon"), GenreLexeme::SINON },
	dls::paire{ dls::vue_chaine_compacte("struct"), GenreLexeme::STRUCT },
	dls::paire{ dls::vue_chaine_compacte("taille_de"), GenreLexeme::TAILLE_DE },
	dls::paire{ dls::vue_chaine_compacte("tantque"), GenreLexeme::TANTQUE },
	dls::paire{ dls::vue_chaine_compacte("tente"), GenreLexeme::TENTE },
	dls::paire{ dls::vue_chaine_compacte("transtype"), GenreLexeme::TRANSTYPE },
	dls::paire{ dls::vue_chaine_compacte("type_de"), GenreLexeme::TYPE_DE },
	dls::paire{ dls::vue_chaine_compacte("union"), GenreLexeme::UNION },
	dls::paire{ dls::vue_chaine_compacte("vrai"), GenreLexeme::VRAI },
	dls::paire{ dls::vue_chaine_compacte("z16"), GenreLexeme::Z16 },
	dls::paire{ dls::vue_chaine_compacte("z32"), GenreLexeme::Z32 },
	dls::paire{ dls::vue_chaine_compacte("z64"), GenreLexeme::Z64 },
	dls::paire{ dls::vue_chaine_compacte("z8"), GenreLexeme::Z8 },
	dls::paire{ dls::vue_chaine_compacte("énum"), GenreLexeme::ENUM },
	dls::paire{ dls::vue_chaine_compacte("énum_drapeau"), GenreLexeme::ENUM_DRAPEAU }
);

static auto paires_digraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("!="), GenreLexeme::DIFFERENCE },
	dls::paire{ dls::vue_chaine_compacte("%="), GenreLexeme::MODULO_EGAL },
	dls::paire{ dls::vue_chaine_compacte("&&"), GenreLexeme::ESP_ESP },
	dls::paire{ dls::vue_chaine_compacte("&="), GenreLexeme::ET_EGAL },
	dls::paire{ dls::vue_chaine_compacte("*/"), GenreLexeme::FIN_BLOC_COMMENTAIRE },
	dls::paire{ dls::vue_chaine_compacte("*="), GenreLexeme::MULTIPLIE_EGAL },
	dls::paire{ dls::vue_chaine_compacte("+="), GenreLexeme::PLUS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("-="), GenreLexeme::MOINS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("->"), GenreLexeme::RETOUR_TYPE },
	dls::paire{ dls::vue_chaine_compacte("/*"), GenreLexeme::DEBUT_BLOC_COMMENTAIRE },
	dls::paire{ dls::vue_chaine_compacte("//"), GenreLexeme::DEBUT_LIGNE_COMMENTAIRE },
	dls::paire{ dls::vue_chaine_compacte("/="), GenreLexeme::DIVISE_EGAL },
	dls::paire{ dls::vue_chaine_compacte("::"), GenreLexeme::DECLARATION_CONSTANTE },
	dls::paire{ dls::vue_chaine_compacte(":="), GenreLexeme::DECLARATION_VARIABLE },
	dls::paire{ dls::vue_chaine_compacte("<<"), GenreLexeme::DECALAGE_GAUCHE },
	dls::paire{ dls::vue_chaine_compacte("<="), GenreLexeme::INFERIEUR_EGAL },
	dls::paire{ dls::vue_chaine_compacte("=="), GenreLexeme::EGALITE },
	dls::paire{ dls::vue_chaine_compacte(">="), GenreLexeme::SUPERIEUR_EGAL },
	dls::paire{ dls::vue_chaine_compacte(">>"), GenreLexeme::DECALAGE_DROITE },
	dls::paire{ dls::vue_chaine_compacte("^="), GenreLexeme::OUX_EGAL },
	dls::paire{ dls::vue_chaine_compacte("|="), GenreLexeme::OU_EGAL },
	dls::paire{ dls::vue_chaine_compacte("||"), GenreLexeme::BARRE_BARRE }
);

static auto paires_trigraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("---"), GenreLexeme::NON_INITIALISATION },
	dls::paire{ dls::vue_chaine_compacte("..."), GenreLexeme::TROIS_POINTS },
	dls::paire{ dls::vue_chaine_compacte("<<="), GenreLexeme::DEC_GAUCHE_EGAL },
	dls::paire{ dls::vue_chaine_compacte(">>="), GenreLexeme::DEC_DROITE_EGAL }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '!', GenreLexeme::EXCLAMATION },
	dls::paire{ '"', GenreLexeme::GUILLEMET },
	dls::paire{ '#', GenreLexeme::DIRECTIVE },
	dls::paire{ '$', GenreLexeme::DOLLAR },
	dls::paire{ '%', GenreLexeme::POURCENT },
	dls::paire{ '&', GenreLexeme::ESPERLUETTE },
	dls::paire{ '\'', GenreLexeme::APOSTROPHE },
	dls::paire{ '(', GenreLexeme::PARENTHESE_OUVRANTE },
	dls::paire{ ')', GenreLexeme::PARENTHESE_FERMANTE },
	dls::paire{ '*', GenreLexeme::FOIS },
	dls::paire{ '+', GenreLexeme::PLUS },
	dls::paire{ ',', GenreLexeme::VIRGULE },
	dls::paire{ '-', GenreLexeme::MOINS },
	dls::paire{ '.', GenreLexeme::POINT },
	dls::paire{ '/', GenreLexeme::DIVISE },
	dls::paire{ ':', GenreLexeme::DOUBLE_POINTS },
	dls::paire{ ';', GenreLexeme::POINT_VIRGULE },
	dls::paire{ '<', GenreLexeme::INFERIEUR },
	dls::paire{ '=', GenreLexeme::EGAL },
	dls::paire{ '>', GenreLexeme::SUPERIEUR },
	dls::paire{ '@', GenreLexeme::AROBASE },
	dls::paire{ '[', GenreLexeme::CROCHET_OUVRANT },
	dls::paire{ ']', GenreLexeme::CROCHET_FERMANT },
	dls::paire{ '^', GenreLexeme::CHAPEAU },
	dls::paire{ '{', GenreLexeme::ACCOLADE_OUVRANTE },
	dls::paire{ '|', GenreLexeme::BARRE },
	dls::paire{ '}', GenreLexeme::ACCOLADE_FERMANTE },
	dls::paire{ '~', GenreLexeme::TILDE }
);

const char *chaine_du_genre_de_lexeme(GenreLexeme id)
{
	switch (id) {
		case GenreLexeme::EXCLAMATION:
			return "GenreLexeme::EXCLAMATION";
		case GenreLexeme::GUILLEMET:
			return "GenreLexeme::GUILLEMET";
		case GenreLexeme::DIRECTIVE:
			return "GenreLexeme::DIRECTIVE";
		case GenreLexeme::DOLLAR:
			return "GenreLexeme::DOLLAR";
		case GenreLexeme::POURCENT:
			return "GenreLexeme::POURCENT";
		case GenreLexeme::ESPERLUETTE:
			return "GenreLexeme::ESPERLUETTE";
		case GenreLexeme::APOSTROPHE:
			return "GenreLexeme::APOSTROPHE";
		case GenreLexeme::PARENTHESE_OUVRANTE:
			return "GenreLexeme::PARENTHESE_OUVRANTE";
		case GenreLexeme::PARENTHESE_FERMANTE:
			return "GenreLexeme::PARENTHESE_FERMANTE";
		case GenreLexeme::FOIS:
			return "GenreLexeme::FOIS";
		case GenreLexeme::PLUS:
			return "GenreLexeme::PLUS";
		case GenreLexeme::VIRGULE:
			return "GenreLexeme::VIRGULE";
		case GenreLexeme::MOINS:
			return "GenreLexeme::MOINS";
		case GenreLexeme::POINT:
			return "GenreLexeme::POINT";
		case GenreLexeme::DIVISE:
			return "GenreLexeme::DIVISE";
		case GenreLexeme::DOUBLE_POINTS:
			return "GenreLexeme::DOUBLE_POINTS";
		case GenreLexeme::POINT_VIRGULE:
			return "GenreLexeme::POINT_VIRGULE";
		case GenreLexeme::INFERIEUR:
			return "GenreLexeme::INFERIEUR";
		case GenreLexeme::EGAL:
			return "GenreLexeme::EGAL";
		case GenreLexeme::SUPERIEUR:
			return "GenreLexeme::SUPERIEUR";
		case GenreLexeme::AROBASE:
			return "GenreLexeme::AROBASE";
		case GenreLexeme::CROCHET_OUVRANT:
			return "GenreLexeme::CROCHET_OUVRANT";
		case GenreLexeme::CROCHET_FERMANT:
			return "GenreLexeme::CROCHET_FERMANT";
		case GenreLexeme::CHAPEAU:
			return "GenreLexeme::CHAPEAU";
		case GenreLexeme::ACCOLADE_OUVRANTE:
			return "GenreLexeme::ACCOLADE_OUVRANTE";
		case GenreLexeme::BARRE:
			return "GenreLexeme::BARRE";
		case GenreLexeme::ACCOLADE_FERMANTE:
			return "GenreLexeme::ACCOLADE_FERMANTE";
		case GenreLexeme::TILDE:
			return "GenreLexeme::TILDE";
		case GenreLexeme::DIFFERENCE:
			return "GenreLexeme::DIFFERENCE";
		case GenreLexeme::MODULO_EGAL:
			return "GenreLexeme::MODULO_EGAL";
		case GenreLexeme::ESP_ESP:
			return "GenreLexeme::ESP_ESP";
		case GenreLexeme::ET_EGAL:
			return "GenreLexeme::ET_EGAL";
		case GenreLexeme::FIN_BLOC_COMMENTAIRE:
			return "GenreLexeme::FIN_BLOC_COMMENTAIRE";
		case GenreLexeme::MULTIPLIE_EGAL:
			return "GenreLexeme::MULTIPLIE_EGAL";
		case GenreLexeme::PLUS_EGAL:
			return "GenreLexeme::PLUS_EGAL";
		case GenreLexeme::MOINS_EGAL:
			return "GenreLexeme::MOINS_EGAL";
		case GenreLexeme::RETOUR_TYPE:
			return "GenreLexeme::RETOUR_TYPE";
		case GenreLexeme::DEBUT_BLOC_COMMENTAIRE:
			return "GenreLexeme::DEBUT_BLOC_COMMENTAIRE";
		case GenreLexeme::DEBUT_LIGNE_COMMENTAIRE:
			return "GenreLexeme::DEBUT_LIGNE_COMMENTAIRE";
		case GenreLexeme::DIVISE_EGAL:
			return "GenreLexeme::DIVISE_EGAL";
		case GenreLexeme::DECLARATION_CONSTANTE:
			return "GenreLexeme::DECLARATION_CONSTANTE";
		case GenreLexeme::DECLARATION_VARIABLE:
			return "GenreLexeme::DECLARATION_VARIABLE";
		case GenreLexeme::DECALAGE_GAUCHE:
			return "GenreLexeme::DECALAGE_GAUCHE";
		case GenreLexeme::INFERIEUR_EGAL:
			return "GenreLexeme::INFERIEUR_EGAL";
		case GenreLexeme::EGALITE:
			return "GenreLexeme::EGALITE";
		case GenreLexeme::SUPERIEUR_EGAL:
			return "GenreLexeme::SUPERIEUR_EGAL";
		case GenreLexeme::DECALAGE_DROITE:
			return "GenreLexeme::DECALAGE_DROITE";
		case GenreLexeme::OUX_EGAL:
			return "GenreLexeme::OUX_EGAL";
		case GenreLexeme::OU_EGAL:
			return "GenreLexeme::OU_EGAL";
		case GenreLexeme::BARRE_BARRE:
			return "GenreLexeme::BARRE_BARRE";
		case GenreLexeme::NON_INITIALISATION:
			return "GenreLexeme::NON_INITIALISATION";
		case GenreLexeme::TROIS_POINTS:
			return "GenreLexeme::TROIS_POINTS";
		case GenreLexeme::DEC_GAUCHE_EGAL:
			return "GenreLexeme::DEC_GAUCHE_EGAL";
		case GenreLexeme::DEC_DROITE_EGAL:
			return "GenreLexeme::DEC_DROITE_EGAL";
		case GenreLexeme::NOMBRE_REEL:
			return "GenreLexeme::NOMBRE_REEL";
		case GenreLexeme::NOMBRE_ENTIER:
			return "GenreLexeme::NOMBRE_ENTIER";
		case GenreLexeme::PLUS_UNAIRE:
			return "GenreLexeme::PLUS_UNAIRE";
		case GenreLexeme::MOINS_UNAIRE:
			return "GenreLexeme::MOINS_UNAIRE";
		case GenreLexeme::CHAINE_CARACTERE:
			return "GenreLexeme::CHAINE_CARACTERE";
		case GenreLexeme::CHAINE_LITTERALE:
			return "GenreLexeme::CHAINE_LITTERALE";
		case GenreLexeme::CARACTERE:
			return "GenreLexeme::CARACTERE";
		case GenreLexeme::POINTEUR:
			return "GenreLexeme::POINTEUR";
		case GenreLexeme::TABLEAU:
			return "GenreLexeme::TABLEAU";
		case GenreLexeme::REFERENCE:
			return "GenreLexeme::REFERENCE";
		case GenreLexeme::INCONNU:
			return "GenreLexeme::INCONNU";
		case GenreLexeme::CARACTERE_BLANC:
			return "GenreLexeme::CARACTERE_BLANC";
		case GenreLexeme::COMMENTAIRE:
			return "GenreLexeme::COMMENTAIRE";
		case GenreLexeme::EXPANSION_VARIADIQUE:
			return "GenreLexeme::EXPANSION_VARIADIQUE";
		case GenreLexeme::ARRETE:
			return "GenreLexeme::ARRETE";
		case GenreLexeme::BOOL:
			return "GenreLexeme::BOOL";
		case GenreLexeme::BOUCLE:
			return "GenreLexeme::BOUCLE";
		case GenreLexeme::CHAINE:
			return "GenreLexeme::CHAINE";
		case GenreLexeme::CHARGE:
			return "GenreLexeme::CHARGE";
		case GenreLexeme::CONTINUE:
			return "GenreLexeme::CONTINUE";
		case GenreLexeme::COROUT:
			return "GenreLexeme::COROUT";
		case GenreLexeme::DANS:
			return "GenreLexeme::DANS";
		case GenreLexeme::DIFFERE:
			return "GenreLexeme::DIFFERE";
		case GenreLexeme::DISCR:
			return "GenreLexeme::DISCR";
		case GenreLexeme::DYN:
			return "GenreLexeme::DYN";
		case GenreLexeme::DELOGE:
			return "GenreLexeme::DELOGE";
		case GenreLexeme::EINI:
			return "GenreLexeme::EINI";
		case GenreLexeme::EINI_ERREUR:
			return "GenreLexeme::EINI_ERREUR";
		case GenreLexeme::EMPL:
			return "GenreLexeme::EMPL";
		case GenreLexeme::ERREUR:
			return "GenreLexeme::ERREUR";
		case GenreLexeme::EXTERNE:
			return "GenreLexeme::EXTERNE";
		case GenreLexeme::FAUX:
			return "GenreLexeme::FAUX";
		case GenreLexeme::FONC:
			return "GenreLexeme::FONC";
		case GenreLexeme::GARDE:
			return "GenreLexeme::GARDE";
		case GenreLexeme::IMPORTE:
			return "GenreLexeme::IMPORTE";
		case GenreLexeme::INFO_DE:
			return "GenreLexeme::INFO_DE";
		case GenreLexeme::LOGE:
			return "GenreLexeme::LOGE";
		case GenreLexeme::MEMOIRE:
			return "GenreLexeme::MEMOIRE";
		case GenreLexeme::N16:
			return "GenreLexeme::N16";
		case GenreLexeme::N32:
			return "GenreLexeme::N32";
		case GenreLexeme::N64:
			return "GenreLexeme::N64";
		case GenreLexeme::N8:
			return "GenreLexeme::N8";
		case GenreLexeme::NONATTEIGNABLE:
			return "GenreLexeme::NONATTEIGNABLE";
		case GenreLexeme::NONSUR:
			return "GenreLexeme::NONSUR";
		case GenreLexeme::NUL:
			return "GenreLexeme::NUL";
		case GenreLexeme::OCTET:
			return "GenreLexeme::OCTET";
		case GenreLexeme::OPERATEUR:
			return "GenreLexeme::OPERATEUR";
		case GenreLexeme::PIEGE:
			return "GenreLexeme::PIEGE";
		case GenreLexeme::POUR:
			return "GenreLexeme::POUR";
		case GenreLexeme::POUSSE_CONTEXTE:
			return "GenreLexeme::POUSSE_CONTEXTE";
		case GenreLexeme::R16:
			return "GenreLexeme::R16";
		case GenreLexeme::R32:
			return "GenreLexeme::R32";
		case GenreLexeme::R64:
			return "GenreLexeme::R64";
		case GenreLexeme::RELOGE:
			return "GenreLexeme::RELOGE";
		case GenreLexeme::RETIENS:
			return "GenreLexeme::RETIENS";
		case GenreLexeme::RETOURNE:
			return "GenreLexeme::RETOURNE";
		case GenreLexeme::RIEN:
			return "GenreLexeme::RIEN";
		case GenreLexeme::REPETE:
			return "GenreLexeme::REPETE";
		case GenreLexeme::SANSARRET:
			return "GenreLexeme::SANSARRET";
		case GenreLexeme::SAUFSI:
			return "GenreLexeme::SAUFSI";
		case GenreLexeme::SI:
			return "GenreLexeme::SI";
		case GenreLexeme::SINON:
			return "GenreLexeme::SINON";
		case GenreLexeme::STRUCT:
			return "GenreLexeme::STRUCT";
		case GenreLexeme::TAILLE_DE:
			return "GenreLexeme::TAILLE_DE";
		case GenreLexeme::TANTQUE:
			return "GenreLexeme::TANTQUE";
		case GenreLexeme::TENTE:
			return "GenreLexeme::TENTE";
		case GenreLexeme::TRANSTYPE:
			return "GenreLexeme::TRANSTYPE";
		case GenreLexeme::TYPE_DE:
			return "GenreLexeme::TYPE_DE";
		case GenreLexeme::UNION:
			return "GenreLexeme::UNION";
		case GenreLexeme::VRAI:
			return "GenreLexeme::VRAI";
		case GenreLexeme::Z16:
			return "GenreLexeme::Z16";
		case GenreLexeme::Z32:
			return "GenreLexeme::Z32";
		case GenreLexeme::Z64:
			return "GenreLexeme::Z64";
		case GenreLexeme::Z8:
			return "GenreLexeme::Z8";
		case GenreLexeme::ENUM:
			return "GenreLexeme::ENUM";
		case GenreLexeme::ENUM_DRAPEAU:
			return "GenreLexeme::ENUM_DRAPEAU";
	};

	return "ERREUR";
}
const char *chaine_du_lexeme(GenreLexeme genre)
{
	switch (genre) {
		case GenreLexeme::EXCLAMATION:
			return "!";
		case GenreLexeme::GUILLEMET:
			return "\"";
		case GenreLexeme::DIRECTIVE:
			return "#";
		case GenreLexeme::DOLLAR:
			return "$";
		case GenreLexeme::POURCENT:
			return "%";
		case GenreLexeme::ESPERLUETTE:
			return "&";
		case GenreLexeme::APOSTROPHE:
			return "\'";
		case GenreLexeme::PARENTHESE_OUVRANTE:
			return "(";
		case GenreLexeme::PARENTHESE_FERMANTE:
			return ")";
		case GenreLexeme::FOIS:
			return "*";
		case GenreLexeme::PLUS:
			return "+";
		case GenreLexeme::VIRGULE:
			return ",";
		case GenreLexeme::MOINS:
			return "-";
		case GenreLexeme::POINT:
			return ".";
		case GenreLexeme::DIVISE:
			return "/";
		case GenreLexeme::DOUBLE_POINTS:
			return ":";
		case GenreLexeme::POINT_VIRGULE:
			return ";";
		case GenreLexeme::INFERIEUR:
			return "<";
		case GenreLexeme::EGAL:
			return "=";
		case GenreLexeme::SUPERIEUR:
			return ">";
		case GenreLexeme::AROBASE:
			return "@";
		case GenreLexeme::CROCHET_OUVRANT:
			return "[";
		case GenreLexeme::CROCHET_FERMANT:
			return "]";
		case GenreLexeme::CHAPEAU:
			return "^";
		case GenreLexeme::ACCOLADE_OUVRANTE:
			return "{";
		case GenreLexeme::BARRE:
			return "|";
		case GenreLexeme::ACCOLADE_FERMANTE:
			return "}";
		case GenreLexeme::TILDE:
			return "~";
		case GenreLexeme::DIFFERENCE:
			return "!=";
		case GenreLexeme::MODULO_EGAL:
			return "%=";
		case GenreLexeme::ESP_ESP:
			return "&&";
		case GenreLexeme::ET_EGAL:
			return "&=";
		case GenreLexeme::FIN_BLOC_COMMENTAIRE:
			return "*/";
		case GenreLexeme::MULTIPLIE_EGAL:
			return "*=";
		case GenreLexeme::PLUS_EGAL:
			return "+=";
		case GenreLexeme::MOINS_EGAL:
			return "-=";
		case GenreLexeme::RETOUR_TYPE:
			return "->";
		case GenreLexeme::DEBUT_BLOC_COMMENTAIRE:
			return "/*";
		case GenreLexeme::DEBUT_LIGNE_COMMENTAIRE:
			return "//";
		case GenreLexeme::DIVISE_EGAL:
			return "/=";
		case GenreLexeme::DECLARATION_CONSTANTE:
			return "::";
		case GenreLexeme::DECLARATION_VARIABLE:
			return ":=";
		case GenreLexeme::DECALAGE_GAUCHE:
			return "<<";
		case GenreLexeme::INFERIEUR_EGAL:
			return "<=";
		case GenreLexeme::EGALITE:
			return "==";
		case GenreLexeme::SUPERIEUR_EGAL:
			return ">=";
		case GenreLexeme::DECALAGE_DROITE:
			return ">>";
		case GenreLexeme::OUX_EGAL:
			return "^=";
		case GenreLexeme::OU_EGAL:
			return "|=";
		case GenreLexeme::BARRE_BARRE:
			return "||";
		case GenreLexeme::NON_INITIALISATION:
			return "---";
		case GenreLexeme::TROIS_POINTS:
			return "...";
		case GenreLexeme::DEC_GAUCHE_EGAL:
			return "<<=";
		case GenreLexeme::DEC_DROITE_EGAL:
			return ">>=";
		case GenreLexeme::NOMBRE_REEL:
			return "1.234";
		case GenreLexeme::NOMBRE_ENTIER:
			return "123";
		case GenreLexeme::PLUS_UNAIRE:
			return "-";
		case GenreLexeme::MOINS_UNAIRE:
			return "+";
		case GenreLexeme::CHAINE_CARACTERE:
			return "chaine_de_caractère";
		case GenreLexeme::CHAINE_LITTERALE:
			return "chaine littérale";
		case GenreLexeme::CARACTERE:
			return "a";
		case GenreLexeme::POINTEUR:
			return "*";
		case GenreLexeme::TABLEAU:
			return "[]";
		case GenreLexeme::REFERENCE:
			return "&";
		case GenreLexeme::INCONNU:
			return "inconnu";
		case GenreLexeme::CARACTERE_BLANC:
			return " ";
		case GenreLexeme::COMMENTAIRE:
			return "// commentaire";
		case GenreLexeme::EXPANSION_VARIADIQUE:
			return "...";
		case GenreLexeme::ARRETE:
			return "arrête";
		case GenreLexeme::BOOL:
			return "bool";
		case GenreLexeme::BOUCLE:
			return "boucle";
		case GenreLexeme::CHAINE:
			return "chaine";
		case GenreLexeme::CHARGE:
			return "charge";
		case GenreLexeme::CONTINUE:
			return "continue";
		case GenreLexeme::COROUT:
			return "corout";
		case GenreLexeme::DANS:
			return "dans";
		case GenreLexeme::DIFFERE:
			return "diffère";
		case GenreLexeme::DISCR:
			return "discr";
		case GenreLexeme::DYN:
			return "dyn";
		case GenreLexeme::DELOGE:
			return "déloge";
		case GenreLexeme::EINI:
			return "eini";
		case GenreLexeme::EINI_ERREUR:
			return "eini_erreur";
		case GenreLexeme::EMPL:
			return "empl";
		case GenreLexeme::ERREUR:
			return "erreur";
		case GenreLexeme::EXTERNE:
			return "externe";
		case GenreLexeme::FAUX:
			return "faux";
		case GenreLexeme::FONC:
			return "fonc";
		case GenreLexeme::GARDE:
			return "garde";
		case GenreLexeme::IMPORTE:
			return "importe";
		case GenreLexeme::INFO_DE:
			return "info_de";
		case GenreLexeme::LOGE:
			return "loge";
		case GenreLexeme::MEMOIRE:
			return "mémoire";
		case GenreLexeme::N16:
			return "n16";
		case GenreLexeme::N32:
			return "n32";
		case GenreLexeme::N64:
			return "n64";
		case GenreLexeme::N8:
			return "n8";
		case GenreLexeme::NONATTEIGNABLE:
			return "nonatteignable";
		case GenreLexeme::NONSUR:
			return "nonsûr";
		case GenreLexeme::NUL:
			return "nul";
		case GenreLexeme::OCTET:
			return "octet";
		case GenreLexeme::OPERATEUR:
			return "opérateur";
		case GenreLexeme::PIEGE:
			return "piège";
		case GenreLexeme::POUR:
			return "pour";
		case GenreLexeme::POUSSE_CONTEXTE:
			return "pousse_contexte";
		case GenreLexeme::R16:
			return "r16";
		case GenreLexeme::R32:
			return "r32";
		case GenreLexeme::R64:
			return "r64";
		case GenreLexeme::RELOGE:
			return "reloge";
		case GenreLexeme::RETIENS:
			return "retiens";
		case GenreLexeme::RETOURNE:
			return "retourne";
		case GenreLexeme::RIEN:
			return "rien";
		case GenreLexeme::REPETE:
			return "répète";
		case GenreLexeme::SANSARRET:
			return "sansarrêt";
		case GenreLexeme::SAUFSI:
			return "saufsi";
		case GenreLexeme::SI:
			return "si";
		case GenreLexeme::SINON:
			return "sinon";
		case GenreLexeme::STRUCT:
			return "struct";
		case GenreLexeme::TAILLE_DE:
			return "taille_de";
		case GenreLexeme::TANTQUE:
			return "tantque";
		case GenreLexeme::TENTE:
			return "tente";
		case GenreLexeme::TRANSTYPE:
			return "transtype";
		case GenreLexeme::TYPE_DE:
			return "type_de";
		case GenreLexeme::UNION:
			return "union";
		case GenreLexeme::VRAI:
			return "vrai";
		case GenreLexeme::Z16:
			return "z16";
		case GenreLexeme::Z32:
			return "z32";
		case GenreLexeme::Z64:
			return "z64";
		case GenreLexeme::Z8:
			return "z8";
		case GenreLexeme::ENUM:
			return "énum";
		case GenreLexeme::ENUM_DRAPEAU:
			return "énum_drapeau";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 15;

static bool tables_caracteres[256] = {};
static GenreLexeme tables_identifiants[256] = {};
static bool tables_digraphes[256] = {};
static bool tables_trigraphes[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_digraphes[i] = false;
		tables_trigraphes[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = GenreLexeme::INCONNU;
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
	    auto plg = paires_digraphes.plage();

	    while (!plg.est_finie()) {
		    tables_digraphes[int(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}

    {
	    auto plg = paires_trigraphes.plage();

	    while (!plg.est_finie()) {
		    tables_trigraphes[int(plg.front().premier[0])] = true;
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

bool est_caractere_special(char c, GenreLexeme &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

GenreLexeme id_digraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_digraphes[int(chaine[0])]) {
		return GenreLexeme::INCONNU;
	}

	auto iterateur = paires_digraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return GenreLexeme::INCONNU;
}

GenreLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_trigraphes[int(chaine[0])]) {
		return GenreLexeme::INCONNU;
	}

	auto iterateur = paires_trigraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return GenreLexeme::INCONNU;
}

GenreLexeme id_chaine(const dls::vue_chaine_compacte &chaine)
{
	if (chaine.taille() == 1 || chaine.taille() > TAILLE_MAX_MOT_CLE) {
		return GenreLexeme::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return GenreLexeme::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return GenreLexeme::CHAINE_CARACTERE;
}
