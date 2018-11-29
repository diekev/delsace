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

#include "commandes_graphes.h"

#include <iostream>

#include <QKeyEvent>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/geometrie/rectangle.h"

#include "graphs/graph_dumper.h"

#include "kamikaze_main.h"

/* ************************************************************************** */

class CommandeDessineGrapheObjet final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto scene_node = scene->active_node();

		if (!scene_node) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto object = static_cast<Object *>(scene_node);

		GraphDumper gd(object->graph());
		gd("/tmp/object_graph.gv");

		if (system("dot /tmp/object_graph.gv -Tpng -o object_graph.png") == -1) {
			std::cerr << "Cannot create graph image from dot\n";
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeDessineGrapheDependance final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		DepsGraphDumper gd(scene->depsgraph());
		gd("/tmp/depsgraph.gv");

		if (system("dot /tmp/depsgraph.gv -Tpng -o depsgraph.png") == -1) {
			std::cerr << "Cannot create graph image from dot\n";
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeGrapheZoom final : public Commande {
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto zoom = 1.0f;

		if (donnees.metadonnee == "10%") {
			zoom = 0.1f;
		}
		else if (donnees.metadonnee == "25%") {
			zoom = 0.25f;
		}
		else if (donnees.metadonnee == "50%") {
			zoom = 0.5f;
		}
		else if (donnees.metadonnee == "75%") {
			zoom = 0.75f;
		}
		else if (donnees.metadonnee == "90%") {
			zoom = 0.90f;
		}
		else if (donnees.metadonnee == "100%") {
			zoom = 1.0f;
		}
		else if (donnees.metadonnee == "150%") {
			zoom = 1.5f;
		}
		else if (donnees.metadonnee == "200%") {
			zoom = 2.0f;
		}
		else if (donnees.metadonnee == "300%") {
			zoom = 3.0f;
		}

		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());
		auto graphe = objet->graph();
		graphe->zoom(zoom);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeGrapheSupprimeSelection final : public Commande {
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		/* À FAIRE : évenements */
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());

		if (contexte.eval_ctx->edit_mode == true) {
			auto graphe = objet->graph();

			graphe->supprime_selection();

			scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
			scene->evalObjectDag(contexte, objet);
		}
		else {
			if (objet) {
				scene->removeObject(objet);
				scene->notify_listeners(type_evenement::objet | type_evenement::modifie);
				scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
			}
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}

	bool evalue_predicat(std::any const &pointeur, const std::string &/*metadonnee*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());

		if (objet == nullptr) {
			return false;
		}

		/* Un objet est sélectionné, mais nous ne sommes pas en mode édition. */
		if (contexte.eval_ctx->edit_mode == false) {
			return true;
		}

		auto graphe = objet->graph();

		return !graphe->selection_vide();
	}
};

/* ************************************************************************** */

class CommandeGrapheCentre final : public Commande {
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());
		auto graphe = objet->graph();

		for (auto &noeud : graphe->noeuds()) {
			noeud->posx(0);
			noeud->posy(0);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeGrapheBasculeExpansion final : public Commande {
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());
		auto graphe = objet->graph();

		for (auto &noeud : graphe->noeuds()) {
			if (donnees.metadonnee == "contracte") {
				noeud->enleve_drapeau(NOEUD_DILATE);
				noeud->ajoute_drapeau(NOEUD_CONTRACTE);
			}
			else {
				noeud->enleve_drapeau(NOEUD_CONTRACTE);
				noeud->ajoute_drapeau(NOEUD_DILATE);
			}
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeGrapheSelection final : public Commande {
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		const auto pos_x = donnees.x;
		const auto pos_y = donnees.y;

	//	std::cerr << "Sélection : x = " << pos_x << ", y = " << pos_y << '\n';

		auto scene = contexte.scene;
		auto objet = static_cast<Object *>(scene->active_node());

		if (contexte.eval_ctx->edit_mode) {
			auto graphe = objet->graph();
			graphe->deselectionne_tout();

			for (const auto &noeud : graphe->noeuds()) {
				const auto rect = Rectangle::depuis_centre(
									  noeud->posx(), noeud->posy(),
									  200.0f, 32.0f);

				if (rect.contiens(pos_x, pos_y)) {
					graphe->ajoute_selection(noeud.get());
				//	std::cerr << "Sélection noeud !\n";
					break;
				}
			}

			scene->notify_listeners(type_evenement::noeud | type_evenement::selectione);
		}
		else {
			scene->set_active_node(nullptr);

			for (const auto &noeud : scene->nodes()) {
				const auto rect = Rectangle::depuis_centre(
									  noeud->xpos(), noeud->ypos(),
									  200.0f, 32.0f);

				if (rect.contiens(pos_x, pos_y)) {
					scene->set_active_node(noeud.get());
				//	std::cerr << "Sélection objet !\n";
					break;
				}
			}

			scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
		}

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees)
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		auto scene = contexte.scene;

		if (contexte.eval_ctx->edit_mode) {
			auto objet = static_cast<Object *>(scene->active_node());
			auto graphe = objet->graph();
			auto noeud = graphe->noeud_actif();

			if (noeud == nullptr) {
				return;
			}

			noeud->posx(donnees.x);
			noeud->posy(donnees.y);
		}
		else {
			auto noeud = scene->active_node();

			if (noeud == nullptr) {
				return;
			}

			noeud->xpos(donnees.x);
			noeud->ypos(donnees.y);
		}

		scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
	}
};

/* ************************************************************************** */

class CommandeGrapheEntreObjet : public Commande {
public:
	CommandeGrapheEntreObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		const auto pos_x = donnees.x;
		const auto pos_y = donnees.y;

	//	std::cerr << "Double clique : x = " << pos_x << ", y = " << pos_y << '\n';

		auto scene = contexte.scene;

		if (contexte.eval_ctx->edit_mode) {
			auto objet = static_cast<Object *>(scene->active_node());
			auto graphe = objet->graph();
			graphe->deselectionne_tout();


			/* À FAIRE : vérifie que le clique ne touche rien. */

			scene->notify_listeners(type_evenement::noeud | type_evenement::modifie);
		}
		else {
			auto noeud = scene->active_node();

			if (noeud == nullptr) {
				return EXECUTION_COMMANDE_ECHOUEE;
			}

			auto rectangle = Rectangle::depuis_centre(
								 noeud->xpos(), noeud->ypos(),
								 200.0f, 32.0f);

			std::cerr << "Rectangle (K) : " << rectangle.x << ", " << rectangle.y << ", "
					  << rectangle.largeur << ", " << rectangle.hauteur << "\n";

			if (rectangle.contiens(pos_x, pos_y)) {
				contexte.eval_ctx->edit_mode = true;
			}
			else {
				scene->set_active_node(nullptr);
			}

			scene->notify_listeners(type_evenement::objet | type_evenement::selectione);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_graphes(UsineCommande *usine)
{
	usine->enregistre_type("dessine_graphe_objet",
						   description_commande<CommandeDessineGrapheObjet>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("dessine_graphe_dependance",
						   description_commande<CommandeDessineGrapheDependance>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("graphe_zoom",
						   description_commande<CommandeGrapheZoom>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("graphe_supprime_selection",
						   description_commande<CommandeGrapheSupprimeSelection>(
							   "graphe", 0, 0, Qt::Key_Delete, false));

	usine->enregistre_type("graphe_centre",
						   description_commande<CommandeGrapheCentre>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("graphe_bascule_expansion",
						   description_commande<CommandeGrapheBasculeExpansion>(
							   "graphe", 0, 0, 0, false));

	usine->enregistre_type("graphe_selection",
						   description_commande<CommandeGrapheSelection>(
							   "graphe", Qt::LeftButton, 0, 0, false));

	usine->enregistre_type("graphe_entre_objet",
						   description_commande<CommandeGrapheEntreObjet>(
							   "graphe", Qt::LeftButton, 0, 0, true));
}
