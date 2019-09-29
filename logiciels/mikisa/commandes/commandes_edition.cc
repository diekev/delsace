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

#include "commandes_edition.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QKeyEvent>
#pragma GCC diagnostic pop

#include "danjo/danjo.h"
#include "danjo/manipulable.h"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/mikisa.h"
#include "coeur/nuanceur.hh"
#include "coeur/operatrice_image.h"

#include "evaluation/evaluation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeAjouterPropriete final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto gestionnaire = mikisa->gestionnaire_entreface;
		auto graphe = mikisa->graphe;

		if (graphe->noeud_actif == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		danjo::Manipulable resultat;
		danjo::DonneesInterface donnees_entreface{};
		donnees_entreface.conteneur = nullptr;
		donnees_entreface.repondant_bouton = mikisa->repondant_commande();
		donnees_entreface.manipulable = &resultat;

		auto ok = gestionnaire->montre_dialogue_fichier(
					donnees_entreface,
					"entreface/ajouter_propriete.jo");

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->noeud_actif;
		auto operatrice = extrait_opimage(noeud->donnees);

		auto attache = resultat.evalue_chaine("attache_propriete");
		auto type = resultat.evalue_enum("type_propriete");

		if (attache == "") {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		danjo::Propriete prop;

		if (type == "entier") {
			prop.type = danjo::TypePropriete::ENTIER;
			prop.valeur = 0;
		}
		else {
			prop.type = danjo::TypePropriete::DECIMAL;
			prop.valeur = 0.0f;
		}

		operatrice->ajoute_propriete_extra(attache, prop);

		mikisa->notifie_observatrices(type_evenement::propriete | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjouterComposite final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		mikisa->bdd.cree_composite("composite");

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjouterNuanceur final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		mikisa->bdd.cree_nuanceur("nuanceur");

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

struct CommandeCreeNuanceurOperatrice final : public Commande {
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);
		auto graphe = mikisa->graphe;

		if (graphe->noeud_actif == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->noeud_actif;

		if (noeud->type != type_noeud::OPERATRICE) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto op = extrait_opimage(noeud->donnees);

		if (op->propriete("nom_nuanceur") == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto resultat = danjo::Manipulable();

		auto donnees_entreface = danjo::DonneesInterface{};
		donnees_entreface.conteneur = nullptr;
		donnees_entreface.repondant_bouton = mikisa->repondant_commande();
		donnees_entreface.manipulable = &resultat;

		auto gestionnaire = mikisa->gestionnaire_entreface;

		auto ok = gestionnaire->montre_dialogue_fichier(
					donnees_entreface,
					"entreface/dialogue_creation_nuanceur.jo");

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto nom_nuanceur = resultat.evalue_chaine("nom_nuanceur");
		auto nuanceur = mikisa->bdd.cree_nuanceur(nom_nuanceur);

		op->valeur_chaine("nom_nuanceur", nuanceur->noeud.nom);

		/* Notifie les graphes des noeuds parents comme étant surrannés */
		marque_parent_surannee(noeud, [](Noeud *n, PriseEntree *prise)
		{
			if (n->type != type_noeud::OPERATRICE) {
				return;
			}

			auto oper = extrait_opimage(n->donnees);
			oper->amont_change(prise);
		});

		op->parametres_changes();

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::ajoute);

		requiers_evaluation(*mikisa, PARAMETRE_CHANGE, "réponse commande ajout nuanceur opératrice");

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjouterRendu final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &/*donnees*/) override
	{
		auto mikisa = extrait_mikisa(pointeur);

		mikisa->bdd.cree_rendu("rendu");

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeDefait final : public Commande {
public:
	bool evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee) override
	{
		INUTILISE(metadonnee);
		auto mikisa = extrait_mikisa(pointeur);
		return !mikisa->pile_defait.est_vide();
	}

	int execute(std::any const &pointeur, DonneesCommande const &metadonnee) override
	{
		INUTILISE(metadonnee);
		auto mikisa = extrait_mikisa(pointeur);
		mikisa->defait();
		mikisa->notifie_observatrices(type_evenement::rafraichissement);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRefait final : public Commande {
public:
	bool evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee) override
	{
		INUTILISE(metadonnee);
		auto mikisa = extrait_mikisa(pointeur);
		return !mikisa->pile_refait.est_vide();
	}

	int execute(std::any const &pointeur, DonneesCommande const &metadonnee) override
	{
		INUTILISE(metadonnee);
		auto mikisa = extrait_mikisa(pointeur);
		mikisa->refait();
		mikisa->notifie_observatrices(type_evenement::rafraichissement);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRenomme final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &metadonnee) override
	{
		INUTILISE(metadonnee);
		auto mikisa = extrait_mikisa(pointeur);
		auto gestionnaire = mikisa->gestionnaire_entreface;
		auto graphe = mikisa->graphe;

		if (graphe->noeud_actif == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->noeud_actif;

		auto resultat = danjo::Manipulable();
		resultat.ajoute_propriete("nouveau_nom", danjo::TypePropriete::CHAINE_CARACTERE, noeud->nom);

		danjo::DonneesInterface donnees_entreface{};
		donnees_entreface.conteneur = nullptr;
		donnees_entreface.repondant_bouton = mikisa->repondant_commande();
		donnees_entreface.manipulable = &resultat;

		auto ok = gestionnaire->montre_dialogue_fichier(
					donnees_entreface,
					"entreface/dialogue_renommage.jo");

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto nom = resultat.evalue_chaine("nouveau_nom");
		noeud->nom = graphe->rend_nom_unique(nom);

		mikisa->notifie_observatrices(type_evenement::noeud | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_edition(UsineCommande &usine)
{
	usine.enregistre_type("ajouter_propriete",
						   description_commande<CommandeAjouterPropriete>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("ajouter_composite",
						   description_commande<CommandeAjouterComposite>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("ajouter_nuanceur",
						   description_commande<CommandeAjouterNuanceur>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("crée_nuanceur_opératrice",
						   description_commande<CommandeCreeNuanceurOperatrice>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("ajouter_rendu",
						   description_commande<CommandeAjouterRendu>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("défait",
						   description_commande<CommandeDefait>(
							   "", 0, Qt::Modifier::CTRL, Qt::Key_Z, false));

	usine.enregistre_type("refait",
						   description_commande<CommandeRefait>(
							   "", 0, Qt::Modifier::CTRL | Qt::Modifier::SHIFT, Qt::Key_Z, false));

	usine.enregistre_type("renomme",
						   description_commande<CommandeRenomme>(
							   "graphe", 0, 0, Qt::Key_F2, false));
}

#pragma clang diagnostic pop
