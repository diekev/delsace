/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestionnaire_interface.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QCoreApplication>
#include <QFileDialog>
#include <QGuiApplication>
#include <QMessageBox>
#pragma GCC diagnostic pop

#include "base_editeur.h"
#include "evenement_kanba.hh"
#include "fenetre_principale.h"

/* ------------------------------------------------------------------------- */
/** \name Gestionnaire Fenêtre.
 * \{ */

static Qt::CursorShape convertis_type_curseur(KNB::TypeCurseur curseur)
{
    switch (curseur) {
        case KNB::TypeCurseur::NORMAL:
            return Qt::CursorShape::ArrowCursor;
        case KNB::TypeCurseur::ATTENTE_BLOQUÉ:
            return Qt::CursorShape::WaitCursor;
        case KNB::TypeCurseur::TÂCHE_ARRIÈRE_PLAN_EN_COURS:
            return Qt::CursorShape::BusyCursor;
        case KNB::TypeCurseur::MAIN_OUVERTE:
            return Qt::CursorShape::OpenHandCursor;
        case KNB::TypeCurseur::MAIN_FERMÉE:
            return Qt::CursorShape::ClosedHandCursor;
    }

    return Qt::CursorShape::ArrowCursor;
}

GestionnaireInterface::GestionnaireInterface(FenetrePrincipale &fenêtre_principale)
    : KNB::GestionnaireFenêtre(), m_fenêtre_principale(fenêtre_principale)
{
}

void GestionnaireInterface::définis_éditrice_active(BaseEditrice *éditrice)
{
    if (m_éditrice_active) {
        m_éditrice_active->actif(false);
    }

    if (éditrice) {
        éditrice->actif(true);
    }

    m_éditrice_active = nullptr;
}

void GestionnaireInterface::notifie_erreur(KNB::Chaine message)
{
    QMessageBox boite_message;
    boite_message.critical(&m_fenêtre_principale, "Erreur", message.vers_std_string().c_str());
    boite_message.setFixedSize(500, 200);
}

void GestionnaireInterface::change_curseur(KNB::TypeCurseur curseur)
{
    QGuiApplication::setOverrideCursor(QCursor(convertis_type_curseur(curseur)));
}

void GestionnaireInterface::restaure_curseur()
{
    QGuiApplication::restoreOverrideCursor();
}

void GestionnaireInterface::définit_titre_application(KNB::Chaine titre)
{
    m_fenêtre_principale.setWindowTitle(titre.vers_std_string().c_str());
}

void GestionnaireInterface::définit_texte_état_logiciel(KNB::Chaine)
{
    // À FAIRE
    // m_fenêtre_principale.définit_texte_état(texte.c_str());
}

bool GestionnaireInterface::demande_permission_avant_de_fermer()
{
    // À FAIRE
    // return m_fenêtre_principale.demande_permission_avant_de_fermer();
    return true;
}

KNB::Chaine GestionnaireInterface::affiche_dialogue_pour_sélection_fichier_lecture()
{
    auto const chemin = QFileDialog::getOpenFileName();
    return chemin.toStdString().c_str();
}

KNB::Chaine GestionnaireInterface::affiche_dialogue_pour_sélection_fichier_écriture()
{
    auto const chemin = QFileDialog::getSaveFileName();
    return chemin.toStdString().c_str();
}

/** \} */
