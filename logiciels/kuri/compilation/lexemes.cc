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
	dls::paire{ dls::vue_chaine_compacte("empl"), GenreLexeme::EMPL },
	dls::paire{ dls::vue_chaine_compacte("externe"), GenreLexeme::EXTERNE },
	dls::paire{ dls::vue_chaine_compacte("faux"), GenreLexeme::FAUX },
	dls::paire{ dls::vue_chaine_compacte("fonc"), GenreLexeme::FONC },
	dls::paire{ dls::vue_chaine_compacte("garde"), GenreLexeme::GARDE },
	dls::paire{ dls::vue_chaine_compacte("importe"), GenreLexeme::IMPORTE },
	dls::paire{ dls::vue_chaine_compacte("info_de"), GenreLexeme::INFO_DE },
	dls::paire{ dls::vue_chaine_compacte("loge"), GenreLexeme::LOGE },
	dls::paire{ dls::vue_chaine_compacte("mémoire"), GenreLexeme::MEMOIRE },
	dls::paire{ dls::vue_chaine_compacte("n128"), GenreLexeme::N128 },
	dls::paire{ dls::vue_chaine_compacte("n16"), GenreLexeme::N16 },
	dls::paire{ dls::vue_chaine_compacte("n32"), GenreLexeme::N32 },
	dls::paire{ dls::vue_chaine_compacte("n64"), GenreLexeme::N64 },
	dls::paire{ dls::vue_chaine_compacte("n8"), GenreLexeme::N8 },
	dls::paire{ dls::vue_chaine_compacte("nonsûr"), GenreLexeme::NONSUR },
	dls::paire{ dls::vue_chaine_compacte("nul"), GenreLexeme::NUL },
	dls::paire{ dls::vue_chaine_compacte("octet"), GenreLexeme::OCTET },
	dls::paire{ dls::vue_chaine_compacte("pour"), GenreLexeme::POUR },
	dls::paire{ dls::vue_chaine_compacte("r128"), GenreLexeme::R128 },
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
	dls::paire{ dls::vue_chaine_compacte("transtype"), GenreLexeme::TRANSTYPE },
	dls::paire{ dls::vue_chaine_compacte("type_de"), GenreLexeme::TYPE_DE },
	dls::paire{ dls::vue_chaine_compacte("union"), GenreLexeme::UNION },
	dls::paire{ dls::vue_chaine_compacte("vrai"), GenreLexeme::VRAI },
	dls::paire{ dls::vue_chaine_compacte("z128"), GenreLexeme::Z128 },
	dls::paire{ dls::vue_chaine_compacte("z16"), GenreLexeme::Z16 },
	dls::paire{ dls::vue_chaine_compacte("z32"), GenreLexeme::Z32 },
	dls::paire{ dls::vue_chaine_compacte("z64"), GenreLexeme::Z64 },
	dls::paire{ dls::vue_chaine_compacte("z8"), GenreLexeme::Z8 },
	dls::paire{ dls::vue_chaine_compacte("énum"), GenreLexeme::ENUM },
	dls::paire{ dls::vue_chaine_compacte("énum_drapeau"), GenreLexeme::ENUM_DRAPEAU }
);

static auto paires_digraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("!="), GenreLexeme::DIFFERENCE },
	dls::paire{ dls::vue_chaine_compacte("#!"), GenreLexeme::DIRECTIVE },
	dls::paire{ dls::vue_chaine_compacte("%="), GenreLexeme::MODULO_EGAL },
	dls::paire{ dls::vue_chaine_compacte("&&"), GenreLexeme::ESP_ESP },
	dls::paire{ dls::vue_chaine_compacte("&="), GenreLexeme::ET_EGAL },
	dls::paire{ dls::vue_chaine_compacte("*="), GenreLexeme::MULTIPLIE_EGAL },
	dls::paire{ dls::vue_chaine_compacte("+="), GenreLexeme::PLUS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("-="), GenreLexeme::MOINS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("->"), GenreLexeme::RETOUR_TYPE },
	dls::paire{ dls::vue_chaine_compacte("/="), GenreLexeme::DIVISE_EGAL },
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
	dls::paire{ dls::vue_chaine_compacte("..."), GenreLexeme::TROIS_POINTS },
	dls::paire{ dls::vue_chaine_compacte("<<="), GenreLexeme::DEC_GAUCHE_EGAL },
	dls::paire{ dls::vue_chaine_compacte(">>="), GenreLexeme::DEC_DROITE_EGAL }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '!', GenreLexeme::EXCLAMATION },
	dls::paire{ '"', GenreLexeme::GUILLEMET },
	dls::paire{ '#', GenreLexeme::DIESE },
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

const char *chaine_identifiant(GenreLexeme id)
{
	switch (id) {
		case GenreLexeme::EXCLAMATION:
			return "TypeLexeme::EXCLAMATION";
		case GenreLexeme::GUILLEMET:
			return "TypeLexeme::GUILLEMET";
		case GenreLexeme::DIESE:
			return "TypeLexeme::DIESE";
		case GenreLexeme::DOLLAR:
			return "TypeLexeme::DOLLAR";
		case GenreLexeme::POURCENT:
			return "TypeLexeme::POURCENT";
		case GenreLexeme::ESPERLUETTE:
			return "TypeLexeme::ESPERLUETTE";
		case GenreLexeme::APOSTROPHE:
			return "TypeLexeme::APOSTROPHE";
		case GenreLexeme::PARENTHESE_OUVRANTE:
			return "TypeLexeme::PARENTHESE_OUVRANTE";
		case GenreLexeme::PARENTHESE_FERMANTE:
			return "TypeLexeme::PARENTHESE_FERMANTE";
		case GenreLexeme::FOIS:
			return "TypeLexeme::FOIS";
		case GenreLexeme::PLUS:
			return "TypeLexeme::PLUS";
		case GenreLexeme::VIRGULE:
			return "TypeLexeme::VIRGULE";
		case GenreLexeme::MOINS:
			return "TypeLexeme::MOINS";
		case GenreLexeme::POINT:
			return "TypeLexeme::POINT";
		case GenreLexeme::DIVISE:
			return "TypeLexeme::DIVISE";
		case GenreLexeme::DOUBLE_POINTS:
			return "TypeLexeme::DOUBLE_POINTS";
		case GenreLexeme::POINT_VIRGULE:
			return "TypeLexeme::POINT_VIRGULE";
		case GenreLexeme::INFERIEUR:
			return "TypeLexeme::INFERIEUR";
		case GenreLexeme::EGAL:
			return "TypeLexeme::EGAL";
		case GenreLexeme::SUPERIEUR:
			return "TypeLexeme::SUPERIEUR";
		case GenreLexeme::AROBASE:
			return "TypeLexeme::AROBASE";
		case GenreLexeme::CROCHET_OUVRANT:
			return "TypeLexeme::CROCHET_OUVRANT";
		case GenreLexeme::CROCHET_FERMANT:
			return "TypeLexeme::CROCHET_FERMANT";
		case GenreLexeme::CHAPEAU:
			return "TypeLexeme::CHAPEAU";
		case GenreLexeme::ACCOLADE_OUVRANTE:
			return "TypeLexeme::ACCOLADE_OUVRANTE";
		case GenreLexeme::BARRE:
			return "TypeLexeme::BARRE";
		case GenreLexeme::ACCOLADE_FERMANTE:
			return "TypeLexeme::ACCOLADE_FERMANTE";
		case GenreLexeme::TILDE:
			return "TypeLexeme::TILDE";
		case GenreLexeme::DIFFERENCE:
			return "TypeLexeme::DIFFERENCE";
		case GenreLexeme::DIRECTIVE:
			return "TypeLexeme::DIRECTIVE";
		case GenreLexeme::MODULO_EGAL:
			return "TypeLexeme::MODULO_EGAL";
		case GenreLexeme::ESP_ESP:
			return "TypeLexeme::ESP_ESP";
		case GenreLexeme::ET_EGAL:
			return "TypeLexeme::ET_EGAL";
		case GenreLexeme::MULTIPLIE_EGAL:
			return "TypeLexeme::MULTIPLIE_EGAL";
		case GenreLexeme::PLUS_EGAL:
			return "TypeLexeme::PLUS_EGAL";
		case GenreLexeme::MOINS_EGAL:
			return "TypeLexeme::MOINS_EGAL";
		case GenreLexeme::RETOUR_TYPE:
			return "TypeLexeme::RETOUR_TYPE";
		case GenreLexeme::DIVISE_EGAL:
			return "TypeLexeme::DIVISE_EGAL";
		case GenreLexeme::DECLARATION_VARIABLE:
			return "TypeLexeme::DECLARATION_VARIABLE";
		case GenreLexeme::DECALAGE_GAUCHE:
			return "TypeLexeme::DECALAGE_GAUCHE";
		case GenreLexeme::INFERIEUR_EGAL:
			return "TypeLexeme::INFERIEUR_EGAL";
		case GenreLexeme::EGALITE:
			return "TypeLexeme::EGALITE";
		case GenreLexeme::SUPERIEUR_EGAL:
			return "TypeLexeme::SUPERIEUR_EGAL";
		case GenreLexeme::DECALAGE_DROITE:
			return "TypeLexeme::DECALAGE_DROITE";
		case GenreLexeme::OUX_EGAL:
			return "TypeLexeme::OUX_EGAL";
		case GenreLexeme::OU_EGAL:
			return "TypeLexeme::OU_EGAL";
		case GenreLexeme::BARRE_BARRE:
			return "TypeLexeme::BARRE_BARRE";
		case GenreLexeme::TROIS_POINTS:
			return "TypeLexeme::TROIS_POINTS";
		case GenreLexeme::DEC_GAUCHE_EGAL:
			return "TypeLexeme::DEC_GAUCHE_EGAL";
		case GenreLexeme::DEC_DROITE_EGAL:
			return "TypeLexeme::DEC_DROITE_EGAL";
		case GenreLexeme::ARRETE:
			return "TypeLexeme::ARRETE";
		case GenreLexeme::BOOL:
			return "TypeLexeme::BOOL";
		case GenreLexeme::BOUCLE:
			return "TypeLexeme::BOUCLE";
		case GenreLexeme::CHAINE:
			return "TypeLexeme::CHAINE";
		case GenreLexeme::CHARGE:
			return "TypeLexeme::CHARGE";
		case GenreLexeme::CONTINUE:
			return "TypeLexeme::CONTINUE";
		case GenreLexeme::COROUT:
			return "TypeLexeme::COROUT";
		case GenreLexeme::DANS:
			return "TypeLexeme::DANS";
		case GenreLexeme::DIFFERE:
			return "TypeLexeme::DIFFERE";
		case GenreLexeme::DISCR:
			return "TypeLexeme::DISCR";
		case GenreLexeme::DYN:
			return "TypeLexeme::DYN";
		case GenreLexeme::DELOGE:
			return "TypeLexeme::DELOGE";
		case GenreLexeme::EINI:
			return "TypeLexeme::EINI";
		case GenreLexeme::EMPL:
			return "TypeLexeme::EMPL";
		case GenreLexeme::EXTERNE:
			return "TypeLexeme::EXTERNE";
		case GenreLexeme::FAUX:
			return "TypeLexeme::FAUX";
		case GenreLexeme::FONC:
			return "TypeLexeme::FONC";
		case GenreLexeme::GARDE:
			return "TypeLexeme::GARDE";
		case GenreLexeme::IMPORTE:
			return "TypeLexeme::IMPORTE";
		case GenreLexeme::INFO_DE:
			return "TypeLexeme::INFO_DE";
		case GenreLexeme::LOGE:
			return "TypeLexeme::LOGE";
		case GenreLexeme::MEMOIRE:
			return "TypeLexeme::MEMOIRE";
		case GenreLexeme::N128:
			return "TypeLexeme::N128";
		case GenreLexeme::N16:
			return "TypeLexeme::N16";
		case GenreLexeme::N32:
			return "TypeLexeme::N32";
		case GenreLexeme::N64:
			return "TypeLexeme::N64";
		case GenreLexeme::N8:
			return "TypeLexeme::N8";
		case GenreLexeme::NONSUR:
			return "TypeLexeme::NONSUR";
		case GenreLexeme::NUL:
			return "TypeLexeme::NUL";
		case GenreLexeme::OCTET:
			return "TypeLexeme::OCTET";
		case GenreLexeme::POUR:
			return "TypeLexeme::POUR";
		case GenreLexeme::R128:
			return "TypeLexeme::R128";
		case GenreLexeme::R16:
			return "TypeLexeme::R16";
		case GenreLexeme::R32:
			return "TypeLexeme::R32";
		case GenreLexeme::R64:
			return "TypeLexeme::R64";
		case GenreLexeme::RELOGE:
			return "TypeLexeme::RELOGE";
		case GenreLexeme::RETIENS:
			return "TypeLexeme::RETIENS";
		case GenreLexeme::RETOURNE:
			return "TypeLexeme::RETOURNE";
		case GenreLexeme::RIEN:
			return "TypeLexeme::RIEN";
		case GenreLexeme::REPETE:
			return "TypeLexeme::REPETE";
		case GenreLexeme::SANSARRET:
			return "TypeLexeme::SANSARRET";
		case GenreLexeme::SAUFSI:
			return "TypeLexeme::SAUFSI";
		case GenreLexeme::SI:
			return "TypeLexeme::SI";
		case GenreLexeme::SINON:
			return "TypeLexeme::SINON";
		case GenreLexeme::STRUCT:
			return "TypeLexeme::STRUCT";
		case GenreLexeme::TAILLE_DE:
			return "TypeLexeme::TAILLE_DE";
		case GenreLexeme::TANTQUE:
			return "TypeLexeme::TANTQUE";
		case GenreLexeme::TRANSTYPE:
			return "TypeLexeme::TRANSTYPE";
		case GenreLexeme::TYPE_DE:
			return "TypeLexeme::TYPE_DE";
		case GenreLexeme::UNION:
			return "TypeLexeme::UNION";
		case GenreLexeme::VRAI:
			return "TypeLexeme::VRAI";
		case GenreLexeme::Z128:
			return "TypeLexeme::Z128";
		case GenreLexeme::Z16:
			return "TypeLexeme::Z16";
		case GenreLexeme::Z32:
			return "TypeLexeme::Z32";
		case GenreLexeme::Z64:
			return "TypeLexeme::Z64";
		case GenreLexeme::Z8:
			return "TypeLexeme::Z8";
		case GenreLexeme::ENUM:
			return "TypeLexeme::ENUM";
		case GenreLexeme::ENUM_DRAPEAU:
			return "TypeLexeme::ENUM_DRAPEAU";
		case GenreLexeme::NOMBRE_REEL:
			return "TypeLexeme::NOMBRE_REEL";
		case GenreLexeme::NOMBRE_ENTIER:
			return "TypeLexeme::NOMBRE_ENTIER";
		case GenreLexeme::NOMBRE_HEXADECIMAL:
			return "TypeLexeme::NOMBRE_HEXADECIMAL";
		case GenreLexeme::NOMBRE_OCTAL:
			return "TypeLexeme::NOMBRE_OCTAL";
		case GenreLexeme::NOMBRE_BINAIRE:
			return "TypeLexeme::NOMBRE_BINAIRE";
		case GenreLexeme::PLUS_UNAIRE:
			return "TypeLexeme::PLUS_UNAIRE";
		case GenreLexeme::MOINS_UNAIRE:
			return "TypeLexeme::MOINS_UNAIRE";
		case GenreLexeme::CHAINE_CARACTERE:
			return "TypeLexeme::CHAINE_CARACTERE";
		case GenreLexeme::CHAINE_LITTERALE:
			return "TypeLexeme::CHAINE_LITTERALE";
		case GenreLexeme::CARACTERE:
			return "TypeLexeme::CARACTERE";
		case GenreLexeme::POINTEUR:
			return "TypeLexeme::POINTEUR";
		case GenreLexeme::TABLEAU:
			return "TypeLexeme::TABLEAU";
		case GenreLexeme::REFERENCE:
			return "TypeLexeme::REFERENCE";
		case GenreLexeme::INCONNU:
			return "TypeLexeme::INCONNU";
		case GenreLexeme::CARACTERE_BLANC:
			return "TypeLexeme::CARACTERE_BLANC";
		case GenreLexeme::COMMENTAIRE:
			return "TypeLexeme::COMMENTAIRE";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 13;

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
