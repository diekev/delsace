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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1201, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "matrice.hh"

namespace dls {
namespace math {

/**
 * Applique une fonction à chaque élément de la matrice.
 */
template <ConceptNombre nombre, typename Function>
void applique_fonction(matrice_dyn<nombre> &mat, Function &&function)
{
	for (int l = 0; l < mat.nombre_lignes(); ++l) {
		for (int c = 0; c < mat.nombre_colonnes(); ++c) {
			mat[l][c] = function(mat[l][c]);
		}
	}
}

/**
 * Applique une fonction qui pourrait dépendre ou aurait besoin de connaître
 * la position de chaque élément à chaque élément de la matrice. La position de
 * l'élément traité est passée à la fonction.
 */
template <ConceptNombre nombre, typename Function>
void applique_fonction_position(matrice_dyn<nombre> &mat, Function &&function)
{
	for (int l = 0; l < mat.nombre_lignes(); ++l) {
		for (int c = 0; c < mat.nombre_colonnes(); ++c) {
			mat[l][c] = function(mat[l][c], l, c);
		}
	}
}

/**
 * Mélange deux matrices selon la fonction spécifiée. La mat_a est directement
 * modifiée, sans passer par une matrice temporaire.
 */
template <ConceptNombre nombre, typename Fonction>
void melange_matrices(matrice_dyn<nombre> &mat_a, const matrice_dyn<nombre> &mat_b, Fonction &&function)
{
	const auto nombre_lignes = std::min(mat_a.nombre_lignes(), mat_b.nombre_lignes());
	const auto nombre_colonnes = std::min(mat_a.nombre_colonnes(), mat_b.nombre_colonnes());

	for (int l = 0; l < nombre_lignes; ++l) {
		for (int c = 0; c < nombre_colonnes; ++c) {
			mat_a[l][c] = function(mat_a[l][c], mat_b[l][c]);
		}
	}
}

/**
 * Retourne la valeur maximale d'une matrice.
 */
template <ConceptNombre nombre>
nombre valeur_maximale(const matrice_dyn<nombre> &mat)
{
	auto resultat = std::numeric_limits<nombre>::min();

	for (int l = 0; l < mat.nombre_lignes(); ++l) {
		for (int c = 0; c < mat.nombre_colonnes(); ++c) {
			resultat = std::max(resultat, mat[l][c]);
		}
	}

	return resultat;
}

/**
 * Retourne la valeur minimale d'une matrice.
 */
template <ConceptNombre nombre>
nombre valeur_minimale(const matrice_dyn<nombre> &mat)
{
	auto resultat = std::numeric_limits<nombre>::max();

	for (int l = 0; l < mat.nombre_lignes(); ++l) {
		for (int c = 0; c < mat.nombre_colonnes(); ++c) {
			resultat = std::min(resultat, mat[l][c]);
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent du produit d'Hadamard des
 * deux matrices spécifiées.
 *
 * Si les deux matrices ont des dimensions différentes, une exception de type
 * ExceptionValeur est lancée.
 */
template <ConceptNombre Nombre>
auto hadamard(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	if (cote_droit.dimensions() != cote_gauche.dimensions()) {
		throw ExceptionValeur(
					"Le produit d'Hadamard n'est défini que pour des matrices de"
					" dimensions similaires !");
	}

	matrice_dyn<Nombre> resultat(cote_gauche.dimensions());

	for (int i = 0; i < cote_gauche.nombre_lignes(); ++i) {
		for (int j = 0; j < cote_gauche.nombre_colonnes(); ++j) {
			resultat[i][j] = cote_gauche[i][j] * cote_droit[i][j];
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent du produit de Kronecker des
 * deux matrices spécifiées.
 */
template <ConceptNombre Nombre>
auto kronecker(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	const auto dim_gauche = cote_gauche.dimensions();
	const auto dim_droite = cote_droit.dimensions();
	const auto hauteur = dim_droite.hauteur * dim_gauche.hauteur;
	const auto largeur = dim_droite.largeur * dim_gauche.largeur;

	auto resultat = matrice_dyn<Nombre>(Hauteur(hauteur), Largeur(largeur));

	for (int i = 0; i < hauteur; ++i) {
		for (int j = 0; j < largeur; ++j) {
			const auto i_g = i / dim_droite.hauteur;
			const auto i_d = i % dim_droite.hauteur;
			const auto j_g = j / dim_droite.largeur;
			const auto j_d = j % dim_droite.largeur;

			resultat[i][j] = cote_gauche[i_g][j_g] * cote_droit[i_d][j_d];
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent de la concaténation
 * horizontale des deux matrices spécifiées.
 *
 * Si les deux matrices n'ont pas le même nombre de lignes, une exception de
 * type ExceptionValeur est lancée.
 */
template <ConceptNombre Nombre>
auto contenation_horizontale(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	if (cote_gauche.nombre_lignes() != cote_droit.nombre_lignes()) {
		throw ExceptionValeur(
					"La concaténation horizontale de deux matrices n'est"
					"spécifiée que si elles ont le même nombre de lignes !");
	}

	const auto hauteur = cote_gauche.nombre_lignes();
	const auto largeur = cote_gauche.nombre_colonnes() + cote_droit.nombre_colonnes();

	auto resultat = matrice_dyn<Nombre>(Hauteur(hauteur), Largeur(largeur));

	for (int i = 0; i < resultat.nombre_lignes(); ++i) {
		int j = 0;

		for (; j < cote_gauche.nombre_colonnes(); ++j) {
			resultat[i][j] = cote_gauche[i][j];
		}

		for (; j < resultat.nombre_colonnes(); ++j) {
			resultat[i][j] = cote_droit[i][j - cote_gauche.nombre_colonnes()];
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent de la concaténation
 * verticale des deux matrices spécifiées.
 *
 * Si les deux matrices n'ont pas le même nombre de colonnes, une exception de
 * type ExceptionValeur est lancée.
 */
template <ConceptNombre Nombre>
auto contenation_veritcale(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	if (cote_gauche.nombre_colonnes() != cote_droit.nombre_colonnes()) {
		throw ExceptionValeur(
					"La concaténation verticale de deux matrices n'est"
					"spécifiée que si elles ont le même nombre de colonnes !");
	}

	const auto hauteur = cote_gauche.nombre_lignes() + cote_droit.nombre_lignes();
	const auto largeur = cote_gauche.nombre_colonnes();

	auto resultat = matrice_dyn<Nombre>(Hauteur(hauteur), Largeur(largeur));

	for (int i = 0; i < cote_gauche.nombre_lignes(); ++i) {
		for (int j = 0; j < cote_gauche.nombre_colonnes(); ++j) {
			resultat[i][j] = cote_gauche[i][j];
		}
	}

	for (int i = cote_gauche.nombre_lignes(); i < hauteur; ++i) {
		for (int j = 0; j < cote_droit.nombre_colonnes(); ++j) {
			resultat[i][j] = cote_droit[i - cote_gauche.nombre_lignes()][j];
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent de la transposition de la
 * matrice spécifiée.
 */
template <ConceptNombre Nombre>
auto transpose(const matrice_dyn<Nombre> &m)
{
	auto resultat = matrice_dyn<Nombre>(Hauteur(m.nombre_colonnes()), Largeur(m.nombre_lignes()));

	for (int i = 0; i < resultat.nombre_lignes(); ++i) {
		for (int j = 0; j < resultat.nombre_colonnes(); ++j) {
			resultat[i][j] = m[j][i];
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent de la sélection de la valeur
 * maximale locale dans une fenêtre de la taille spécifiée. La matrice retournée
 * aura des dimensions égales aux dimensions de la matrice d'entrée divisées par
 * la taille de la fenêtre (si la taille est de 2, la matrice résultante sera
 * 2 fois plus petite que l'originale).
 */
template <ConceptNombre Nombre>
auto maximum_local(const matrice_dyn<Nombre> &m, int taille)
{
	auto resultat = matrice_dyn<Nombre>(
						Hauteur(m.nombre_lignes() / taille),
						Largeur(m.nombre_colonnes() / taille));

	for (int i = 0; i < resultat.nombre_lignes(); ++i) {
		for (int j = 0; j < resultat.nombre_colonnes(); ++j) {
			auto max = static_cast<Nombre>(std::numeric_limits<Nombre>::min());
			const auto i2 = i * taille;
			const auto j2 = j * taille;

			for (int mi = 0; mi < taille; ++mi) {
				for (int mj = 0; mj < taille; ++mj) {
					max = std::max(max, m[i2 + mi][j2 + mj]);
				}
			}

			resultat[i][j] = max;
		}
	}

	return resultat;
}

/**
 * Retourne une matrice dont les valeurs résultent de la sélection de la valeur
 * minimale locale dans une fenêtre de la taille spécifiée. La matrice retournée
 * aura des dimensions égales aux dimensions de la matrice d'entrée divisées par
 * la taille de la fenêtre (si la taille est de 2, la matrice résultante sera
 * 2 fois plus petite que l'originale).
 */
template <ConceptNombre Nombre>
auto minimum_local(const matrice_dyn<Nombre> &m, int taille)
{
	auto resultat = matrice_dyn<Nombre>(
						Hauteur(m.nombre_lignes() / taille),
						Largeur(m.nombre_colonnes() / taille));

	for (int i = 0; i < resultat.nombre_lignes(); ++i) {
		for (int j = 0; j < resultat.nombre_colonnes(); ++j) {
			auto min = static_cast<Nombre>(std::numeric_limits<Nombre>::max());
			const auto i2 = i * taille;
			const auto j2 = j * taille;

			for (int mi = 0; mi < taille; ++mi) {
				for (int mj = 0; mj < taille; ++mj) {
					min = std::min(min, m[i2 + mi][j2 + mj]);
				}
			}

			resultat[i][j] = min;
		}
	}

	return resultat;
}

}  /* namespace math */
}  /* namespace dls */
