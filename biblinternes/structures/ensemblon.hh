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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "ensemble.hh"

namespace dls {

template <typename T, unsigned long TAILLE_INITIALE, typename Comparaison = std::less<T>>
struct ensemblon {
private:
	T m_ensemblon[TAILLE_INITIALE];
	dls::ensemble<T, Comparaison> m_ensemble{};

	long m_taille = 0;

public:
	ensemblon() = default;

	ensemblon(ensemblon const &autre)
	{
		copie_donnees(autre);
	}

	ensemblon &operator=(ensemblon const &autre)
	{
		copie_donnees(autre);
	}

	ensemblon(ensemblon &&autre)
	{
		echange(autre);
	}

	ensemblon &operator=(ensemblon &&autre)
	{
		echange(autre);
	}

	~ensemblon() = default;

	bool est_stocke_dans_classe() const
	{
		return m_taille <= static_cast<long>(TAILLE_INITIALE);
	}

	void echange(ensemblon &autre)
	{
		if (this->est_stocke_dans_classe() && autre.est_stocke_dans_classe()) {
			for (auto i = 0; i < TAILLE_INITIALE; ++i) {
				std::swap(m_ensemblon[i], autre.m_ensemblon[i]);
			}
		}
		else if (this->est_stocke_dans_classe()) {
			echange_donnees(*this, autre);
		}
		else if (autre.est_stocke_dans_classe()) {
			echange_donnees(autre, *this);
		}
		else {
			m_ensemble.echange(autre.m_ensemble);
		}

		std::swap(m_taille, autre.m_taille);
	}

	void copie_donnees(ensemblon const &autre)
	{
		m_taille = 0;
		m_ensemble.efface();

		if (autre.est_stocke_dans_classe()) {
			for (auto i = 0; i < autre.taille(); ++i) {
				m_ensemblon[i] = autre.m_ensemblon[i];
				m_taille += 1;
			}

			return;
		}

		m_ensemble = autre.m_ensemble;
	}

	void insere(T const &valeur)
	{
		if (possede(valeur)) {
			return;
		}

		if (est_stocke_dans_classe()) {
			if (m_taille + 1 <= static_cast<long>(TAILLE_INITIALE)) {
				m_ensemblon[m_taille] = valeur;
				m_taille += 1;
				return;
			}

			for (auto i = 0; i < taille(); ++i) {
				m_ensemble.insere(m_ensemblon[i]);
			}
		}

		m_ensemble.insere(valeur);
		m_taille += 1;
	}

	bool possede(T const &valeur) const
	{
		if (est_stocke_dans_classe()) {
			for (auto i = 0; i < taille(); ++i) {
				if (m_ensemblon[i] == valeur) {
					return true;
				}
			}

			return false;
		}

		return m_ensemble.trouve(valeur) != m_ensemble.fin();
	}

	long taille() const
	{
		return m_taille;
	}

	T const *donnees_ensemblon() const
	{
		return m_ensemblon;
	}

	dls::ensemble<T, Comparaison> const &ensemble() const
	{
		return m_ensemble;
	}

	void efface()
	{
		if (!est_stocke_dans_classe()) {
			m_ensemble.efface();
		}

		m_taille = 0;
	}

private:
	void echange_donnees(ensemblon &ensemblon_local, ensemblon &ensemblon_memoire)
	{
		for (auto i = 0; i < TAILLE_INITIALE; ++i) {
			std::swap(ensemblon_local.m_ensemblon[i], ensemblon_memoire.m_ensemblon[i]);
		}

		ensemblon_local.m_ensemble.echange(ensemblon_memoire.m_ensemble);
	}
};

enum class DecisionIteration {
	Arrete,
	Continue
};

template <typename T, unsigned long TAILLE_INITIALE, typename Rappel, typename Comparaison = std::less<T>>
void pour_chaque_element(ensemblon<T, TAILLE_INITIALE, Comparaison> const &ens, Rappel rappel)
{
	if (ens.est_stocke_dans_classe()) {
		for (auto i = 0; i < ens.taille(); ++i) {
			auto action = rappel(ens.donnees_ensemblon()[i]);

			if (action == DecisionIteration::Arrete) {
				break;
			}
		}

		return;
	}

	for (auto const &valeur : ens.ensemble()) {
		auto action = rappel(valeur);

		if (action == DecisionIteration::Arrete) {
			break;
		}
	}
}

}
