/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "coeur/jorjala.hh"

class FenetrePrincipale;

/* ------------------------------------------------------------------------- */
/** \name Connexion avec l'interface utilisateur.
 * \{ */

class GestionnaireInterface final : public JJL::GestionnaireFenêtre {
    FenetrePrincipale &m_fenêtre_principale;
    TaskNotifier *m_task_notifier = nullptr;

  public:
    GestionnaireInterface(FenetrePrincipale &fenêtre_principale);

    GestionnaireInterface(GestionnaireInterface const &) = delete;
    GestionnaireInterface &operator=(GestionnaireInterface const &) = delete;

    void notifie_observatrices(JJL::TypeÉvènement evenement) override;

    void notifie_erreur(JJL::Chaine message) override;

    void change_curseur(JJL::TypeCurseur curseur) override;

    void restaure_curseur() override;

    void définit_titre_application(JJL::Chaine titre) override;

    void définit_texte_état_logiciel(JJL::Chaine texte) override;

    void notifie_tâche_démarrée() override;

    void notifie_tâche_terminée() override;

    bool demande_permission_avant_de_fermer() override;

    TaskNotifier *donne_task_notifier();

    JJL::CodeFemetureDialogue affiche_dialogue_pour_propriétés_noeud(JJL::Noeud noeud) override;

    JJL::CheminFichier affiche_dialogue_pour_sélection_fichier_lecture(
        JJL::Chaine extension) override;

    JJL::CheminFichier affiche_dialogue_pour_sélection_fichier_écriture(
        JJL::Chaine extension) override;
};

/** \} */
