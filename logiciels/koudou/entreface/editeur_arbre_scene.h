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

#pragma once

#include "base_editeur.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QTreeWidget>
#pragma GCC diagnostic pop

namespace kdo {
class Objet;
}
class QScrollArea;
class QGridLayout;

/* ************************************************************************** */

class ItemObjet : public QTreeWidgetItem {
	const kdo::Objet *m_pointeur;

public:
	explicit ItemObjet(const kdo::Objet *pointeur, QTreeWidgetItem *parent = nullptr);

	ItemObjet(ItemObjet const &) = default;
	ItemObjet &operator=(ItemObjet const &) = default;

	const kdo::Objet *pointeur() const;
};

/* ************************************************************************** */

class WidgetArbre : public QTreeWidget {
	BaseEditrice *m_base = nullptr;

public:
	explicit WidgetArbre(QWidget *parent = nullptr);

	WidgetArbre(WidgetArbre const &) = default;
	WidgetArbre &operator=(WidgetArbre const &) = default;

	void set_base(BaseEditrice *base);

	void mousePressEvent(QMouseEvent *e) override;
};

/* ************************************************************************** */

class EditeurArbreScene final : public BaseEditrice {
	Q_OBJECT

	WidgetArbre *m_widget_arbre;

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;

public:
	EditeurArbreScene(kdo::Koudou *koudou, QWidget *parent = nullptr);

	EditeurArbreScene(EditeurArbreScene const &) = default;
	EditeurArbreScene &operator=(EditeurArbreScene const &) = default;

	~EditeurArbreScene() override;

	void ajourne_etat(int evenement) override;

private Q_SLOTS:
	void ajourne_vue();
	void repond_selection();
};

