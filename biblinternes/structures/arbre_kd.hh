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

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

#include <tbb/parallel_invoke.h>

/**
 * Un arbre k-d pour stocker des points.
 * Les méthodes cherche_points et trouve_plus_proche retournent les points
 * voisins d'une location donnée.
 *
 * L'implémentation dérive de celle de Cem Yuksel : http://cemyuksel.com/.
 */
template <typename type_point, typename type_taille = int>
struct ArbreKd {
	using type_valeur = typename type_point::type_scalaire;
	static constexpr auto DIMENSIONS = type_point::nombre_composants;

	/* ********************************************************************** */

	ArbreKd() = default;

	ArbreKd(type_taille nombre_pts, type_point const *pts, type_taille const *index_persos = nullptr)
		: m_points()
		, m_nombre_points(0)
	{
		construit(nombre_pts, pts, index_persos);
	}

	~ArbreKd() = default;

	ArbreKd(ArbreKd const &) = default;
	ArbreKd &operator=(ArbreKd const &) = default;

	/* ********************************************************************** */

	type_taille compte_points() const
	{
		return m_nombre_points - 1;
	}

	type_point const &pos_point(type_taille i) const
	{
		return m_points[i + 1].pos();
	}

	type_taille index_point(type_taille i) const
	{
		return m_points[i + 1].index();
	}

	/* ********************************************************************** */

	/**
	 * Construit un arbre k-d point les points données.
	 * Les positions sont stockées en interne.
	 */
	void construit(type_taille nombre_pts, type_point const *pts)
	{
		construit_avec_fonction(nombre_pts, [&pts](type_taille i){ return pts[i]; });
	}

	/**
	 * Construit un arbre k-d point les points données.
	 * Les positions et les index donnés sont stockées en interne.
	 */
	void construit(type_taille nombre_pts, type_point const *pts, const type_taille *index_persos)
	{
		construit_avec_fonction(nombre_pts, [&pts](type_taille i){ return pts[i]; }, [&index_persos](type_taille i){ return index_persos[i]; });
	}

	/**
	 * Construit un arbre k-d point des points données.
	 * Les positions sont passées par une fonction donnée et stockées en interne.
	 */
	template <typename FoncPosPoint>
	void construit_avec_fonction(type_taille nombre_pts, FoncPosPoint fonc_pos_point)
	{
		construit_avec_fonction(nombre_pts, fonc_pos_point, [](type_taille i){ return i; });
	}

	/**
	 * Construit un arbre k-d point des points données.
	 * Les positions et index sont passés par une fonction donnée et stockés en
	 * interne.
	 */
	template <typename FoncPosPoint, typename FoncIndexPersos>
	void construit_avec_fonction(type_taille nombre_pts, FoncPosPoint fonc_pos_point, FoncIndexPersos fonc_index_persos)
	{
		m_points.efface();
		m_nombre_points = nombre_pts;

		if (m_nombre_points == 0) {
			return;
		}

		m_points.redimensionne((m_nombre_points | 1) + 1);

		auto orig = dls::tableau<DonneesPoint>(m_nombre_points);

		type_point limite_min((std::numeric_limits<type_valeur>::max)()), limite_max((std::numeric_limits<type_valeur>::min)());

		for (type_taille i=0; i<m_nombre_points; i++) {
			type_point p = fonc_pos_point(i);
			orig[i].init(p, fonc_index_persos(i));

			for (auto j = 0ul; j < DIMENSIONS; j++) {
				if (limite_min[j] > p[j]) {
					limite_min[j] = p[j];
				}

				if (limite_max[j] < p[j]) {
					limite_max[j] = p[j];
				}
			}
		}

		construit_arbre(&orig[0], limite_min, limite_max, 1, 0, m_nombre_points);

		if ((m_nombre_points & 1) == 0) {
			/* si le compte de point est pair, nous devons ajouter un faux point */
			m_points[m_nombre_points+1].init(type_point(constantes<type_valeur>::INFINITE), 0, 0);
		}

		m_nombre_interne = m_nombre_points / 2;
	}

	/* ********************************************************************** */

	/**
	 * Méthodes de recherche générale.
	 */

	using type_rappel_recherche_point = std::function<void(type_taille /* index */,
														   type_point const&/* point */,
														   type_valeur /* distance_carree */,
														   type_valeur& /* rayon_carre */)>;

	/**
	 * Cherche tous les points près de la position donnée dans le rayon donnée.
	 * Pour chaque point trouvé, la fonction de rappel "point_trouve" est
	 * appelée.
	 *
	 * Le fonction point_trouve donnée peut réduire la valeur rayon_carre de la
	 * fonction de rappel.
	 */
	void cherche_points(
			type_point const &position,
			type_valeur rayon,
			type_rappel_recherche_point point_trouve) const
	{
		auto r2 = rayon * rayon;
		cherche_points(position, r2, point_trouve, 1);
	}

	/**
	 * Utilisée par l'une des méthodes cherche_points().
	 *
	 * Pour chaque point, garde son index, sa position, et sa distance carrée
	 * d'une position de recherche donnée.
	 */
	struct InfoPoint {
		type_taille index = 0;
		type_point pos{};
		type_valeur distance_carree = 0;

		bool operator<(const InfoPoint &b) const
		{
			return distance_carree < b.distance_carree;
		}
	};

	/**
	 * Cherche les compte_max points les plus près de la position donnée dans le
	 * rayon données. Retourne le compte de points trouvés.
	 */
	int cherche_points(
			type_point const &position,
			type_valeur rayon,
			type_taille compte_max,
			InfoPoint *points_proches) const
	{
		int points_trouve = 0;

		cherche_points(position, rayon, [&](type_taille i, type_point const &p, type_valeur d2, type_valeur &r2)
		{
			if (points_trouve == compte_max) {
				std::pop_heap(points_proches, points_proches+compte_max);
				points_proches[compte_max-1].index = i;
				points_proches[compte_max-1].pos = p;
				points_proches[compte_max-1].distance_carree = d2;
				std::push_heap(points_proches, points_proches+compte_max);
				r2 = points_proches[0].distance_carree;
			}
			else {
				points_proches[points_trouve].index = i;
				points_proches[points_trouve].pos = p;
				points_proches[points_trouve].distance_carree = d2;
				points_trouve++;

				if (points_trouve == compte_max) {
					std::make_heap(points_proches, points_proches+compte_max);
					r2 = points_proches[0].distance_carree;
				}
			}
		});

		return points_trouve;
	}

	/**
	 * Cherche les compte_max points les plus près de la position donnée avec un
	 * rayon infini. Retourne le compte de points trouvés.
	 */
	int cherche_points(
			type_point const &position,
			type_taille compte_max,
			InfoPoint *points_proches) const
	{
		return cherche_points(position, (std::numeric_limits<type_valeur>::max)(), compte_max, points_proches);
	}

	/* ********************************************************************** */

	/**
	 * Méthodes de recherche de point le plus proche.
	 */

	/**
	 * Retourne le point le plus proche de la position donnée dans le rayon
	 * donné.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche(type_point const &position, type_valeur rayon, type_taille &index_proche, type_point &position_proche, type_valeur &distance_carree_proche) const
	{
		bool found = false;
		type_valeur dist2 = rayon * rayon;
		cherche_points(position, dist2, [&](type_taille i, type_point const &p, type_valeur d2, type_valeur &r2){ found=true; index_proche=i; position_proche=p; distance_carree_proche=d2; r2=d2; }, 1);
		return found;
	}

	/**
	 * Retourne le point le plus proche de la position donnée.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche(type_point const &position, type_taille &index_proche, type_point &position_proche, type_valeur &distance_carree_proche) const
	{
		return trouve_plus_proche(position, (std::numeric_limits<type_valeur>::max)(), index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne le point le plus proche et son index de la position donnée dans
	 * le rayon donné.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche(type_point const &position, type_valeur rayon, type_taille &index_proche, type_point &position_proche) const
	{
		type_valeur distance_carree_proche;
		return trouve_plus_proche(position, rayon, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne le point le plus proche et son index de la position donnée.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche(type_point const &position, type_taille &index_proche, type_point &position_proche) const
	{
		type_valeur distance_carree_proche;
		return trouve_plus_proche(position, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne l'index du point le plus proche de la position donnée dans le
	 * rayon donné.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_index(type_point const &position, type_valeur rayon, type_taille &index_proche) const
	{
		type_valeur distance_carree_proche;
		type_point position_proche;
		return trouve_plus_proche(position, rayon, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne l'index du point le plus proche de la position donnée.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_index(type_point const &position, type_taille &index_proche) const
	{
		type_valeur distance_carree_proche;
		type_point position_proche;
		return trouve_plus_proche(position, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne la position du point le plus proche de la position donnée dans
	 * le rayon donné.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_position(type_point const &position, type_valeur rayon, type_point &position_proche) const
	{
		type_taille index_proche;
		type_valeur distance_carree_proche;
		return trouve_plus_proche(position, rayon, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne la position du point le plus proche de la position donnée.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_position(type_point const &position, type_point &position_proche) const
	{
		type_taille index_proche;
		type_valeur distance_carree_proche;
		return trouve_plus_proche(position, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne la distance carrée du point le plus proche de la position donnée
	 * dans le rayon donné.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_distance_carree(type_point const &position, type_valeur rayon, type_valeur &distance_carree_proche) const
	{
		type_taille index_proche;
		type_point position_proche;
		return trouve_plus_proche(position, rayon, index_proche, position_proche, distance_carree_proche);
	}

	/**
	 * Retourne la distance carrée du point le plus proche de la position donnée.
	 * Retourne vrai si un point est trouvé.
	 */
	bool trouve_plus_proche_distance_carree(type_point const &position, type_valeur &distance_carree_proche) const
	{
		type_taille index_proche;
		type_point position_proche;
		return trouve_plus_proche(position, index_proche, position_proche, distance_carree_proche);
	}

private:
	/* ********************************************************************** */

	/**
	 * Structures et méthodes internes.
	 */

	class DonneesPoint {
	private:
		/* Les premiers N bits indiquent le plan de coupe, le reste stocke
		 * l'index du point */
		type_taille m_index_et_plan_de_coupe{};
		type_point m_pos{};

	public:
		void init(type_point const &pt, type_taille index, uint32_t plane = 0)
		{
			m_pos = pt;
			m_index_et_plan_de_coupe = static_cast<int>((static_cast<uint32_t>(index) << NBits()) | (plane & ((1u << NBits()) - 1u)));
		}

		void plan(uint32_t plane)
		{
			m_index_et_plan_de_coupe = static_cast<int>((static_cast<uint32_t>(m_index_et_plan_de_coupe) & (~((1u << NBits()) - 1u))) | plane);
		}

		int plan() const
		{
			return m_index_et_plan_de_coupe & ((1<<NBits())-1);
		}

		type_taille index() const
		{
			return m_index_et_plan_de_coupe >> NBits();
		}

		type_point const &pos() const
		{
			return m_pos;
		}

	private:
		constexpr uint32_t NBits(uint32_t v = DIMENSIONS) const
		{
			return v < 2 ? v : 1 + NBits(v >> 1);
		}
	};

	dls::tableau<DonneesPoint> m_points{};
	type_taille m_nombre_points = 0;

	/* le nombre de noeud interne de l'arbre */
	type_taille m_nombre_interne = 0;

	/**
	 * Méthode principale de construction de l'arbre k-d.
	 */
	void construit_arbre(
			DonneesPoint *orig,
			type_point limite_min,
			type_point limite_max,
			type_taille index_kd,
			type_taille debut_ix,
			type_taille fin_ix)
	{
		type_taille n = fin_ix - debut_ix;

		if (n > 1) {
			auto axis = static_cast<unsigned>(axe_decoupe(limite_min, limite_max));
			type_taille leftSize = taille_sous_arbre_gauche(n);
			type_taille ixMid = debut_ix+leftSize;

			std::nth_element(orig+debut_ix, orig+ixMid, orig+fin_ix,
							 [axis](const DonneesPoint &a, const DonneesPoint &b)
			{
				return a.pos()[axis] < b.pos()[axis];
			});

			m_points[index_kd] = orig[ixMid];
			m_points[index_kd].plan(axis);

			type_point bMax = limite_max;
			bMax[axis] = orig[ixMid].pos()[axis];
			type_point bMin = limite_min;
			bMin[axis] = orig[ixMid].pos()[axis];
			const type_taille parallel_invoke_threshold = 256;

			if (ixMid-debut_ix > parallel_invoke_threshold && fin_ix - ixMid+1 > parallel_invoke_threshold) {
				tbb::parallel_invoke(
					[&]{ construit_arbre(orig, limite_min, bMax, index_kd*2,   debut_ix, ixMid); },
					[&]{ construit_arbre(orig, bMin, limite_max, index_kd*2+1, ixMid+1, fin_ix); }
				);
			}
			else {
				construit_arbre(orig, limite_min, bMax, index_kd*2,   debut_ix, ixMid);
				construit_arbre(orig, bMin, limite_max, index_kd*2+1, ixMid+1, fin_ix);
			}
		}
		else if (n > 0) {
			m_points[index_kd] = orig[debut_ix];
		}
	}

	/**
	 * Retourne le nombre total de noeuds du sous-arbre gauche d'un arbre k-d
	 * complet de taille n.
	 */
	static type_taille taille_sous_arbre_gauche(type_taille n)
	{
		/* taille de l'arbre complet */
		auto arbre = n;

		for (type_taille s = 1; s < static_cast<int>(8 * sizeof(type_taille)); s *= 2) {
			arbre |= arbre >> s;
		}

		/* taille de l'enfant gauche complet */
		auto gauche = arbre >> 1;
		/* taille de l'enfant gauche complet sans les feuilles */
		auto droite = gauche >> 1;

		return (gauche + droite + 1 <= n) ? gauche : n - droite - 1;
	}

	// Returns axis with the largest span, used as the splitting axis for building the k-d tree
	static int axe_decoupe(type_point const &limite_min, type_point const &limite_max)
	{
		auto d = limite_max - limite_min;
		auto axe = 0;
		auto dmax = d[0];

		for (auto j = 1ul; j < DIMENSIONS; j++) {
			if (dmax < d[j]) {
				axe = static_cast<int>(j);
				dmax = d[j];
			}
		}

		return axe;
	}

	void cherche_points(
			type_point const &position,
			type_valeur &dist2,
			type_rappel_recherche_point point_trouve,
			type_taille id_noeud) const
	{
		auto pile_id = dls::pile_fixe<type_taille, 8>();

		traverse_plus_pres(position, dist2, point_trouve, id_noeud, pile_id);

		/* vide la pile */
		while (!pile_id.est_vide()) {
			auto loc_id_noeud = pile_id.depile();
			/* vérifie le point de noeud interne */
			auto const &p = m_points[loc_id_noeud];
			auto const pos = p.pos();
			auto axis = p.plan();
			auto dist1 = position[static_cast<size_t>(axis)] - pos[static_cast<size_t>(axis)];

			if (dist1 * dist1 >= dist2) {
				continue;
			}

			/* vérifie son point */
			auto d2 = dls::math::longueur_carree(position - pos);

			if (d2 < dist2) {
				point_trouve(p.index(), pos, d2, dist2);
			}

			/* descend vers l'autre noeud enfant */
			auto child = 2 * loc_id_noeud;
			loc_id_noeud = dist1 < 0 ? child + 1 : child;
			traverse_plus_pres(position, dist2, point_trouve, loc_id_noeud, pile_id);
		}
	}

	void traverse_plus_pres(
			type_point const &position,
			type_valeur &dist2,
			type_rappel_recherche_point point_trouve,
			type_taille id_noeud,
			dls::pile_fixe<type_taille, 8> &pile_id) const
	{
		/* descend vers un noeud feuille le long de la branch la plus proche */
		while (id_noeud <= m_nombre_interne) {
			pile_id.empile(id_noeud);
			auto const &p = m_points[id_noeud];
			auto const pos = p.pos();
			auto axis = p.plan();
			auto dist1 = position[static_cast<size_t>(axis)] - pos[static_cast<size_t>(axis)];
			auto enfant = 2 * id_noeud;
			id_noeud = dist1 < 0 ? enfant : enfant + 1;
		}

		/* Maintenant que nous sommes à une feuille, testons. */
		auto const &p = m_points[id_noeud];
		auto const pos = p.pos();
		auto d2 = dls::math::longueur_carree(position - pos);

		if (d2 < dist2) {
			point_trouve(p.index(), pos, d2, dist2);
		}
	}
};

using arbre_2df = ArbreKd<dls::math::vec2f>;
using arbre_2dd = ArbreKd<dls::math::vec2d>;
using arbre_3df = ArbreKd<dls::math::vec3f>;
using arbre_3dd = ArbreKd<dls::math::vec3d>;
