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

#include "biblinternes/patrons_conception/commande.h"

#include "coeur/jorjala.hh"

#include "entreface/gestion_entreface.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeOuvrir final : public Commande {
public:
    int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
        auto jorjala = extrait_jorjala(pointeur);

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

        /* À FAIRE : erreur de lecture. */
        jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
        jorjala.lis_projet(chemin_projet.c_str());
        jorjala.restaure_curseur_application();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static void sauve_fichier_sous(JJL::Jorjala &jorjala, JJL::Chaine chemin)
{
    auto const &chemin_projet = affiche_dialogue(FICHIER_SAUVEGARDE, "*.jorjala");

    if (chemin_projet != "") {
        jorjala.change_curseur_application(JJL::TypeCurseur::ATTENTE_BLOQUÉ);
        jorjala.sauvegarde_projet(chemin);
        jorjala.restaure_curseur_application();
    }
}

static void sauve_fichier_sous(JJL::Jorjala &jorjala)
{
    auto const &chemin_projet = affiche_dialogue(FICHIER_SAUVEGARDE, "*.jorjala");

    if (chemin_projet != "") {
        sauve_fichier_sous(jorjala, chemin_projet.c_str());
    }
}

class CommandeSauvegarder final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
    {
		auto jorjala = extrait_jorjala(pointeur);

        if (!jorjala.chemin_fichier_projet().vers_std_string().empty()) {
            sauve_fichier_sous(jorjala, jorjala.chemin_fichier_projet());
		}
        else {
            sauve_fichier_sous(jorjala);
        }

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSauvegarderSous final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto jorjala = extrait_jorjala(pointeur);
        sauve_fichier_sous(jorjala);
		return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_projet(UsineCommande &usine)
{
	usine.enregistre_type("ouvrir_fichier",
						   description_commande<CommandeOuvrir>(
                               "projet", 0, 0, 0, false));

	usine.enregistre_type("sauvegarder",
						   description_commande<CommandeSauvegarder>(
							   "projet", 0, 0, 0, false));

	usine.enregistre_type("sauvegarder_sous",
						   description_commande<CommandeSauvegarderSous>(
                               "projet", 0, 0, 0, false));
}

#pragma clang diagnostic pop
