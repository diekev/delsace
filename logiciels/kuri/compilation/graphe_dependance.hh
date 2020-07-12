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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "arbre_syntaxique.hh"

/**
 * GrapheDependance de dépendance entre les fonctions et les types.
 */

enum class TypeNoeudDependance {
	INVALIDE,
	FONCTION,
	TYPE,
	GLOBALE,
};

#define ENUMERE_TYPES_RELATION \
	ENUMERE_TYPE_RELATION_EX(INVALIDE) \
	ENUMERE_TYPE_RELATION_EX(UTILISE_TYPE) \
	ENUMERE_TYPE_RELATION_EX(UTILISE_FONCTION) \
	ENUMERE_TYPE_RELATION_EX(UTILISE_GLOBALE)

enum class TypeRelation : int {
#define ENUMERE_TYPE_RELATION_EX(type) type,
	ENUMERE_TYPES_RELATION
#undef ENUMERE_TYPE_RELATION_EX
};

const char *chaine_type_relation(TypeRelation type);
std::ostream &operator<<(std::ostream &os, TypeRelation type);

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
	Type *type_{};

	// pour tous les noeuds
	NoeudExpression *noeud_syntaxique{};

	bool fut_visite = false;
	bool deja_genere = false;

	/* pour certains algorithmes de travail sur le graphe */
	char drapeaux = 0;
};

struct DonneesDependance {
	dls::ensemblon<NoeudDeclarationFonction const *, 16> fonctions_utilisees{};
	dls::ensemblon<NoeudDeclarationVariable const *, 16> globales_utilisees{};
	dls::ensemblon<Type *, 16> types_utilises{};
};

struct GrapheDependance {
	tableau_page<NoeudDependance> noeuds{};

	// CRÉE (:FONCTION { nom = $nom })
	NoeudDependance *cree_noeud_fonction(NoeudDeclarationFonction *noeud_syntaxique);

	// CRÉE (:GLOBALE { nom = $nom })
	NoeudDependance *cree_noeud_globale(NoeudDeclarationVariable *noeud_syntaxique);

	// FUSIONNE (:TYPE { index = $index })
	NoeudDependance *cree_noeud_type(Type *type);

	// CHERCHE (:FONCTION { nom = $nom })
	NoeudDependance *cherche_noeud_fonction(dls::vue_chaine_compacte const &nom) const;

	// CHERCHE (type1 :TYPE { index = $index1 })
	// CHERCHE (type2 :TYPE { index = $index1 })
	// FUSIONNE (type1)-[:UTILISE_TYPE]->(type2)
	void connecte_type_type(NoeudDependance &type1, NoeudDependance &type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void connecte_type_type(Type *type1, Type *type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void ajoute_dependances(NoeudDependance &noeud, DonneesDependance &donnees);

	void connecte_noeuds(NoeudDependance &noeud1, NoeudDependance &noeud2, TypeRelation type_relation);

	size_t memoire_utilisee() const;
};

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance);

void reduction_transitive(GrapheDependance &graphe_dependance);

template <typename Rappel>
void traverse_graphe(NoeudDependance *racine, Rappel rappel)
{
	racine->fut_visite = true;

	for (auto const &relation : racine->relations) {
		auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
		accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
		accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

		if (!accepte) {
			continue;
		}

		if (relation.noeud_fin->fut_visite) {
			continue;
		}

		traverse_graphe(relation.noeud_fin, rappel);
	}

	rappel(racine);
}
