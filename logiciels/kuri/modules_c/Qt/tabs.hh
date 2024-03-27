/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QTabWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/outils/definitions.h"

#include "qt_ipa_c.h"

class TabWidget : public QTabWidget {
    Q_OBJECT

    QT_Rappels_TabWidget *m_rappels = nullptr;

  public:
    TabWidget(QT_Rappels_TabWidget *rappels, QWidget *parent = nullptr);

    EMPECHE_COPIE(TabWidget);

  public Q_SLOTS:
    void sur_changement_page(int index);
    void sur_fermeture_page(int index);
};
