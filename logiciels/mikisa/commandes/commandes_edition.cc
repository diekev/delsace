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

#include "danjo/danjo.h"
#include "danjo/manipulable.h"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/mikisa.h"

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

		auto const texte_entree = dls::contenu_fichier("entreface/ajouter_propriete.jo");
		auto ok = gestionnaire->montre_dialogue(donnees_entreface, texte_entree.c_str());

		if (!ok) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto noeud = graphe->noeud_actif;
		auto operatrice = extrait_opimage(noeud->donnees());

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

void enregistre_commandes_edition(UsineCommande &usine)
{
	usine.enregistre_type("ajouter_propriete",
						   description_commande<CommandeAjouterPropriete>(
							   "", 0, 0, 0, false));

	usine.enregistre_type("ajouter_composite",
						   description_commande<CommandeAjouterComposite>(
							   "", 0, 0, 0, false));
}

#pragma clang diagnostic pop
