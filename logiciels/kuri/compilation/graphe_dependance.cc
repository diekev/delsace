/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "graphe_dependance.hh"

#include <iostream>

#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "statistiques/statistiques.hh"

#include "erreur.h"
#include "metaprogramme.hh"
#include "typage.hh"
#include "utilitaires/log.hh"

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

NoeudDépendance::NoeudDépendance(NoeudDéclarationVariable *globale)
    : m_noeud_globale(globale), m_type_noeud(TypeNoeudDependance::GLOBALE)
{
}

NoeudDépendance::NoeudDépendance(NoeudDéclarationEntêteFonction *fonction)
    : m_noeud_fonction(fonction), m_type_noeud(TypeNoeudDependance::FONCTION)
{
}

NoeudDépendance::NoeudDépendance(Type *t) : m_type(t), m_type_noeud(TypeNoeudDependance::TYPE)
{
}

void NoeudDépendance::ajoute_relation(Badge<GrapheDépendance>, const Relation &relation)
{
    POUR (m_relations.plage()) {
        if (it.type == relation.type && it.noeud_fin == relation.noeud_fin) {
            return;
        }
    }

    m_relations.ajoute(relation);
}

kuri::tableau_compresse<Relation> const &NoeudDépendance::relations() const
{
    return m_relations;
}

void NoeudDépendance::relations(Badge<GrapheDépendance>,
                                kuri::tableau_compresse<Relation> &&relations)
{
    m_relations = relations;
}

NoeudDépendance *GrapheDépendance::crée_noeud_fonction(
    NoeudDéclarationEntêteFonction *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dépendance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dépendance = noeud;
    }

    return noeud_syntaxique->noeud_dépendance;
}

NoeudDépendance *GrapheDépendance::crée_noeud_globale(NoeudDéclarationVariable *noeud_syntaxique)
{
    if (noeud_syntaxique->noeud_dépendance == nullptr) {
        auto noeud = noeuds.ajoute_element(noeud_syntaxique);
        noeud_syntaxique->noeud_dépendance = noeud;
    }

    return noeud_syntaxique->noeud_dépendance;
}

NoeudDépendance *GrapheDépendance::crée_noeud_type(Type *type)
{
    if (type->noeud_dépendance == nullptr) {
        auto noeud = noeuds.ajoute_element(type);
        type->noeud_dépendance = noeud;
    }

    return type->noeud_dépendance;
}

void GrapheDépendance::connecte_type_type(NoeudDépendance &type1,
                                          NoeudDépendance &type2,
                                          TypeRelation type_rel)
{
    assert(type1.est_type());
    assert(type2.est_type());

    type1.ajoute_relation({}, {type_rel, &type1, &type2});
}

void GrapheDépendance::connecte_type_type(Type *type1, Type *type2, TypeRelation type_rel)
{
    auto noeud1 = crée_noeud_type(type1);
    auto noeud2 = crée_noeud_type(type2);

    connecte_type_type(*noeud1, *noeud2, type_rel);
}

void GrapheDépendance::connecte_noeuds(NoeudDépendance &noeud1,
                                       NoeudDépendance &noeud2,
                                       TypeRelation type_relation)
{
    noeud1.ajoute_relation({}, {type_relation, &noeud1, &noeud2});
}

void GrapheDépendance::rassemble_statistiques(Statistiques &stats) const
{
    auto memoire = int64_t(0);
    memoire += noeuds.memoire_utilisee();

    POUR_TABLEAU_PAGE (noeuds) {
        memoire += it.relations().taille_mémoire();
    }

    auto &stats_graphe = stats.stats_graphe_dépendance;
    stats_graphe.fusionne_entrée({"NoeudDependance", noeuds.taille(), memoire});
}

void GrapheDépendance::ajoute_dépendances(NoeudDépendance &noeud, DonnéesDépendance &donnees)
{
    kuri::pour_chaque_élément(donnees.types_utilisés, [&](auto &type) {
        auto noeud_type = crée_noeud_type(type);
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_TYPE);
        return kuri::DécisionItération::Continue;
    });

    //    if (donnees.init_types_utilisés.taille()) {
    //        dbg() << __func__ << " " << donnees.init_types_utilisés.taille();
    //    }
    kuri::pour_chaque_élément(donnees.init_types_utilisés, [&](auto &type) {
        auto noeud_type = crée_noeud_type(type);
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_INIT_TYPE);
        return kuri::DécisionItération::Continue;
    });

    kuri::pour_chaque_élément(donnees.fonctions_utilisées, [&](auto &fonction_utilisee) {
        auto noeud_type = crée_noeud_fonction(
            const_cast<NoeudDéclarationEntêteFonction *>(fonction_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_FONCTION);
        return kuri::DécisionItération::Continue;
    });

    kuri::pour_chaque_élément(donnees.globales_utilisées, [&](auto &globale_utilisee) {
        auto noeud_type = crée_noeud_globale(
            const_cast<NoeudDéclarationVariable *>(globale_utilisee));
        connecte_noeuds(noeud, *noeud_type, TypeRelation::UTILISE_GLOBALE);
        return kuri::DécisionItération::Continue;
    });
}

void imprime_fonctions_inutilisées(GrapheDépendance &graphe_dependance)
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

static void marque_chemins_atteignables(NoeudDépendance &noeud)
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

void GrapheDépendance::réduction_transitive()
{
    info() << "Réduction transitive du graphe...";

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

    info() << "Nombre de relations supprimées : " << relations_supprimees << " sur "
           << relations_totales;
}

void GrapheDépendance::prépare_visite()
{
    index_visite++;
}

void GrapheDépendance::rassemble_fonctions_utilisées(NoeudDépendance *racine,
                                                     kuri::tableau<AtomeFonction *> &fonctions,
                                                     kuri::ensemble<AtomeFonction *> &utilises)
{
    prépare_visite();
    traverse(racine, [&](NoeudDépendance *noeud) {
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

NoeudDépendance *GrapheDépendance::garantie_noeud_dépendance(EspaceDeTravail *espace,
                                                             NoeudExpression *noeud)
{
    /* N'utilise pas est_declaration_variable_globale car nous voulons également les opaques et
     * les constantes. */
    if (noeud->est_déclaration_variable()) {
        assert_rappel(noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE), [&]() {
            dbg() << erreur::imprime_site(*espace, noeud) << '\n' << *noeud;
        });
        return crée_noeud_globale(noeud->comme_déclaration_variable());
    }

    if (noeud->est_déclaration_variable_multiple()) {
        assert_rappel(noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE), [&]() {
            dbg() << erreur::imprime_site(*espace, noeud) << '\n' << *noeud;
        });

        auto decl = noeud->comme_déclaration_variable_multiple();
        POUR (decl->données_decl.plage()) {
            POUR_NOMME (ref, it.variables.plage()) {
                crée_noeud_globale(ref->comme_référence_déclaration()
                                       ->déclaration_référée->comme_déclaration_variable());
            }
        }

        /* À FAIRE : retourne tous les noeuds. */
        return crée_noeud_globale(decl->données_decl[0]
                                      .variables[0]
                                      ->comme_référence_déclaration()
                                      ->déclaration_référée->comme_déclaration_variable());
    }

    if (noeud->est_entête_fonction()) {
        return crée_noeud_fonction(noeud->comme_entête_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        return crée_noeud_fonction(corps->entête);
    }

    if (noeud->est_exécute()) {
        auto execute = noeud->comme_exécute();
        assert(execute->métaprogramme);
        auto metaprogramme = execute->métaprogramme;
        assert(metaprogramme->fonction);
        return crée_noeud_fonction(metaprogramme->fonction);
    }

    if (noeud->est_déclaration_type()) {
        return crée_noeud_type(noeud->comme_déclaration_type());
    }

    assert(!"Noeud non géré pour les dépendances !\n");
    return nullptr;
}

void imprime_dépendances(const DonnéesDépendance &dependances,
                         EspaceDeTravail *espace,
                         const char *message,
                         std::ostream &flux)
{
    flux << "Dépendances pour : " << message << '\n';

    flux << "fonctions :\n";
    kuri::pour_chaque_élément(dependances.fonctions_utilisées, [&](auto &fonction) {
        flux << erreur::imprime_site(*espace, fonction);
        return kuri::DécisionItération::Continue;
    });

    flux << "globales :\n";
    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_élément(dependances.globales_utilisées, [&](auto &globale) {
        flux << erreur::imprime_site(*espace, globale);
        return kuri::DécisionItération::Continue;
    });

    flux << "types :\n";
    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_élément(dependances.types_utilisés, [&](auto &type) {
        flux << chaine_type(type) << '\n';
        return kuri::DécisionItération::Continue;
    });
}

void DonnéesDépendance::fusionne(const DonnéesDépendance &autre)
{
    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_élément(autre.types_utilisés, [&](auto &type) {
        if (type->est_type_type_de_données()) {
            auto type_de_donnees = type->comme_type_type_de_données();
            if (type_de_donnees->type_connu) {
                types_utilisés.insère(type_de_donnees->type_connu);
            }
        }
        else {
            types_utilisés.insère(type);
        }
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_élément(autre.fonctions_utilisées, [&](auto &fonction) {
        fonctions_utilisées.insère(fonction);
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    pour_chaque_élément(autre.globales_utilisées, [&](auto &globale) {
        globales_utilisées.insère(globale);
        return kuri::DécisionItération::Continue;
    });
}

static void imprime_dépendances(NoeudDépendance const *noeud_dep,
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

kuri::chaine imprime_dépendances(NoeudDéclarationSymbole const *symbole)
{
    Enchaineuse sortie;
    imprime_dépendances(
        symbole->noeud_dépendance, nom_humainement_lisible(symbole), symbole, sortie);
    return sortie.chaine();
}

kuri::chaine imprime_dépendances(Type const *type)
{
    Enchaineuse sortie;
    imprime_dépendances(type->noeud_dépendance, chaine_type(type), type, sortie);
    return sortie.chaine();
}
