/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "dialogues.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "editrice_proprietes.h"

/* ------------------------------------------------------------------------- */
/** \name DialogueProprietesNoeud
 * \{ */

DialogueProprietesNoeud::DialogueProprietesNoeud(JJL::Noeud &noeud, QWidget *parent)
    : QDialog(parent), m_disposition(new QVBoxLayout(this)),
      m_disposition_boutons(new QHBoxLayout()), m_noeud(noeud)
{
    /* Éditrice. */
    auto éditrice = new EditriceProprietesNoeudDialogue(noeud, this);
    m_disposition->addWidget(éditrice);

    /* Boutons. */
    auto bouton_accepter = new QPushButton("Accepter", this);
    auto bouton_annuler = new QPushButton("Annuler", this);

    connect(bouton_accepter, &QPushButton::pressed, this, &DialogueProprietesNoeud::accept);
    connect(bouton_annuler, &QPushButton::pressed, this, &DialogueProprietesNoeud::reject);

    m_disposition_boutons->addWidget(bouton_accepter);
    m_disposition_boutons->addWidget(bouton_annuler);

    m_disposition->addLayout(m_disposition_boutons);

    this->resize(600, 400);
}

/** \} */
