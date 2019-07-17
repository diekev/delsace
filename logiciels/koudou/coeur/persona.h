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
#include "biblinternes/math/vecteur.hh"
#include <any>
#include <iostream>
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

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
	dls::chaine nom{};
	int valeur{};
};

struct ProprieteEnumerante {
	dls::tableau<PaireNomValeur> paires{};

	void ajoute(dls::chaine nom, int valeur)
	{
		PaireNomValeur paire;
		paire.nom = std::move(nom);
		paire.valeur = valeur;

		paires.pousse(std::move(paire));
	}
};

struct Propriete {
	dls::chaine nom{};
	dls::chaine nom_entreface{};
	dls::chaine infobulle{};
	TypePropriete type{};

	std::any donnee{};
	std::any valeur_defaut{};

	ProprieteEnumerante items_enumeration{};

	float min{}, max{};
	bool visible{};
};

/**
 * Classe de base pour définir des objets qui ont des propriétés montrées dans
 * l'entreface utilisateur.
 */
class Persona {
	dls::tableau<Propriete> m_proprietes{};

public:
	virtual ~Persona() = default;

	void ajoute_propriete(dls::chaine nom, dls::chaine nom_entreface, TypePropriete type);

	/* Modification des propriétés. */

	void rend_visible(dls::chaine const &nom_propriete, bool visible);

	void etablie_infobulle(dls::chaine tooltip);

	void etablie_min_max(const float min, const float max);

	void etablie_valeur_enum(ProprieteEnumerante const &enum_prop);

	void etablie_valeur_int_defaut(int valeur);

	void etablie_valeur_float_defaut(float valeur);

	void etablie_valeur_bool_defaut(bool valeur);

	void etablie_valeur_string_defaut(dls::chaine const &valeur);

	void etablie_valeur_vec3_defaut(dls::math::vec3f const &valeur);

	void etablie_valeur_couleur_defaut(dls::math::vec4f const &valeur);

	void ajourne_valeur_float(dls::chaine const &nom_propriete, float valeur);

	void ajourne_valeur_int(dls::chaine const &nom_propriete, int valeur);

	void ajourne_valeur_bool(dls::chaine const &nom_propriete, bool valeur);

	void ajourne_valeur_string(dls::chaine const &nom_propriete, dls::chaine const &valeur);

	void ajourne_valeur_couleur(dls::chaine const &nom_propriete, dls::math::vec4f const &valeur);

	void ajourne_valeur_vec3(dls::chaine const &nom_propriete, dls::math::vec3f const &valeur);

	void ajourne_valeur_enum(dls::chaine const &nom_propriete, ProprieteEnumerante const &propriete);

	/* Évaluation. */

	int evalue_int(dls::chaine const &nom_propriete);

	float evalue_float(dls::chaine const &nom_propriete);

	int evalue_enum(dls::chaine const &nom_propriete);

	int evalue_bool(dls::chaine const &nom_propriete);

	dls::math::vec3f evalue_vec3(dls::chaine const &nom_propriete);

	dls::math::vec4f evalue_couleur(dls::chaine const &nom_propriete);

	dls::chaine evalue_string(dls::chaine const &nom_propriete);

	/* Accès. */

	dls::tableau<Propriete> &proprietes();

	/* Interface. */

	virtual bool ajourne_proprietes();

private:
	inline Propriete *trouve_propriete(dls::chaine const &nom_propriete)
	{
		auto const &iter = std::find_if(
							   m_proprietes.debut(), m_proprietes.fin(),
							   [&](Propriete const &prop)
		{
			return prop.nom == nom_propriete;
		});

		if (iter == m_proprietes.fin()) {
			std::cerr << "Impossible de trouver la propriété : "
					  << nom_propriete << '\n';
			return nullptr;
		}

		auto const index = iter - m_proprietes.debut();
		return &m_proprietes[index];
	}
};
