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

#include "outils.hh"

#include <iostream>

namespace dls {
namespace chrono {

/**
 * Cette classe sert à chronométrer les méthodes d'un objet à chaque fois que
 * celles-ci sont invoquées.
 *
 * La classe englobe un méchanisme de minuterie de portée qui est construit
 * avant d'appeler la méthode désirée et détruit juste après son exécution,
 * permettant de chronométrer son temps d'exécution.
 */
template <typename __type>
class pointeur_chronometre {
	__type *m_pointeur = nullptr;
	std::ostream &m_flux_sortie;

public:
	/**
	 * Cette classe sert à chronométrer sa propre durée de vie.
	 *
	 * Nous nous servons de l'opérateur '->' pour accéder au pointeur contenu
	 * dans la classe parente et la méthode ainsi appelée sera chronométrée.
	 */
	class chronometre_portee {
		pointeur_chronometre *m_parent = nullptr;
		compte_seconde m_debut{};

	public:
		explicit chronometre_portee(pointeur_chronometre *parent)
			: m_parent(parent)
			, m_debut(compte_seconde())
		{}

		~chronometre_portee()
		{
			if (m_parent != nullptr) {
				m_parent->m_flux_sortie << m_debut.temps() << '\n';
			}
		}

		chronometre_portee(const chronometre_portee &rhs) = delete;
		chronometre_portee &operator=(const chronometre_portee &rhs) = delete;

		chronometre_portee(chronometre_portee &&rhs)
			: m_parent(rhs.m_parent)
			, m_debut(rhs.m_debut)
		{
			rhs.m_parent = nullptr;
            rhs.m_debut = 0.0;
		}

		chronometre_portee &operator=(chronometre_portee &&rhs)
		{
			m_parent = rhs.m_parent;
			m_debut = rhs.m_debut;
			rhs.m_parent = nullptr;
			rhs.m_debut = 0.0;

			return *this;
		}

		__type *operator->()
		{
			return (m_parent != nullptr) ? m_parent->m_pointeur : nullptr;
		}
	};

	pointeur_chronometre() = delete;

	pointeur_chronometre(__type *pointeur, std::ostream &flux_sortie)
		: m_pointeur(pointeur)
		, m_flux_sortie(flux_sortie)
	{}

	pointeur_chronometre(const pointeur_chronometre &rhs) = delete;
	pointeur_chronometre &operator=(const pointeur_chronometre &rhs) = delete;

	pointeur_chronometre(pointeur_chronometre &&rhs)
		: m_pointeur(rhs.m_pointeur)
		, m_flux_sortie(rhs.m_flux_sortie)
	{
		rhs.m_pointeur = nullptr;
	}

	pointeur_chronometre &operator=(pointeur_chronometre &&rhs)
	{
		m_pointeur = rhs.m_pointeur;
        // m_flux_sortie = rhs.m_flux_sortie;
		rhs.m_pointeur = nullptr;

		return *this;
	}

	/**
	 * Construit le chronomètre temporaire qui nous servira à chronométrer la
	 * méthode invoquée. Le pointeur contenu dans cette classe sera accédé dans
	 * l'objet temporaire créé.
	 */
	chronometre_portee operator->()
	{
		return chronometre_portee(this);
	}
};

/**
 * Ce macro imprime dans 'flux' la chaîne de caractère représentant 'x' avant
 * d'appeler 'x'.
 */
#define IMPRIME_PUIS_EXECUTE(flux, x) \
	flux << #x" : "; \
	x;

}  /* namespace chrono */
}  /* namespace dls */
