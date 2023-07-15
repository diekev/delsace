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

#include "base_editeur.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QVariant>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/kanba.h"

BaseEditrice::BaseEditrice(const char *identifiant, Kanba &kanba, QWidget *parent)
    : danjo::ConteneurControles(parent), m_kanba(&kanba), m_cadre(new QFrame(this)),
      m_agencement(new QVBoxLayout()), m_identifiant(identifiant)
{
    this->observe(&kanba);

    QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    size_policy.setHorizontalStretch(0);
    size_policy.setVerticalStretch(0);
    size_policy.setHeightForWidth(m_cadre->sizePolicy().hasHeightForWidth());

    /* Intern frame, where individual entreface regions put their buttons. */

    m_cadre->setSizePolicy(size_policy);
    m_cadre->setFrameShape(QFrame::StyledPanel);
    m_cadre->setFrameShadow(QFrame::Raised);

    m_agencement->addWidget(m_cadre);

    m_agencement->setMargin(0);
    this->setLayout(m_agencement);

    m_agencement_principal = new QHBoxLayout(m_cadre);
    m_agencement_principal->setMargin(6);

    this->actif(false);
}

void BaseEditrice::actif(bool yesno)
{
    m_cadre->setProperty("state", (yesno) ? "on" : "off");
    m_cadre->setStyle(QApplication::style());
}

void BaseEditrice::rend_actif()
{
    if (m_kanba->widget_actif) {
        m_kanba->widget_actif->actif(false);
    }

    m_kanba->widget_actif = this;
    this->actif(true);
}

void BaseEditrice::mousePressEvent(QMouseEvent *e)
{
    this->rend_actif();
    QWidget::mousePressEvent(e);

    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(e->pos().x());
    donnees.y = static_cast<float>(e->pos().y());
    donnees.souris = e->button();
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    m_kanba->repondant_commande->appele_commande(m_identifiant, donnees);
}

void BaseEditrice::mouseMoveEvent(QMouseEvent *e)
{
    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(e->pos().x());
    donnees.y = static_cast<float>(e->pos().y());
    donnees.souris = e->button();

    m_kanba->repondant_commande->ajourne_commande_modale(donnees);
}

void BaseEditrice::wheelEvent(QWheelEvent *e)
{
    /* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
     * roulement de la molette de la souris, on prétend que le roulement est un
     * double clique de la molette. */
    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(e->delta());
    donnees.souris = Qt::MidButton;
    donnees.double_clique = true;

    m_kanba->repondant_commande->appele_commande(m_identifiant, donnees);
}

void BaseEditrice::mouseReleaseEvent(QMouseEvent *e)
{
    DonneesCommande donnees;
    donnees.x = static_cast<float>(e->pos().x());
    donnees.y = static_cast<float>(e->pos().y());

    m_kanba->repondant_commande->acheve_commande_modale(donnees);
}
