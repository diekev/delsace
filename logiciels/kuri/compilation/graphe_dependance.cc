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

#include "compilatrice.hh"
#include "modules.hh"
#include "profilage.hh"

const char *chaine_type_relation(TypeRelation type)
{
	switch (type) {
#define ENUMERE_TYPE_RELATION_EX(type) case TypeRelation::type: return #type;
	ENUMERE_TYPES_RELATION
#undef ENUMERE_TYPE_RELATION_EX
	}
	return "erreur : relation inconnue";
}

std::ostream &operator<<(std::ostream &os, TypeRelation type)
{
	os << chaine_type_relation(type);
	return os;
}

GrapheDependance::~GrapheDependance()
{
	for (auto noeud : noeuds) {
		memoire::deloge("NoeudDependance", noeud);
	}
}

NoeudDependance *GrapheDependance::cree_noeud_fonction(NoeudDeclarationFonction *noeud_syntactique)
{
	PROFILE_FONCTION;

	if (noeud_syntactique->noeud_dependance == nullptr) {
		auto noeud = memoire::loge<NoeudDependance>("NoeudDependance");
		noeud->noeud_syntactique = noeud_syntactique;
		noeud->type = TypeNoeudDependance::FONCTION;

		noeud_syntactique->noeud_dependance = noeud;
		noeuds.pousse(noeud);
	}

	return noeud_syntactique->noeud_dependance;
}

NoeudDependance *GrapheDependance::cree_noeud_globale(NoeudDeclarationVariable *noeud_syntactique)
{
	PROFILE_FONCTION;

	if (noeud_syntactique->noeud_dependance == nullptr) {
		auto noeud = memoire::loge<NoeudDependance>("NoeudDependance");
		noeud->noeud_syntactique = noeud_syntactique;
		noeud->type = TypeNoeudDependance::GLOBALE;

		noeud_syntactique->noeud_dependance = noeud;
		noeuds.pousse(noeud);
	}

	return noeud_syntactique->noeud_dependance;
}

NoeudDependance *GrapheDependance::cree_noeud_type(Type *type)
{
	PROFILE_FONCTION;

	if (type->noeud_dependance == nullptr) {
		auto noeud = memoire::loge<NoeudDependance>("NoeudDependance");
		noeud->type_ = type;
		noeud->type = TypeNoeudDependance::TYPE;

		type->noeud_dependance = noeud;
		noeuds.pousse(noeud);
	}

	return type->noeud_dependance;
}

NoeudDependance *GrapheDependance::cherche_noeud_fonction(const dls::vue_chaine_compacte &nom) const
{
	PROFILE_FONCTION;

	POUR (noeuds) {
		if (it->type != TypeNoeudDependance::FONCTION) {
			continue;
		}

		auto decl_fonction = static_cast<NoeudDeclarationFonction *>(it->noeud_syntactique);

		if (decl_fonction->nom_broye == nom) {
			return it;
		}
	}

	return nullptr;
}

void GrapheDependance::connecte_type_type(NoeudDependance &type1, NoeudDependance &type2, TypeRelation type_rel)
{
	PROFILE_FONCTION;

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

void GrapheDependance::connecte_noeuds(
		NoeudDependance &noeud1,
		NoeudDependance &noeud2,
		TypeRelation type_relation)
{
	PROFILE_FONCTION;

	for (auto const &relation : noeud1.relations) {
		if (relation.type == type_relation && relation.noeud_fin == &noeud2) {
			return;
		}
	}

	noeud1.relations.pousse({ type_relation, &noeud1, &noeud2 });
}

size_t GrapheDependance::memoire_utilisee() const
{
	auto total = 0ul;
	total += static_cast<size_t>(noeuds.taille()) * (sizeof(NoeudDependance *) + sizeof(NoeudDependance));

	POUR (noeuds) {
		total += static_cast<size_t>(it->relations.taille()) * sizeof(Relation);
	}

	return total;
}

void GrapheDependance::ajoute_dependances(
		NoeudDependance &noeud,
		DonneesDependance &donnees)
{
	PROFILE_FONCTION;

	dls::pour_chaque_element(donnees.types_utilises, [&](auto &type)
	{
		auto noeud_type = cree_noeud_type(type);
		connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_TYPE);
		return dls::DecisionIteration::Continue;
	});

	dls::pour_chaque_element(donnees.fonctions_utilisees, [&](auto &fonction_utilisee)
	{
		auto noeud_type = cree_noeud_fonction(const_cast<NoeudDeclarationFonction *>(fonction_utilisee));
		connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_FONCTION);
		return dls::DecisionIteration::Continue;
	});

	dls::pour_chaque_element(donnees.globales_utilisees, [&](auto &globale_utilisee)
	{
		auto noeud_type = cree_noeud_globale(const_cast<NoeudDeclarationVariable *>(globale_utilisee));
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
	auto nombre_utilisees = 0;

	POUR (graphe_dependance.noeuds) {
		it->fut_visite = false;
		nombre_fonctions += (it->type == TypeNoeudDependance::FONCTION);
	}

	auto noeud_dependance = graphe_dependance.cherche_noeud_fonction("principale");

	traverse_graphe(noeud_dependance, [&](NoeudDependance *noeud)
	{
		if (noeud->type != TypeNoeudDependance::FONCTION) {
			return;
		}

		nombre_utilisees += 1;
	});

	POUR (graphe_dependance.noeuds) {
		if (it->type != TypeNoeudDependance::FONCTION) {
			continue;
		}

		if (it->fut_visite) {
			continue;
		}

		auto decl_fonction = static_cast<NoeudDeclarationFonction *>(it->noeud_syntactique);
		std::cerr << "Fonction inutilisée : " << decl_fonction->nom_broye << '\n';
	}

	std::cerr << (nombre_fonctions - nombre_utilisees) << " fonctions sont inutilisées sur " << nombre_fonctions << '\n';
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
	PROFILE_FONCTION;

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
