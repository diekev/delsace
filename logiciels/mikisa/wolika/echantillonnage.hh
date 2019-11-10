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

#include "grille_dense.hh"

namespace wlk {

/* échantillonnage 2D */

template <typename T>
auto echantillonne_proche(grille_dense_2d<T> const &grille, float x, float y)
{
	auto const res = grille.desc().resolution;

	/* enveloppe pour que 0 <= (x, y) < 1 */
	x = std::fmod(x, 1.0f);
	y = std::fmod(y, 1.0f);

	if (x < 0.0f) {
		x += 1.0f;
	}

	if (y < 0.0f) {
		y += 1.0f;
	}

	auto xc = x * static_cast<float>(res.x);
	auto yc = y * static_cast<float>(res.y);

	auto const entier_x = static_cast<int>(xc);
	auto const entier_y = static_cast<int>(yc);

	auto const x1 = std::max(0, std::min(entier_x, res.x - 1));
	auto const y1 = std::max(0, std::min(entier_y, res.y - 1));

	auto const index = x1 + y1 * res.y;

	return grille.valeur(index);
}

template <typename T>
auto echantillonne_lineaire(grille_dense_2d<T> const &grille, float x, float y)
{
	auto const res = grille.desc().resolution;

	/* enveloppe pour que 0 <= (x, y) < 1 */
	x = std::fmod(x, 1.0f);
	y = std::fmod(y, 1.0f);

	if (x < 0.0f) {
		x += 1.0f;
	}

	if (y < 0.0f) {
		y += 1.0f;
	}

	auto xc = x * static_cast<float>(res.x);
	auto yc = y * static_cast<float>(res.y);

	auto i0 = static_cast<int>(xc);
	auto j0 = static_cast<int>(yc);

	auto i1 = i0 + 1;
	auto j1 = j0 + 1;

	auto const frac_x = xc - static_cast<float>(i0);
	auto const frac_y = yc - static_cast<float>(j0);

	i0 %= res.x;
	j0 %= res.y;

	i1 %= res.x;
	j1 %= res.y;

	auto const idx0 = i0 + j0 * res.x;
	auto const idx1 = i1 + j0 * res.x;
	auto const idx2 = i0 + j1 * res.x;
	auto const idx3 = i1 + j1 * res.x;

	auto valeur = T(0);
	valeur += frac_x * frac_y * grille.valeur(idx0);
	valeur += (1.0f - frac_x) * frac_y * grille.valeur(idx1);
	valeur += frac_x * (1.0f - frac_y) * grille.valeur(idx2);
	valeur += (1.0f - frac_x) * (1.0f - frac_y) * grille.valeur(idx3);

	return valeur;
}

template <typename T>
inline auto catrom(T p0, T p1, T p2, T p3, T f)
{
	auto const MOITIE = static_cast<T>(0.5);
	auto const DEUX = static_cast<T>(2.0);
	auto const TROIS = static_cast<T>(2.0);
	auto const QUATRE = static_cast<T>(2.0);
	auto const CINQ = static_cast<T>(2.0);

	return MOITIE * ((DEUX * p1) +
				  (-p0 + p2) * f +
				  (DEUX * p0 - CINQ * p1 + QUATRE * p2 - p3) * f * f +
				  (-p0 + TROIS * p1 - TROIS * p2 + p3) * f * f * f);
}

template <typename T>
auto echantillonne_catrom(grille_dense_2d<T> const &grille, float x, float y)
{
	auto const res = grille.desc().resolution;

	/* enveloppe pour que 0 <= (x, y) < 1 */
	x = std::fmod(x, 1.0f);
	y = std::fmod(y, 1.0f);

	if (x < 0) x += 1.0f;
	if (y < 0) y += 1.0f;

	auto uu = x * static_cast<float>(res.x);
	auto vv = y * static_cast<float>(res.y);

	auto i1 = static_cast<int>(std::floor(uu));
	auto j1 = static_cast<int>(std::floor(vv));

	auto i2 = (i1 + 1);
	auto j2 = (j1 + 1);

	auto frac_x = uu - static_cast<float>(i1);
	auto frac_y = vv - static_cast<float>(j1);

	i1 = i1 % res.x;
	j1 = j1 % res.y;

	i2 = i2 % res.x;
	j2 = j2 % res.y;

	auto i0 = (i1 - 1);
	auto i3 = (i2 + 1);
	i0 = i0 <   0 ? i0 + res.x : i0;
	i3 = i3 >= res.x ? i3 - res.x : i3;

	auto j0 = (j1 - 1);
	auto j3 = (j2 + 1);
	j0 = j0 <   0 ? j0 + res.y : j0;
	j3 = j3 >= res.y ? j3 - res.y : j3;

	return catrom(catrom(grille.valeur(i0 + res.x * j0), grille.valeur(i1 + res.x * j0),
						 grille.valeur(i2 + res.x * j0), grille.valeur(i3 + res.x * j0), T(frac_x)),
				  catrom(grille.valeur(i0 + res.x * j1), grille.valeur(i1 + res.x * j1),
						 grille.valeur(i2 + res.x * j1), grille.valeur(i3 + res.x * j1), T(frac_x)),
				  catrom(grille.valeur(i0 + res.x * j2), grille.valeur(i1 + res.x * j2),
						 grille.valeur(i2 + res.x * j2), grille.valeur(i3 + res.x * j2), T(frac_x)),
				  catrom(grille.valeur(i0 + res.x * j3), grille.valeur(i1 + res.x * j3),
						 grille.valeur(i2 + res.x * j3), grille.valeur(i3 + res.x * j3), T(frac_x)),
				  T(frac_y));
}

/* échantillonnage 3D */

/**
 * Le but fondamental d'un tampon de voxel est évidemment d'être écrit vers, et
 * lu depuis. Pour modéliser des tampons complexes nécessite plus que d'écrire
 * une valeur à un voxel, par exemple : tampon(x, y, z) = valeur. Il faut
 * également pouvoir écrire entre les voxels.
 */
template <typename T>
struct Rasteriseur {
private:
	grille_dense_3d<T> &m_grille;

public:
	explicit Rasteriseur(grille_dense_3d<T> &grille)
		: m_grille(grille)
	{}

	grille_dense_3d<T> const &grille() const
	{
		return m_grille;
	}

	/* Le plus simple pour écrire des valeurs entre les voxels est de simplement
	 * arrondire les coordonnées aux nombres entiers le plus proche. Cela pose
	 * des problème d'anti-aliasing, mais peut être raisonnable, notamment quand
	 * nous devons écrire de grosses quantité de valeurs de basses densités qui
	 * se mélangerons quand prises ensembles. */
	void ecris_voisin_plus_proche(dls::math::vec3f const &vsp, T valeur) const
	{
		/* continu_vers_discret nous donne la même chose que l'on recherche :
		 * le voxel où le point d'échantillons se trouve */
		auto dvsp = dls::math::continu_vers_discret<int>(vsp);

		m_grille.valeur(static_cast<size_t>(dvsp.x), static_cast<size_t>(dvsp.y), static_cast<size_t>(dvsp.z), valeur);
	}

	/* Si l'anti-aliasing est important, nous pouvons utiliser un filtre pour
	 * écrire les valeurs. Le plus simple, et plus utilisé communément est le
	 * filtre triangulaire avec un rayon de 1 voxel. Ce filtre aura au plus
	 * une contribution non-nulle au huit voxels entourant le lieu de
	 * l'échantillon. La valeur à écrire est simplement distribuée entre les
	 * voxels avoisinants, chacun pesé par le filtre triangulaire. */
	void ecris_trilineaire(dls::math::vec3f const &vsp, T valeur) const
	{
		/* Décale la position en espace voxel relative aux centres des voxels.
		 * Le reste des calculs seront fait dans cet espace. */
		auto p = dls::math::vec3f(vsp.x - 0.5f, vsp.y - 0.5f, vsp.z - 0.5f);

		/* Trouve le coin en bas à gauche du cube de 8 voxels qu'il nous faut
		 * accéder. */
		auto c = dls::math::continu_vers_discret<int>(p);

		/* Calcul la distance fractionnelle entre les voxels.
		 * Nous commençons avec (1.0 - fraction) puisque chaque étape de la
		 * boucle inversera la valeur. */
		auto fraction = dls::math::vec3f(1.0f) - (dls::math::vec3f(static_cast<float>(c.x) + 1.0f, static_cast<float>(c.y) + 1.0f, static_cast<float>(c.z) + 1.0f) - p);

		/* Boucle sur les 8 voxels et distribue la valeur. */
		auto cur = dls::math::vec3i();

		for (int k = 0; k < 2; ++k) {
			fraction[2] = 1.0f - fraction[2];
			cur.z = c.z + k;

			for (int j = 0; j < 2; ++j) {
				fraction[1] = 1.0f - fraction[1];
				cur.y = c.y + j;

				for (int i = 0; i < 2; ++i) {
					fraction[0] = 1.0f - fraction[0];
					cur.x = c.x + i;

					auto poids = fraction[0] * fraction[1] * fraction[2];
					m_grille.valeur(cur) += valeur * poids;
				}
			}
		}
	}
};

/* ************************************************************************** */

/**
 * Afin d'échantilloner un point arbitraire dans un tampon de voxel nous devons
 * utiliser une interpolation. La plus utilisée est la trilinéaire qui calcul
 * une combinaison linéaire des 8 points de données autour de l'échantillon. Le
 * concep et l'implémentation sont similaires à l'écriture trilinéaire.
 *
 * On pourrait utiliser des plus grands ordres d'interpolation, mais qui sont
 * plus chers à calculer. Le profilage révèle qu'une portion significante de
 * temps de rendu d'un volume se passe à interpoler les données voxel. La raison
 * principale est qu'une structure de tampon naïve n'offre qu'une très mauvaise
 * cohérence de cache. Un stockage en carreau combiné avec des accès structurés
 * amélioreront la performance, mais requirera un implémentation plus complexe.
 */
template <typename T>
struct Echantilloneuse {
private:
	grille_dense_3d<T> const &m_grille;

public:
	explicit Echantilloneuse(grille_dense_3d<T> const &grille)
		: m_grille(grille)
	{}

	T echantillone_trilineaire(dls::math::vec3f const &vsp) const
	{
		/* Décale la position en espace voxel relative aux centres des voxels.
		 * Le reste des calculs seront fait dans cet espace. */
		auto p = dls::math::vec3f(vsp.x - 0.5f, vsp.y - 0.5f, vsp.z - 0.5f);

		/* Trouve le coin en bas à gauche du cube de 8 voxels qu'il nous faut
		 * accéder. */
		auto c = dls::math::continu_vers_discret<int>(p);

		float poids[3];
		auto valeur = static_cast<T>(0.0);

		auto cur = dls::math::vec3i();

		for (int i = 0; i < 2; ++i) {
			cur.x = c[0] + i;
			poids[0] = 1.0f - std::abs(p[0] - static_cast<float>(cur.x));

			for (int j = 0; j < 2; ++j) {
				cur.y  = c[1] + j;
				poids[1] = 1.0f - std::abs(p[1] - static_cast<float>(cur.y));

				for (int k = 0; k < 2; ++k) {
					cur.z = c[2] + k;
					poids[2] = 1.0f - std::abs(p[2] - static_cast<float>(cur.z));

					valeur += poids[0] * poids[1] * poids[2] * m_grille.valeur(cur);
				}
			}
		}

		return valeur;
	}
};

/* ************************************************************************** */

template <typename T>
auto reechantillonne(
		grille_dense_3d<T> const &entree,
		double taille_voxel)
{
	auto desc = entree.desc();
	desc.taille_voxel = taille_voxel;

	auto resultat = grille_dense_3d<T>(desc);
	auto res = resultat.desc().resolution;
	auto res0 = entree.desc().resolution;

	for (auto z = 0; z < res.z; ++z) {
		for (auto y = 0; y < res.y; ++y) {
			for (auto x = 0; x < res.x; ++x) {
				auto index = x + (y + z * res.y) * res.x;
				auto valeur = T(0);
				auto poids = 0.0f;

				auto pos_mnd = resultat.index_vers_monde(dls::math::vec3i(x, y, z));
				auto pos_orig = entree.monde_vers_index(pos_mnd);

				auto min_x = std::max(0, pos_orig.x - 1);
				auto min_y = std::max(0, pos_orig.y - 1);
				auto min_z = std::max(0, pos_orig.z - 1);

				auto max_x = std::min(res0.x, pos_orig.x + 1);
				auto max_y = std::min(res0.y, pos_orig.y + 1);
				auto max_z = std::min(res0.z, pos_orig.z + 1);

				for (auto zi = min_z; zi < max_z; ++zi) {
					for (auto yi = min_y; yi < max_y; ++yi) {
						for (auto xi = min_x; xi < max_x; ++xi) {
							auto index0 = xi + (yi + zi * res0.y) * res0.x;
							valeur += entree.valeur(index0);
							poids += 1.0f;
						}
					}
				}

				if (poids != 0.0f) {
					valeur /= poids;
				}

				resultat.valeur(index) = valeur;
			}
		}
	}

	return resultat;
}

}  /* namespace wlk */
