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

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QKeyEvent>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "danjo/danjo.h"
#include "danjo/manipulable.h"

#include "biblinternes/outils/fichier.hh"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "commande_jorjala.hh"

//#include "coeur/composite.h"
#include "coeur/jorjala.hh"
//#include "coeur/nuanceur.hh"
//#include "coeur/operatrice_image.h"

// #include "evaluation/evaluation.hh"

#include "entreface/gestion_entreface.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class CommandeAjouterPropriete final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
        auto gestionnaire = gestionnaire_danjo(jorjala);
        auto graphe = jorjala.graphe();

        if (graphe.noeud_actif() == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        danjo::Manipulable resultat;
        auto donnees_entreface = cree_donnees_interface_danjo(jorjala, &resultat);

        auto ok = gestionnaire->montre_dialogue_fichier(donnees_entreface,
                                                        "entreface/ajouter_propriete.jo");

        if (!ok) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto attache = resultat.evalue_chaine("attache_propriete");
        auto type = resultat.evalue_enum("type_propriete");

        if (attache == "") {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        danjo::Propriete prop;

        if (type == "entier") {
            prop.type_ = danjo::TypePropriete::ENTIER;
            prop.valeur = 0;
        }
        else {
            prop.type_ = danjo::TypePropriete::DECIMAL;
            prop.valeur = 0.0f;
        }

#if 0  // À FAIRE
        auto noeud = graphe.noeud_actif;
        auto operatrice = extrait_opimage(noeud->donnees);
		operatrice->ajoute_propriete_extra(attache, prop);
#endif

        jorjala.notifie_observatrices(JJL::TypeEvenement::PROPRIÉTÉ | JJL::TypeEvenement::AJOUTÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeAjouterComposite final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
        jorjala.crée_racine_composite("composite");
        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeAjouterNuanceur final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
#if 0  // À FAIRE
        jorjala.bdd.cree_nuanceur("nuanceur");
#endif

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeAjouterNuanceurCycles final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
#if 0  // À FAIRE
        auto nuanceur = jorjala.bdd.cree_nuanceur("nuanceur");
		nuanceur->marque_est_cycles();
#endif

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

struct CommandeCreeNuanceurOperatrice final : public CommandeJorjala {
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
#if 0  // À FAIRE
        auto graphe = jorjala.graphe();

        if (graphe.noeud_actif() == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

        auto noeud = graphe.noeud_actif();

		if (noeud->type != type_noeud::OPERATRICE) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto op = extrait_opimage(noeud->donnees);

		if (op->propriete("nom_nuanceur") == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto resultat = danjo::Manipulable();
        auto donnees_entreface = cree_donnees_interface_danjo(jorjala, &resultat);
        auto gestionnaire = gestionnaire_danjo(jorjala);

        auto ok = gestionnaire->montre_dialogue_fichier(
					donnees_entreface,
					"entreface/dialogue_creation_nuanceur.jo");

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto nom_nuanceur = resultat.evalue_chaine("nom_nuanceur");
        auto nuanceur = jorjala.bdd.cree_nuanceur(nom_nuanceur);

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

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);

		requiers_evaluation(*jorjala, PARAMETRE_CHANGE, "réponse commande ajout nuanceur opératrice");
#endif

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeAjouterRendu final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const & /*donnees*/) override
    {
#if 0  // À FAIRE
        jorjala.bdd.cree_rendu("rendu");
#endif

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::AJOUTÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeDefait final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
    {
        INUTILISE(metadonnee);
        return jorjala.possède_changement_à_défaire();
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &metadonnee) override
    {
        INUTILISE(metadonnee);
        jorjala.défait_changement();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeRefait final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::IGNORE;
    }

    bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee) override
    {
        INUTILISE(metadonnee);
        return jorjala.possède_changement_à_refaire();
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &metadonnee) override
    {
        INUTILISE(metadonnee);
        jorjala.refait_changement();
        jorjala.notifie_observatrices(JJL::TypeEvenement::RAFRAICHISSEMENT);
        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

class CommandeRenomme final : public CommandeJorjala {
  public:
    ModeInsertionHistorique donne_mode_insertion_historique() const override
    {
        return ModeInsertionHistorique::INSÈRE_TOUJOURS;
    }

    int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &metadonnee) override
    {
        INUTILISE(metadonnee);
        auto graphe = jorjala.graphe();

        if (graphe.noeud_actif() == nullptr) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

        auto noeud = graphe.noeud_actif();

        auto resultat = danjo::Manipulable();
        resultat.ajoute_propriete(
            "nouveau_nom", danjo::TypePropriete::CHAINE_CARACTERE, noeud.nom().vers_std_string());

        auto donnees_entreface = cree_donnees_interface_danjo(jorjala, &resultat);
        auto gestionnaire = gestionnaire_danjo(jorjala);

        auto ok = gestionnaire->montre_dialogue_fichier(donnees_entreface,
                                                        "entreface/dialogue_renommage.jo");

        if (!ok) {
            return EXECUTION_COMMANDE_ECHOUEE;
        }

#if 0  // À FAIRE
		auto nom = resultat.evalue_chaine("nouveau_nom");
        noeud->nom = graphe.rend_nom_unique(nom);
#endif

        jorjala.notifie_observatrices(JJL::TypeEvenement::NOEUD | JJL::TypeEvenement::MODIFIÉ);

        return EXECUTION_COMMANDE_REUSSIE;
    }
};

/* ************************************************************************** */

void enregistre_commandes_edition(UsineCommande &usine)
{
    usine.enregistre_type("ajouter_propriete",
                          description_commande<CommandeAjouterPropriete>("", 0, 0, 0, false));

    usine.enregistre_type("ajouter_composite",
                          description_commande<CommandeAjouterComposite>("", 0, 0, 0, false));

    usine.enregistre_type("ajouter_nuanceur",
                          description_commande<CommandeAjouterNuanceur>("", 0, 0, 0, false));

    usine.enregistre_type("ajouter_nuanceur_cycles",
                          description_commande<CommandeAjouterNuanceurCycles>("", 0, 0, 0, false));

    usine.enregistre_type(
        "crée_nuanceur_opératrice",
        description_commande<CommandeCreeNuanceurOperatrice>("", 0, 0, 0, false));

    usine.enregistre_type("ajouter_rendu",
                          description_commande<CommandeAjouterRendu>("", 0, 0, 0, false));

    usine.enregistre_type(
        "défait",
        description_commande<CommandeDefait>("", 0, Qt::Modifier::CTRL, Qt::Key_Z, false, false));

    usine.enregistre_type(
        "refait",
        description_commande<CommandeRefait>(
            "", 0, Qt::Modifier::CTRL | Qt::Modifier::SHIFT, Qt::Key_Z, false, false));

    usine.enregistre_type(
        "renomme", description_commande<CommandeRenomme>("graphe", 0, 0, Qt::Key_F2, false));
}

#pragma clang diagnostic pop
