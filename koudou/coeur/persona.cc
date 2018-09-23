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

void Persona::ajoute_propriete(std::string nom, std::string nom_entreface, TypePropriete type)
{
	Propriete prop;
	prop.nom = std::move(nom);
	prop.nom_entreface = std::move(nom_entreface);
	prop.type = type;
	prop.visible = true;

	switch (type) {
		case TypePropriete::BOOL:
			prop.donnee = std::experimental::any(false);
			break;
		case TypePropriete::FLOAT:
			prop.donnee = std::experimental::any(0.0f);
			break;
		case TypePropriete::VEC3:
			prop.donnee = std::experimental::any(glm::vec3(0.0f));
			break;
		case TypePropriete::COULEUR:
			prop.donnee = std::experimental::any(glm::vec4(0.0f));
			break;
		case TypePropriete::ENUM:
		case TypePropriete::INT:
			prop.donnee = std::experimental::any(int(0));
			break;
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::STRING:
		case TypePropriete::LISTE:
			prop.donnee = std::experimental::any(std::string(""));
			break;
	}

	assert(!prop.donnee.empty());

	m_proprietes.push_back(std::move(prop));
}

void Persona::rend_visible(const std::string &nom_propriete, bool visible)
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

int Persona::evalue_int(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<int>(prop->donnee);
	}

	return 0;
}

float Persona::evalue_float(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<float>(prop->donnee);
	}

	return 0.0f;
}

int Persona::evalue_enum(const std::string &nom_propriete)
{
	return evalue_int(nom_propriete);
}

int Persona::evalue_bool(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<bool>(prop->donnee);
	}

	return false;
}

glm::vec3 Persona::evalue_vec3(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<glm::vec3>(prop->donnee);
	}

	return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec4 Persona::evalue_couleur(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<glm::vec4>(prop->donnee);
	}

	return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

std::string Persona::evalue_string(const std::string &nom_propriete)
{
	const Propriete *prop = trouve_propriete(nom_propriete);

	if (prop) {
		return std::experimental::any_cast<std::string>(prop->donnee);
	}

	return {};
}

void Persona::etablie_min_max(const float min, const float max)
{
	Propriete &prop = this->m_proprietes.back();
	prop.min = min;
	prop.max = max;
}

void Persona::etablie_valeur_enum(const ProprieteEnumerante &enum_prop)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::ENUM || prop.type == TypePropriete::LISTE);

	prop.items_enumeration = enum_prop;
}

void Persona::etablie_valeur_int_defaut(int valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::INT || prop.type == TypePropriete::ENUM);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::etablie_valeur_float_defaut(float valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::FLOAT);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::etablie_valeur_bool_defaut(bool valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::BOOL);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::etablie_valeur_string_defaut(const std::string &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::STRING ||
	       prop.type == TypePropriete::FICHIER_ENTREE ||
	       prop.type == TypePropriete::FICHIER_SORTIE ||
	       prop.type == TypePropriete::LISTE);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::etablie_valeur_vec3_defaut(const glm::vec3 &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::VEC3);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::etablie_valeur_couleur_defaut(const glm::vec4 &valeur)
{
	Propriete &prop = this->m_proprietes.back();

	assert(prop.type == TypePropriete::COULEUR);

	prop.donnee = std::experimental::any(valeur);
}

void Persona::ajourne_valeur_float(const std::string &nom_propriete, float valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<float>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_int(const std::string &nom_propriete, int valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<int>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_bool(const std::string &nom_propriete, bool valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<bool>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_couleur(const std::string &nom_propriete, const glm::vec4 &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<glm::vec4>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_string(const std::string &nom_propriete, const std::string &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<std::string>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::ajourne_valeur_enum(const std::string &nom_propriete, const ProprieteEnumerante &propriete)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	assert(prop->type == TypePropriete::ENUM || prop->type == TypePropriete::LISTE);

	prop->items_enumeration = propriete;
}

void Persona::ajourne_valeur_vec3(const std::string &nom_propriete, const glm::vec3 &valeur)
{
	Propriete *prop = trouve_propriete(nom_propriete);

	if (!prop) {
		return;
	}

	auto donnees = std::experimental::any_cast<glm::vec3>(&prop->donnee);
	(*donnees) = valeur;
}

void Persona::etablie_infobulle(std::string tooltip)
{
	Propriete &prop = this->m_proprietes.back();
	prop.infobulle = std::move(tooltip);
}

std::vector<Propriete> &Persona::proprietes()
{
	return m_proprietes;
}


