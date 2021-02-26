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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cmath>
#include <cstdint>

#include "biblinternes/memoire/logeuse_memoire.hh"

namespace kuri {

template <typename T>
struct plage_iterable {
	T debut_{};
	T fin_{};

	T begin()
	{
		return debut_;
	}

	T end()
	{
		return fin_;
	}
};

/* Valeur magique pour déterminer si un tableau_compresse fut alloué ou non.
 * Le pointeur des données pointe sur cette adresse tant que nous n'avons pas
 * allouée de la mémoire. Il pointera à la bonne adresse après une allocation. */
static constexpr uintptr_t POINTEUR_MORT = static_cast<uintptr_t>(0xdeadc0de);

/* Tableau n'allouant de la mémoire que lorsqu'une valeur ajoutée est différente de la première. */
template <typename T, typename TypeIndex = long>
struct tableau_compresse {
private:
	T *m_pointeur = reinterpret_cast<T *>(POINTEUR_MORT);

	TypeIndex m_taille = 0;
	TypeIndex m_capacite = 0;

	T m_premiere_valeur{};

public:
	tableau_compresse() = default;

	tableau_compresse(tableau_compresse const &autre)
	{
		*this = autre;
	}

	tableau_compresse(tableau_compresse &&autre)
	{
		if (this != &autre) {
			this->permute(autre);
		}
	}

	tableau_compresse &operator=(tableau_compresse const &autre)
	{
		if (this != &autre) {
			supprime_donnees();

			m_taille = autre.m_taille;
			m_capacite = autre.m_capacite;
			m_premiere_valeur = autre.m_premiere_valeur;

			if (!autre.alloue()) {
				m_pointeur = reinterpret_cast<T *>(POINTEUR_MORT);
			}
			else {
				m_pointeur = memoire::loge_tableau<T>("tableau_compresse", m_capacite);

				for (auto i = 0; i < autre.m_taille; ++i) {
					m_pointeur[i] = autre.m_pointeur[i];
				}
			}
		}

		return *this;
	}

	tableau_compresse &operator=(tableau_compresse &&autre)
	{
		if (this != &autre) {
			this->permute(autre);
		}

		return *this;
	}

	~tableau_compresse()
	{
		supprime_donnees();
	}

	bool alloue() const
	{
		return m_pointeur != reinterpret_cast<T *>(POINTEUR_MORT);
	}

	TypeIndex taille() const
	{
		return m_taille;
	}

	TypeIndex capacite() const
	{
		return m_capacite;
	}

	plage_iterable<T *> plage()
	{
		/* si la taille est de zéro, aucune valeur n'a pour le moment été ajoutée, retourne une plage vide */
		if (taille() == 0) {
			return {m_pointeur, m_pointeur};
		}

		if (!alloue()) {
			return {&m_premiere_valeur, (&m_premiere_valeur) + 1};
		}

		return {m_pointeur, m_pointeur + m_taille};
	}

	plage_iterable<const T *> plage() const
	{
		/* si la taille est de zéro, aucune valeur n'a pour le moment été ajoutée, retourne une plage vide */
		if (taille() == 0) {
			return {m_pointeur, m_pointeur + m_taille};
		}

		if (!alloue()) {
			return {&m_premiere_valeur, (&m_premiere_valeur) + 1};
		}

		return {m_pointeur, m_pointeur + m_taille};
	}

	void ajoute(T valeur)
	{
		if (m_taille == 0) {
			m_premiere_valeur = valeur;
			++m_taille;
		}
		else if (!alloue() && valeur == m_premiere_valeur) {
			++m_taille;
			if (m_taille > m_capacite) {
				m_capacite = m_taille;
			}
		}
		else {
			if (!alloue()) {
				m_capacite = std::max(static_cast<TypeIndex>(m_taille + 1), m_capacite);
				m_pointeur = memoire::loge_tableau<T>("tableau_compresse", m_capacite);

				if (!std::is_trivially_constructible_v<T>) {
					for (auto i = 0; i < m_taille; ++i) {
						new (&m_pointeur[i]) T;
					}
				}

				for (auto i = 0; i < m_taille; ++i) {
					m_pointeur[i] = m_premiere_valeur;
				}
			}

			reserve(static_cast<TypeIndex>(m_taille + 1));

			if (!std::is_trivially_constructible_v<T>) {
				new (&m_pointeur[static_cast<long>(m_taille)]) T;
			}

			m_pointeur[static_cast<long>(m_taille)] = valeur;
			++m_taille;
		}
	}

	void reserve(TypeIndex nombre)
	{
		if (!alloue()) {
			m_capacite = nombre;
		}
		else {
			if (nombre <= m_capacite) {
				return;
			}

			memoire::reloge_tableau("tableau_compresse", m_pointeur, m_capacite, nombre);
			m_capacite = nombre;
		}
	}

	T const &operator [] (TypeIndex idx) const
	{
		if (!alloue()) {
			return m_premiere_valeur;
		}

		return m_pointeur[idx];
	}

	void permute(tableau_compresse &autre)
	{
		std::swap(m_pointeur, autre.m_pointeur);
		std::swap(m_taille, autre.m_taille);
		std::swap(m_capacite, autre.m_capacite);
		std::swap(m_premiere_valeur, autre.m_premiere_valeur);
	}

private:
	void supprime_donnees()
	{
		if (alloue()) {
			memoire::deloge_tableau("tableau_compresse", m_pointeur, m_capacite);
		}

		m_taille = 0;
		m_capacite = 0;
	}
};

}
