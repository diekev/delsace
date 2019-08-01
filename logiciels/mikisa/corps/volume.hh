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

#include "biblinternes/math/limites.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/structures/plage.hh"
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

struct description_volume {
	limites3f etendues{};
	limites3f fenetre_donnees{};
	dls::math::vec3i resolution{};
	float taille_voxel = 0.0f;
};

class BaseGrille {
protected:
	description_volume m_desc{};
	size_t m_nombre_voxels = 0;	

	bool hors_des_limites(size_t x, size_t y, size_t z) const;

public:
	BaseGrille() = default;

	/**
	 * Une grille peut avoir plusieurs limites : les limites de son tampon de
	 * voxels, ou les limites de sa fenêtre de données.
	 */
	BaseGrille(description_volume const &descr);

	virtual ~BaseGrille() = default;

	virtual BaseGrille *copie() const = 0;
	virtual type_volume type() const = 0;
	virtual bool est_eparse() const = 0;

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

	/* converti un point de l'espace mondiale vers l'espace index */
	dls::math::vec3i monde_vers_index(dls::math::vec3f const &wsp) const;

	description_volume const &desc() const;

	dls::math::vec3i resolution() const;

	limites3f const &etendu() const;

	limites3f const &fenetre_donnees() const;

	float taille_voxel() const;

	long calcul_index(size_t x, size_t y, size_t z) const;

	long nombre_voxels() const;
};

/* ************************************************************************** */

template <typename T>
class Grille : public BaseGrille {
protected:
	dls::tableau<T> m_donnees = {};

	T m_arriere_plan = T(0);
	T m_dummy = T(0);

public:
	Grille() = default;

	Grille(description_volume const &descr)
		: BaseGrille(descr)
	{
		m_donnees.redimensionne(static_cast<long>(m_nombre_voxels), T(0));
	}

	T &valeur(long index)
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T const &valeur(long index) const
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T const &valeur(size_t x, size_t y, size_t z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[calcul_index(x, y, z)];
	}

	/* XXX ne fais pas de vérification de limites */
	T &valeur(size_t x, size_t y, size_t z)
	{
		if (hors_des_limites(x, y, z)) {
			return m_dummy;
		}

		return m_donnees[calcul_index(x, y, z)];
	}

	void valeur(long index, T v)
	{
		if (index >= m_nombre_voxels) {
			return;
		}

		m_donnees[index] = v;
	}

	void valeur(size_t x, size_t y, size_t z, T v)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[calcul_index(x, y, z)] = v;
	}

	void valeur(dls::math::vec3i const &pos, T v)
	{
		this->valeur(static_cast<size_t>(pos.x), static_cast<size_t>(pos.y), static_cast<size_t>(pos.z), v);
	}

	void copie_donnees(const Grille<T> &grille)
	{
		for (auto i = 0; i < m_nombre_voxels; ++i) {
			m_donnees[i] = grille.m_donnees[i];
		}
	}

	void arriere_plan(const T &v)
	{
		m_arriere_plan = v;
	}

	void const *donnees() const
	{
		return m_donnees.donnees();
	}

	size_t taille_octet() const
	{
		return m_nombre_voxels * sizeof(T);
	}

	BaseGrille *copie() const override
	{
		auto grille = memoire::loge<Grille<T>>("grille", desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_donnees = this->m_donnees;

		return grille;
	}

	bool est_eparse() const override
	{
		return false;
	}

	type_volume type() const override
	{
		return type_volume::SCALAIRE;
	}

	void echange(Grille<T> &autre)
	{
		std::swap(m_desc.etendues, autre.m_desc.etendues);
		std::swap(m_desc.resolution, autre.m_desc.resolution);
		std::swap(m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(m_desc.taille_voxel, autre.m_desc.taille_voxel);

		m_donnees.echange(autre.m_donnees);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(m_dummy, autre.m_dummy);
		std::swap(m_nombre_voxels, autre.m_nombre_voxels);
	}
};

class GrilleMAC : public Grille<dls::math::vec3f> {
public:
	GrilleMAC(description_volume const &descr)
		: Grille<dls::math::vec3f>(descr)
	{}

	dls::math::vec3f valeur_centree(dls::math::vec3i const &pos)
	{
		return valeur_centree(pos.x, pos.y, pos.z);
	}

	dls::math::vec3f valeur_centree(int i, int j, int k)
	{
		if (hors_des_limites(static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k))) {
			return m_arriere_plan;
		}

		auto idx = calcul_index(static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k));

		auto vc = this->valeur(idx);
		auto vx = this->valeur(static_cast<size_t>(i) + 1, static_cast<size_t>(j), static_cast<size_t>(k));
		auto vy = this->valeur(static_cast<size_t>(i), static_cast<size_t>(j) + 1, static_cast<size_t>(k));
		auto vz = this->valeur(static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k) + 1);

		return dls::math::vec3f(
					0.5f * (vc.x + vx.x),
					0.5f * (vc.y + vy.y),
					0.5f * (vc.z + vz.z));
	}
};

/* ************************************************************************** */

static constexpr auto TAILLE_TUILE = 8;

template <typename T>
struct grille_eparse : public BaseGrille {
	using type_valeur = T;

	struct tuile {
		type_valeur donnees[static_cast<size_t>(TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE)];
		dls::math::vec3i min{};
		dls::math::vec3i max{};
		bool garde = false;
	};

private:
	dls::tableau<long> m_index_tuiles{};
	dls::tableau<tuile *> m_tuiles{};

	int m_tuiles_x = 0;
	int m_tuiles_y = 0;
	int m_tuiles_z = 0;

	type_valeur m_arriere_plan = type_valeur(0);

	bool hors_des_limites(int i, int j, int k) const
	{
		if (i < 0 || i >= m_tuiles_x) {
			return true;
		}

		if (j < 0 || j >= m_tuiles_y) {
			return true;
		}

		if (k < 0 || k >= m_tuiles_z) {
			return true;
		}

		return false;
	}

	inline long index_tuile(long i, long j, long k) const
	{
		return i + (j + k * m_tuiles_y) * m_tuiles_x;
	}

	inline int converti_nombre_tuile(int i) const
	{
		return i / TAILLE_TUILE + ((i % TAILLE_TUILE) != 0);
	}

public:
	using plage_tuile = dls::plage_continue<tuile *>;
	using plage_tuile_const = dls::plage_continue<tuile * const>;

	grille_eparse(description_volume const &descr)
		: BaseGrille(descr)
	{
		m_tuiles_x = converti_nombre_tuile(m_desc.resolution.x);
		m_tuiles_y = converti_nombre_tuile(m_desc.resolution.y);
		m_tuiles_z = converti_nombre_tuile(m_desc.resolution.z);

		auto nombre_tuiles = m_tuiles_x * m_tuiles_y * m_tuiles_z;

		m_index_tuiles.redimensionne(nombre_tuiles, -1l);
	}

	~grille_eparse()
	{
		for (auto t : m_tuiles) {
			memoire::deloge("tuile", t);
		}
	}

	void assure_tuiles(limites3f const &fenetre_donnees)
	{
		auto min = monde_vers_index(fenetre_donnees.min);
		auto max = monde_vers_index(fenetre_donnees.max);

		assure_tuiles(limites3i{min, max});
	}

	void assure_tuiles(limites3i const &fenetre_donnees)
	{
		auto min_tx = fenetre_donnees.min.x / TAILLE_TUILE;
		auto min_ty = fenetre_donnees.min.y / TAILLE_TUILE;
		auto min_tz = fenetre_donnees.min.z / TAILLE_TUILE;

		auto max_tx = converti_nombre_tuile(fenetre_donnees.max.x);
		auto max_ty = converti_nombre_tuile(fenetre_donnees.max.y);
		auto max_tz = converti_nombre_tuile(fenetre_donnees.max.z);

		for (auto z = min_tz; z < max_tz; ++z) {
			for (auto y = min_ty; y < max_ty; ++y) {
				for (auto x = min_tx; x < max_tx; ++x) {
					auto idx_tuile = index_tuile(x, y, z);

					if (m_index_tuiles[idx_tuile] != -1) {
						continue;
					}

					auto t = memoire::loge<tuile>("tuile");
					t->min = dls::math::vec3i(x, y, z) * TAILLE_TUILE;
					t->max = t->min + dls::math::vec3i(TAILLE_TUILE);

					m_index_tuiles[idx_tuile] = m_tuiles.taille();
					m_tuiles.pousse(t);
				}
			}
		}
	}

	void elague()
	{
		auto tuiles_gardees = dls::tableau<tuile *>();

		for (auto t : m_tuiles) {
			t->garde = false;

			for (auto i = 0; i < (TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE); ++i) {
				if (t->donnees[i] != m_arriere_plan) {
					tuiles_gardees.pousse(t);
					t->garde = true;
					break;
				}
			}
		}

		auto suppr = 0;
		for (auto t : m_tuiles) {
			if (t->garde == false) {
				memoire::deloge("tuile", t);
				suppr++;
			}
		}

		m_tuiles = tuiles_gardees;

		for (auto i = 0; i < m_index_tuiles.taille(); ++i) {
			m_index_tuiles[i] = -1;
		}

		auto i = 0;
		for (auto t : m_tuiles) {
			auto idx_tuile = index_tuile(t->min.x / TAILLE_TUILE, t->min.y / TAILLE_TUILE, t->min.z / TAILLE_TUILE);
			m_index_tuiles[idx_tuile] = i++;
		}
	}

	float valeur(int i, int j, int k) const
	{
		// trouve la tuile
		auto it = i / TAILLE_TUILE;
		auto jt = j / TAILLE_TUILE;
		auto kt = k / TAILLE_TUILE;

		if (hors_des_limites(it, jt, kt)) {
			return m_arriere_plan;
		}

		auto idx_tuile = index_tuile(it, jt, kt);

		if (m_index_tuiles[idx_tuile] == -1) {
			return m_arriere_plan;
		}

		auto t = m_tuiles[idx_tuile];

		// calcul l'index dans la tuile
		auto xt = i - it * TAILLE_TUILE;
		auto yt = j - jt * TAILLE_TUILE;
		auto zt = k - kt * TAILLE_TUILE;

		return t->donnees[static_cast<size_t>(xt + (yt + zt * TAILLE_TUILE) * TAILLE_TUILE)];
	}

	plage_tuile plage()
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile(d, f);
	}

	plage_tuile_const plage() const
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile_const(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile_const(d, f);
	}

	BaseGrille *copie() const override
	{
		auto grille = memoire::loge<grille_eparse<T>>("grille", desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_index_tuiles = this->m_index_tuiles;
		grille->m_tuiles = this->m_tuiles;

		return grille;
	}

	bool est_eparse() const override
	{
		return true;
	}

	type_volume type() const override
	{
		return type_volume::SCALAIRE;
	}

	void echange(Grille<T> &autre)
	{
		std::swap(m_desc.etendues, autre.m_desc.etendues);
		std::swap(m_desc.resolution, autre.m_desc.resolution);
		std::swap(m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(m_desc.taille_voxel, autre.m_desc.taille_voxel);

		m_index_tuiles.echange(autre.m_index_tuiles);
		m_tuiles.echange(autre.m_tuiles);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(m_tuiles_x, autre.m_tuiles_x);
		std::swap(m_tuiles_y, autre.m_tuiles_y);
		std::swap(m_tuiles_z, autre.m_tuiles_z);
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

class Volume final : public Primitive {
public:
	BaseGrille *grille = nullptr;

	~Volume();

	type_primitive type_prim() const;
};
