/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QAction>
#include <QTabWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "coeur/jorjala.hh"

class QMenu;
class QPushButton;

/* ------------------------------------------------------------------------- */
/** \name Sous-classe de QAction afin de controler les signaux.
 * \{ */

class ActionAjoutEditrice final : public QAction {
    Q_OBJECT

  public:
    explicit ActionAjoutEditrice(QString texte, QObject *parent = nullptr);

  private Q_SLOTS:
    void sur_declenchage();

  Q_SIGNALS:
    void ajoute_editrice(int type);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VueRegion
 *  Widget pour afficher une JJL::RégionInterface et ses JJL::Éditrices.
 * \{ */

class VueRegion final : public QTabWidget {
    Q_OBJECT

    JJL::Jorjala &m_jorjala;
    JJL::RégionInterface m_région;
    QPushButton *m_bouton_affichage_liste = nullptr;
    QMenu *m_menu_liste_éditrices = nullptr;

  public:
    VueRegion(JJL::Jorjala &jorjala, JJL::RégionInterface région, QWidget *parent = nullptr);

    VueRegion(VueRegion const &) = delete;
    VueRegion &operator=(VueRegion const &) = delete;

    /** Transmet l'évènement à l'éditrice courante. */
    void ajourne_éditrice_active(JJL::ChangementÉditrice changement);

  private:
    void ajoute_page_pour_éditrice(JJL::Éditrice éditrice, bool définis_comme_page_courante);

  private Q_SLOTS:
    void ajourne_pour_changement_page(int index);
    void sur_fermeture_page(int index);
    void montre_liste();
    void sur_ajout_editrice(int type);
};

/** \} */
