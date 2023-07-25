/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestionnaire_interface.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QCoreApplication>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "dialogues.hh"
#include "evenement_jorjala.hh"
#include "fenetre_principale.h"
#include "tache.h"

/* ------------------------------------------------------------------------- */
/** \name Fonctions utilitaires locales.
 * \{ */

static Qt::CursorShape convertis_type_curseur(JJL::TypeCurseur curseur)
{
    switch (curseur) {
        case JJL::TypeCurseur::NORMAL:
            return Qt::CursorShape::ArrowCursor;
        case JJL::TypeCurseur::ATTENTE_BLOQUÉ:
            return Qt::CursorShape::WaitCursor;
        case JJL::TypeCurseur::TÂCHE_ARRIÈRE_PLAN_EN_COURS:
            return Qt::CursorShape::BusyCursor;
        case JJL::TypeCurseur::MAIN_OUVERTE:
            return Qt::CursorShape::OpenHandCursor;
        case JJL::TypeCurseur::MAIN_FERMÉE:
            return Qt::CursorShape::ClosedHandCursor;
    }

    return Qt::CursorShape::ArrowCursor;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GestionnaireInterface
 * \{ */

GestionnaireInterface::GestionnaireInterface(FenetrePrincipale &fenêtre_principale)
    : JJL::GestionnaireFenêtre(), m_fenêtre_principale(fenêtre_principale),
      m_task_notifier(memoire::loge<TaskNotifier>("TaskNotifier", &m_fenêtre_principale))
{
}

void GestionnaireInterface::notifie_observatrices(JJL::TypeÉvènement evenement)
{
    auto event = new EvenementJorjala(evenement);
    QCoreApplication::postEvent(&m_fenêtre_principale, event);
}

void GestionnaireInterface::notifie_erreur(JJL::Chaine message)
{
    QMessageBox boite_message;
    boite_message.critical(&m_fenêtre_principale, "Erreur", message.vers_std_string().c_str());
    boite_message.setFixedSize(500, 200);
}

void GestionnaireInterface::change_curseur(JJL::TypeCurseur curseur)
{
    QGuiApplication::setOverrideCursor(QCursor(convertis_type_curseur(curseur)));
}

void GestionnaireInterface::restaure_curseur()
{
    QGuiApplication::restoreOverrideCursor();
}

void GestionnaireInterface::définis_titre_application(JJL::Chaine titre)
{
    m_fenêtre_principale.setWindowTitle(titre.vers_std_string().c_str());
}

void GestionnaireInterface::définis_texte_état_logiciel(JJL::Chaine texte)
{
    m_fenêtre_principale.définis_texte_état(texte.vers_std_string().c_str());
}

void GestionnaireInterface::notifie_tâche_démarrée()
{
    m_task_notifier->signale_debut_evaluation("", 0, 0);
}

void GestionnaireInterface::notifie_tâche_terminée()
{
    m_task_notifier->signale_fin_tache();
}

bool GestionnaireInterface::demande_permission_avant_de_fermer()
{
    return m_fenêtre_principale.demande_permission_avant_de_fermer();
}

TaskNotifier *GestionnaireInterface::donne_task_notifier()
{
    return m_task_notifier;
}

JJL::CodeFemetureDialogue GestionnaireInterface::affiche_dialogue_pour_propriétés_noeud(
    JJL::Noeud noeud)
{
    auto dialogue = DialogueProprietesNoeud(noeud, &m_fenêtre_principale);
    dialogue.show();
    auto ok = dialogue.exec();

    if (ok == QDialog::Accepted) {
        return JJL::CodeFemetureDialogue::OK;
    }

    return JJL::CodeFemetureDialogue::ANNULÉ;
}

enum {
    FICHIER_SAUVEGARDE,
    FICHIER_OUVERTURE,
};

static JJL::CheminFichier chemin_depuis_qstring(QString const &qstring)
{
    auto résultat = JJL::CheminFichier::construit(qstring.toStdString().c_str());
    if (résultat.has_value()) {
        return résultat.value();
    }

    return JJL::CheminFichier({});
}

static JJL::CheminFichier affiche_dialogue(int type, JJL::Chaine filtre)
{
    auto parent = static_cast<QWidget *>(nullptr);
    auto caption = "";
    auto dir = "";
    auto filtre_str = filtre.vers_std_string();

    if (type == FICHIER_OUVERTURE) {
        auto const chemin = QFileDialog::getOpenFileName(parent, caption, dir, filtre_str.c_str());
        return chemin_depuis_qstring(chemin);
    }

    if (type == FICHIER_SAUVEGARDE) {
        auto const chemin = QFileDialog::getSaveFileName(parent, caption, dir, filtre_str.c_str());
        return chemin_depuis_qstring(chemin);
    }

    return JJL::CheminFichier({});
}

JJL::CheminFichier GestionnaireInterface::affiche_dialogue_pour_sélection_fichier_lecture(
    JJL::Chaine extension)
{
    return affiche_dialogue(FICHIER_OUVERTURE, extension);
}

JJL::CheminFichier GestionnaireInterface::affiche_dialogue_pour_sélection_fichier_écriture(
    JJL::Chaine extension)
{
    return affiche_dialogue(FICHIER_SAUVEGARDE, extension);
}

/** \} */
