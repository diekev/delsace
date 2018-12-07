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

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/outils/constantes.h"

#include "../composite.h"
#include "../evaluation.h"
#include "../evenement.h"
#include "../imprimeuse_graphe.h"
#include "../manipulatrice.h"
#include "../mikisa.h"
#include "../noeud_image.h"
#include "../operatrice_graphe_maillage.h"
#include "../operatrice_graphe_pixel.h"
#include "../operatrice_image.h"
#include "../operatrice_objet.h"
#include "../operatrice_scene.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/**
 * Ajourne le noeud_actif du graphe pour être égal au noeud spécifié et
 * retourne vrai si le graphe doit être évalué, c'est-à-dire si une visionneuse
 * différente de la dernière visionneuse a été sélectionnée. La fonction ajourne
 * également les pointeurs derniere_visionneuse_selectionnee et
 * derniere_scene_selectionnee de mikisa si le noeud est une visionneuse ou
 * une opératrice scène.
 */
static bool selectionne_noeud(Mikisa *mikisa, Noeud *noeud, Graphe &graphe)
{
	graphe.noeud_actif = noeud;

	if (graphe.noeud_actif == nullptr) {
		return false;
	}

	if (noeud->type() == NOEUD_IMAGE_SORTIE) {
		const auto besoin_ajournement = (graphe.noeud_actif != mikisa->derniere_visionneuse_selectionnee);

		mikisa->derniere_visionneuse_selectionnee = graphe.noeud_actif;
		graphe.dernier_noeud_sortie = graphe.noeud_actif;

		return besoin_ajournement;
	}

	if (noeud->type() == NOEUD_OBJET_SORTIE) {
		const auto besoin_ajournement = (graphe.noeud_actif != graphe.dernier_noeud_sortie);

		graphe.dernier_noeud_sortie = graphe.noeud_actif;

		return besoin_ajournement;
	}

	auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

	if (operatrice->type() == OPERATRICE_SCENE) {
		mikisa->derniere_scene_selectionnee = graphe.noeud_actif;
	}

	if (operatrice->possede_manipulatrice_3d(mikisa->type_manipulation_3d)) {
		mikisa->manipulatrice_3d = operatrice->manipulatrice_3d(mikisa->type_manipulation_3d);
	}
	else {
		mikisa->manipulatrice_3d = nullptr;
	}

	return false;
}

/**
 * Retourne vrai si le noeud possède une connexion vers la sortie.
 */
static bool noeud_connecte_sortie(Noeud *noeud, Noeud *sortie)
{
	if (noeud == nullptr || sortie == nullptr) {
		return false;
	}

	if ((noeud->type() == NOEUD_IMAGE_SORTIE || noeud->type() == NOEUD_OBJET_SORTIE) && noeud == sortie) {
		return true;
	}

	for (auto prise_sortie : noeud->sorties()) {
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
		return false;
	}

	return true;
}

/* ************************************************************************** */

class CommandeDessineGrapheComposite final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto composite = mikisa->composite;

		ImprimeuseGraphe gd(&composite->graph());
		gd("/tmp/graphe_composite.gv");

		if (system("dot /tmp/graphe_composite.gv -Tpng -o /tmp/graphe_composite.png") == -1) {
			std::cerr << "Impossible de créer une image depuis 'dot'\n";
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjoutNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto noeud = new Noeud(supprime_operatrice_image);
		auto nom = donnees.metadonnee;

		auto op = (*mikisa->usine_operatrices())(nom, noeud);
		noeud->nom(op->class_name());
		synchronise_donnees_operatrice(noeud);

		if (op->type() == OPERATRICE_SORTIE_IMAGE) {
			noeud->type(NOEUD_IMAGE_SORTIE);
		}
		else if (op->type() == OPERATRICE_SORTIE_CORPS) {
			noeud->type(NOEUD_OBJET_SORTIE);
		}

		mikisa->graphe->ajoute(noeud);
		selectionne_noeud(mikisa, noeud, *mikisa->graphe);

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::ajoute);

		if (mikisa->contexte == GRAPHE_SCENE) {
			evalue_resultat(mikisa);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSelectionGraphe final : public Commande {
	float delta_x = 0.0f;
	float delta_y = 0.0f;
	bool m_prise_entree_deconnectee = false;

public:
	CommandeSelectionGraphe() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		Noeud *noeud_selection = nullptr;
		PriseEntree *prise_entree = nullptr;
		PriseSortie *prise_sortie = nullptr;

		trouve_noeud_prise(graphe->noeuds(), donnees.x, donnees.y, noeud_selection, prise_entree, prise_sortie);

		if (noeud_selection != nullptr) {
			delta_x = donnees.x - static_cast<float>(noeud_selection->pos_x());
			delta_y = donnees.y - static_cast<float>(noeud_selection->pos_y());
		}

		if (prise_entree || prise_sortie) {
			auto connexion = new Connexion;
			connexion->x = donnees.x;
			connexion->y = donnees.y;

			if (prise_entree && prise_entree->lien) {
				connexion->prise_entree = nullptr;
				connexion->prise_sortie = prise_entree->lien;

				if (graphe->deconnecte(prise_entree->lien, prise_entree)) {
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

		bool besoin_evaluation = selectionne_noeud(mikisa, noeud_selection, *graphe);

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::selectionne);

		/* évalue le graphe si un visionneur a été sélectionné */
		if (besoin_evaluation) {
			evalue_resultat(mikisa);
		}

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
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

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);
	}

	void termine_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		bool connexion = false;

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
				if (entree->lien != nullptr) {
					graphe->deconnecte(entree->lien, entree);
				}

				graphe->connecte(sortie, entree);

				connexion = noeud_connecte_sortie(
								entree->parent,
								graphe->dernier_noeud_sortie);
			}

			delete graphe->connexion_active;
			graphe->connexion_active = nullptr;
		}

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);

		if (connexion || m_prise_entree_deconnectee || mikisa->contexte == GRAPHE_MAILLAGE) {
			evalue_resultat(mikisa);
		}
	}
};

/* ************************************************************************** */

class CommandeSupprimeSelection final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;
		auto noeud = graphe->noeud_actif;

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (noeud == mikisa->derniere_scene_selectionnee) {
			mikisa->derniere_scene_selectionnee = nullptr;
		}
		else if (noeud == mikisa->derniere_visionneuse_selectionnee) {
			mikisa->derniere_visionneuse_selectionnee = nullptr;
		}
		if (noeud == graphe->dernier_noeud_sortie) {
			graphe->dernier_noeud_sortie = nullptr;
		}

		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

		if (operatrice->manipulatrice_3d(mikisa->type_manipulation_3d) == mikisa->manipulatrice_3d) {
			mikisa->manipulatrice_3d = nullptr;
		}

		const auto besoin_execution = noeud_connecte_sortie(noeud, graphe->dernier_noeud_sortie);

		graphe->supprime(noeud);

		graphe->noeud_actif = nullptr;

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::enleve);

		if (besoin_execution || mikisa->contexte == GRAPHE_MAILLAGE) {
			evalue_resultat(mikisa);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeInfoNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		if (mikisa->contexte == GRAPHE_PIXEL || mikisa->contexte == GRAPHE_MAILLAGE) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto graphe = mikisa->graphe;

		auto noeud = trouve_noeud(graphe->noeuds(), donnees.x, donnees.y);

		if (noeud != nullptr) {
			auto info_noeud = new InfoNoeud();
			info_noeud->x = donnees.x;
			info_noeud->y = donnees.y;

			std::stringstream ss;
			ss << "<p>Opératrice : " << noeud->nom() << "</p>";
			ss << "<hr/>";
			ss << "<p>Temps d'exécution :";
			ss << "<p>- dernière : " << noeud->temps_execution() << " secondes.</p>";
			ss << "<p>- minimum : " << noeud->temps_execution_minimum() << " secondes.</p>";
			ss << "<hr/>";
			ss << "<p>Nombre d'exécution : " << noeud->compte_execution() << "</p>";
			ss << "<hr/>";

			info_noeud->informations = ss.str();

			graphe->info_noeud = info_noeud;
		}

		selectionne_noeud(mikisa, noeud, *graphe);

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::selectionne);

		return EXECUTION_COMMANDE_MODALE;
	}

	void termine_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		delete graphe->info_noeud;
		graphe->info_noeud = nullptr;

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeDeplaceGraphe final : public Commande {
	float m_delta_x = 0.0f;
	float m_delta_y = 0.0f;

public:
	CommandeDeplaceGraphe() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		m_delta_x = donnees.x - graphe->centre_x;
		m_delta_y = donnees.y - graphe->centre_y;

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		graphe->centre_x += m_delta_x - donnees.x;
		graphe->centre_y += m_delta_y - donnees.y;

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeZoomGraphe final : public Commande {
public:
	CommandeZoomGraphe() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;

		graphe->zoom *= (donnees.y > 0) ? static_cast<float>(PHI) : static_cast<float>(PHI_INV);

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeEntreNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto graphe = mikisa->graphe;
		auto noeud = trouve_noeud(graphe->noeuds(), donnees.x, donnees.y);
		selectionne_noeud(mikisa, noeud, *graphe);

		if (noeud == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());

		if (operatrice->type() == OPERATRICE_GRAPHE_PIXEL) {
			auto operatrice_graphe = dynamic_cast<OperatriceGraphePixel *>(operatrice);

			mikisa->graphe = operatrice_graphe->graphe();
			mikisa->contexte = GRAPHE_PIXEL;

			mikisa->chemin_courant = "/composite/" + noeud->nom() + "/";
		}
		else if (operatrice->type() == OPERATRICE_SCENE) {
			auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);

			mikisa->graphe = operatrice_scene->graphe();
			mikisa->contexte = GRAPHE_SCENE;

			mikisa->chemin_courant = "/composite/" + noeud->nom() + "/";
		}
		else if (operatrice->type() == OPERATRICE_OBJET) {
			assert(mikisa->contexte == GRAPHE_SCENE);

			auto operatrice_objet = dynamic_cast<OperatriceObjet *>(operatrice);

			mikisa->graphe = operatrice_objet->graphe();
			mikisa->contexte = GRAPHE_OBJET;

			mikisa->chemin_courant += noeud->nom() + "/";
		}
		else if (operatrice->type() == OPERATRICE_GRAPHE_MAILLAGE) {
			assert(mikisa->contexte == GRAPHE_OBJET);

			auto operatrice_graphe = dynamic_cast<OperatriceGrapheMaillage *>(operatrice);

			mikisa->graphe = operatrice_graphe->graphe();
			mikisa->contexte = GRAPHE_MAILLAGE;

			mikisa->chemin_courant += noeud->nom() + "/";
		}

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSorsNoeud final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		if (mikisa->contexte == GRAPHE_COMPOSITE) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (mikisa->contexte == GRAPHE_OBJET) {
			auto noeud_scene = mikisa->derniere_scene_selectionnee;
			auto operatrice = std::any_cast<OperatriceImage *>(noeud_scene->donnees());
			auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);

			mikisa->graphe = operatrice_scene->graphe();
			mikisa->contexte = GRAPHE_SCENE;
			mikisa->chemin_courant = "/composite/" + noeud_scene->nom() + "/";
		}
		else if (mikisa->contexte == GRAPHE_MAILLAGE) {
			auto noeud_scene = mikisa->derniere_scene_selectionnee;
			auto operatrice = std::any_cast<OperatriceImage *>(noeud_scene->donnees());
			auto operatrice_scene = dynamic_cast<OperatriceScene *>(operatrice);
			auto noeud_objet = operatrice_scene->graphe()->noeud_actif;
			operatrice = std::any_cast<OperatriceImage *>(noeud_objet->donnees());
			auto operatrice_objet = dynamic_cast<OperatriceObjet *>(operatrice);

			mikisa->graphe = operatrice_objet->graphe();
			mikisa->contexte = GRAPHE_OBJET;
			mikisa->chemin_courant = "/composite/" + noeud_scene->nom() + "/" + noeud_objet->nom() + "/";
		}
		else {
			auto composite = mikisa->composite;

			mikisa->graphe = &composite->graph();
			mikisa->contexte = GRAPHE_COMPOSITE;
			mikisa->chemin_courant = "/composite/";
		}

		mikisa->notifie_auditeurs(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_graphes(UsineCommande *usine)
{
	usine->enregistre_type("dessine_graphe_composite",
						   description_commande<CommandeDessineGrapheComposite>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("ajouter_noeud",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("ajouter_noeud_vision",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, Qt::Key_V, false, "Visionneur"));

	usine->enregistre_type("ajouter_noeud_image",
						   description_commande<CommandeAjoutNoeud>(
							   "graphe", 0, 0, Qt::Key_I, false, "Lecture Image"));

	usine->enregistre_type("selection_graphe",
						   description_commande<CommandeSelectionGraphe>(
							   "graphe", Qt::LeftButton, 0, 0, false));

	usine->enregistre_type("supprime_selection",
						   description_commande<CommandeSupprimeSelection>(
							   "graphe", 0, 0, Qt::Key_Delete, false));

	usine->enregistre_type("information_noeud",
						   description_commande<CommandeInfoNoeud>(
							   "graphe", Qt::MiddleButton, 0, 0, false));

	usine->enregistre_type("deplace_graphe",
						   description_commande<CommandeDeplaceGraphe>(
							   "graphe", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

	usine->enregistre_type("zoom_graphe",
						   description_commande<CommandeZoomGraphe>(
							   "graphe", Qt::MiddleButton, 0, 0, true));

	usine->enregistre_type("entre_noeud",
						   description_commande<CommandeEntreNoeud>(
							   "graphe", Qt::LeftButton, 0, 0, true));

	usine->enregistre_type("sors_noeud",
						   description_commande<CommandeSorsNoeud>(
							   "graphe", 0, 0, 0, false));
}

#pragma clang diagnostic pop
