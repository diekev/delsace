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
    JJL::MessageInterface m_message{{}};

  public:
    static QEvent::Type id_type_qt;

    EvenementJorjala(JJL::MessageInterface message) : QEvent(id_type_qt), m_message(message)
    {
    }

    JJL::MessageInterface message() const
    {
        return m_message;
    }
};

/** \} */
