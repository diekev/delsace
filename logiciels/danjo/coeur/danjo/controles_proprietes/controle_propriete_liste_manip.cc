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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "controle_propriete_liste_manip.hh"

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include "commun.hh"
#include "donnees_controle.h"
#include "types/liste_manip.hh"

namespace danjo {

/* ************************************************************************** */

ItemArbreManip::ItemArbreManip(const Manipulable *manip, QTreeWidgetItem *parent)
    : QTreeWidgetItem(parent), m_manipulable(manip)
{
    setText(0, "manip");
}

const Manipulable *ItemArbreManip::pointeur() const
{
    return m_manipulable;
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

/* ************************************************************************** */

ControleProprieteListeManip::ControleProprieteListeManip(BasePropriete *p,
                                                         int temps,
                                                         QWidget *parent)
    : ControlePropriete(p, temps, parent), m_disp_horiz(crée_hbox_layout(this)),
      m_disp_boutons(new QVBoxLayout()), m_bouton_ajoute(new QPushButton("ajoute", this)),
      m_bouton_enleve(new QPushButton("enlève", this)),
      m_bouton_monte(new QPushButton("monte", this)),
      m_bouton_descend(new QPushButton("descend", this)), m_widget_arbre(new TreeWidget(this))
{
    m_disp_boutons->addWidget(m_bouton_ajoute);
    m_disp_boutons->addWidget(m_bouton_enleve);
    m_disp_boutons->addWidget(m_bouton_monte);
    m_disp_boutons->addWidget(m_bouton_descend);
    m_disp_horiz->addWidget(m_widget_arbre);
    m_disp_horiz->addLayout(m_disp_boutons);

    connect(m_bouton_ajoute,
            &QPushButton::clicked,
            this,
            &ControleProprieteListeManip::ajoute_manipulable);
    connect(m_bouton_enleve,
            &QPushButton::clicked,
            this,
            &ControleProprieteListeManip::enleve_manipulable);
    connect(m_bouton_monte,
            &QPushButton::clicked,
            this,
            &ControleProprieteListeManip::monte_manipulable);
    connect(m_bouton_descend,
            &QPushButton::clicked,
            this,
            &ControleProprieteListeManip::descend_manipulable);

    connect(m_widget_arbre,
            &QTreeWidget::itemSelectionChanged,
            this,
            &ControleProprieteListeManip::repond_selection);

    setLayout(m_disp_horiz);
}

void ControleProprieteListeManip::finalise(const DonneesControle &donnees)
{
    m_pointeur = static_cast<ListeManipulable *>(donnees.pointeur);
    init_arbre();
}

void ControleProprieteListeManip::init_arbre()
{
    m_widget_arbre->clear();

    for (auto &manip : m_pointeur->manipulables) {
        auto item = new ItemArbreManip(&manip);

        if (&manip == m_manipulable_courant) {
            item->setSelected(true);
        }

        m_widget_arbre->addTopLevelItem(item);
    }

    update();
}

void ControleProprieteListeManip::ajoute_manipulable()
{
    std::cerr << __func__ << '\n';
    émets_controle_changé_simple([this]() {
        m_pointeur->manipulables.ajoute(Manipulable());
        m_manipulable_courant = &m_pointeur->manipulables.back();
        m_index_courant = m_pointeur->manipulables.taille() - 1;

        init_arbre();
    });
}

void ControleProprieteListeManip::enleve_manipulable()
{
    std::cerr << __func__ << '\n';
    émets_controle_changé_simple([this]() {
        auto index = m_index_courant;

        if (m_pointeur->manipulables.taille() == 0) {
            return;
        }

        m_pointeur->manipulables.erase(m_pointeur->manipulables.debut() + index);

        if (index >= m_pointeur->manipulables.taille()) {
            m_index_courant = m_pointeur->manipulables.taille() - 1;
        }

        if (m_pointeur->manipulables.est_vide()) {
            m_manipulable_courant = nullptr;
        }
        else {
            m_manipulable_courant = &m_pointeur->manipulables[m_index_courant];
        }

        init_arbre();
    });
}

void ControleProprieteListeManip::monte_manipulable()
{
    std::cerr << __func__ << '\n';
    init_arbre();
}

void ControleProprieteListeManip::descend_manipulable()
{
    std::cerr << __func__ << '\n';
    init_arbre();
}

void ControleProprieteListeManip::repond_selection()
{
    std::cerr << __func__ << '\n';
    auto items = m_widget_arbre->selectedItems();

    if (items.size() != 1) {
        return;
    }

    auto item = items[0];

    auto item_manip = dynamic_cast<ItemArbreManip *>(item);

    if (!item_manip) {
        return;
    }

    auto index = 0;

    for (auto &manip : m_pointeur->manipulables) {
        if (&manip == item_manip->pointeur()) {
            m_manipulable_courant = &manip;
            m_index_courant = index;
            break;
        }

        ++index;
    }
}

} /* namespace danjo */
