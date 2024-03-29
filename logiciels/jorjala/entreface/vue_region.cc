/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "vue_region.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QMenu>
#include <QPushButton>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

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

static BaseEditrice *qéditrice_depuis_éditrice(JJL::Jorjala &jorjala, JJL::Éditrice éditrice)
{
    switch (éditrice.donne_type()) {
        case JJL::TypeÉditrice::GRAPHE:
            return new EditriceGraphe(jorjala, éditrice);
        case JJL::TypeÉditrice::PROPRIÉTÉS_NOEUDS:
            return new EditriceProprietes(jorjala, éditrice);
        case JJL::TypeÉditrice::LIGNE_TEMPS:
            return new EditriceLigneTemps(jorjala, éditrice);
        case JJL::TypeÉditrice::RENDU:
            // return new EditriceRendu(jorjala);
            return nullptr;
        case JJL::TypeÉditrice::VUE_2D:
            return new EditriceVue2D(jorjala, éditrice);
        case JJL::TypeÉditrice::VUE_3D:
            return new EditriceVue3D(jorjala, éditrice);
        case JJL::TypeÉditrice::ARBORESCENCE:
            // return new EditriceArborescence(m_jorjala);
            return nullptr;
        case JJL::TypeÉditrice::ATTRIBUTS:
            return new EditriceAttributs(jorjala, éditrice);
    }

    return nullptr;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ActionAjoutEditrice.
 * \{ */

ActionAjoutEditrice::ActionAjoutEditrice(QString texte, QObject *parent) : QAction(texte, parent)
{
    connect(this, &QAction::triggered, this, &ActionAjoutEditrice::sur_declenchage);
}

void ActionAjoutEditrice::sur_declenchage()
{
    Q_EMIT(ajoute_editrice(this->data().toInt()));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VueRegion
 * \{ */

VueRegion::VueRegion(JJL::Jorjala &jorjala, JJL::RégionInterface région, QWidget *parent)
    : QTabWidget(parent), m_jorjala(jorjala), m_région(région),
      m_bouton_affichage_liste(new QPushButton("Ajouter Éditrice", this)),
      m_menu_liste_éditrices(new QMenu(this))
{
    for (auto éditrice : m_région.donne_éditrices()) {
        ajoute_page_pour_éditrice(éditrice, false);
    }

    m_région.définis_éditrice_visible(0);

    connect(m_bouton_affichage_liste, &QPushButton::clicked, this, &VueRegion::montre_liste);

    setTabsClosable(true);
    setCornerWidget(m_bouton_affichage_liste);

    connect(this, &QTabWidget::currentChanged, this, &VueRegion::ajourne_pour_changement_page);
    connect(this, &QTabWidget::tabCloseRequested, this, &VueRegion::sur_fermeture_page);

    /* Construit la liste d'éditrices. */
    ActionAjoutEditrice *action;

#define AJOUTE_ACTION(type_jorjala)                                                               \
    action = new ActionAjoutEditrice(                                                             \
        JJL::nom_pour_type_éditrice(type_jorjala).vers_std_string().c_str(), this);               \
    m_menu_liste_éditrices->addAction(action);                                                    \
    action->setData(QVariant(int(type_jorjala)));                                                 \
    connect(action, &ActionAjoutEditrice::ajoute_editrice, this, &VueRegion::sur_ajout_editrice);

    // AJOUTE_ACTION(JJL::TypeÉditrice::ARBORESCENCE)
    AJOUTE_ACTION(JJL::TypeÉditrice::ATTRIBUTS)
    AJOUTE_ACTION(JJL::TypeÉditrice::GRAPHE)
    AJOUTE_ACTION(JJL::TypeÉditrice::LIGNE_TEMPS)
    AJOUTE_ACTION(JJL::TypeÉditrice::PROPRIÉTÉS_NOEUDS)
    // AJOUTE_ACTION(JJL::TypeÉditrice::RENDU)
    AJOUTE_ACTION(JJL::TypeÉditrice::VUE_2D)
    AJOUTE_ACTION(JJL::TypeÉditrice::VUE_3D)

#undef AJOUTE_ACTION
}

void VueRegion::ajourne_éditrice_active(JJL::ChangementÉditrice changement)
{
    auto éditrice = dynamic_cast<BaseEditrice *>(currentWidget());
    if (!éditrice) {
        return;
    }

    éditrice->ajourne_état(changement);
}

void VueRegion::ajoute_page_pour_éditrice(JJL::Éditrice éditrice, bool définis_comme_page_courante)
{
    auto qéditrice = qéditrice_depuis_éditrice(m_jorjala, éditrice);
    if (!qéditrice) {
        return;
    }

    addTab(qéditrice, éditrice.donne_nom().vers_std_string().c_str());

    if (définis_comme_page_courante) {
        m_région.définis_éditrice_visible(count() - 1);
        setCurrentIndex(count() - 1);
    }
}

void VueRegion::ajourne_pour_changement_page(int index)
{
    m_région.définis_éditrice_visible(index);
    ajourne_éditrice_active(JJL::ChangementÉditrice::RAFRAICHIS);
}

void VueRegion::sur_fermeture_page(int index)
{
    auto éditrice = dynamic_cast<BaseEditrice *>(widget(index));
    if (!éditrice) {
        return;
    }

    m_région.supprime_éditrice_à_l_index(index);
    removeTab(index);
}

void VueRegion::montre_liste()
{
    /* La liste est positionnée en dessous du bouton, alignée à sa gauche. */
    const auto &rect = m_bouton_affichage_liste->geometry();
    const auto &bas_gauche = m_bouton_affichage_liste->parentWidget()->mapToGlobal(
        rect.bottomLeft());

    m_menu_liste_éditrices->popup(bas_gauche);
}

void VueRegion::sur_ajout_editrice(int type)
{
    auto type_éditrice = static_cast<JJL::TypeÉditrice>(type);
    auto éditrice = m_région.ajoute_une_éditrice(m_jorjala, type_éditrice);
    ajoute_page_pour_éditrice(éditrice, true);
}

/** \} */
