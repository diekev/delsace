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

#include "editeur_brosse.h"
#include "editeur_calques.h"
#include "editeur_canevas.h"
#include "editeur_parametres.h"

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires locales.
 * \{ */

static BaseEditrice *qéditrice_depuis_éditrice(KNB::Kanba &kanba, KNB::Éditrice &éditrice)
{
    switch (éditrice.donne_type()) {
        case KNB::TypeÉditrice::VUE_2D:
            return new EditriceCannevas2D(kanba);
        case KNB::TypeÉditrice::VUE_3D:
            return new EditriceCannevas3D(kanba);
        case KNB::TypeÉditrice::PARAMÈTRES_BROSSE:
            return new EditeurBrosse(kanba);
        case KNB::TypeÉditrice::CALQUES:
            return new EditeurCalques(kanba);
        case KNB::TypeÉditrice::INFORMATIONS:
            return nullptr;
        case KNB::TypeÉditrice::PARAMÈTRES_GLOBAUX:
            return new EditeurParametres(kanba);
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

VueRegion::VueRegion(KNB::Kanba &kanba, KNB::RégionInterface &région, QWidget *parent)
    : QTabWidget(parent), m_kanba(kanba), m_région(région),
      m_bouton_affichage_liste(new QPushButton("Ajouter Éditrice", this)),
      m_menu_liste_éditrices(new QMenu(this))
{
    for (auto éditrice : m_région.donne_éditrices()) {
        ajoute_page_pour_éditrice(éditrice, false);
    }

    connect(m_bouton_affichage_liste, &QPushButton::clicked, this, &VueRegion::montre_liste);

    setTabsClosable(true);
    setCornerWidget(m_bouton_affichage_liste);

    connect(this, &QTabWidget::currentChanged, this, &VueRegion::ajourne_pour_changement_page);
    connect(this, &QTabWidget::tabCloseRequested, this, &VueRegion::sur_fermeture_page);

    /* Construit la liste d'éditrices. */
    ActionAjoutEditrice *action;

#define AJOUTE_ACTION(type_kanba)                                                                 \
    action = new ActionAjoutEditrice(                                                             \
        KNB::nom_pour_type_éditrice(type_kanba).vers_std_string().c_str(), this);                 \
    m_menu_liste_éditrices->addAction(action);                                                    \
    action->setData(QVariant(int(type_kanba)));                                                   \
    connect(action, &ActionAjoutEditrice::ajoute_editrice, this, &VueRegion::sur_ajout_editrice);

    AJOUTE_ACTION(KNB::TypeÉditrice::VUE_2D)
    AJOUTE_ACTION(KNB::TypeÉditrice::VUE_3D)
    AJOUTE_ACTION(KNB::TypeÉditrice::PARAMÈTRES_BROSSE)
    AJOUTE_ACTION(KNB::TypeÉditrice::CALQUES)
    AJOUTE_ACTION(KNB::TypeÉditrice::INFORMATIONS)

#undef AJOUTE_ACTION
}

void VueRegion::ajourne_éditrice_active(KNB::TypeÉvènement évènement)
{
    auto éditrice = dynamic_cast<BaseEditrice *>(currentWidget());
    if (!éditrice) {
        return;
    }

    éditrice->ajourne_état(évènement);
}

void VueRegion::ajoute_page_pour_éditrice(KNB::Éditrice &éditrice,
                                          bool définit_comme_page_courante)
{
    auto qéditrice = qéditrice_depuis_éditrice(m_kanba, éditrice);
    if (!qéditrice) {
        return;
    }

    addTab(qéditrice, éditrice.donne_nom().vers_std_string().c_str());

    if (définit_comme_page_courante) {
        setCurrentIndex(count() - 1);
    }
}

void VueRegion::ajourne_pour_changement_page(int /*index*/)
{
    ajourne_éditrice_active(KNB::TypeÉvènement::RAFRAICHISSEMENT);
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
    auto type_éditrice = static_cast<KNB::TypeÉditrice>(type);
    auto éditrice = m_région.ajoute_une_éditrice(type_éditrice);
    ajoute_page_pour_éditrice(éditrice, true);
}

/** \} */
