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

#include "base_editrice.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QKeyEvent>
#include <QFrame>
#include <QHBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "gestion_entreface.hh"

#include "coeur/jorjala.hh"

BaseEditrice::BaseEditrice(const char *identifiant_, JJL::Jorjala &jorjala, QWidget *parent)
	: danjo::ConteneurControles(parent)
	, m_jorjala(jorjala)
    , m_frame(new QFrame(this))
    , m_layout(new QVBoxLayout())
	, m_main_layout(new QHBoxLayout(m_frame))
    , identifiant(identifiant_)
{
    /* Mise en place de l'observation. */
    ajoute_observatrice(m_jorjala, this);

	QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	size_policy.setHorizontalStretch(0);
	size_policy.setVerticalStretch(0);
	size_policy.setHeightForWidth(m_frame->sizePolicy().hasHeightForWidth());

	/* Intern frame, where individual entreface regions put their buttons. */

	m_frame->setSizePolicy(size_policy);
	m_frame->setFrameShape(QFrame::StyledPanel);
	m_frame->setFrameShadow(QFrame::Raised);

	m_layout->addWidget(m_frame);

	m_layout->setMargin(0);
	this->setLayout(m_layout);

	m_main_layout->setMargin(0);

	this->actif(false);
}

void BaseEditrice::actif(bool ouinon)
{
	m_frame->setProperty("state", (ouinon) ? "on" : "off");
	m_frame->setStyle(QApplication::style());
}

void BaseEditrice::rend_actif()
{
    active_editrice(m_jorjala, this);
	this->actif(true);
}

/* ------------------------------------------------------------------------- */

static DonneesCommande donnees_commande_depuis_event(QMouseEvent *e, QPointF position)
{
    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(position.x());
    donnees.y = static_cast<float>(position.y());
    donnees.souris = static_cast<int>(e->buttons());
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
    return donnees;
}

void BaseEditrice::mousePressEvent(QMouseEvent *event)
{
    rend_actif();

    /* À FAIRE : menu contextuel */
    //			m_gestionnaire->ajourne_menu("Éditeur Noeud");
    //			m_menu_contexte->popup(event->globalPos());

    auto const position = transforme_position_evenement(event->pos());
    auto donnees = donnees_commande_depuis_event(event, position);
    repondant_commande(m_jorjala)->appele_commande(this->identifiant, donnees);
}

void BaseEditrice::keyPressEvent(QKeyEvent *event)
{
    rend_actif();
    DonneesCommande donnees;
    donnees.cle = event->key();
    repondant_commande(m_jorjala)->appele_commande(this->identifiant, donnees);
}

void BaseEditrice::wheelEvent(QWheelEvent *event)
{
    /* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
     * roulement de la molette de la souris, on prétend que le roulement est un
     * double clique de la molette. */
    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(event->angleDelta().x());
    donnees.y = static_cast<float>(event->angleDelta().y());
    donnees.souris = Qt::MiddleButton;
    donnees.double_clique = true;
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(m_jorjala)->appele_commande(this->identifiant, donnees);
}

void BaseEditrice::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() != 0) {
        rend_actif();
    }

    auto const position = transforme_position_evenement(event->pos());
    auto donnees = donnees_commande_depuis_event(event, position);
    repondant_commande(m_jorjala)->ajourne_commande_modale(donnees);
}

void BaseEditrice::mouseDoubleClickEvent(QMouseEvent *event)
{
    rend_actif();

    auto const position = transforme_position_evenement(event->pos());
    auto donnees = donnees_commande_depuis_event(event, position);
    donnees.double_clique = true;
    repondant_commande(m_jorjala)->appele_commande(this->identifiant, donnees);
}

void BaseEditrice::mouseReleaseEvent(QMouseEvent *event)
{
    rend_actif();

    auto const position = transforme_position_evenement(event->pos());
    auto donnees = donnees_commande_depuis_event(event, position);
    repondant_commande(m_jorjala)->acheve_commande_modale(donnees);
}

QPointF BaseEditrice::transforme_position_evenement(QPoint pos)
{
    return QPointF(pos.x(), pos.y());
}
