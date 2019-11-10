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

#include <QMenu>

#include "biblinternes/structures/dico.hh"

/**
 * Un MenuFiltrable est un menu qui nous permet de chercher une de ses actions
 * en tapant un texte dans une barre de recherche. Les actions pouvant
 * correspondre au texte sont ajoutées dans un menu auxiliaire affiché à côté
 * du MenuFiltrable actif.
 *
 * L'implémentation est tiré de
 * http://www.mprazak.info/posts/fuzzy-search-menu-in-qt/
 */
class MenuFiltrable final : public QMenu {
	Q_OBJECT

	dls::dico<QString, QAction *> m_actions{};
	QMenu *m_menu_auxiliaire = nullptr;

public:
	MenuFiltrable(const QString &titre, QWidget *parent = nullptr);

	MenuFiltrable(MenuFiltrable const &) = default;
	MenuFiltrable &operator=(MenuFiltrable const &) = default;

protected:
	void showEvent(QShowEvent *event) override;

private:
	/* Initialisation recursive du tableau d'actions depuis un menu et de ses
	 * sous-menu. */
	void init(QMenu *menu);

private Q_SLOTS:
	void changement_texte(const QString &texte);

	void evalue_predicats_action();
};
