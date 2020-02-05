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

#pragma once

#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

#include "arbre_syntactic.h"

/**
 * GrapheDependance de dépendance entre les fonctions et les types.
 */

enum class TypeNoeudDependance {
	INVALIDE,
	FONCTION,
	TYPE,
	GLOBALE,
};

enum class TypeRelation : int {
	INVALIDE,
	UTILISE_TYPE,
	UTILISE_FONCTION,
	UTILISE_GLOBALE,

	/* pour les relations entre les types
	 * l'idée est de stocker dans le graphe les relations entre les types, afin
	 * de ne pas avoir à construire partout des DonneesTypeFinal
	 */
	TYPE_TABLEAU,
	TYPE_REFERENCE,
	TYPE_POINTEUR,
	/* le type du déréférencement d'un pointeur ou d'un tableau */
	TYPE_DEREFERENCE,
};

const char *chaine_type_relation(TypeRelation type);

struct NoeudDependance;

struct Relation {
	TypeRelation type = TypeRelation::INVALIDE;
	NoeudDependance *noeud_debut  = nullptr;
	NoeudDependance *noeud_fin  = nullptr;

	COPIE_CONSTRUCT(Relation);
};

struct NoeudDependance {
	TypeNoeudDependance type = TypeNoeudDependance::INVALIDE;
	dls::tableau<Relation> relations{};

	// pour les types
	long index{};

	// pour les fonctions ou variables globales
	dls::vue_chaine_compacte nom{};

	// pour tous les noeuds
	noeud::base *noeud_syntactique{};

	// pour le graphe de dépendance syntaxique
	dls::tableau<noeud::base *> noeuds_syntaxiques{};

	bool fut_visite = false;
	bool deja_genere = false;
	bool typedef_genere = false;

	/* pour certains algorithmes de travail sur le graphe */
	char drapeaux = 0;
};

struct DonneesDependance {
	dls::ensemble<dls::vue_chaine_compacte> fonctions_utilisees{};
	dls::ensemble<dls::vue_chaine_compacte> globales_utilisees{};
	dls::ensemble<long> types_utilises{};
};

struct GrapheDependance {
	dls::tableau<NoeudDependance *> noeuds{};

	dls::dico<long, NoeudDependance *> index_noeuds_type{};

	~GrapheDependance();

	// CRÉE (:FONCTION { nom = $nom })
	NoeudDependance *cree_noeud_fonction(dls::vue_chaine_compacte const &nom, noeud::base *noeud_syntactique);

	// CRÉE (:GLOBALE { nom = $nom })
	NoeudDependance *cree_noeud_globale(dls::vue_chaine_compacte const &nom, noeud::base *noeud_syntactique);

	// FUSIONNE (:TYPE { index = $index })
	NoeudDependance *cree_noeud_type(long index);

	// CHERCHE (:FONCTION { nom = $nom })
	NoeudDependance *cherche_noeud_fonction(dls::vue_chaine_compacte const &nom) const;

	// CHERCHE (:GLOBALE { nom = $nom })
	NoeudDependance *cherche_noeud_globale(dls::vue_chaine_compacte const &nom) const;

	// CHERCHE (:TYPE { index = $index })
	NoeudDependance *cherche_noeud_type(long index) const;

	// CHERCHE (fonction1 :FONCTION { nom = $nom1 })
	// CHERCHE (fonction2 :FONCTION { nom = $nom2 })
	// FUSIONNE (fonction1)-[:UTILISE_FONCTION]->(fonction2)
	void connecte_fonction_fonction(NoeudDependance &fonction1, NoeudDependance &fonction2);

	void connecte_fonction_fonction(const dls::vue_chaine_compacte &fonction1, const dls::vue_chaine_compacte &fonction2);

	// CHERCHE (fonction :FONCTION { nom = $nom1 })
	// CHERCHE (globale :GLOBALE { nom = $nom2 })
	// FUSIONNE (fonction)-[:UTILISE_FONCTION]->(globale)
	void connecte_fonction_globale(NoeudDependance &fonction, NoeudDependance &globale);

	void connecte_fonction_globale(const dls::vue_chaine_compacte &fonction, const dls::vue_chaine_compacte &globale);

	// CHERCHE (fonction :FONCTION { nom = $nom })
	// CHERCHE (type :TYPE { index = $index })
	// FUSIONNE (fonction)-[:UTILISE_TYPE]->(type)
	void connecte_fonction_type(NoeudDependance &fonction, NoeudDependance &type);

	// CHERCHE (type1 :TYPE { index = $index1 })
	// CHERCHE (type2 :TYPE { index = $index1 })
	// FUSIONNE (type1)-[:UTILISE_TYPE]->(type2)
	void connecte_type_type(NoeudDependance &type1, NoeudDependance &type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void connecte_type_type(long type1, long type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void ajoute_dependances(NoeudDependance &noeud, DonneesDependance &donnees);

	long trouve_index_type(long index_racine, TypeRelation type) const;

	void connecte_noeuds(NoeudDependance &noeud1, NoeudDependance &noeud2, TypeRelation type_relation);
};

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance);

void reduction_transitive(GrapheDependance &graphe_dependance);
