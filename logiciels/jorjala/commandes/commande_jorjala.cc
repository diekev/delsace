/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "commande_jorjala.hh"

#include "coeur/jorjala.hh"

int CommandeJorjala::execute(const std::any &pointeur, const DonneesCommande &donnees)
{
    auto jorjala = extrait_jorjala(pointeur);
    auto const mode_insertion = donne_mode_insertion_historique();
    /* À FAIRE : préférence pour l'insertion dans l'historique des évènements de l'interface. */
    auto const ajoute_à_l_historique = mode_insertion == ModeInsertionHistorique::INSÈRE_TOUJOURS;

    if (ajoute_à_l_historique) {
        jorjala.prépare_pour_changement(donnees.identifiant.c_str());
    }

    auto résultat = execute_jorjala(jorjala, donnees);

    switch (résultat) {
        case EXECUTION_COMMANDE_MODALE:
        {
            if (ajoute_à_l_historique) {
                jorjala.change_curseur_application(type_curseur_modal());
            }
            break;
        }
        case EXECUTION_COMMANDE_ECHOUEE:
        {
            if (ajoute_à_l_historique) {
                jorjala.annule_changement();
            }
            break;
        }
        case EXECUTION_COMMANDE_REUSSIE:
        {
            if (ajoute_à_l_historique) {
                jorjala.soumets_changement();
            }
            break;
        }
    }

    return résultat;
}

void CommandeJorjala::ajourne_execution_modale(std::any const &pointeur,
                                               DonneesCommande const &donnees)
{
    auto jorjala = extrait_jorjala(pointeur);
    ajourne_execution_modale_jorjala(jorjala, donnees);
}

void CommandeJorjala::termine_execution_modale(std::any const &pointeur,
                                               DonneesCommande const &donnees)
{
    auto jorjala = extrait_jorjala(pointeur);
    termine_execution_modale_jorjala(jorjala, donnees);
    jorjala.restaure_curseur_application();
    auto const mode_insertion = donne_mode_insertion_historique();
    /* À FAIRE : préférence pour l'insertion dans l'historique des évènements de l'interface. */
    auto const ajoute_à_l_historique = mode_insertion == ModeInsertionHistorique::INSÈRE_TOUJOURS;

    if (ajoute_à_l_historique) {
        jorjala.soumets_changement();
    }
}

bool CommandeJorjala::evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee)
{
    auto jorjala = extrait_jorjala(pointeur);
    return evalue_predicat_jorjala(jorjala, metadonnee);
}

int CommandeJorjala::execute_jorjala(JJL::Jorjala & /*jorjala*/,
                                     DonneesCommande const & /*donnees*/)
{
    return EXECUTION_COMMANDE_REUSSIE;
}

void CommandeJorjala::ajourne_execution_modale_jorjala(JJL::Jorjala & /*jorjala*/,
                                                       DonneesCommande const & /*donnees*/)
{
}

void CommandeJorjala::termine_execution_modale_jorjala(JJL::Jorjala & /*jorjala*/,
                                                       DonneesCommande const & /*donnees*/)
{
}

bool CommandeJorjala::evalue_predicat_jorjala(JJL::Jorjala & /*jorjala*/,
                                              dls::chaine const & /*metadonnee*/)
{
    return true;
}

JJL::TypeCurseur CommandeJorjala::type_curseur_modal()
{
    return JJL::TypeCurseur::NORMAL;
}
