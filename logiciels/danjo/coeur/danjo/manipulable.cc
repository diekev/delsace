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

#include <assert.h>

#include "types/courbe_bezier.h"
#include "types/liste_manip.hh"
#include "types/rampe_couleur.h"

namespace danjo {

/* ************************************************************************** */

void Propriete::ajoute_cle(const int v, int temps)
{
	assert(type == TypePropriete::ENTIER);
	ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const float v, int temps)
{
	assert(type == TypePropriete::DECIMAL);
	ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const dls::math::vec3f &v, int temps)
{
	assert(type == TypePropriete::VECTEUR);
	ajoute_cle_impl(std::any(v), temps);
}

void Propriete::ajoute_cle(const dls::phys::couleur32 &v, int temps)
{
	assert(type == TypePropriete::COULEUR);
	ajoute_cle_impl(std::any(v), temps);
}

void Propriete::supprime_animation()
{
	courbe.efface();
}

bool Propriete::est_animee() const
{
	return !courbe.est_vide();
}

bool Propriete::possede_cle(int temps) const
{
	for (const auto &val : courbe) {
		if (val.first == temps) {
			return true;
		}
	}

	return false;
}

int Propriete::evalue_entier(int temps) const
{
	assert(type == TypePropriete::ENTIER);
	std::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::any_cast<int>(v1);
	}

	auto dt = t2 - t1;
	auto fac = static_cast<float>(temps - t1) / static_cast<float>(dt);
	auto i1 = static_cast<float>(std::any_cast<int>(v1));
	auto i2 = static_cast<float>(std::any_cast<int>(v2));

	return static_cast<int>((1.0f - fac) * i1 + fac * i2);
}

float Propriete::evalue_decimal(int temps) const
{
	assert(type == TypePropriete::DECIMAL);
	std::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::any_cast<float>(v1);
	}

	auto dt = t2 - t1;
	auto fac = static_cast<float>(temps - t1) / static_cast<float>(dt);
	auto i1 = std::any_cast<float>(v1);
	auto i2 = std::any_cast<float>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

dls::math::vec3f Propriete::evalue_vecteur(int temps) const
{
	assert(type == TypePropriete::VECTEUR);
	std::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::any_cast<dls::math::vec3f>(v1);
	}

	auto dt = t2 - t1;
	auto fac = static_cast<float>(temps - t1) / static_cast<float>(dt);
	auto i1 = std::any_cast<dls::math::vec3f>(v1);
	auto i2 = std::any_cast<dls::math::vec3f>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

dls::phys::couleur32 Propriete::evalue_couleur(int temps) const
{
	assert(type == TypePropriete::COULEUR);
	std::any v1, v2;
	int t1, t2;

	if (trouve_valeurs_temps(temps, v1, v2, t1, t2)) {
		return std::any_cast<dls::phys::couleur32>(v1);
	}

	auto dt = t2 - t1;
	auto fac = static_cast<float>(temps - t1) / static_cast<float>(dt);
	auto i1 = std::any_cast<dls::phys::couleur32>(v1);
	auto i2 = std::any_cast<dls::phys::couleur32>(v2);

	return (1.0f - fac) * i1 + fac * i2;
}

void Propriete::ajoute_cle_impl(const std::any &v, int temps)
{
	bool insere = false;
	long i = 0;

	for (; i < courbe.taille(); ++i) {
		if (courbe[i].first == temps) {
			courbe[i].second = v;
			insere = true;
			break;
		}

		if (courbe[i].first > temps) {
			courbe.insere(courbe.debut() + i, std::make_pair(temps, v));
			insere = true;
			break;
		}
	}

	if (!insere) {
		courbe.pousse(std::make_pair(temps, v));
	}
}

bool Propriete::trouve_valeurs_temps(int temps, std::any &v1, std::any &v2, int &t1, int &t2) const
{
	bool v1_trouve = false;
	bool v2_trouve = false;

	for (auto i = 0; i < courbe.taille(); ++i) {
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
	return m_proprietes.debut();
}

Manipulable::iterateur_const Manipulable::debut() const
{
	return m_proprietes.debut();
}

Manipulable::iterateur Manipulable::fin()
{
	return m_proprietes.fin();
}

Manipulable::iterateur_const Manipulable::fin() const
{
	return m_proprietes.fin();
}

void Manipulable::ajoute_propriete(const dls::chaine &nom, TypePropriete type, const std::any &valeur)
{
	m_proprietes.insere({nom, {valeur, type}});
}

void Manipulable::ajoute_propriete(const dls::chaine &nom, TypePropriete type)
{
	std::any valeur;

	switch (type) {
		case TypePropriete::ENTIER:
			valeur = std::any(0);
			break;
		case TypePropriete::DECIMAL:
			valeur = std::any(0.0f);
			break;
		case TypePropriete::VECTEUR:
			valeur = std::any(dls::math::vec3f(0));
			break;
		case TypePropriete::COULEUR:
			valeur = std::any(dls::phys::couleur32(0));
			break;
		case TypePropriete::ENUM:
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::CHAINE_CARACTERE:
		case TypePropriete::TEXTE:
			valeur = std::any(dls::chaine(""));
			break;
		case TypePropriete::BOOL:
			valeur = std::any(false);
			break;
		case TypePropriete::COURBE_COULEUR:
			valeur = std::any(CourbeCouleur());
			break;
		case TypePropriete::COURBE_VALEUR:
		{
			auto courbe = CourbeBezier();
			cree_courbe_defaut(courbe);
			valeur = std::any(courbe);
			break;
		}
		case TypePropriete::RAMPE_COULEUR:
		{
			auto rampe = RampeCouleur();
			cree_rampe_defaut(rampe);
			valeur = std::any(rampe);
			break;
		}
		case TypePropriete::LISTE_MANIP:
		{
			auto liste = ListeManipulable();
			valeur = std::any(liste);
			break;
		}
	}

	m_proprietes[nom] = Propriete{valeur, type};
}

void Manipulable::ajoute_propriete_extra(const dls::chaine &nom, const Propriete &propriete)
{
	Propriete prop = propriete;
	prop.est_extra = true;
	m_proprietes[nom] = prop;
}

int Manipulable::evalue_entier(const dls::chaine &nom, int temps) const
{
	auto prop = propriete(nom);

	if (prop->est_animee()) {
		return prop->evalue_entier(temps);
	}

	return std::any_cast<int>(prop->valeur);
}

float Manipulable::evalue_decimal(const dls::chaine &nom, int temps) const
{
	auto prop = propriete(nom);

	if (prop->est_animee()) {
		return prop->evalue_decimal(temps);
	}

	return std::any_cast<float>(prop->valeur);
}

dls::math::vec3f Manipulable::evalue_vecteur(const dls::chaine &nom, int temps) const
{
	auto prop = propriete(nom);

	if (prop->est_animee()) {
		return prop->evalue_vecteur(temps);
	}

	return std::any_cast<dls::math::vec3f>(prop->valeur);
}

dls::phys::couleur32 Manipulable::evalue_couleur(const dls::chaine &nom, int temps) const
{
	auto prop = propriete(nom);

	if (prop->est_animee()) {
		return prop->evalue_couleur(temps);
	}

	return std::any_cast<dls::phys::couleur32>(prop->valeur);
}

dls::chaine Manipulable::evalue_fichier_entree(const dls::chaine &nom) const
{
	return std::any_cast<dls::chaine>(propriete(nom)->valeur);
}

dls::chaine Manipulable::evalue_fichier_sortie(const dls::chaine &nom) const
{
	return std::any_cast<dls::chaine>(propriete(nom)->valeur);
}

dls::chaine Manipulable::evalue_chaine(const dls::chaine &nom) const
{
	return std::any_cast<dls::chaine>(propriete(nom)->valeur);
}

bool Manipulable::evalue_bool(const dls::chaine &nom) const
{
	return std::any_cast<bool>(propriete(nom)->valeur);
}

dls::chaine Manipulable::evalue_enum(const dls::chaine &nom) const
{
	return std::any_cast<dls::chaine>(propriete(nom)->valeur);
}

dls::chaine Manipulable::evalue_liste(const dls::chaine &nom) const
{
	return std::any_cast<dls::chaine>(propriete(nom)->valeur);
}

CourbeCouleur const *Manipulable::evalue_courbe_couleur(const dls::chaine &nom) const
{
	return std::any_cast<CourbeCouleur>(&propriete(nom)->valeur);
}

CourbeBezier const *Manipulable::evalue_courbe_valeur(const dls::chaine &nom) const
{
	return std::any_cast<CourbeBezier>(&propriete(nom)->valeur);
}

RampeCouleur const *Manipulable::evalue_rampe_couleur(const dls::chaine &nom) const
{
	return std::any_cast<RampeCouleur>(&propriete(nom)->valeur);
}

ListeManipulable const *Manipulable::evalue_liste_manip(const dls::chaine &nom) const
{
	return std::any_cast<ListeManipulable>(&propriete(nom)->valeur);
}

void Manipulable::rend_propriete_visible(const dls::chaine &nom, bool ouinon)
{
	m_proprietes[nom].visible = ouinon;
}

bool Manipulable::ajourne_proprietes()
{
	return true;
}

void Manipulable::valeur_bool(const dls::chaine &nom, bool valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_entier(const dls::chaine &nom, int valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_decimal(const dls::chaine &nom, float valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_vecteur(const dls::chaine &nom, const dls::math::vec3f &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_couleur(const dls::chaine &nom, const dls::phys::couleur32 &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void Manipulable::valeur_chaine(const dls::chaine &nom, const dls::chaine &valeur)
{
	m_proprietes[nom].valeur = valeur;
}

void *Manipulable::operator[](const dls::chaine &nom)
{
	auto &propriete = m_proprietes[nom];
	void *pointeur = nullptr;

	switch (propriete.type) {
		case TypePropriete::ENTIER:
			pointeur = std::any_cast<int>(&propriete.valeur);
			break;
		case TypePropriete::DECIMAL:
			pointeur = std::any_cast<float>(&propriete.valeur);
			break;
		case TypePropriete::VECTEUR:
			pointeur = std::any_cast<dls::math::vec3f>(&propriete.valeur);
			break;
		case TypePropriete::COULEUR:
			pointeur = std::any_cast<dls::phys::couleur32>(&propriete.valeur);
			break;
		case TypePropriete::ENUM:
		case TypePropriete::FICHIER_ENTREE:
		case TypePropriete::FICHIER_SORTIE:
		case TypePropriete::CHAINE_CARACTERE:
		case TypePropriete::TEXTE:
			pointeur = std::any_cast<dls::chaine>(&propriete.valeur);
			break;
		case TypePropriete::BOOL:
			pointeur = std::any_cast<bool>(&propriete.valeur);
			break;
		case TypePropriete::COURBE_COULEUR:
			pointeur = std::any_cast<CourbeCouleur>(&propriete.valeur);
			break;
		case TypePropriete::COURBE_VALEUR:
			pointeur = std::any_cast<CourbeBezier>(&propriete.valeur);
			break;
		case TypePropriete::RAMPE_COULEUR:
			pointeur = std::any_cast<RampeCouleur>(&propriete.valeur);
			break;
		case TypePropriete::LISTE_MANIP:
			pointeur = std::any_cast<ListeManipulable>(&propriete.valeur);
			break;
	}

	return pointeur;
}

TypePropriete Manipulable::type_propriete(const dls::chaine &nom) const
{
	auto prop = propriete(nom);
	return prop->type;
}

Propriete *Manipulable::propriete(const dls::chaine &nom)
{
	auto iter = m_proprietes.trouve(nom);

	if (iter == m_proprietes.fin()) {
		return nullptr;
	}

	return &(iter->second);
}

Propriete const *Manipulable::propriete(const dls::chaine &nom) const
{
	auto iter = m_proprietes.trouve(nom);

	if (iter == m_proprietes.fin()) {
		return nullptr;
	}

	return &(iter->second);
}

bool Manipulable::possede_animation() const
{
	for (auto iter = this->debut(); iter != this->fin(); ++iter) {
		auto &prop = iter->second;

		if (prop.est_animee()) {
			return true;
		}
	}

	return false;
}

void Manipulable::performe_versionnage()
{
}

}  /* namespace danjo */
