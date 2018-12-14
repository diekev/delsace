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

#include "../evenement.h"
#include "../mikisa.h"
#include "../sauvegarde.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static void ouvre_fichier_implementation(Mikisa &mikisa, const std::string &chemin_projet)
{
	auto const erreur = coeur::ouvre_projet(chemin_projet, mikisa);

	switch (erreur) {
		case coeur::erreur_fichier::AUCUNE_ERREUR:
			break;
		case coeur::erreur_fichier::CORROMPU:
			mikisa.affiche_erreur("Le fichier est corrompu !");
			return;
		case coeur::erreur_fichier::NON_OUVERT:
			mikisa.affiche_erreur("Le fichier n'est pas ouvert !");
			return;
		case coeur::erreur_fichier::NON_TROUVE:
			mikisa.affiche_erreur("Le fichier n'a pas été trouvé !");
			return;
		case coeur::erreur_fichier::INCONNU:
			mikisa.affiche_erreur("Erreur inconnu !");
			return;
		case coeur::erreur_fichier::GREFFON_MANQUANT:
			mikisa.affiche_erreur("Le fichier ne pas être ouvert car il"
								 " y a un greffon manquant !");
			return;
	}

	mikisa.chemin_projet(chemin_projet);
	mikisa.projet_ouvert(true);

#if 0
	setWindowTitle(chemin_projet.c_str());
#endif
}

class CommandeOuvrir final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		auto const chemin_projet = mikisa->requiers_dialogue(FICHIER_OUVERTURE);

		if (chemin_projet.empty()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		ouvre_fichier_implementation(*mikisa, chemin_projet);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeOuvrirRecent final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		ouvre_fichier_implementation(*mikisa, donnees.metadonnee);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

static void sauve_fichier_sous(Mikisa &mikisa)
{
	auto const &chemin_projet = mikisa.requiers_dialogue(FICHIER_SAUVEGARDE);

	mikisa.chemin_projet(chemin_projet);
	mikisa.projet_ouvert(true);

	coeur::sauvegarde_projet(chemin_projet, mikisa);
}

class CommandeSauvegarder final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		if (mikisa->projet_ouvert()) {
			coeur::sauvegarde_projet(mikisa->chemin_projet(), *mikisa);
		}
		else {
			sauve_fichier_sous(*mikisa);
		}

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeSauvegarderSous final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);
		sauve_fichier_sous(*mikisa);

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
