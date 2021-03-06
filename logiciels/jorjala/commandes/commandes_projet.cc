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

#include "coeur/evenement.h"
#include "coeur/jorjala.hh"
#include "coeur/sauvegarde.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeOuvrir final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		auto const chemin_projet = jorjala->requiers_dialogue(FICHIER_OUVERTURE, "*.jorjala");

		if (chemin_projet.est_vide()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		coeur::ouvre_projet(chemin_projet.c_str(), *jorjala);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeOuvrirRecent final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);
		coeur::ouvre_projet(donnees.metadonnee.c_str(), *jorjala);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static void sauve_fichier_sous(Jorjala &jorjala)
{
	auto const &chemin_projet = jorjala.requiers_dialogue(FICHIER_SAUVEGARDE, "*.jorjala");

	jorjala.chemin_projet(chemin_projet);
	jorjala.projet_ouvert(true);

	coeur::sauvegarde_projet(chemin_projet.c_str(), jorjala);
}

class CommandeSauvegarder final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto jorjala = extrait_jorjala(pointeur);

		if (jorjala->projet_ouvert()) {
			coeur::sauvegarde_projet(jorjala->chemin_projet().c_str(), *jorjala);
		}
		else {
			sauve_fichier_sous(*jorjala);
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
		sauve_fichier_sous(*jorjala);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_projet(UsineCommande &usine)
{
	usine.enregistre_type("ouvrir_fichier",
						   description_commande<CommandeOuvrir>(
							   "projet", 0, 0, 0, false));

	usine.enregistre_type("ouvrir_fichier_recent",
						   description_commande<CommandeOuvrirRecent>(
							   "projet", 0, 0, 0, false));

	usine.enregistre_type("sauvegarder",
						   description_commande<CommandeSauvegarder>(
							   "projet", 0, 0, 0, false));

	usine.enregistre_type("sauvegarder_sous",
						   description_commande<CommandeSauvegarderSous>(
							   "projet", 0, 0, 0, false));
}

#pragma clang diagnostic pop
