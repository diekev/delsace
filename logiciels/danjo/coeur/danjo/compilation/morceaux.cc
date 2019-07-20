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
 
#include "morceaux.h"

#include "biblinternes/structures/dico_fixe.hh"

namespace danjo {

static auto paires_mots_cles = dls::cree_dico(
	dls::paire{ dls::vue_chaine("action"), id_morceau::ACTION },
	dls::paire{ dls::vue_chaine("attache"), id_morceau::ATTACHE },
	dls::paire{ dls::vue_chaine("barre_outils"), id_morceau::BARRE_OUTILS },
	dls::paire{ dls::vue_chaine("bouton"), id_morceau::BOUTON },
	dls::paire{ dls::vue_chaine("case"), id_morceau::CASE },
	dls::paire{ dls::vue_chaine("chaine"), id_morceau::CHAINE },
	dls::paire{ dls::vue_chaine("colonne"), id_morceau::COLONNE },
	dls::paire{ dls::vue_chaine("couleur"), id_morceau::COULEUR },
	dls::paire{ dls::vue_chaine("courbe_couleur"), id_morceau::COURBE_COULEUR },
	dls::paire{ dls::vue_chaine("courbe_valeur"), id_morceau::COURBE_VALEUR },
	dls::paire{ dls::vue_chaine("disposition"), id_morceau::DISPOSITION },
	dls::paire{ dls::vue_chaine("dossier"), id_morceau::DOSSIER },
	dls::paire{ dls::vue_chaine("décimal"), id_morceau::DECIMAL },
	dls::paire{ dls::vue_chaine("entier"), id_morceau::ENTIER },
	dls::paire{ dls::vue_chaine("entreface"), id_morceau::ENTREFACE },
	dls::paire{ dls::vue_chaine("entrée"), id_morceau::ENTREE },
	dls::paire{ dls::vue_chaine("faux"), id_morceau::FAUX },
	dls::paire{ dls::vue_chaine("feuille"), id_morceau::FEUILLE },
	dls::paire{ dls::vue_chaine("fichier_entrée"), id_morceau::FICHIER_ENTREE },
	dls::paire{ dls::vue_chaine("fichier_sortie"), id_morceau::FICHIER_SORTIE },
	dls::paire{ dls::vue_chaine("filtres"), id_morceau::FILTRES },
	dls::paire{ dls::vue_chaine("icône"), id_morceau::ICONE },
	dls::paire{ dls::vue_chaine("infobulle"), id_morceau::INFOBULLE },
	dls::paire{ dls::vue_chaine("items"), id_morceau::ITEMS },
	dls::paire{ dls::vue_chaine("ligne"), id_morceau::LIGNE },
	dls::paire{ dls::vue_chaine("liste"), id_morceau::LISTE },
	dls::paire{ dls::vue_chaine("liste_manip"), id_morceau::LISTE_MANIP },
	dls::paire{ dls::vue_chaine("logique"), id_morceau::LOGIQUE },
	dls::paire{ dls::vue_chaine("max"), id_morceau::MAX },
	dls::paire{ dls::vue_chaine("menu"), id_morceau::MENU },
	dls::paire{ dls::vue_chaine("min"), id_morceau::MIN },
	dls::paire{ dls::vue_chaine("métadonnée"), id_morceau::METADONNEE },
	dls::paire{ dls::vue_chaine("nom"), id_morceau::NOM },
	dls::paire{ dls::vue_chaine("onglet"), id_morceau::ONGLET },
	dls::paire{ dls::vue_chaine("pas"), id_morceau::PAS },
	dls::paire{ dls::vue_chaine("précision"), id_morceau::PRECISION },
	dls::paire{ dls::vue_chaine("quand"), id_morceau::QUAND },
	dls::paire{ dls::vue_chaine("rampe_couleur"), id_morceau::RAMPE_COULEUR },
	dls::paire{ dls::vue_chaine("relation"), id_morceau::RELATION },
	dls::paire{ dls::vue_chaine("résultat"), id_morceau::RESULTAT },
	dls::paire{ dls::vue_chaine("sortie"), id_morceau::SORTIE },
	dls::paire{ dls::vue_chaine("suffixe"), id_morceau::SUFFIXE },
	dls::paire{ dls::vue_chaine("séparateur"), id_morceau::SEPARATEUR },
	dls::paire{ dls::vue_chaine("texte"), id_morceau::TEXTE },
	dls::paire{ dls::vue_chaine("valeur"), id_morceau::VALEUR },
	dls::paire{ dls::vue_chaine("vecteur"), id_morceau::VECTEUR },
	dls::paire{ dls::vue_chaine("vrai"), id_morceau::VRAI },
	dls::paire{ dls::vue_chaine("énum"), id_morceau::ENUM },
	dls::paire{ dls::vue_chaine("étiquette"), id_morceau::ETIQUETTE }
);

static auto paires_caracteres_double = dls::cree_dico(
	dls::paire{ dls::vue_chaine("!="), id_morceau::DIFFERENCE },
	dls::paire{ dls::vue_chaine("&&"), id_morceau::ESP_ESP },
	dls::paire{ dls::vue_chaine("&="), id_morceau::ET_EGAL },
	dls::paire{ dls::vue_chaine("*="), id_morceau::FOIS_EGAL },
	dls::paire{ dls::vue_chaine("++"), id_morceau::PLUS_PLUS },
	dls::paire{ dls::vue_chaine("+="), id_morceau::PLUS_EGAL },
	dls::paire{ dls::vue_chaine("--"), id_morceau::MOINS_MOINS },
	dls::paire{ dls::vue_chaine("-="), id_morceau::MOINS_EGAL },
	dls::paire{ dls::vue_chaine("->"), id_morceau::FLECHE },
	dls::paire{ dls::vue_chaine("..."), id_morceau::TROIS_POINT },
	dls::paire{ dls::vue_chaine("/="), id_morceau::DIVISE_EGAL },
	dls::paire{ dls::vue_chaine("<<"), id_morceau::DECALAGE_GAUCHE },
	dls::paire{ dls::vue_chaine("<="), id_morceau::INFERIEUR_EGAL },
	dls::paire{ dls::vue_chaine("=="), id_morceau::EGALITE },
	dls::paire{ dls::vue_chaine(">="), id_morceau::SUPERIEUR_EGAL },
	dls::paire{ dls::vue_chaine(">>"), id_morceau::DECALAGE_DROITE },
	dls::paire{ dls::vue_chaine("^="), id_morceau::OUX_EGAL },
	dls::paire{ dls::vue_chaine("|="), id_morceau::OU_EGAL },
	dls::paire{ dls::vue_chaine("||"), id_morceau::BARE_BARRE }
);

static auto paires_caracteres_speciaux = dls::cree_dico(
	dls::paire{ '!', id_morceau::EXCLAMATION },
	dls::paire{ '"', id_morceau::GUILLEMET },
	dls::paire{ '#', id_morceau::DIESE },
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
		case id_morceau::ET_EGAL:
			return "id_morceau::ET_EGAL";
		case id_morceau::FOIS_EGAL:
			return "id_morceau::FOIS_EGAL";
		case id_morceau::PLUS_PLUS:
			return "id_morceau::PLUS_PLUS";
		case id_morceau::PLUS_EGAL:
			return "id_morceau::PLUS_EGAL";
		case id_morceau::MOINS_MOINS:
			return "id_morceau::MOINS_MOINS";
		case id_morceau::MOINS_EGAL:
			return "id_morceau::MOINS_EGAL";
		case id_morceau::FLECHE:
			return "id_morceau::FLECHE";
		case id_morceau::TROIS_POINT:
			return "id_morceau::TROIS_POINT";
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
		case id_morceau::BARE_BARRE:
			return "id_morceau::BARE_BARRE";
		case id_morceau::ACTION:
			return "id_morceau::ACTION";
		case id_morceau::ATTACHE:
			return "id_morceau::ATTACHE";
		case id_morceau::BARRE_OUTILS:
			return "id_morceau::BARRE_OUTILS";
		case id_morceau::BOUTON:
			return "id_morceau::BOUTON";
		case id_morceau::CASE:
			return "id_morceau::CASE";
		case id_morceau::CHAINE:
			return "id_morceau::CHAINE";
		case id_morceau::COLONNE:
			return "id_morceau::COLONNE";
		case id_morceau::COULEUR:
			return "id_morceau::COULEUR";
		case id_morceau::COURBE_COULEUR:
			return "id_morceau::COURBE_COULEUR";
		case id_morceau::COURBE_VALEUR:
			return "id_morceau::COURBE_VALEUR";
		case id_morceau::DISPOSITION:
			return "id_morceau::DISPOSITION";
		case id_morceau::DOSSIER:
			return "id_morceau::DOSSIER";
		case id_morceau::DECIMAL:
			return "id_morceau::DECIMAL";
		case id_morceau::ENTIER:
			return "id_morceau::ENTIER";
		case id_morceau::ENTREFACE:
			return "id_morceau::ENTREFACE";
		case id_morceau::ENTREE:
			return "id_morceau::ENTREE";
		case id_morceau::FAUX:
			return "id_morceau::FAUX";
		case id_morceau::FEUILLE:
			return "id_morceau::FEUILLE";
		case id_morceau::FICHIER_ENTREE:
			return "id_morceau::FICHIER_ENTREE";
		case id_morceau::FICHIER_SORTIE:
			return "id_morceau::FICHIER_SORTIE";
		case id_morceau::FILTRES:
			return "id_morceau::FILTRES";
		case id_morceau::ICONE:
			return "id_morceau::ICONE";
		case id_morceau::INFOBULLE:
			return "id_morceau::INFOBULLE";
		case id_morceau::ITEMS:
			return "id_morceau::ITEMS";
		case id_morceau::LIGNE:
			return "id_morceau::LIGNE";
		case id_morceau::LISTE:
			return "id_morceau::LISTE";
		case id_morceau::LISTE_MANIP:
			return "id_morceau::LISTE_MANIP";
		case id_morceau::LOGIQUE:
			return "id_morceau::LOGIQUE";
		case id_morceau::MAX:
			return "id_morceau::MAX";
		case id_morceau::MENU:
			return "id_morceau::MENU";
		case id_morceau::MIN:
			return "id_morceau::MIN";
		case id_morceau::METADONNEE:
			return "id_morceau::METADONNEE";
		case id_morceau::NOM:
			return "id_morceau::NOM";
		case id_morceau::ONGLET:
			return "id_morceau::ONGLET";
		case id_morceau::PAS:
			return "id_morceau::PAS";
		case id_morceau::PRECISION:
			return "id_morceau::PRECISION";
		case id_morceau::QUAND:
			return "id_morceau::QUAND";
		case id_morceau::RAMPE_COULEUR:
			return "id_morceau::RAMPE_COULEUR";
		case id_morceau::RELATION:
			return "id_morceau::RELATION";
		case id_morceau::RESULTAT:
			return "id_morceau::RESULTAT";
		case id_morceau::SORTIE:
			return "id_morceau::SORTIE";
		case id_morceau::SUFFIXE:
			return "id_morceau::SUFFIXE";
		case id_morceau::SEPARATEUR:
			return "id_morceau::SEPARATEUR";
		case id_morceau::TEXTE:
			return "id_morceau::TEXTE";
		case id_morceau::VALEUR:
			return "id_morceau::VALEUR";
		case id_morceau::VECTEUR:
			return "id_morceau::VECTEUR";
		case id_morceau::VRAI:
			return "id_morceau::VRAI";
		case id_morceau::ENUM:
			return "id_morceau::ENUM";
		case id_morceau::ETIQUETTE:
			return "id_morceau::ETIQUETTE";
		case id_morceau::CHAINE_CARACTERE:
			return "id_morceau::CHAINE_CARACTERE";
		case id_morceau::CHAINE_LITTERALE:
			return "id_morceau::CHAINE_LITTERALE";
		case id_morceau::CARACTERE:
			return "id_morceau::CARACTERE";
		case id_morceau::NOMBRE:
			return "id_morceau::NOMBRE";
		case id_morceau::NOMBRE_DECIMAL:
			return "id_morceau::NOMBRE_DECIMAL";
		case id_morceau::BOOL:
			return "id_morceau::BOOL";
		case id_morceau::NUL:
			return "id_morceau::NUL";
		case id_morceau::INCONNU:
			return "id_morceau::INCONNU";
	};

	return "ERREUR";
}

static constexpr auto TAILLE_MAX_MOT_CLE = 15;

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

}  /* namespace danjo */
