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
#include <QMainWindow>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/outils/definitions.h"

#include "qt_ipa_c.h"

class FenetrePrincipale : public QMainWindow {
    Q_OBJECT

    QT_Rappels_Fenetre_Principale *m_rappels = nullptr;

  public:
    FenetrePrincipale(QT_Rappels_Fenetre_Principale *rappels);
    EMPECHE_COPIE(FenetrePrincipale);

    QT_Rappels_Fenetre_Principale *donne_rappels()
    {
        return m_rappels;
    }

    bool eventFilter(QObject *object, QEvent *event);

    void closeEvent(QCloseEvent *event);

  private:
    void construit_barre_de_menu();

  public Q_SLOTS:
    void repond_clique_menu();
};
