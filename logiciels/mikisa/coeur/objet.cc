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
	}
}

void Objet::performe_versionnage()
{
	if (this->propriete("pivot") == nullptr) {
		this->ajoute_propriete("pivot", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("position", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("echelle", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0f));
		this->ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f));
		this->ajoute_propriete("echelle_uniforme", danjo::TypePropriete::DECIMAL, 1.0f);
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

	auto pvt = evalue_vecteur("pivot");
	auto pos = evalue_vecteur("position");
	auto ech = evalue_vecteur("echelle");
	auto rot = evalue_vecteur("rotation");

	pivot = dls::math::point3f(pvt);
	position = dls::math::point3f(pos);
	echelle = dls::math::point3f(ech);
	rotation = dls::math::point3f(rot);
	echelle_uniforme = evalue_decimal("echelle_uniforme");

	transformation = math::construit_transformation(pos, rot, ech * echelle_uniforme);
}
