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

#pragma once

#include "base_editeur.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QCheckBox>
#include <QTreeWidget>
#pragma GCC diagnostic pop

namespace KNB {
class Calque;
enum class DrapeauxCalque : uint32_t;
}  // namespace KNB

class QGridLayout;
class QScrollArea;

struct ArgumentCréationItem {
    KNB::Kanba &kanba;
    KNB::Calque &calque;
};

/* ------------------------------------------------------------------------- */
/** \name Boîte à cocher item calque.
 * \{ */

class BoiteACocherItem : public QCheckBox {
    Q_OBJECT

    KNB::Kanba &m_kanba;
    KNB::Calque &m_calque;
    KNB::DrapeauxCalque m_drapeaux;

  public:
    explicit BoiteACocherItem(const ArgumentCréationItem &args,
                              KNB::DrapeauxCalque drapeaux,
                              QWidget *parent = nullptr);

    EMPECHE_COPIE(BoiteACocherItem);

  private Q_SLOTS:
    void ajourne_etat_calque(int state);
};

/** \} */

/* ************************************************************************** */

class ItemArbreCalque : public QTreeWidgetItem {
    const KNB::Calque &m_calque;

  public:
    explicit ItemArbreCalque(const KNB::Calque &calque, QTreeWidgetItem *parent = nullptr);

    ItemArbreCalque(ItemArbreCalque const &) = default;
    ItemArbreCalque &operator=(ItemArbreCalque const &) = default;

    const KNB::Calque &pointeur() const;
};

/* ************************************************************************** */

class TreeWidget : public QTreeWidget {
    BaseEditrice *m_base = nullptr;

  public:
    explicit TreeWidget(QWidget *parent = nullptr);

    EMPECHE_COPIE(TreeWidget);

    void set_base(BaseEditrice *base);

    void mousePressEvent(QMouseEvent *e) override;
};

/* ************************************************************************** */

class EditeurCalques final : public BaseEditrice {
    Q_OBJECT

    TreeWidget *m_widget_arbre;

    QWidget *m_widget;
    QScrollArea *m_scroll;
    QGridLayout *m_glayout;

  public:
    EditeurCalques(KNB::Kanba &kanba, QWidget *parent = nullptr);

    EMPECHE_COPIE(EditeurCalques);

    ~EditeurCalques() override;

    void ajourne_état(KNB::TypeÉvènement evenement) override;

    void ajourne_manipulable() override
    {
    }

  private Q_SLOTS:
    void ajourne_vue();
    void repond_bouton();
    void repond_selection(QTreeWidgetItem *item, int column);
};
