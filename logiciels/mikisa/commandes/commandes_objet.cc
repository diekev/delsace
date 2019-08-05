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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "commandes_objet.hh"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/outils/fichier.hh"

#include "danjo/danjo.h"

#include "evaluation/evaluation.hh"

#include "coeur/evenement.h"
#include "coeur/objet.h"
#include "coeur/operatrice_image.h"
#include "coeur/mikisa.h"
#include "coeur/noeud_image.h"
#include "coeur/scene.h"

/* ************************************************************************** */

static auto cree_noeud_op(
		Graphe &graphe,
		UsineOperatrice &usine,
		dls::chaine const &nom_noeud,
		const char *nom_op,
		bool est_sortie)
{
	auto noeud = graphe.cree_noeud(nom_noeud);

	/* À FAIRE : un oublie peut faire boguer le logiciel. */
	if (est_sortie) {
		noeud->type(NOEUD_OBJET_SORTIE);
	}

	auto op = usine(nom_op, graphe, noeud);

	auto texte = dls::contenu_fichier(op->chemin_entreface());
	danjo::initialise_entreface(op, texte.c_str());

	synchronise_donnees_operatrice(noeud);

	return noeud;
}

static auto cree_graphe_creation_objet(
		Graphe &graphe,
		UsineOperatrice &usine,
		dls::chaine const &nom_noeud,
		const char *nom_op)
{
	auto noeud_creation = cree_noeud_op(graphe, usine, nom_noeud, nom_op, false);
	auto noeud_sortie = cree_noeud_op(graphe, usine, "sortie", "Sortie Corps", true);

	graphe.connecte(noeud_creation->sortie(0), noeud_sortie->entree(0));
	graphe.dernier_noeud_sortie = noeud_sortie;
}

/* ************************************************************************** */

class CommandeAjoutePrereglage final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override;
};

int CommandeAjoutePrereglage::execute(const std::any &pointeur, const DonneesCommande &donnees)
{
	auto mikisa = extrait_mikisa(pointeur);
	auto &bdd = mikisa->bdd;
	auto nom = donnees.metadonnee;

	auto objet = bdd.cree_objet(nom, type_objet::CORPS);

	if (nom == "boîte") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Cube");
	}
	else if (nom == "grille") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Grille");
	}
	else if (nom == "cercle") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Cercle");
	}
	else if (nom == "icosphère") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Sphère Ico");
	}
	else if (nom == "tube") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Cylindre");
	}
	else if (nom == "cone") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Cone");
	}
	else if (nom == "torus") {
		cree_graphe_creation_objet(objet->graphe, mikisa->usine_operatrices(), nom, "Création Torus");
	}
	else {
		throw std::runtime_error("Type de préréglage inconnu");
	}

	mikisa->scene->ajoute_objet(objet);

	mikisa->notifie_observatrices(type_evenement::objet | type_evenement::ajoute);

	requiers_evaluation(*mikisa, OBJET_AJOUTE, "exécution préréglage");

	return EXECUTION_COMMANDE_REUSSIE;
}

/* ************************************************************************** */


class CommandeAjouteObjet final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override;
};

int CommandeAjouteObjet::execute(const std::any &pointeur, const DonneesCommande &donnees)
{
	auto mikisa = extrait_mikisa(pointeur);
	auto &bdd = mikisa->bdd;
	auto nom = donnees.metadonnee;

	auto objet = static_cast<Objet *>(nullptr);

	if (nom == "caméra") {
		objet = bdd.cree_objet(nom, type_objet::CAMERA);
	}
	else {
		throw std::runtime_error("Type de préréglage inconnu");
	}

	mikisa->scene->ajoute_objet(objet);

	mikisa->notifie_observatrices(type_evenement::objet | type_evenement::ajoute);

	requiers_evaluation(*mikisa, OBJET_AJOUTE, "exécution préréglage");

	return EXECUTION_COMMANDE_REUSSIE;
}

/* ************************************************************************** */

void enregistre_commandes_objet(UsineCommande &usine)
{
	usine.enregistre_type("ajoute_prereglage",
						   description_commande<CommandeAjoutePrereglage>(
							   "objet", 0, 0, 0, false));

	usine.enregistre_type("ajoute_objet",
						   description_commande<CommandeAjouteObjet>(
							   "objet", 0, 0, 0, false));
}
