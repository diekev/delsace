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

#include "bibliotheques/commandes/commande.h"

#include "kamikaze_main.h"
#include "sauvegarde.h"

/* ************************************************************************** */

static void ouvre_fichier_implementation(Main *main, const Context &contexte, const std::string &chemin_projet)
{
	const auto erreur = kamikaze::ouvre_projet(chemin_projet, *main, contexte);

	switch (erreur) {
		case kamikaze::erreur_fichier::AUCUNE_ERREUR:
			break;
		case kamikaze::erreur_fichier::CORROMPU:
			main->affiche_erreur("Le fichier est corrompu !");
			return;
		case kamikaze::erreur_fichier::NON_OUVERT:
			main->affiche_erreur("Le fichier n'est pas ouvert !");
			return;
		case kamikaze::erreur_fichier::NON_TROUVE:
			main->affiche_erreur("Le fichier n'a pas été trouvé !");
			return;
		case kamikaze::erreur_fichier::INCONNU:
			main->affiche_erreur("Erreur inconnu !");
			return;
		case kamikaze::erreur_fichier::GREFFON_MANQUANT:
			main->affiche_erreur("Le fichier ne pas être ouvert car il"
								 " y a un greffon manquant !");
			return;
	}

	main->chemin_projet(chemin_projet);
	main->projet_ouvert(true);

#if 0
	setWindowTitle(chemin_projet.c_str());
#endif
}

class CommandeOuvrir final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = static_cast<Main *>(pointeur);
		const auto chemin_projet = main->requiers_dialogue(FICHIER_OUVERTURE);

		if (chemin_projet.empty()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		ouvre_fichier_implementation(main, main->contexte, chemin_projet);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeOuvrirRecent final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &donnees) override
	{
		auto main = static_cast<Main *>(pointeur);

		ouvre_fichier_implementation(main, main->contexte, donnees.metadonnee);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static void sauve_fichier_sous(Main *main, const Context &context)
{
	const auto &chemin_projet = main->requiers_dialogue(FICHIER_SAUVEGARDE);

	main->chemin_projet(chemin_projet);
	main->projet_ouvert(true);

	kamikaze::sauvegarde_projet(chemin_projet, *main, context.scene);
}

class CommandeSauvegarder final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = static_cast<Main *>(pointeur);

		if (main->projet_ouvert()) {
			kamikaze::sauvegarde_projet(main->chemin_projet(), *main, main->contexte.scene);
		}
		else {
			sauve_fichier_sous(main, main->contexte);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSauvegarderSous final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto main = static_cast<Main *>(pointeur);
		sauve_fichier_sous(main, main->contexte);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeDefaire final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &/*donnees*/) override
	{
		/* À FAIRE */
		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRefaire final : public Commande {
public:
	int execute(void *pointeur, const DonneesCommande &/*donnees*/) override
	{
		/* À FAIRE */
		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_projet(UsineCommande *usine)
{
	usine->enregistre_type("ouvrir_fichier",
						   description_commande<CommandeOuvrir>(
							   "projet", 0, 0, 0, false));

	usine->enregistre_type("ouvrir_fichier_recent",
						   description_commande<CommandeOuvrirRecent>(
							   "projet", 0, 0, 0, false));

	usine->enregistre_type("sauvegarder",
						   description_commande<CommandeSauvegarder>(
							   "projet", 0, 0, 0, false));

	usine->enregistre_type("sauvegarder_sous",
						   description_commande<CommandeSauvegarderSous>(
							   "projet", 0, 0, 0, false));

	usine->enregistre_type("défaire",
						   description_commande<CommandeDefaire>(
							   "projet", 0, 0, 0, false));

	usine->enregistre_type("refaire",
						   description_commande<CommandeRefaire>(
							   "projet", 0, 0, 0, false));
}
