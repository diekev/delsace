/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QEvent>
#pragma GCC diagnostic pop

#include "coeur/evenement.h"

/* ------------------------------------------------------------------------- */
/** \name Évènement Kanba pour Qt.
 * \{ */

/* Sous-classe de QEvent pour ajouter les évnènements de Kanba à la boucle
 * d'évènements de Qt. */
class EvenementKanba : public QEvent {
    KNB::TypeÉvènement m_type;

  public:
    static QEvent::Type id_type_qt;

    EvenementKanba(KNB::TypeÉvènement type_evenenemt_kanba)
        : QEvent(id_type_qt), m_type(type_evenenemt_kanba)
    {
    }

    KNB::TypeÉvènement pour_quoi() const
    {
        return m_type;
    }
};

/** \} */
