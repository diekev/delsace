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

#include "bibloc/logeuse_memoire.hh"

#include "evaluation/evaluation.hh"
#include "evaluation/plan.hh"

#include "evenement.h"
#include "composite.h"
#include "objet.h"
#include "mikisa.h"
#include "noeud_image.h"
#include "operatrice_graphe_maillage.h"
#include "operatrice_graphe_pixel.h"
#include "operatrice_image.h"
#include "operatrice_simulation.hh"
#include "scene.h"
#include "usine_operatrice.h"

namespace coeur {

static Graphe *graphe_operatrice(OperatriceImage *operatrice)
{
	switch (operatrice->type()) {
		default:
		{
			return nullptr;
		}
		case OPERATRICE_GRAPHE_MAILLAGE:
		{
			auto op_maillage = dynamic_cast<OperatriceGrapheMaillage *>(operatrice);
			return op_maillage->graphe();
		}
		case OPERATRICE_GRAPHE_PIXEL:
		{
			auto op_pixel = dynamic_cast<OperatriceGraphePixel *>(operatrice);
			return op_pixel->graphe();
		}
		case OPERATRICE_SIMULATION:
		{
			auto op_simulation = dynamic_cast<OperatriceSimulation *>(operatrice);
			return op_simulation->graphe();
		}
	}
}

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

					for (auto const &valeur : prop.courbe) {
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

					for (auto const &valeur : prop.courbe) {
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
				dls::math::vec3f donnees = manipulable->evalue_vecteur(nom);

				element_donnees->SetAttribute("valeurx", donnees.x);
				element_donnees->SetAttribute("valeury", donnees.y);
				element_donnees->SetAttribute("valeurz", donnees.z);

				if (prop.est_anime()) {
					auto element_animation = doc.NewElement("animation");

					for (auto const &valeur : prop.courbe) {
						auto element_cle = doc.NewElement("cle");
						auto vec = std::experimental::any_cast<dls::math::vec3f>(valeur.second);
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
				auto const donnees = manipulable->evalue_couleur(nom);

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
			case danjo::TypePropriete::TEXTE:
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

static void ecris_graphe(
		tinyxml2::XMLDocument &doc,
		tinyxml2::XMLElement *racine_graphe,
		Graphe const &graphe);

static auto ecris_noeud(
		tinyxml2::XMLDocument &doc,
		tinyxml2::XMLElement *racine_graphe,
		Noeud *noeud)
{
	/* Noeud */
	auto element_noeud = doc.NewElement("noeud");
	element_noeud->SetAttribute("nom", noeud->nom().c_str());
//		element_noeud->SetAttribute("drapeaux", noeud->flags());
	element_noeud->SetAttribute("posx", noeud->pos_x());
	element_noeud->SetAttribute("posy", noeud->pos_y());

	/* Prises d'entrée. */
	auto racine_prise_entree = doc.NewElement("prises_entree");

	for (auto const &prise : noeud->entrees()) {
		auto element_prise = doc.NewElement("entree");
		element_prise->SetAttribute("nom", prise->nom.c_str());
		element_prise->SetAttribute("id", id_depuis_pointeur(prise).c_str());

		if (!prise->liens.empty()) {
			element_prise->SetAttribute("connexion", id_depuis_pointeur(prise->liens[0]).c_str());
		}

		racine_prise_entree->InsertEndChild(element_prise);
	}

	element_noeud->InsertEndChild(racine_prise_entree);

	/* Prises de sortie */
	auto racine_prise_sortie = doc.NewElement("prises_sortie");

	for (auto const &prise : noeud->sorties()) {
		/* REMARQUE : par optimisation on ne sauvegarde que les
			 * connexions depuis les prises d'entrées. */
		auto element_prise = doc.NewElement("sortie");
		element_prise->SetAttribute("nom", prise->nom.c_str());
		element_prise->SetAttribute("id", id_depuis_pointeur(prise).c_str());

		racine_prise_sortie->InsertEndChild(element_prise);
	}

	element_noeud->InsertEndChild(racine_prise_sortie);

	/* Opératrice */
	switch (noeud->type()) {
		case NOEUD_OBJET:
		{
			auto objet = std::any_cast<Objet *>(noeud->donnees());

			auto element_objet = doc.NewElement("objet");
			element_objet->SetAttribute("nom", objet->nom.c_str());

			element_noeud->InsertEndChild(element_objet);

			break;
		}
		default:
		{
			auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
			auto element_operatrice = doc.NewElement("operatrice");
			element_operatrice->SetAttribute("nom", operatrice->nom_classe());

			sauvegarde_proprietes(doc, element_operatrice, operatrice);

			/* Graphe */
			auto graphe_op = graphe_operatrice(operatrice);

			if (graphe_op != nullptr) {
				auto racine_graphe_op = doc.NewElement("graphe");
				element_operatrice->InsertEndChild(racine_graphe_op);
				ecris_graphe(doc, racine_graphe_op, *graphe_op);
			}

			element_noeud->InsertEndChild(element_operatrice);

			break;
		}
	}

	racine_graphe->InsertEndChild(element_noeud);
}

static void ecris_graphe(
		tinyxml2::XMLDocument &doc,
		tinyxml2::XMLElement *racine_graphe,
		Graphe const &graphe)
{
	for (auto const &noeud : graphe.noeuds()) {
		ecris_noeud(doc, racine_graphe, noeud);
	}
}

erreur_fichier sauvegarde_projet(filesystem::path const &chemin, Mikisa const &mikisa)
{
	tinyxml2::XMLDocument doc;
	doc.InsertFirstChild(doc.NewDeclaration());

	auto racine_projet = doc.NewElement("projet");
	doc.InsertEndChild(racine_projet);

	auto &bdd = mikisa.bdd;

	/* objets */

	auto racine_objets = doc.NewElement("objets");
	racine_projet->InsertEndChild(racine_objets);

	for (auto const objet : bdd.objets()) {
		/* Écriture de l'objet. */
		auto racine_objet = doc.NewElement("objet");
		racine_objet->SetAttribute("nom", objet->nom.c_str());

		racine_objets->InsertEndChild(racine_objet);

		/* Écriture du graphe. */
		auto racine_graphe = doc.NewElement("graphe");
		racine_objet->InsertEndChild(racine_graphe);

		ecris_graphe(doc, racine_graphe, objet->graphe);
	}

	/* composites */

	auto racine_composites = doc.NewElement("composites");
	racine_projet->InsertEndChild(racine_composites);

	for (auto const composite : bdd.composites()) {
		/* Écriture du composite. */
		auto racine_composite = doc.NewElement("composite");
		racine_composite->SetAttribute("nom", composite->nom.c_str());

		racine_composites->InsertEndChild(racine_composite);

		/* Écriture du graphe. */
		auto racine_graphe = doc.NewElement("graphe");
		racine_composite->InsertEndChild(racine_graphe);

		ecris_graphe(doc, racine_graphe, composite->graph());
	}

	/* scenes */

	auto racine_scenes = doc.NewElement("scenes");
	racine_projet->InsertEndChild(racine_scenes);


	auto planifieuse = Planifieuse{};
	auto compileuse = CompilatriceReseau{};

	for (auto const scene : bdd.scenes()) {
		/* Écriture de la scène. */
		auto racine_scene = doc.NewElement("scene");
		racine_scene->SetAttribute("nom", scene->nom.c_str());

		racine_scenes->InsertEndChild(racine_scene);

		compileuse.reseau = &scene->reseau;
		compileuse.compile_reseau(scene);

		auto plan = planifieuse.requiers_plan_pour_scene(scene->reseau);

		/* Écriture du graphe. */
		auto racine_graphe = doc.NewElement("graphe");
		racine_scene->InsertEndChild(racine_graphe);

		for (auto &noeud : plan->noeuds) {
			ecris_noeud(doc, racine_graphe, noeud->noeud_objet);
		}
	}

	auto const resultat = doc.SaveFile(chemin.c_str());

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
	auto const type_prop = element->Attribute("type");
	auto const nom_prop = element->Attribute("nom");

	auto const element_donnees = element->FirstChildElement("donnees");

	switch (static_cast<danjo::TypePropriete>(atoi(type_prop))) {
		case danjo::TypePropriete::BOOL:
		{
			auto const donnees = static_cast<bool>(atoi(element_donnees->Attribute("valeur")));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::BOOL, donnees);
			break;
		}
		case danjo::TypePropriete::ENTIER:
		{
			auto const donnees = element_donnees->Attribute("valeur");
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENTIER, atoi(donnees));

			auto const element_animation = element_donnees->FirstChildElement("animation");

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
			auto const donnees = static_cast<float>(atof(element_donnees->Attribute("valeur")));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::DECIMAL, donnees);

			auto const element_animation = element_donnees->FirstChildElement("animation");

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
			auto const donnee_x = atof(element_donnees->Attribute("valeurx"));
			auto const donnee_y = atof(element_donnees->Attribute("valeury"));
			auto const donnee_z = atof(element_donnees->Attribute("valeurz"));
			auto const donnees = dls::math::vec3f{static_cast<float>(donnee_x), static_cast<float>(donnee_y), static_cast<float>(donnee_z)};
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::VECTEUR, donnees);

			auto const element_animation = element_donnees->FirstChildElement("animation");

			if (element_animation) {
				auto prop = manipulable->propriete(nom_prop);

				auto element_cle = element_animation->FirstChildElement("cle");

				for (; element_cle != nullptr; element_cle = element_cle->NextSiblingElement("cle")) {
					auto temps = atoi(element_cle->Attribute("temps"));
					auto valeurx = static_cast<float>(atof(element_cle->Attribute("valeurx")));
					auto valeury = static_cast<float>(atof(element_cle->Attribute("valeury")));
					auto valeurz = static_cast<float>(atof(element_cle->Attribute("valeurz")));

					prop->ajoute_cle(dls::math::vec3f{valeurx, valeury, valeurz}, temps);
				}
			}

			break;
		}
		case danjo::TypePropriete::COULEUR:
		{
			auto const donnee_x = atof(element_donnees->Attribute("valeurx"));
			auto const donnee_y = atof(element_donnees->Attribute("valeury"));
			auto const donnee_z = atof(element_donnees->Attribute("valeurz"));
			auto const donnee_w = atof(element_donnees->Attribute("valeurw"));
			auto const donnees = dls::phys::couleur32(static_cast<float>(donnee_x),
										   static_cast<float>(donnee_y),
										   static_cast<float>(donnee_z),
										   static_cast<float>(donnee_w));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::COULEUR, donnees);
			break;
		}
		case danjo::TypePropriete::ENUM:
		{
			auto const donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENUM, donnees);
			break;
		}
		case danjo::TypePropriete::FICHIER_SORTIE:
		{
			auto const donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_SORTIE, donnees);
			break;
		}
		case danjo::TypePropriete::FICHIER_ENTREE:
		{
			auto const donnees = std::string(element_donnees->Attribute("valeur"));
			manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_ENTREE, donnees);
			break;
		}
		case danjo::TypePropriete::TEXTE:
		case danjo::TypePropriete::CHAINE_CARACTERE:
		{
			auto const donnees = std::string(element_donnees->Attribute("valeur"));
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
	auto const racine_propriete = element->FirstChildElement("proprietes");
	auto element_propriete = racine_propriete->FirstChildElement("propriete");

	for (; element_propriete != nullptr; element_propriete = element_propriete->NextSiblingElement("propriete")) {
		lecture_propriete(element_propriete, manipulable);
	}
}

struct DonneesConnexions {
	/* Tableau faisant correspondre les ids des prises connectées entre elles.
	 * La clé du tableau est l'id de la prise d'entrée, la valeur, celle de la
	 * prise de sortie. */
	std::unordered_map<std::string, std::string> tableau_connexion_id{};

	std::unordered_map<std::string, PriseEntree *> tableau_id_prise_entree{};
	std::unordered_map<std::string, PriseSortie *> tableau_id_prise_sortie{};
};

static void lecture_graphe(
		tinyxml2::XMLElement *element_objet,
		Mikisa &mikisa,
		Graphe *graphe);

static void lecture_noeud(
		tinyxml2::XMLElement *element_noeud,
		Mikisa &mikisa,
		Graphe *graphe,
		DonneesConnexions &donnees_connexion)
{
	auto const nom_noeud = element_noeud->Attribute("nom");
	auto const posx = element_noeud->Attribute("posx");
	auto const posy = element_noeud->Attribute("posy");

	auto noeud = graphe->cree_noeud(nom_noeud);

	switch (noeud->type()) {
		case NOEUD_OBJET:
		{
			auto const element_objet = element_noeud->FirstChildElement("objet");
			auto const nom_objet = element_objet->Attribute("nom");

			auto objet = mikisa.bdd.objet(nom_objet);

			noeud->donnees(objet);

			break;
		}
		default:
		{
			auto const element_operatrice = element_noeud->FirstChildElement("operatrice");
			auto const nom_operatrice = element_operatrice->Attribute("nom");

			OperatriceImage *operatrice = (mikisa.usine_operatrices())(nom_operatrice, *graphe, noeud);
			lecture_proprietes(element_operatrice, operatrice);
			synchronise_donnees_operatrice(noeud);

			if (std::strcmp(nom_operatrice, "Visionneur") == 0) {
				noeud->type(NOEUD_IMAGE_SORTIE);
				graphe->dernier_noeud_sortie = noeud;
			}
			else if (std::strcmp(nom_operatrice, "Sortie Corps") == 0) {
				noeud->type(NOEUD_OBJET_SORTIE);
				graphe->dernier_noeud_sortie = noeud;
			}

			auto element_graphe = element_operatrice->FirstChildElement("graphe");

			if (element_graphe != nullptr) {
				auto graphe_op = graphe_operatrice(operatrice);
				lecture_graphe(element_graphe, mikisa, graphe_op);
			}

			break;
		}
	}

	noeud->pos_x(static_cast<float>(atoi(posx)));
	noeud->pos_y(static_cast<float>(atoi(posy)));

	auto const racine_prise_entree = element_noeud->FirstChildElement("prises_entree");
	auto element_prise_entree = racine_prise_entree->FirstChildElement("entree");

	for (; element_prise_entree != nullptr; element_prise_entree = element_prise_entree->NextSiblingElement("entree")) {
		auto const nom_prise = element_prise_entree->Attribute("nom");
		auto const id_prise = element_prise_entree->Attribute("id");
		auto const connexion = element_prise_entree->Attribute("connexion");

		if (connexion == nullptr) {
			continue;
		}

		donnees_connexion.tableau_connexion_id[id_prise] = connexion;
		donnees_connexion.tableau_id_prise_entree[id_prise] = noeud->entree(nom_prise);
	}

	auto const racine_prise_sortie = element_noeud->FirstChildElement("prises_sortie");
	auto element_prise_sortie = racine_prise_sortie->FirstChildElement("sortie");

	for (; element_prise_sortie != nullptr; element_prise_sortie = element_prise_sortie->NextSiblingElement("sortie")) {
		auto const nom_prise = element_prise_sortie->Attribute("nom");
		auto const id_prise = element_prise_sortie->Attribute("id");

		donnees_connexion.tableau_id_prise_sortie[id_prise] = noeud->sortie(nom_prise);
	}
}

void lecture_graphe(
		tinyxml2::XMLElement *racine_graphe,
		Mikisa &mikisa,
		Graphe *graphe)
{
	auto element_noeud = racine_graphe->FirstChildElement("noeud");

	DonneesConnexions donnees_connexions;

	for (; element_noeud != nullptr; element_noeud = element_noeud->NextSiblingElement("noeud")) {
		lecture_noeud(element_noeud, mikisa, graphe, donnees_connexions);
	}

	/* Création des connexions. */
	for (auto const &connexion : donnees_connexions.tableau_connexion_id) {
		auto const &id_de = connexion.second;
		auto const &id_a = connexion.first;

		auto const &pointer_de = donnees_connexions.tableau_id_prise_sortie[id_de];
		auto const &pointer_a = donnees_connexions.tableau_id_prise_entree[id_a];

		if (pointer_de && pointer_a) {
			graphe->connecte(pointer_de, pointer_a);
		}
	}
}

static void lis_objets(
		tinyxml2::XMLElement *racine_objets,
		Mikisa &mikisa)
{
	auto element_objet = racine_objets->FirstChildElement("objet");

	for (; element_objet != nullptr; element_objet = element_objet->NextSiblingElement("objet")) {
		auto const nom = element_objet->Attribute("nom");
		auto objet = mikisa.bdd.cree_objet(nom);
		auto racine_graphe = element_objet->FirstChildElement("graphe");

		lecture_graphe(racine_graphe, mikisa, &objet->graphe);
	}
}

static void lis_composites(
		tinyxml2::XMLElement *racine_objets,
		Mikisa &mikisa)
{
	auto element_compo = racine_objets->FirstChildElement("composite");

	for (; element_compo != nullptr; element_compo = element_compo->NextSiblingElement("composite")) {
		auto const nom = element_compo->Attribute("nom");
		auto composite = mikisa.bdd.cree_composite(nom);
		auto racine_graphe = element_compo->FirstChildElement("graphe");

		lecture_graphe(racine_graphe, mikisa, &composite->graph());
	}
}

static void lis_scenes(
		tinyxml2::XMLElement *racine_objets,
		Mikisa &mikisa)
{
	auto element_scene = racine_objets->FirstChildElement("scene");

	for (; element_scene != nullptr; element_scene = element_scene->NextSiblingElement("scene")) {
		auto const nom = element_scene->Attribute("nom");
		auto scene = mikisa.bdd.cree_scene(nom);
		auto racine_graphe = element_scene->FirstChildElement("graphe");

		lecture_graphe(racine_graphe, mikisa, &scene->graphe);

		/* met en place les objets */
		for (auto noeud : scene->graphe.noeuds()) {
			scene->ajoute_objet(noeud, std::any_cast<Objet *>(noeud->donnees()));
		}
	}
}

erreur_fichier ouvre_projet(filesystem::path const &chemin, Mikisa &mikisa)
{
	if (!std::experimental::filesystem::exists(chemin)) {
		return erreur_fichier::NON_TROUVE;
	}

	tinyxml2::XMLDocument doc;
	doc.LoadFile(chemin.c_str());

	/* À FAIRE : sauvegarde et restauration de l'état du logiciel. */
	mikisa.derniere_visionneuse_selectionnee = nullptr;
	mikisa.manipulation_2d_activee = false;
	mikisa.type_manipulation_2d = 0;
	mikisa.manipulatrice_2d = nullptr;
	mikisa.manipulation_3d_activee = false;
	mikisa.type_manipulation_3d = 0;
	mikisa.manipulatrice_3d = nullptr;
	mikisa.chemin_courant = "/composite/";
	mikisa.bdd.reinitialise();

	/* Lecture du projet. */
	auto const racine_projet = doc.FirstChildElement("projet");

	if (racine_projet == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	/* objets */
	auto const racine_objets = racine_projet->FirstChildElement("objets");

	if (racine_objets == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	lis_objets(racine_objets, mikisa);

	/* composites */
	auto const racine_composites = racine_projet->FirstChildElement("composites");

	if (racine_composites == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	lis_composites(racine_composites, mikisa);

	/* scènes */
	auto const racine_scenes = racine_projet->FirstChildElement("scenes");

	if (racine_scenes == nullptr) {
		return erreur_fichier::CORROMPU;
	}

	lis_scenes(racine_scenes, mikisa);

	/* À FAIRE : restaure état. */
	mikisa.composite = mikisa.bdd.composites()[0];
	mikisa.scene = mikisa.bdd.scenes()[0];
	mikisa.graphe = &mikisa.scene->graphe;
	mikisa.chemin_courant = "/scènes/" + mikisa.scene->nom + "/";

	requiers_evaluation(mikisa, FICHIER_OUVERT, "chargement d'un projet");

	mikisa.notifie_observatrices(type_evenement::rafraichissement);

	return erreur_fichier::AUCUNE_ERREUR;
}

}  /* namespace coeur */
