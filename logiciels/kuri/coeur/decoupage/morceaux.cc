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
	dls::paire{ dls::vue_chaine("arrête"), id_morceau::ARRETE },
	dls::paire{ dls::vue_chaine("associe"), id_morceau::ASSOCIE },
	dls::paire{ dls::vue_chaine("bool"), id_morceau::BOOL },
	dls::paire{ dls::vue_chaine("boucle"), id_morceau::BOUCLE },
	dls::paire{ dls::vue_chaine("chaine"), id_morceau::CHAINE },
	dls::paire{ dls::vue_chaine("continue"), id_morceau::CONTINUE },
	dls::paire{ dls::vue_chaine("corout"), id_morceau::COROUT },
	dls::paire{ dls::vue_chaine("dans"), id_morceau::DANS },
	dls::paire{ dls::vue_chaine("de"), id_morceau::DE },
	dls::paire{ dls::vue_chaine("diffère"), id_morceau::DIFFERE },
	dls::paire{ dls::vue_chaine("dyn"), id_morceau::DYN },
	dls::paire{ dls::vue_chaine("déloge"), id_morceau::DELOGE },
	dls::paire{ dls::vue_chaine("eini"), id_morceau::EINI },
	dls::paire{ dls::vue_chaine("empl"), id_morceau::EMPL },
	dls::paire{ dls::vue_chaine("externe"), id_morceau::EXTERNE },
	dls::paire{ dls::vue_chaine("faux"), id_morceau::FAUX },
	dls::paire{ dls::vue_chaine("fonc"), id_morceau::FONC },
	dls::paire{ dls::vue_chaine("gabarit"), id_morceau::GABARIT },
	dls::paire{ dls::vue_chaine("garde"), id_morceau::GARDE },
	dls::paire{ dls::vue_chaine("importe"), id_morceau::IMPORTE },
	dls::paire{ dls::vue_chaine("info_de"), id_morceau::INFO_DE },
	dls::paire{ dls::vue_chaine("loge"), id_morceau::LOGE },
	dls::paire{ dls::vue_chaine("mémoire"), id_morceau::MEMOIRE },
	dls::paire{ dls::vue_chaine("n16"), id_morceau::N16 },
	dls::paire{ dls::vue_chaine("n32"), id_morceau::N32 },
	dls::paire{ dls::vue_chaine("n64"), id_morceau::N64 },
	dls::paire{ dls::vue_chaine("n8"), id_morceau::N8 },
	dls::paire{ dls::vue_chaine("nonsûr"), id_morceau::NONSUR },
	dls::paire{ dls::vue_chaine("nul"), id_morceau::NUL },
	dls::paire{ dls::vue_chaine("octet"), id_morceau::OCTET },
	dls::paire{ dls::vue_chaine("pour"), id_morceau::POUR },
	dls::paire{ dls::vue_chaine("r16"), id_morceau::R16 },
	dls::paire{ dls::vue_chaine("r32"), id_morceau::R32 },
	dls::paire{ dls::vue_chaine("r64"), id_morceau::R64 },
	dls::paire{ dls::vue_chaine("reloge"), id_morceau::RELOGE },
	dls::paire{ dls::vue_chaine("retiens"), id_morceau::RETIENS },
	dls::paire{ dls::vue_chaine("retourne"), id_morceau::RETOURNE },
	dls::paire{ dls::vue_chaine("rien"), id_morceau::RIEN },
	dls::paire{ dls::vue_chaine("sansarrêt"), id_morceau::SANSARRET },
	dls::paire{ dls::vue_chaine("saufsi"), id_morceau::SAUFSI },
	dls::paire{ dls::vue_chaine("si"), id_morceau::SI },
	dls::paire{ dls::vue_chaine("sinon"), id_morceau::SINON },
	dls::paire{ dls::vue_chaine("soit"), id_morceau::SOIT },
	dls::paire{ dls::vue_chaine("structure"), id_morceau::STRUCTURE },
	dls::paire{ dls::vue_chaine("taille_de"), id_morceau::TAILLE_DE },
	dls::paire{ dls::vue_chaine("tantque"), id_morceau::TANTQUE },
	dls::paire{ dls::vue_chaine("transtype"), id_morceau::TRANSTYPE },
	dls::paire{ dls::vue_chaine("type_de"), id_morceau::TYPE_DE },
	dls::paire{ dls::vue_chaine("vrai"), id_morceau::VRAI },
	dls::paire{ dls::vue_chaine("z16"), id_morceau::Z16 },
	dls::paire{ dls::vue_chaine("z32"), id_morceau::Z32 },
	dls::paire{ dls::vue_chaine("z64"), id_morceau::Z64 },
	dls::paire{ dls::vue_chaine("z8"), id_morceau::Z8 },
	dls::paire{ dls::vue_chaine("énum"), id_morceau::ENUM }
);

static auto paires_digraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine("!="), id_morceau::DIFFERENCE },
	dls::paire{ dls::vue_chaine("#!"), id_morceau::DIRECTIVE },
	dls::paire{ dls::vue_chaine("%="), id_morceau::MODULO_EGAL },
	dls::paire{ dls::vue_chaine("&&"), id_morceau::ESP_ESP },
	dls::paire{ dls::vue_chaine("&="), id_morceau::ET_EGAL },
	dls::paire{ dls::vue_chaine("*="), id_morceau::MULTIPLIE_EGAL },
	dls::paire{ dls::vue_chaine("+="), id_morceau::PLUS_EGAL },
	dls::paire{ dls::vue_chaine("-="), id_morceau::MOINS_EGAL },
	dls::paire{ dls::vue_chaine("/="), id_morceau::DIVISE_EGAL },
	dls::paire{ dls::vue_chaine("<<"), id_morceau::DECALAGE_GAUCHE },
	dls::paire{ dls::vue_chaine("<="), id_morceau::INFERIEUR_EGAL },
	dls::paire{ dls::vue_chaine("=="), id_morceau::EGALITE },
	dls::paire{ dls::vue_chaine(">="), id_morceau::SUPERIEUR_EGAL },
	dls::paire{ dls::vue_chaine(">>"), id_morceau::DECALAGE_DROITE },
	dls::paire{ dls::vue_chaine("^="), id_morceau::OUX_EGAL },
	dls::paire{ dls::vue_chaine("|="), id_morceau::OU_EGAL },
	dls::paire{ dls::vue_chaine("||"), id_morceau::BARRE_BARRE }
);

static auto paires_trigraphes = dls::cree_dico(
	dls::paire{ dls::vue_chaine("..."), id_morceau::TROIS_POINTS },
	dls::paire{ dls::vue_chaine("<<="), id_morceau::DEC_GAUCHE_EGAL },
	dls::paire{ dls::vue_chaine(">>="), id_morceau::DEC_DROITE_EGAL }
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
		case id_morceau::DIRECTIVE:
			return "id_morceau::DIRECTIVE";
		case id_morceau::MODULO_EGAL:
			return "id_morceau::MODULO_EGAL";
		case id_morceau::ESP_ESP:
			return "id_morceau::ESP_ESP";
		case id_morceau::ET_EGAL:
			return "id_morceau::ET_EGAL";
		case id_morceau::MULTIPLIE_EGAL:
			return "id_morceau::MULTIPLIE_EGAL";
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
		case id_morceau::OUX_EGAL:
			return "id_morceau::OUX_EGAL";
		case id_morceau::OU_EGAL:
			return "id_morceau::OU_EGAL";
		case id_morceau::BARRE_BARRE:
			return "id_morceau::BARRE_BARRE";
		case id_morceau::TROIS_POINTS:
			return "id_morceau::TROIS_POINTS";
		case id_morceau::DEC_GAUCHE_EGAL:
			return "id_morceau::DEC_GAUCHE_EGAL";
		case id_morceau::DEC_DROITE_EGAL:
			return "id_morceau::DEC_DROITE_EGAL";
		case id_morceau::ARRETE:
			return "id_morceau::ARRETE";
		case id_morceau::ASSOCIE:
			return "id_morceau::ASSOCIE";
		case id_morceau::BOOL:
			return "id_morceau::BOOL";
		case id_morceau::BOUCLE:
			return "id_morceau::BOUCLE";
		case id_morceau::CHAINE:
			return "id_morceau::CHAINE";
		case id_morceau::CONTINUE:
			return "id_morceau::CONTINUE";
		case id_morceau::COROUT:
			return "id_morceau::COROUT";
		case id_morceau::DANS:
			return "id_morceau::DANS";
		case id_morceau::DE:
			return "id_morceau::DE";
		case id_morceau::DIFFERE:
			return "id_morceau::DIFFERE";
		case id_morceau::DYN:
			return "id_morceau::DYN";
		case id_morceau::DELOGE:
			return "id_morceau::DELOGE";
		case id_morceau::EINI:
			return "id_morceau::EINI";
		case id_morceau::EMPL:
			return "id_morceau::EMPL";
		case id_morceau::EXTERNE:
			return "id_morceau::EXTERNE";
		case id_morceau::FAUX:
			return "id_morceau::FAUX";
		case id_morceau::FONC:
			return "id_morceau::FONC";
		case id_morceau::GABARIT:
			return "id_morceau::GABARIT";
		case id_morceau::GARDE:
			return "id_morceau::GARDE";
		case id_morceau::IMPORTE:
			return "id_morceau::IMPORTE";
		case id_morceau::INFO_DE:
			return "id_morceau::INFO_DE";
		case id_morceau::LOGE:
			return "id_morceau::LOGE";
		case id_morceau::MEMOIRE:
			return "id_morceau::MEMOIRE";
		case id_morceau::N16:
			return "id_morceau::N16";
		case id_morceau::N32:
			return "id_morceau::N32";
		case id_morceau::N64:
			return "id_morceau::N64";
		case id_morceau::N8:
			return "id_morceau::N8";
		case id_morceau::NONSUR:
			return "id_morceau::NONSUR";
		case id_morceau::NUL:
			return "id_morceau::NUL";
		case id_morceau::OCTET:
			return "id_morceau::OCTET";
		case id_morceau::POUR:
			return "id_morceau::POUR";
		case id_morceau::R16:
			return "id_morceau::R16";
		case id_morceau::R32:
			return "id_morceau::R32";
		case id_morceau::R64:
			return "id_morceau::R64";
		case id_morceau::RELOGE:
			return "id_morceau::RELOGE";
		case id_morceau::RETIENS:
			return "id_morceau::RETIENS";
		case id_morceau::RETOURNE:
			return "id_morceau::RETOURNE";
		case id_morceau::RIEN:
			return "id_morceau::RIEN";
		case id_morceau::SANSARRET:
			return "id_morceau::SANSARRET";
		case id_morceau::SAUFSI:
			return "id_morceau::SAUFSI";
		case id_morceau::SI:
			return "id_morceau::SI";
		case id_morceau::SINON:
			return "id_morceau::SINON";
		case id_morceau::SOIT:
			return "id_morceau::SOIT";
		case id_morceau::STRUCTURE:
			return "id_morceau::STRUCTURE";
		case id_morceau::TAILLE_DE:
			return "id_morceau::TAILLE_DE";
		case id_morceau::TANTQUE:
			return "id_morceau::TANTQUE";
		case id_morceau::TRANSTYPE:
			return "id_morceau::TRANSTYPE";
		case id_morceau::TYPE_DE:
			return "id_morceau::TYPE_DE";
		case id_morceau::VRAI:
			return "id_morceau::VRAI";
		case id_morceau::Z16:
			return "id_morceau::Z16";
		case id_morceau::Z32:
			return "id_morceau::Z32";
		case id_morceau::Z64:
			return "id_morceau::Z64";
		case id_morceau::Z8:
			return "id_morceau::Z8";
		case id_morceau::ENUM:
			return "id_morceau::ENUM";
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
		case id_morceau::CHAINE_CARACTERE:
			return "id_morceau::CHAINE_CARACTERE";
		case id_morceau::CHAINE_LITTERALE:
			return "id_morceau::CHAINE_LITTERALE";
		case id_morceau::CARACTERE:
			return "id_morceau::CARACTERE";
		case id_morceau::POINTEUR:
			return "id_morceau::POINTEUR";
		case id_morceau::TABLEAU:
			return "id_morceau::TABLEAU";
		case id_morceau::REFERENCE:
			return "id_morceau::REFERENCE";
		case id_morceau::INCONNU:
			return "id_morceau::INCONNU";
		case id_morceau::CARACTERE_BLANC:
			return "id_morceau::CARACTERE_BLANC";
		case id_morceau::COMMENTAIRE:
			return "id_morceau::COMMENTAIRE";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 10;

static bool tables_caracteres[256] = {};
static id_morceau tables_identifiants[256] = {};
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

bool est_caractere_special(char c, id_morceau &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

id_morceau id_digraphe(const dls::vue_chaine &chaine)
{
	if (!tables_digraphes[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_digraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return id_morceau::INCONNU;
}

id_morceau id_trigraphe(const dls::vue_chaine &chaine)
{
	if (!tables_trigraphes[int(chaine[0])]) {
		return id_morceau::INCONNU;
	}

	auto iterateur = paires_trigraphes.trouve_binaire(chaine);

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
