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

#include "objet.h"

#include "biblinternes/outils/constantes.h"

#include "noeud.hh"
#include "operatrice_image.h"

Objet::Objet()
{}

Objet::~Objet()
{
	switch (this->type) {
		case type_objet::NUL:
		{
			break;
		}
		case type_objet::CORPS:
		{
			auto ptr = static_cast<DonneesCorps *>(donnees.m_ptr);
			memoire::deloge("DonneesCorps", ptr);
			break;
		}
		case type_objet::CAMERA:
		{
			auto ptr = static_cast<DonneesCamera *>(donnees.m_ptr);
			memoire::deloge("DonneesCamera", ptr);
			break;
		}
		case type_objet::LUMIERE:
		{
			auto ptr = static_cast<DonneesLumiere *>(donnees.m_ptr);
			memoire::deloge("DonneesLumiere", ptr);
			break;
		}
	}
}

void Objet::performe_versionnage()
{
	if (this->noeud->propriete("pivot") == nullptr) {
		this->noeud->ajoute_propriete("pivot", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->noeud->ajoute_propriete("position", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->noeud->ajoute_propriete("echelle", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0f));
		this->noeud->ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->noeud->ajoute_propriete("echelle_uniforme", danjo::TypePropriete::DECIMAL, 1.0f);
		this->noeud->ajoute_propriete("nom", danjo::TypePropriete::CHAINE_CARACTERE, dls::chaine("objet"));
		this->noeud->ajoute_propriete("rendu_scene", danjo::TypePropriete::BOOL, true);
	}
}

const char *Objet::chemin_entreface() const
{
	switch (this->type) {
		case type_objet::NUL:
		case type_objet::CORPS:
		{
			break;
		}
		case type_objet::CAMERA:
		{
			return "entreface/objet_camera.jo";
		}
		case type_objet::LUMIERE:
		{
			return "entreface/objet_lumiere.jo";
		}
	}

	return "entreface/objet.jo";
}

void Objet::ajourne_parametres()
{
	rendu_scene = noeud->evalue_bool("rendu_scene");

	auto pvt = noeud->evalue_vecteur("pivot");
	auto pos = noeud->evalue_vecteur("position");
	auto ech = noeud->evalue_vecteur("echelle");
	auto rot = noeud->evalue_vecteur("rotation");

	pivot = dls::math::point3f(pvt);
	position = dls::math::point3f(pos);
	echelle = dls::math::point3f(ech);
	rotation = dls::math::point3f(rot);
	echelle_uniforme = noeud->evalue_decimal("echelle_uniforme");

	if (this->type == type_objet::CAMERA) {
		/* N'applique pas de transformation car la caméra prend en charge la
		 * position et la rotation.
		 * À FAIRE : pivot caméra. */
		transformation = math::transformation();

		this->donnees.accede_ecriture([this, &pos, &rot](DonneesObjet *donnees_)
		{
			auto &camera = static_cast<DonneesCamera *>(donnees_)->camera;

			auto const largeur = noeud->evalue_entier("largeur");
			auto const hauteur = noeud->evalue_entier("hauteur");
			auto const longueur_focale = noeud->evalue_decimal("longueur_focale");
			auto const largeur_senseur = noeud->evalue_decimal("largeur_senseur");
			auto const proche = noeud->evalue_decimal("proche");
			auto const eloigne = noeud->evalue_decimal("éloigné");
			auto const projection = noeud->evalue_enum("projection");

			if (projection == "perspective") {
				camera.projection(vision::TypeProjection::PERSPECTIVE);
			}
			else if (projection == "orthographique") {
				camera.projection(vision::TypeProjection::ORTHOGRAPHIQUE);
			}

			camera.redimensionne(largeur, hauteur);
			camera.longueur_focale(longueur_focale);
			camera.largeur_senseur(largeur_senseur);
			camera.profondeur(proche, eloigne);
			camera.position(pos);
			camera.rotation(rot * constantes<float>::POIDS_DEG_RAD);
			camera.ajourne_pour_operatrice();
		});
	}
	else if (this->type == type_objet::LUMIERE) {
		transformation = math::construit_transformation(pos, rot, ech * echelle_uniforme);

		this->donnees.accede_ecriture([this](DonneesObjet *donnees_)
		{
			auto &lumiere = extrait_lumiere(donnees_);

			auto type_lum = noeud->evalue_enum("type");

			if (type_lum == "point") {
				lumiere.type = LUMIERE_POINT;
			}
			else if (type_lum == "distante") {
				lumiere.type = LUMIERE_DISTANTE;
			}

			lumiere.intensite = noeud->evalue_decimal("intensité");
			lumiere.spectre = noeud->evalue_couleur("spectre");
		});
	}
	else {
		transformation = math::construit_transformation(pos, rot, ech * echelle_uniforme);
	}
}
