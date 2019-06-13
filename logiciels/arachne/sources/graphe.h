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

#include <list>

#include "autrice_fichier.h"
#include "lectrice_fichier.h"
#include "magasin_chaine.h"

namespace arachne {

/**
 * La struct registre_noeud contient les éléments qui sont inscrits dans chaque
 * registre de noeud dans le fichier de registre des noeuds.
 *
 * Les noeuds sont écrits de la manière suivante :
 * - 1 octet pour définir si le registre est utilisé
 * - 4 octets pour l'id de la première relation du noeud
 * - 4 octets pour l'id de la première propriété du noeud
 * - 4 octets pour l'id du magasin d'étiquettes du noeud
 * - 1 octet pour des drapeaux divers
 */
#if 0
struct registre_noeud {
	char utilise{};
	unsigned int id_relation{};
	unsigned int id_propriete{};
	unsigned int id_etiquette{};
	char drapeaux{};
};
#endif

static constexpr auto TAILLE_REGISTRE_NOEUD = 14;
//static_assert(TAILLE_REGISTRE_NOEUD == sizeof(registre_noeud), "");

/**
 * La struct registre_relation contient les éléments qui sont inscrits dans
 * chaque registre de noeud dans le fichier de registre des relations.
 *
 * Les relations sont écrites de la manière suivante :
 * - 1 octet pour définir si le registre est utilisé
 * - 4 octets : id du noeud de départ
 * - 4 octets : id du noeud de fin
 * - 4 octets : pointeur vers le type de relation
 * - 4 octets : pointeur vers la relation suivante du noeud de départ
 * - 4 octets : pointeur vers la relation précédente du noeud de départ
 * - 4 octets : pointeur vers la relation suivante du noeud de fin
 * - 4 octets : pointeur vers la relation précédente du noeud de fin
 * - 4 octets : id de la première propriété
 * - 1 octet : drapeau pour savoir si la relation est la première dans la liste de relation
 */
#if 0
struct registre_relation {
	char utilise{};
	unsigned int id_noeud_depart{};
	unsigned int id_noeud_fin{};
	unsigned int id_type_relation{};
	unsigned int id_relation_suivante_noeud_depart{};
	unsigned int id_relation_precedente_noeud_depart{};
	unsigned int id_relation_suivante_noeud_fin{};
	unsigned int id_relation_precedente_noeud_fin{};
	unsigned int id_propriete{};
	char drapeaux{};
};
#endif

static constexpr auto TAILLE_REGISTRE_RELATION = 34;
//static_assert(TAILLE_REGISTRE_RELATION == sizeof(registre_relation), "");

static constexpr auto TAILLE_REGISTRE_PROPRIETE = 14;

struct propriete;
struct relation;

struct etiquette {
	etiquette *suivante = nullptr;
	unsigned int id = 0;
	unsigned int id_nom = 0;
};

struct noeud {
	std::list<propriete *> proprietes{};
	std::list<relation *> relations{};
	std::list<etiquette *> etiquettes{};

	unsigned int id = 0;
	char utilise = 0;
};

struct relation {
	noeud *debut{};
	noeud *fin{};

	relation *rel_prev_debut{};
	relation *rel_suiv_debut{};
	relation *rel_prev_fin{};
	relation *rel_suiv_fin{};

	unsigned int type_relation{};

	std::list<propriete *> proprietes{};
	unsigned int id{};
};

struct propriete {
	propriete *suivante{};

	unsigned int id{};
	unsigned int id_nom{};
	int valeur{};
};

static constexpr const char *FICHIER_NOEUD = "arachne.noeud.bd";
static constexpr const char *FICHIER_PROPRIETE = "arachne.propriete.bd";
static constexpr const char *FICHIER_NOM_PROPRIETE = "arachne.nom_proprietes.bd";
static constexpr const char *FICHIER_RELATION = "arachne.relation.bd";
static constexpr const char *FICHIER_TYPE_RELATION = "arachne.type_relation.bd";
static constexpr const char *FICHIER_ETIQUETTE = "arachne.etiquette.bd";
static constexpr const char *FICHIER_NOM_ETIQUETTE = "arachne.nom_etiquette.bd";

struct infos_fichiers {
	std::experimental::filesystem::path chemin_fichier_noeud{};
	std::experimental::filesystem::path chemin_fichier_propriete{};
	std::experimental::filesystem::path chemin_fichier_nom_proprietes{};
	std::experimental::filesystem::path chemin_fichier_relation{};
	std::experimental::filesystem::path chemin_fichier_type_relation{};
	std::experimental::filesystem::path chemin_fichier_etiquette{};
	std::experimental::filesystem::path chemin_fichier_nom_etiquette{};
};

class graphe {
	std::list<noeud *> m_noeuds{};
	std::list<relation *> m_relations{};
	std::list<propriete *> m_proprietes{};
	std::list<etiquette *> m_etiquettes{};

	magasin_chaine m_nom_proprietes{};
	magasin_chaine m_type_relations{};
	magasin_chaine m_nom_etiquettes{};

	/* À chaque fois qu'un noeud est ajouté, le compte est incrémenté, et la
	 * valeur incrémentée est assigné au noeud comme identifiant, de sorte que
	 * l'identifiant 0 est réservé pour vérifié les noeuds nuls. */
	unsigned int m_nombre_noeuds = 0;

	/* À chaque fois qu'une relation est ajoutée, le compte est incrémenté, et
	 * la valeur incrémentée est assigné à la relation comme identifiant, de
	 * sorte que l'identifiant 0 est réservé pour vérifié les relations nulles.
	 */
	unsigned int m_nombre_relations = 0;

	/* À chaque fois qu'une propriété est ajoutée, le compte est incrémenté, et
	 * la valeur incrémentée est assigné à la propriété comme identifiant, de
	 * sorte que l'identifiant 0 est réservé pour vérifié les propriétés nulles.
	 */
	unsigned int m_nombre_proprietes = 0;

	/* À chaque fois qu'une étiquette est ajoutée, le compte est incrémenté, et
	 * la valeur incrémentée est assigné à l'étiquette comme identifiant, de
	 * sorte que l'identifiant 0 est réservé pour vérifié les étiquettes nulles.
	 */
	unsigned int m_nombre_etiquettes = 0;

	infos_fichiers m_infos_fichiers{};

	autrice_fichier m_autrice_noeuds{};
	autrice_fichier m_autrice_proprietes{};
	autrice_fichier m_autrice_nom_proprietes{};
	autrice_fichier m_autrice_relations{};
	autrice_fichier m_autrice_type_relations{};

	lectrice_fichier m_lectrice_noeuds{};
	lectrice_fichier m_lectrice_proprietes{};
	lectrice_fichier m_lectrice_nom_proprietes{};
	lectrice_fichier m_lectrice_relations{};
	lectrice_fichier m_lectrice_type_relations{};

public:
	explicit graphe(const infos_fichiers &infos);

	~graphe();

	noeud *ajoute_noeud(const std::string &nom, int valeur);

	void ajoute_propriete(noeud *n, const std::string &nom, int valeur);

	void ajoute_etiquette(noeud *n, const std::string &nom);

	void ajoute_propriete(relation *r, const std::string &nom, int valeur);

	void ajoute_relation(noeud *debut, noeud *fin, const std::string &nom);

	void ecris_noeud();

	void lis_noeud();

	void ecris_proprietes();

	void lis_proprietes();

	void ecris_noms_proprietes();

	void lis_noms_proprietes();

	/**
	 * La struct registre_relation contient les éléments qui sont inscrits dans
	 * chaque registre de noeud dans le fichier de registre des relations.
	 *
	 * Les relations sont écrites de la manière suivante :
	 * - 1 octet pour définir si le registre est utilisé
	 * - 4 octets : id du noeud de départ
	 * - 4 octets : id du noeud de fin
	 * - 4 octets : pointeur vers le type de relation
	 * - 4 octets : pointeur vers la relation précédente du noeud de départ
	 * - 4 octets : pointeur vers la relation suivante du noeud de départ
	 * - 4 octets : pointeur vers la relation précédente du noeud de fin
	 * - 4 octets : pointeur vers la relation suivante du noeud de fin
	 * - 4 octets : id de la première propriété
	 * - 1 octet : drapeau pour savoir si la relation est la première dans la liste de relation
	 */
	void ecris_relations();

	void lis_relations();

	void ecris_types_relations();

	void lis_types_relations();

	void ecris_etiquettes();

	void lis_etiquettes();

	void ecris_noms_etiquettes();

	void lis_noms_etiquettes();

	unsigned int id_etiquette(const std::string &etiq) const;

	std::list<noeud *> noeuds() const;
};

}  /* namespace arachne */
