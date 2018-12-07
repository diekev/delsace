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

#include "editeur_calques.h"

#include <numero7/outils/iterateurs.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "bibliotheques/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/kanba.h"
#include "coeur/maillage.h"

enum {
	COLONNE_VISIBILITE_CALQUE,
	COLONNE_NOM_CALQUE,
	COLONNE_PEINTURE_CALQUE,
	COLONNE_VERROUILLE_CALQUE,

	NOMBRE_COLONNES,
};

/* ************************************************************************** */

class BoutonItemCalque : public QPushButton {
	Calque *m_calque;

public:
	BoutonItemCalque(Calque *calque, const QString &texte, QWidget *parent = nullptr)
		: QPushButton(texte, parent)
		, m_calque(calque)
	{}

	BoutonItemCalque(BoutonItemCalque const &) = default;
	BoutonItemCalque &operator=(BoutonItemCalque const &) = default;
};

/* ************************************************************************** */

ItemArbreCalque::ItemArbreCalque(const Calque *calque, QTreeWidgetItem *parent)
	: QTreeWidgetItem(parent)
	, m_calque(calque)
{
	setText(COLONNE_NOM_CALQUE, m_calque->nom.c_str());
}

const Calque *ItemArbreCalque::pointeur() const
{
	return m_calque;
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
	setFocusPolicy(Qt::NoFocus);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setHeaderHidden(true);
	setDragDropMode(NoDragDrop);
	setDragEnabled(false);
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

/* ************************************************************************** */

EditeurCalques::EditeurCalques(Kanba *kanba, QWidget *parent)
	: BaseEditrice(*kanba, parent)
	, m_widget_arbre(new TreeWidget(this))
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

	m_widget_arbre->setColumnCount(NOMBRE_COLONNES);
	m_widget_arbre->set_base(this);
	m_glayout->addWidget(m_widget_arbre);

	auto bouton_ajout_calque = new QPushButton("ajouter_calque");
	connect(bouton_ajout_calque, SIGNAL(clicked()), this, SLOT(repond_bouton()));
	m_glayout->addWidget(bouton_ajout_calque);

	auto bouton_supprimer_calque = new QPushButton("supprimer_calque");
	connect(bouton_supprimer_calque, SIGNAL(clicked()), this, SLOT(repond_bouton()));
	m_glayout->addWidget(bouton_supprimer_calque);

	connect(m_widget_arbre, SIGNAL(itemSelectionChanged()),
			this, SLOT(repond_selection()));
}

EditeurCalques::~EditeurCalques()
{

}

void EditeurCalques::ajourne_etat(int evenement)
{
	auto maillage = m_kanba->maillage;

	if (maillage == nullptr) {
		return;
	}

	auto dessine_arbre = (evenement == (type_evenement::calque | type_evenement::ajoute));
	dessine_arbre |= (evenement == (type_evenement::calque | type_evenement::supprime));
	dessine_arbre |= (evenement == (type_evenement::projet | type_evenement::charge));

	if (dessine_arbre) {
		m_widget_arbre->clear();

		const auto &canaux = maillage->canaux_texture();

		for (const auto calque : numero7::outils::inverse_iterateur(canaux.calques[TypeCanal::DIFFUSION])) {
			auto item = new ItemArbreCalque(calque);
			auto bouton_visible = new BoutonItemCalque(calque, "visible");
			auto bouton_peinture = new BoutonItemCalque(calque, "peinture");
			auto bouton_verrouille = new BoutonItemCalque(calque, "verrouille");

			m_widget_arbre->addTopLevelItem(item);
			m_widget_arbre->setItemWidget(item, COLONNE_VISIBILITE_CALQUE, bouton_visible);
			m_widget_arbre->setItemWidget(item, COLONNE_PEINTURE_CALQUE, bouton_peinture);
			m_widget_arbre->setItemWidget(item, COLONNE_VERROUILLE_CALQUE, bouton_verrouille);

			if ((calque->drapeaux & CALQUE_ACTIF) != 0) {
				item->setSelected(true);
			}
		}
	}

	auto dessine_props = (evenement == (type_evenement::calque | type_evenement::selection));
	dessine_props |= (evenement == (type_evenement::calque | type_evenement::supprime));
	dessine_props |= (evenement == (type_evenement::projet | type_evenement::charge));

	if (dessine_props) {
		/* À FAIRE : dessine les propriétés des calques. */
	}
}

void EditeurCalques::ajourne_vue()
{

}

void EditeurCalques::repond_bouton()
{
	auto bouton = qobject_cast<QPushButton *>(sender());
	m_kanba->repondant_commande->repond_clique(bouton->text().toStdString(), "");
}

void EditeurCalques::repond_selection()
{
	auto items = m_widget_arbre->selectedItems();

	if (items.size() != 1) {
		return;
	}

	auto item = items[0];

	auto item_calque = dynamic_cast<ItemArbreCalque *>(item);

	if (!item_calque) {
		return;
	}

	auto maillage = m_kanba->maillage;
	maillage->calque_actif(const_cast<Calque *>(item_calque->pointeur()));
	m_kanba->notifie_auditeurs(type_evenement::calque | type_evenement::selection);
}
