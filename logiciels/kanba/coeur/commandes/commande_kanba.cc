/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "commande_kanba.hh"

#include "../kanba.h"

static inline KNB::Kanba extrait_kanba(std::any pointeur)
{
    return std::any_cast<KNB::Kanba>(pointeur);
}

int CommandeKanba::execute(const std::any &pointeur, const DonneesCommande &donnees)
{
    auto kanba = extrait_kanba(pointeur);
    auto const mode_insertion = donne_mode_insertion_historique();
    /* À FAIRE : préférence pour l'insertion dans l'historique des évènements de l'interface. */
    auto const ajoute_à_l_historique = mode_insertion == ModeInsertionHistorique::INSÈRE_TOUJOURS;

    if (ajoute_à_l_historique) {
        kanba.prépare_pour_changement(donnees.identifiant.c_str());
    }

    auto résultat = execute_kanba(kanba, donnees);

    switch (résultat) {
        case EXECUTION_COMMANDE_MODALE:
        {
            kanba.change_curseur_application(type_curseur_modal());
            break;
        }
        case EXECUTION_COMMANDE_ECHOUEE:
        {
            if (ajoute_à_l_historique) {
                kanba.annule_changement();
            }
            break;
        }
        case EXECUTION_COMMANDE_REUSSIE:
        {
            if (ajoute_à_l_historique) {
                kanba.soumets_changement();
            }
            break;
        }
    }

    return résultat;
}

void CommandeKanba::ajourne_execution_modale(std::any const &pointeur,
                                             DonneesCommande const &donnees)
{
    auto kanba = extrait_kanba(pointeur);
    ajourne_execution_modale_kanba(kanba, donnees);
}

void CommandeKanba::termine_execution_modale(std::any const &pointeur,
                                             DonneesCommande const &donnees)
{
    auto kanba = extrait_kanba(pointeur);
    termine_execution_modale_kanba(kanba, donnees);
    kanba.restaure_curseur_application();
    auto const mode_insertion = donne_mode_insertion_historique();
    /* À FAIRE : préférence pour l'insertion dans l'historique des évènements de l'interface. */
    auto const ajoute_à_l_historique = mode_insertion == ModeInsertionHistorique::INSÈRE_TOUJOURS;

    if (ajoute_à_l_historique) {
        kanba.soumets_changement();
    }
}

bool CommandeKanba::evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee)
{
    auto kanba = extrait_kanba(pointeur);
    return evalue_predicat_kanba(kanba, metadonnee);
}

int CommandeKanba::execute_kanba(KNB::Kanba & /*kanba*/, DonneesCommande const & /*donnees*/)
{
    return EXECUTION_COMMANDE_REUSSIE;
}

void CommandeKanba::ajourne_execution_modale_kanba(KNB::Kanba & /*kanba*/,
                                                   DonneesCommande const & /*donnees*/)
{
}

void CommandeKanba::termine_execution_modale_kanba(KNB::Kanba & /*kanba*/,
                                                   DonneesCommande const & /*donnees*/)
{
}

bool CommandeKanba::evalue_predicat_kanba(KNB::Kanba & /*kanba*/,
                                          dls::chaine const & /*metadonnee*/)
{
    return true;
}

KNB::TypeCurseur CommandeKanba::type_curseur_modal()
{
    return KNB::TypeCurseur::NORMAL;
}
