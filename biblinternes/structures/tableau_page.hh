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

#include "tableau.hh"

template <typename T, size_t TAILLE_PAGE = 128>
struct tableau_page {
	struct page {
		T *donnees = nullptr;
		long occupe = 0;

		T *begin()
		{
			return donnees;
		}

		T const *begin() const
		{
			return donnees;
		}

		T *end()
		{
			return donnees + occupe;
		}

		T const *end() const
		{
			return donnees + occupe;
		}
	};

	dls::tableau<page> pages{};
	page *page_courante = nullptr;
	long nombre_elements = 0;

	tableau_page()
	{
		ajoute_page();
	}

	tableau_page(tableau_page const &) = delete;
	tableau_page &operator=(tableau_page const &) = delete;

	~tableau_page()
	{
		for (auto &it : pages) {
			for (auto i = 0; i < it.occupe; ++i) {
				it.donnees[i].~T();
			}

			memoire::deloge_tableau("page", it.donnees, TAILLE_PAGE);
		}
	}

	void ajoute_page()
	{
		auto p = page();
		p.donnees = memoire::loge_tableau<T>("page", TAILLE_PAGE);
		pages.pousse(p);

		page_courante = &pages.back();
	}

	template <typename... Args>
	T *ajoute_element(Args &&...args)
	{
		if (page_courante->occupe == TAILLE_PAGE) {
			ajoute_page();
		}

		auto ptr = &page_courante->donnees[page_courante->occupe];
		page_courante->occupe += 1;
		nombre_elements += 1;

		new (ptr) T(std::move(args)...);

		return ptr;
	}

	long taille() const
	{
		return nombre_elements;
	}

	unsigned long memoire_utilisee() const
	{
		return static_cast<size_t>(pages.taille()) * (TAILLE_PAGE * sizeof(T) + sizeof(page));
	}

	T &operator[](long i)
	{
		assert(i >= 0);
		assert(i < taille());

		auto idx_page = i / static_cast<long>(TAILLE_PAGE);
		auto idx_elem = i % static_cast<long>(TAILLE_PAGE);

		return pages[idx_page].donnees[idx_elem];
	}

	T const &operator[](long i) const
	{
		assert(i >= 0);
		assert(i < taille());

		auto idx_page = i / static_cast<long>(TAILLE_PAGE);
		auto idx_elem = i % static_cast<long>(TAILLE_PAGE);

		return pages[idx_page].donnees[idx_elem];
	}
};

#define POUR_TABLEAU_PAGE(x) \
	for (auto &p : x.pages) \
		for (auto &it : p)

#define POUR_TABLEAU_PAGE_NOMME(nom_iter, x) \
	for (auto &p##nom_iter : x.pages) \
		for (auto &nom_iter : p##nom_iter)

template <typename T, size_t TAILLE_PAGE, typename Rappel>
void pour_chaque_element(tableau_page<T, TAILLE_PAGE> const &tableau, Rappel rappel)
{
	POUR_TABLEAU_PAGE(tableau) {
		rappel(it);
	}
}
