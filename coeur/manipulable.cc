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
			valeur = std::experimental::any(0);
			break;
		case TypePropriete::COULEUR:
			valeur = std::experimental::any(0);
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
	}

	m_proprietes.insert({nom, {valeur, type}});
}

int Manipulable::evalue_entier(const std::string &nom)
{
	return std::experimental::any_cast<int>(m_proprietes[nom].valeur);
}

float Manipulable::evalue_decimal(const std::string &nom)
{
	return std::experimental::any_cast<float>(m_proprietes[nom].valeur);
}

int Manipulable::evalue_vecteur(const std::string &nom)
{
	return std::experimental::any_cast<int>(m_proprietes[nom].valeur);
}

int Manipulable::evalue_couleur(const std::string &nom)
{
	return std::experimental::any_cast<int>(m_proprietes[nom].valeur);
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

int Manipulable::evalue_liste(const std::string &nom)
{
	return std::experimental::any_cast<int>(m_proprietes[nom].valeur);
}

void *Manipulable::operator[](const std::string &nom)
{
	auto &propriete = m_proprietes[nom];
	void *pointeur = nullptr;

	switch (propriete.type) {
		default:
		case TypePropriete::ENTIER:
			pointeur = std::experimental::any_cast<int>(&propriete.valeur);
			break;
		case TypePropriete::DECIMAL:
			pointeur = std::experimental::any_cast<float>(&propriete.valeur);
			break;
		case TypePropriete::VECTEUR:
			pointeur = std::experimental::any_cast<int>(&propriete.valeur);
			break;
		case TypePropriete::COULEUR:
			pointeur = std::experimental::any_cast<int>(&propriete.valeur);
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
	}

	return pointeur;
}
