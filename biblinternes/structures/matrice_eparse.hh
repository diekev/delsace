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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/definitions.h"

#include "tableau.hh"

/**
 * Implémentations de matrices éparses, où les zéros ne sont pas stockés,
 * inspirées de https://www.geeksforgeeks.org/sparse-matrix-representation/
 */

/* ****************************************************************** */

template <typename T, int O>
struct type_strict {
	T t{};

	type_strict() = default;

	explicit type_strict(T v)
		: t(v)
	{}

	operator T()
	{
		return t;
	}
};

using type_ligne = type_strict<long, 0>;
using type_colonne = type_strict<long, 1>;

/* ****************************************************************** */

template <typename T>
struct matrice_eparse {
	using type_valeur = T;

	struct noeud {
		noeud *prec = nullptr;
		noeud *suiv = nullptr;
		long ligne = 0;
		long colonne = 0;
		type_valeur valeur{};

		noeud() = default;

		COPIE_CONSTRUCT(noeud);
	};

	noeud *premier = nullptr;

	COPIE_CONSTRUCT(matrice_eparse);

	~matrice_eparse()
	{
		auto n = premier;

		while (n != nullptr) {
			auto tmp = n->suivant;
			memoire::deloge("matrice_eparse::noeud", n);
			n = tmp;
		}
	}

	type_valeur operator()(type_ligne ligne, type_colonne colonne) const
	{
		auto n = trouve_noeud(ligne, colonne);

		if (n == nullptr) {
			return {};
		}

		return n->valeur;
	}

	type_valeur &operator()(type_ligne ligne, type_colonne colonne)
	{
		auto n = trouve_noeud(ligne, colonne);

		if (n == nullptr) {
			n = insere_noeud(ligne, colonne);
		}

		return n->valeur;
	}

private:
	noeud *trouve_noeud(long ligne, long colonne)
	{
		auto n = this->premier;

		while (n != nullptr) {
			if (n->ligne == ligne && n->colonne == colonne) {
				break;
			}

			n = n->suivant;
		}

		return n;
	}

	noeud *insere_noeud(long ligne, long colonne)
	{
		auto n = memoire::loge<noeud>("matrice_eparse::noeud");
		n->ligne = ligne;
		n->colonne = colonne;

		if (premier == nullptr) {
			premier = n;
		}
		else {
			n->suivant = premier;
			premier = n;
		}

		return n;
	}
};

/* ****************************************************************** */

template <typename T>
struct matrice_colonne_eparse {
	using type_valeur = T;

	struct noeud {
		noeud *precedent = nullptr;
		noeud *suivant = nullptr;

		long colonne = 0;
		type_valeur valeur{};

		noeud() = default;

		COPIE_CONSTRUCT(noeud);
	};

	long nombre_lignes = 0;
	long nombre_colonnes = 0;

	dls::tableau<noeud *> noeuds{};
	dls::tableau<noeud *> lignes{};

	matrice_colonne_eparse(type_ligne nl, type_colonne nc)
		: nombre_lignes(nl)
		, nombre_colonnes(nc)
		, lignes(nl, nullptr)
	{}

	matrice_colonne_eparse(type_colonne nc, type_ligne nl)
		: matrice_colonne_eparse(nl, nc)
	{}

	~matrice_colonne_eparse()
	{
//		for (auto n : lignes) {
//			while (n != nullptr) {
//				auto tmp = n->suivant;
//				memoire::deloge("matrice_colonne_eparse::noeud", n);
//				n = tmp;
//			}
//		}
		for (auto n : noeuds) {
			memoire::deloge("matrice_colonne_eparse::noeud", n);
		}
	}

	type_valeur operator()(type_ligne ligne, type_colonne colonne) const
	{
		auto const &l = lignes[ligne];
		auto n = trouve_noeud(l, colonne);

		if (n == nullptr) {
			return type_valeur();
		}

		return n->valeur;
	}

	type_valeur operator()(type_colonne colonne, type_ligne ligne) const
	{
		return this->operator()(ligne, colonne);
	}

	type_valeur &operator()(type_ligne ligne, type_colonne colonne)
	{
		auto &l = lignes[ligne];
		auto n = trouve_noeud(l, colonne);

		if (n == nullptr) {
			n = insere_noeud(l, colonne);
		}

		return n->valeur;
	}

	type_valeur &operator()(type_colonne colonne, type_ligne ligne)
	{
		return this->operator()(ligne, colonne);
	}

private:
	noeud *trouve_noeud(noeud *l, long colonne)
	{
		auto n = l;

		while (n != nullptr) {
			if (n->colonne == colonne) {
				break;
			}

			n = n->suivant;
		}

		return n;
	}

	noeud *insere_noeud(noeud *&l, long colonne)
	{
		auto n = memoire::loge<noeud>("matrice_colonne_eparse::noeud");
		n->colonne = colonne;

		noeuds.pousse(n);

		if (l == nullptr) {
			l = n;
		}
		else {
			auto nn = l;
			auto dernier = nn;
			auto insere = false;

			while (nn != nullptr) {
				dernier = nn;

				if (nn->colonne > colonne) {
					n->precedent = nn->precedent;

					if (n->precedent) {
						n->precedent->suivant = n;
					}

					n->suivant = nn;
					nn->precedent = n;
					insere = true;

					break;
				}

				nn = nn->suivant;
			}

			if (insere == false) {
				if (dernier != nullptr) {
					dernier->suivant = n;
				}

				n->precedent = dernier;
			}
		}

		return n;
	}
};

template <typename T>
bool matrice_valide(matrice_colonne_eparse<T> const &mat)
{
	for (auto i = 0; i < mat.lignes.taille(); ++i) {
		auto n = mat.lignes[i];

		while (n != nullptr) {
			auto s = n->suivant;

			if (s) {
				if (n->colonne > s->colonne) {
					return false;
				}
			}

			n = s;
		}
	}

	return true;
}
