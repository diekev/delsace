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

#pragma once

#include <string>
#include <vector>

namespace danjo {

/* Liste générée par genere_donnees_mot_cle.py */
enum {
	IDENTIFIANT_ACTION,
	IDENTIFIANT_ATTACHE,
	IDENTIFIANT_BARRE_OUTILS,
	IDENTIFIANT_BOUTON,
	IDENTIFIANT_CASE,
	IDENTIFIANT_CHAINE,
	IDENTIFIANT_COLONNE,
	IDENTIFIANT_COULEUR,
	IDENTIFIANT_DISPOSITION,
	IDENTIFIANT_DOSSIER,
	IDENTIFIANT_DECIMAL,
	IDENTIFIANT_ENTIER,
	IDENTIFIANT_ENTREE,
	IDENTIFIANT_FAUX,
	IDENTIFIANT_FEUILLE,
	IDENTIFIANT_FICHIER_ENTREE,
	IDENTIFIANT_FICHIER_SORTIE,
	IDENTIFIANT_FILTRES,
	IDENTIFIANT_ICONE,
	IDENTIFIANT_INFOBULLE,
	IDENTIFIANT_INTERFACE,
	IDENTIFIANT_ITEMS,
	IDENTIFIANT_LIGNE,
	IDENTIFIANT_LISTE,
	IDENTIFIANT_LOGIQUE,
	IDENTIFIANT_MAX,
	IDENTIFIANT_MENU,
	IDENTIFIANT_MIN,
	IDENTIFIANT_METADONNEE,
	IDENTIFIANT_NOM,
	IDENTIFIANT_ONGLET,
	IDENTIFIANT_PAS,
	IDENTIFIANT_PRECISION,
	IDENTIFIANT_QUAND,
	IDENTIFIANT_RELATION,
	IDENTIFIANT_RESULTAT,
	IDENTIFIANT_SORTIE,
	IDENTIFIANT_SEPARATEUR,
	IDENTIFIANT_VALEUR,
	IDENTIFIANT_VECTEUR,
	IDENTIFIANT_VRAI,
	IDENTIFIANT_ETIQUETTE,
	IDENTIFIANT_EXCLAMATION,
	IDENTIFIANT_GUILLEMET,
	IDENTIFIANT_DIESE,
	IDENTIFIANT_POURCENT,
	IDENTIFIANT_ESPERLUETTE,
	IDENTIFIANT_APOSTROPHE,
	IDENTIFIANT_PARENTHESE_OUVRANTE,
	IDENTIFIANT_PARENTHESE_FERMANTE,
	IDENTIFIANT_FOIS,
	IDENTIFIANT_PLUS,
	IDENTIFIANT_VIRGULE,
	IDENTIFIANT_MOINS,
	IDENTIFIANT_POINT,
	IDENTIFIANT_DIVISE,
	IDENTIFIANT_DOUBLE_POINT,
	IDENTIFIANT_POINT_VIRGULE,
	IDENTIFIANT_INFERIEUR,
	IDENTIFIANT_EGAL,
	IDENTIFIANT_SUPERIEUR,
	IDENTIFIANT_CROCHET_OUVRANT,
	IDENTIFIANT_CROCHET_FERMANT,
	IDENTIFIANT_CHAPEAU,
	IDENTIFIANT_ACCOLADE_OUVRANTE,
	IDENTIFIANT_BARRE,
	IDENTIFIANT_ACCOLADE_FERMANTE,
	IDENTIFIANT_TILDE,
	IDENTIFIANT_DIFFERENCE,
	IDENTIFIANT_ESP_ESP,
	IDENTIFIANT_ET_EGAL,
	IDENTIFIANT_FOIS_EGAL,
	IDENTIFIANT_PLUS_PLUS,
	IDENTIFIANT_PLUS_EGAL,
	IDENTIFIANT_MOINS_MOINS,
	IDENTIFIANT_MOINS_EGAL,
	IDENTIFIANT_FLECHE,
	IDENTIFIANT_TROIS_POINT,
	IDENTIFIANT_DIVISE_EGAL,
	IDENTIFIANT_DECALAGE_GAUCHE,
	IDENTIFIANT_INFERIEUR_EGAL,
	IDENTIFIANT_EGALITE,
	IDENTIFIANT_SUPERIEUR_EGAL,
	IDENTIFIANT_DECALAGE_DROITE,
	IDENTIFIANT_OUX_EGAL,
	IDENTIFIANT_OU_EGAL,
	IDENTIFIANT_BARE_BARRE,
	IDENTIFIANT_CHAINE_CARACTERE,
	IDENTIFIANT_CHAINE_LITTERALE,
	IDENTIFIANT_CARACTERE,
	IDENTIFIANT_NOMBRE,
	IDENTIFIANT_NOMBRE_DECIMAL,
	IDENTIFIANT_BOOL,
	IDENTIFIANT_NUL,
};

struct DonneesMorceaux {
	int identifiant = 0;
	int numero_ligne = 0;
	int position_ligne = 0;
	std::string contenu = "";
	std::string_view ligne;

	DonneesMorceaux() = default;
};

std::vector<std::string> decoupe(const std::string &chaine, const char delimiteur = ' ');

const char *chaine_identifiant(int identifiant);

}  /* namespace danjo */
