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

#include "commande_jorjala.hh"

#include "coeur/jorjala.hh"

#if 0
#    include "biblinternes/outils/fichier.hh"

#    include "danjo/danjo.h"

#    include "evaluation/evaluation.hh"

#    include "coeur/evenement.h"
#    include "coeur/noeud_image.h"
#    include "coeur/objet.h"
#    include "coeur/operatrice_image.h"

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

	initialise_entreface(gestionnaire, op, op->chemin_entreface());

	synchronise_donnees_operatrice(*noeud);

	return noeud;
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
#endif

/* ************************************************************************** */

class CommandeAjouteObjet final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees) override;
};

int CommandeAjouteObjet::execute_jorjala(JJL::Jorjala &jorjala, const DonneesCommande &donnees)
{
    jorjala.crée_objet("objet");
    jorjala.notifie_observatrices(JJL::TypeÉvènement::OBJET | JJL::TypeÉvènement::AJOUTÉ);

#if 0
	auto &bdd = jorjala->bdd;
	auto nom = donnees.metadonnee;

	if (nom == "caméra") {
		bdd.cree_objet(nom, type_objet::CAMERA);
	}
	else if (nom == "lumière") {
		bdd.cree_objet(nom, type_objet::LUMIERE);
	}
	else {
		jorjala->affiche_erreur("Type de préréglage objet inconnu");
		return EXECUTION_COMMANDE_ECHOUEE;
	}

    jorjala->notifie_observatrices(JJL::TypeÉvènement::OBJET | JJL::TypeÉvènement::AJOUTÉ);

	requiers_evaluation(*jorjala, OBJET_AJOUTE, "exécution préréglage");
#endif

    return EXECUTION_COMMANDE_REUSSIE;
}

/* ************************************************************************** */

struct CommandeImportObjet final : public CommandeJorjala {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
#if 0
		auto const chemin = jorjala->requiers_dialogue(FICHIER_OUVERTURE, "*.obj *.stl");

		if (chemin.est_vide()) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto &usine = jorjala->usine_operatrices();
		auto gestionnaire = jorjala->gestionnaire_entreface;

		auto obj = jorjala->bdd.cree_objet("objet", type_objet::CORPS);
		auto &graphe = obj->noeud->graphe;

		auto noeud_lecture = cree_noeud_op(gestionnaire, graphe, usine, "lecture", "Import Objet");
		auto noeud_sortie = cree_noeud_op(gestionnaire, graphe, usine, "sortie", "Sortie Corps");

		noeud_lecture->pos_y(-200.0f);

		auto op_lecture = extrait_opimage(noeud_lecture->donnees);
		op_lecture->valeur_chaine("chemin", chemin);

		graphe.connecte(noeud_lecture->sortie(0), noeud_sortie->entree(0));
		graphe.dernier_noeud_sortie = noeud_sortie;

        jorjala->notifie_observatrices(JJL::TypeÉvènement::OBJET | JJL::TypeÉvènement::AJOUTÉ);

		requiers_evaluation(*jorjala, OBJET_AJOUTE, "exécution import objet");

#endif
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_objet(UsineCommande &usine)
{
    usine.enregistre_type("ajoute_objet",
                          description_commande<CommandeAjouteObjet>("objet", 0, 0, 0, false));

    usine.enregistre_type("import_objet",
                          description_commande<CommandeImportObjet>("objet", 0, 0, 0, false));
}
