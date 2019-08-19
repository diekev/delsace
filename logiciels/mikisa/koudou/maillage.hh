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

#pragma once

#include "biblinternes/math/boite_englobante.hh"
#include "biblinternes/math/transformation.hh"
#include "biblinternes/structures/tableau.hh"

namespace kdo {

class Nuanceur;

/**
 * Représentation d'un triangle et de son vecteur normal dans l'espace
 * tridimensionel.
 */
struct Triangle {
	int v0 = 0;
	int v1 = 0;
	int v2 = 0;

	int n0 = 0;
	int n1 = 0;
	int n2 = 0;

	/* ceci est pour savoir quel est le maillage du triangle dans le délégué scène */
	int idx_maillage = 0;
};

/**
 * La classe Maillage contient les triangles formant un objet dans l'espace
 * tridimensionel.
 */
class Maillage {

	BoiteEnglobante m_boite_englobante{};
	math::transformation m_transformation{};

	Nuanceur *m_nuanceur = nullptr;

	dls::chaine m_nom = "maillage";

	bool m_dessine_normaux = false;

public:
	dls::tableau<Triangle *> m_triangles{};

	dls::tableau<dls::math::vec3d> points{};
	dls::tableau<dls::math::vec3d> normaux{};

	int volume = -1;

	Maillage();

	Maillage(Maillage const &autre) = default;
	Maillage &operator=(Maillage const &autre) = default;

	~Maillage();

	using iterateur = dls::tableau<Triangle *>::iteratrice;
	using const_iterateur = dls::tableau<Triangle *>::const_iteratrice;

	/**
	 * Retourne un itérateur pointant vers le début de la liste de triangle de
	 * ce maillage.
	 */
	iterateur begin();

	/**
	 * Retourne un itérateur pointant vers la fin de la liste de triangle de ce
	 * maillage.
	 */
	iterateur end();

	/**
	 * Retourne un itérateur constant pointant vers le début de la liste de
	 * triangle de ce maillage.
	 */
	const_iterateur begin() const;

	/**
	 * Retourne un itérateur constant pointant vers la fin de la liste de
	 * triangle de ce maillage.
	 */
	const_iterateur end() const;

	void transformation(math::transformation const &transforme);

	/**
	 * Retourne la transformation de ce maillage.
	 */
	math::transformation const &transformation() const;

	/**
	 * Retourne la boîte englobante de ce maillage.
	 */
	BoiteEnglobante const &boite_englobante() const;

	void nuanceur(Nuanceur *n);

	Nuanceur *nuanceur() const;

	/**
	 * Calcule la boîte englobante de ce maillage.
	 */
	void calcule_boite_englobante();

	/**
	 * Calcul les limites de l'objet selon l'algorithme des volumes englobants.
	 */
	void calcule_limites(dls::math::vec3d const &normal, double &d_proche, double &d_eloigne) const;

	/**
	 * Modifie le nom de ce maillage en fonction de celui passé en paramètre.
	 */
	void nom(dls::chaine const &nom);

	/**
	 * Retourne le nom de ce maillage.
	 */
	dls::chaine const &nom() const;

	/**
	 * Défini si oui ou non il faut dessiner les vecteurs normaux de ce maillage.
	 */
	void dessine_normaux(bool ouinon)
	{
		m_dessine_normaux = ouinon;
	}

	/**
	 * Retourne si oui ou non il faut dessiner les vecteurs normaux de ce maillage.
	 */
	bool dessine_normaux() const
	{
		return m_dessine_normaux;
	}
};

}  /* namespace kdo */
