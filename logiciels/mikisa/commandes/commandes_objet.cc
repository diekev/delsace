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

/* ************************************************************************** */

static auto cree_noeud_op(
		danjo::GestionnaireInterface *gestionnaire,
		Graphe &graphe,
		UsineOperatrice &usine,
		dls::chaine const &nom_noeud,
		const char *nom_op)
{
	auto noeud = graphe.cree_noeud(nom_noeud, type_noeud::OPERATRICE);

	auto op = usine(nom_op, graphe, *noeud);

	auto texte = dls::contenu_fichier(op->chemin_entreface());
	gestionnaire->initialise_entreface(op, texte.c_str());

	synchronise_donnees_operatrice(*noeud);

	return noeud;
}

static auto cree_graphe_creation_objet(
		danjo::GestionnaireInterface *gestionnaire,
		Graphe &graphe,
		UsineOperatrice &usine,
		dls::chaine const &nom_noeud,
		const char *nom_op)
{
	auto noeud_creation = cree_noeud_op(gestionnaire, graphe, usine, nom_noeud, nom_op);
	auto noeud_sortie = cree_noeud_op(gestionnaire, graphe, usine, "sortie", "Sortie Corps");

	noeud_creation->pos_y(-200.0f);

	graphe.connecte(noeud_creation->sortie(0), noeud_sortie->entree(0));
	graphe.dernier_noeud_sortie = noeud_sortie;
}

static auto cree_graphe_objet_vide(
		danjo::GestionnaireInterface *gestionnaire,
		Graphe &graphe,
		UsineOperatrice &usine)
{
	auto noeud_sortie = cree_noeud_op(gestionnaire, graphe, usine, "sortie", "Sortie Corps");
	graphe.dernier_noeud_sortie = noeud_sortie;
}

static auto cree_graphe_ocean(
		danjo::GestionnaireInterface *gestionnaire,
		Graphe &graphe,
		UsineOperatrice &usine,
		int temps_debut,
		int temps_fin)
{
	auto noeud_grille = cree_noeud_op(gestionnaire, graphe, usine, "grille", "Création Grille");
	auto noeud_ocean = cree_noeud_op(gestionnaire, graphe, usine, "océan", "Océan");
	auto noeud_sortie = cree_noeud_op(gestionnaire, graphe, usine, "sortie", "Sortie Corps");

	noeud_grille->pos_y(-200.0f);
	noeud_sortie->pos_y( 200.0f);

	graphe.connecte(noeud_grille->sortie(0), noeud_ocean->entree(0));
	graphe.connecte(noeud_ocean->sortie(0), noeud_sortie->entree(0));
	graphe.dernier_noeud_sortie = noeud_sortie;

	/* donne des valeurs sensées à la grille */
	auto op = extrait_opimage(noeud_grille->donnees);
	op->valeur_decimal("taille_x", 10.0f);
	op->valeur_decimal("taille_y", 10.0f);
	op->valeur_entier("lignes", 200);
	op->valeur_entier("colonnes", 200);

	/* anime l'océan */
	op = extrait_opimage(noeud_ocean->donnees);

	auto prop = op->propriete("temps");
	prop->ajoute_cle(static_cast<float>(temps_debut), temps_debut);
	prop->ajoute_cle(static_cast<float>(temps_fin), temps_fin);
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
	auto gestionnaire = mikisa->gestionnaire_entreface;

	auto objet = bdd.cree_objet(nom, type_objet::CORPS);

	if (nom == "boîte") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Cube");
	}
	else if (nom == "grille") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Grille");
	}
	else if (nom == "cercle") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Cercle");
	}
	else if (nom == "icosphère") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Sphère Ico");
	}
	else if (nom == "tube") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Cylindre");
	}
	else if (nom == "cone") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Cone");
	}
	else if (nom == "torus") {
		cree_graphe_creation_objet(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), nom, "Création Torus");
	}
	else if (nom == "océan") {
		cree_graphe_ocean(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices(), mikisa->temps_debut, mikisa->temps_fin);
	}
	else if (nom == "vide") {
		cree_graphe_objet_vide(gestionnaire, objet->noeud->graphe, mikisa->usine_operatrices());
	}
	else {
		mikisa->affiche_erreur("Type de préréglage inconnu");
		bdd.enleve_objet(objet);
		return EXECUTION_COMMANDE_ECHOUEE;
	}

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

	if (nom == "caméra") {
		bdd.cree_objet(nom, type_objet::CAMERA);
	}
	else if (nom == "lumière") {
		bdd.cree_objet(nom, type_objet::LUMIERE);
	}
	else {
		mikisa->affiche_erreur("Type de préréglage objet inconnu");
		return EXECUTION_COMMANDE_ECHOUEE;
	}

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
