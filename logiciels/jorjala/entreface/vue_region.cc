/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "vue_region.hh"

//#include "editrice_arborescence.hh"
#include "editrice_ligne_temps.h"
#include "editrice_noeud.h"
#include "editrice_proprietes.h"
//#include "editrice_rendu.h"
#include "editrice_attributs.hh"
#include "editrice_vue2d.h"
#include "editrice_vue3d.h"

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires locales.
 * \{ */

static BaseEditrice *qéditrice_depuis_éditrice(JJL::Jorjala &jorjala, JJL::Editrice éditrice)
{
    switch (éditrice.type()) {
        case JJL::TypeEditrice::GRAPHE:
            return new EditriceGraphe(jorjala);
        case JJL::TypeEditrice::PROPRIÉTÉS_NOEUDS:
            return new EditriceProprietes(jorjala);
        case JJL::TypeEditrice::LIGNE_TEMPS:
            return new EditriceLigneTemps(jorjala);
        case JJL::TypeEditrice::RENDU:
            // return new EditriceRendu(jorjala);
            return nullptr;
        case JJL::TypeEditrice::VUE_2D:
            return new EditriceVue2D(jorjala);
        case JJL::TypeEditrice::VUE_3D:
            return new EditriceVue3D(jorjala);
        case JJL::TypeEditrice::ARBORESCENCE:
            // return new EditriceArborescence(m_jorjala);
            return nullptr;
        case JJL::TypeEditrice::ATTRIBUTS:
            return new EditriceAttributs(jorjala);
    }

    return nullptr;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VueRegion
 * \{ */

VueRegion::VueRegion(JJL::Jorjala &jorjala, JJL::RegionInterface région, QWidget *parent)
    : QTabWidget(parent), m_région(région)
{
    for (auto éditrice : m_région.éditrices()) {
        auto qéditrice = qéditrice_depuis_éditrice(jorjala, éditrice);
        if (!qéditrice) {
            continue;
        }

        addTab(qéditrice, éditrice.nom().vers_std_string().c_str());
    }

    connect(this, &QTabWidget::currentChanged, this, &VueRegion::ajourne_pour_changement_page);
}

void VueRegion::ajourne_éditrice_active(JJL::TypeEvenement évènement)
{
    auto éditrice = dynamic_cast<BaseEditrice *>(currentWidget());
    if (!éditrice) {
        return;
    }

    éditrice->ajourne_état(évènement);
}

void VueRegion::ajourne_pour_changement_page(int /*index*/)
{
    ajourne_éditrice_active(JJL::TypeEvenement::RAFRAICHISSEMENT);
}

/** \} */
