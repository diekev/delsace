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

#include <QAction>

namespace danjo {

class RepondantBouton;

/**
 * La classe bouton représente un bouton cliquable dans l'entreface. Elle
 * contient un pointeur vers un RepondantBouton qui est appelé quand le bouton
 * est cliqué.
 */
class Action : public QAction {
	Q_OBJECT

	RepondantBouton *m_repondant = nullptr;
	std::string m_attache = "";
	std::string m_metadonnee = "";

public:
	explicit Action(QWidget *parent = nullptr);

	Action(Action const &) = default;
	Action &operator=(Action const &) = default;

	/**
	 * Installe le RepondantBouton qui sera appelé lors d'un clique sur ce
	 * bouton.
	 */
	void installe_repondant(RepondantBouton *repondant);

	/**
	 * Établie l'attache du bouton, c'est-à-dire l'identifiant qui sera passé
	 * au RepondantBouton lors d'un clique sur ce bouton.
	 */
	void etablie_attache(const std::string &attache);

	/**
	 * Établie la valeur du bouton, c'est-à-dire la chaîne de caractère qui sera
	 * affichée sur le bouton dans l'entreface.
	 */
	void etablie_valeur(const std::string &valeur);

	/**
	 * Établie le contenu de l'infobulle du bouton.
	 */
	void etablie_infobulle(const std::string &valeur);

	/**
	 * Établie la métadonnée de l'action, c'est-à-dire la chaîne de caractère
	 * qui sera passée au RepondantBouton lors d'un clique sur ce bouton.
	 */
	void etablie_metadonnee(const std::string &metadonnee);

	/**
	 * Établie l'icône de cette action.
	 */
	void etablie_icone(const std::string &valeur);

	/**
	 * Évalue le prédicat de l'attache de cette action, pour soit activer, soit
	 * désactiver l'action.
	 */
	void evalue_predicat();

public Q_SLOTS:
	/**
	 * Notifie le RepondantBouton que le bouton a été pressé.
	 */
	void action_presse();
};

}  /* namespace danjo */
