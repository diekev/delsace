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
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "evaluation/evaluation.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/imprimeuse_graphe.h"
#include "coeur/manipulatrice.h"
#include "coeur/mikisa.h"
#include "coeur/noeud_image.h"
#include "coeur/objet.h"
#include "coeur/operatrice_graphe_detail.hh"
#include "coeur/operatrice_image.h"
#include "coeur/operatrice_simulation.hh"
#include "coeur/usine_operatrice.h"

#include "lcc/lcc.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/**
 * Ajourne le noeud_actif du graphe pour être égal au noeud spécifié et
 * retourne vrai si le graphe doit être évalué, c'est-à-dire si une visionneuse
 * différente de la dernière visionneuse a été sélectionnée. La fonction ajourne
 * également le pointeur derniere_visionneuse_selectionnee du graphe si le noeud
 * est une visionneuse.
 */
static bool selectionne_noeud(Mikisa &mikisa, Noeud *noeud, Graphe &graphe)
{
	graphe.noeud_actif = noeud;

	if (graphe.noeud_actif == nullptr) {
		return false;
	}

	if (noeud->type == type_noeud::OBJET || noeud->type == type_noeud::COMPOSITE) {
		/* À FAIRE : considère avoir et mettre en place un objet actif. */
		return false;
	}

	if (noeud->est_sortie) {
		auto const besoin_ajournement = (graphe.noeud_actif != graphe.dernier_noeud_sortie);

		graphe.dernier_noeud_sortie = noeud;

		return besoin_ajournement;
	}

	auto operatrice = extrait_opimage(noeud->donnees);

	if (operatrice->possede_manipulatrice_3d(mikisa.type_manipulation_3d)) {
		mikisa.manipulatrice_3d = operatrice->manipulatrice_3d(mikisa.type_manipulation_3d);
	}
	else {
		mikisa.manipulatrice_3d = nullptr;
	}

	return false;
}

/**
 * Retourne vrai si le noeud possède une connexion vers la sortie.
 */
static bool noeud_connecte_sortie(Noeud *noeud, Noeud *sortie)
{
	if (noeud == nullptr) {
		return false;
	}

	if (noeud->est_sortie) {
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

/**
 * Retourne vrai si l'entrée et la sortie sont compatibles pour être connectées.
 */
static bool peut_connecter(PriseEntree *entree, PriseSortie *sortie)
{
	if (entree == nullptr || sortie == nullptr) {
		return false;
	}

	if (entree->parent == sortie->parent) {
		return false;
	}

	if (entree->type != sortie->type) {
		if (entree->type == type_prise::POLYMORPHIQUE || sortie->type == type_prise::POLYMORPHIQUE) {
			return true;
		}

		return false;
	}

	return true;
}

/* ************************************************************************** */

class CommandeDessineGrapheComposite final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto const &noeud_composite = mikisa->bdd.graphe_composites()->noeud_actif;

		if (noeud_composite == nullptr) {
			mikisa->affiche_erreur("Aucun noeud composite sélectionné");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &composite = extrait_composite(noeud_composite->donnees);

		ImprimeuseGraphe gd(&composite->noeud->graphe);
		gd("/tmp/graphe_composite.gv");

		if (system("dot /tmp/graphe_composite.gv -Tpng -o /tmp/graphe_composite.png") == -1) {
			std::cerr << "Impossible de créer une image depuis 'dot'\n";
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static bool finalise_ajout_noeud(
		Mikisa &mikisa,
		Graphe &graphe,
		Noeud &noeud)
{
	noeud.pos_x(graphe.centre_x);
	noeud.pos_y(graphe.centre_y);

	if (graphe.connexion_active != nullptr) {
		if (graphe.connexion_active->prise_entree != nullptr) {
			graphe.connecte(noeud.sortie(0), graphe.connexion_active->prise_entree);
		}
		else if (graphe.connexion_active->prise_sortie != nullptr) {
			graphe.connecte(graphe.connexion_active->prise_sortie, noeud.entree(0));
		}

		memoire::deloge("Connexion", graphe.connexion_active);
	}

	auto besoin_evaluation = selectionne_noeud(mikisa, &noeud, graphe);

	mikisa.notifie_observatrices(type_evenement::noeud | type_evenement::ajoute);

	return besoin_evaluation;
}

/* ************************************************************************** */

class CommandeAjoutNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		auto nom = donnees.metadonnee;
		auto graphe = mikisa->graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);
		noeud->graphe.type = type_graphe::OBJET;

		(mikisa->usine_operatrices())(nom, *mikisa->graphe, *noeud);
		synchronise_donnees_operatrice(*noeud);

		auto besoin_evaluation = finalise_ajout_noeud(*mikisa, *graphe, *noeud);

		if (besoin_evaluation) {
			requiers_evaluation(*mikisa, NOEUD_AJOUTE, "noeud ajouté");
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeudDetail final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		auto nom = donnees.metadonnee;
		auto graphe = mikisa->graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);
		noeud->graphe.type = type_graphe::DETAIL;

		auto op = cree_op_detail(*mikisa, *graphe, *noeud, nom);
		op->cree_proprietes();
		synchronise_donnees_operatrice(*noeud);

		finalise_ajout_noeud(*mikisa, *graphe, *noeud);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeudDetailSpecial final : public Commande {
public:
	bool evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		if (graphe->type != type_graphe::DETAIL) {
			return false;
		}

		if (graphe->donnees.est_vide()) {
			return false;
		}

		auto type_detail = std::any_cast<int>(graphe->donnees[0]);

		using dls::outils::est_element;

		if (est_element(metadonnee, "Entrée Détail", "Sortie Détail")) {
			return est_element(type_detail,
							   DETAIL_PIXELS,
							   DETAIL_VOXELS,
							   DETAIL_POINTS,
							   DETAIL_TERRAIN);
		}

		if (est_element(metadonnee, "Entrée Attribut", "Sortie Attribut")) {
			return est_element(type_detail, DETAIL_POINTS);
		}

		return false;
	}

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		auto nom = donnees.metadonnee;
		auto graphe = mikisa->graphe;

		if (graphe->type != type_graphe::DETAIL) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* À FAIRE : les prédicats ne sont appelés qu'à travers un répondant
		 * bouton... */
		if (!evalue_predicat(pointeur, nom)) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);

		(mikisa->usine_operatrices())(nom, *graphe, *noeud);
		synchronise_donnees_operatrice(*noeud);

		auto besoin_evaluation = finalise_ajout_noeud(*mikisa, *graphe, *noeud);

		if (besoin_evaluation) {
			requiers_evaluation(*mikisa, NOEUD_AJOUTE, "noeud ajouté");
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutGrapheDetail final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);

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
			mikisa->affiche_erreur("Type de graphe détail inconnu");
			return EXECUTION_ECHOUEE;
		}

		auto graphe = mikisa->graphe;
		auto noeud = graphe->cree_noeud(nom, type_noeud::OPERATRICE);

		auto op = (mikisa->usine_operatrices())("Graphe Détail", *graphe, *noeud);

		auto op_graphe = dynamic_cast<OperatriceGrapheDetail *>(op);
		op_graphe->type_detail = type_detail;
		/* il faut que le type de détail soit correct car il est possible que
		 * l'opératrice ne fut pas exécutée avant que des noeuds soient ajoutés
		 * dans son graphe */
		noeud->graphe.donnees.efface();
		noeud->graphe.donnees.pousse(type_detail);
		noeud->graphe.type = type_graphe::DETAIL;

		/* la synchronisation doit se faire après puisque nous avons besoin du
		 * type de détail pour déterminer les types de sorties */
		synchronise_donnees_operatrice(*noeud);

		auto besoin_evaluation = finalise_ajout_noeud(*mikisa, *graphe, *noeud);

		if (besoin_evaluation) {
			requiers_evaluation(*mikisa, NOEUD_AJOUTE, "noeud ajouté");
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSelectionGraphe final : public Commande {
	float delta_x = 0.0f;
	float delta_y = 0.0f;
	bool m_prise_entree_deconnectee = false;
	char m_pad[6];

public:
	CommandeSelectionGraphe()
		: Commande()
	{
		INUTILISE(m_pad); /* Pour faire taire les avertissements. */
	}

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		Noeud *noeud_selection = nullptr;
		PriseEntree *prise_entree = nullptr;
		PriseSortie *prise_sortie = nullptr;

		trouve_noeud_prise(graphe->noeuds(), donnees.x, donnees.y, noeud_selection, prise_entree, prise_sortie);

		if (noeud_selection != nullptr) {
			delta_x = donnees.x - noeud_selection->pos_x();
			delta_y = donnees.y - noeud_selection->pos_y();
		}

		if (prise_entree || prise_sortie) {
			auto connexion = memoire::loge<Connexion>("Connexion");
			connexion->x = donnees.x;
			connexion->y = donnees.y;

			if (prise_entree && !prise_entree->liens.est_vide()) {
				connexion->prise_entree = nullptr;
				/* déconnecte le dernier lien, À FAIRE : meilleure interface */
				connexion->prise_sortie = prise_entree->liens.back();

				if (graphe->deconnecte(prise_entree->liens.back(), prise_entree)) {
					m_prise_entree_deconnectee = noeud_connecte_sortie(
													 prise_entree->parent,
													 graphe->dernier_noeud_sortie);
				}
			}
			else {
				connexion->prise_entree = prise_entree;
				connexion->prise_sortie = prise_sortie;
			}

			graphe->connexion_active = connexion;
		}

		bool besoin_evaluation = selectionne_noeud(*mikisa, noeud_selection, *graphe);

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::selectionne);

		/* évalue le graphe si un visionneur a été sélectionné */
		if (besoin_evaluation) {
			requiers_evaluation(*mikisa, NOEUD_SELECTIONE, "noeud sélectionné");
		}

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		if (graphe->connexion_active != nullptr) {
			graphe->connexion_active->x = donnees.x;
			graphe->connexion_active->y = donnees.y;
		}
		else {
			Noeud *noeud_actif = graphe->noeud_actif;

			if (noeud_actif == nullptr) {
				return;
			}

			noeud_actif->pos_x(donnees.x - delta_x);
			noeud_actif->pos_y(donnees.y - delta_y);
		}

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);
	}

	void termine_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		bool connexion_sortie = false;

		if (graphe->connexion_active) {
			PriseEntree *entree = nullptr;
			PriseSortie *sortie = nullptr;

			if (graphe->connexion_active->prise_entree != nullptr) {
				entree = graphe->connexion_active->prise_entree;
				sortie = trouve_prise_sortie(graphe->noeuds(), donnees.x, donnees.y);
			}
			else {
				entree = trouve_prise_entree(graphe->noeuds(), donnees.x, donnees.y);
				sortie = graphe->connexion_active->prise_sortie;
			}

			if (peut_connecter(entree, sortie)) {
				if (!entree->liens.est_vide() && !entree->multiple_connexions) {
					graphe->deconnecte(entree->liens[0], entree);
				}

				graphe->connecte(sortie, entree);

				connexion_sortie = noeud_connecte_sortie(
								entree->parent,
								graphe->dernier_noeud_sortie);
			}

			memoire::deloge("Connexion", graphe->connexion_active);
		}

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

		if (connexion_sortie || m_prise_entree_deconnectee) {
			marque_parent_surannee(&graphe->noeud_parent, [](Noeud *n, PriseEntree *prise)
			{
				if (n->type != type_noeud::OPERATRICE) {
					return;
				}

				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			requiers_evaluation(*mikisa, GRAPHE_MODIFIE, "graphe modifié");
		}
	}
};

/* ************************************************************************** */

class CommandeSupprimeSelection final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;
		auto noeud = graphe->noeud_actif;

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (noeud == graphe->dernier_noeud_sortie) {
			graphe->dernier_noeud_sortie = nullptr;
		}

		auto besoin_execution = false;

		if (noeud->type == type_noeud::OBJET) {
			besoin_execution = true;

			auto objet = extrait_objet(noeud->donnees);
			mikisa->bdd.enleve_objet(objet);
		}
		else if (noeud->type == type_noeud::COMPOSITE) {
			besoin_execution = true;

			auto compo = extrait_composite(noeud->donnees);
			mikisa->bdd.enleve_composite(compo);
		}
		else {
			auto operatrice = extrait_opimage(noeud->donnees);

			if (operatrice->manipulatrice_3d(mikisa->type_manipulation_3d) == mikisa->manipulatrice_3d) {
				mikisa->manipulatrice_3d = nullptr;
			}

			besoin_execution = noeud_connecte_sortie(noeud, graphe->dernier_noeud_sortie);

			graphe->supprime(noeud);
		}

		graphe->noeud_actif = nullptr;

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::enleve);

		if (besoin_execution) {
			marque_parent_surannee(&graphe->noeud_parent, [](Noeud *n, PriseEntree *prise)
			{
				if (n->type != type_noeud::OPERATRICE) {
					return;
				}

				auto op = extrait_opimage(n->donnees);
				op->amont_change(prise);
			});

			requiers_evaluation(*mikisa, NOEUD_ENLEVE, "noeud supprimé");
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

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

class CommandeInfoNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;
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

			if (op->type() == OPERATRICE_CORPS || op->type() == OPERATRICE_SORTIE_CORPS) {
				auto op_objet = dynamic_cast<OperatriceCorps *>(op);
				auto corps = op_objet->corps();

				ss << "<p>Points         : " << corps->points_pour_lecture()->taille() << "</p>";
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

		selectionne_noeud(*mikisa, noeud, *graphe);

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::selectionne);

		return EXECUTION_COMMANDE_MODALE;
	}

	void termine_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(donnees);

		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		memoire::deloge("InfoNoeud", graphe->info_noeud);

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeDeplaceGraphe final : public Commande {
	float m_orig_x = 0.0f;
	float m_orig_y = 0.0f;

public:
	CommandeDeplaceGraphe() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		INUTILISE(pointeur);

		m_orig_x = donnees.x;
		m_orig_y = donnees.y;

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		graphe->centre_x += m_orig_x - donnees.x;
		graphe->centre_y += m_orig_y - donnees.y;

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeZoomGraphe final : public Commande {
public:
	CommandeZoomGraphe() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		graphe->zoom *= (donnees.y > 0) ? constantes<float>::PHI : constantes<float>::PHI_INV;

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeEntreNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;
		auto noeud = trouve_noeud(graphe->noeuds(), donnees.x, donnees.y);
		selectionne_noeud(*mikisa, noeud, *graphe);

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (!noeud->peut_avoir_graphe) {
			return EXECUTION_COMMANDE_REUSSIE;
		}

		mikisa->noeud = noeud;
		mikisa->graphe = &noeud->graphe;
		mikisa->chemin_courant = noeud->chemin();

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSorsNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		auto noeud_parent = mikisa->noeud->parent;

		if (!noeud_parent) {
			return EXECUTION_ECHOUEE;
		}

		mikisa->noeud = noeud_parent;
		mikisa->graphe = &noeud_parent->graphe;
		mikisa->chemin_courant = noeud_parent->chemin();

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

#if 0
#if 0
 NA       // 4 // 0  // 0
 NB  NC // 3 // 1  // 1 // 0
   ND    // 2 // 2  // 2
 NE  NF // 1 // 3  // 3 // 3
     NG // O // 4       // 4

min(niveau, max(niveau_enfants) + 1)
NA | 4 | 4 | 4
NB | 3 | 3 | 3
NC | 3 | 3 | 3
ND | 2 | 2 | 2
NE | 1 | 0 | 1
NF | 1 | 1 | 1
NG | 0 | 0 | 0

   | NA | NB | NC | ND | NE | NF | NG
NA |  0 |  1 |  0 |  0 |  0 |  0 |  0
NB |  1 |  0 |  0 |  1 |  0 |  0 |  0
NC |  0 |  0 |  0 |  1 |  0 |  0 |  0
ND |  0 |  1 |  1 |  0 |  1 |  1 |  0
NE |  0 |  0 |  0 |  1 |  0 |  0 |  0
NF |  0 |  0 |  0 |  1 |  0 |  0 |  1
NG |  0 |  0 |  0 |  0 |  0 |  1 |  0
#endif

static void assigne_niveau_parent(Noeud *noeud, int niveau)
{
	if (noeud->niveau >= niveau) {
		return;
	}

	noeud->niveau = niveau;

	for (auto &prise : noeuds->prise_entrees()) {
		if (prise->lien == nullptr) {
			continue;
		}

		assigne_niveau_parent(prise->lien->parent, niveau + 1);
	}
}

static void corrige_niveau_selon_enfant(Noeud *noeud)
{
	auto max_niveau_enfant = -1;

	for (auto &prise : noeuds->sorties()) {
		for (auto &liens : prise->liens()) {
			if (lien->parent->niveau > max_niveau_enfant) {
				max_niveau_enfant = lien->parent->niveau;
			}
		}
	}

	noeud->niveau = std::min(noeud->niveau, max_niveau_enfant + 1);

	for (auto &prise : noeuds->sorties()) {
		for (auto &liens : prise->liens()) {
			if (lien->parent->niveau > max_niveau_enfant) {
				corrige_niveau_selon_enfant(lien->parent);
			}
		}
	}
}

static bool est_racine(Noeud *noeud)
{
	return noeud->entrees.taille() == 0;
}

static bool est_feuille(Noeud *noeud)
{
	return noeud->sorties.taille() == 0;
}

static constexpr DISTANCE_ENTRE_NOEUDS_X = 200
static constexpr DISTANCE_ENTRE_NOEUDS_Y = 100

// À FAIRE : noeuds orphelin
static void autodispose_graphe(Graphe &graphe)
{
	auto pos_x_min = std::numeric_limits<int>::max();
	auto pos_y_min = std::numeric_limits<int>::max();

	// assigne le niveau -1 à tous les noeuds, 0 pour les noeuds sans sorties
	for (auto &noeud : graphe.noeuds()) {
		if (est_feuille(noeud)) {
			noeud->niveau = 0;
		}
		else {
			noeud->niveau = -1;
		}

		pos_x_min = std::min(pos_x_min, noeud->pos_x);
		pos_x_min = std::min(pos_y_min, noeud->pos_y);
	}

	// remonte le graphe à travers les noeuds feuille et assigne niveau = max(niveau, niveau_enfant + 1)
	for (auto &noeud : graphe.noeuds()) {
		if (noeud->niveau != 0) {
			continue;
		}

		for (auto &prise : noeuds->entrees()) {
			if (prise->lien == nullptr) {
				continue;
			}

			assigne_niveau_parent(prise->lien->parent, 1);
		}
	}

	// descend le graphe à travers les noeuds racines et assigne
	// niveau = min(niveau, max(niveau_enfants) + 1)
	for (auto &noeud : graphe.noeuds()) {
		if (!est_racine(noeud)) {
			continue;
		}

		corrige_niveau_selon_enfant(noeud);
	}

	// compte le nombre de noeuds dans chaque niveau pour leur assigner un déplacement horizontal
	dls::dico<int, int> noeuds_par_niveau;
	for (auto &noeud : graphe.noeuds()) {
		noeuds_par_niveau[noeud->niveau] += 1;

		noeud->pos_y = pos_y_min + DISTANCE_ENTRE_NOEUDS_Y * noeud->niveau;
	}


	// calcul les limites du graphe
	// zoom sur le graphe
}
#endif

class CommandeArrangeGraphe final : public Commande {
public:
	CommandeArrangeGraphe() = default;

	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		//auto mikisa = extrait_mikisa(pointeur);
		//auto graphe = mikisa->graphe;

		//autodispose_graphe(*graphe);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeChangeContexte final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		auto const &metadonnee = donnees.metadonnee;

		if (metadonnee == "composites") {
			mikisa->graphe = mikisa->bdd.graphe_composites();
			mikisa->chemin_courant = "/composites/";
			mikisa->noeud = nullptr;
		}
		else if (metadonnee == "objets") {
			mikisa->graphe = mikisa->bdd.graphe_objets();
			mikisa->chemin_courant = "/objets/";
			mikisa->noeud = nullptr;
		}

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

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
}

#pragma clang diagnostic pop
