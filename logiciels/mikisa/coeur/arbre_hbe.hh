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
				noeud->enfants[NOEUD_GAUCHE] = 0;
				noeud->enfants[NOEUD_DROITE] = 0;
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
					noeud->enfants[NOEUD_GAUCHE] = 0;
					noeud->enfants[NOEUD_DROITE] = 0;
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
	auto const &racine = arbre.noeuds[1];

	auto distance_courant = racine.test_intersection_rapide(rayon);
	auto esect = dls::phys::esectd{};

	if (distance_courant < -0.5) {
		return esect;
	}

	auto t_proche = rayon.distance_max;

	auto file = dls::file_priorite<ElementTraverse>();
	file.enfile({ &racine, 0.0 });

	while (!file.est_vide()) {
		auto const noeud = file.haut().noeud;
		file.defile();

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

				auto dist_gauche = enfant.test_intersection_rapide(rayon);

				if (dist_gauche > -0.5) {
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
		auto node = file.haut().noeud;
		file.defile();
		cherche_point_plus_proche_ex(arbre, delegue, donnees, file, *node);
	}

	return donnees.dn_plus_proche;
}
