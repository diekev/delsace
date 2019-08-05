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

#include "editeur_arbre_scene.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "coeur/evenement.h"
#include "coeur/koudou.h"
#include "coeur/objet.h"
#include "coeur/maillage.h"
#include "coeur/scene.h"

/* ************************************************************************** */

ItemObjet::ItemObjet(const kdo::Objet *pointeur, QTreeWidgetItem *parent)
	: QTreeWidgetItem(parent)
	, m_pointeur(pointeur)
{
	setText(0, pointeur->nom.c_str());
}

const kdo::Objet *ItemObjet::pointeur() const
{
	return m_pointeur;
}

/* ************************************************************************** */

WidgetArbre::WidgetArbre(QWidget *parent)
	: QTreeWidget(parent)
{
	setIconSize(QSize(20, 20));
	setAllColumnsShowFocus(true);
	setAnimated(false);
	setAutoScroll(false);
	setUniformRowHeights(true);
	setSelectionMode(SingleSelection);
	setFocusPolicy(Qt::NoFocus);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setHeaderHidden(true);
	setDragDropMode(NoDragDrop);
	setDragEnabled(false);
}

void WidgetArbre::set_base(BaseEditrice *base)
{
	m_base = base;
}

void WidgetArbre::mousePressEvent(QMouseEvent *e)
{
	m_base->rend_actif();
	QTreeWidget::mousePressEvent(e);
}

/* ************************************************************************** */

EditeurArbreScene::EditeurArbreScene(kdo::Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
	, m_widget_arbre(new WidgetArbre(this))
	, m_widget(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QGridLayout(m_widget))
{
	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);

	m_widget_arbre->set_base(this);
	m_glayout->addWidget(m_widget_arbre);

	connect(m_widget_arbre, SIGNAL(itemSelectionChanged()),
			this, SLOT(repond_selection()));
}

EditeurArbreScene::~EditeurArbreScene()
{

}

void EditeurArbreScene::ajourne_etat(int evenement)
{
	auto creation = (evenement == (type_evenement::objet | type_evenement::ajoute));
	creation |= (evenement == (type_evenement(-1)));

	if (!creation) {
		return;
	}

	m_widget_arbre->clear();

	auto &scene = m_koudou->parametres_rendu.scene;

	auto racine_maillages = new QTreeWidgetItem();
	racine_maillages->setText(0, "maillages");
	m_widget_arbre->addTopLevelItem(racine_maillages);
	racine_maillages->setExpanded(true);

	auto racine_lumieres = new QTreeWidgetItem();
	racine_lumieres->setText(0, "lumieres");
	m_widget_arbre->addTopLevelItem(racine_lumieres);
	racine_lumieres->setExpanded(true);

	for (auto const &objet : scene.objets) {
		auto item = new ItemObjet(objet);
		item->setSelected(objet == scene.objet_actif);

		if (objet->type == kdo::TypeObjet::MAILLAGE) {
			racine_maillages->addChild(item);
		}
		else if (objet->type == kdo::TypeObjet::LUMIERE) {
			racine_lumieres->addChild(item);
		}
	}
}

void EditeurArbreScene::ajourne_vue()
{

}

void EditeurArbreScene::repond_selection()
{
	auto items = m_widget_arbre->selectedItems();

	if (items.size() != 1) {
		return;
	}

	auto item = items[0];

	auto item_objet = dynamic_cast<ItemObjet *>(item);

	if (!item_objet) {
		return;
	}

	auto &scene = m_koudou->parametres_rendu.scene;
	scene.objet_actif = const_cast<kdo::Objet *>(item_objet->pointeur());
	m_koudou->notifie_observatrices(type_evenement::objet | type_evenement::selectione);
}
