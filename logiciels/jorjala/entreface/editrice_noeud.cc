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

#include "editrice_noeud.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QToolTip>
#pragma GCC diagnostic pop

#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "graphe/item_noeud.h"
#include "graphe/vue_editrice_graphe.h"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/jorjala.hh"

EditriceGraphe::EditriceGraphe(Jorjala &jorjala, QWidget *parent)
	: BaseEditrice(jorjala, parent)
	, m_scene(new QGraphicsScene(this))
	, m_vue(new VueEditeurNoeud(jorjala, this, this))
	, m_barre_chemin(new QLineEdit())
	, m_selecteur_graphe(new QComboBox(this))
{
	m_vue->setScene(m_scene);

	auto disposition_vert = new QVBoxLayout();
	auto disposition_barre = new QHBoxLayout();

	m_selecteur_graphe->addItem("Graphe Composites", QVariant("composites"));
	m_selecteur_graphe->addItem("Graphe Nuanceurs", QVariant("nuanceurs"));
	m_selecteur_graphe->addItem("Graphe Objets", QVariant("objets"));
	m_selecteur_graphe->addItem("Graphe Rendus", QVariant("rendus"));

	m_selecteur_graphe->setCurrentIndex(2);

	connect(m_selecteur_graphe, SIGNAL(currentIndexChanged(int)),
			this, SLOT(change_contexte(int)));

	disposition_barre->addWidget(m_selecteur_graphe);
	disposition_barre->addWidget(m_barre_chemin);

	auto bouton_retour = new QPushButton("^");
	disposition_barre->addWidget(bouton_retour);

	connect(bouton_retour, &QPushButton::clicked, this, &EditriceGraphe::sors_noeud);

	disposition_vert->addLayout(disposition_barre);
	disposition_vert->addWidget(m_vue);

	m_main_layout->addLayout(disposition_vert);
}

EditriceGraphe::~EditriceGraphe()
{
	delete m_scene;
}

void EditriceGraphe::ajourne_etat(int evenement)
{
	auto creation = (categorie_evenement(type_evenement(evenement)) == type_evenement::noeud);
	creation |= (evenement == (type_evenement::image | type_evenement::traite));
	creation |= (evenement == (type_evenement::objet | type_evenement::ajoute));
	creation |= (evenement == (type_evenement::objet | type_evenement::enleve));
	creation |= (evenement == (type_evenement::rafraichissement));

	if (!creation) {
		return;
	}

	/* ajourne le sélecteur, car il sera désynchronisé lors des ouvertures de
	 * fichiers */
	{
		auto const bloque_signaux = m_selecteur_graphe->blockSignals(true);

		switch (m_jorjala.chemin_courant[1]) {
			case 'c':
			{
				m_selecteur_graphe->setCurrentIndex(0);
				break;
			}
			case 'n':
			{
				m_selecteur_graphe->setCurrentIndex(1);
				break;
			}
			case 'o':
			{
				m_selecteur_graphe->setCurrentIndex(2);
				break;
			}
			case 'r':
			{
				m_selecteur_graphe->setCurrentIndex(3);
				break;
			}
		}

		m_selecteur_graphe->blockSignals(bloque_signaux);
	}

	m_scene->clear();
	m_scene->items().clear();
	assert(m_scene->items().size() == 0);

	auto const graphe = m_jorjala.graphe;

	if (graphe == nullptr) {
		return;
	}

	m_vue->resetTransform();

	/* m_vue->centerOn ne semble pas fonctionner donc on modifier le rectangle
	 * de la scène */
	auto rect_scene = m_scene->sceneRect();
	auto const largeur = rect_scene.width();
	auto const hauteur = rect_scene.height();

	rect_scene = QRectF(graphe->centre_x - static_cast<float>(largeur) * 0.5f,
						graphe->centre_y - static_cast<float>(hauteur) * 0.5f,
						largeur,
						hauteur);

	m_scene->setSceneRect(rect_scene);

	m_vue->scale(graphe->zoom, graphe->zoom);

	for (auto node_ptr : graphe->noeuds()) {
		auto item = new ItemNoeud(
					node_ptr,
					node_ptr == graphe->noeud_actif,
					graphe->type == type_graphe::DETAIL || graphe->type == type_graphe::CYCLES);
		m_scene->addItem(item);

		for (PriseEntree *prise : node_ptr->entrees) {
			if (prise->liens.est_vide()) {
				continue;
			}

			for (auto lien : prise->liens) {
				auto const x1 = prise->rectangle.x + prise->rectangle.largeur / 2.0f;
				auto const y1 = prise->rectangle.y + prise->rectangle.hauteur / 2.0f;
				auto const x2 = lien->rectangle.x + lien->rectangle.largeur / 2.0f;
				auto const y2 = lien->rectangle.y + lien->rectangle.hauteur / 2.0f;

				auto ligne = new QGraphicsLineItem();
				ligne->setPen(QPen(Qt::white, 2.0));
				ligne->setLine(x1, y1, x2, y2);

				m_scene->addItem(ligne);
			}
		}
	}

	if (graphe->connexion_active) {
		float x1, y1;

		if (graphe->connexion_active->prise_entree) {
			auto prise_entree = graphe->connexion_active->prise_entree;
			x1 = prise_entree->rectangle.x + prise_entree->rectangle.largeur / 2.0f;
			y1 = prise_entree->rectangle.y + prise_entree->rectangle.hauteur / 2.0f;
		}
		else {
			auto prise_sortie = graphe->connexion_active->prise_sortie;
			x1 = prise_sortie->rectangle.x + prise_sortie->rectangle.largeur / 2.0f;
			y1 = prise_sortie->rectangle.y + prise_sortie->rectangle.hauteur / 2.0f;
		}

		auto const x2 = graphe->connexion_active->x;
		auto const y2 = graphe->connexion_active->y;

		auto ligne = new QGraphicsLineItem();
		ligne->setPen(QPen(Qt::white, 2.0));
		ligne->setLine(x1, y1, x2, y2);

		m_scene->addItem(ligne);
	}

	if (graphe->info_noeud) {
		auto point = m_vue->mapFromScene(graphe->info_noeud->x, graphe->info_noeud->y);
		point = m_vue->mapToGlobal(point);
		QToolTip::showText(point, graphe->info_noeud->informations.c_str());
	}

	m_barre_chemin->setText(m_jorjala.chemin_courant.c_str());
}

void EditriceGraphe::sors_noeud()
{
	m_jorjala.repondant_commande()->repond_clique("sors_noeud", "");
}

void EditriceGraphe::change_contexte(int index)
{
	INUTILISE(index);
	auto repondant_commande = m_jorjala.repondant_commande();
	auto valeur = m_selecteur_graphe->currentData().toString().toStdString();

	repondant_commande->repond_clique("change_contexte", valeur);
}
