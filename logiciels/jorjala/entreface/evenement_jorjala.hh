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
#include <QEvent>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "coeur/jorjala.hh"

/* ------------------------------------------------------------------------- */
/** \name EvenementJorjala.
 *
 * Sous-classe de QEvent pour ajouter les évnènements de Jorjala à la boucle
 * d'évènements de Qt.
 * \{ */

class EvenementJorjala : public QEvent {
    JJL::TypeÉvènement m_type;

  public:
    static QEvent::Type id_type_qt;

    EvenementJorjala(JJL::TypeÉvènement type_evenenemt_jorjala)
        : QEvent(id_type_qt), m_type(type_evenenemt_jorjala)
    {
    }

    JJL::TypeÉvènement pour_quoi() const
    {
        return m_type;
    }
};

/** \} */
