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

#include "volume.hh"

/* ************************************************************************** */

/**
 * Le but fondamental d'un tampon de voxel est évidemment d'être écrit vers, et
 * lu depuis. Pour modéliser des tampons complexes nécessite plus que d'écrire
 * une valeur à un voxel, par exemple : tampon(x, y, z) = valeur. Il faut
 * également pouvoir écrire entre les voxels.
 */
template <typename T>
struct Rasteriseur {
private:
	Grille<T> &m_grille;

public:
	explicit Rasteriseur(Grille<T> &grille)
		: m_grille(grille)
	{}

	Grille<T> const &grille() const
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
		/* continue_vers_discret nous donne la même chose que l'on recherche :
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
		for (int k = 0; k < 2; ++k) {
			fraction[2] = 1.0f - fraction[2];

			for (int j = 0; j < 2; ++j) {
				fraction[1] = 1.0f - fraction[1];

				for (int i = 0; i < 2; ++i) {
					fraction[0] = 1.0f - fraction[0];

					auto poids = fraction[0] * fraction[1] * fraction[2];
					m_grille.valeur(static_cast<size_t>(c.x + i), static_cast<size_t>(c.y + j), static_cast<size_t>(c.z + k)) += valeur * poids;
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
	Grille<T> const &m_grille;

public:
	explicit Echantilloneuse(Grille<T> const &grille)
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

		for (int i = 0; i < 2; ++i) {
			int cur_x = c[0] + i;
			poids[0] = 1.0f - std::abs(p[0] - static_cast<float>(cur_x));

			for (int j = 0; j < 2; ++j) {
				int cur_y = c[1] + j;
				poids[1] = 1.0f - std::abs(p[1] - static_cast<float>(cur_y));

				for (int k = 0; k < 2; ++k) {
					int cur_z = c[2] + k;
					poids[2] = 1.0f - std::abs(p[2] - static_cast<float>(cur_z));

					valeur += poids[0] * poids[1] * poids[2] * m_grille.valeur(static_cast<size_t>(cur_x), static_cast<size_t>(cur_y), static_cast<size_t>(cur_z));
				}
			}
		}

		return valeur;
	}
};

/* ************************************************************************** */

template <typename T>
auto reechantillonne(
		Grille<T> const &entree,
		float taille_voxel)
{
	auto desc = entree.desc();
	desc.taille_voxel = taille_voxel;

	auto resultat = Grille<T>(desc);
	auto res = resultat.resolution();
	auto res0 = entree.resolution();

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
