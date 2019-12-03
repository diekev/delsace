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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editrice_arborescence.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDropEvent>
#include <QHBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/outils/conditions.h"

#include "coeur/evenement.h"
#include "coeur/jorjala.hh"
#include "coeur/objet.h"

//#define DRAG_DROP_PARENTING

/* ************************************************************************** */

SceneTreeWidgetItem::SceneTreeWidgetItem(BaseDeDonnees *scene, QWidget *parent)
    : QWidget(parent)
    , m_scene(scene)
    , m_visited(false)
{
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	setText(0, "objets");
}

BaseDeDonnees *SceneTreeWidgetItem::getScene() const
{
	return m_scene;
}

bool SceneTreeWidgetItem::visited() const
{
	return m_visited;
}

void SceneTreeWidgetItem::setVisited()
{
	m_visited = true;
}

/* ************************************************************************** */

ObjectTreeWidgetItem::ObjectTreeWidgetItem(Objet *scene_node, QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent)
    , m_scene_node(scene_node)
    , m_visited(false)
{
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	setText(0, m_scene_node->noeud->nom.c_str());

#ifdef DRAG_DROP_PARENTING
	setFlags(flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
#endif
}

Objet *ObjectTreeWidgetItem::getNode() const
{
	return m_scene_node;
}

bool ObjectTreeWidgetItem::visited() const
{
	return m_visited;
}

void ObjectTreeWidgetItem::setVisited()
{
	m_visited = true;
}

/* ************************************************************************** */

ObjectNodeTreeWidgetItem::ObjectNodeTreeWidgetItem(Noeud *noeud, QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent)
	, m_noeud(noeud)
{
	setText(0, m_noeud->nom.c_str());
}

Noeud *ObjectNodeTreeWidgetItem::pointeur_noeud() const
{
	return m_noeud;
}

/* ************************************************************************** */

TreeWidget::TreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
	setIconSize(QSize(20, 20));
	setAllColumnsShowFocus(true);
	setAnimated(false);
	setAutoScroll(false);
	setUniformRowHeights(true);
	setSelectionMode(SingleSelection);
#ifdef DRAG_DROP_PARENTING
	setDragDropMode(InternalMove);
	setDragEnabled(true);
#else
	setDragDropMode(NoDragDrop);
	setDragEnabled(false);
#endif
	setFocusPolicy(Qt::NoFocus);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setHeaderHidden(true);
}

void TreeWidget::set_base(BaseEditrice *base)
{
	m_base = base;
}

void TreeWidget::mousePressEvent(QMouseEvent *e)
{
	m_base->rend_actif();
	QTreeWidget::mousePressEvent(e);
}

void TreeWidget::dropEvent(QDropEvent *event)
{
	if (event->source() != this) {
		return;
	}

#ifdef DRAG_DROP_PARENTING
	auto item = itemAt(event->pos());

	if (!item) {
		return;
	}

	auto object_item = dynamic_cast<ObjectTreeWidgetItem *>(item);

	if (!object_item) {
		return;
	}

	auto source_item = dynamic_cast<ObjectTreeWidgetItem *>(selectedItems()[0]);

	if (!source_item) {
		return;
	}

	if (source_item == object_item) {
		return;
	}

	m_context->scene->connect(m_context, object_item->getNode(), source_item->getNode());
#endif

	QTreeView::dropEvent(event);
}

/* ************************************************************************** */

EditriceArborescence::EditriceArborescence(Jorjala &jorjala, QWidget *parent)
	: BaseEditrice(jorjala, parent)
    , m_tree_widget(new TreeWidget(this))
{
	m_main_layout->addWidget(m_tree_widget);

	m_tree_widget->set_base(this);

	connect(m_tree_widget, SIGNAL(itemExpanded(QTreeWidgetItem *)),
	        this, SLOT(handleItemExpanded(QTreeWidgetItem *)));

	connect(m_tree_widget, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
	        this, SLOT(handleItemCollapsed(QTreeWidgetItem *)));

	connect(m_tree_widget, SIGNAL(itemSelectionChanged()),
	        this, SLOT(handleItemSelection()));
}

void EditriceArborescence::ajourne_etat(int evenement_)
{
	auto evenement = static_cast<type_evenement>(evenement_);

	if (evenement == static_cast<type_evenement>(-1)) {
		return;
	}

	if (!dls::outils::est_element(categorie_evenement(evenement), type_evenement::objet, type_evenement::noeud)) {
		return;
	}

	if (!dls::outils::est_element(action_evenement(evenement), type_evenement::ajoute, type_evenement::modifie)) {
		return;
	}

	/* For now we clear and recreate everything from scratch on every call for
	 * updates. Maybe there is a slightly better way to do so. */
	m_tree_widget->clear();

	auto item = new SceneTreeWidgetItem(&m_jorjala.bdd, this);
	m_tree_widget->addTopLevelItem(item);

	/* Need to first add the item to the tree. */
//	item->setExpanded(scene->has_flags(SCENE_OL_EXPANDED));
}

void EditriceArborescence::handleItemExpanded(QTreeWidgetItem *item)
{
	auto scene_item = dynamic_cast<SceneTreeWidgetItem *>(item);

	if (scene_item && !scene_item->visited()) {
		auto scene = scene_item->getScene();
	//	scene->set_flags(SCENE_OL_EXPANDED);

		for (auto const &objet : scene->objets()) {
//			if (object->parent() != nullptr) {
//				continue;
//			}

			auto child = new ObjectTreeWidgetItem(objet, scene_item);
//			child->setSelected(objet == scene->objet_actif());
			scene_item->addChild(child);
//			child->setExpanded(node->has_flags(SNODE_OL_EXPANDED));
		}

		scene_item->setVisited();
		return;
	}

	auto object_item = dynamic_cast<ObjectTreeWidgetItem *>(item);

	if (object_item && !object_item->visited()) {
//		auto objet = object_item->getNode();
//		object->set_flags(SNODE_OL_EXPANDED);

//		for (auto const &noeud : objet->graph()->noeuds()) {
//			auto node_item = new ObjectNodeTreeWidgetItem(noeud.get(), object_item);
//			object_item->addChild(node_item);
//		}

//		for (auto const &child : object->children()) {
//			auto child_item = new ObjectTreeWidgetItem(child, object_item);
//			child_item->setSelected(child == m_context->scene->active_node());
//			object_item->addChild(child_item);
//			child_item->setExpanded(child->has_flags(SNODE_OL_EXPANDED));
//		}

		object_item->setVisited();
		return;
	}
}

void EditriceArborescence::handleItemCollapsed(QTreeWidgetItem *item)
{
	auto scene_item = dynamic_cast<SceneTreeWidgetItem *>(item);

	if (scene_item) {
//		Scene *scene = scene_item->getScene();
	//	scene->unset_flags(SCENE_OL_EXPANDED);
		return;
	}

	auto object_item = dynamic_cast<ObjectTreeWidgetItem *>(item);

	if (object_item) {
//		auto node = object_item->getNode();
	//	node->unset_flags(SNODE_OL_EXPANDED);
		return;
	}
}

void EditriceArborescence::handleItemSelection()
{
	auto items = m_tree_widget->selectedItems();

	if (items.size() != 1) {
		return;
	}

	auto item = items[0];

	auto scene_item = dynamic_cast<SceneTreeWidgetItem *>(item);

	if (scene_item) {
		return;
	}

	auto object_item = dynamic_cast<ObjectTreeWidgetItem *>(item);

	if (object_item) {
	//	m_context->scene->set_active_node(object_item->getNode());
		return;
	}
}
