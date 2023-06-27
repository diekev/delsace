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
#include <QTabWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "coeur/jorjala.hh"

/* ------------------------------------------------------------------------- */
/** \name VueRegion
 *  Widget pour afficher une JJL::RegionInterface et ses JJL::Éditrices.
 * \{ */

class VueRegion final : public QTabWidget {
    Q_OBJECT

    JJL::RegionInterface m_région;

  public:
    VueRegion(JJL::Jorjala &jorjala, JJL::RegionInterface région, QWidget *parent = nullptr);

    /** Transmet l'évènement à l'éditrice courante. */
    void ajourne_éditrice_active(JJL::TypeEvenement évènement);

  private Q_SLOTS:
    void ajourne_pour_changement_page(int index);
};

/** \} */
