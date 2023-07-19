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

#include "biblinternes/outils/iterateurs.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/outils/definitions.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/kanba.h"
#include "coeur/maillage.h"

enum {
    COLONNE_VISIBILITE_CALQUE,
    COLONNE_VERROUILLE_CALQUE,
    COLONNE_NOM_CALQUE,
    COLONNE_PEINTURE_CALQUE,

    NOMBRE_COLONNES,
};

/* ------------------------------------------------------------------------- */
/** \name Boîte à cocher item calque.
 * \{ */

static QString donne_feuille_de_style_boite_à_cocher(QString const &chemin_icone_actif,
                                                     QString const &chemin_icone_inactif)
{
    QString résultat = "QCheckBox::indicator { width: 16px; height: 16px;}";

    if (!chemin_icone_actif.isEmpty()) {
        résultat += "QCheckBox::indicator:checked {image: url(" + chemin_icone_actif + ");}";
    }

    if (!chemin_icone_inactif.isEmpty()) {
        résultat += "QCheckBox::indicator:unchecked {image: url(" + chemin_icone_inactif + ");}";
    }

    return résultat;
}

BoiteACocherItem::BoiteACocherItem(const ArgumentCréationItem &args, int drapeaux, QWidget *parent)
    : QCheckBox(parent), m_kanba(args.kanba), m_calque(args.calque), m_drapeaux(drapeaux)
{
    setChecked((m_calque->drapeaux & m_drapeaux) != 0);

    if (drapeaux == KNB::CALQUE_VISIBLE) {
        this->setStyleSheet(
            donne_feuille_de_style_boite_à_cocher("/home/kevin/icons8-eye-96.png", ""));
    }
    else if (drapeaux == KNB::CALQUE_VERROUILLÉ) {
        this->setStyleSheet(donne_feuille_de_style_boite_à_cocher(
            "/home/kevin/icons8-lock-50.png", "/home/kevin/icons8-unlock-50.png"));
    }

    connect(this, &BoiteACocherItem::stateChanged, this, &BoiteACocherItem::ajourne_etat_calque);
}

void BoiteACocherItem::ajourne_etat_calque(int state)
{
    if (state == Qt::CheckState::Checked) {
        m_calque->drapeaux |= m_drapeaux;
    }
    else {
        m_calque->drapeaux &= ~m_drapeaux;
    }

    auto maillage = m_kanba->donne_maillage();
    maillage->marque_chose_à_recalculer(KNB::ChoseÀRecalculer::CANAL_FUSIONNÉ);
    m_kanba->notifie_observatrices(KNB::TypeÉvènement::DESSIN);
}

/** \} */

/* ************************************************************************** */

class IconItemCalque : public QLabel {
  public:
    IconItemCalque(KNB::Calque *calque, QString const &texte, QWidget *parent = nullptr)
        : QLabel(texte, parent)
    {
        auto pixmap = QPixmap("/home/kevin/icons8-brush-100.png");
        setPixmap(pixmap.scaled(16, 16));
    }

    EMPECHE_COPIE(IconItemCalque);
};

/* ************************************************************************** */

ItemArbreCalque::ItemArbreCalque(const KNB::Calque *calque, QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent), m_calque(calque)
{
    setText(COLONNE_NOM_CALQUE, m_calque->nom.c_str());
}

const KNB::Calque *ItemArbreCalque::pointeur() const
{
    return m_calque;
}

/* ************************************************************************** */

TreeWidget::TreeWidget(QWidget *parent) : QTreeWidget(parent)
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

EditeurCalques::EditeurCalques(KNB::Kanba *kanba, QWidget *parent)
    : BaseEditrice("calque", *kanba, parent), m_widget_arbre(new TreeWidget(this)),
      m_widget(new QWidget()), m_scroll(new QScrollArea()), m_glayout(new QGridLayout(m_widget))
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

    connect(m_widget_arbre, &QTreeWidget::itemPressed, this, &EditeurCalques::repond_selection);
}

EditeurCalques::~EditeurCalques()
{
}

void EditeurCalques::ajourne_état(KNB::TypeÉvènement evenement)
{
    auto maillage = m_kanba->donne_maillage();

    if (maillage == nullptr) {
        return;
    }

    auto dessine_arbre = (evenement == (KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::AJOUTÉ));
    dessine_arbre |= (evenement == (KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::SUPPRIMÉ));
    dessine_arbre |= (evenement == (KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::SÉLECTIONNÉ));
    dessine_arbre |= (evenement == (KNB::TypeÉvènement::PROJET | KNB::TypeÉvènement::CHARGÉ));

    if (dessine_arbre) {
        m_widget_arbre->clear();

        auto const &canaux = maillage->canaux_texture();

        for (auto const calque :
             dls::outils::inverse_iterateur(canaux.calques[KNB::TypeCanal::DIFFUSION])) {
            auto item = new ItemArbreCalque(calque);

            auto args = ArgumentCréationItem{m_kanba, calque};

            auto bouton_visible = new BoiteACocherItem(args, KNB::CALQUE_VISIBLE);
            auto bouton_verrouille = new BoiteACocherItem(args, KNB::CALQUE_VERROUILLÉ);
            IconItemCalque *icone_pinceau = nullptr;
            if ((calque->drapeaux & KNB::CALQUE_ACTIF) != 0) {
                icone_pinceau = new IconItemCalque(calque, "peinture");
            }

            m_widget_arbre->addTopLevelItem(item);
            m_widget_arbre->setItemWidget(item, COLONNE_VISIBILITE_CALQUE, bouton_visible);
            m_widget_arbre->setItemWidget(item, COLONNE_VERROUILLE_CALQUE, bouton_verrouille);

            if ((calque->drapeaux & KNB::CALQUE_ACTIF) != 0) {
                m_widget_arbre->setItemWidget(item, COLONNE_PEINTURE_CALQUE, icone_pinceau);
                item->setSelected(true);
            }
        }
    }

    auto dessine_props = (evenement ==
                          (KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::SÉLECTIONNÉ));
    dessine_props |= (evenement == (KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::SUPPRIMÉ));
    dessine_props |= (evenement == (KNB::TypeÉvènement::PROJET | KNB::TypeÉvènement::CHARGÉ));

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
    m_kanba->donne_repondant_commande()->repond_clique(bouton->text().toStdString(), "");
}

void EditeurCalques::repond_selection(QTreeWidgetItem *item, int column)
{
    if (column != COLONNE_NOM_CALQUE) {
        return;
    }

    auto item_calque = dynamic_cast<ItemArbreCalque *>(item);
    if (!item_calque) {
        return;
    }

    auto calque = const_cast<KNB::Calque *>(item_calque->pointeur());

    auto maillage = m_kanba->donne_maillage();
    if (calque == maillage->calque_actif()) {
        return;
    }

    maillage->calque_actif(calque);
    m_kanba->notifie_observatrices(KNB::TypeÉvènement::CALQUE | KNB::TypeÉvènement::SÉLECTIONNÉ);
}
