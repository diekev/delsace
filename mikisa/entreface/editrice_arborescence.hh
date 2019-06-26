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

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QTreeWidget>
#pragma GCC diagnostic pop

#include "base_editrice.h"

class Noeud;
class Scene;
class Objet;

/* ************************************************************************** */

class SceneTreeWidgetItem : public QWidget, public QTreeWidgetItem {
	Scene *m_scene;
    bool m_visited;

public:
    explicit SceneTreeWidgetItem(Scene *scene, QWidget *parent = nullptr);

	SceneTreeWidgetItem(SceneTreeWidgetItem const &) = default;
	SceneTreeWidgetItem &operator=(SceneTreeWidgetItem const &) = default;

    Scene *getScene() const;

    bool visited() const;
    void setVisited();
};

/* ************************************************************************** */

class ObjectTreeWidgetItem : public QTreeWidgetItem {
	Objet *m_scene_node;
    bool m_visited;

public:
	explicit ObjectTreeWidgetItem(Objet *scene_node, QTreeWidgetItem *parent = nullptr);

	ObjectTreeWidgetItem(ObjectTreeWidgetItem const &) = default;
	ObjectTreeWidgetItem &operator=(ObjectTreeWidgetItem const &) = default;

	Objet *getNode() const;

    bool visited() const;
    void setVisited();
};

/* ************************************************************************** */

class ObjectNodeTreeWidgetItem : public QTreeWidgetItem {
	Noeud *m_noeud;

public:
	explicit ObjectNodeTreeWidgetItem(Noeud *noeud, QTreeWidgetItem *parent = nullptr);

	ObjectNodeTreeWidgetItem(ObjectNodeTreeWidgetItem const &) = default;
	ObjectNodeTreeWidgetItem &operator=(ObjectNodeTreeWidgetItem const &) = default;

	Noeud *pointeur_noeud() const;
};

/* ************************************************************************** */

class TreeWidget : public QTreeWidget {
	BaseEditrice *m_base = nullptr;

public:
	explicit TreeWidget(QWidget *parent = nullptr);

	TreeWidget(TreeWidget const &) = default;
	TreeWidget &operator=(TreeWidget const &) = default;

	void set_base(BaseEditrice *base);

	void mousePressEvent(QMouseEvent *e) override;
	void dropEvent(QDropEvent *event) override;
};

/* ************************************************************************** */

/* This is to add a level of indirection because we can't have an object derive
 * from both QTreeWidget and WidgetBase, and we can't apparently use virtual
 * inheritance with Qt classes. */
class EditriceArborescence : public BaseEditrice {
	Q_OBJECT

	TreeWidget *m_tree_widget;

public:
	explicit EditriceArborescence(Mikisa &mikisa, QWidget *parent = nullptr);

	EditriceArborescence(EditriceArborescence const &) = default;
	EditriceArborescence &operator=(EditriceArborescence const &) = default;

	void ajourne_etat(int evenement) override;

	void ajourne_manipulable() override {}

public Q_SLOTS:
	void handleItemCollapsed(QTreeWidgetItem *item);
	void handleItemExpanded(QTreeWidgetItem *item);
	void handleItemSelection();
};
