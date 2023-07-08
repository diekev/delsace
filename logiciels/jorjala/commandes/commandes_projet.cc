/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "commandes_projet.h"

#include "commande_jorjala.hh"

#include "coeur/jorjala.hh"

#include "entreface/gestion_entreface.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

std::optional<JJL::CheminFichier> crée_chemin_fichier(JJL::Chaine chaine)
{
    return JJL::CheminFichier::construit(chaine);
}

/* ************************************************************************** */

class CommandeOuvrir final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        if (!jorjala.demande_permission_avant_de_fermer()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        dls::chaine chemin_projet = "";
        if (!donnees.metadonnee.est_vide()) {
            /* Nous pouvons avoir une métadonnée pour le chemin si nous sommes
             * appelé au début de l'exécution pour gérer un chemin passé en
             * ligne de commande. */
            chemin_projet = donnees.metadonnee;
        }
        else {
            chemin_projet = affiche_dialogue(FICHIER_OUVERTURE, "*.jorjala");
            if (chemin_projet.est_vide()) {
                return EXECUTION_COMMANDE_ECHOUEE;
            }
        }

        auto opt_chemin_fichier = crée_chemin_fichier(chemin_projet.c_str());
        if (!opt_chemin_fichier.has_value()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        /* À FAIRE : erreur de lecture. */
        jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
        jorjala.lis_projet(opt_chemin_fichier.value());
        jorjala.restaure_curseur_application();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

static void sauve_fichier_sous(JJL::Jorjala &jorjala, JJL::CheminFichier chemin)
{
    jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
    jorjala.sauvegarde_projet(chemin);
    jorjala.restaure_curseur_application();
}

static void sauve_fichier_sous(JJL::Jorjala &jorjala)
{
    auto const &chemin_projet = affiche_dialogue(FICHIER_SAUVEGARDE, "*.jorjala");
    if (chemin_projet == "") {
        return;
    }
    auto opt_chemin_fichier = crée_chemin_fichier(chemin_projet.c_str());
    if (!opt_chemin_fichier.has_value()) {
        return;
    }

    sauve_fichier_sous(jorjala, opt_chemin_fichier.value());
}

class CommandeSauvegarder final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
        if (!jorjala.chemin_fichier_projet().vers_std_string().empty()) {
            auto opt_chemin_fichier = crée_chemin_fichier(jorjala.chemin_fichier_projet());
            if (!opt_chemin_fichier.has_value()) {
                return EXECUTION_COMMANDE_ECHOUEE;
            }
            sauve_fichier_sous(jorjala, opt_chemin_fichier.value());
        }
        else {
            sauve_fichier_sous(jorjala);
        }

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeSauvegarderSous final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
        sauve_fichier_sous(jorjala);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeSauvegarderRessource final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala,
                                 dls::chaine const & /*metadonnee*/) override
    {
        /* À FAIRE. */
        return true;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        dls::chaine chemin_projet = affiche_dialogue(FICHIER_SAUVEGARDE, "*.jjr");
        if (chemin_projet.est_vide()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto opt_chemin_fichier = crée_chemin_fichier(chemin_projet.c_str());
        if (!opt_chemin_fichier.has_value()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        /* À FAIRE : erreur de lecture. */
        jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
        jorjala.sauvegarde_ressource_jorjala(opt_chemin_fichier.value());
        jorjala.restaure_curseur_application();
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeLectureRessource final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        dls::chaine chemin_projet;
        if (!donnees.metadonnee.est_vide()) {
            /* Nous pouvons avoir une métadonnée pour le chemin si nous sommes
             * appelé pour un préréglage. */
            chemin_projet = "ressources/" + donnees.metadonnee + ".jjr";
        }
        else {
            chemin_projet = affiche_dialogue(FICHIER_OUVERTURE, "*.jjr");
            if (chemin_projet.est_vide()) {
                return EXECUTION_COMMANDE_ECHOUEE;
            }
        }

        auto opt_chemin_fichier = crée_chemin_fichier(chemin_projet.c_str());
        if (!opt_chemin_fichier.has_value()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        /* À FAIRE : erreur de lecture. */
        jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
        jorjala.lis_ressource_jorjala(opt_chemin_fichier.value());
        jorjala.restaure_curseur_application();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeNouveauProjet final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        if (!jorjala.demande_permission_avant_de_fermer()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        jorjala.réinitialise_pour_lecture_projet();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeExportAlembic final : public CommandeJorjala {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override
    {
        if (!jorjala.exporte_vers_alembic()) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_projet(UsineCommande &usine)
{
    usine.enregistre_type("ouvrir_fichier",
                          description_commande<CommandeOuvrir>(
                              "projet", 0, Qt::Modifier::CTRL, Qt::Key_O, false, false));

    usine.enregistre_type("sauvegarder",
                          description_commande<CommandeSauvegarder>(
                              "projet", 0, Qt::Modifier::CTRL, Qt::Key_S, false, false));

    usine.enregistre_type(
        "sauvegarder_sous",
        description_commande<CommandeSauvegarderSous>(
            "projet", 0, Qt::Modifier::CTRL | Qt::Modifier::SHIFT, Qt::Key_S, false, false));

    usine.enregistre_type(
        "sauvegarder_ressource_sous",
        description_commande<CommandeSauvegarderRessource>("projet", 0, 0, 0, false, false));

    usine.enregistre_type(
        "ouvrir_ressource",
        description_commande<CommandeLectureRessource>("projet", 0, 0, 0, false));

    usine.enregistre_type("nouveau_projet",
                          description_commande<CommandeNouveauProjet>(
                              "projet", 0, Qt::Modifier::CTRL, Qt::Key_N, false));

    usine.enregistre_type(
        "export_alembic",
        description_commande<CommandeExportAlembic>("projet", 0, 0, 0, false, false));
}

#pragma clang diagnostic pop
