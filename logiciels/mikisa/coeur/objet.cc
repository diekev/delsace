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

#include "operatrice_image.h"

Objet::Objet()
	: graphe(cree_noeud_image, supprime_noeud_image)
{}

void Objet::performe_versionnage()
{
	if (this->propriete("pivot") == nullptr) {
		this->ajoute_propriete("pivot", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("position", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("echelle", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0f));
		this->ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("echelle_uniforme", danjo::TypePropriete::DECIMAL, dls::math::vec3f(1.0f));
		this->ajoute_propriete("nom", danjo::TypePropriete::CHAINE_CARACTERE, dls::chaine("objet"));
		this->ajoute_propriete("rendu_scene", danjo::TypePropriete::BOOL, true);
	}
}

const char *Objet::chemin_entreface() const
{
	return "entreface/objet.jo";
}

void Objet::ajourne_parametres()
{
	rendu_scene = evalue_bool("rendu_scene");
	pivot = dls::math::point3f(evalue_vecteur("pivot"));
	position = dls::math::point3f(evalue_vecteur("position"));
	echelle = dls::math::point3f(evalue_vecteur("echelle"));
	rotation = dls::math::point3f(evalue_vecteur("rotation"));
	echelle_uniforme = evalue_decimal("echelle_uniforme");

	transformation = math::transformation();
	transformation *= math::translation(position.x, position.y, position.z);
	transformation *= math::rotation_x(rotation.x * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_y(rotation.y * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_z(rotation.z * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::echelle(echelle.x * echelle_uniforme, echelle.y * echelle_uniforme, echelle.z * echelle_uniforme);
}
