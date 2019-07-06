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

#include "biblinternes/commandes/commande.h"

#include "../evaluation/evaluation.hh"

#include "../evenement.h"
#include "../objet.h"
#include "../operatrice_image.h"
#include "../mikisa.h"
#include "../noeud_image.h"
#include "../scene.h"

/* ************************************************************************** */

#include "danjo/danjo.h"

class CommandeAjoutePrereglage final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override;
};

int CommandeAjoutePrereglage::execute(const std::any &pointeur, const DonneesCommande &donnees)
{
	auto mikisa = std::any_cast<Mikisa *>(pointeur);
	auto &bdd = mikisa->bdd;
	auto nom = donnees.metadonnee;
	auto op = static_cast<OperatriceImage *>(nullptr);

	auto objet = bdd.cree_objet(nom);

	auto noeud_creation = objet->graphe.cree_noeud(nom);

	if (nom == "boîte") {
		op = (mikisa->usine_operatrices())("Création Cube", objet->graphe, noeud_creation);
	}
	else if (nom == "grille") {
		op = (mikisa->usine_operatrices())("Création Grille", objet->graphe, noeud_creation);
	}
	else if (nom == "cercle") {
		op = (mikisa->usine_operatrices())("Création Cercle", objet->graphe, noeud_creation);
	}
	else if (nom == "icosphère") {
		op = (mikisa->usine_operatrices())("Création Sphère Ico", objet->graphe, noeud_creation);
	}
	else if (nom == "tube") {
		op = (mikisa->usine_operatrices())("Création Cylindre", objet->graphe, noeud_creation);
	}
	else if (nom == "cone") {
		op = (mikisa->usine_operatrices())("Création Cone", objet->graphe, noeud_creation);
	}
	else if (nom == "torus") {
		op = (mikisa->usine_operatrices())("Création Torus", objet->graphe, noeud_creation);
	}
	else {
		throw std::runtime_error("Type de préréglage inconnu");
	}

	auto texte = danjo::contenu_fichier(op->chemin_entreface());
	danjo::initialise_entreface(op, texte.c_str());

	synchronise_donnees_operatrice(noeud_creation);	

	auto noeud_sortie = objet->graphe.cree_noeud("Sortie Corps");
	/* À FAIRE : un oublie peut faire boguer le logiciel. */
	noeud_sortie->type(NOEUD_OBJET_SORTIE);
	op = (mikisa->usine_operatrices())("Sortie Corps", objet->graphe, noeud_sortie);
	synchronise_donnees_operatrice(noeud_sortie);

	objet->graphe.dernier_noeud_sortie = noeud_sortie;

	objet->graphe.connecte(noeud_creation->sortie(0), noeud_sortie->entree(0));

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
}
