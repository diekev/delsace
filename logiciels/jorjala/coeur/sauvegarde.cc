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

#include "danjo/manipulable.h"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/xml/declaration.h"
#include "biblinternes/xml/document.h"
#include "biblinternes/xml/element.h"
#include "biblinternes/xml/noeud.h"

#include "evaluation/evaluation.hh"
#include "evaluation/plan.hh"

#include "composite.h"
#include "contexte_evaluation.hh"
#include "evenement.h"
#include "jorjala.hh"
#include "noeud_image.h"
#include "nuanceur.hh"
#include "objet.h"
#include "operatrice_graphe_detail.hh"
#include "operatrice_image.h"
#include "operatrice_simulation.hh"
#include "rendu.hh"
#include "usine_operatrice.h"

namespace coeur {

static dls::chaine id_depuis_pointeur(void *pointeur)
{
    dls::flux_chaine ss;
    ss << pointeur;
    return ss.chn();
}

static void sauvegarde_proprietes(dls::xml::Document &doc,
                                  dls::xml::Element *element,
                                  danjo::Manipulable *manipulable)
{
    dls::xml::Element *racine_propriete = doc.NewElement("proprietes");
    element->InsertEndChild(racine_propriete);

    for (auto iter = manipulable->debut(); iter != manipulable->fin(); ++iter) {
        danjo::Propriete prop = iter->second;
        auto nom = iter->first;

        auto element_prop = doc.NewElement("propriete");
        element_prop->SetAttribut("nom", nom.c_str());
        //		element_prop->SetAttribut("min", prop.min);
        //		element_prop->SetAttribut("max", prop.max);
        element_prop->SetAttribut("visible", prop.visible);
        element_prop->SetAttribut("type", static_cast<int>(prop.type));
        element_prop->SetAttribut("anime", prop.est_animee());
        element_prop->SetAttribut("est_extra", prop.est_extra);

        auto element_donnees = doc.NewElement("donnees");

        switch (prop.type) {
            case danjo::TypePropriete::BOOL:
            {
                auto donnees = manipulable->evalue_bool(nom);
                element_donnees->SetAttribut("valeur", donnees);
                break;
            }
            case danjo::TypePropriete::ENTIER:
            {
                auto donnees = manipulable->evalue_entier(nom);
                element_donnees->SetAttribut("valeur", donnees);

                if (prop.est_animee()) {
                    auto element_animation = doc.NewElement("animation");

                    for (auto const &valeur : prop.courbe) {
                        auto element_cle = doc.NewElement("cle");
                        element_cle->SetAttribut("temps", valeur.first);
                        element_cle->SetAttribut("valeur", std::any_cast<int>(valeur.second));
                        element_animation->InsertEndChild(element_cle);
                    }

                    element_donnees->InsertEndChild(element_animation);
                }

                break;
            }
            case danjo::TypePropriete::DECIMAL:
            {
                auto donnees = manipulable->evalue_decimal(nom);
                element_donnees->SetAttribut("valeur", donnees);

                if (prop.est_animee()) {
                    auto element_animation = doc.NewElement("animation");

                    for (auto const &valeur : prop.courbe) {
                        auto element_cle = doc.NewElement("cle");
                        element_cle->SetAttribut("temps", valeur.first);
                        element_cle->SetAttribut("valeur", std::any_cast<float>(valeur.second));
                        element_animation->InsertEndChild(element_cle);
                    }

                    element_donnees->InsertEndChild(element_animation);
                }

                break;
            }
            case danjo::TypePropriete::VECTEUR:
            {
                dls::math::vec3f donnees = manipulable->evalue_vecteur(nom);

                element_donnees->SetAttribut("valeurx", donnees.x);
                element_donnees->SetAttribut("valeury", donnees.y);
                element_donnees->SetAttribut("valeurz", donnees.z);

                if (prop.est_animee()) {
                    auto element_animation = doc.NewElement("animation");

                    for (auto const &valeur : prop.courbe) {
                        auto element_cle = doc.NewElement("cle");
                        auto vec = std::any_cast<dls::math::vec3f>(valeur.second);
                        element_cle->SetAttribut("temps", valeur.first);
                        element_cle->SetAttribut("valeurx", vec.x);
                        element_cle->SetAttribut("valeury", vec.y);
                        element_cle->SetAttribut("valeurz", vec.z);
                        element_animation->InsertEndChild(element_cle);
                    }

                    element_donnees->InsertEndChild(element_animation);
                }

                break;
            }
            case danjo::TypePropriete::COULEUR:
            {
                auto const donnees = manipulable->evalue_couleur(nom);

                element_donnees->SetAttribut("valeurx", donnees.r);
                element_donnees->SetAttribut("valeury", donnees.v);
                element_donnees->SetAttribut("valeurz", donnees.b);
                element_donnees->SetAttribut("valeurw", donnees.a);
                break;
            }
            case danjo::TypePropriete::ENUM:
            case danjo::TypePropriete::FICHIER_SORTIE:
            case danjo::TypePropriete::FICHIER_ENTREE:
            case danjo::TypePropriete::CHAINE_CARACTERE:
            case danjo::TypePropriete::TEXTE:
            {
                dls::chaine donnees = manipulable->evalue_chaine(nom);
                element_donnees->SetAttribut("valeur", donnees.c_str());
                break;
            }
            case danjo::COURBE_COULEUR:
            case danjo::COURBE_VALEUR:
            case danjo::RAMPE_COULEUR:
            case danjo::TypePropriete::LISTE_MANIP:
                /* À FAIRE */
                break;
        }

        element_prop->InsertEndChild(element_donnees);

        racine_propriete->InsertEndChild(element_prop);
    }
}

static void ecris_graphe(dls::xml::Document &doc,
                         dls::xml::Element *racine_graphe,
                         Graphe const &graphe);

static auto ecris_noeud(dls::xml::Document &doc, dls::xml::Element *racine_graphe, Noeud *noeud)
{
    /* Noeud */
    auto element_noeud = doc.NewElement("noeud");
    element_noeud->SetAttribut("nom", noeud->nom.c_str());
    //		element_noeud->SetAttribut("drapeaux", noeud->flags());
    element_noeud->SetAttribut("posx", noeud->pos_x());
    element_noeud->SetAttribut("posy", noeud->pos_y());

    /* Prises d'entrée. */
    auto racine_prise_entree = doc.NewElement("prises_entree");

    for (auto const &prise : noeud->entrees) {
        auto element_prise = doc.NewElement("entree");
        element_prise->SetAttribut("nom", prise->nom.c_str());
        element_prise->SetAttribut("id", id_depuis_pointeur(prise).c_str());

        auto i = 0;
        for (auto lien : prise->liens) {
            element_prise->SetAttribut(("connexion" + dls::vers_chaine(i++)).c_str(),
                                       id_depuis_pointeur(lien).c_str());
        }

        racine_prise_entree->InsertEndChild(element_prise);
    }

    element_noeud->InsertEndChild(racine_prise_entree);

    /* Prises de sortie */
    auto racine_prise_sortie = doc.NewElement("prises_sortie");

    for (auto const &prise : noeud->sorties) {
        /* REMARQUE : par optimisation on ne sauvegarde que les
         * connexions depuis les prises d'entrées. */
        auto element_prise = doc.NewElement("sortie");
        element_prise->SetAttribut("nom", prise->nom.c_str());
        element_prise->SetAttribut("id", id_depuis_pointeur(prise).c_str());

        racine_prise_sortie->InsertEndChild(element_prise);
    }

    element_noeud->InsertEndChild(racine_prise_sortie);

    /* Opératrice */
    switch (noeud->type) {
        case type_noeud::OBJET:
        {
            auto objet = extrait_objet(noeud->donnees);

            auto element_objet = doc.NewElement("objet");
            element_objet->SetAttribut("nom", objet->noeud->nom.c_str());

            element_noeud->InsertEndChild(element_objet);

            break;
        }
        case type_noeud::NUANCEUR:
        {
            auto nuanceur = extrait_nuanceur(noeud->donnees);

            auto element_nuanceur = doc.NewElement("nuanceur");
            element_nuanceur->SetAttribut("nom", nuanceur->noeud.nom.c_str());

            element_noeud->InsertEndChild(element_nuanceur);

            break;
        }
        case type_noeud::RENDU:
        {
            auto rendu = extrait_rendu(noeud->donnees);

            auto element_rendu = doc.NewElement("objet");
            element_rendu->SetAttribut("nom", rendu->noeud.nom.c_str());

            element_noeud->InsertEndChild(element_rendu);

            break;
        }
        case type_noeud::COMPOSITE:
        {
            auto composite = extrait_composite(noeud->donnees);

            auto element_composite = doc.NewElement("composite");
            element_composite->SetAttribut("nom", composite->noeud->nom.c_str());

            element_noeud->InsertEndChild(element_composite);

            break;
        }
        case type_noeud::OPERATRICE:
        {
            auto operatrice = extrait_opimage(noeud->donnees);
            auto element_operatrice = doc.NewElement("operatrice");
            element_operatrice->SetAttribut("nom", operatrice->nom_classe());

            if (operatrice->type() == OPERATRICE_DETAIL) {
                auto op_detail = dynamic_cast<OperatriceFonctionDetail *>(operatrice);
                element_operatrice->SetAttribut("detail", op_detail->nom_fonction.c_str());
            }
            else if (operatrice->type() == OPERATRICE_GRAPHE_DETAIL) {
                auto op_detail = dynamic_cast<OperatriceGrapheDetail *>(operatrice);
                element_operatrice->SetAttribut("type_detail", op_detail->type_detail);
            }

            sauvegarde_proprietes(doc, element_operatrice, operatrice);

            /* Graphe */
            if (noeud->peut_avoir_graphe) {
                auto racine_graphe_op = doc.NewElement("graphe");
                element_operatrice->InsertEndChild(racine_graphe_op);
                ecris_graphe(doc, racine_graphe_op, noeud->graphe);
            }

            element_noeud->InsertEndChild(element_operatrice);

            break;
        }
        case type_noeud::INVALIDE:
        {
            break;
        }
    }

    racine_graphe->InsertEndChild(element_noeud);
}

static void ecris_graphe(dls::xml::Document &doc,
                         dls::xml::Element *racine_graphe,
                         Graphe const &graphe)
{
    racine_graphe->SetAttribut("centre_x", graphe.centre_x);
    racine_graphe->SetAttribut("centre_y", graphe.centre_y);
    racine_graphe->SetAttribut("zoom", graphe.zoom);

    for (auto const &noeud : graphe.noeuds()) {
        ecris_noeud(doc, racine_graphe, noeud);
    }
}

static void sauvegarde_etat(dls::xml::Document &doc, Jorjala const &jorjala)
{
    auto racine_etat = doc.NewElement("etat");
    doc.InsertEndChild(racine_etat);

    racine_etat->SetAttribut("chemin_courant", jorjala.chemin_courant.c_str());
}

erreur_fichier sauvegarde_projet(filesystem::path const &chemin, Jorjala const &jorjala)
{
    dls::xml::Document doc;
    doc.InsertFirstChild(doc.NewDeclaration());

    sauvegarde_etat(doc, jorjala);

    auto racine_projet = doc.NewElement("projet");
    doc.InsertEndChild(racine_projet);

    auto &bdd = jorjala.bdd;

    /* objets */

    auto racine_objets = doc.NewElement("objets");
    racine_projet->InsertEndChild(racine_objets);

    for (auto const objet : bdd.objets()) {
        /* Écriture de l'objet. */
        auto racine_objet = doc.NewElement("objet");
        racine_objet->SetAttribut("nom", objet->noeud->nom.c_str());
        racine_objet->SetAttribut("type", static_cast<int>(objet->type));

        racine_objets->InsertEndChild(racine_objet);

        /* Écriture du graphe. */
        auto racine_graphe = doc.NewElement("graphe");
        racine_objet->InsertEndChild(racine_graphe);

        ecris_graphe(doc, racine_graphe, objet->noeud->graphe);
        sauvegarde_proprietes(doc, racine_objet, objet->noeud);
    }

    /* Écriture du graphe. */
    auto racine_graphe_objs = doc.NewElement("graphe");
    racine_objets->InsertEndChild(racine_graphe_objs);
    ecris_graphe(doc, racine_graphe_objs, *jorjala.bdd.graphe_objets());

    /* composites */

    auto racine_composites = doc.NewElement("composites");
    racine_projet->InsertEndChild(racine_composites);

    for (auto const composite : bdd.composites()) {
        /* Écriture du composite. */
        auto racine_composite = doc.NewElement("composite");
        racine_composite->SetAttribut("nom", composite->noeud->nom.c_str());

        racine_composites->InsertEndChild(racine_composite);

        /* Écriture du graphe. */
        auto racine_graphe = doc.NewElement("graphe");
        racine_composite->InsertEndChild(racine_graphe);

        ecris_graphe(doc, racine_graphe, composite->noeud->graphe);
    }

    /* Écriture du graphe. */
    auto racine_graphe_comps = doc.NewElement("graphe");
    racine_composites->InsertEndChild(racine_graphe_comps);
    ecris_graphe(doc, racine_graphe_comps, *jorjala.bdd.graphe_composites());

    /* nuanceurs */

    auto racine_nuanceurs = doc.NewElement("nuanceurs");
    racine_projet->InsertEndChild(racine_nuanceurs);

    for (auto const nuanceur : bdd.nuanceurs()) {
        /* Écriture du nuanceur. */
        auto racine_nuanceur = doc.NewElement("nuanceur");
        racine_nuanceur->SetAttribut("nom", nuanceur->noeud.nom.c_str());

        racine_nuanceurs->InsertEndChild(racine_nuanceur);

        /* Écriture du graphe. */
        auto racine_graphe = doc.NewElement("graphe");
        racine_nuanceur->InsertEndChild(racine_graphe);

        ecris_graphe(doc, racine_graphe, nuanceur->noeud.graphe);
    }

    /* Écriture du graphe. */
    auto racine_graphe_nuanceurs = doc.NewElement("graphe");
    racine_nuanceurs->InsertEndChild(racine_graphe_nuanceurs);
    ecris_graphe(doc, racine_graphe_nuanceurs, *jorjala.bdd.graphe_nuanceurs());

    /* rendus */

    auto racine_rendus = doc.NewElement("rendus");
    racine_projet->InsertEndChild(racine_rendus);

    for (auto const rendu : bdd.rendus()) {
        /* Écriture du rendu. */
        auto racine_rendu = doc.NewElement("rendu");
        racine_rendu->SetAttribut("nom", rendu->noeud.nom.c_str());

        racine_rendus->InsertEndChild(racine_rendu);

        /* Écriture du graphe. */
        auto racine_graphe = doc.NewElement("graphe");
        racine_rendu->InsertEndChild(racine_graphe);

        ecris_graphe(doc, racine_graphe, rendu->noeud.graphe);
    }

    /* Écriture du graphe. */
    auto racine_graphe_rendus = doc.NewElement("graphe");
    racine_rendus->InsertEndChild(racine_graphe_rendus);
    ecris_graphe(doc, racine_graphe_rendus, *jorjala.bdd.graphe_rendus());

    auto const resultat = doc.SaveFile(chemin.c_str());

    if (resultat == dls::xml::XML_ERROR_FILE_COULD_NOT_BE_OPENED) {
        return erreur_fichier::NON_OUVERT;
    }

    if (resultat != dls::xml::XML_SUCCESS) {
        return erreur_fichier::INCONNU;
    }

    return erreur_fichier::AUCUNE_ERREUR;
}

/* ************************************************************************** */

static void lecture_propriete(dls::xml::Element *element, danjo::Manipulable *manipulable)
{
    auto const type_prop = element->attribut("type");
    auto const nom_prop = element->attribut("nom");
    auto const est_extra = element->attribut("est_extra");

    auto const element_donnees = element->FirstChildElement("donnees");

    switch (static_cast<danjo::TypePropriete>(atoi(type_prop))) {
        case danjo::TypePropriete::BOOL:
        {
            auto const donnees = static_cast<bool>(atoi(element_donnees->attribut("valeur")));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::BOOL, donnees);
            break;
        }
        case danjo::TypePropriete::ENTIER:
        {
            auto const donnees = element_donnees->attribut("valeur");
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENTIER, atoi(donnees));

            auto const element_animation = element_donnees->FirstChildElement("animation");

            if (element_animation) {
                auto prop = manipulable->propriete(nom_prop);

                auto element_cle = element_animation->FirstChildElement("cle");

                for (; element_cle != nullptr;
                     element_cle = element_cle->NextSiblingElement("cle")) {
                    auto temps = atoi(element_cle->attribut("temps"));
                    auto valeur = atoi(element_cle->attribut("valeur"));

                    prop->ajoute_cle(valeur, temps);
                }
            }

            break;
        }
        case danjo::TypePropriete::DECIMAL:
        {
            auto const donnees = static_cast<float>(atof(element_donnees->attribut("valeur")));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::DECIMAL, donnees);

            auto const element_animation = element_donnees->FirstChildElement("animation");

            if (element_animation) {
                auto prop = manipulable->propriete(nom_prop);

                auto element_cle = element_animation->FirstChildElement("cle");

                for (; element_cle != nullptr;
                     element_cle = element_cle->NextSiblingElement("cle")) {
                    auto temps = atoi(element_cle->attribut("temps"));
                    auto valeur = static_cast<float>(atof(element_cle->attribut("valeur")));

                    prop->ajoute_cle(valeur, temps);
                }
            }

            break;
        }
        case danjo::TypePropriete::VECTEUR:
        {
            auto const donnee_x = atof(element_donnees->attribut("valeurx"));
            auto const donnee_y = atof(element_donnees->attribut("valeury"));
            auto const donnee_z = atof(element_donnees->attribut("valeurz"));
            auto const donnees = dls::math::vec3f{static_cast<float>(donnee_x),
                                                  static_cast<float>(donnee_y),
                                                  static_cast<float>(donnee_z)};
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::VECTEUR, donnees);

            auto const element_animation = element_donnees->FirstChildElement("animation");

            if (element_animation) {
                auto prop = manipulable->propriete(nom_prop);

                auto element_cle = element_animation->FirstChildElement("cle");

                for (; element_cle != nullptr;
                     element_cle = element_cle->NextSiblingElement("cle")) {
                    auto temps = atoi(element_cle->attribut("temps"));
                    auto valeurx = static_cast<float>(atof(element_cle->attribut("valeurx")));
                    auto valeury = static_cast<float>(atof(element_cle->attribut("valeury")));
                    auto valeurz = static_cast<float>(atof(element_cle->attribut("valeurz")));

                    prop->ajoute_cle(dls::math::vec3f{valeurx, valeury, valeurz}, temps);
                }
            }

            break;
        }
        case danjo::TypePropriete::COULEUR:
        {
            auto const donnee_x = atof(element_donnees->attribut("valeurx"));
            auto const donnee_y = atof(element_donnees->attribut("valeury"));
            auto const donnee_z = atof(element_donnees->attribut("valeurz"));
            auto const donnee_w = atof(element_donnees->attribut("valeurw"));
            auto const donnees = dls::phys::couleur32(static_cast<float>(donnee_x),
                                                      static_cast<float>(donnee_y),
                                                      static_cast<float>(donnee_z),
                                                      static_cast<float>(donnee_w));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::COULEUR, donnees);
            break;
        }
        case danjo::TypePropriete::ENUM:
        {
            auto const donnees = dls::chaine(element_donnees->attribut("valeur"));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::ENUM, donnees);
            break;
        }
        case danjo::TypePropriete::FICHIER_SORTIE:
        {
            auto const donnees = dls::chaine(element_donnees->attribut("valeur"));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_SORTIE, donnees);
            break;
        }
        case danjo::TypePropriete::FICHIER_ENTREE:
        {
            auto const donnees = dls::chaine(element_donnees->attribut("valeur"));
            manipulable->ajoute_propriete(nom_prop, danjo::TypePropriete::FICHIER_ENTREE, donnees);
            break;
        }
        case danjo::TypePropriete::TEXTE:
        case danjo::TypePropriete::CHAINE_CARACTERE:
        {
            auto const donnees = dls::chaine(element_donnees->attribut("valeur"));
            manipulable->ajoute_propriete(
                nom_prop, danjo::TypePropriete::CHAINE_CARACTERE, donnees);
            break;
        }
        case danjo::COURBE_COULEUR:
        case danjo::COURBE_VALEUR:
        case danjo::RAMPE_COULEUR:
        case danjo::TypePropriete::LISTE_MANIP:
            /* À FAIRE */
            break;
    }

    if (est_extra && static_cast<bool>(atoi(est_extra))) {
        auto prop = manipulable->propriete(nom_prop);
        prop->est_extra = true;
    }
}

static void lecture_proprietes(dls::xml::Element *element, danjo::Manipulable *manipulable)
{
    auto const racine_propriete = element->FirstChildElement("proprietes");

    if (racine_propriete == nullptr) {
        return;
    }

    auto element_propriete = racine_propriete->FirstChildElement("propriete");

    for (; element_propriete != nullptr;
         element_propriete = element_propriete->NextSiblingElement("propriete")) {
        lecture_propriete(element_propriete, manipulable);
    }
}

struct DonneesConnexions {
    /* Tableau faisant correspondre les ids des prises connectées entre elles.
     * La clé du tableau est l'id de la prise d'entrée, la valeur, celle de la
     * prise de sortie. */
    dls::tableau<std::pair<dls::chaine, dls::chaine>> tableau_connexion_id{};

    dls::dico_desordonne<dls::chaine, PriseEntree *> tableau_id_prise_entree{};
    dls::dico_desordonne<dls::chaine, PriseSortie *> tableau_id_prise_sortie{};
};

static void lecture_graphe(dls::xml::Element *element_objet,
                           Jorjala &jorjala,
                           Graphe *graphe,
                           type_noeud type);

static void lecture_noeud(dls::xml::Element *element_noeud,
                          Jorjala &jorjala,
                          Graphe *graphe,
                          DonneesConnexions &donnees_connexion,
                          type_noeud type)
{
    auto const nom_noeud = element_noeud->attribut("nom");
    auto const posx = element_noeud->attribut("posx");
    auto const posy = element_noeud->attribut("posy");

    auto noeud = graphe->cree_noeud(nom_noeud, type);

    switch (type) {
        case type_noeud::INVALIDE:
        {
            break;
        }
        case type_noeud::OBJET:
        {
            /* fait via lecture_objets */
            break;
        }
        case type_noeud::COMPOSITE:
        {
            /* fait via lecture_composites */
            break;
        }
        case type_noeud::NUANCEUR:
        {
            /* fait via lecture_nuanceurs */
            break;
        }
        case type_noeud::RENDU:
        {
            /* fait via lecture_rendus */
            break;
        }
        case type_noeud::OPERATRICE:
        {
            auto const element_operatrice = element_noeud->FirstChildElement("operatrice");
            auto const nom_operatrice = element_operatrice->attribut("nom");
            auto const nom_detail = element_operatrice->attribut("detail");
            auto const type_detail = element_operatrice->attribut("type_detail");

            if (nom_detail == nullptr) {
                auto operatrice = (jorjala.usine_operatrices())(nom_operatrice, *graphe, *noeud);
                lecture_proprietes(element_operatrice, operatrice);
                operatrice->performe_versionnage();

                if (type_detail != nullptr && operatrice->type() == OPERATRICE_GRAPHE_DETAIL) {
                    auto op_detail = dynamic_cast<OperatriceGrapheDetail *>(operatrice);
                    op_detail->type_detail = std::atoi(type_detail);
                    /* il faut que le type de détail soit correct car
                     * l'opératrice n'est pas exécutée quand les noeuds sont
                     * ajoutés dans son graphe */
                    noeud->graphe.donnees.efface();
                    noeud->graphe.donnees.ajoute(op_detail->type_detail);
                }

                /* il nous faut savoir le type de détail avant de pouvoir synchroniser */
                synchronise_donnees_operatrice(*noeud);

                if (noeud->sorties.est_vide()) {
                    graphe->dernier_noeud_sortie = noeud;
                }

                auto element_graphe = element_operatrice->FirstChildElement("graphe");

                if (element_graphe != nullptr) {
                    lecture_graphe(
                        element_graphe, jorjala, &noeud->graphe, type_noeud::OPERATRICE);
                }
            }
            else {
                auto op = cree_op_detail(jorjala, *graphe, *noeud, nom_detail);
                synchronise_donnees_operatrice(*noeud);
                lecture_proprietes(element_operatrice, op);
            }

            break;
        }
    }

    noeud->pos_x(static_cast<float>(atoi(posx)));
    noeud->pos_y(static_cast<float>(atoi(posy)));

    auto const racine_prise_entree = element_noeud->FirstChildElement("prises_entree");
    auto element_prise_entree = racine_prise_entree->FirstChildElement("entree");

    for (; element_prise_entree != nullptr;
         element_prise_entree = element_prise_entree->NextSiblingElement("entree")) {
        auto const nom_prise = element_prise_entree->attribut("nom");
        auto const id_prise = element_prise_entree->attribut("id");

        int i = 0;
        while (true) {
            auto nom_connexion = "connexion" + dls::vers_chaine(i++);
            auto const connexion = element_prise_entree->attribut(nom_connexion.c_str());

            if (connexion == nullptr) {
                break;
            }

            donnees_connexion.tableau_connexion_id.ajoute({id_prise, connexion});
        }

        donnees_connexion.tableau_id_prise_entree[id_prise] = noeud->entree(nom_prise);
    }

    auto const racine_prise_sortie = element_noeud->FirstChildElement("prises_sortie");
    auto element_prise_sortie = racine_prise_sortie->FirstChildElement("sortie");

    for (; element_prise_sortie != nullptr;
         element_prise_sortie = element_prise_sortie->NextSiblingElement("sortie")) {
        auto const nom_prise = element_prise_sortie->attribut("nom");
        auto const id_prise = element_prise_sortie->attribut("id");

        donnees_connexion.tableau_id_prise_sortie[id_prise] = noeud->sortie(nom_prise);
    }
}

void lecture_graphe(dls::xml::Element *racine_graphe,
                    Jorjala &jorjala,
                    Graphe *graphe,
                    type_noeud type)
{
    graphe->centre_x = racine_graphe->FloatAttribut("centre_x");
    graphe->centre_y = racine_graphe->FloatAttribut("centre_y");
    graphe->zoom = racine_graphe->FloatAttribut("zoom");

    /* dans les exécutables en mode debug il arrive que les zoom inférieurs à 1
     * soient mis à zéro, peut-être que scanf a du mal ? */
    if (graphe->zoom == 0.0f) {
        graphe->zoom = 1.0f;
    }

    auto element_noeud = racine_graphe->FirstChildElement("noeud");

    DonneesConnexions donnees_connexions;

    for (; element_noeud != nullptr; element_noeud = element_noeud->NextSiblingElement("noeud")) {
        lecture_noeud(element_noeud, jorjala, graphe, donnees_connexions, type);
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

static void lecture_graphe_racine(dls::xml::Element *racine_graphe, Graphe *graphe)
{
    graphe->centre_x = racine_graphe->FloatAttribut("centre_x");
    graphe->centre_y = racine_graphe->FloatAttribut("centre_y");
    graphe->zoom = racine_graphe->FloatAttribut("zoom");

    /* dans les exécutables en mode debug il arrive que les zoom inférieurs à 1
     * soient mis à zéro, peut-être que scanf a du mal ? */
    if (graphe->zoom == 0.0f) {
        graphe->zoom = 1.0f;
    }

    auto element_noeud = racine_graphe->FirstChildElement("noeud");

    for (; element_noeud != nullptr; element_noeud = element_noeud->NextSiblingElement("noeud")) {
        auto const nom_noeud = element_noeud->attribut("nom");
        auto const posx = element_noeud->attribut("posx");
        auto const posy = element_noeud->attribut("posy");

        for (auto noeud : graphe->noeuds()) {
            if (noeud->nom == nom_noeud) {
                noeud->pos_x(static_cast<float>(atoi(posx)));
                noeud->pos_y(static_cast<float>(atoi(posy)));
            }
        }
    }
}

static void lis_objets(dls::xml::Element *racine_objets, Jorjala &jorjala)
{
    auto element_objet = racine_objets->FirstChildElement("objet");

    for (; element_objet != nullptr; element_objet = element_objet->NextSiblingElement("objet")) {
        auto const nom = element_objet->attribut("nom");
        auto const attr_type = element_objet->attribut("type");
        auto type = type_objet::CORPS;

        /* versionnage */
        if (attr_type != nullptr) {
            type = static_cast<type_objet>(std::atoi(attr_type));
        }

        auto objet = jorjala.bdd.cree_objet(nom, type);
        auto racine_graphe = element_objet->FirstChildElement("graphe");

        lecture_graphe(racine_graphe, jorjala, &objet->noeud->graphe, type_noeud::OPERATRICE);
        lecture_proprietes(element_objet, objet->noeud);

        objet->performe_versionnage();
    }
}

static void lis_composites(dls::xml::Element *racine_objets, Jorjala &jorjala)
{
    auto element_compo = racine_objets->FirstChildElement("composite");

    for (; element_compo != nullptr;
         element_compo = element_compo->NextSiblingElement("composite")) {
        auto const nom = element_compo->attribut("nom");
        auto composite = jorjala.bdd.cree_composite(nom);
        auto racine_graphe = element_compo->FirstChildElement("graphe");

        lecture_graphe(racine_graphe, jorjala, &composite->noeud->graphe, type_noeud::OPERATRICE);
    }
}

static void lis_nuanceurs(dls::xml::Element *racine_objets, Jorjala &jorjala)
{
    auto element_nuanceur = racine_objets->FirstChildElement("nuanceur");

    for (; element_nuanceur != nullptr;
         element_nuanceur = element_nuanceur->NextSiblingElement("nuanceur")) {
        auto const nom = element_nuanceur->attribut("nom");
        auto nuanceur = jorjala.bdd.cree_nuanceur(nom);
        auto racine_graphe = element_nuanceur->FirstChildElement("graphe");

        lecture_graphe(racine_graphe, jorjala, &nuanceur->noeud.graphe, type_noeud::OPERATRICE);
    }
}

static void lis_rendus(dls::xml::Element *racine_objets, Jorjala &jorjala)
{
    auto element_rendu = racine_objets->FirstChildElement("rendu");

    for (; element_rendu != nullptr; element_rendu = element_rendu->NextSiblingElement("rendu")) {
        auto const nom = element_rendu->attribut("nom");
        auto rendu = jorjala.bdd.cree_rendu(nom);
        auto racine_graphe = element_rendu->FirstChildElement("graphe");

        lecture_graphe(racine_graphe, jorjala, &rendu->noeud.graphe, type_noeud::OPERATRICE);
    }
}

static auto lis_etat(dls::xml::Document &doc, Jorjala &jorjala)
{
    auto elem_etat = doc.FirstChildElement("etat");

    if (elem_etat == nullptr) {
        /* versionnage */
        jorjala.graphe = jorjala.bdd.graphe_objets();
        jorjala.chemin_courant = "/objets/";
        return;
    }

    auto chemin = elem_etat->attribut("chemin_courant");
    jorjala.chemin_courant = chemin;

    jorjala.noeud = cherche_noeud_pour_chemin(jorjala.bdd, jorjala.chemin_courant);

    if (jorjala.noeud != nullptr) {
        jorjala.graphe = &jorjala.noeud->graphe;
    }
}

static auto reinitialise_jorjala(Jorjala &jorjala)
{
    jorjala.manipulation_2d_activee = false;
    jorjala.type_manipulation_2d = 0;
    jorjala.manipulatrice_2d = nullptr;
    jorjala.manipulation_3d_activee = false;
    jorjala.type_manipulation_3d = 0;
    jorjala.manipulatrice_3d = nullptr;
    jorjala.noeud = nullptr;
    jorjala.graphe = nullptr;
    jorjala.bdd.reinitialise();
}

static auto lis_fichier(filesystem::path const &chemin, Jorjala &jorjala)
{
    if (!std::filesystem::exists(chemin)) {
        return erreur_fichier::NON_TROUVE;
    }

    dls::xml::Document doc;
    doc.LoadFile(chemin.c_str());

    reinitialise_jorjala(jorjala);

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

    lis_objets(racine_objets, jorjala);

    auto const racine_graphe_objs = racine_objets->FirstChildElement("graphe");

    if (racine_graphe_objs != nullptr) {
        lecture_graphe_racine(racine_graphe_objs, jorjala.bdd.graphe_objets());
    }

    /* composites */
    auto const racine_composites = racine_projet->FirstChildElement("composites");

    if (racine_composites == nullptr) {
        return erreur_fichier::CORROMPU;
    }

    lis_composites(racine_composites, jorjala);

    auto const racine_graphe_comps = racine_composites->FirstChildElement("graphe");

    if (racine_graphe_comps != nullptr) {
        lecture_graphe_racine(racine_graphe_comps, jorjala.bdd.graphe_composites());
    }

    /* nuanceurs */
    auto const racine_nuanceurs = racine_projet->FirstChildElement("nuanceurs");

    if (racine_nuanceurs != nullptr) {
        lis_nuanceurs(racine_nuanceurs, jorjala);

        auto const racine_graphe_nuanceurs = racine_nuanceurs->FirstChildElement("graphe");

        if (racine_graphe_nuanceurs != nullptr) {
            lecture_graphe_racine(racine_graphe_nuanceurs, jorjala.bdd.graphe_nuanceurs());
        }
    }

    /* rendus */
    auto const racine_rendus = racine_projet->FirstChildElement("rendus");

    if (racine_rendus != nullptr) {
        lis_rendus(racine_rendus, jorjala);

        auto const racine_graphe_rendus = racine_rendus->FirstChildElement("graphe");

        if (racine_graphe_rendus != nullptr) {
            lecture_graphe_racine(racine_graphe_rendus, jorjala.bdd.graphe_rendus());
        }
    }
    else {
        /* versionnage */
        cree_rendu_defaut(jorjala);
    }

    lis_etat(doc, jorjala);

    requiers_evaluation(jorjala, FICHIER_OUVERT, "chargement d'un projet");

    jorjala.notifie_observatrices(type_evenement::rafraichissement);

    return erreur_fichier::AUCUNE_ERREUR;
}

void ouvre_projet(filesystem::path const &chemin, Jorjala &jorjala)
{
    auto erreur = lis_fichier(chemin, jorjala);

    switch (erreur) {
        case coeur::erreur_fichier::AUCUNE_ERREUR:
            break;
        case coeur::erreur_fichier::CORROMPU:
            jorjala.affiche_erreur("Le fichier est corrompu !");
            return;
        case coeur::erreur_fichier::NON_OUVERT:
            jorjala.affiche_erreur("Le fichier n'est pas ouvert !");
            return;
        case coeur::erreur_fichier::NON_TROUVE:
            jorjala.affiche_erreur("Le fichier n'a pas été trouvé !");
            return;
        case coeur::erreur_fichier::INCONNU:
            jorjala.affiche_erreur("Erreur inconnu !");
            return;
        case coeur::erreur_fichier::GREFFON_MANQUANT:
            jorjala.affiche_erreur("Le fichier ne pas être ouvert car il"
                                   " y a un greffon manquant !");
            return;
    }

    jorjala.chemin_projet(chemin.c_str());
    jorjala.projet_ouvert(true);

#if 0
	setWindowTitle(chemin_projet.c_str());
#endif
}

} /* namespace coeur */
