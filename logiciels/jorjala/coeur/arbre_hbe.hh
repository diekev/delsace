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

#include "biblinternes/math/boite_englobante.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/phys/rayon.hh"
#include "biblinternes/structures/file.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

enum {
	NOEUD_GAUCHE,
	NOEUD_DROITE,
};

static constexpr auto PROFONDEUR_MAX = 24ul;

struct ArbreHBE {
	struct Noeud {
		Noeud() = default;

		BoiteEnglobante limites{};

		long enfants[2];
		long nombre_references = 0;
		long decalage_reference = 0;
		long id_noeud = 0;
		Axe axe_princ{};

		bool est_feuille() const
		{
			return enfants[0] == 0 && enfants[1] == 0;
		}

		double test_intersection_rapide(dls::phys::rayond const &r) const
		{
			return entresection_rapide_min_max(r, limites.min, limites.max);
		}

		double test_intersection_rapide_imper(dls::phys::rayond const &r) const
		{
			return entresection_rapide_min_max_impermeable(r, limites.min, limites.max);
		}
	};

	long nombre_noeud = 0;
	dls::tableau<ArbreHBE::Noeud> noeuds{};
	long nombre_index_refs = 0;
	dls::tableau<long> index_refs{};

	ArbreHBE() = default;
};

double calcul_cout_scission(
		double const scission,
		const ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite);

double trouve_meilleure_scission(
		ArbreHBE::Noeud &noeud,
		dls::tableau<long> &references,
		Axe const direction,
		unsigned int const qualite,
		dls::tableau<BoiteEnglobante> const &boites_alignees,
		long &compte_gauche,
		long &compte_droite);

template <typename TypeDelegue>
auto construit_arbre_hbe(
		TypeDelegue const &delegue_prims,
		unsigned int profondeur_max)
{
	/* rassemble la liste des boîtes alignées */
	auto const nombre_de_boites = delegue_prims.nombre_elements();
	auto boites_alignees  = dls::tableau<BoiteEnglobante>(nombre_de_boites);

	for (auto i = 0; i < boites_alignees.taille(); ++i) {
		boites_alignees[i] = delegue_prims.boite_englobante(i);
	}

	/* tableau pour contenir les noeuds durant la construction */
	auto const feuilles_max = boites_alignees.taille();
	auto const coeff_arbre = 8; /* 2^3 */

	/* La taille estimée fut 2^profondeur_max, mais pouvait réserver plusieurs
	 * gigaoctets pour quelques centaines de triangles. Ceci vient de Blender.
	 */
	auto branches_implicites = [](long coef, long feuiles)
	{
		return std::max(1l, (feuiles + coef - 3) / (coef - 1));
	};

	auto const taille_estimee =
			feuilles_max
			+ branches_implicites(coeff_arbre, feuilles_max)
			+ coeff_arbre;

	auto arbre = dls::tableau<ArbreHBE::Noeud>();
	arbre.reserve(taille_estimee);

	auto compte_noeud = 0u;
	auto compte_couche = 0u;

	auto refs_courantes = dls::tableau<dls::tableau<long>>();
	refs_courantes.reserve(taille_estimee);

	auto couche_courante = dls::tableau<ArbreHBE::Noeud *>();
	auto liste_ref_finale = dls::tableau<long>();

	/* crée un noeud nul pour l'index 0; l'arbre commence à l'index 1 */
	arbre.ajoute(ArbreHBE::Noeud());
	++compte_noeud;

	/* crée une racine avec une boîte englobant toutes les autres */
	arbre.ajoute(ArbreHBE::Noeud());
	++compte_noeud;

	auto racine = &arbre[1];

	for (auto const &boite : boites_alignees) {
		racine->limites.etend(boite);
	}

	/* peuple la racine avec tous les ids */
	racine->nombre_references = nombre_de_boites;
	racine->decalage_reference = 0;
	racine->id_noeud = 1;

	auto references_racine = dls::tableau<long>();
	references_racine.reserve(nombre_de_boites);

	for (auto const &boite : boites_alignees) {
		references_racine.ajoute(boite.id);
	}

	couche_courante.ajoute(racine);
	refs_courantes.ajoute(references_racine);

	/* pour chaque couche, scinde les noeuds et va à la suivante */
	for (auto couche = 0u; couche < profondeur_max; ++couche) {
		auto couche_refs_suivante = dls::tableau<dls::tableau<long>>();
		auto couche_suivante = dls::tableau<ArbreHBE::Noeud *>();

		/* pour chaque noeud dans la couche courante, scinde et pousse résultat */
		auto nombre_noeud_couche = couche_courante.taille();

		if (nombre_noeud_couche > 0) {
			++compte_couche;
		}

		for (auto i = 0; i < nombre_noeud_couche; ++i) {
			auto compte_ref = refs_courantes[i].taille();
			auto noeud = couche_courante[i];

			if (compte_ref <= 5 || (couche == profondeur_max - 1)) {
				noeud->enfants[NOEUD_GAUCHE] = 0;
				noeud->enfants[NOEUD_DROITE] = 0;
				noeud->nombre_references = compte_ref;
				noeud->decalage_reference = liste_ref_finale.taille();

				for (auto j = 0; j < noeud->nombre_references; ++j) {
					liste_ref_finale.ajoute(refs_courantes[i][j]);
				}
			}
			else {
				/* trouve la meilleure scission via SAH et ajoute les enfants à
				 * la couche suivante. */
				auto axe_scission = noeud->limites.ampleur_maximale();
				auto compte_gauche = 0l;
				auto compte_droite = 0l;

				auto meilleure_scission = trouve_meilleure_scission(
							*noeud, refs_courantes[i], axe_scission, 10, boites_alignees, compte_gauche, compte_droite);

				if (compte_gauche == 0 || compte_droite == 0) {
					compte_gauche = 0;
					compte_droite = 0;

					if (axe_scission == Axe::X) {
						axe_scission = Axe::Y;
					}
					else if (axe_scission == Axe::Y) {
						axe_scission = Axe::Z;
					}
					else if (axe_scission == Axe::Z) {
						axe_scission = Axe::X;
					}

					meilleure_scission = trouve_meilleure_scission(
								*noeud, refs_courantes[i], axe_scission, 10, boites_alignees, compte_gauche, compte_droite);
				}

				if (compte_gauche == 0 || compte_droite == 0) {
					compte_gauche = 0;
					compte_droite = 0;

					if (axe_scission == Axe::X) {
						axe_scission = Axe::Y;
					}
					else if (axe_scission == Axe::Y) {
						axe_scission = Axe::Z;
					}
					else if (axe_scission == Axe::Z) {
						axe_scission = Axe::X;
					}

					meilleure_scission = trouve_meilleure_scission(
								*noeud, refs_courantes[i], axe_scission, 10, boites_alignees, compte_gauche, compte_droite);
				}

				if (compte_gauche == 0 || compte_droite == 0) {
					noeud->enfants[NOEUD_GAUCHE] = 0;
					noeud->enfants[NOEUD_DROITE] = 0;
					noeud->nombre_references = compte_ref;
					noeud->decalage_reference = liste_ref_finale.taille();

					for (auto j = 0; j < noeud->nombre_references; ++j) {
						liste_ref_finale.ajoute(refs_courantes[i][j]);
					}
				}
				else {
					/* crée les noeuds gauche et droite */
					arbre.ajoute(ArbreHBE::Noeud());
					++compte_noeud;

					auto id_gauche = arbre.taille() - 1;
					auto gauche = &arbre[id_gauche];
					gauche->id_noeud = id_gauche;
					gauche->nombre_references = compte_gauche;

					arbre.ajoute(ArbreHBE::Noeud());
					++compte_noeud;

					auto id_droite = arbre.taille() - 1;
					auto droite = &arbre[id_droite];
					droite->id_noeud = id_droite;
					droite->nombre_references = compte_droite;

					noeud->enfants[NOEUD_GAUCHE] = id_gauche;
					noeud->enfants[NOEUD_DROITE] = id_droite;
					noeud->axe_princ = axe_scission;

					/* crée les listes des objets gauche et droite en se basant
					 * sur le centroïde */
					auto refs_gauche = dls::tableau<long>();
					auto refs_droite = dls::tableau<long>();
					refs_gauche.reserve(compte_gauche);
					refs_gauche.reserve(compte_droite);

					for (auto j = 0; j < noeud->nombre_references; ++j) {
						auto const &index_ref = refs_courantes[i][j];
						auto const &ref = boites_alignees[index_ref];

						if (ref.centroide[static_cast<size_t>(axe_scission)] <= meilleure_scission) {
							refs_gauche.ajoute(index_ref);
							gauche->limites.etend(ref);
						}
						else {
							refs_droite.ajoute(index_ref);
							droite->limites.etend(ref);
						}
					}

					/* ajoute les enfants à la couche suivante */
					couche_suivante.ajoute(gauche);
					couche_suivante.ajoute(droite);

					couche_refs_suivante.ajoute(refs_gauche);
					couche_refs_suivante.ajoute(refs_droite);
				}
			}
		}

		couche_courante = couche_suivante;
		refs_courantes = couche_refs_suivante;
	}

	auto arbre_hbe = ArbreHBE{};
	arbre_hbe.nombre_noeud = arbre.taille();
	arbre_hbe.noeuds = arbre;
	arbre_hbe.nombre_index_refs = liste_ref_finale.taille();
	arbre_hbe.index_refs = liste_ref_finale;

#if 0
	std::cerr << "-----------------------------------------------\n";
	for (auto noeud : arbre_hbe.noeuds) {
		if (!noeud.est_feuille()) {
			continue;
		}

		std::cerr << "Nombre de réfs : "
				  << noeud.nombre_references
//				  << ", min : " << noeud.limites.min
//				  << ", max : " << noeud.limites.max
				  << ' ';

		auto virgule = '(';
		for (auto i = 0; i < noeud.nombre_references; ++i) {
			auto id_prim = arbre_hbe.index_refs[noeud.decalage_reference + i];
			std::cerr << virgule << id_prim;
			virgule = ',';
		}

		std::cerr <<")\n";
	}
#endif

	return arbre_hbe;
}

/* ************************************************************************** */

struct ElementTraverse {
	ArbreHBE::Noeud const *noeud = nullptr;
	double t = 0.0;
};

inline bool operator<(ElementTraverse const &p1, ElementTraverse const &p2)
{
	return p1.t < p2.t;
}

template <typename T>
auto fast_ray_nearest_hit(
		ArbreHBE::Noeud const *node,
		int *index,
		dls::phys::rayon<T> const &ray,
		dls::math::vec3<T> const &idot_axis,
		T dist)
{
	const T bv[6] = {
		node->limites.min[0],
		node->limites.max[0],
		node->limites.min[1],
		node->limites.max[1],
		node->limites.min[2],
		node->limites.max[2]
	};

	auto t1x = (bv[index[0]] - ray.origine[0]) * idot_axis[0];
	auto t2x = (bv[index[1]] - ray.origine[0]) * idot_axis[0];
	auto t1y = (bv[index[2]] - ray.origine[1]) * idot_axis[1];
	auto t2y = (bv[index[3]] - ray.origine[1]) * idot_axis[1];
	auto t1z = (bv[index[4]] - ray.origine[2]) * idot_axis[2];
	auto t2z = (bv[index[5]] - ray.origine[2]) * idot_axis[2];

	if ((t1x > t2y || t2x < t1y || t1x > t2z || t2x < t1z || t1y > t2z || t2y < t1z) ||
			(t2x < 0.0 || t2y < 0.0 || t2z < 0.0) ||
			(t1x > dist || t1y > dist || t1z > dist)) {
		return constantes<T>::INFINITE;
	}

	return std::max(t1x, std::max(t1y, t1z));
}

/* À FAIRE : la traversé contient un bug au niveau de l'intersection des boites
 * englobantes de l'arbre : la désactiver nous donne un rendu de toutes les
 * primitives, alors qu'avec certains triangles disparaissent.
 */
template <typename TypeDelegue>
auto traverse(
		ArbreHBE const &arbre,
		TypeDelegue const &delegue,
		dls::phys::rayond const &rayon)
{
	/* précalcule quelque données */
	auto ray_dot_axis = dls::math::vec3d();
	auto idot_axis = dls::math::vec3d();
	int index[6];

	static const dls::math::vec3d kdop_axes[3] = {
		dls::math::vec3d(1.0, 0.0, 0.0),
		dls::math::vec3d(0.0, 1.0, 0.0),
		dls::math::vec3d(0.0, 0.0, 1.0)
	};

	for (auto i = 0ul; i < 3; i++) {
	 ray_dot_axis[i] = produit_scalaire(rayon.direction, kdop_axes[i]);
	  idot_axis[i] = 1.0 / ray_dot_axis[i];

	  if (std::abs(ray_dot_axis[i]) < 1e-6) {
		ray_dot_axis[i] = 0.0;
	  }
	  index[2 * i] = idot_axis[i] < 0.0 ? 1 : 0;
	  index[2 * i + 1] = 1 - index[2 * i];
	  index[2 * i] += 2 * static_cast<int>(i);
	  index[2 * i + 1] += 2 * static_cast<int>(i);
	}

	auto const &racine = arbre.noeuds[1];

	auto dist_max = constantes<double>::INFINITE / 2.0;

	//auto distance_courant = racine.test_intersection_rapide(rayon);
	auto distance_courant = fast_ray_nearest_hit(&racine, index, rayon, idot_axis, dist_max);
	auto esect = dls::phys::esectd{};

	if (distance_courant > dist_max) {
		return esect;
	}

	auto t_proche = rayon.distance_max;

	auto file = dls::file_priorite<ElementTraverse>();
	file.enfile({ &racine, 0.0 });

	while (!file.est_vide()) {
		auto const noeud = file.defile().noeud;

		if (noeud->est_feuille()) {
			for (auto i = 0; i < noeud->nombre_references; ++i) {
				auto id_prim = arbre.index_refs[noeud->decalage_reference + i];
				auto intersection = delegue.intersecte_element(id_prim, rayon);

				if (!intersection.touche) {
					continue;
				}

				if (intersection.distance < t_proche) {
					t_proche = intersection.distance;
					esect = intersection;
				}
			}
		}
		else {
			for (auto e = 0; e < 2; ++e) {
				auto const &enfant = arbre.noeuds[noeud->enfants[e]];

				auto dist_gauche = fast_ray_nearest_hit(&enfant, index, rayon, idot_axis, dist_max);

				if (dist_gauche < dist_max) {
					file.enfile({ &enfant, dist_gauche });
				}
			}
		}
	}

	return esect;
}

/* ************************************************************************** */

struct PaireDistanceNoeud {
	ArbreHBE::Noeud *noeud = nullptr;
	double distance = 0.0;
};

inline bool operator<(PaireDistanceNoeud const &p1, PaireDistanceNoeud const &p2)
{
	return p1.distance < p2.distance;
}

struct DonneesPointPlusProche {
	/* Distance entre l'origine de la recherche et le point. */
	double distance_carree = std::numeric_limits<double>::max();

	/* Index de la primitive contenant le point. */
	long index = -1;

	/* Le point le plus proche. */
	dls::math::point3d point{};
};

struct DonneesRecherchePoint {
	DonneesPointPlusProche dn_plus_proche{};
	dls::math::point3d point{};
};

double calcul_point_plus_proche(
		ArbreHBE::Noeud const &noeud,
		dls::math::point3d const &point,
		dls::math::point3d &plus_proche);

template <typename TypeDelegue>
auto cherche_point_plus_proche_ex(
		ArbreHBE &arbre,
		TypeDelegue const &delegue,
		DonneesRecherchePoint &donnees,
		dls::file_priorite<PaireDistanceNoeud> &file,
		ArbreHBE::Noeud const &noeud)
{
	if (noeud.est_feuille()) {
		for (auto i = 0; i < noeud.nombre_references; ++i) {
			auto id_prim = arbre.index_refs[noeud.decalage_reference + i];
			auto dn_plus_proche = delegue.calcule_point_plus_proche(id_prim, donnees.point);

			if (dn_plus_proche.distance_carree < donnees.dn_plus_proche.distance_carree) {
				donnees.dn_plus_proche = dn_plus_proche;
			}
		}
	}
	else {
		dls::math::point3d plus_proche;

		for (auto e = 0; e < 2; ++e) {
			auto enfant = &arbre.noeuds[noeud.enfants[e]];

			auto dist = calcul_point_plus_proche(*enfant, donnees.point, plus_proche);

			if (dist < donnees.dn_plus_proche.distance_carree) {
				file.enfile({ enfant, dist });
			}
		}
	}
}

template <typename TypeDelegue>
auto cherche_point_plus_proche(
		ArbreHBE &arbre,
		TypeDelegue const &delegue,
		const dls::math::point3d &point,
		const double distance_max)
{
	if (arbre.nombre_noeud < 2) {
		return DonneesPointPlusProche{0.0, -1l};
	}

	auto plus_proche = dls::math::point3d();

	auto donnees = DonneesRecherchePoint{};
	donnees.point = point;
	donnees.dn_plus_proche.distance_carree = distance_max;

	auto const &racine = arbre.noeuds[1];

	auto dist_sq = calcul_point_plus_proche(racine, donnees.point, plus_proche);

	if (dist_sq >= distance_max) {
		return donnees.dn_plus_proche;
	}

	auto file = dls::file_priorite<PaireDistanceNoeud>();

	cherche_point_plus_proche_ex(arbre, delegue, donnees, file, racine);

	while (!file.est_vide() && file.haut().distance < donnees.dn_plus_proche.distance_carree) {
		auto node = file.defile().noeud;
		cherche_point_plus_proche_ex(arbre, delegue, donnees, file, *node);
	}

	return donnees.dn_plus_proche;
}

namespace bli {

typedef unsigned char axis_t;

struct BVHNode {
	float *bv{};
	BVHNode **children{};
	struct BVHNode *parent{};
	int index{};
	char totnode{};
	char main_axis{};
};

struct BVHTree {
	float epsilon{};
	int tree_type{};
	axis_t axis{};
	axis_t start_axis{};
	axis_t stop_axis{};
	int totleaf{};
	int totbranch{};

	dls::tableau<BVHNode *> nodes{};
	dls::tableau<float> nodebv{};
	dls::tableau<BVHNode *> nodechild{};
	dls::tableau<BVHNode> nodearray{};
};

BVHTree *bvhtree_new(int nombre_elements, float epsilon, char tree_type, char axis);

void insere(BVHTree *tree, int index, dls::math::vec3f const *co, int numpoints);

void balance(BVHTree *tree);

template <typename TypeDelegue>
auto cree_arbre_bvh(TypeDelegue const &delegue)
{
	auto const epsilon = 0.0f;
	auto const tree_type = 4;
	auto const axis = 6;

	auto nombre_element = static_cast<int>(delegue.nombre_elements());

	auto arbre_hbe = bvhtree_new(nombre_element, epsilon, tree_type, axis);

	for (auto i = 0; i < nombre_element; ++i) {
		auto cos = dls::tableau<dls::math::vec3f>();
		delegue.coords_element(i, cos);

		insere(arbre_hbe, i, cos.donnees(), static_cast<int>(cos.taille()));
	}

	balance(arbre_hbe);

	return arbre_hbe;
}

template <typename T>
auto fast_ray_nearest_hit(
		BVHNode const *node,
		int *index,
		dls::phys::rayon<T> const &ray,
		dls::math::vec3<T> const &idot_axis,
		T dist)
{
	auto t1x = (static_cast<T>(node->bv[index[0]]) - ray.origine[0]) * idot_axis[0];
	auto t2x = (static_cast<T>(node->bv[index[1]]) - ray.origine[0]) * idot_axis[0];
	auto t1y = (static_cast<T>(node->bv[index[2]] )- ray.origine[1]) * idot_axis[1];
	auto t2y = (static_cast<T>(node->bv[index[3]]) - ray.origine[1]) * idot_axis[1];
	auto t1z = (static_cast<T>(node->bv[index[4]]) - ray.origine[2]) * idot_axis[2];
	auto t2z = (static_cast<T>(node->bv[index[5]]) - ray.origine[2]) * idot_axis[2];

	if ((t1x > t2y || t2x < t1y || t1x > t2z || t2x < t1z || t1y > t2z || t2y < t1z) ||
			(t2x < 0.0 || t2y < 0.0 || t2z < 0.0) ||
			(t1x > dist || t1y > dist || t1z > dist)) {
		return constantes<T>::INFINITE;
	}

	return std::max(t1x, std::max(t1y, t1z));
}

struct BVHElement {
	BVHNode const *noeud = nullptr;
	double t = 0.0;
};

inline bool operator<(BVHElement const &p1, BVHElement const &p2)
{
	return p1.t < p2.t;
}

template <typename TypeDelegue>
auto traverse(BVHTree *tree, TypeDelegue const &delegue, dls::phys::rayond const &rayon)
{
	/* précalcule quelque données */
	auto ray_dot_axis = dls::math::vec3d();
	auto idot_axis = dls::math::vec3d();
	int index[6];

	static const dls::math::vec3d kdop_axes[3] = {
		dls::math::vec3d(1.0, 0.0, 0.0),
		dls::math::vec3d(0.0, 1.0, 0.0),
		dls::math::vec3d(0.0, 0.0, 1.0)
	};

	for (auto i = 0ul; i < 3; i++) {
		ray_dot_axis[i] = produit_scalaire(rayon.direction, kdop_axes[i]);
		idot_axis[i] = 1.0 / ray_dot_axis[i];

		if (std::abs(ray_dot_axis[i]) < 1e-6) {
			ray_dot_axis[i] = 0.0;
		}

		index[2 * i] = idot_axis[i] < 0.0 ? 1 : 0;
		index[2 * i + 1] = 1 - index[2 * i];
		index[2 * i] += 2 * static_cast<int>(i);
		index[2 * i + 1] += 2 * static_cast<int>(i);
	}

	auto const &racine = tree->nodes[tree->totleaf];

	auto dist_max = constantes<double>::INFINITE / 2.0;

	//auto distance_courant = racine.test_intersection_rapide(rayon);
	auto distance_courant = fast_ray_nearest_hit(racine, index, rayon, idot_axis, dist_max);
	auto esect = dls::phys::esectd{};

	if (distance_courant > dist_max) {
		return esect;
	}

	auto t_proche = rayon.distance_max;

	auto file = dls::file_priorite<BVHElement>();
	file.enfile({ racine, 0.0 });

	while (!file.est_vide()) {
		auto const noeud = file.defile().noeud;

		if (noeud->totnode == 0) {
			auto intersection = delegue.intersecte_element(noeud->index, rayon);

			if (!intersection.touche) {
				continue;
			}

			if (intersection.distance < t_proche) {
				t_proche = intersection.distance;
				esect = intersection;
			}
		}
		else {
			if (ray_dot_axis[static_cast<size_t>(noeud->main_axis)] > 0.0) {
				for (auto i = 0; i < noeud->totnode; ++i) {
					auto enfant = noeud->children[i];

					distance_courant = fast_ray_nearest_hit(enfant, index, rayon, idot_axis, dist_max);

					if (distance_courant < dist_max) {
						file.enfile({ enfant, distance_courant });
					}
				}
			}
			else {
				for (auto i = noeud->totnode - 1; i >= 0; --i) {
					auto enfant = noeud->children[i];

					distance_courant = fast_ray_nearest_hit(enfant, index, rayon, idot_axis, dist_max);

					if (distance_courant < dist_max) {
						file.enfile({ enfant, distance_courant });
					}
				}
			}
		}
	}

	return esect;
}

}
