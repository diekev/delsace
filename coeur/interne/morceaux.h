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

namespace kangao {

enum {
	/* disposition */
	IDENTIFIANT_DISPOSITION,
	IDENTIFIANT_MENU,
	IDENTIFIANT_COLONNE,
	IDENTIFIANT_LIGNE,
	IDENTIFIANT_DOSSIER,
	IDENTIFIANT_ONGLET,
	IDENTIFIANT_BARRE_OUTILS,

	/* caractères */
	IDENTIFIANT_PARENTHESE_OUVERTE,
	IDENTIFIANT_PARENTHESE_FERMEE,
	IDENTIFIANT_ACCOLADE_OUVERTE,
	IDENTIFIANT_ACCOLADE_FERMEE,
	IDENTIFIANT_EGAL,
	IDENTIFIANT_CHAINE_CARACTERE,
	IDENTIFIANT_POINT_VIRGULE,
	IDENTIFIANT_CROCHET_OUVERT,
	IDENTIFIANT_CROCHET_FERME,
	IDENTIFIANT_VIRGULE,

	/* contrôles */
	IDENTIFIANT_CONTROLE_CURSEUR,
	IDENTIFIANT_CONTROLE_CURSEUR_DECIMAL,
	IDENTIFIANT_CONTROLE_ETIQUETTE,
	IDENTIFIANT_CONTROLE_LISTE,
	IDENTIFIANT_CONTROLE_CASE_COCHER,
	IDENTIFIANT_CONTROLE_CHAINE,
	IDENTIFIANT_CONTROLE_FICHIER_ENTREE,
	IDENTIFIANT_CONTROLE_FICHIER_SORTIE,
	IDENTIFIANT_CONTROLE_COULEUR,
	IDENTIFIANT_CONTROLE_VECTEUR,
	IDENTIFIANT_CONTROLE_BOUTON,
	IDENTIFIANT_CONTROLE_ACTION,
	IDENTIFIANT_CONTROLE_SEPARATEUR,

	/* propriétés */
	IDENTIFIANT_PROPRIETE_INFOBULLE,
	IDENTIFIANT_PROPRIETE_MIN,
	IDENTIFIANT_PROPRIETE_MAX,
	IDENTIFIANT_PROPRIETE_VALEUR,
	IDENTIFIANT_PROPRIETE_ATTACHE,
	IDENTIFIANT_PROPRIETE_PRECISION,
	IDENTIFIANT_PROPRIETE_PAS,
	IDENTIFIANT_PROPRIETE_ITEM,
	IDENTIFIANT_PROPRIETE_NOM,
	IDENTIFIANT_PROPRIETE_METADONNEE,
	IDENTIFIANT_PROPRIETE_ICONE,

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

}  /* namespace kangao */
