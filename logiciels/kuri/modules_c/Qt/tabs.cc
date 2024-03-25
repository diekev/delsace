/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "tabs.hh"

TabWidget::TabWidget(QT_Rappels_TabWidget *rappels, QWidget *parent)
    : QTabWidget(parent), m_rappels(rappels)
{
    if (!m_rappels) {
        return;
    }

    if (m_rappels->sur_changement_page) {
        connect(this, &QTabWidget::currentChanged, this, &TabWidget::sur_changement_page);
    }

    if (m_rappels->sur_fermeture_page) {
        connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::sur_fermeture_page);
    }
}

void TabWidget::sur_changement_page(int index)
{
    if (m_rappels->sur_changement_page) {
        m_rappels->sur_changement_page(m_rappels, index);
    }
}

void TabWidget::sur_fermeture_page(int index)
{
    if (m_rappels->sur_fermeture_page) {
        m_rappels->sur_fermeture_page(m_rappels, index);
    }
}
