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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "sauvegarde.h"

#include <iostream>
#include <sstream>

#include <danjo/manipulable.h>

#include "bibliotheques/xml/tinyxml2.h"

#include "evenement.h"
#include "composite.h"
#include "mikisa.h"
#include "noeud_image.h"
#include "operatrice_image.h"
#include "usine_operatrice.h"

namespace coeur {

static std::string id_depuis_pointeur(void *pointeur)
{
	std::stringstream ss;
	ss << pointeur;
	return ss.str();
}

static void sauvegarde_proprietes(
		tinyxml2::XMLDocument &doc,
		tinyxml2::XMLElement *element,
		danjo::Manipulable *manipulable)
{
	tinyxml2::XMLElement *racine_propriete = doc.NewElement("proprietes");
	element->InsertEndChild(racine_propriete);

	for (auto iter = manipulable->debut(); iter != manipulable->fin(); ++iter) {
		danjo::Propriete prop = iter->second;
		auto nom = iter->first;

		auto element_prop = doc.NewElement("propriete");
		element_prop->SetAttribute("nom", nom.c_str());
//		element_prop->SetAttribute("min", prop.min);
//		element_prop->SetAttribute("max", prop.max);
		element_prop->SetAttribute("visible", prop.visible);
		element_prop->SetAttribute("type", static_cast<int>(prop.type));
		element_prop->SetAttribute("anime", prop.est_anime());

		auto element_donnees = doc.NewElement("donnees");

		switch (prop.type) {
			case danjo::TypePropriete::BOOL:
			{
				auto donnees = manipulable->evalue_bool(nom);
				element_donnees->SetAttribute("valeur", donnees);
				break;
			}
			case danjo::TypePropriete::ENTIER:
			{
				auto donnees = manipulable->evalue_entier(nom);
				element_donnees->SetAttribute("valeur", donnees);

				if (prop.est_anime()) {
					auto element_animation = doc.NewElement("animation");

					for (const auto &valeur : prop.courbe) {
						auto element_cle = doc.NewElement("cle");
						element_cle->SetAttribute("temps", valeur.first);
						element_cle->SetAttribute("valeur", std::experimental::any_cast<int>(valeur.second));
						element_animation->InsertEndChild(element_cle);
					}

					element_donnees->InsertEndChild(element_animation);
				}

				break;
			}
			case danjo::TypePropriete::DECIMAL:
			{
				auto donnees = manipulable->evalue_decimal(nom);
				element_donnees->SetAttribute("valeur", donnees);

				if (prop.est_anime()) {
					auto element_animation = doc.NewElement("animation");

					for (const auto &valeur : prop.courbe) {
						auto element_cle = doc.NewElement("cle");
						element_cle->SetAttribute("temps", valeur.first);
						element_cle->SetAttribute("valeur", std::experimental::any_cast<float>(valeur.second));
						element_animation->InsertEndChild(element_cle);
					}

					element_donnees->InsertEndChild(element_animation);
				}

				break;
			}
			case danjo::TypePropriete::VECTEUR:
			{
				glm::vec3 donnees = manipulable->evalue_vecteur(nom);

				element_donnees->SetAttribute("valeurx", donnees.x);
				element_donnees->SetAttribute("valeury", donnees.y);
				element_donnees->SetAttribute("valeurz", donnees.z);

				if (prop.est_anime()) {
					auto element_animation = doc.NewElement("animation");

					for (const auto &valeur : prop.courbe) {
						auto element_cle = doc.NewElement("cle");
						auto vec = std::experimental::any_cast<glm::vec3>(valeur.second);
						element_cle->SetAttribute("temps", valeur.first);
						element_cle->SetAttribute("valeurx", vec.x);
						element_cle->SetAttribute("valeury", vec.y);
						element_cle->SetAttribute("valeurz", vec.z);
						element_animation->InsertEndChild(element_cle);
					}

					element_donnees->InsertEndChild(element_animation);
				}

				break;
			}
			case danjo::TypePropriete::COULEUR:
			{
				const auto donnees = manipulable->evalue_couleur(nom);

				element_donnees->SetAttribute("valeurx", donnees.r);
				element_donnees->SetAttribute("valeury", donnees.v);
				element_donnees->SetAttribute("valeurz", donnees.b);
				element_donnees->SetAttribute("valeurw", donnees.a);
				break;
			}
			case danjo::TypePropriete::ENUM:
			case danjo::TypePropriete::FICHIER_SORTIE:
			case danjo::TypePropriete::FICHIER_ENTREE:
			case danjo::TypePropriete::CHAINE_CARACTERE:
			{
				std::string donnees = manipulable->evalue_chaine(nom);
				element_donnees->SetAttribute("valeur", donnees.c_str());
				break;
			}
			case danjo::COURBE_COULEUR:
			case danjo::COURBE_VALEUR:
			case danjo::RAMPE_COULEUR:
				/* À FAIRE */
				break;
		}

		element_prop->InsertEndChild(element_donnees);

		racine_propriete->InsertEndChild(element_prop);
	}
}

erreur_fichier sauvegarde_projet(const filesystem::path &chemin, const Mikisa &mikisa)
{
	tinyxml2::XMLDocument doc;
	doc.InsertFirstChild(doc.NewDeclaration());

	auto racine_projet = doc.NewElement("projet");
	doc.InsertEndChild(racine_projet);

	/* Écriture du composite. */
	tinyxml2::XMLElement *racine_composite = doc.NewElement("composite");
	racine_projet->InsertEndChild(racine_composite);

	/* Écriture du graphe. */
	tinyxml2::XMLElement *racine_graphe = doc.NewElement("graphe");
	racine_composite->InsertEndChild(racine_graphe);

	for (const auto &noeud : mikisa.composite->graph().noeuds()) {
		/* Noeud */
		auto element_noeud = doc.NewElement("noeud");
		element_noeud->SetAttribute("nom", noeud->nom().c_str());
//		element_noeud->SetAttribute("drapeaux", noeud->flags());
		element_noeud->SetAttribute("posx", noeud->pos_x());
		element_noeud->SetAttribute("posy", noeud->pos_y());

		/* Prises d'entrée. */
		auto racine_prise_entree = doc.NewElement("prises_entree");

		for (const auto &prise : noeud->entrees()) {
			auto element_prise = doc.NewElement("entree");
			element_prise->SetAttribute("nom", prise->nom.c_str());
			element_prise->SetAttribute("id", id_depuis_pointeur(prise).c_str());
			element_prise->SetAttribute("connexion", id_depuis_pointeur(prise->lien).c_str());

			racine_prise_entree->InsertEndChild(element_prise);
		}

		element_noeud->InsertEndChild(racine_prise_entree);

		/* Prises de sortie */
		auto racine_prise_sortie = doc.NewElement("prises_sortie");

		for (const auto &prise : noeud->sorties()) {
			/* REMARQUE : par optimisation on ne sauvegarde que les
				 * connexions depuis les prises d'entrées. */
			auto element_prise = doc.NewElement("sortie");
			element_prise->SetAttribute("nom", prise->nom.c_str());
			element_prise->SetAttribute("id", id_depuis_pointeur(prise).c_str());

			racine_prise_sortie->InsertEndChild(element_prise);
		}

		element_noeud->InsertEndChild(racine_prise_sortie);

		/* Opérateur */
		auto operatrice = static_cast<OperatriceImage *>(noeud->donnees());
		auto element_operatrice = doc.NewElement("operatrice");
		element_operatrice->SetAttribute("nom", operatrice->class_name());

		sauvegarde_proprietes(doc, element_operatrice, operatrice);

		element_noeud->InsertEndChild(element_operatrice);
		racine_graphe->InsertEndChild(element_noeud);
	}

	const auto resultat = doc.SaveFile(chemin.c_str());

	if (resultat == tinyxml2::XML_ERROR_FILE_COULD_NOT_BE_OPENED) {
		return erreur_fichier::NON_OUVERT;
	}

	/* À FAIRE : trouver quelles sont les autres erreurs possibles. */
	if (resultat != tinyxml2::XML_SUCCESS) {
		return erreur_fichier::INCONNU;
	}

	return erreur_fichier::AUCUNE_ERREUR;
}

/* ************************************************************************** */

static void lecture_propriete(
		tinyxml2::XMLElement *element,
		danjo::Manipulable *manipulable)
{
	const auto type_prop = element->Attribute("type");
	const auto nom_prop = element->Attribute("nom");

	const auto element_donnees = element->FirstChildElement("donnees");

	switch (static_cast<danjo::TypePropriete>(atoi(type_prop))) {
		case danjo::TypePropriete::BOOL:
		{
			const auto donnees = static_cast<bool>(atoi(element_donnees->Attribute("valeur")));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::BOOL, donnees);
			break;
		}
		case danjo::TypePropriete::ENTIER:
		{
			const auto donnees = element_donnees->Attribute("valeur");
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENTIER, atoi(donnees));

			const auto element_animation = element_donnees->FirstChildElement("animation");

			if (element_animation) {
				auto prop = manipulable->propriete(nom_prop);

				auto element_cle = element_animation->FirstChildElement("cle");

				for (; element_cle != nullptr; element_cle = element_cle->NextSiblingElement("cle")) {
					auto temps = atoi(element_cle->Attribute("temps"));
					auto valeur = atoi(element_cle->Attribute("valeur"));

					prop->ajoute_cle(valeur, temps);
				}
			}

			break;
		}
		case danjo::TypePropriete::DECIMAL:
		{
			const auto donnees = static_cast<float>(atof(element_donnees->Attribute("valeur")));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::DECIMAL, donnees);

			const auto element_animation = element_donnees->FirstChildElement("animation");

			if (element_animation) {
				auto prop = manipulable->propriete(nom_prop);

				auto element_cle = element_animation->FirstChildElement("cle");

				for (; element_cle != nullptr; element_cle = element_cle->NextSiblingElement("cle")) {
					auto temps = atoi(element_cle->Attribute("temps"));
					auto valeur = static_cast<float>(atof(element_cle->Attribute("valeur")));

					prop->ajoute_cle(valeur, temps);
				}
			}

			break;
		}
		case danjo::TypePropriete::VECTEUR:
		{
			const auto donnee_x = atof(element_donnees->Attribute("valeurx"));
			const auto donnee_y = atof(element_donnees->Attribute("valeury"));
			const auto donnee_z = atof(element_donnees->Attribute("valeurz"));
			const auto donnees = glm::vec3{donnee_x, donnee_y, donnee_z};
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::VECTEUR, donnees);

			const auto element_animation = element_donnees->FirstChildElement("animation");

			if (element_animation) {
				auto prop = manipulable->propriete(nom_prop);

				auto element_cle = element_animation->FirstChildElement("cle");

				for (; element_cle != nullptr; element_cle = element_cle->NextSiblingElement("cle")) {
					auto temps = atoi(element_cle->Attribute("temps"));
					auto valeurx = static_cast<float>(atof(element_cle->Attribute("valeurx")));
					auto valeury = static_cast<float>(atof(element_cle->Attribute("valeury")));
					auto valeurz = static_cast<float>(atof(element_cle->Attribute("valeurz")));

					prop->ajoute_cle(glm::vec3{valeurx, valeury, valeurz}, temps);
				}
			}

			break;
		}
		case danjo::TypePropriete::COULEUR:
		{
			const auto donnee_x = atof(element_donnees->Attribute("valeurx"));
			const auto donnee_y = atof(element_donnees->Attribute("valeury"));
			const auto donnee_z = atof(element_donnees->Attribute("valeurz"));
			const auto donnee_w = atof(element_donnees->Attribute("valeurw"));
			const auto donnees = couleur32(donnee_x, donnee_y, donnee_z, donnee_w);
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::COULEUR, donnees);
			break;
		}
		case danjo::TypePropriete::ENUM:
		{
			const auto donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENUM, donnees);
			break;
		}
		case danjo::TypePropriete::FICHIER_SORTIE:
		{
			const auto donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_SORTIE, donnees);
			break;
		}
		case danjo::TypePropriete::FICHIER_ENTREE:
		{
			const auto donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_ENTREE, donnees);
			break;
		}
		case danjo::TypePropriete::CHAINE_CARACTERE:
		{
			const auto donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::CHAINE_CARACTERE, donnees);
			break;
		}
		case danjo::COURBE_COULEUR:
		case danjo::COURBE_VALEUR:
		case danjo::RAMPE_COULEUR:
			/* À FAIRE */
			break;
	}
}

static void lecture_proprietes(
		tinyxml2::XMLElement *element,
		danjo::Manipulable *manipulable)
{
	const auto racine_propriete = element->FirstChildElement("proprietes");
	auto element_propriete = racine_propriete->FirstChildElement("propriete");

	for (; element_propriete != nullptr; element_propriete = element_propriete->NextSiblingElement("propriete")) {
		lecture_propriete(element_propriete, manipulable);
	}
}

struct DonneesConnexions {
	/* Tableau faisant correspondre les ids des prises connectées entre elles.
	 * La clé du tableau est l'id de la prise d'entrée, la valeur, celle de la
	 * prise de sortie. */
	std::unordered_map<std::string, std::string> tableau_connexion_id;

	std::unordered_map<std::string, PriseEntree *> tableau_id_prise_entree;
	std::unordered_map<std::string, PriseSortie *> tableau_id_prise_sortie;
};

static void lecture_noeud(
		tinyxml2::XMLElement *element_noeud,
		Mikisa *mikisa,
		Composite *composite,
		DonneesConnexions &donnees_connexion)
{
	const auto nom_noeud = element_noeud->Attribute("nom");
	const auto posx = element_noeud->Attribute("posx");
	const auto posy = element_noeud->Attribute("posy");

	Noeud *noeud = new Noeud;
	noeud->nom(nom_noeud);

	const auto element_operatrice = element_noeud->FirstChildElement("operatrice");
	const auto nom_operatrice = element_operatrice->Attribute("nom");

	OperatriceImage *operatrice = (*mikisa->usine_operatrices())(nom_operatrice, noeud);
	lecture_proprietes(element_operatrice, operatrice);
	synchronise_donnees_operatrice(noeud);

	composite->graph().ajoute(noeud);

	if (std::strcmp(nom_operatrice, "Visionneur") == 0) {
		noeud->type(NOEUD_IMAGE_SORTIE);
	}

	noeud->pos_x(atoi(posx));
	noeud->pos_y(atoi(posy));

	const auto racine_prise_entree = element_noeud->FirstChildElement("prises_entree");
	auto element_prise_entree = racine_prise_entree->FirstChildElement("entree");

	for (; element_prise_entree != nullptr; element_prise_entree = element_prise_entree->NextSiblingElement("entree")) {
		const auto nom_prise = element_prise_entree->Attribute("nom");
		const auto id_prise = element_prise_entree->Attribute("id");
		const auto connexion = element_prise_entree->Attribute("connexion");

		donnees_connexion.tableau_connexion_id[id_prise] = connexion;
		donnees_connexion.tableau_id_prise_entree[id_prise] = noeud->entree(nom_prise);
	}

	const auto racine_prise_sortie = element_noeud->FirstChildElement("prises_sortie");
	auto element_prise_sortie = racine_prise_sortie->FirstChildElement("sortie");

	for (; element_prise_sortie != nullptr; element_prise_sortie = element_prise_sortie->NextSiblingElement("sortie")) {
		const auto nom_prise = element_prise_sortie->Attribute("nom");
		const auto id_prise = element_prise_sortie->Attribute("id");

		donnees_connexion.tableau_id_prise_sortie[id_prise] = noeud->sortie(nom_prise);
	}
}

static void lecture_graphe(
		tinyxml2::XMLElement *element_objet,
		Mikisa *mikisa,
		Composite *composite)
{
	auto racine_graphe = element_objet->FirstChildElement("graphe");
	auto element_noeud = racine_graphe->FirstChildElement("noeud");

	DonneesConnexions donnees_connexions;

	for (; element_noeud != nullptr; element_noeud = element_noeud->NextSiblingElement("noeud")) {
		lecture_noeud(element_noeud, mikisa, composite, donnees_connexions);
	}

	/* Création des connexions. */
	for (const auto &connexion : donnees_connexions.tableau_connexion_id) {
		const auto &id_de = connexion.second;
		const auto &id_a = connexion.first;

		const auto &pointer_de = donnees_connexions.tableau_id_prise_sortie[id_de];
		const auto &pointer_a = donnees_connexions.tableau_id_prise_entree[id_a];

		if (pointer_de && pointer_a) {
			composite->graph().connecte(pointer_de, pointer_a);
		}
	}
}

erreur_fichier ouvre_projet(const filesystem::path &chemin, Mikisa *mikisa)
{
	if (!std::experimental::filesystem::exists(chemin)) {
		return erreur_fichier::NON_TROUVE;
	}

	tinyxml2::XMLDocument doc;
	doc.LoadFile(chemin.c_str());

	auto composite = mikisa->composite;
	composite->graph().supprime_tout();

	/* Lecture du projet. */
	const auto racine_projet = doc.FirstChildElement("projet");

	if (racine_projet == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	/* Lecture du composite. */
	const auto racine_composite = racine_projet->FirstChildElement("composite");

	if (racine_composite == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	/* Lecture du graphe. */
	lecture_graphe(racine_composite, mikisa, composite);

	mikisa->notifie_auditeurs(type_evenement::rafraichissement);

	return erreur_fichier::AUCUNE_ERREUR;
}

}  /* namespace coeur */
