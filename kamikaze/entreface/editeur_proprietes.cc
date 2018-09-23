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

#include "editeur_proprietes.h"

#include <iostream>

#include <QHBoxLayout>
#include <QScrollArea>

#include "sdk/operatrice.h"

#include <danjo/danjo.h>

#include "coeur/graphs/object_graph.h"
#include "coeur/object.h"
#include "coeur/scene.h"

#include "util/utils.h"

/* La hierarchie est la suivante :
 *
 * Disposition Principale
 * -- Scroll
 * ---- Widget
 * ------ VLayout
 * --------- Widget Alarmes
 * --------- Widget Interface
 */

EditriceProprietes::EditriceProprietes(QWidget *parent)
	: BaseEditrice(parent)
    , m_widget(new QWidget())
	, m_conteneur_disposition(new QWidget())
	, m_conteneur_alarmes(nullptr)
    , m_scroll(new QScrollArea())
	, m_disposition_widget(new QVBoxLayout(m_widget))
{
	m_widget->setSizePolicy(m_frame->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_main_layout->addWidget(m_scroll);

	m_disposition_widget->addWidget(m_conteneur_disposition);
}

void EditriceProprietes::update_state(type_evenement event)
{
	danjo::Manipulable *manipulable = nullptr;
	auto scene = m_context->scene;
	const char *chemin_entreface = "";

	if (scene->active_node() == nullptr) {
		return;
	}

	const auto &event_category = categorie_evenement(event);
	const auto &event_action = action_evenement(event);

	//std::vector<std::string> warnings;

	if (event_category == type_evenement::objet) {
		if (is_elem(event_action, type_evenement::ajoute, type_evenement::selectione)) {
			manipulable = scene->active_node();
			chemin_entreface = "entreface/proprietes_objet.jo";
		}
		else if (is_elem(event_action, type_evenement::enleve)) {
			efface_disposition();
			return;
		}
	}
	else if (event_category == (type_evenement::noeud)) {
		if (is_elem(event_action, type_evenement::selectione, type_evenement::traite)) {
			auto scene_node = scene->active_node();
			auto object = static_cast<Object *>(scene_node);
			auto graph = object->graph();
			auto noeud = graph->noeud_actif();

			if (noeud == nullptr) {
				return;
			}

			auto operatrice = noeud->operatrice();
			manipulable = operatrice;
			chemin_entreface = operatrice->chemin_entreface();
			//warnings = operatrice->avertissements();
		}
		else if (is_elem(event_action, type_evenement::enleve)) {
			efface_disposition();
			return;
		}
	}
	else {
		return;
	}

	if (manipulable == nullptr) {
		return;
	}

	efface_disposition();

	/* À FAIRE : affiche avertissements */

	/* À FAIRE : set_context */
	dessine_entreface(manipulable, chemin_entreface);
}

void EditriceProprietes::dessine_entreface(danjo::Manipulable *manipulable, const char *chemin_entreface)
{
	manipulable->ajourne_proprietes();

	const auto &texte = danjo::contenu_fichier(chemin_entreface);

	if (texte.empty()) {
		return;
	}

	danjo::DonneesInterface donnees;
	donnees.manipulable = manipulable;
	donnees.conteneur = this;

	auto disposition = danjo::compile_entreface(donnees, texte.c_str());

	m_conteneur_disposition->setLayout(disposition);

	m_manipulable = manipulable;
}

void EditriceProprietes::ajourne_manipulable()
{
	m_manipulable->ajourne_proprietes();

	/* À FAIRE : redessine entreface. */

	if (m_context->eval_ctx->edit_mode) {
		/* À FAIRE : n'évalue le graphe que si le noeud était connecté. */
		evalue_graphe();
	}
	else {
		ajourne_objet();
	}
}

void EditriceProprietes::efface_disposition()
{
	if (!m_conteneur_disposition->layout()) {
		return;
	}

	/* Qt ne permet d'extrait la disposition d'un widget que si celle-ci est
	 * assignée à un autre widget. Donc pour détruire la disposition précédente
	 * nous la reparentons à un widget temporaire qui la détruira dans son
	 * destructeur. */
	QWidget temp;
	temp.setLayout(m_conteneur_disposition->layout());
}

void EditriceProprietes::evalue_graphe()
{
	this->set_active();
	auto scene = m_context->scene;
	auto scene_node = scene->active_node();
	auto object = static_cast<Object *>(scene_node);
	auto graph = object->graph();
	auto noeud = graph->noeud_actif();

	signifie_sale_aval(noeud);

	scene->evalObjectDag(*m_context, scene_node);
	scene->notify_listeners(static_cast<type_evenement>(-1));
}

void EditriceProprietes::ajourne_objet()
{
	this->set_active();
	m_context->scene->tagObjectUpdate();
}
