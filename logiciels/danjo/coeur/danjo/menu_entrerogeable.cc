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

#include "menu_entrerogeable.h"

#include <cassert>

#include <QApplication>
#include <QLineEdit>
#include <QKeyEvent>
#include <QWidgetAction>

/* ************************************************************************** */

static bool correspondance_floue(const QString &entree, const QString &texte)
{
	/* Une entrée vide correspond à tous les textes. */
	if (entree.isEmpty()) {
		return true;
	}

	/* Compte de caractères correspondants. */
	auto compte = 0;

	for (const auto &c : texte) {
		/* Essaye de correspondre un caractère simple depuis l'entrée. */
		if (c == entree[compte]) {
			++compte;
		}

		/* Si tous les caractères de l'entrée sont trouvés, retourne vrai. */
		if (compte == entree.length()) {
			return true;
		}
	}

	/* Tous les caractères de l'entrée n'ont pas été correspondus, retourne faux. */
	return false;
}

/* ************************************************************************** */

/**
 * Filtre spécial pour pouvoir capturer et redifuser les évènements de touches
 * claviers depuis l'éditeur de menu. Le but étant de pouvoir les flèches pour
 * sélectionner l'action désirée, et la touche entrée pour appuyer sur l'action.
 * Les autres touches sont renvoyées à l'éditeur afin de taper le nom de
 * l'action à rechercher.
 */
class FiltreEditeurMenu final : public QObject {
	QWidget *m_cible;

public:
	explicit FiltreEditeurMenu(QWidget *cible)
		: QObject(cible)
		, m_cible(cible)
	{}

	FiltreEditeurMenu(FiltreEditeurMenu const &) = default;
	FiltreEditeurMenu &operator=(FiltreEditeurMenu const &) = default;

protected:
	bool eventFilter(QObject *watched, QEvent *event) override
	{
		bool acceptee = false;

		if (event->type() == QEvent::KeyPress) {
			auto evenement_cle = static_cast<QKeyEvent *>(event);

			if(
					// moving in the child menu
					evenement_cle->key() != Qt::Key_Down && evenement_cle->key() != Qt::Key_Up &&
					// activation of selected item
					evenement_cle->key() != Qt::Key_Enter && evenement_cle->key() != Qt::Key_Return &&
					// first close this menu, then parent
					evenement_cle->key() != Qt::Key_Escape) {

				QApplication::sendEvent(m_cible, evenement_cle);

				acceptee = true;
			}
		}

		// standard event processing
		if (!acceptee) {
			return QObject::eventFilter(watched, event);
		}

		return true;
	}
};

/* ************************************************************************** */

MenuEntrerogeable::MenuEntrerogeable(const QString &titre, QWidget *parent)
	: QMenu(titre, parent)
{}

void MenuEntrerogeable::init(QMenu *menu)
{
	const auto &acts = menu->actions();

	for (const auto &act : acts) {
		/* Évite l'éditeur de ligne. */
		if (dynamic_cast<QWidgetAction *>(act) != nullptr) {
			continue;
		}

		/* Initialisation récursive. */
		if (act->menu()) {
			init(act->menu());
		}
		else {
			m_actions.insere(std::make_pair(act->text(), act));
		}
	}
}

void MenuEntrerogeable::changement_texte(const QString &texte)
{
	assert(m_menu_auxiliaire);
	m_menu_auxiliaire->hide();
	m_menu_auxiliaire->clear();

	assert(actions().length() > 0);
	auto wa = dynamic_cast<QWidgetAction *>(actions()[0]);
	assert(wa != nullptr);

	auto editeur_ligne = dynamic_cast<QLineEdit *>(wa->defaultWidget());
	assert(editeur_ligne != nullptr);

	for (const auto &a : m_actions) {
		if (correspondance_floue(texte, a.first)) {
			auto act = m_menu_auxiliaire->addAction(a.first);
			connect(act, &QAction::triggered, [a, this]()
			{
				a.second->triggered();
				m_menu_auxiliaire->close();
				close();
			});

			if (m_menu_auxiliaire->actions().length() == 1) {
				m_menu_auxiliaire->setActiveAction(act);
			}
		}
	}

	if (!texte.isEmpty() && !m_menu_auxiliaire->actions().empty()) {
		auto pos = editeur_ligne->mapToGlobal(editeur_ligne->pos());
		pos.setX(pos.x() + editeur_ligne->width());

		m_menu_auxiliaire->popup(pos);
	}

	editeur_ligne->setFocus();
}

void MenuEntrerogeable::showEvent(QShowEvent *event)
{
	if (this->actions().size() > 0) {
		/* Obtiens le QWidgetAction, en présumant qu'il n'y en a qu'un. */
		QWidgetAction *wa = nullptr;
		{
			auto action = actions()[0];
			wa = dynamic_cast<QWidgetAction *>(actions()[0]);

			if (wa == nullptr) {
				wa = new QWidgetAction(this);
				wa->setDefaultWidget(new QLineEdit());

				insertAction(action, wa);
			}
		}
		assert(wa != nullptr);

		/* Obtiens le QLineEdit depuis wa. */
		auto editeur_ligne = dynamic_cast<QLineEdit *>(wa->defaultWidget());
		assert(editeur_ligne != nullptr);

		connect(editeur_ligne, &QLineEdit::textEdited, this, &MenuEntrerogeable::changement_texte);

		/* Prépare l'éditeur de texte et capture le focus pour écouter les
			 * frappes de clavier. */
		editeur_ligne->clear();
		editeur_ligne->setFocus();

		/* Initialisation recursive des actions. */
		m_actions.efface();
		init(this);

		/* Initialisation du menu des items correspondantes. */
		if (m_menu_auxiliaire == nullptr) {
			m_menu_auxiliaire = new QMenu("", this);
			m_menu_auxiliaire->installEventFilter(new FiltreEditeurMenu(editeur_ligne));
		}

		m_menu_auxiliaire->hide();
	}

	QMenu::showEvent(event);
}
