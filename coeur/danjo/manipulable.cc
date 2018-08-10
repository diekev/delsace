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

#include "manipulable.h"

#include "types/courbe_bezier.h"

namespace danjo {

/* ************************************************************************** */

void Propriete::ajoute_cle(const int v, int temps)
{
	assert(type == TypePropriete::ENTIER);
	ajoute_cle_impl(std::experimental::any(v), temps);
}

void Propriete::ajoute_cle(const float v, int temps)
{
	assert(type == TypePropriete::DECIMAL);
	ajoute_cle_impl(std::experimental::any(v), temps);
}

void Propriete::ajoute_cle(const glm::vec3 &v, int temps)
{
	assert(type == TypePropriete::VECTEUR);
	ajoute_cle_impl(std::experimental::any(v), temps);
}

void Propriete::ajoute_cle(const glm::vec4 &v, int temps)
{
	assert(type == TypePropriete::COULEUR);
	ajoute_cle_impl(std::experimental::any(v), temps);
}

void Propriete::supprime_animation()
{
	courbe.clear();
}

bool Propriete::est_anime() const
{
	return !courbe.empty();
}

bool Propriete::possede_cle(int temps) const
{
	for (const auto &valeur : courbe) {
		if (valeur.first == temps) {
			return true;
		}
	}

	return false;
}

int Propriete::evalue_entier(int temps)
{
	assert(type == TypePropriete::ENTIER);
	std::experimental::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::experimental::any_cast<int>(v1);
	}

	auto dt = t2 - t1;
	auto fac = (temps - t1) / static_cast<float>(dt);
	auto i1 = std::experimental::any_cast<int>(v1);
	auto i2 = std::experimental::any_cast<int>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

float Propriete::evalue_decimal(int temps)
{
	assert(type == TypePropriete::DECIMAL);
	std::experimental::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::experimental::any_cast<float>(v1);
	}

	auto dt = t2 - t1;
	auto fac = (temps - t1) / static_cast<float>(dt);
	auto i1 = std::experimental::any_cast<float>(v1);
	auto i2 = std::experimental::any_cast<float>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

glm::vec3 Propriete::evalue_vecteur(int temps)
{
	assert(type == TypePropriete::VECTEUR);
	std::experimental::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::experimental::any_cast<glm::vec3>(v1);
	}

	auto dt = t2 - t1;
	auto fac = (temps - t1) / static_cast<float>(dt);
	auto i1 = std::experimental::any_cast<glm::vec3>(v1);
	auto i2 = std::experimental::any_cast<glm::vec3>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

glm::vec4 Propriete::evalue_couleur(int temps)
{
	assert(type == TypePropriete::COULEUR);
	std::experimental::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::experimental::any_cast<glm::vec4>(v1);
	}

	auto dt = t2 - t1;
	auto fac = (temps - t1) / static_cast<float>(dt);
	auto i1 = std::experimental::any_cast<glm::vec4>(v1);
	auto i2 = std::experimental::any_cast<glm::vec4>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

void Propriete::ajoute_cle_impl(const std::experimental::any &v, int temps)
{
	bool insere = false;
	size_t i = 0;

	for (; i < courbe.size(); ++i) {
		if (courbe[i].first == temps) {
			courbe[i].second = v;
			insere = true;
			break;
		}

		if (courbe[i].first > temps) {
			courbe.insert(courbe.begin() + i, std::make_pair(temps, v));
			insere = true;
			break;
		}
	}

	if (!insere) {
		courbe.push_back(std::make_pair(temps, v));
	}
}

bool Propriete::trouve_valeurs_temps(int temps, std::experimental::any &v1, std::experimental::any &v2, int &t1, int &t2)
{
	bool v1_trouve = false;
	bool v2_trouve = false;

	for (size_t i = 0; i < courbe.size(); ++i) {
		if (courbe[i].first < temps) {
			v1 = courbe[i].second;
			t1 = courbe[i].first;
			v1_trouve = true;
			continue;
		}

		if (courbe[i].first == temps) {
			v1 = courbe[i].second;
			return true;
		}

		if (courbe[i].first > temps) {
			v2 = courbe[i].second;
			t2 = courbe[i].first;
			v2_trouve = true;
			break;
		}
	}

	if (!v1_trouve) {
		v1 = v2;
	}

	if (!v2_trouve) {
		v2 = v1;
	}

	return false;
}

/* ************************************************************************** */

Manipulable::iterateur Manipulable::debut()
{
	return m_proprietes.begin();
}

Manipulable::iterateur Manipulable::fin()
{
	return m_proprietes.end();
}

void Manipulable::ajoute_propriete(const std::string &nom, TypePropriete type, const std::experimental::any &valeur)
{
	m_proprietes.insert({nom, {valeur, type}});
}

void Manipulable::ajoute_propriete(const std::string &nom, TypePropriete type)
{
	std::experimental::any valeur;

	switch (type) {
		default:
		case TypePropriete::ENTIER:
			valeur = std::experimental::any(0);
			break;
		case TypePropriete::DECIMAL:
			valeur = std::experimental::any(0.0f);
			break;
		case TypePropriete::VECTEUR:
			valeur = std::experimental::any(glm::vec3(0));
			break;
		case TypePropriete::COULEUR:
			valeur = std::experimental::any(glm::vec4(0));
			break;
		case TypePropriete::ENUM:
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::CHAINE_CARACTERE:
			valeur = std::experimental::any(std::string(""));
			break;
		case TypePropriete::BOOL:
			valeur = std::experimental::any(false);
			break;
		case TypePropriete::COURBE_COULEUR:
			valeur = std::experimental::any(CourbeCouleur());
			break;
		case TypePropriete::COURBE_VALEUR:
		{
			auto courbe = CourbeBezier();
			cree_courbe_defaut(courbe);
			valeur = std::experimental::any(courbe);
			break;
		}
	}

	m_proprietes[nom] = Propriete{valeur, type};
}

void Manipulable::ajoute_propriete_extra(const std::string &nom, const Propriete &propriete)
{
	Propriete prop = propriete;
	prop.est_extra = true;
	m_proprietes[nom] = prop;
}

int Manipulable::evalue_entier(const std::string &nom, int temps)
{
	Propriete &prop = m_proprietes[nom];

	if (prop.est_anime()) {
		return prop.evalue_entier(temps);
	}

	return std::experimental::any_cast<int>(prop.valeur);
}

float Manipulable::evalue_decimal(const std::string &nom, int temps)
{
	Propriete &prop = m_proprietes[nom];

	if (prop.est_anime()) {
		return prop.evalue_decimal(temps);
	}

	return std::experimental::any_cast<float>(prop.valeur);
}

glm::vec3 Manipulable::evalue_vecteur(const std::string &nom, int temps)
{
	Propriete &prop = m_proprietes[nom];

	if (prop.est_anime()) {
		return prop.evalue_vecteur(temps);
	}

	return std::experimental::any_cast<glm::vec3>(prop.valeur);
}

glm::vec4 Manipulable::evalue_couleur(const std::string &nom, int temps)
{
	Propriete &prop = m_proprietes[nom];

	if (prop.est_anime()) {
		return prop.evalue_couleur(temps);
	}

	return std::experimental::any_cast<glm::vec4>(prop.valeur);
}

std::string Manipulable::evalue_fichier_entree(const std::string &nom)
{
	return std::experimental::any_cast<std::string>(m_proprietes[nom].valeur);
}

std::string Manipulable::evalue_fichier_sortie(const std::string &nom)
{
	return std::experimental::any_cast<std::string>(m_proprietes[nom].valeur);
}

std::string Manipulable::evalue_chaine(const std::string &nom)
{
	return std::experimental::any_cast<std::string>(m_proprietes[nom].valeur);
}

bool Manipulable::evalue_bool(const std::string &nom)
{
	return std::experimental::any_cast<bool>(m_proprietes[nom].valeur);
}

std::string Manipulable::evalue_enum(const std::string &nom)
{
	return std::experimental::any_cast<std::string>(m_proprietes[nom].valeur);
}

std::string Manipulable::evalue_liste(const std::string &nom)
{
	return std::experimental::any_cast<std::string>(m_proprietes[nom].valeur);
}

CourbeCouleur *Manipulable::evalue_courbe_couleur(const std::string &nom)
{
	return std::experimental::any_cast<CourbeCouleur>(&m_proprietes[nom].valeur);
}

CourbeBezier *Manipulable::evalue_courbe_valeur(const std::string &nom)
{
	return std::experimental::any_cast<CourbeBezier>(&m_proprietes[nom].valeur);
}

void Manipulable::rend_propriete_visible(const std::string &nom, bool ouinon)
{
	m_proprietes[nom].visible = ouinon;
}

bool Manipulable::ajourne_proprietes()
{
	return true;
}

void Manipulable::valeur_bool(const std::string &nom, bool valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_entier(const std::string &nom, int valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_decimal(const std::string &nom, float valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_vecteur(const std::string &nom, const glm::vec3 &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_couleur(const std::string &nom, const glm::vec4 &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_chaine(const std::string &nom, const std::string &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void *Manipulable::operator[](const std::string &nom)
{
	auto &propriete = m_proprietes[nom];
	void *pointeur = nullptr;

	switch (propriete.type) {
		case TypePropriete::ENTIER:
			pointeur = std::experimental::any_cast<int>(&propriete.valeur);
			break;
		case TypePropriete::DECIMAL:
			pointeur = std::experimental::any_cast<float>(&propriete.valeur);
			break;
		case TypePropriete::VECTEUR:
			pointeur = std::experimental::any_cast<glm::vec3>(&propriete.valeur);
			break;
		case TypePropriete::COULEUR:
			pointeur = std::experimental::any_cast<glm::vec4>(&propriete.valeur);
			break;
		case TypePropriete::ENUM:
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::CHAINE_CARACTERE:
			pointeur = std::experimental::any_cast<std::string>(&propriete.valeur);
			break;
		case TypePropriete::BOOL:
			pointeur = std::experimental::any_cast<bool>(&propriete.valeur);
			break;
		case TypePropriete::COURBE_COULEUR:
			pointeur = std::experimental::any_cast<CourbeCouleur>(&propriete.valeur);
			break;
		case TypePropriete::COURBE_VALEUR:
			pointeur = std::experimental::any_cast<CourbeBezier>(&propriete.valeur);
			break;
	}

	return pointeur;
}

TypePropriete Manipulable::type_propriete(const std::string &nom)
{
	const auto &propriete = m_proprietes[nom];
	return propriete.type;
}

Propriete *Manipulable::propriete(const std::string &nom)
{
	auto iter = m_proprietes.find(nom);

	if (iter == m_proprietes.end()) {
		return nullptr;
	}

	return &(iter->second);
}

}  /* namespace danjo */
