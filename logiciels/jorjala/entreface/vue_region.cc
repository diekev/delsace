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

VueRegion::VueRegion(JJL::Jorjala &jorjala, JJL::RegionInterface région, QWidget *parent)
    : QTabWidget(parent), m_jorjala(jorjala), m_région(région),
      m_bouton_affichage_liste(new QPushButton("Ajouter Éditrice", this)),
      m_menu_liste_éditrices(new QMenu(this))
{
    for (auto éditrice : m_région.éditrices()) {
        ajoute_page_pour_éditrice(éditrice, false);
    }

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

    // AJOUTE_ACTION(JJL::TypeEditrice::ARBORESCENCE)
    AJOUTE_ACTION(JJL::TypeEditrice::ATTRIBUTS)
    AJOUTE_ACTION(JJL::TypeEditrice::GRAPHE)
    AJOUTE_ACTION(JJL::TypeEditrice::LIGNE_TEMPS)
    AJOUTE_ACTION(JJL::TypeEditrice::PROPRIÉTÉS_NOEUDS)
    // AJOUTE_ACTION(JJL::TypeEditrice::RENDU)
    AJOUTE_ACTION(JJL::TypeEditrice::VUE_2D)
    AJOUTE_ACTION(JJL::TypeEditrice::VUE_3D)

#undef AJOUTE_ACTION
}

void VueRegion::ajourne_éditrice_active(JJL::TypeEvenement évènement)
{
    auto éditrice = dynamic_cast<BaseEditrice *>(currentWidget());
    if (!éditrice) {
        return;
    }

    éditrice->ajourne_état(évènement);
}

void VueRegion::ajoute_page_pour_éditrice(JJL::Editrice éditrice, bool définit_comme_page_courante)
{
    auto qéditrice = qéditrice_depuis_éditrice(m_jorjala, éditrice);
    if (!qéditrice) {
        return;
    }

    addTab(qéditrice, éditrice.nom().vers_std_string().c_str());

    if (définit_comme_page_courante) {
        setCurrentIndex(count() - 1);
    }
}

void VueRegion::ajourne_pour_changement_page(int /*index*/)
{
    ajourne_éditrice_active(JJL::TypeEvenement::RAFRAICHISSEMENT);
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
    auto type_éditrice = static_cast<JJL::TypeEditrice>(type);
    auto éditrice = m_région.ajoute_une_éditrice(type_éditrice);
    ajoute_page_pour_éditrice(éditrice, true);
}

/** \} */
