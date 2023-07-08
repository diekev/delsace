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
#include <QDialog>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

namespace JJL {
class Noeud;
}

class QHBoxLayout;
class QVBoxLayout;

/* ------------------------------------------------------------------------- */
/** \name Dialogue pour afficher les propriétés d'un noeud.
 * \{ */

class DialogueProprietesNoeud : public QDialog {
    QVBoxLayout *m_disposition;
    QHBoxLayout *m_disposition_boutons{};

    JJL::Noeud &m_noeud;

  public:
    explicit DialogueProprietesNoeud(JJL::Noeud &noeud, QWidget *parent = nullptr);

    DialogueProprietesNoeud(DialogueProprietesNoeud const &) = delete;
    DialogueProprietesNoeud &operator=(DialogueProprietesNoeud const &) = delete;
};

/** \} */
