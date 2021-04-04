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

#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "statistiques/statistiques.hh"
#include "typage.hh"

const char *chaine_type_relation(TypeRelation type)
{
    switch (type) {
#define ENUMERE_TYPE_RELATION_EX(type)                                                            \
    case TypeRelation::type:                                                                      \
        return #type;
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

NoeudDependance::NoeudDependance(NoeudDeclarationVariable *globale)
    : m_noeud_globale(globale), m_type_noeud(TypeNoeudDependance::GLOBALE)
{
}

NoeudDependance::NoeudDependance(NoeudDeclarationEnteteFonction *fonction)
    : m_noeud_fonction(fonction), m_type_noeud(TypeNoeudDependance::FONCTION)
{
}

NoeudDependance::NoeudDependance(Type *t) : m_type(t), m_type_noeud(TypeNoeudDependance::TYPE)
{
}

void NoeudDependance::ajoute_relation(Badge<GrapheDependance>, const Relation &relation)
{
    POUR (m_relations.plage()) {
        if (it.type == relation.type && it.noeud_fin == relation.noeud_fin) {
            return;
        }
    }

    m_relations.ajoute(relation);
}

kuri::tableau_compresse<Relation> const &NoeudDependance::relations() const
{
    return m_relations;
}

void NoeudDependance::relations(Badge<GrapheDependance>,
                                kuri::tableau_compresse<Relation> &&relations)
{
    m_relations = relations;
}

NoeudDependance *GrapheDependance::cree_noeud_fonction(
    NoeudDeclarationEnteteFonction *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dependance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dependance = noeud;
    }

    return noeud_syntaxique->noeud_dependance;
}

NoeudDependance *GrapheDependance::cree_noeud_globale(NoeudDeclarationVariable *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dependance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dependance = noeud;
    }

    return noeud_syntaxique->noeud_dependance;
}

NoeudDependance *GrapheDependance::cree_noeud_type(Type *type)
{
    if (type->noeud_dependance == nullptr) {
        auto noeud = noeuds.ajoute_element(type);
        type->noeud_dependance = noeud;
    }

    return type->noeud_dependance;
}

void GrapheDependance::connecte_type_type(NoeudDependance &type1,
                                          NoeudDependance &type2,
                                          TypeRelation type_rel)
{
    assert(type1.est_type());
    assert(type2.est_type());

    type1.ajoute_relation({}, {type_rel, &type1, &type2});
}

void GrapheDependance::connecte_type_type(Type *type1, Type *type2, TypeRelation type_rel)
{
    auto noeud1 = cree_noeud_type(type1);
    auto noeud2 = cree_noeud_type(type2);

    connecte_type_type(*noeud1, *noeud2, type_rel);
}

void GrapheDependance::connecte_noeuds(NoeudDependance &noeud1,
                                       NoeudDependance &noeud2,
                                       TypeRelation type_relation)
{
    noeud1.ajoute_relation({}, {type_relation, &noeud1, &noeud2});
}

void GrapheDependance::rassemble_statistiques(Statistiques &stats) const
{
    auto memoire = 0l;
    memoire += noeuds.memoire_utilisee();

    POUR_TABLEAU_PAGE (noeuds) {
        memoire += it.relations().taille() * taille_de(Relation);
    }

    auto &stats_graphe = stats.stats_graphe_dependance;
    stats_graphe.fusionne_entree({"NoeudDependance", noeuds.taille(), memoire});
}

void GrapheDependance::ajoute_dependances(NoeudDependance &noeud,
                                          DonneesDependance &donnees,
                                          bool efface_donnees)
{
    dls::pour_chaque_element(donnees.types_utilises, [&](auto &type) {
        auto noeud_type = cree_noeud_type(type);
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_TYPE);
        return dls::DecisionIteration::Continue;
    });

    dls::pour_chaque_element(donnees.fonctions_utilisees, [&](auto &fonction_utilisee) {
        auto noeud_type = cree_noeud_fonction(
            const_cast<NoeudDeclarationEnteteFonction *>(fonction_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_FONCTION);
        return dls::DecisionIteration::Continue;
    });

    dls::pour_chaque_element(donnees.globales_utilisees, [&](auto &globale_utilisee) {
        auto noeud_type = cree_noeud_globale(
            const_cast<NoeudDeclarationVariable *>(globale_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_GLOBALE);
        return dls::DecisionIteration::Continue;
    });

    /* libère la mémoire */
    if (efface_donnees) {
        donnees.types_utilises.efface();
        donnees.fonctions_utilisees.efface();
        donnees.globales_utilisees.efface();
    }
}

void imprime_fonctions_inutilisees(GrapheDependance &graphe_dependance)
{
#if 0
	auto nombre_fonctions = 0;
	auto nombre_utilisees = 0;

	POUR_TABLEAU_PAGE(graphe_dependance.noeuds) {
		it.fut_visite = false;
		nombre_fonctions += (it.est_fonction());
	}

	auto noeud_dependance = graphe_dependance.cherche_noeud_fonction("principale");

	graphe_dependance.traverse(noeud_dependance, [&](NoeudDependance *noeud)
	{
		if (!noeud->est_fonction()) {
			return;
		}

		nombre_utilisees += 1;
	});

	POUR_TABLEAU_PAGE(graphe_dependance.noeuds) {
		if (!it.est_fonction()) {
			continue;
		}

		if (it.fut_visite) {
			continue;
		}

		auto decl_fonction = it.fonction();
		std::cerr << "Fonction inutilisée : " << decl_fonction->nom_broye << '\n';
	}

	std::cerr << (nombre_fonctions - nombre_utilisees) << " fonctions sont inutilisées sur " << nombre_fonctions << '\n';
#endif
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

    for (auto &relation : noeud.relations().plage()) {
        marque_chemins_atteignables(*relation.noeud_fin);
        relation.noeud_fin->drapeaux |= ATTEIGNABLE;
    }
}

void GrapheDependance::reduction_transitive()
{
    std::cout << "Réduction transitive du graphe..." << std::endl;

    auto relations_supprimees = 0;
    auto relations_totales = 0;

    auto relations_filtrees = kuri::tableau_compresse<Relation>();

    POUR_TABLEAU_PAGE_NOMME(cible, noeuds)
    {
        /* Réinitialisation des drapeaux. */
        POUR_TABLEAU_PAGE_NOMME(noeud, noeuds)
        {
            noeud.drapeaux = 0;
        }

        /* Marque les noeuds que nous pouvons atteindre depuis la cible.
         * Commence avec les enfants, afin que la cible et ses enfants ne soient
         * pas marqués.
         */
        cible.drapeaux |= VISITE;
        for (auto &relation : cible.relations().plage()) {
            marque_chemins_atteignables(*relation.noeud_fin);
        }

        relations_filtrees = cible.relations();

        for (auto &relation : cible.relations().plage()) {
            ++relations_totales;

            if ((relation.noeud_fin->drapeaux & ATTEIGNABLE) != 0) {
                ++relations_supprimees;
                continue;
            }

            relations_filtrees.ajoute(relation);
        }

        cible.relations({}, std::move(relations_filtrees));
    }

    std::cout << "Nombre de relations supprimées : " << relations_supprimees << " sur "
              << relations_totales << std::endl;
}

void GrapheDependance::rassemble_fonctions_utilisees(NoeudDependance *racine,
                                                     kuri::tableau<AtomeFonction *> &fonctions,
                                                     dls::ensemble<AtomeFonction *> &utilises)
{
    traverse(racine, [&](NoeudDependance *noeud) {
        if (noeud->est_fonction()) {
            auto noeud_fonction = noeud->fonction();
            auto atome_fonction = static_cast<AtomeFonction *>(noeud_fonction->atome);
            assert(atome_fonction);

            if (utilises.trouve(atome_fonction) != utilises.fin()) {
                return;
            }

            fonctions.ajoute(atome_fonction);

            utilises.insere(atome_fonction);
        }
        else if (noeud->est_type()) {
            auto type = noeud->type();

            if (type->genre == GenreType::STRUCTURE || type->genre == GenreType::UNION) {
                auto atome_fonction = type->fonction_init;
                assert(atome_fonction);
                fonctions.ajoute(atome_fonction);
            }
        }
    });
}
