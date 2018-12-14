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

#pragma once

#include <algorithm>
#include <delsace/math/vecteur.hh>
#include <experimental/any>
#include <iostream>
#include <string>
#include <vector>

enum class TypePropriete {
	BOOL,
	INT,
	ENUM,
	FLOAT,
	VEC3,
	STRING,
	FICHIER_ENTREE,
	FICHIER_SORTIE,
	LISTE,
	COULEUR,
};

struct PaireNomValeur {
	std::string nom{};
	int valeur{};
};

struct ProprieteEnumerante {
	std::vector<PaireNomValeur> paires{};

	void ajoute(std::string nom, int valeur)
	{
		PaireNomValeur paire;
		paire.nom = std::move(nom);
		paire.valeur = valeur;

		paires.push_back(std::move(paire));
	}
};

struct Propriete {
	std::string nom{};
	std::string nom_entreface{};
	std::string infobulle{};
	TypePropriete type{};

	std::experimental::any donnee{};
	std::experimental::any valeur_defaut{};

	ProprieteEnumerante items_enumeration{};

	float min{}, max{};
	bool visible{};
};

/**
 * Classe de base pour définir des objets qui ont des propriétés montrées dans
 * l'entreface utilisateur.
 */
class Persona {
	std::vector<Propriete> m_proprietes{};

public:
	virtual ~Persona() = default;

	void ajoute_propriete(std::string nom, std::string nom_entreface, TypePropriete type);

	/* Modification des propriétés. */

	void rend_visible(const std::string &nom_propriete, bool visible);

	void etablie_infobulle(std::string tooltip);

	void etablie_min_max(const float min, const float max);

	void etablie_valeur_enum(const ProprieteEnumerante &enum_prop);

	void etablie_valeur_int_defaut(int valeur);

	void etablie_valeur_float_defaut(float valeur);

	void etablie_valeur_bool_defaut(bool valeur);

	void etablie_valeur_string_defaut(const std::string &valeur);

	void etablie_valeur_vec3_defaut(const dls::math::vec3f &valeur);

	void etablie_valeur_couleur_defaut(const dls::math::vec4f &valeur);

	void ajourne_valeur_float(const std::string &nom_propriete, float valeur);

	void ajourne_valeur_int(const std::string &nom_propriete, int valeur);

	void ajourne_valeur_bool(const std::string &nom_propriete, bool valeur);

	void ajourne_valeur_string(const std::string &nom_propriete, const std::string &valeur);

	void ajourne_valeur_couleur(const std::string &nom_propriete, const dls::math::vec4f &valeur);

	void ajourne_valeur_vec3(const std::string &nom_propriete, const dls::math::vec3f &valeur);

	void ajourne_valeur_enum(const std::string &nom_propriete, const ProprieteEnumerante &propriete);

	/* Évaluation. */

	int evalue_int(const std::string &nom_propriete);

	float evalue_float(const std::string &nom_propriete);

	int evalue_enum(const std::string &nom_propriete);

	int evalue_bool(const std::string &nom_propriete);

	dls::math::vec3f evalue_vec3(const std::string &nom_propriete);

	dls::math::vec4f evalue_couleur(const std::string &nom_propriete);

	std::string evalue_string(const std::string &nom_propriete);

	/* Accès. */

	std::vector<Propriete> &proprietes();

	/* Interface. */

	virtual bool ajourne_proprietes();

private:
	inline Propriete *trouve_propriete(const std::string &nom_propriete)
	{
		auto const &iter = std::find_if(
							   m_proprietes.begin(), m_proprietes.end(),
							   [&](const Propriete &prop)
		{
			return prop.nom == nom_propriete;
		});

		if (iter == m_proprietes.end()) {
			std::cerr << "Impossible de trouver la propriété : "
					  << nom_propriete << '\n';
			return nullptr;
		}

		auto const index = iter - m_proprietes.begin();
		return &m_proprietes[static_cast<size_t>(index)];
	}
};
