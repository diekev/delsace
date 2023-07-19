/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "coeur/gestionnaire_fenetre.hh"

class BaseEditrice;
class FenetrePrincipale;

/* ------------------------------------------------------------------------- */
/** \name Gestionnaire Fenêtre.
 * \{ */

class GestionnaireInterface final : public KNB::GestionnaireFenetre {
    FenetrePrincipale &m_fenêtre_principale;
    BaseEditrice *m_éditrice_active = nullptr;

  public:
    GestionnaireInterface(FenetrePrincipale &fenêtre_principale);

    GestionnaireInterface(GestionnaireInterface const &) = delete;
    GestionnaireInterface &operator=(GestionnaireInterface const &) = delete;

    void définis_éditrice_active(BaseEditrice *éditrice);

    void notifie_observatrices(KNB::TypeÉvènement evenement) override;

    void notifie_erreur(dls::chaine const &message) override;

    void change_curseur(KNB::TypeCurseur curseur) override;

    void restaure_curseur() override;

    void définit_titre_application(dls::chaine const &titre) override;

    void définit_texte_état_logiciel(dls::chaine const & /*texte*/) override;

    bool demande_permission_avant_de_fermer() override;
};

/** \} */
