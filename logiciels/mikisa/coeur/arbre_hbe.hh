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

struct ArbreHBE {
	struct Noeud {
		Noeud() = default;

		BoiteEnglobante limites{};

		long gauche = 0;
		long droite = 0;
		long nombre_references = 0;
		long decalage_reference = 0;
		long id_noeud = 0;
		Axe axe_princ{};

		bool est_feuille() const
		{
			return gauche == 0 && droite == 0;
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
	arbre.pousse(ArbreHBE::Noeud());
	++compte_noeud;

	/* crée une racine avec une boîte englobant toutes les autres */
	arbre.pousse(ArbreHBE::Noeud());
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
		references_racine.pousse(boite.id);
	}

	couche_courante.pousse(racine);
	refs_courantes.pousse(references_racine);

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
				noeud->gauche = 0;
				noeud->droite = 0;
				noeud->nombre_references = compte_ref;
				noeud->decalage_reference = liste_ref_finale.taille();

				for (auto j = 0; j < noeud->nombre_references; ++j) {
					liste_ref_finale.pousse(refs_courantes[i][j]);
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
					noeud->gauche = 0;
					noeud->droite = 0;
					noeud->nombre_references = compte_ref;
					noeud->decalage_reference = liste_ref_finale.taille();

					for (auto j = 0; j < noeud->nombre_references; ++j) {
						liste_ref_finale.pousse(refs_courantes[i][j]);
					}
				}
				else {
					/* crée les noeuds gauche et droite */
					arbre.pousse(ArbreHBE::Noeud());
					++compte_noeud;

					auto id_gauche = arbre.taille() - 1;
					auto gauche = &arbre[id_gauche];
					gauche->id_noeud = id_gauche;
					gauche->nombre_references = compte_gauche;

					arbre.pousse(ArbreHBE::Noeud());
					++compte_noeud;

					auto id_droite = arbre.taille() - 1;
					auto droite = &arbre[id_droite];
					droite->id_noeud = id_droite;
					droite->nombre_references = compte_droite;

					noeud->gauche = id_gauche;
					noeud->droite = id_droite;
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
							refs_gauche.pousse(index_ref);
							gauche->limites.etend(ref);
						}
						else {
							refs_droite.pousse(index_ref);
							droite->limites.etend(ref);
						}
					}

					/* ajoute les enfants à la couche suivante */
					couche_suivante.pousse(gauche);
					couche_suivante.pousse(droite);

					couche_refs_suivante.pousse(refs_gauche);
					couche_refs_suivante.pousse(refs_droite);
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

	return arbre_hbe;
}

struct AccumulatriceTraverse {
private:
	dls::phys::esectd m_intersection{};
	long m_id_noeud{};
	long m_nombre_touche{};
	dls::math::point3d m_origine{};

public:
	AccumulatriceTraverse(dls::math::point3d const &origine)
		: m_nombre_touche(0)
		, m_origine(origine)
	{}

	void enregistre_intersection(dls::phys::esectd const &intersect, long id_noeud)
	{
		if (m_intersection.touche == false && intersect.touche==true) {
			m_intersection = intersect;
			m_id_noeud = id_noeud;
			m_nombre_touche++;
		}
		else if (intersect.touche == true) {
			auto currentDistance = longueur(m_intersection.point - m_origine);
			auto newDistance = longueur(intersect.point - m_origine);

			if (newDistance < currentDistance) {
				m_intersection = intersect;
				m_id_noeud = id_noeud;
			}

			m_nombre_touche++;
		}
	}

	dls::phys::esectd const &intersection() const
	{
		return m_intersection;
	}

	long nombre_touche() const
	{
		return m_nombre_touche;
	}
};

template <typename TypeDelegue>
void traverse_impl(
		ArbreHBE const &arbre,
		TypeDelegue const &delegue,
		dls::phys::rayond const r,
		AccumulatriceTraverse &resultat,
		ArbreHBE::Noeud const &noeud,
		dls::math::vec3d const &ray_dot_axis)
{
	auto distance = noeud.test_intersection_rapide(r);

	if (distance < -0.5) {
		return;
	}

	if (distance >= r.distance_max) {
		return;
	}

	if (noeud.est_feuille()) {
		for (auto i = 0; i < noeud.nombre_references; ++i) {
			auto id_prim = arbre.index_refs[noeud.decalage_reference + i];
			auto intersection = delegue.intersecte_element(id_prim, r);

			if (!intersection.touche) {
				continue;
			}

			auto n = normalise(intersection.point - r.origine);
			auto degree = std::acos(produit_scalaire(n, r.direction));

			if (degree < (constantes<double>::PI / 2.0) || degree != degree) {
				resultat.enregistre_intersection(intersection, noeud.id_noeud);
			}
		}
	}
	else {
		auto const &gauche = arbre.noeuds[noeud.gauche];
		auto const &droite = arbre.noeuds[noeud.droite];

		/* pick loop direction to dive into the tree (based on ray direction and split axis) */
		if (ray_dot_axis[static_cast<size_t>(noeud.axe_princ)] > 0.0) {
			traverse_impl(arbre, delegue, r, resultat, gauche, ray_dot_axis);
			traverse_impl(arbre, delegue, r, resultat, droite, ray_dot_axis);
		}
		else {
			traverse_impl(arbre, delegue, r, resultat, droite, ray_dot_axis);
			traverse_impl(arbre, delegue, r, resultat, gauche, ray_dot_axis);
		}
	}
}

template <typename TypeDelegue>
void traverse(
		ArbreHBE const &arbre,
		TypeDelegue const &delegue,
		dls::phys::rayond const rayon,
		AccumulatriceTraverse &resultat)
{
	auto r = rayon;
	//r.direction_inverse = 1.0 / r.direction;
	//precalc_rayon_impermeable(r);
#if 0
	auto const &racine = arbre.noeuds[1];

	auto ray_dot_axis = dls::math::vec3d{};
	ray_dot_axis[0] = produit_scalaire(dls::math::vec3d(1.0, 0.0, 0.0), r.direction);
	ray_dot_axis[1] = produit_scalaire(dls::math::vec3d(0.0, 1.0, 0.0), r.direction);
	ray_dot_axis[2] = produit_scalaire(dls::math::vec3d(0.0, 0.0, 1.0), r.direction);

	traverse_impl(arbre, delegue, r, resultat, racine, ray_dot_axis);
#else
	if (arbre.nombre_noeud < 2) {
		return;
	}

	/* À FAIRE : petite pile. */
	auto pile = dls::pile<ArbreHBE::Noeud const *>();
	pile.empile(&arbre.noeuds[1]);

	auto courant = &arbre.noeuds[1];

	auto distance_courant = courant->test_intersection_rapide(r);

	if (distance_courant < -0.5) {
		return;
	}

	distance_courant = 10000000000000.0;

	while (!pile.est_vide()) {
		/* traverse et empile jusqu'à l'obtention d'une feuille */
		auto est_vide = false;

		while (!courant->est_feuille() && !est_vide) {
			auto gauche = &arbre.noeuds[courant->gauche];
			auto droite = &arbre.noeuds[courant->droite];

			/* trouve l'enfant le plus proche et le plus éloigné */
			auto distance_gauche = gauche->test_intersection_rapide(r);
			auto distance_droite = droite->test_intersection_rapide(r);

			/* si le rayon intersecte les deux enfants, empile le noeud courant */
			if (distance_gauche > -0.5 && distance_droite > -0.5) {
				pile.empile(courant);

				if (distance_gauche < distance_droite) {
					courant = gauche;
				}
				else {
					courant = droite;
				}
			}
			else if (distance_gauche > -0.5 && distance_droite < -0.5) {
				courant = gauche;
			}
			else if (distance_gauche < -0.5 && distance_droite > -0.5) {
				courant = droite;
			}
			else {
				est_vide = true;
			}
		}

		if (courant->est_feuille()) {
			for (auto i = 0; i < courant->nombre_references; ++i) {
				auto id_prim = arbre.index_refs[courant->decalage_reference + i];
				auto intersection = delegue.intersecte_element(id_prim, r);

				if (!intersection.touche) {
					continue;
				}

				auto n = normalise(intersection.point - r.origine);
				auto degree = std::acos(produit_scalaire(n, r.direction));

				if (degree < (constantes<double>::PI / 2.0) || degree != degree) {
					resultat.enregistre_intersection(intersection, courant->id_noeud);
				}
			}
		}

		if (pile.est_vide()) {
			continue;
		}

		courant = pile.haut();
		pile.depile();

		auto gauche = &arbre.noeuds[courant->gauche];
		auto droite = &arbre.noeuds[courant->droite];

		auto distance_gauche = gauche->test_intersection_rapide(r);
		auto distance_droite = droite->test_intersection_rapide(r);

		if (distance_gauche >= distance_droite) {
			courant = gauche;
		}
		else {
			courant = droite;
		}
	}
#endif
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

		auto gauche = &arbre.noeuds[noeud.gauche];
		auto droite = &arbre.noeuds[noeud.droite];

		auto dist = calcul_point_plus_proche(*gauche, donnees.point, plus_proche);

		if (dist < donnees.dn_plus_proche.distance_carree) {
			file.enfile({ gauche, dist });
		}

		dist = calcul_point_plus_proche(*droite, donnees.point, plus_proche);

		if (dist < donnees.dn_plus_proche.distance_carree) {
			file.enfile({ droite, dist });
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
		auto node = file.haut().noeud;
		file.defile();
		cherche_point_plus_proche_ex(arbre, delegue, donnees, file, *node);
	}

	return donnees.dn_plus_proche;
}
