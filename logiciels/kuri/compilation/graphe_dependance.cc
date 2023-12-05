/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "graphe_dependance.hh"

#include <iostream>

#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "statistiques/statistiques.hh"

#include "erreur.h"
#include "log.hh"
#include "metaprogramme.hh"
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

NoeudDependance *GrapheDependance::crée_noeud_fonction(
    NoeudDeclarationEnteteFonction *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dependance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dependance = noeud;
    }

    return noeud_syntaxique->noeud_dependance;
}

NoeudDependance *GrapheDependance::crée_noeud_globale(NoeudDeclarationVariable *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dependance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dependance = noeud;
    }

    return noeud_syntaxique->noeud_dependance;
}

NoeudDependance *GrapheDependance::crée_noeud_type(Type *type)
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
    auto noeud1 = crée_noeud_type(type1);
    auto noeud2 = crée_noeud_type(type2);

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
    auto memoire = int64_t(0);
    memoire += noeuds.memoire_utilisee();

    POUR_TABLEAU_PAGE (noeuds) {
        memoire += it.relations().taille() * taille_de(Relation);
    }

    auto &stats_graphe = stats.stats_graphe_dependance;
    stats_graphe.fusionne_entrée({"NoeudDependance", noeuds.taille(), memoire});
}

void GrapheDependance::ajoute_dependances(NoeudDependance &noeud, DonneesDependance &donnees)
{
    kuri::pour_chaque_element(donnees.types_utilises, [&](auto &type) {
        auto noeud_type = crée_noeud_type(type);
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_TYPE);
        return kuri::DécisionItération::Continue;
    });

    kuri::pour_chaque_element(donnees.fonctions_utilisees, [&](auto &fonction_utilisee) {
        auto noeud_type = crée_noeud_fonction(
            const_cast<NoeudDeclarationEnteteFonction *>(fonction_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_FONCTION);
        return kuri::DécisionItération::Continue;
    });

    kuri::pour_chaque_element(donnees.globales_utilisees, [&](auto &globale_utilisee) {
        auto noeud_type = crée_noeud_globale(
            const_cast<NoeudDeclarationVariable *>(globale_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_GLOBALE);
        return kuri::DécisionItération::Continue;
    });
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
        dbg() << "Fonction inutilisée : " << decl_fonction->nom_broye;
	}

    dbg() << (nombre_fonctions - nombre_utilisees) << " fonctions sont inutilisées sur " << nombre_fonctions;
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

        auto relations_filtrees = kuri::tableau_compresse<Relation>();

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

void GrapheDependance::prepare_visite()
{
    index_visite++;
}

void GrapheDependance::rassemble_fonctions_utilisees(NoeudDependance *racine,
                                                     kuri::tableau<AtomeFonction *> &fonctions,
                                                     kuri::ensemble<AtomeFonction *> &utilises)
{
    prepare_visite();
    traverse(racine, [&](NoeudDependance *noeud) {
        AtomeFonction *atome_fonction = nullptr;

        if (noeud->est_fonction()) {
            auto noeud_fonction = noeud->fonction();
            atome_fonction = noeud_fonction->atome->comme_fonction();
        }
        else if (noeud->est_type()) {
            auto type = noeud->type();
            if (!type->fonction_init) {
                return;
            }
            atome_fonction = type->fonction_init->atome->comme_fonction();
        }
        else {
            return;
        }

        assert(atome_fonction);

        if (utilises.possède(atome_fonction)) {
            return;
        }
        fonctions.ajoute(atome_fonction);
        utilises.insère(atome_fonction);
    });
}

NoeudDependance *GrapheDependance::garantie_noeud_dépendance(EspaceDeTravail *espace,
                                                             NoeudExpression *noeud)
{
    /* N'utilise pas est_declaration_variable_globale car nous voulons également les opaques et
     * les constantes. */
    if (noeud->est_declaration_variable()) {
        assert_rappel(noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE), [&]() {
            dbg() << erreur::imprime_site(*espace, noeud) << '\n' << *noeud;
        });
        return crée_noeud_globale(noeud->comme_declaration_variable());
    }

    if (noeud->est_entete_fonction()) {
        return crée_noeud_fonction(noeud->comme_entete_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        return crée_noeud_fonction(corps->entete);
    }

    if (noeud->est_execute()) {
        auto execute = noeud->comme_execute();
        assert(execute->metaprogramme);
        auto metaprogramme = execute->metaprogramme;
        assert(metaprogramme->fonction);
        return crée_noeud_fonction(metaprogramme->fonction);
    }

    if (noeud->est_declaration_type()) {
        return crée_noeud_type(noeud->type);
    }

    assert(!"Noeud non géré pour les dépendances !\n");
    return nullptr;
}

void imprime_dependances(const DonneesDependance &dependances,
                         EspaceDeTravail *espace,
                         const char *message,
                         std::ostream &flux)
{
    flux << "Dépendances pour : " << message << '\n';

    flux << "fonctions :\n";
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        flux << erreur::imprime_site(*espace, fonction);
        return kuri::DécisionItération::Continue;
    });

    flux << "globales :\n";
    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        flux << erreur::imprime_site(*espace, globale);
        return kuri::DécisionItération::Continue;
    });

    flux << "types :\n";
    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        flux << chaine_type(type) << '\n';
        return kuri::DécisionItération::Continue;
    });
}

void DonneesDependance::fusionne(const DonneesDependance &autre)
{
    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_element(autre.types_utilises, [&](auto &type) {
        if (type->est_type_type_de_donnees()) {
            auto type_de_donnees = type->comme_type_type_de_donnees();
            if (type_de_donnees->type_connu) {
                types_utilises.insere(type_de_donnees->type_connu);
            }
        }
        else {
            types_utilises.insere(type);
        }
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_element(autre.fonctions_utilisees, [&](auto &fonction) {
        fonctions_utilisees.insere(fonction);
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_element(autre.globales_utilisees, [&](auto &globale) {
        globales_utilisees.insere(globale);
        return kuri::DécisionItération::Continue;
    });
}

static void imprime_dépendances(NoeudDependance const *noeud_dep,
                                kuri::chaine_statique nom,
                                void const *adresse,
                                Enchaineuse &sortie)
{
    sortie << nom << " (" << adresse << ") a " << noeud_dep->relations().taille()
           << " relations\n";

    POUR (noeud_dep->relations().plage()) {
        if (it.noeud_fin->est_type()) {
            sortie << "- type " << chaine_type(it.noeud_fin->type()) << '\n';
        }
        else if (it.noeud_fin->est_fonction()) {
            sortie << "- fonction " << nom_humainement_lisible(it.noeud_fin->fonction()) << '\n';
        }
        else if (it.noeud_fin->est_globale()) {
            sortie << "- globale " << nom_humainement_lisible(it.noeud_fin->globale()) << '\n';
        }
    }
}

kuri::chaine imprime_dépendances(NoeudDeclarationSymbole const *symbole)
{
    Enchaineuse sortie;
    imprime_dépendances(
        symbole->noeud_dependance, nom_humainement_lisible(symbole), symbole, sortie);
    return sortie.chaine();
}

kuri::chaine imprime_dépendances(Type const *type)
{
    Enchaineuse sortie;
    imprime_dépendances(type->noeud_dependance, chaine_type(type), type, sortie);
    return sortie.chaine();
}
