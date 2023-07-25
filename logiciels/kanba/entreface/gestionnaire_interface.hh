/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "coeur/kanba.h"

class BaseEditrice;
class FenetrePrincipale;

/* ------------------------------------------------------------------------- */
/** \name Gestionnaire Fenêtre.
 * \{ */

class GestionnaireInterface final : public KNB::GestionnaireFenêtre {
    FenetrePrincipale &m_fenêtre_principale;
    BaseEditrice *m_éditrice_active = nullptr;

  public:
    GestionnaireInterface(FenetrePrincipale &fenêtre_principale);

    GestionnaireInterface(GestionnaireInterface const &) = delete;
    GestionnaireInterface &operator=(GestionnaireInterface const &) = delete;

    void définis_éditrice_active(BaseEditrice *éditrice);

    void notifie_erreur(KNB::Chaine message) override;

    void change_curseur(KNB::TypeCurseur curseur) override;

    void restaure_curseur() override;

    void définis_titre_application(KNB::Chaine titre) override;

    void définis_texte_état_logiciel(KNB::Chaine /*texte*/) override;

    bool demande_permission_avant_de_fermer() override;

    KNB::Chaine affiche_dialogue_pour_sélection_fichier_lecture() override;

    KNB::Chaine affiche_dialogue_pour_sélection_fichier_écriture() override;
};

/** \} */
