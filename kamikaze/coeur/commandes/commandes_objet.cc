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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "commandes_objet.h"

#include "sdk/noeud.h"
#include "sdk/operatrice.h"
#include "sdk/primitive.h"

#include "bibliotheques/commandes/commande.h"

#include "graphs/object_graph.h"
#include "operatrices/operatrices_standards.h"

#include "kamikaze_main.h"
#include "object.h"
#include "scene.h"

/* *************************** add object command *************************** */

class CommandeAjoutObjet : public Commande {
	Object *m_object = nullptr;
	Scene *m_scene = nullptr;

public:
	CommandeAjoutObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override;
};

int CommandeAjoutObjet::execute(std::any const &pointeur, const DonneesCommande &donnees)
{
	auto main = std::any_cast<Main *>(pointeur);
	const auto &contexte = main->contexte;
	m_scene = contexte.scene;

	m_object = new Object(contexte);
	m_object->name(donnees.metadonnee);

	assert(m_scene != nullptr);
	m_scene->addObject(m_object);

	return EXECUTION_COMMANDE_REUSSIE;
}

/* **************************** add node command **************************** */

class CommandeAjoutNoeud : public Commande {
	Object *m_object = nullptr;
	Scene *m_scene = nullptr;

public:
	CommandeAjoutNoeud() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override;
};

int CommandeAjoutNoeud::execute(std::any const &pointeur, const DonneesCommande &donnees)
{
	auto main = std::any_cast<Main *>(pointeur);
	const auto &contexte = main->contexte;
	m_scene = contexte.scene;
	auto scene_node = m_scene->active_node();

	if (scene_node == nullptr) {
		return EXECUTION_COMMANDE_ECHOUEE;
	}

	m_object = static_cast<Object *>(scene_node);

	assert(m_object != nullptr);

	auto noeud = new Noeud();
	noeud->nom(donnees.metadonnee);

	auto operatrice = (*contexte.usine_operatrice)(donnees.metadonnee, noeud, contexte);
	static_cast<void>(operatrice);

	noeud->synchronise_donnees();

	m_object->ajoute_noeud(noeud);

	m_scene->notify_listeners(type_evenement::noeud | type_evenement::ajoute);

	return EXECUTION_COMMANDE_REUSSIE;
}

/* **************************** add torus command **************************** */

class CommandeObjetPrereglage : public Commande {
	Object *m_object = nullptr;
	Scene *m_scene = nullptr;

public:
	CommandeObjetPrereglage() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override;
};

int CommandeObjetPrereglage::execute(std::any const &pointeur, const DonneesCommande &donnees)
{
	auto main = std::any_cast<Main *>(pointeur);
	const auto &contexte = main->contexte;
	m_scene = contexte.scene;

	if (contexte.eval_ctx->edit_mode) {
		auto scene_node = m_scene->active_node();

		/* Sanity check. */
		if (scene_node == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		m_object = static_cast<Object *>(scene_node);
	}
	else {
		m_object = new Object(contexte);
		m_object->name(donnees.metadonnee);
	}

	assert(m_object != nullptr);

	auto noeud = new Noeud();
	noeud->posx(-300);
	noeud->posy(-100);

	(*contexte.usine_operatrice)(donnees.metadonnee, noeud, contexte);

	noeud->synchronise_donnees();

	m_object->ajoute_noeud(noeud);

	auto graph = m_object->graph();
	graph->ajoute_selection(noeud);
	graph->connecte(noeud->sortie(0), graph->sortie()->entree(0));

	if (!contexte.eval_ctx->edit_mode) {
		m_scene->addObject(m_object);
		m_scene->evalObjectDag(contexte, m_object);
	}
	else {
		m_scene->notify_listeners(type_evenement::noeud | type_evenement::ajoute);
	}

	return EXECUTION_COMMANDE_REUSSIE;
}

/* ************************************************************************** */

class CommandeEntreObjet : public Commande {
public:
	CommandeEntreObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		contexte.eval_ctx->edit_mode = true;
		contexte.scene->notify_listeners(type_evenement::objet | type_evenement::selectione);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSorsObjet : public Commande {
public:
	CommandeSorsObjet() = default;

	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = std::any_cast<Main *>(pointeur);
		const auto &contexte = main->contexte;
		contexte.eval_ctx->edit_mode = false;
		contexte.scene->notify_listeners(type_evenement::objet | type_evenement::selectione);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_objet(UsineCommande *usine)
{
	usine->enregistre_type("ajouter_objet",
						   description_commande<CommandeAjoutObjet>(
							   "objet", 0, 0, 0, false));

	usine->enregistre_type("ajouter_noeud",
						   description_commande<CommandeAjoutNoeud>(
							   "objet", 0, 0, 0, false));

	usine->enregistre_type("ajouter_prereglage",
						   description_commande<CommandeObjetPrereglage>(
							   "objet", 0, 0, 0, false));

	usine->enregistre_type("objet_entre",
						   description_commande<CommandeEntreObjet>(
							   "objet", 0, 0, 0, false));

	usine->enregistre_type("objet_sors",
						   description_commande<CommandeSorsObjet>(
							   "objet", 0, 0, 0, false));
}
