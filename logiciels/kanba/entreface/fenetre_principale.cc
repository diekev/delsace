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

#include "fenetre_principale.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/kanba.h"

#include "evenement_kanba.hh"
#include "gestionnaire_interface.hh"
#include "vue_region.hh"

FenetrePrincipale::FenetrePrincipale(KNB::Kanba &kanba, QWidget *parent)
    : QMainWindow(parent), m_kanba(kanba)
{
    KNB::enregistre_commandes(m_kanba);
    m_kanba.définis_gestionnaire_fenêtre(new GestionnaireInterface(*this));

    m_progress_bar = new QProgressBar(this);
    statusBar()->addWidget(m_progress_bar);
    m_progress_bar->setRange(0, 100);
    m_progress_bar->setVisible(false);

    construit_interface_depuis_kanba();

    auto menu = menuBar()->addMenu("Fichier");
    auto action = menu->addAction("Ouvrir projet");
    action->setData("ouvrir_projet");

    connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

    action = menu->addAction("Sauvegarder projet");
    action->setData("sauvegarder_projet");

    menu->addSeparator();

    connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

    action = menu->addAction("Ouvrir fichier");
    action->setData("ouvrir_fichier");

    connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

    menu = menuBar()->addMenu("Édition");
    action = menu->addAction("Ajouter cube");
    action->setData("ajouter_cube");

    connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

    action = menu->addAction("Ajouter sphere");
    action->setData("ajouter_sphere");

    connect(action, SIGNAL(triggered(bool)), this, SLOT(repond_action()));

    /* Afin d'utiliser eventFilter pour filtrer les évènements de Kanba. */
    qApp->installEventFilter(this);
}

static QLayout *qlayout_depuis_disposition(KNB::Disposition &disposition)
{
    switch (disposition.donne_direction()) {
        case KNB::DirectionDisposition::HORIZONTAL:
        {
            return new QHBoxLayout();
        }
        case KNB::DirectionDisposition::VERTICAL:
        {
            return new QVBoxLayout();
        }
    }
    return nullptr;
}

static Qt::Orientation donne_orientation_qsplitter_disposition(KNB::Disposition &disposition)
{
    switch (disposition.donne_direction()) {
        case KNB::DirectionDisposition::HORIZONTAL:
        {
            return Qt::Horizontal;
        }
        case KNB::DirectionDisposition::VERTICAL:
        {
            return Qt::Vertical;
        }
    }
    return Qt::Vertical;
}

static QWidget *génère_interface_disposition(KNB::Kanba &kanba,
                                             KNB::Disposition région,
                                             QVector<VueRegion *> &régions);

static void génère_interface_région(KNB::Kanba &kanba,
                                    KNB::RégionInterface &région,
                                    QSplitter *layout,
                                    QVector<VueRegion *> &régions)
{
    auto qwidget_région = new QWidget();
    layout->addWidget(qwidget_région);

    auto qwidget_région_layout = new QVBoxLayout();
    qwidget_région_layout->setMargin(0);
    qwidget_région->setLayout(qwidget_région_layout);

    if (région.donne_type() == KNB::TypeRégion::CONTENEUR_ÉDITRICE) {
        auto vue_région = new VueRegion(kanba, région, qwidget_région);
        régions.append(vue_région);
        qwidget_région_layout->addWidget(vue_région);
    }
    else {
        auto widget = génère_interface_disposition(kanba, région.donne_disposition(), régions);
        qwidget_région_layout->addWidget(widget);
    }
}

static QWidget *génère_interface_disposition(KNB::Kanba &kanba,
                                             KNB::Disposition disposition,
                                             QVector<VueRegion *> &régions)
{
    auto qsplitter = new QSplitter();
    qsplitter->setOrientation(donne_orientation_qsplitter_disposition(disposition));

    for (auto région : disposition.donne_régions()) {
        génère_interface_région(kanba, région, qsplitter, régions);
    }

    auto qlayout = qlayout_depuis_disposition(disposition);
    qlayout->setMargin(0);
    qlayout->addWidget(qsplitter);

    auto qwidget = new QWidget();
    qwidget->setLayout(qlayout);

    return qwidget;
}

void FenetrePrincipale::construit_interface_depuis_kanba()
{
    auto interface = m_kanba.donne_interface_graphique();
    m_régions.clear();

    auto disposition = interface.donne_disposition();

    auto qwidget = génère_interface_disposition(m_kanba, disposition, m_régions);
    setCentralWidget(qwidget);

    m_kanba.notifie_observatrices(KNB::TypeÉvènement::RAFRAICHISSEMENT);
}

bool FenetrePrincipale::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() != EvenementKanba::id_type_qt) {
        return QWidget::eventFilter(object, event);
    }

    auto event_kanba = static_cast<EvenementKanba *>(event);

    if (event_kanba->pour_quoi() == (KNB::TypeÉvènement::PROJET | KNB::TypeÉvènement::CHARGÉ)) {
        construit_interface_depuis_kanba();
        return true;
    }

    for (auto région : m_régions) {
        région->ajourne_éditrice_active(event_kanba->pour_quoi());
    }

    return true;
}

void FenetrePrincipale::tache_commence()
{
    m_progress_bar->setValue(0);
    m_progress_bar->setVisible(true);
}

void FenetrePrincipale::progres_avance(float progress)
{
    m_progress_bar->setValue(static_cast<int>(progress));
}

void FenetrePrincipale::progres_temps(int echantillon,
                                      float temps_echantillon,
                                      float temps_ecoule,
                                      float temps_restant)
{
    //	auto moteur_rendu = m_kanba.moteur_rendu;
    //	moteur_rendu->notifie_observatrices(KNB::type_evenement::rafraichissement);
}

void FenetrePrincipale::tache_fini()
{
    m_progress_bar->setVisible(false);
}

void FenetrePrincipale::repond_action()
{
    auto action = qobject_cast<QAction *>(sender());

    if (!action) {
        return;
    }

    KNB::donne_repondant_commande()->repond_clique(action->data().toString().toStdString(), "");
}
