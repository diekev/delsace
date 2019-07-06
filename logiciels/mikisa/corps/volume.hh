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

#include "primitive.hh"

#include "biblinternes/math/vecteur.hh"

#include "biblinternes/geometrie/limites.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/structures/tableau.hh"

enum class type_volume : char {
	SCALAIRE,
	VECTOR,
};

/* ************************************************************************** */

/* idée pour un typage strict
using co_monde = dls::math::vec3f;
using co_local = dls::math::vecteur<10, float, 0, 1, 2>;    // (0.0 : 1.0)
using co_continue = dls::math::vecteur<11, float, 0, 1, 2>; // (0.0 : res.0)
using co_discrete = dls::math::vec3i;    // (0 : res - 1)
*/

/* ************************************************************************** */

/**
 * Coordonnées :
 * - mondiales
 * - locales (entre 0 et 1)
 * - voxiales (entre 0 et res - 1)
 */

/**
 * L'espace voxel peut être accéder de deux façons : par des coordonnées entière
 * ou par des coordonnées décimales. L'accès entier est utilisé pour des accès
 * directs à un voxel individuel, et l'accès décimal pour les interpolations. Il
 * faut faire attention en convertissant entre les deux. Le centre du voxel
 * (0, 0, 0) a les coordonnées (0.5, 0.5, 0.5). Ainsi, les côtés d'un champs
 * avec une résolution de 100 sont à 0.0 et 100.0 quand on utilise des
 * coordonnées décimales, mais à 0 et 99 pour des entières. Un bon survol de
 * ceci peut se trouver dans un article de Paul S. Heckbert :
 * What Are The Coordinates Of A Pixel? [Heckbert, 1990]
 */

//static int continue_vers_discret(float cont)
//{
//	return static_cast<int>(std::floor(cont));
//}

//static float discret_vers_continue(int disc)
//{
//	return static_cast<float>(disc) + 0.5f;
//}

namespace dls::math {

template <typename Ent, int O, typename Dec, int... Ns>
[[nodiscard]] auto continue_vers_discret(vecteur<O, Dec, Ns...> const &cont) noexcept
{
	static_assert(std::is_floating_point<Dec>::value
				  && std::is_integral<Ent>::value,
				  "continue_vers_discret va de décimal à entier");

	auto tmp = vecteur<O, Ent, Ns...>();
	((tmp[Ns] = static_cast<Ent>(std::floor(cont[Ns]))), ...);
	return tmp;
}

template <typename Dec, int O, typename Ent, int... Ns>
[[nodiscard]] auto discret_vers_continue(vecteur<O, Ent, Ns...> const &cont) noexcept
{
	static_assert(std::is_floating_point<Dec>::value
				  && std::is_integral<Ent>::value,
				  "discret_vers_continue va de entier à décimal");

	auto tmp = vecteur<O, Dec, Ns...>();
	((tmp[Ns] = static_cast<Dec>(cont[Ns]) + static_cast<Dec>(0.5)), ...);
	return tmp;
}

}

class BaseGrille {
protected:
	dls::math::vec3i m_res = dls::math::vec3i(0);
	float m_taille_voxel{};
	size_t m_nombre_voxels = 0;

	limites3f m_etendu{};
	limites3f m_fenetre_donnees{};

	long calcul_index(size_t x, size_t y, size_t z) const;

	bool hors_des_limites(size_t x, size_t y, size_t z) const;

public:
	/**
	 * Une grille peut avoir plusieurs limites : les limites de son tampon de
	 * voxels, ou les limites de sa fenêtre de données.
	 */
	BaseGrille(limites3f const &etendu, limites3f const &fenetre_donnees, float taille_voxel);

	virtual ~BaseGrille() = default;

	virtual BaseGrille *copie() const = 0;
	virtual type_volume type() const = 0;

	/* converti un point de l'espace voxel discret vers l'espace unitaire */
	dls::math::vec3f index_vers_unit(dls::math::vec3i const &vsp) const;

	/* converti un point de l'espace voxel continue vers l'espace unitaire */
	dls::math::vec3f index_vers_unit(dls::math::vec3f const &vsp) const;

	/* converti un point de l'espace voxel continue vers l'espace unitaire */
	dls::math::vec3f index_vers_monde(dls::math::vec3i const &isp) const;

	/* converti un point de l'espace unitaire (0.0 - 1.0) vers l'espace mondiale */
	dls::math::vec3f unit_vers_monde(dls::math::vec3f const &vsp) const;

	/* converti un point de l'espace mondiale vers l'espace unitaire (0.0 - 1.0) */
	dls::math::vec3f monde_vers_unit(dls::math::vec3f const &wsp) const;

	/* converti un point de l'espace mondiale vers l'espace continue */
	dls::math::vec3f monde_vers_continue(dls::math::vec3f const &wsp) const;

	dls::math::vec3i resolution() const;

	limites3f const &etendu() const;

	limites3f const &fenetre_donnees() const;

	float taille_voxel() const;
};

/* ************************************************************************** */

#undef UTILISE_TUILES

#ifdef UTILISE_TUILES

#include <cassert>

static constexpr auto TAILLE_TUILLE = 16;

template <typename T>
struct Tuile {
	//T donnees[TAILLE_TUILLE * TAILLE_TUILLE * TAILLE_TUILLE];

	/* optimisation pour les tuiles ayant une valeur constante */
	T valeur = T(0);
	T *donnees = nullptr;

	/* position dans l'espace pour accélerer le calcul des index */
	dls::math::vec3i pos;
};
#endif

template <typename T>
class Grille : public BaseGrille {
protected:
#ifdef UTILISE_TUILES
	std::vector<Tuile<T> *> m_tuiles{};
	std::vector<size_t> m_table_index{};
	dls::math::vec3i m_tuile{};
#else
	dls::tableau<T> m_donnees = {};
#endif

	T m_arriere_plan = T(0);
	T m_dummy = T(0);
	size_t m_nombre_tuiles{};

public:
	Grille(limites3f const &etendu, limites3f const &fenetre_donnees, float taille_voxel)
		: BaseGrille(etendu, fenetre_donnees, taille_voxel)
	{
#ifdef UTILISE_TUILES
		m_tuile.x = m_res[0] / TAILLE_TUILLE;
		m_tuile.y = m_res[1] / TAILLE_TUILLE;
		m_tuile.z = m_res[2] / TAILLE_TUILLE;

		m_nombre_tuiles = m_tuile.x * m_tuile.y * m_tuile.z;

		m_table_index.resize(m_nombre_tuiles);
		std::fill(m_table_index.begin(), m_table_index.end(), -1ul);
#else
		m_donnees.redimensionne(static_cast<long>(m_nombre_voxels), T(0));
#endif
	}

	T &valeur(size_t index) const
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

#ifdef UTILISE_TUILES
		assert(false);
		return m_arriere_plan;
#else
		return m_donnees[index];
#endif
	}

	T const &valeur(size_t x, size_t y, size_t z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

#ifdef UTILISE_TUILES
		auto tx = x / TAILLE_TUILLE;
		auto ty = y / TAILLE_TUILLE;
		auto tz = z / TAILLE_TUILLE;

		auto idx_table = tx + ty * m_tuile.x + tz * m_tuile.x * m_tuile.y;

		if (m_table_index[idx_table] == -1ul) {
			return m_arriere_plan;
		}

		auto const &tuile = m_tuiles[m_table_index[idx_table]];

		tx = x - tuile->pos.x; // x % TAILLE_TUILLE;
		ty = y - tuile->pos.y; // y % TAILLE_TUILLE;
		tz = z - tuile->pos.z; // z % TAILLE_TUILLE;

		return tuile->donnees[tx + TAILLE_TUILLE * (ty + tz * TAILLE_TUILLE)];
#else
		return m_donnees[calcul_index(x, y, z)];
#endif
	}

	/* XXX ne fais pas de vérification de limites */
	T &valeur(size_t x, size_t y, size_t z)
	{
		if (hors_des_limites(x, y, z)) {
			return m_dummy;
		}

#ifdef UTILISE_TUILES
		auto tx = x / TAILLE_TUILLE;
		auto ty = y / TAILLE_TUILLE;
		auto tz = z / TAILLE_TUILLE;

		auto idx_table = tx + ty * m_tuile.x + tz * m_tuile.x * m_tuile.y;

		if (m_table_index[idx_table] == -1ul) {
			return m_dummy;
		}

		auto &tuile = m_tuiles[m_table_index[idx_table]];

		tx = x - tuile->pos.x; // x % TAILLE_TUILLE;
		ty = y - tuile->pos.y; // y % TAILLE_TUILLE;
		tz = z - tuile->pos.z; // z % TAILLE_TUILLE;

		return tuile->donnees[tx + TAILLE_TUILLE * (ty + tz * TAILLE_TUILLE)];
#else
		return m_donnees[calcul_index(x, y, z)];
#endif
	}

	void valeur(size_t index, T v)
	{
		if (index >= m_nombre_voxels) {
			return;
		}

#ifdef UTILISE_TUILES
		assert(false);
#else
		m_donnees[index] = v;
#endif
	}

	void valeur(size_t x, size_t y, size_t z, T v)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

#ifdef UTILISE_TUILES
		auto tx = x / TAILLE_TUILLE;
		auto ty = y / TAILLE_TUILLE;
		auto tz = z / TAILLE_TUILLE;

		auto idx_table = tx + ty * m_tuile.x + tz * m_tuile.x * m_tuile.y;

		auto tuile = static_cast<Tuile<T>>(nullptr);

		if (m_table_index[idx_table] == -1ul) {
			tuile = new Tuile<T>{};
		}
		else {
			tuile = m_tuiles[m_table_index[idx_table]];
		}

		tx = x - tuile->pos.x; // x % TAILLE_TUILLE;
		ty = y - tuile->pos.y; // y % TAILLE_TUILLE;
		tz = z - tuile->pos.z; // z % TAILLE_TUILLE;

		tuile->donnees[tx + TAILLE_TUILLE * (ty + tz * TAILLE_TUILLE)] = v;
#else
		m_donnees[calcul_index(x, y, z)] = v;
#endif
	}

	void valeur(dls::math::vec3i const &pos, T v)
	{
		this->valeur(static_cast<size_t>(pos.x), static_cast<size_t>(pos.y), static_cast<size_t>(pos.z), v);
	}

	void copie_donnees(const Grille<T> &grille)
	{
#ifdef UTILISE_TUILES
#else
		for (size_t i = 0; i < m_nombre_voxels; ++i) {
			m_donnees[i] = grille.m_donnees[i];
		}
#endif
	}

	void arriere_plan(const T &v)
	{
		m_arriere_plan = v;
	}

	void const *donnees() const
	{
#ifdef UTILISE_TUILES
		return nullptr;
#else
		return m_donnees.donnees();
#endif
	}

	size_t taille_octet() const
	{
#ifdef UTILISE_TUILES
		return m_tuiles.size() * (TAILLE_TUILLE * TAILLE_TUILLE * TAILLE_TUILLE) * sizeof(T);
#else
		return m_nombre_voxels * sizeof(T);
#endif
	}

	BaseGrille *copie() const override
	{
		auto grille = memoire::loge<Grille<T>>("grille", etendu(), fenetre_donnees(), taille_voxel());
		grille->m_arriere_plan = this->m_arriere_plan;

#ifdef UTILISE_TUILES
#else
		grille->m_donnees = this->m_donnees;
#endif

		return grille;
	}

	type_volume type() const override
	{
		return type_volume::SCALAIRE;
	}

	void echange(Grille<T> &autre)
	{
#ifdef UTILISE_TUILES
		m_tuiles.swap(autre.m_tuiles);
		m_tuiles.swap(autre.m_tuiles);
		std::swap(m_tuile, autre.m_tuile);
#else
		m_donnees.swap(autre.m_donnees);
#endif

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(m_dummy, autre.m_dummy);
		std::swap(m_nombre_tuiles, autre.m_nombre_tuiles);
	}
};

class GrilleMAC : public Grille<dls::math::vec3f> {
public:
	dls::math::vec3f valeur_centree(dls::math::vec3i const &pos)
	{
		return valeur_centree(pos.x, pos.y, pos.z);
	}

	dls::math::vec3f valeur_centree(int i, int j, int k)
	{
		auto idx = calcul_index(static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k));

		return dls::math::vec3f(
					0.5f * (m_donnees[idx].x + m_donnees[idx + 1].x),
					0.5f * (m_donnees[idx].y + m_donnees[idx + m_res.x].y),
					0.5f * (m_donnees[idx].z + m_donnees[idx + m_res.x * m_res.y].z));
	}
};

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
		auto dvsp = dls::math::continue_vers_discret<int>(vsp);

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
		auto c = dls::math::continue_vers_discret<int>(p);

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
	Grille<T> m_grille;

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
		auto c = dls::math::continue_vers_discret<int>(p);

		float poids[3];
		auto valeur = static_cast<T>(0.0);

		for (int i = 0; i < 2; ++i) {
			int cur_x = c[0] + i;
			poids[0] = 1.0f - std::abs(p[0] - cur_x);

			for (int j = 0; j < 2; ++j) {
				int cur_y = c[1] + j;
				poids[1] = 1.0f - std::abs(p[1] - cur_y);

				for (int k = 0; k < 2; ++k) {
					int cur_z = c[2] + k;
					poids[2] = 1.0f - std::abs(p[2] - cur_z);

					valeur += poids[0] * poids[1] * poids[2] * m_grille.valeur(cur_x, cur_y, cur_z);
				}
			}
		}

		return valeur;
	}
};

/* ************************************************************************** */

class Volume final : public Primitive {
public:
	BaseGrille *grille = nullptr;

	~Volume();

	type_primitive type_prim() const;
};
