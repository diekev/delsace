/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/definitions.h"

#include "qt_ipa_c.h"

#include "danjo/conteneur_controles.h"
#include "danjo/danjo.h"
#include "danjo/dialogues_chemins.hh"
#include "danjo/fournisseuse_icones.hh"

/* ------------------------------------------------------------------------- */
/** \name ConteneurControles
 * \{ */

class ConteneurControles : public danjo::ConteneurControles {
    DNJ_Rappels_Widget *m_rappels = nullptr;

  public:
    ConteneurControles(DNJ_Rappels_Widget *rappels, QWidget *parent = nullptr);

    EMPECHE_COPIE(ConteneurControles);

    ~ConteneurControles() override;

    void ajourne_manipulable() override;

    void debute_changement_controle() override;

    void termine_changement_controle() override;

    void onglet_dossier_change(int index) override;

    void obtiens_liste(const dls::chaine &attache, dls::tableau<dls::chaine> &chaines) override;

    RepondantCommande *donne_repondant_commande();
    danjo::GestionnaireInterface *donne_gestionnaire();

    void crée_interface();
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PiloteClique
 * \{ */

class PiloteClique final : public RepondantCommande {
    DNJ_Rappels_Pilote_Clique *m_rappels = nullptr;

  public:
    PiloteClique(DNJ_Rappels_Pilote_Clique *rappels);
    EMPECHE_COPIE(PiloteClique);
    ~PiloteClique() override;
    bool evalue_predicat(dls::chaine const &identifiant, dls::chaine const &metadonnee) override;
    void repond_clique(dls::chaine const &identifiant, dls::chaine const &metadonnee) override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name FournisseuseIcône
 * \{ */

class FournisseuseIcône final : public danjo::FournisseuseIcône {
    DNJ_Rappels_Fournisseuse_Icone *m_rappels = nullptr;

  public:
    FournisseuseIcône(DNJ_Rappels_Fournisseuse_Icone *rappels);
    EMPECHE_COPIE(FournisseuseIcône);
    ~FournisseuseIcône() override;

    std::optional<QIcon> icone_pour_bouton(const danjo::IcônePourBouton bouton,
                                           danjo::ÉtatIcône état) override;
    std::optional<QIcon> icone_pour_identifiant(std::string const &identifiant,
                                                danjo::ÉtatIcône état) override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DialoguesChemins
 * \{ */

class DialoguesChemins final : public danjo::DialoguesChemins {
    DNJ_Rappels_DialoguesChemins *m_rappels = nullptr;

  public:
    DialoguesChemins(DNJ_Rappels_DialoguesChemins *rappels);
    EMPECHE_COPIE(DialoguesChemins);
    ~DialoguesChemins() override;

    QString donne_chemin_pour_ouverture(QString const &chemin_existant,
                                        QString const &caption,
                                        QString const &dossier,
                                        QString const &filtres) override;

    QString donne_chemin_pour_écriture(QString const &chemin_existant,
                                       QString const &caption,
                                       QString const &dossier,
                                       QString const &filtres) override;
};

/** \} */
