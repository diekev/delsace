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

#include "commandes_noeuds.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "commande_jorjala.hh"

#include "coeur/jorjala.hh"

#if 0

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "danjo/danjo.h"

#include "evaluation/evaluation.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/imprimeuse_graphe.h"
#include "coeur/manipulatrice.h"
#include "coeur/jorjala.hh"
#include "coeur/noeud_image.h"
#include "coeur/nuanceur.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_graphe_detail.hh"
#include "coeur/operatrice_image.h"
#include "coeur/operatrice_simulation.hh"
#include "coeur/rendu.hh"
#include "coeur/usine_operatrice.h"

#include "operatrices/operatrices_cycles.hh"

#include "lcc/contexte_execution.hh"
#include "lcc/lcc.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif

/* ************************************************************************** */

/**
 * Ajourne le noeud_actif du graphe pour être égal au noeud spécifié et
 * retourne vrai si le graphe doit être évalué, c'est-à-dire si une visionneuse
 * différente de la dernière visionneuse a été sélectionnée. La fonction ajourne
 * également le pointeur derniere_visionneuse_selectionnee du graphe si le noeud
 * est une visionneuse.
 */
static bool selectionne_noeud(JJL::Jorjala &jorjala, JJL::Noeud noeud, JJL::Graphe graphe)
{
    graphe.noeud_actif(noeud);

    if (graphe.noeud_actif() == nullptr) {
		return false;
	}

#if 0
	using dls::outils::est_element;

	if (est_element(noeud->type, type_noeud::OBJET, type_noeud::COMPOSITE, type_noeud::RENDU, type_noeud::INVALIDE, type_noeud::NUANCEUR)) {
		/* À FAIRE : considère avoir et mettre en place un objet actif. */
		return false;
	}

	if (noeud->sorties.est_vide()) {
		auto const besoin_ajournement = (graphe.noeud_actif != graphe.dernier_noeud_sortie);

		graphe.dernier_noeud_sortie = noeud;

		return besoin_ajournement;
	}

	auto operatrice = extrait_opimage(noeud->donnees);

	if (operatrice->possede_manipulatrice_3d(jorjala.type_manipulation_3d)) {
		jorjala.manipulatrice_3d = operatrice->manipulatrice_3d(jorjala.type_manipulation_3d);
	}
	else {
		jorjala.manipulatrice_3d = nullptr;
	}
#endif

	return false;
}

#if 0
/**
 * Retourne vrai si le noeud possède une connexion vers la sortie.
 */
static bool noeud_connecte_sortie(Noeud *noeud, Noeud *sortie)
{
	if (noeud == nullptr) {
		return false;
	}

	if (noeud->sorties.est_vide()) {
		return true;
	}

	for (auto prise_sortie : noeud->sorties) {
		for (auto prise_entree : prise_sortie->liens) {
			if (noeud_connecte_sortie(prise_entree->parent, sortie)) {
				return true;
			}
		}
	}

	return false;
}
#endif

/**
 * Retourne vrai si l'entrée et la sortie sont compatibles pour être connectées.
 */
static bool peut_connecter(JJL::PriseEntree entree, JJL::PriseSortie sortie)
{
	if (entree == nullptr || sortie == nullptr) {
		return false;
	}

    if (entree.noeud_parent() == sortie.noeud_parent()) {
		return false;
	}

    if (entree.type() != sortie.type()) {
#if 0
		if (entree->type == type_prise::POLYMORPHIQUE || sortie->type == type_prise::POLYMORPHIQUE) {
			return true;
		}
#endif

		return false;
	}

	return true;
}

/* ************************************************************************** */

class CommandeDessineGrapheComposite final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &/*donnees*/) override
	{
#if 0
        auto const &noeud_composite = jorjala.bdd.graphe_composites()->noeud_actif;

		if (noeud_composite == nullptr) {
            jorjala.affiche_erreur("Aucun noeud composite sélectionné");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &composite = extrait_composite(noeud_composite->donnees);

		ImprimeuseGraphe gd(&composite->noeud->graphe);
		gd("/tmp/graphe_composite.gv");

		if (system("dot /tmp/graphe_composite.gv -Tpng -o /tmp/graphe_composite.png") == -1) {
			std::cerr << "Impossible de créer une image depuis 'dot'\n";
			return EXECUTION_COMMANDE_ECHOUEE;
		}
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static bool finalise_ajout_noeud(
        JJL::Jorjala &jorjala,
        JJL::Graphe &graphe,
        JJL::Noeud &noeud)
{
    noeud.pos_x(graphe.centre_x());
    noeud.pos_y(graphe.centre_y());

#if 0
	if (graphe.connexion_active != nullptr) {
		if (graphe.connexion_active->prise_entree != nullptr) {
			graphe.connecte(noeud.sortie(0), graphe.connexion_active->prise_entree);
		}
		else if (graphe.connexion_active->prise_sortie != nullptr) {
			graphe.connecte(graphe.connexion_active->prise_sortie, noeud.entree(0));
		}

		memoire::deloge("Connexion", graphe.connexion_active);
	}
#endif

    auto besoin_evaluation = selectionne_noeud(jorjala, noeud, graphe);

    jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);

	return besoin_evaluation;
}

/* ************************************************************************** */

class CommandeAjoutNoeud final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
		auto nom = donnees.metadonnee;
        auto graphe = jorjala.graphe();

        auto noeud = jorjala.crée_noeud_dans_graphe(graphe, nom.c_str());
        if (noeud == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto besoin_evaluation = finalise_ajout_noeud(jorjala, graphe, noeud);

		if (besoin_evaluation) {
#if 0
			requiers_evaluation(*jorjala, NOEUD_AJOUTE, "noeud ajouté");
#endif
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeudDetail final : public CommandeJorjala {
public:
    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
	{
#if 0
        auto graphe = jorjala.graphe;

		if (graphe->type != type_graphe::DETAIL) {
			return false;
		}

		if (graphe->donnees.est_vide()) {
			return false;
		}

        auto const &lcc = jorjala.lcc;
		auto type_detail = std::any_cast<int>(graphe->donnees[0]);

		using dls::outils::est_element;
		using dls::outils::possede_drapeau;

		auto const &df = lcc->fonctions.table[metadonnee][0];

		auto detail_points = est_element(
					type_detail,
					DETAIL_PIXELS,
					DETAIL_POINTS,
					DETAIL_VOXELS,
					DETAIL_TERRAIN,
					DETAIL_NUANCAGE,
					DETAIL_POSEIDON_GAZ);

		if (detail_points) {
			return possede_drapeau(df.ctx, lcc::ctx_script::detail);
		}
#endif

		return false;
	}

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		auto nom = donnees.metadonnee;
        auto graphe = jorjala.graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);
		noeud->graphe.type = type_graphe::DETAIL;

		auto op = cree_op_detail(*jorjala, *graphe, *noeud, nom);
		op->cree_proprietes();
		synchronise_donnees_operatrice(*noeud);
        initialise_entreface(jorjala.gestionnaire_entreface, op, op->chemin_entreface());

		finalise_ajout_noeud(*jorjala, *graphe, *noeud);
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeudCycles final : public CommandeJorjala {
public:
    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
	{
#if 0
        auto graphe = jorjala.graphe;

		if (graphe->type != type_graphe::CYCLES) {
			return false;
		}

		return true;
#else
        return false;
#endif
	}

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		auto nom = donnees.metadonnee;
        auto graphe = jorjala.graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);

		auto op = OperatriceCycles::cree(*graphe, *noeud, nom);
		synchronise_donnees_operatrice(*noeud);
        initialise_entreface(jorjala.gestionnaire_entreface, op, op->chemin_entreface());

		finalise_ajout_noeud(*jorjala, *graphe, *noeud);
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeudDetailSpecial final : public CommandeJorjala {
public:
    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
	{
#if 0
        auto graphe = jorjala.graphe;

		if (graphe->type != type_graphe::DETAIL) {
			return false;
		}

		if (graphe->donnees.est_vide()) {
			return false;
		}

		auto type_detail = std::any_cast<int>(graphe->donnees[0]);

		using dls::outils::est_element;

		if (est_element(metadonnee,
						"Entrée Détail",
						"Sortie Détail",
						"Info Exécution",
						"Charge Image",
						"Cherche Caméra",
						"Crée Courbe Couleur",
						"Crée Courbe Valeur",
						"Crée Rampe Couleur"))
		{
			return est_element(type_detail,
							   DETAIL_PIXELS,
							   DETAIL_VOXELS,
							   DETAIL_POINTS,
							   DETAIL_TERRAIN,
							   DETAIL_POSEIDON_GAZ,
							   DETAIL_NUANCAGE);
		}

		if (est_element(metadonnee, "Entrée Attribut")) {
			return est_element(type_detail, DETAIL_POINTS, DETAIL_NUANCAGE);
		}

		if (est_element(metadonnee, "Sortie Attribut")) {
			return est_element(type_detail, DETAIL_POINTS);
		}

#endif
		return false;
	}

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		auto nom = donnees.metadonnee;
        auto graphe = jorjala.graphe;

		if (graphe->type != type_graphe::DETAIL) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);

        auto op = (jorjala.usine_operatrices())(nom, *graphe, *noeud);
		synchronise_donnees_operatrice(*noeud);

        initialise_entreface(jorjala.gestionnaire_entreface, op, op->chemin_entreface());

		auto besoin_evaluation = finalise_ajout_noeud(*jorjala, *graphe, *noeud);

		if (besoin_evaluation) {
			requiers_evaluation(*jorjala, NOEUD_AJOUTE, "noeud ajouté");
		}
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutGrapheDetail final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		auto nom = donnees.metadonnee;
		auto type_detail = 0;

		if (nom == "détail_point") {
			type_detail = DETAIL_POINTS;
		}
		else if (nom == "détail_voxel") {
			type_detail = DETAIL_VOXELS;
		}
		else if (nom == "détail_pixel") {
			type_detail = DETAIL_PIXELS;
		}
		else {
            jorjala.affiche_erreur("Type de graphe détail inconnu");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        auto graphe = jorjala.graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);

        auto op = (jorjala.usine_operatrices())("Graphe Détail", *graphe, *noeud);

		auto op_graphe = dynamic_cast<OperatriceGrapheDetail *>(op);
		op_graphe->type_detail = type_detail;
		/* il faut que le type de détail soit correct car il est possible que
		 * l'opératrice ne fut pas exécutée avant que des noeuds soient ajoutés
		 * dans son graphe */
		noeud->graphe.donnees.efface();
		noeud->graphe.donnees.ajoute(type_detail);
		noeud->graphe.type = type_graphe::DETAIL;

		/* la synchronisation doit se faire après puisque nous avons besoin du
		 * type de détail pour déterminer les types de sorties */
		synchronise_donnees_operatrice(*noeud);

        initialise_entreface(jorjala.gestionnaire_entreface, op, op->chemin_entreface());

		auto besoin_evaluation = finalise_ajout_noeud(*jorjala, *graphe, *noeud);

		if (besoin_evaluation) {
			requiers_evaluation(*jorjala, NOEUD_AJOUTE, "noeud ajouté");
		}
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static JJL::Noeud trouve_noeud(JJL::Graphe &graphe, float x, float y)
{
    JJL::Noeud noeud_le_plus_proche(nullptr);
    float distance_la_plus_proche = std::numeric_limits<float>::max();

    for (auto noeud : graphe.noeuds()) {
        auto rectangle_noeud = noeud.rectangle();

        if (!rectangle_noeud.contient(x, y)) {
            continue;
        }

        auto centre = rectangle_noeud.position_centrale();

        auto dx = centre.x() - x;
        auto dy = centre.y() - y;

        auto distance_carrée = dx * dx + dy * dy;

        if (distance_carrée < distance_la_plus_proche) {
            distance_la_plus_proche = distance_carrée;
            noeud_le_plus_proche = noeud;
        }
    }

    return noeud_le_plus_proche;
}

static JJL::PriseSortie trouve_prise_sortie(JJL::Noeud &noeud, float x, float y)
{
    for (auto sortie : noeud.sorties()) {
        if (sortie.rectangle().contient(x, y)) {
            return sortie;
        }
    }

    return nullptr;
}

static JJL::PriseSortie trouve_prise_sortie(JJL::Graphe &graphe, float x, float y)
{
    JJL::Noeud noeud_le_plus_proche = trouve_noeud(graphe, x, y);

    if (noeud_le_plus_proche == nullptr) {
        return nullptr;
    }

    return trouve_prise_sortie(noeud_le_plus_proche, x, y);
}

static JJL::PriseEntree trouve_prise_entree(JJL::Noeud &noeud, float x, float y)
{
    for (auto entree : noeud.entrées()) {
        if (entree.rectangle().contient(x, y)) {
            return entree;
        }
    }

    return nullptr;
}

static JJL::PriseEntree trouve_prise_entree(JJL::Graphe &graphe, float x, float y)
{
    JJL::Noeud noeud_le_plus_proche = trouve_noeud(graphe, x, y);

    if (noeud_le_plus_proche == nullptr) {
        return nullptr;
    }

    return trouve_prise_entree(noeud_le_plus_proche, x, y);
}

static void trouve_noeud_prise(JJL::Graphe &graphe, float x, float y, JJL::Noeud *r_noeud, JJL::PriseEntree *r_entree, JJL::PriseSortie *r_sortie)
{
    JJL::Noeud noeud_le_plus_proche = trouve_noeud(graphe, x, y);

    if (noeud_le_plus_proche == nullptr) {
        return;
    }

    *r_noeud = noeud_le_plus_proche;
    *r_entree = trouve_prise_entree(noeud_le_plus_proche, x, y);
    *r_sortie = trouve_prise_sortie(noeud_le_plus_proche, x, y);
}

class CommandeSelectionGraphe final : public CommandeJorjala {
	float delta_x = 0.0f;
	float delta_y = 0.0f;
	bool m_prise_entree_deconnectee = false;
    bool m_chose_sélectionnée = false;
    char m_pad[5];


public:
	CommandeSelectionGraphe()
        : CommandeJorjala()
	{
		INUTILISE(m_pad); /* Pour faire taire les avertissements. */
	}

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
        auto graphe = jorjala.graphe();

        JJL::Noeud noeud_selection = nullptr;
        JJL::PriseEntree prise_entree = nullptr;
        JJL::PriseSortie prise_sortie = nullptr;

        trouve_noeud_prise(graphe, donnees.x, donnees.y, &noeud_selection, &prise_entree, &prise_sortie);

        if (noeud_selection  != nullptr) {
            delta_x = donnees.x - noeud_selection.pos_x();
            delta_y = donnees.y - noeud_selection.pos_y();
            m_chose_sélectionnée = true;
		}

        if (prise_entree != nullptr || prise_sortie != nullptr) {
            auto connexion = graphe.débute_connexion_interactive();
            connexion.x(donnees.x);
            connexion.y(donnees.y);

            if (prise_entree != nullptr && prise_entree.connexion() != nullptr) {
                prise_sortie = prise_entree.connexion().prise_sortie();
                graphe.déconnecte(prise_entree);
                prise_entree = nullptr;

#if 0
					m_prise_entree_deconnectee = noeud_connecte_sortie(
													 prise_entree->parent,
													 graphe->dernier_noeud_sortie);
#endif
			}

            connexion.prise_entrée(prise_entree);
            connexion.prise_sortie(prise_sortie);
            m_chose_sélectionnée = true;
		}

        bool besoin_evaluation = selectionne_noeud(jorjala, noeud_selection, graphe);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::SÉLECTIONNÉ);

		/* évalue le graphe si un visionneur a été sélectionné */
		if (besoin_evaluation) {
#if 0
			requiers_evaluation(*jorjala, NOEUD_SELECTIONE, "noeud sélectionné");
#endif
		}

        if (m_chose_sélectionnée) {
            return EXECUTION_COMMANDE_MODALE;
        }

        return EXECUTION_COMMANDE_REUSSIE;
	}

    void ajourne_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto graphe = jorjala.graphe();

        if (graphe.connexion_interactive()) {
            auto connexion = graphe.débute_connexion_interactive();
            connexion.x(donnees.x);
            connexion.y(donnees.y);
		}
        else {
            auto noeud_actif = graphe.noeud_actif();

			if (noeud_actif == nullptr) {
				return;
			}

            noeud_actif.pos_x(donnees.x - delta_x);
            noeud_actif.pos_y(donnees.y - delta_y);
		}

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);
	}

    void termine_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto graphe = jorjala.graphe();

#if 0
		bool connexion_sortie = false;
#endif

        if (graphe.connexion_interactive()) {
            auto connexion = graphe.connexion_interactive();
            JJL::PriseEntree entree = nullptr;
            JJL::PriseSortie sortie = nullptr;

            if (connexion.prise_entrée() != nullptr) {
                entree = connexion.prise_entrée();
                sortie = trouve_prise_sortie(graphe, donnees.x, donnees.y);
			}
			else {
                entree = trouve_prise_entree(graphe, donnees.x, donnees.y);
                sortie = connexion.prise_sortie();
			}

            if (peut_connecter(entree, sortie)) {
                graphe.crée_connexion(entree, sortie);

#if 0
				connexion_sortie = noeud_connecte_sortie(
								entree->parent,
								graphe->dernier_noeud_sortie);
#endif
			}

            graphe.termine_connexion_interactive();
        }

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

#if 0
		if (connexion_sortie || m_prise_entree_deconnectee) {
			if (graphe->noeud_parent.type == type_noeud::NUANCEUR) {
				auto nuanceur = extrait_nuanceur(graphe->noeud_parent.donnees);
				nuanceur->temps_modifie += 1;
			}

			marque_parent_surannee(&graphe->noeud_parent, [](Noeud *n, PriseEntree *prise)
			{
				if (n->type != type_noeud::OPERATRICE) {
					return;
				}

				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			requiers_evaluation(*jorjala, GRAPHE_MODIFIE, "graphe modifié");
		}
#endif
	}

    JJL::TypeCurseur type_curseur_modal() override
    {
        return JJL::TypeCurseur::MAIN_FERMÉE;
    }
};

/* ************************************************************************** */

class CommandeSupprimeSelection final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &/*donnees*/) override
	{
#if 0
        auto graphe = jorjala.graphe;
		auto noeud = graphe->noeud_actif;

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (noeud == graphe->dernier_noeud_sortie) {
			graphe->dernier_noeud_sortie = nullptr;
		}

		auto besoin_execution = false;

		switch (noeud->type) {
			case type_noeud::INVALIDE:
			{
				break;
			}
			case type_noeud::OBJET:
			{
				besoin_execution = true;

				auto objet = extrait_objet(noeud->donnees);
                jorjala.bdd.enleve_objet(objet);
				break;
			}
			case type_noeud::COMPOSITE:
			{
				besoin_execution = true;

				auto compo = extrait_composite(noeud->donnees);
                jorjala.bdd.enleve_composite(compo);
				break;
			}
			case type_noeud::NUANCEUR:
			{
				besoin_execution = true;

				auto nuanceur = extrait_nuanceur(noeud->donnees);
                jorjala.bdd.enleve_nuanceur(nuanceur);
				break;
			}
			case type_noeud::RENDU:
			{
				besoin_execution = true;

				auto rendu = extrait_rendu(noeud->donnees);
                jorjala.bdd.enleve_rendu(rendu);
				break;
			}
			case type_noeud::OPERATRICE:
			{
				auto operatrice = extrait_opimage(noeud->donnees);

                if (operatrice->manipulatrice_3d(jorjala.type_manipulation_3d) == jorjala.manipulatrice_3d) {
                    jorjala.manipulatrice_3d = nullptr;
				}

				besoin_execution = noeud_connecte_sortie(noeud, graphe->dernier_noeud_sortie);

				graphe->supprime(noeud);

				break;
			}
		}

		graphe->noeud_actif = nullptr;

        jorjala.notifie_observatrices(type_evenement::noeud | type_evenement::enleve);

		if (besoin_execution) {
			if (graphe->noeud_parent.type == type_noeud::NUANCEUR) {
				auto nuanceur = extrait_nuanceur(graphe->noeud_parent.donnees);
				nuanceur->temps_modifie += 1;
			}

			marque_parent_surannee(&graphe->noeud_parent, [](Noeud *n, PriseEntree *prise)
			{
				if (n->type != type_noeud::OPERATRICE) {
					return;
				}

				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			requiers_evaluation(*jorjala, NOEUD_ENLEVE, "noeud supprimé");
		}
#endif

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

#if 0
static const char *chaine_attribut(type_attribut type)
{
	switch (type) {
		case type_attribut::Z8:
			return "z8";
		case type_attribut::Z16:
			return "z16";
		case type_attribut::Z32:
			return "z32";
		case type_attribut::Z64:
			return "z64";
		case type_attribut::N8:
			return "n8";
		case type_attribut::N16:
			return "n16";
		case type_attribut::N32:
			return "n32";
		case type_attribut::N64:
			return "n64";
		case type_attribut::R16:
			return "r16";
		case type_attribut::R32:
			return "r32";
		case type_attribut::R64:
			return "r64";
		case type_attribut::INVALIDE:
			return "invalide";
		case type_attribut::CHAINE:
			return "chaine";
	}

	return "quelque chose va mal";
}

static const char *chaine_portee(portee_attr portee)
{
	switch (portee) {
		case portee_attr::CORPS:
			return "corps";
		case portee_attr::POINT:
			return "point";
		case portee_attr::PRIMITIVE:
			return "primitive";
		case portee_attr::VERTEX:
			return "vertex";
		case portee_attr::GROUPE:
			return "groupe";
	}

	return "quelque chose va mal";
}
#endif

class CommandeInfoNoeud final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
        auto graphe = jorjala.graphe;
		auto noeud = trouve_noeud(graphe->noeuds(), donnees.x, donnees.y);

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto sans_info = dls::outils::est_element(
					noeud->type,
					type_noeud::OBJET,
					type_noeud::COMPOSITE);

		if (sans_info) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (noeud != nullptr) {
			auto info_noeud = memoire::loge<InfoNoeud>("InfoNoeud");
			info_noeud->x = donnees.x;
			info_noeud->y = donnees.y;

			dls::flux_chaine ss;
			ss << "<p>Opératrice : " << noeud->nom << "</p>";
			ss << "<hr/>";

			auto op = extrait_opimage(noeud->donnees);

			if (op->type() == OPERATRICE_CORPS) {
				auto op_objet = dynamic_cast<OperatriceCorps *>(op);
				auto corps = op_objet->corps();

				ss << "<p>Points         : " << corps->points_pour_lecture().taille() << "</p>";
				ss << "<p>Prims          : " << corps->prims()->taille() << "</p>";

				ss << "<hr/>";

				ss << "<p>Groupes points : " << corps->groupes_points().taille() << "</p>";
				ss << "<p>Groupes prims  : " << corps->groupes_prims().taille() << "</p>";

				ss << "<hr/>";

				ss << "<p>Attributs : </p>";

				for (auto const &attr : corps->attributs()) {
					ss << "<p>"
					   << attr.nom()
					   << " : "
					   << chaine_attribut(attr.type())
					   << " (" << chaine_portee(attr.portee) << ")"
					   << "</p>";
				}

				ss << "<hr/>";
			}
			else if (op->type() == OPERATRICE_IMAGE) {
				auto image = op->image();

				if (image->est_profonde) {
					ss << "<p>Image profonde</p>";
					ss << "<hr/>";

					ss << "<p>Calques : </p>";

					for (auto calque : image->m_calques_profond) {
						ss << "<p>" << calque->nom << " (";
						ss << calque->tampon()->desc().resolution.x;
						ss << "x";
						ss << calque->tampon()->desc().resolution.y;
						ss << ")</p>";
					}
				}
				else {
					ss << "<p>Image plate</p>";
					ss << "<hr/>";

					for (auto calque : image->calques()) {
						ss << "<p>" << calque->nom << " (";
						ss << calque->tampon()->desc().resolution.x;
						ss << "x";
						ss << calque->tampon()->desc().resolution.y;
						ss << ")</p>";
					}
				}

				ss << "<hr/>";
			}

			ss << "<p>Temps d'exécution :";
			ss << "<p>- dernière : " << noeud->temps_execution << " secondes.</p>";
			ss << "<hr/>";
			ss << "<p>Nombre d'exécution : " << noeud->executions << "</p>";
			ss << "<hr/>";

			info_noeud->informations = ss.chn();

			graphe->info_noeud = info_noeud;
		}

		selectionne_noeud(*jorjala, noeud, *graphe);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::SÉLECTIONNÉ);
#endif

		return EXECUTION_COMMANDE_MODALE;
	}

    void termine_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		INUTILISE(donnees);

        auto graphe = jorjala.graphe;

		memoire::deloge("InfoNoeud", graphe->info_noeud);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);
#endif
    }
};

/* ************************************************************************** */

class CommandeDeplaceGraphe final : public CommandeJorjala {
	float m_orig_x = 0.0f;
	float m_orig_y = 0.0f;

public:
	CommandeDeplaceGraphe() = default;

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
        INUTILISE(jorjala);

		m_orig_x = donnees.x;
		m_orig_y = donnees.y;

		return EXECUTION_COMMANDE_MODALE;
	}

    void ajourne_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto graphe = jorjala.graphe();

        graphe.centre_x(graphe.centre_x() + m_orig_x - donnees.x);
        graphe.centre_y(graphe.centre_y() + m_orig_y - donnees.y);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);
	}

    JJL::TypeCurseur type_curseur_modal() override
    {
        return JJL::TypeCurseur::MAIN_FERMÉE;
    }
};

/* ************************************************************************** */

class CommandeZoomGraphe final : public CommandeJorjala {
public:
	CommandeZoomGraphe() = default;

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto graphe = jorjala.graphe();

        auto valeur = graphe.zoom() * ((donnees.y > 0) ? constantes<float>::PHI : constantes<float>::PHI_INV);
        graphe.zoom(valeur);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeEntreNoeud final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        auto graphe = jorjala.graphe();
        auto noeud = trouve_noeud(graphe, donnees.x, donnees.y);
        selectionne_noeud(jorjala, noeud, graphe);

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        auto sous_graphe = noeud.accède_sous_graphe();
        if (sous_graphe == nullptr) {
			return EXECUTION_COMMANDE_REUSSIE;
		}

        jorjala.définit_graphe_courant(sous_graphe);
        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSorsNoeud final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &/*donnees*/) override
    {
        auto graphe = jorjala.graphe();

        auto noeud_parent = graphe.noeud_parent();
        auto graphe_parent = noeud_parent.graphe_parent();

        if (graphe_parent == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        jorjala.définit_graphe_courant(graphe_parent);
        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeArrangeGraphe final : public CommandeJorjala {
public:
	CommandeArrangeGraphe() = default;

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		INUTILISE(donnees);

        auto graphe = jorjala.graphe;

		GVC_t *gvc = gvContext();

		if (gvc == nullptr) {
            jorjala.affiche_erreur("Ne peut pas créer le contexte GrapheViz");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto chn_graphe = chaine_dot_pour_graphe(*graphe);
		Agraph_t *g = agmemread(chn_graphe.c_str());

		if (gvc == nullptr) {
			gvFreeContext(gvc);
            jorjala.affiche_erreur("Ne peut pas créer le graphe GrapheViz");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		gvLayout(gvc, g, "dot");
		gvRender(gvc, g, "dot", nullptr);

		for (auto noeud : graphe->noeuds()) {
			auto id = id_dot_pour_noeud(noeud, false);
			auto gnoeud = agnode(g, const_cast<char *>(id.c_str()), false);

			if (gnoeud == nullptr) {
				std::cerr << "Impossible de trouver le noeud '" << id << "'\n";
				continue;
			}

			auto pos_noeud = dls::chaine(agget(gnoeud, const_cast<char *>("pos")));

			auto morceaux = dls::morcelle(pos_noeud, ',');

			auto pos_x = static_cast<float>(std::atoi(morceaux[0].c_str()));
			auto pos_y = static_cast<float>(std::atoi(morceaux[1].c_str()));

			noeud->pos_x(pos_x);
			noeud->pos_y(pos_y);
		}

		gvFreeLayout(gvc, g);
		agclose(g);
		gvFreeContext(gvc);

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

#endif
        return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeChangeContexte final : public CommandeJorjala {
public:
    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
		auto const &metadonnee = donnees.metadonnee;
        jorjala.définit_racine_courante(metadonnee.c_str());
        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);
        return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

struct CommandeAjoutPriseNoeud final : public CommandeJorjala {
    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
	{
#if 0
		INUTILISE(metadonnee);
        auto graphe = jorjala.graphe;
		auto noeud_actif = graphe->noeud_actif;

		if (noeud_actif == nullptr) {
			return false;
		}

		return noeud_actif->type == type_noeud::OPERATRICE && noeud_actif->prises_dynamiques;
#else
        return false;
#endif
	}

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
	{
#if 0
		INUTILISE(donnees);
        auto graphe = jorjala.graphe;

		auto noeud_actif = graphe->noeud_actif;

		if (noeud_actif == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (noeud_actif->type != type_noeud::OPERATRICE || !noeud_actif->prises_dynamiques) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        auto gestionnaire = jorjala.gestionnaire_entreface;
		auto resultat = danjo::Manipulable{};
		auto donnees_entreface = danjo::DonneesInterface{};
		donnees_entreface.conteneur = nullptr;
        donnees_entreface.repondant_bouton = jorjala.repondant_commande();
		donnees_entreface.manipulable = &resultat;

		auto ok = gestionnaire->montre_dialogue_fichier(
					donnees_entreface,
					"entreface/ajout_prise_noeud.jo");

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto nom_prise = resultat.evalue_chaine("nom_prise");
		auto chn_type_prise = resultat.evalue_enum("type_prise");
		auto chn_type_donnees = resultat.evalue_enum("type_donnée");

		auto dico_type_donnees = dls::cree_dico(
					dls::paire(dls::vue_chaine("chaine"), type_prise::CHAINE),
					dls::paire(dls::vue_chaine("corps"), type_prise::CORPS),
					dls::paire(dls::vue_chaine("couleur"), type_prise::COULEUR),
					dls::paire(dls::vue_chaine("décimal"), type_prise::DECIMAL),
					dls::paire(dls::vue_chaine("entier"), type_prise::ENTIER),
					dls::paire(dls::vue_chaine("image"), type_prise::IMAGE),
					dls::paire(dls::vue_chaine("mat3"), type_prise::MAT3),
					dls::paire(dls::vue_chaine("mat4"), type_prise::MAT4),
					dls::paire(dls::vue_chaine("objet"), type_prise::OBJET),
					dls::paire(dls::vue_chaine("tableau"), type_prise::TABLEAU),
					dls::paire(dls::vue_chaine("vec2"), type_prise::VEC2),
					dls::paire(dls::vue_chaine("vec3"), type_prise::VEC3),
					dls::paire(dls::vue_chaine("vec4"), type_prise::VEC4));

		auto plg_type = dico_type_donnees.trouve(chn_type_prise);

		if (plg_type.est_finie()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto op = extrait_opimage(noeud_actif->donnees);

		auto type = plg_type.front().second;

		if (chn_type_prise == "entrée") {
			noeud_actif->ajoute_entree(nom_prise, type, false);

			auto index = op->entrees();
			op->entrees(op->entrees() + 1);
			op->donnees_entree(index, noeud_actif->entrees.back());
		}
		else if (chn_type_prise == "sortie") {
			noeud_actif->ajoute_sortie(nom_prise, type);

			auto index = op->sorties();
			op->sorties(op->sorties() + 1);
			op->donnees_sortie(index, noeud_actif->sorties.back());
		}

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

#endif
		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_graphes(UsineCommande &usine)
{
	usine.enregistre_type("dessine_graphe_composite",
						   description_commande<CommandeDessineGrapheComposite>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajouter_noeud",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajouter_noeud_vision",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, Qt::Key_V, false, "Visionneur"));

	usine.enregistre_type("ajouter_noeud_image",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, Qt::Key_I, false, "Lecture Image"));

	usine.enregistre_type("ajouter_noeud_detail",
						   description_commande<CommandeAjoutNoeudDetail>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajouter_noeud_cycles",
						   description_commande<CommandeAjoutNoeudCycles>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajouter_noeud_spécial_détail",
						   description_commande<CommandeAjoutNoeudDetailSpecial>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajouter_graphe_detail",
						   description_commande<CommandeAjoutGrapheDetail>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("selection_graphe",
						   description_commande<CommandeSelectionGraphe>(
							   "graphe", Qt::LeftButton, 0, 0, false));

	usine.enregistre_type("supprime_selection",
						   description_commande<CommandeSupprimeSelection>(
							   "graphe", 0, 0, Qt::Key_Delete, false));

	usine.enregistre_type("information_noeud",
						   description_commande<CommandeInfoNoeud>(
							   "graphe", Qt::MiddleButton, 0, 0, false));

	usine.enregistre_type("deplace_graphe",
						   description_commande<CommandeDeplaceGraphe>(
							   "graphe", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

	usine.enregistre_type("zoom_graphe",
						   description_commande<CommandeZoomGraphe>(
							   "graphe", Qt::MiddleButton, 0, 0, true));

	usine.enregistre_type("entre_noeud",
						   description_commande<CommandeEntreNoeud>(
							   "graphe", Qt::LeftButton, 0, 0, true));

	usine.enregistre_type("sors_noeud",
						   description_commande<CommandeSorsNoeud>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("arrange_graphe",
						   description_commande<CommandeArrangeGraphe>(
							   "graphe", 0, 0, Qt::Key_L, false));

	usine.enregistre_type("change_contexte",
						   description_commande<CommandeChangeContexte>(
							   "graphe", 0, 0, 0, false));

	usine.enregistre_type("ajout_prise_noeud",
						   description_commande<CommandeAjoutPriseNoeud>(
							   "graphe", 0, 0, 0, false));
}

#pragma clang diagnostic pop
