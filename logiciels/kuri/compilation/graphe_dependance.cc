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

#include "graphe_dependance.hh"

#include "modules.hh"

const char *chaine_type_relation(TypeRelation type)
{
#define CAS_TYPE(x) case TypeRelation::x: return #x;
	switch (type) {
		CAS_TYPE(INVALIDE)
		CAS_TYPE(UTILISE_TYPE)
		CAS_TYPE(UTILISE_FONCTION)
		CAS_TYPE(UTILISE_GLOBALE)
	}

	return "erreur : relation inconnue";
#undef CAS_TYPE
}

GrapheDependance::~GrapheDependance()
{
	for (auto noeud : noeuds) {
		memoire::deloge("NoeudDependance", noeud);
	}
}

NoeudDependance *GrapheDependance::cree_noeud_fonction(const dls::vue_chaine_compacte &nom, NoeudExpression *noeud_syntactique)
{
	/* différents modules peuvent déclarer la même fonction externe (p.e printf),
	 * donc cherche d'abord le noeud. */
	auto noeud = cherche_noeud_fonction(nom);

	if (noeud == nullptr) {
		noeud = memoire::loge<NoeudDependance>("NoeudDependance");
		noeud->nom = nom;
		noeud->noeud_syntactique = noeud_syntactique;
		noeud->type = TypeNoeudDependance::FONCTION;

		noeuds.pousse(noeud);
	}

	return noeud;
}

NoeudDependance *GrapheDependance::cree_noeud_globale(const dls::vue_chaine_compacte &nom, NoeudExpression *noeud_syntactique)
{
	auto noeud = memoire::loge<NoeudDependance>("NoeudDependance");
	noeud->nom = nom;
	noeud->noeud_syntactique = noeud_syntactique;
	noeud->type = TypeNoeudDependance::GLOBALE;

	noeuds.pousse(noeud);

	return noeud;
}

NoeudDependance *GrapheDependance::cree_noeud_type(Type *type)
{
	auto noeud = cherche_noeud_type(type);

	if (noeud == nullptr) {
		noeud = memoire::loge<NoeudDependance>("NoeudDependance");
		noeud->type_ = type;
		noeud->type = TypeNoeudDependance::TYPE;

		noeuds.pousse(noeud);
		index_noeuds_type.insere({type, noeud});
	}

	return noeud;
}

NoeudDependance *GrapheDependance::cherche_noeud_fonction(const dls::vue_chaine_compacte &nom) const
{
	for (auto noeud : noeuds) {
		if (noeud->type != TypeNoeudDependance::FONCTION) {
			continue;
		}

		if (noeud->nom == nom) {
			return noeud;
		}
	}

	return nullptr;
}

NoeudDependance *GrapheDependance::cherche_noeud_globale(const dls::vue_chaine_compacte &nom) const
{
	for (auto noeud : noeuds) {
		if (noeud->type != TypeNoeudDependance::GLOBALE) {
			continue;
		}

		if (noeud->nom == nom) {
			return noeud;
		}
	}

	return nullptr;
}

NoeudDependance *GrapheDependance::cherche_noeud_type(Type *type) const
{
	auto iter = index_noeuds_type.trouve(type);

	if (iter != index_noeuds_type.fin()) {
		return iter->second;
	}

	return nullptr;
}

void GrapheDependance::connecte_fonction_fonction(NoeudDependance &fonction1, NoeudDependance &fonction2)
{
	assert(fonction1.type == TypeNoeudDependance::FONCTION);
	assert(fonction2.type == TypeNoeudDependance::FONCTION);

	for (auto const &relation : fonction1.relations) {
		if (relation.type == TypeRelation::UTILISE_FONCTION && relation.noeud_fin == &fonction2) {
			return;
		}
	}

	fonction1.relations.pousse({ TypeRelation::UTILISE_FONCTION, &fonction1, &fonction2 });
}

void GrapheDependance::connecte_fonction_fonction(
		dls::vue_chaine_compacte const &fonction1,
		dls::vue_chaine_compacte const &fonction2)
{
	auto noeud1 = cherche_noeud_fonction(fonction1);
	auto noeud2 = cherche_noeud_fonction(fonction2);

	connecte_fonction_fonction(*noeud1, *noeud2);
}

void GrapheDependance::connecte_fonction_globale(
		NoeudDependance &fonction,
		NoeudDependance &globale)
{
	assert(fonction.type == TypeNoeudDependance::FONCTION);
	assert(globale.type == TypeNoeudDependance::GLOBALE);

	for (auto const &relation : fonction.relations) {
		if (relation.type == TypeRelation::UTILISE_GLOBALE && relation.noeud_fin == &globale) {
			return;
		}
	}

	fonction.relations.pousse({ TypeRelation::UTILISE_GLOBALE, &fonction, &globale });
}

void GrapheDependance::connecte_fonction_globale(
		const dls::vue_chaine_compacte &fonction,
		const dls::vue_chaine_compacte &globale)
{
	auto noeud1 = cherche_noeud_fonction(fonction);
	auto noeud2 = cherche_noeud_fonction(globale);

	connecte_fonction_globale(*noeud1, *noeud2);
}

void GrapheDependance::connecte_fonction_type(NoeudDependance &fonction, NoeudDependance &type)
{
	assert(fonction.type == TypeNoeudDependance::FONCTION);
	assert(type.type == TypeNoeudDependance::TYPE);

	for (auto const &relation : fonction.relations) {
		if (relation.type == TypeRelation::UTILISE_TYPE && relation.noeud_fin == &type) {
			return;
		}
	}

	fonction.relations.pousse({ TypeRelation::UTILISE_TYPE, &fonction, &type });
}

void GrapheDependance::connecte_type_type(NoeudDependance &type1, NoeudDependance &type2, TypeRelation type_rel)
{
	assert(type1.type == TypeNoeudDependance::TYPE);
	assert(type2.type == TypeNoeudDependance::TYPE);

	for (auto const &relation : type1.relations) {
		if (relation.type == type_rel && relation.noeud_fin == &type2) {
			return;
		}
	}

	type1.relations.pousse({ type_rel, &type1, &type2 });
}

void GrapheDependance::connecte_type_type(Type *type1, Type *type2, TypeRelation type_rel)
{
	auto noeud1 = cree_noeud_type(type1);
	auto noeud2 = cree_noeud_type(type2);

	connecte_type_type(*noeud1, *noeud2, type_rel);
}

Type *GrapheDependance::trouve_type(Type *type_racine, TypeRelation type) const
{
	auto noeud = cherche_noeud_type(type_racine);

	for (auto const &relation : noeud->relations) {
		if (relation.type == type) {
			return relation.noeud_fin->type_;
		}
	}

	return nullptr;
}

void GrapheDependance::connecte_noeuds(
		NoeudDependance &noeud1,
		NoeudDependance &noeud2,
		TypeRelation type_relation)
{
	for (auto const &relation : noeud1.relations) {
		if (relation.type == type_relation && relation.noeud_fin == &noeud2) {
			return;
		}
	}

	noeud1.relations.pousse({ type_relation, &noeud1, &noeud2 });
}

void GrapheDependance::ajoute_dependances(
		NoeudDependance &noeud,
		DonneesDependance &donnees)
{
	dls::pour_chaque_element(donnees.types_utilises, [&](auto &type)
	{
		auto noeud_type = cree_noeud_type(type);
		connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_TYPE);
		return dls::DecisionIteration::Continue;
	});

	dls::pour_chaque_element(donnees.fonctions_utilisees, [&](auto &fonction_utilisee)
	{
		auto noeud_type = cherche_noeud_fonction(fonction_utilisee);
		connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_FONCTION);
		return dls::DecisionIteration::Continue;
	});

	dls::pour_chaque_element(donnees.globales_utilisees, [&](auto &globale_utilisee)
	{
		auto noeud_type = cherche_noeud_globale(globale_utilisee);
		connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_GLOBALE);
		return dls::DecisionIteration::Continue;
	});

	/* libère la mémoire */
	donnees.types_utilises.efface();
	donnees.fonctions_utilisees.efface();
	donnees.globales_utilisees.efface();
}

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance)
{
	auto nombre_fonctions = 0;
	auto nombre_inutilisees = 0;

	for (auto noeud : graphe_dependance.noeuds) {
		if (noeud->type == TypeNoeudDependance::FONCTION) {
			std::cerr << "Fonction inutilisée : " << noeud->nom << '\n';
			nombre_fonctions += 1;
			nombre_inutilisees += !noeud->fut_visite;
		}
	}

	std::cerr << nombre_inutilisees << " fonctions sont inutilisées sur " << nombre_fonctions << '\n';
}

/**
 * Algorithme de réduction transitive d'un graphe visant à supprimer les
 * connexions redondantes :
 *
 * Si A dépend de B et de C, et que B dépend de C, nous pouvons supprimer les
 * la relation entre A et C. L'algortithme prend en compte les cas cycliques.
 *
 * Voir : https://en.wikipedia.org/wiki/Transitive_reduction
 *
 * La complexité de l'algorithme serait dans le pire cas O(V * E).
 *
 * Un meilleur algorithme serait :
 * http://www.sciencedirect.com/science/article/pii/0304397588900321/pdf?md5=3391e309b708b6f9cdedcd08f84f4afc&pid=1-s2.
 */

enum {
	ATTEIGNABLE = 1,
	VISITE = 2,
};

static void marque_chemins_atteignables(NoeudDependance &noeud)
{
	if ((noeud.drapeaux & VISITE) != 0) {
		return;
	}

	noeud.drapeaux |= VISITE;

	for (auto &relation : noeud.relations) {
		marque_chemins_atteignables(*relation.noeud_fin);
		relation.noeud_fin->drapeaux |= ATTEIGNABLE;
	}
}

void reduction_transitive(GrapheDependance &graphe_dependance)
{
	std::cout << "Réduction transitive du graphe..." << std::endl;

	auto relations_supprimees = 0;
	auto relations_totales = 0;

	auto relations_filtrees = dls::tableau<Relation>();

	for (auto cible : graphe_dependance.noeuds) {
		/* Réinitialisation des drapeaux. */
		for (auto noeud : graphe_dependance.noeuds) {
			noeud->drapeaux = 0;
		}

		/* Marque les noeuds que nous pouvons atteindre depuis la cible.
		 * Commence avec les enfants, afin que la cible et ses enfants ne soient
		 * pas marqués.
		 */
		cible->drapeaux |= VISITE;
		for (auto &relation : cible->relations) {
			marque_chemins_atteignables(*relation.noeud_fin);
		}

		relations_filtrees = cible->relations;

		for (auto &relation : cible->relations) {
			++relations_totales;

			if ((relation.noeud_fin->drapeaux & ATTEIGNABLE) != 0) {
				++relations_supprimees;
				continue;
			}

			relations_filtrees.pousse(relation);
		}

		cible->relations = relations_filtrees;
	}

	std::cout << "Nombre de relations supprimées : "
			  << relations_supprimees << " sur " << relations_totales
			  << std::endl;
}
