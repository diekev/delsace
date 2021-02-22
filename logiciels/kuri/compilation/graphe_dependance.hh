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

#include "biblinternes/outils/badge.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "structures/tableau.hh"
#include "structures/tableau_compresse.hh"

struct AtomeFonction;
struct GrapheDependance;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudDependance;
struct NoeudExpression;
struct Statistiques;
struct Type;

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

struct Relation {
	TypeRelation type = TypeRelation::INVALIDE;
	NoeudDependance *noeud_debut  = nullptr;
	NoeudDependance *noeud_fin  = nullptr;
};

inline bool operator == (Relation const &r1, Relation const &r2)
{
	return r1.type == r2.type && r1.noeud_debut == r2.noeud_debut && r1.noeud_fin == r2.noeud_fin;
}

struct NoeudDependance {
private:
	kuri::tableau_compresse<Relation> m_relations{};

	union {
		Type *m_type;
		NoeudDeclarationEnteteFonction *m_noeud_fonction;
		NoeudDeclarationVariable *m_noeud_globale;
	};

	TypeNoeudDependance m_type_noeud = TypeNoeudDependance::INVALIDE;

public:
	bool fut_visite = false;

	/* pour certains algorithmes de travail sur le graphe */
	char drapeaux = 0;

	explicit NoeudDependance(NoeudDeclarationVariable *globale);
	explicit NoeudDependance(NoeudDeclarationEnteteFonction *fonction);
	explicit NoeudDependance(Type *t);

	inline bool est_type() const
	{
		return m_type_noeud == TypeNoeudDependance::TYPE;
	}

	inline bool est_globale() const
	{
		return m_type_noeud == TypeNoeudDependance::GLOBALE;
	}

	inline bool est_fonction() const
	{
		return m_type_noeud == TypeNoeudDependance::FONCTION;
	}

	inline Type *type() const
	{
		assert(est_type());
		return m_type;
	}

	inline NoeudDeclarationEnteteFonction *fonction() const
	{
		assert(est_fonction());
		return m_noeud_fonction;
	}

	inline NoeudDeclarationVariable *globale() const
	{
		assert(est_globale());
		return m_noeud_globale;
	}

	void ajoute_relation(Badge<GrapheDependance>, const Relation &relation);

	kuri::tableau_compresse<Relation> const &relations() const;

	void relations(Badge<GrapheDependance>, kuri::tableau_compresse<Relation> &&relations);
};

struct DonneesDependance {
	dls::ensemblon<NoeudDeclarationEnteteFonction const *, 16> fonctions_utilisees{};
	dls::ensemblon<NoeudDeclarationVariable const *, 16> globales_utilisees{};
	dls::ensemblon<Type *, 16> types_utilises{};
};

struct GrapheDependance {
	tableau_page<NoeudDependance> noeuds{};

	// CRÉE (:FONCTION { nom = $nom })
	NoeudDependance *cree_noeud_fonction(NoeudDeclarationEnteteFonction *noeud_syntaxique);

	// CRÉE (:GLOBALE { nom = $nom })
	NoeudDependance *cree_noeud_globale(NoeudDeclarationVariable *noeud_syntaxique);

	// FUSIONNE (:TYPE { index = $index })
	NoeudDependance *cree_noeud_type(Type *type);

	// CHERCHE (type1 :TYPE { index = $index1 })
	// CHERCHE (type2 :TYPE { index = $index1 })
	// FUSIONNE (type1)-[:UTILISE_TYPE]->(type2)
	void connecte_type_type(NoeudDependance &type1, NoeudDependance &type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void connecte_type_type(Type *type1, Type *type2, TypeRelation type_rel = TypeRelation::UTILISE_TYPE);

	void ajoute_dependances(NoeudDependance &noeud, DonneesDependance &donnees, bool efface_donnees = true);

	void connecte_noeuds(NoeudDependance &noeud1, NoeudDependance &noeud2, TypeRelation type_relation);

	void rassemble_statistiques(Statistiques &stats) const;

	void reduction_transitive();

	void rassemble_fonctions_utilisees(NoeudDependance *racine, kuri::tableau<AtomeFonction *> &fonctions, dls::ensemble<AtomeFonction *> &utilises);

	template <typename Rappel>
	void traverse(NoeudDependance *racine, Rappel rappel)
	{
		racine->fut_visite = true;

		for (auto const &relation : racine->relations().plage()) {
			auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
			accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
			accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

			if (!accepte) {
				continue;
			}

			if (relation.noeud_fin->fut_visite) {
				continue;
			}

			traverse(relation.noeud_fin, rappel);
		}

		rappel(racine);
	}
};

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance);

