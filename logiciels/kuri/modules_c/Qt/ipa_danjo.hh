/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "qt_ipa_c.h"

#include "danjo/conteneur_controles.h"
#include "danjo/danjo.h"

/* ------------------------------------------------------------------------- */
/** \name ConteneurControles
 * \{ */

class ConteneurControles : public danjo::ConteneurControles {
    DNJ_Rappels_Widget *m_rappels = nullptr;

  public:
    ConteneurControles(DNJ_Rappels_Widget *rappels, QWidget *parent = nullptr);

    void ajourne_manipulable() override;

    void debute_changement_controle() override;

    void termine_changement_controle() override;

    void onglet_dossier_change(int index) override;

    void obtiens_liste(const dls::chaine &attache, dls::tableau<dls::chaine> &chaines) override;

    RepondantCommande *donne_repondant_commande();

    QLayout *crée_interface();
};

/** \} */
