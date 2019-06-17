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
 
#pragma once

#include <string>

namespace danjo {

enum class id_morceau : unsigned int {
	EXCLAMATION,
	GUILLEMET,
	DIESE,
	POURCENT,
	ESPERLUETTE,
	APOSTROPHE,
	PARENTHESE_OUVRANTE,
	PARENTHESE_FERMANTE,
	FOIS,
	PLUS,
	VIRGULE,
	MOINS,
	POINT,
	DIVISE,
	DOUBLE_POINTS,
	POINT_VIRGULE,
	INFERIEUR,
	EGAL,
	SUPERIEUR,
	AROBASE,
	CROCHET_OUVRANT,
	CROCHET_FERMANT,
	CHAPEAU,
	ACCOLADE_OUVRANTE,
	BARRE,
	ACCOLADE_FERMANTE,
	TILDE,
	DIFFERENCE,
	ESP_ESP,
	ET_EGAL,
	FOIS_EGAL,
	PLUS_PLUS,
	PLUS_EGAL,
	MOINS_MOINS,
	MOINS_EGAL,
	FLECHE,
	TROIS_POINT,
	DIVISE_EGAL,
	DECALAGE_GAUCHE,
	INFERIEUR_EGAL,
	EGALITE,
	SUPERIEUR_EGAL,
	DECALAGE_DROITE,
	OUX_EGAL,
	OU_EGAL,
	BARE_BARRE,
	ACTION,
	ATTACHE,
	BARRE_OUTILS,
	BOUTON,
	CASE,
	CHAINE,
	COLONNE,
	COULEUR,
	COURBE_COULEUR,
	COURBE_VALEUR,
	DISPOSITION,
	DOSSIER,
	DECIMAL,
	ENTIER,
	ENTREFACE,
	ENTREE,
	FAUX,
	FEUILLE,
	FICHIER_ENTREE,
	FICHIER_SORTIE,
	FILTRES,
	ICONE,
	INFOBULLE,
	ITEMS,
	LIGNE,
	LISTE,
	LOGIQUE,
	MAX,
	MENU,
	MIN,
	METADONNEE,
	NOM,
	ONGLET,
	PAS,
	PRECISION,
	QUAND,
	RAMPE_COULEUR,
	RELATION,
	RESULTAT,
	SORTIE,
	SUFFIXE,
	SEPARATEUR,
	TEXTE,
	VALEUR,
	VECTEUR,
	VRAI,
	ENUM,
	ETIQUETTE,
	CHAINE_CARACTERE,
	CHAINE_LITTERALE,
	CARACTERE,
	NOMBRE,
	NOMBRE_DECIMAL,
	BOOL,
	NUL,
	INCONNU,
};

inline id_morceau operator&(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) & id2);
}

inline id_morceau operator|(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) | id2);
}

inline id_morceau operator|(id_morceau id1, id_morceau id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline id_morceau operator<<(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) << id2);
}

inline id_morceau operator>>(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) >> id2);
}

struct DonneesMorceaux {
	using type = id_morceau;
	static constexpr type INCONNU = id_morceau::INCONNU;

	std::string_view chaine;
	size_t ligne_pos;
	id_morceau identifiant;
	int pad = 0;
};

const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

id_morceau id_caractere_double(const std::string_view &chaine);

id_morceau id_chaine(const std::string_view &chaine);

}  /* namespace danjo */
