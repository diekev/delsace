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
	dls::paire{ dls::vue_chaine_compacte("arrête"), TypeLexeme::ARRETE },
	dls::paire{ dls::vue_chaine_compacte("bool"), TypeLexeme::BOOL },
	dls::paire{ dls::vue_chaine_compacte("boucle"), TypeLexeme::BOUCLE },
	dls::paire{ dls::vue_chaine_compacte("chaine"), TypeLexeme::CHAINE },
	dls::paire{ dls::vue_chaine_compacte("charge"), TypeLexeme::CHARGE },
	dls::paire{ dls::vue_chaine_compacte("continue"), TypeLexeme::CONTINUE },
	dls::paire{ dls::vue_chaine_compacte("corout"), TypeLexeme::COROUT },
	dls::paire{ dls::vue_chaine_compacte("dans"), TypeLexeme::DANS },
	dls::paire{ dls::vue_chaine_compacte("diffère"), TypeLexeme::DIFFERE },
	dls::paire{ dls::vue_chaine_compacte("discr"), TypeLexeme::DISCR },
	dls::paire{ dls::vue_chaine_compacte("dyn"), TypeLexeme::DYN },
	dls::paire{ dls::vue_chaine_compacte("déloge"), TypeLexeme::DELOGE },
	dls::paire{ dls::vue_chaine_compacte("eini"), TypeLexeme::EINI },
	dls::paire{ dls::vue_chaine_compacte("empl"), TypeLexeme::EMPL },
	dls::paire{ dls::vue_chaine_compacte("externe"), TypeLexeme::EXTERNE },
	dls::paire{ dls::vue_chaine_compacte("faux"), TypeLexeme::FAUX },
	dls::paire{ dls::vue_chaine_compacte("fonc"), TypeLexeme::FONC },
	dls::paire{ dls::vue_chaine_compacte("garde"), TypeLexeme::GARDE },
	dls::paire{ dls::vue_chaine_compacte("importe"), TypeLexeme::IMPORTE },
	dls::paire{ dls::vue_chaine_compacte("info_de"), TypeLexeme::INFO_DE },
	dls::paire{ dls::vue_chaine_compacte("loge"), TypeLexeme::LOGE },
	dls::paire{ dls::vue_chaine_compacte("mémoire"), TypeLexeme::MEMOIRE },
	dls::paire{ dls::vue_chaine_compacte("n128"), TypeLexeme::N128 },
	dls::paire{ dls::vue_chaine_compacte("n16"), TypeLexeme::N16 },
	dls::paire{ dls::vue_chaine_compacte("n32"), TypeLexeme::N32 },
	dls::paire{ dls::vue_chaine_compacte("n64"), TypeLexeme::N64 },
	dls::paire{ dls::vue_chaine_compacte("n8"), TypeLexeme::N8 },
	dls::paire{ dls::vue_chaine_compacte("nonsûr"), TypeLexeme::NONSUR },
	dls::paire{ dls::vue_chaine_compacte("nul"), TypeLexeme::NUL },
	dls::paire{ dls::vue_chaine_compacte("octet"), TypeLexeme::OCTET },
	dls::paire{ dls::vue_chaine_compacte("pour"), TypeLexeme::POUR },
	dls::paire{ dls::vue_chaine_compacte("r128"), TypeLexeme::R128 },
	dls::paire{ dls::vue_chaine_compacte("r16"), TypeLexeme::R16 },
	dls::paire{ dls::vue_chaine_compacte("r32"), TypeLexeme::R32 },
	dls::paire{ dls::vue_chaine_compacte("r64"), TypeLexeme::R64 },
	dls::paire{ dls::vue_chaine_compacte("reloge"), TypeLexeme::RELOGE },
	dls::paire{ dls::vue_chaine_compacte("retiens"), TypeLexeme::RETIENS },
	dls::paire{ dls::vue_chaine_compacte("retourne"), TypeLexeme::RETOURNE },
	dls::paire{ dls::vue_chaine_compacte("rien"), TypeLexeme::RIEN },
	dls::paire{ dls::vue_chaine_compacte("répète"), TypeLexeme::REPETE },
	dls::paire{ dls::vue_chaine_compacte("sansarrêt"), TypeLexeme::SANSARRET },
	dls::paire{ dls::vue_chaine_compacte("saufsi"), TypeLexeme::SAUFSI },
	dls::paire{ dls::vue_chaine_compacte("si"), TypeLexeme::SI },
	dls::paire{ dls::vue_chaine_compacte("sinon"), TypeLexeme::SINON },
	dls::paire{ dls::vue_chaine_compacte("struct"), TypeLexeme::STRUCT },
	dls::paire{ dls::vue_chaine_compacte("taille_de"), TypeLexeme::TAILLE_DE },
	dls::paire{ dls::vue_chaine_compacte("tantque"), TypeLexeme::TANTQUE },
	dls::paire{ dls::vue_chaine_compacte("transtype"), TypeLexeme::TRANSTYPE },
	dls::paire{ dls::vue_chaine_compacte("type_de"), TypeLexeme::TYPE_DE },
	dls::paire{ dls::vue_chaine_compacte("union"), TypeLexeme::UNION },
	dls::paire{ dls::vue_chaine_compacte("vrai"), TypeLexeme::VRAI },
	dls::paire{ dls::vue_chaine_compacte("z128"), TypeLexeme::Z128 },
	dls::paire{ dls::vue_chaine_compacte("z16"), TypeLexeme::Z16 },
	dls::paire{ dls::vue_chaine_compacte("z32"), TypeLexeme::Z32 },
	dls::paire{ dls::vue_chaine_compacte("z64"), TypeLexeme::Z64 },
	dls::paire{ dls::vue_chaine_compacte("z8"), TypeLexeme::Z8 },
	dls::paire{ dls::vue_chaine_compacte("énum"), TypeLexeme::ENUM },
	dls::paire{ dls::vue_chaine_compacte("énum_drapeau"), TypeLexeme::ENUM_DRAPEAU }
);

static auto paires_digraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("!="), TypeLexeme::DIFFERENCE },
	dls::paire{ dls::vue_chaine_compacte("#!"), TypeLexeme::DIRECTIVE },
	dls::paire{ dls::vue_chaine_compacte("%="), TypeLexeme::MODULO_EGAL },
	dls::paire{ dls::vue_chaine_compacte("&&"), TypeLexeme::ESP_ESP },
	dls::paire{ dls::vue_chaine_compacte("&="), TypeLexeme::ET_EGAL },
	dls::paire{ dls::vue_chaine_compacte("*="), TypeLexeme::MULTIPLIE_EGAL },
	dls::paire{ dls::vue_chaine_compacte("+="), TypeLexeme::PLUS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("-="), TypeLexeme::MOINS_EGAL },
	dls::paire{ dls::vue_chaine_compacte("->"), TypeLexeme::RETOUR_TYPE },
	dls::paire{ dls::vue_chaine_compacte("/="), TypeLexeme::DIVISE_EGAL },
	dls::paire{ dls::vue_chaine_compacte(":="), TypeLexeme::DECLARATION_VARIABLE },
	dls::paire{ dls::vue_chaine_compacte("<<"), TypeLexeme::DECALAGE_GAUCHE },
	dls::paire{ dls::vue_chaine_compacte("<="), TypeLexeme::INFERIEUR_EGAL },
	dls::paire{ dls::vue_chaine_compacte("=="), TypeLexeme::EGALITE },
	dls::paire{ dls::vue_chaine_compacte(">="), TypeLexeme::SUPERIEUR_EGAL },
	dls::paire{ dls::vue_chaine_compacte(">>"), TypeLexeme::DECALAGE_DROITE },
	dls::paire{ dls::vue_chaine_compacte("^="), TypeLexeme::OUX_EGAL },
	dls::paire{ dls::vue_chaine_compacte("|="), TypeLexeme::OU_EGAL },
	dls::paire{ dls::vue_chaine_compacte("||"), TypeLexeme::BARRE_BARRE }
);

static auto paires_trigraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine_compacte("..."), TypeLexeme::TROIS_POINTS },
	dls::paire{ dls::vue_chaine_compacte("<<="), TypeLexeme::DEC_GAUCHE_EGAL },
	dls::paire{ dls::vue_chaine_compacte(">>="), TypeLexeme::DEC_DROITE_EGAL }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '!', TypeLexeme::EXCLAMATION },
	dls::paire{ '"', TypeLexeme::GUILLEMET },
	dls::paire{ '#', TypeLexeme::DIESE },
	dls::paire{ '$', TypeLexeme::DOLLAR },
	dls::paire{ '%', TypeLexeme::POURCENT },
	dls::paire{ '&', TypeLexeme::ESPERLUETTE },
	dls::paire{ '\'', TypeLexeme::APOSTROPHE },
	dls::paire{ '(', TypeLexeme::PARENTHESE_OUVRANTE },
	dls::paire{ ')', TypeLexeme::PARENTHESE_FERMANTE },
	dls::paire{ '*', TypeLexeme::FOIS },
	dls::paire{ '+', TypeLexeme::PLUS },
	dls::paire{ ',', TypeLexeme::VIRGULE },
	dls::paire{ '-', TypeLexeme::MOINS },
	dls::paire{ '.', TypeLexeme::POINT },
	dls::paire{ '/', TypeLexeme::DIVISE },
	dls::paire{ ':', TypeLexeme::DOUBLE_POINTS },
	dls::paire{ ';', TypeLexeme::POINT_VIRGULE },
	dls::paire{ '<', TypeLexeme::INFERIEUR },
	dls::paire{ '=', TypeLexeme::EGAL },
	dls::paire{ '>', TypeLexeme::SUPERIEUR },
	dls::paire{ '@', TypeLexeme::AROBASE },
	dls::paire{ '[', TypeLexeme::CROCHET_OUVRANT },
	dls::paire{ ']', TypeLexeme::CROCHET_FERMANT },
	dls::paire{ '^', TypeLexeme::CHAPEAU },
	dls::paire{ '{', TypeLexeme::ACCOLADE_OUVRANTE },
	dls::paire{ '|', TypeLexeme::BARRE },
	dls::paire{ '}', TypeLexeme::ACCOLADE_FERMANTE },
	dls::paire{ '~', TypeLexeme::TILDE }
);

const char *chaine_identifiant(TypeLexeme id)
{
	switch (id) {
		case TypeLexeme::EXCLAMATION:
			return "TypeLexeme::EXCLAMATION";
		case TypeLexeme::GUILLEMET:
			return "TypeLexeme::GUILLEMET";
		case TypeLexeme::DIESE:
			return "TypeLexeme::DIESE";
		case TypeLexeme::DOLLAR:
			return "TypeLexeme::DOLLAR";
		case TypeLexeme::POURCENT:
			return "TypeLexeme::POURCENT";
		case TypeLexeme::ESPERLUETTE:
			return "TypeLexeme::ESPERLUETTE";
		case TypeLexeme::APOSTROPHE:
			return "TypeLexeme::APOSTROPHE";
		case TypeLexeme::PARENTHESE_OUVRANTE:
			return "TypeLexeme::PARENTHESE_OUVRANTE";
		case TypeLexeme::PARENTHESE_FERMANTE:
			return "TypeLexeme::PARENTHESE_FERMANTE";
		case TypeLexeme::FOIS:
			return "TypeLexeme::FOIS";
		case TypeLexeme::PLUS:
			return "TypeLexeme::PLUS";
		case TypeLexeme::VIRGULE:
			return "TypeLexeme::VIRGULE";
		case TypeLexeme::MOINS:
			return "TypeLexeme::MOINS";
		case TypeLexeme::POINT:
			return "TypeLexeme::POINT";
		case TypeLexeme::DIVISE:
			return "TypeLexeme::DIVISE";
		case TypeLexeme::DOUBLE_POINTS:
			return "TypeLexeme::DOUBLE_POINTS";
		case TypeLexeme::POINT_VIRGULE:
			return "TypeLexeme::POINT_VIRGULE";
		case TypeLexeme::INFERIEUR:
			return "TypeLexeme::INFERIEUR";
		case TypeLexeme::EGAL:
			return "TypeLexeme::EGAL";
		case TypeLexeme::SUPERIEUR:
			return "TypeLexeme::SUPERIEUR";
		case TypeLexeme::AROBASE:
			return "TypeLexeme::AROBASE";
		case TypeLexeme::CROCHET_OUVRANT:
			return "TypeLexeme::CROCHET_OUVRANT";
		case TypeLexeme::CROCHET_FERMANT:
			return "TypeLexeme::CROCHET_FERMANT";
		case TypeLexeme::CHAPEAU:
			return "TypeLexeme::CHAPEAU";
		case TypeLexeme::ACCOLADE_OUVRANTE:
			return "TypeLexeme::ACCOLADE_OUVRANTE";
		case TypeLexeme::BARRE:
			return "TypeLexeme::BARRE";
		case TypeLexeme::ACCOLADE_FERMANTE:
			return "TypeLexeme::ACCOLADE_FERMANTE";
		case TypeLexeme::TILDE:
			return "TypeLexeme::TILDE";
		case TypeLexeme::DIFFERENCE:
			return "TypeLexeme::DIFFERENCE";
		case TypeLexeme::DIRECTIVE:
			return "TypeLexeme::DIRECTIVE";
		case TypeLexeme::MODULO_EGAL:
			return "TypeLexeme::MODULO_EGAL";
		case TypeLexeme::ESP_ESP:
			return "TypeLexeme::ESP_ESP";
		case TypeLexeme::ET_EGAL:
			return "TypeLexeme::ET_EGAL";
		case TypeLexeme::MULTIPLIE_EGAL:
			return "TypeLexeme::MULTIPLIE_EGAL";
		case TypeLexeme::PLUS_EGAL:
			return "TypeLexeme::PLUS_EGAL";
		case TypeLexeme::MOINS_EGAL:
			return "TypeLexeme::MOINS_EGAL";
		case TypeLexeme::RETOUR_TYPE:
			return "TypeLexeme::RETOUR_TYPE";
		case TypeLexeme::DIVISE_EGAL:
			return "TypeLexeme::DIVISE_EGAL";
		case TypeLexeme::DECLARATION_VARIABLE:
			return "TypeLexeme::DECLARATION_VARIABLE";
		case TypeLexeme::DECALAGE_GAUCHE:
			return "TypeLexeme::DECALAGE_GAUCHE";
		case TypeLexeme::INFERIEUR_EGAL:
			return "TypeLexeme::INFERIEUR_EGAL";
		case TypeLexeme::EGALITE:
			return "TypeLexeme::EGALITE";
		case TypeLexeme::SUPERIEUR_EGAL:
			return "TypeLexeme::SUPERIEUR_EGAL";
		case TypeLexeme::DECALAGE_DROITE:
			return "TypeLexeme::DECALAGE_DROITE";
		case TypeLexeme::OUX_EGAL:
			return "TypeLexeme::OUX_EGAL";
		case TypeLexeme::OU_EGAL:
			return "TypeLexeme::OU_EGAL";
		case TypeLexeme::BARRE_BARRE:
			return "TypeLexeme::BARRE_BARRE";
		case TypeLexeme::TROIS_POINTS:
			return "TypeLexeme::TROIS_POINTS";
		case TypeLexeme::DEC_GAUCHE_EGAL:
			return "TypeLexeme::DEC_GAUCHE_EGAL";
		case TypeLexeme::DEC_DROITE_EGAL:
			return "TypeLexeme::DEC_DROITE_EGAL";
		case TypeLexeme::ARRETE:
			return "TypeLexeme::ARRETE";
		case TypeLexeme::BOOL:
			return "TypeLexeme::BOOL";
		case TypeLexeme::BOUCLE:
			return "TypeLexeme::BOUCLE";
		case TypeLexeme::CHAINE:
			return "TypeLexeme::CHAINE";
		case TypeLexeme::CHARGE:
			return "TypeLexeme::CHARGE";
		case TypeLexeme::CONTINUE:
			return "TypeLexeme::CONTINUE";
		case TypeLexeme::COROUT:
			return "TypeLexeme::COROUT";
		case TypeLexeme::DANS:
			return "TypeLexeme::DANS";
		case TypeLexeme::DIFFERE:
			return "TypeLexeme::DIFFERE";
		case TypeLexeme::DISCR:
			return "TypeLexeme::DISCR";
		case TypeLexeme::DYN:
			return "TypeLexeme::DYN";
		case TypeLexeme::DELOGE:
			return "TypeLexeme::DELOGE";
		case TypeLexeme::EINI:
			return "TypeLexeme::EINI";
		case TypeLexeme::EMPL:
			return "TypeLexeme::EMPL";
		case TypeLexeme::EXTERNE:
			return "TypeLexeme::EXTERNE";
		case TypeLexeme::FAUX:
			return "TypeLexeme::FAUX";
		case TypeLexeme::FONC:
			return "TypeLexeme::FONC";
		case TypeLexeme::GARDE:
			return "TypeLexeme::GARDE";
		case TypeLexeme::IMPORTE:
			return "TypeLexeme::IMPORTE";
		case TypeLexeme::INFO_DE:
			return "TypeLexeme::INFO_DE";
		case TypeLexeme::LOGE:
			return "TypeLexeme::LOGE";
		case TypeLexeme::MEMOIRE:
			return "TypeLexeme::MEMOIRE";
		case TypeLexeme::N128:
			return "TypeLexeme::N128";
		case TypeLexeme::N16:
			return "TypeLexeme::N16";
		case TypeLexeme::N32:
			return "TypeLexeme::N32";
		case TypeLexeme::N64:
			return "TypeLexeme::N64";
		case TypeLexeme::N8:
			return "TypeLexeme::N8";
		case TypeLexeme::NONSUR:
			return "TypeLexeme::NONSUR";
		case TypeLexeme::NUL:
			return "TypeLexeme::NUL";
		case TypeLexeme::OCTET:
			return "TypeLexeme::OCTET";
		case TypeLexeme::POUR:
			return "TypeLexeme::POUR";
		case TypeLexeme::R128:
			return "TypeLexeme::R128";
		case TypeLexeme::R16:
			return "TypeLexeme::R16";
		case TypeLexeme::R32:
			return "TypeLexeme::R32";
		case TypeLexeme::R64:
			return "TypeLexeme::R64";
		case TypeLexeme::RELOGE:
			return "TypeLexeme::RELOGE";
		case TypeLexeme::RETIENS:
			return "TypeLexeme::RETIENS";
		case TypeLexeme::RETOURNE:
			return "TypeLexeme::RETOURNE";
		case TypeLexeme::RIEN:
			return "TypeLexeme::RIEN";
		case TypeLexeme::REPETE:
			return "TypeLexeme::REPETE";
		case TypeLexeme::SANSARRET:
			return "TypeLexeme::SANSARRET";
		case TypeLexeme::SAUFSI:
			return "TypeLexeme::SAUFSI";
		case TypeLexeme::SI:
			return "TypeLexeme::SI";
		case TypeLexeme::SINON:
			return "TypeLexeme::SINON";
		case TypeLexeme::STRUCT:
			return "TypeLexeme::STRUCT";
		case TypeLexeme::TAILLE_DE:
			return "TypeLexeme::TAILLE_DE";
		case TypeLexeme::TANTQUE:
			return "TypeLexeme::TANTQUE";
		case TypeLexeme::TRANSTYPE:
			return "TypeLexeme::TRANSTYPE";
		case TypeLexeme::TYPE_DE:
			return "TypeLexeme::TYPE_DE";
		case TypeLexeme::UNION:
			return "TypeLexeme::UNION";
		case TypeLexeme::VRAI:
			return "TypeLexeme::VRAI";
		case TypeLexeme::Z128:
			return "TypeLexeme::Z128";
		case TypeLexeme::Z16:
			return "TypeLexeme::Z16";
		case TypeLexeme::Z32:
			return "TypeLexeme::Z32";
		case TypeLexeme::Z64:
			return "TypeLexeme::Z64";
		case TypeLexeme::Z8:
			return "TypeLexeme::Z8";
		case TypeLexeme::ENUM:
			return "TypeLexeme::ENUM";
		case TypeLexeme::ENUM_DRAPEAU:
			return "TypeLexeme::ENUM_DRAPEAU";
		case TypeLexeme::NOMBRE_REEL:
			return "TypeLexeme::NOMBRE_REEL";
		case TypeLexeme::NOMBRE_ENTIER:
			return "TypeLexeme::NOMBRE_ENTIER";
		case TypeLexeme::NOMBRE_HEXADECIMAL:
			return "TypeLexeme::NOMBRE_HEXADECIMAL";
		case TypeLexeme::NOMBRE_OCTAL:
			return "TypeLexeme::NOMBRE_OCTAL";
		case TypeLexeme::NOMBRE_BINAIRE:
			return "TypeLexeme::NOMBRE_BINAIRE";
		case TypeLexeme::PLUS_UNAIRE:
			return "TypeLexeme::PLUS_UNAIRE";
		case TypeLexeme::MOINS_UNAIRE:
			return "TypeLexeme::MOINS_UNAIRE";
		case TypeLexeme::CHAINE_CARACTERE:
			return "TypeLexeme::CHAINE_CARACTERE";
		case TypeLexeme::CHAINE_LITTERALE:
			return "TypeLexeme::CHAINE_LITTERALE";
		case TypeLexeme::CARACTERE:
			return "TypeLexeme::CARACTERE";
		case TypeLexeme::POINTEUR:
			return "TypeLexeme::POINTEUR";
		case TypeLexeme::TABLEAU:
			return "TypeLexeme::TABLEAU";
		case TypeLexeme::REFERENCE:
			return "TypeLexeme::REFERENCE";
		case TypeLexeme::INCONNU:
			return "TypeLexeme::INCONNU";
		case TypeLexeme::CARACTERE_BLANC:
			return "TypeLexeme::CARACTERE_BLANC";
		case TypeLexeme::COMMENTAIRE:
			return "TypeLexeme::COMMENTAIRE";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 13;

static bool tables_caracteres[256] = {};
static TypeLexeme tables_identifiants[256] = {};
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
		tables_identifiants[i] = TypeLexeme::INCONNU;
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

bool est_caractere_special(char c, TypeLexeme &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

TypeLexeme id_digraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_digraphes[int(chaine[0])]) {
		return TypeLexeme::INCONNU;
	}

	auto iterateur = paires_digraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::INCONNU;
}

TypeLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_trigraphes[int(chaine[0])]) {
		return TypeLexeme::INCONNU;
	}

	auto iterateur = paires_trigraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::INCONNU;
}

TypeLexeme id_chaine(const dls::vue_chaine_compacte &chaine)
{
	if (chaine.taille() == 1 || chaine.taille() > TAILLE_MAX_MOT_CLE) {
		return TypeLexeme::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return TypeLexeme::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::CHAINE_CARACTERE;
}
