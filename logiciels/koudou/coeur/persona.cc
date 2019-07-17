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

#include "persona.h"

#include <cassert>

void Persona::ajoute_propriete(dls::chaine nom, dls::chaine nom_entreface, TypePropriete type)
{
	Propriete prop;
	prop.nom = std::move(nom);
	prop.nom_entreface = std::move(nom_entreface);
	prop.type = type;
	prop.visible = true;

	switch (type) {
		case TypePropriete::BOOL:
			prop.donnee = std::any(false);
			break;
		case TypePropriete::FLOAT:
			prop.donnee = std::any(0.0f);
			break;
		case TypePropriete::VEC3:
			prop.donnee = std::any(dls::math::vec3f(0.0f));
			break;
		case TypePropriete::COULEUR:
			prop.donnee = std::any(dls::math::vec4f(0.0f));
			break;
		case TypePropriete::ENUM:
		case TypePropriete::INT:
			prop.donnee = std::any(0);
			break;
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::STRING:
		case TypePropriete::LISTE:
			prop.donnee = std::any(dls::chaine(""));
			break;
	}

	assert(prop.donnee.has_value());

	m_proprietes.pousse(std::move(prop));
}

void Persona::rend_visible(dls::chaine const &nom_propriete, bool visible)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		prop->visible = visible;
	}
}

bool Persona::ajourne_proprietes()
{
	return false;
}

int Persona::evalue_int(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<int>(prop->donnee);
	}

	return 0;
}

float Persona::evalue_float(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<float>(prop->donnee);
	}

	return 0.0f;
}

int Persona::evalue_enum(dls::chaine const &nom_propriete)
{
	return evalue_int(nom_propriete);
}

int Persona::evalue_bool(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<bool>(prop->donnee);
	}

	return false;
}

dls::math::vec3f Persona::evalue_vec3(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<dls::math::vec3f>(prop->donnee);
	}

	return dls::math::vec3f(0.0f, 0.0f, 0.0f);
}

dls::math::vec4f Persona::evalue_couleur(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<dls::math::vec4f>(prop->donnee);
	}

	return dls::math::vec4f(0.0f, 0.0f, 0.0f, 0.0f);
}

dls::chaine Persona::evalue_string(dls::chaine const &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::any_cast<dls::chaine>(prop->donnee);
	}

	return {};
}

void Persona::etablie_min_max(const float min, const float max)
{
	Propriete &prop = this->m_proprietes.back();
	prop.min = min;
	prop.max = max;
}

void Persona::etablie_valeur_enum(ProprieteEnumerante const &enum_prop)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::ENUM || prop.type == TypePropriete::LISTE);

	prop.items_enumeration = enum_prop;
}

void Persona::etablie_valeur_int_defaut(int valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::INT || prop.type == TypePropriete::ENUM);

	prop.donnee = std::any(valeur);
}

void Persona::etablie_valeur_float_defaut(float valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::FLOAT);

	prop.donnee = std::any(valeur);
}

void Persona::etablie_valeur_bool_defaut(bool valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::BOOL);

	prop.donnee = std::any(valeur);
}

void Persona::etablie_valeur_string_defaut(dls::chaine const &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::STRING ||
	       prop.type == TypePropriete::FICHIER_ENTREE ||
	       prop.type == TypePropriete::FICHIER_SORTIE ||
	       prop.type == TypePropriete::LISTE);

	prop.donnee = std::any(valeur);
}

void Persona::etablie_valeur_vec3_defaut(dls::math::vec3f const &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::VEC3);

	prop.donnee = std::any(valeur);
}

void Persona::etablie_valeur_couleur_defaut(dls::math::vec4f const &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::COULEUR);

	prop.donnee = std::any(valeur);
}

void Persona::ajourne_valeur_float(dls::chaine const &nom_propriete, float valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<float>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_int(dls::chaine const &nom_propriete, int valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<int>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_bool(dls::chaine const &nom_propriete, bool valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<bool>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_couleur(dls::chaine const &nom_propriete, dls::math::vec4f const &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<dls::math::vec4f>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_string(dls::chaine const &nom_propriete, dls::chaine const &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<dls::chaine>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_enum(dls::chaine const &nom_propriete, ProprieteEnumerante const &propriete)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	assert(prop->type == TypePropriete::ENUM || prop->type == TypePropriete::LISTE);

	prop->items_enumeration = propriete;
}

void Persona::ajourne_valeur_vec3(dls::chaine const &nom_propriete, dls::math::vec3f const &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::any_cast<dls::math::vec3f>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::etablie_infobulle(dls::chaine tooltip)
{
	Propriete &prop = this->m_proprietes.back();
	prop.infobulle = std::move(tooltip);
}

dls::tableau<Propriete> &Persona::proprietes()
{
	return m_proprietes;
}


