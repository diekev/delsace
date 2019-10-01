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

#include "operatrices_maillage.hh"

#include <eigen3/Eigen/Eigenvalues>

#include "biblinternes/bruit/evaluation.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/file.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "corps/iteration_corps.hh"
#include "corps/limites_corps.hh"
#include "corps/polyedre.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "courbure.hh"
#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static auto cherche_index_voisins(Corps const &corps)
{
	auto points_entree = corps.points_pour_lecture();

	dls::tableau<dls::ensemble<long>> voisins(points_entree->taille());

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		for (auto j = 0; j < poly->nombre_sommets() - 1; ++j) {
			auto i0 = poly->index_point(j);
			auto i1 = poly->index_point(j + 1);

			voisins[i0].insere(i1);
			voisins[i1].insere(i0);
		}

		auto dernier = poly->index_point(poly->nombre_sommets() - 1);
		auto premier = poly->index_point(0);

		voisins[premier].insere(dernier);
		voisins[dernier].insere(premier);
	});

	return voisins;
}

static auto cherche_index_adjacents(Corps const &corps)
{
	auto points_entree = corps.points_pour_lecture();

	dls::tableau<dls::ensemble<long>> adjacents(points_entree->taille());

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *polygone)
	{
		INUTILISE(corps_entree);

		for (auto j = 0; j < polygone->nombre_sommets(); ++j) {
			auto i0 = polygone->index_point(j);

			adjacents[i0].insere(polygone->index);
		}
	});

	return adjacents;
}

static auto cherche_index_bordures(
		dls::tableau<dls::ensemble<long>> const &voisins,
		dls::tableau<dls::ensemble<long>> const &adjacents)
{
	dls::tableau<char> bordures(voisins.taille());

	for (auto i = 0; i < voisins.taille(); ++i) {
		bordures[i] = voisins[i].taille() != adjacents[i].taille();
	}

	return bordures;
}

/* ************************************************************************** */

static auto calcule_lissage_normal(
		ListePoints3D const *points_entree,
		dls::tableau<char> const &bordures,
		dls::tableau<dls::ensemble<long>> const &voisins,
		dls::tableau<dls::math::vec3f> &deplacement,
		bool preserve_bordures)
{
	boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); i++) {
			if (bordures[i]) {
				if (preserve_bordures) {
					deplacement[i] = dls::math::vec3f(0.0f);
				}
				else {
					auto nn = voisins[i].taille();
					if (!nn) {
						continue;
					}

					int nnused = 0;

					for (auto j : voisins[i]) {
						if (bordures[j]) {
							continue;
						}

						deplacement[i] += points_entree->point(j);
						nnused++;
					}

					deplacement[i] /= static_cast<float>(nnused);
					deplacement[i] -= points_entree->point(i);
				}
			}
			else {
				auto nn = voisins[i].taille();

				if (!nn) {
					continue;
				}

				for (auto j : voisins[i]) {
					deplacement[i] += points_entree->point(j);
				}

				deplacement[i] /= static_cast<float>(nn);
				deplacement[i] -= points_entree->point(i);
			}
		}
	});
}

static auto calcule_lissage_pondere(
		ListePoints3D const *points_entree,
		dls::tableau<char> const &bordures,
		dls::tableau<dls::ensemble<long>> const &voisins,
		dls::tableau<dls::math::vec3f> &deplacement,
		bool preserve_bordures)
{
	boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); i++) {
			if (bordures[i]) {
				if (preserve_bordures) {
					deplacement[i] = dls::math::vec3f(0.0f);
				}
				else {
					auto nn = voisins[i].taille();
					if (!nn) {
						continue;
					}

					auto p = dls::math::vec3f(0.0f);
					auto const &xi = points_entree->point(i);
					auto poids = 1.0f;

					for (auto j : voisins[i]) {
						if (bordures[j]) {
							continue;
						}

						auto xj = points_entree->point(j);
						auto ai = 1.0f / longueur(xi - xj);

						p += ai * xj;
						poids += ai;
					}

					p /= poids;
					deplacement[i] = p - xi;
				}
			}
			else {
				auto nn = voisins[i].taille();

				if (!nn) {
					continue;
				}

				auto p = dls::math::vec3f(0.0f);
				auto const &xi = points_entree->point(i);
				auto poids = 1.0f;

				for (auto j : voisins[i]) {
					auto xj = points_entree->point(j);
					auto ai = 1.0f / longueur(xi - xj);

					p += ai * xj;
					poids += ai;
				}

				p /= poids;
				deplacement[i] = p - xi;
			}
		}
	});
}

static auto applique_lissage(
		ListePoints3D *points_entree,
		Attribut *attr_N,
		dls::tableau<dls::math::vec3f> const &deplacement,
		float poids_lissage,
		bool tangeante)
{
	boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); i++) {
			auto p = points_entree->point(i);

			if (tangeante) {
				auto n = dls::math::vec3f();
				extrait(attr_N->r32(i), n);
				auto const &d = deplacement[i];
				p += poids_lissage * (d - n * produit_scalaire(d, n));
			}
			else {
				p += poids_lissage * deplacement[i];
			}

			points_entree->point(i, p);
		}
	});
}

class OperatriceLissageLaplacien final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Lissage Laplacien";
	static constexpr auto AIDE = "Performe un lissage laplacien des points du corps d'entrée.";

	OperatriceLissageLaplacien(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_lissage_laplacien.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points_entree = m_corps.points_pour_ecriture();

		auto const voisins = cherche_index_voisins(m_corps);
		auto const adjacents = cherche_index_adjacents(m_corps);
		auto const bordures = cherche_index_bordures(voisins, adjacents);

		auto const pondere_distance = evalue_bool("pondère_distance");
		auto const iterations = evalue_entier("itérations");
		auto const poids_lissage = evalue_decimal("poids");
		auto const preserve_bordures = evalue_bool("préserve_bordures");
		auto const tangeante = evalue_bool("tangeante");

		auto attr_N = m_corps.attribut("N");

		if (tangeante) {
			if (attr_N == nullptr || attr_N->portee != portee_attr::POINT) {
				calcul_normaux(m_corps, false, false);
				attr_N = m_corps.attribut("N");
			}
		}

		dls::tableau<dls::math::vec3f> deplacement(points_entree->taille());

		/* IDÉES :
		 * - application de l'algorithme sur les normaux des points.
		 * - deux passes : poids positif, puis poids négatif
		 */

		for (auto k = 0; k < iterations; ++k) {
			if (pondere_distance) {
				calcule_lissage_pondere(points_entree, bordures, voisins, deplacement, preserve_bordures);
			}
			else {
				calcule_lissage_normal(points_entree, bordures, voisins, deplacement, preserve_bordures);
			}

			applique_lissage(points_entree, attr_N, deplacement, poids_lissage, tangeante);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceTriangulation final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Triangulation";
	static constexpr auto AIDE = "Performe une triangulation des polygones du corps d'entrée.";

	OperatriceTriangulation(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, true, true)) {
			return res_exec::ECHOUEE;
		}

		/* copie les points et les attributs et groupes n'étant pas sur les
		 * primitives */
		*m_corps.points_pour_ecriture() = *corps_entree->points_pour_lecture();

		for (auto const &attr : corps_entree->attributs()) {
			if (attr.portee == portee_attr::PRIMITIVE || attr.portee == portee_attr::VERTEX) {
				continue;
			}

			auto nattr = m_corps.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee, true);
			*nattr = attr;
		}

		for (auto const &grp : corps_entree->groupes_points()) {
			auto ngrp = m_corps.ajoute_groupe_point(grp.nom);
			*ngrp = grp;
		}

		/* prépare transfère groupe primitive */
		auto paires_grps = dls::tableau<std::pair<GroupePrimitive const *, GroupePrimitive *>>();

		for (auto const &grp : corps_entree->groupes_prims()) {
			auto ngrp = m_corps.ajoute_groupe_primitive(grp.nom);

			paires_grps.pousse({ &grp, ngrp });
		}

		/* triangule */
		auto polyedre = construit_corps_polyedre_triangle(*corps_entree);
		auto transferante = TransferanteAttribut(*corps_entree, m_corps, TRANSFERE_ATTR_PRIMS | TRANSFERE_ATTR_SOMMETS);

		for (auto face : polyedre.faces) {
			auto poly = m_corps.ajoute_polygone(type_polygone::FERME, 3);

			auto debut = face->arete;
			auto fin = debut;

			do {
				auto idx_sommet = m_corps.ajoute_sommet(poly, debut->sommet->label);
				transferante.transfere_attributs_sommets(debut->label, idx_sommet);

				debut = debut->suivante;
			} while (debut != fin);

			transferante.transfere_attributs_prims(face->label, poly->index);

			for (auto paire : paires_grps) {
				if (!paire.first->contient(face->label)) {
					continue;
				}

				paire.second->ajoute_index(poly->index);
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

static auto centre_masse_maillage(Corps const &corps)
{
	auto points = corps.points_pour_lecture();
	auto centre_masse = dls::math::vec3f(0.0f);
	auto masse_totale = 0.0f;

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto aire_poly = 0.0f;

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto v0 = points->point(poly->index_point(0));
			auto v1 = points->point(poly->index_point(j - 1));
			auto v2 = points->point(poly->index_point(j));

			auto centre_tri = (v0 + v1 + v2) / 3.0f;
			auto aire_tri = calcule_aire(v0, v1, v2);
			centre_masse += aire_tri * centre_tri;
			masse_totale += aire_tri;

			aire_poly += aire_tri;
		}
	});

	if (masse_totale != 0.0f) {
		centre_masse /= masse_totale;
	}

	return centre_masse;
}

static auto covariance_maillage(Corps const &corps)
{
	auto points = corps.points_pour_lecture();

	auto MC = dls::math::mat3x3f(0.0f);
	auto aire_totale = 0.0f;

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto aire_poly = 0.0f;

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto v0 = points->point(poly->index_point(0));
			auto v1 = points->point(poly->index_point(j - 1));
			auto v2 = points->point(poly->index_point(j));

			auto aire_tri = calcule_aire(v0, v1, v2);
			aire_totale += aire_tri;

			aire_poly += aire_tri;
		}

		auto centroid = dls::math::vec3f(0.0f);

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			centroid += points->point(poly->index_point(j));
		}

		centroid /= static_cast<float>(poly->nombre_sommets());

		/* covariance avec le centroide */
		auto const poids_point = aire_poly / (static_cast<float>(poly->nombre_sommets()) * 3.0f);
		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			auto pc = points->point(poly->index_point(j)) - centroid;

			for (auto ii = 0ul; ii < 3; ++ii) {
				for (auto jj = 0ul; jj < 3; ++jj) {
					MC[ii][jj] += poids_point * pc[ii] * pc[jj];
				}
			}
		}

		/* covariance du centroide */
		for (auto ii = 0ul; ii < 3; ++ii) {
			for (auto jj = 0ul; jj < 3; ++jj) {
				MC[ii][jj] += aire_poly * centroid[ii] * centroid[jj];
			}
		}
	});

	for (auto ii = 0ul; ii < 3; ++ii) {
		for (auto jj = 0ul; jj < 3; ++jj) {
			MC[ii][jj] /= aire_totale;
		}
	}

	MC[1][0] = MC[0][1];
	MC[2][0] = MC[0][2];
	MC[2][1] = MC[1][2];

	return MC;
}

class OperatriceNormaliseCovariance final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Normalise Covariance";
	static constexpr auto AIDE = "Modifie les points pour que la covariance moyenne soit égale à 1.";

	OperatriceNormaliseCovariance(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_normalise_covariance.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points = m_corps.points_pour_ecriture();

		auto poids = evalue_decimal("poids");

		auto centre_masse = centre_masse_maillage(m_corps);
		auto MC = covariance_maillage(m_corps);

		auto echelle = poids / std::sqrt(MC[0][0] + MC[1][1] + MC[2][2]);

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			/* peut-être une matrice de transformation */
			point -= centre_masse;
			point *= echelle;
			point += centre_masse;
			points->point(i, point);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceAligneCovariance final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Aligne Covariance";
	static constexpr auto AIDE = "Tourne l'objet de sorte que l'axe locale de "
								 "covariance le plus grand soit aligné avec "
								 "l'axe des X, et le second avec l'axe des Y.";

	OperatriceAligneCovariance(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points = m_corps.points_pour_ecriture();

		/* IDÉES :
		 * - définition des axes d'alignement par l'utilisateur, avec des poids
		 *   pour interpoler la rotation
		 * - définition du calcul de la covariance en espace modial
		 */

		auto centre_masse = centre_masse_maillage(m_corps);

		/* centre le maillage */
		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			point -= centre_masse;
			points->point(i, point);
		}

		auto C = covariance_maillage(m_corps);

		auto A = Eigen::Matrix<float, 3, 3>();
		A(0, 0) = C[0][0];
		A(0, 1) = C[0][1];
		A(0, 2) = C[0][2];
		A(1, 0) = C[1][0];
		A(1, 1) = C[1][1];
		A(1, 2) = C[1][2];
		A(2, 0) = C[2][0];
		A(2, 1) = C[2][1];
		A(2, 2) = C[2][2];

		auto s = Eigen::SelfAdjointEigenSolver<Eigen::Matrix<float, 3, 3>>(A);

		auto vpx_e = s.eigenvectors().col(0);
		auto vpy_e = s.eigenvectors().col(1);

		auto premier = dls::math::vec3f(vpx_e[0], vpx_e[1], vpx_e[2]);
		auto second = dls::math::vec3f(vpy_e[0], vpy_e[1], vpy_e[2]);

		auto npos = 0;
		auto nombre_points = points->taille();

		for (auto i = 0; i < nombre_points; i++) {
			if (produit_scalaire(points->point(i), premier) > 0.0f) {
				npos++;
			}
		}

		if (npos < nombre_points / 2) {
			premier = -premier;
		}

		npos = 0;
		for (int i = 0; i < nombre_points; i++) {
			if (produit_scalaire(points->point(i), second) > 0.0f) {
				npos++;
			}
		}

		if (npos < nombre_points / 2) {
			second = -second;
		}

		auto troisieme = produit_croix(premier, second);

		auto mat = dls::math::mat3x3f(
					premier.x, premier.y, premier.z,
					second.x, second.y, second.z,
					troisieme.x, troisieme.y, troisieme.z);

		mat = inverse(mat);

		/* applique la matrice et repositione le maillage */
		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			point = mat * point;
			point -= centre_masse;
			points->point(i, point);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceBruitTopologique final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Bruit Topologique";
	static constexpr auto AIDE = "";

	OperatriceBruitTopologique(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_bruit_topologique.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points = m_corps.points_pour_ecriture();

		auto attr_N = m_corps.attribut("N");

		if (attr_N == nullptr || attr_N->portee != portee_attr::POINT) {
			calcul_normaux(m_corps, false, false);
			attr_N = m_corps.attribut("N");
		}

		auto graine = evalue_entier("graine");
		auto poids = evalue_decimal("poids");
		auto poids_normaux = evalue_decimal("poids_normaux");
		auto poids_tangeantes = evalue_decimal("poids_tangeantes");

		auto index_voisins = cherche_index_voisins(m_corps);

		auto gna = GNA(graine);

		auto deplacement = dls::tableau<dls::math::vec3f>(
					points->taille(),
					dls::math::vec3f(0.0f));

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);

			/* tangeante */
			auto const &voisins = index_voisins[i];

			for (auto voisin : voisins) {
				auto const pv = points->point(voisin);
				auto echelle = poids_tangeantes / (poids_tangeantes + longueur(pv - point));
				deplacement[i] += gna.uniforme(0.0f, echelle) * (pv - point);
			}

			if (voisins.taille() != 0) {
				deplacement[i] /= static_cast<float>(voisins.taille());
			}

			/* normal */
			auto n = dls::math::vec3f();
			extrait(attr_N->r32(i), n);
			deplacement[i] += gna.uniforme(0.0f, poids_normaux) * n;
		}

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			point += poids * deplacement[i];
			points->point(i, point);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceErosionMaillage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Érosion Maillage";
	static constexpr auto AIDE = "";

	OperatriceErosionMaillage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_erosion_maillage.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto iterations = evalue_entier("itérations", contexte.temps_courant);
		auto inverse = evalue_bool("inverse");

		auto polyedre = converti_corps_polyedre(*corps_entree);

		for (auto i = 0; i < iterations; ++i) {
			/* marque les faces ayant une arête dont l'opposée est nulle comme
			 * ayant besoin d'être éliminées */
			for (auto f : polyedre.faces) {
				auto debut = f->arete;
				auto fin = f->arete;

				do {
					auto paire = debut->paire;

					if (paire == nullptr || dls::outils::possede_drapeau(paire->drapeaux, mi_drapeau::SUPPRIME)) {
						f->drapeaux |= mi_drapeau::SUPPRIME;
						break;
					}

					debut = debut->suivante;
				} while (debut != fin);
			}

			/* marque les arêtes des faces supprimées comme ayant besoin d'être
			 * éliminiées */
			for (auto f : polyedre.faces) {
				if (!dls::outils::possede_drapeau(f->drapeaux, mi_drapeau::SUPPRIME)) {
					continue;
				}

				auto debut = f->arete;
				auto fin = f->arete;

				do {
					debut->drapeaux |= mi_drapeau::SUPPRIME;
					debut = debut->suivante;
				} while (debut != fin);
			}
		}

		/* marque les points comme ayant besoin d'être éliminées */
		for (auto s : polyedre.sommets) {
			s->drapeaux |= mi_drapeau::SUPPRIME;
		}

		if (inverse) {
			for (auto f : polyedre.faces) {
				if (dls::outils::possede_drapeau(f->drapeaux, mi_drapeau::SUPPRIME)) {
					f->drapeaux &= ~mi_drapeau::SUPPRIME;
				}
				else {
					f->drapeaux |= mi_drapeau::SUPPRIME;
				}
			}
		}

		for (auto f : polyedre.faces) {
			if (dls::outils::possede_drapeau(f->drapeaux, mi_drapeau::SUPPRIME)) {
				continue;
			}

			auto debut = f->arete;
			auto fin = f->arete;

			do {
				debut->sommet->drapeaux &= ~mi_drapeau::SUPPRIME;
				debut = debut->suivante;
			} while (debut != fin);
		}

		/* copie les polygones et points restants */

		auto transferante = TransferanteAttribut(*corps_entree, m_corps, TRANSFERE_ATTR_POINTS | TRANSFERE_ATTR_PRIMS | TRANSFERE_ATTR_SOMMETS);

		/* transfère tous les points */
		for (auto s : polyedre.sommets) {
			if (dls::outils::possede_drapeau(s->drapeaux, mi_drapeau::SUPPRIME)) {
				continue;
			}

			auto idx = m_corps.ajoute_point(s->p);
			s->index = idx;

			transferante.transfere_attributs_points(s->label, s->index);
		}

		/* transfère les polygones */
		for (auto f : polyedre.faces) {
			if (dls::outils::possede_drapeau(f->drapeaux, mi_drapeau::SUPPRIME)) {
				continue;
			}

			auto debut = f->arete;
			auto fin = f->arete;

			auto poly = m_corps.ajoute_polygone(type_polygone::FERME);

			do {
				auto idx_sommet = m_corps.ajoute_sommet(poly, debut->sommet->index);
				transferante.transfere_attributs_sommets(debut->label, idx_sommet);

				debut = debut->suivante;
			} while (debut != fin);

			transferante.transfere_attributs_prims(f->label, poly->index);
		}

		return res_exec::REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("itérations") == nullptr) {
			ajoute_propriete("itérations", danjo::TypePropriete::ENTIER, 1);
		}

		if (propriete("inverse") == nullptr) {
			ajoute_propriete("inverse", danjo::TypePropriete::BOOL, false);
		}
	}
};

/* ************************************************************************** */

static auto calcul_barycentre(ListePoints3D const *points)
{
	auto barycentre = dls::math::vec3f(0.0f);

	for (auto i = 0; i < points->taille(); ++i) {
		barycentre += points->point(i);
	}

	barycentre /= (static_cast<float>(points->taille()));

	return barycentre;
}

static auto couleur_min_max(float valeur, float valeur_min, float valeur_max)
{
	auto c = dls::math::vec3f(1.0f);

	if (valeur == 0.0f) {
		return c;
	}

	if (std::isnan(valeur)) {
		return dls::math::vec3f(1.0f);
	}

	if (valeur < valeur_min) {
		valeur = valeur_min;
	}

	if (valeur > valeur_max) {
		valeur = valeur_max;
	}

	auto dv = valeur_max - valeur_min;

	if (valeur < (valeur_min + 0.5f * dv)) {
		c.r = 2.0f * (valeur - valeur_min) / dv;
		c.g = 2.0f * (valeur - valeur_min) / dv;
		c.b = 1.0f;
	}
	else {
		c.b = 2.0f - 2.0f * (valeur - valeur_min) / dv;
		c.g = 2.0f - 2.0f * (valeur - valeur_min) / dv;
		c.r = 1.0f;
	}

	return c;
}

static auto calcul_donnees_aire(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto aires = corps.ajoute_attribut("aire", type_attribut::R32, 1, portee_attr::PRIMITIVE);

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto aire_poly = 0.0f;

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto v0 = points->point(poly->index_point(0));
			auto v1 = points->point(poly->index_point(j - 1));
			auto v2 = points->point(poly->index_point(j));

			auto aire_tri = calcule_aire(v0, v1, v2);

			aire_poly += aire_tri;
		}

		assigne(aires->r32(poly->index), aire_poly);
	});

	return aires;
}

static auto calcul_donnees_perimetres(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto perimetres = corps.ajoute_attribut("aire", type_attribut::R32, 1, portee_attr::PRIMITIVE);

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto peri_poly = 0.0f;
		auto k = poly->index_point(poly->nombre_sommets() - 1);

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			auto idx = poly->index_point(j);
			auto v0 = points->point(k);
			auto v1 = points->point(idx);

			peri_poly += longueur(v1 - v0);
			k = idx;
		}

		assigne(perimetres->r32(poly->index), peri_poly);
	});

	return perimetres;
}

static auto calcul_barycentre_poly(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto barycentres = corps.ajoute_attribut("barycentre", type_attribut::R32, 3, portee_attr::PRIMITIVE);

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto barycentre = dls::math::vec3f(0.0f);

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			barycentre += points->point(poly->index_point(j));
		}

		barycentre /= static_cast<float>(poly->nombre_sommets());

		assigne(barycentres->r32(poly->index), barycentre);
	});

	return barycentres;
}

/* Le calcul des centroides se fait en pondérant les sommets par les aires des
 * polygones les entourants. */
static auto calcul_centroide_poly(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto idx_voisins = cherche_index_adjacents(corps);
	auto aires_poly = calcul_donnees_aire(corps);

	auto aires_sommets = dls::tableau<float>(points->taille());

	for (auto i = 0; i < points->taille(); ++i) {
		auto aire = 0.0f;

		for (auto const &voisin : idx_voisins[i]) {
			aire += aires_poly->r32(voisin)[0];
		}

		aires_sommets[i] = aire;
	}

	auto centroides = corps.ajoute_attribut("centroide", type_attribut::R32, 3, portee_attr::PRIMITIVE);

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto centroide = dls::math::vec3f(0.0f);
		auto poids = 0.0f;

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			auto idx = poly->index_point(j);
			auto v1 = points->point(idx);

			centroide += v1 * aires_sommets[idx];
			poids += aires_sommets[idx];
		}

		if (poids != 0.0f) {
			centroide /= poids;
		}

		assigne(centroides->r32(poly->index), centroide);
	});

	return centroides;
}

static auto calcul_arrete_plus_longues(Corps &corps)
{
	auto points = corps.points_pour_lecture();
	auto longueur_max = 0.0f;

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		for (auto j = 1; j < poly->nombre_sommets(); ++j) {
			auto v0 = points->point(poly->index_point(j - 1));
			auto v1 = points->point(poly->index_point(j));

			auto l = longueur(v0 - v1);

			if (l > longueur_max) {
				longueur_max = l;
			}
		}
	});

	return longueur_max;
}

static auto calcul_tangeantes(Corps &corps)
{
	auto tangeantes = corps.ajoute_attribut("tangeantes", type_attribut::R32, 3, portee_attr::POINT);
	auto points = corps.points_pour_lecture();

	auto index_voisins = cherche_index_voisins(corps);

	for (auto i = 0; i < points->taille(); ++i) {
		auto const &voisins = index_voisins[i];

		auto tangeante = dls::math::vec3f(0.0f);
		auto p0 = points->point(i);
		auto poids_total = 0.0f;

		for (auto v : voisins) {
			auto p1 = points->point(v);
			auto dir = (p1 - p0);
			auto poids = longueur(dir);
			tangeante += poids * dir;
			poids_total += poids;
		}

		if (voisins.taille() != 0) {
			tangeante /= poids_total;
		}

		assigne(tangeantes->r32(i), tangeante);
	}

	return tangeantes;
}

static auto calcul_donnees_dist_point(Corps &corps, dls::math::vec3f const &centre)
{
	auto points = corps.points_pour_lecture();

	auto dist = corps.ajoute_attribut("distance", type_attribut::R32, 1, portee_attr::POINT);

	for (auto i = 0; i < points->taille(); ++i) {
		auto d = longueur(points->point(i) - centre);

		assigne(dist->r32(i), d);
	}

	return dist;
}

static auto calcul_donnees_dist_barycentre(Corps &corps)
{
	auto points = corps.points_pour_lecture();
	auto barycentre = calcul_barycentre(points);
	return calcul_donnees_dist_point(corps, barycentre);
}

static auto calcul_donnees_dist_centroide(Corps &corps)
{
	auto centre_masse = centre_masse_maillage(corps);
	return calcul_donnees_dist_point(corps, centre_masse);
}

static auto min_max_attribut(Attribut *attr, float &valeur_min, float &valeur_max)
{
	for (auto i = 0; i < attr->taille(); ++i) {
		auto v = attr->r32(i)[0];

		if (v < valeur_min) {
			valeur_min = v;
		}

		if (v > valeur_max) {
			valeur_max = v;
		}
	}
}

static auto restreint_attribut_max(Attribut *attr, float const valeur_max)
{
	if (attr->type() != type_attribut::R32) {
		return;
	}

	for (auto i = 0; i < attr->taille(); ++i) {
		auto v = attr->r32(i)[0];

		if (v > valeur_max) {
			assigne(attr->r32(i), valeur_max);
		}
	}
}

static auto calcul_valence(Corps &corps)
{
	auto polyedre = converti_corps_polyedre(corps);

	auto attr = corps.ajoute_attribut("valence", type_attribut::Z32, 1, portee_attr::POINT);

	for (auto sommet : polyedre.sommets) {
		auto valence = 0;

		auto debut = sommet->arete;
		auto fin = debut;

		do {
			++valence;
			debut = suivante_autour_point(debut);
		} while (debut != fin && debut != nullptr);

		attr->z32(sommet->label)[0] = valence;
	}

	return attr;
}

static auto calcul_angle_sommets(Corps &corps)
{
	auto attr = corps.ajoute_attribut("angle_sommet", type_attribut::R32, 1, portee_attr::VERTEX);

	pour_chaque_polygone_ferme(corps, [&](Corps &corps_entree, Polygone *polygone)
	{
		auto points = corps_entree.points_pour_lecture();
		auto nombre_sommets = polygone->nombre_sommets();

		auto i0 = nombre_sommets - 2;
		auto i1 = nombre_sommets - 1;
		auto i2 = 0;

		for (auto i = 0; i < polygone->nombre_sommets(); ++i) {
			auto idx_p0 = polygone->index_point(i0);
			auto idx_p1 = polygone->index_point(i1);
			auto idx_p2 = polygone->index_point(i2);

			auto p0 = points->point(idx_p0);
			auto p1 = points->point(idx_p1);
			auto p2 = points->point(idx_p2);

			auto e0 = p0 - p1;
			auto e1 = p2 - p1;

			auto angle = produit_scalaire(e0, e1);

			attr->r32(polygone->index_sommet(i1))[0] = angle;

			i0 = i1;
			i1 = i2;
			i2 = i + 1;
		}
	});

	return attr;
}

static auto calcul_angle_diedre(Corps &corps)
{
	auto polyedre = converti_corps_polyedre(corps);

	auto attr_N = corps.attribut("N");
	auto ancien_attr_N = static_cast<Attribut *>(nullptr);

	if (attr_N == nullptr) {
		calcul_normaux(corps, location_normal::PRIMITIVE, pesee_normal::AIRE, false);

		attr_N = corps.attribut("N");
	}
	else if (attr_N->portee != portee_attr::PRIMITIVE) {
		attr_N->nom("N_sauvegarde");
		ancien_attr_N = attr_N;

		calcul_normaux(corps, location_normal::PRIMITIVE, pesee_normal::AIRE, false);
		attr_N = corps.attribut("N");
	}

	auto attr = corps.ajoute_attribut("angle_dièdre", type_attribut::R32, 1, portee_attr::VERTEX);

	auto n0 = dls::math::vec3f();
	auto n1 = dls::math::vec3f();

	for (auto face : polyedre.faces) {
		auto debut = face->arete;
		auto fin = debut;

		do {
			if (debut->paire != nullptr && !dls::outils::possede_drapeau(debut->drapeaux, mi_drapeau::VALIDE)) {
				extrait(attr_N->r32(face->label), n0);
				extrait(attr_N->r32(debut->paire->face->label), n1);

				auto angle = produit_scalaire(n0, n1);

				attr->r32(debut->label)[0] = angle;
				attr->r32(debut->paire->label)[0] = angle;

				debut->drapeaux |= mi_drapeau::VALIDE;
				debut->paire->drapeaux |= mi_drapeau::VALIDE;
			}

			debut = debut->suivante;
		} while (debut != fin);
	}

	if (ancien_attr_N != nullptr) {
		corps.supprime_attribut("N");
		ancien_attr_N->nom("N");
	}

	return attr;
}

static auto calcul_longueur_aretes(Corps &corps)
{
	auto polyedre = converti_corps_polyedre(corps);

	auto attr = corps.ajoute_attribut("longueur_arête", type_attribut::R32, 1, portee_attr::VERTEX);

	for (auto face : polyedre.faces) {
		auto a0 = face->arete;
		auto a1 = a0->suivante;
		auto fin = a1;

		do {
			if (!dls::outils::possede_drapeau(a1->drapeaux, mi_drapeau::VALIDE)) {
				auto const &p0 = a0->sommet->p;
				auto const &p1 = a1->sommet->p;

				auto l = longueur(p0 - p1);

				attr->r32(a1->label)[0] = l;
				a1->drapeaux |= mi_drapeau::VALIDE;

				if (a1->paire != nullptr) {
					attr->r32(a1->paire->label)[0] = l;
					a1->paire->drapeaux |= mi_drapeau::VALIDE;
				}
			}

			a0 = a1;
			a1 = a0->suivante;
		} while (a1 != fin);
	}

	return attr;
}

/* +---------------+----------+---------+---------------------------------------+
 * | AIRE          | MAILLAGE | FAIT    | aire de chaque polygone               |
 * | PÉRIMÈTRE     | MAILLAGE | FAIT    | périmètre de chaque polygone          |
 * | VALENCE       | MAILLAGE | FAIT    | nombre de voisins pour chaque sommets |
 * | ANGLE         | MAILLAGE | FAIT    | angle de chaque vertex                |
 * | ANGLE DIEDRE  | MAILLAGE | FAIT    | angle dièdre de chaque arête          |
 * | LONGUEUR COTE | MAILLAGE | FAIT    | longueur de chaque coté               |
 * | CENTROIDE     | MAILLAGE | FAIT    | centre de masse de chaque polygone    |
 * | BARYCENTRE    | MAILLAGE | FAIT    | barycentre de chaque polygone         |
 * | COURBURE      | MAILLAGE | FAIT    | courbures du maillage                 |
 * | GAUSSIEN      | MAILLAGE | FAIT    | produit des courbures principales     |
 * | MOYENNE       | MAILLAGE | FAIT    | moyenne des courbures principales     |
 * | GRADIENT      | ATTRIBUT | À FAIRE |                                       |
 * | LAPLACIEN     | ATTRIBUT | À FAIRE |                                       |
 * +---------------+----------+---------+---------------------------------------+
 */

class OpGeometrieMaillage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Géométrie Maillage";
	static constexpr auto AIDE = "";

	OpGeometrieMaillage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_geometrie_maillage.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points = m_corps.points_pour_lecture();

		auto valeur_max = evalue_decimal("valeur_max");
		auto type_metrie = evalue_enum("type_metrie");

		auto attr_sortie = static_cast<Attribut *>(nullptr);

		if (type_metrie == "aire") {
			attr_sortie = calcul_donnees_aire(m_corps);
		}
		else if (type_metrie == "périmètre") {
			attr_sortie = calcul_donnees_perimetres(m_corps);
		}
		else if (type_metrie == "barycentre_poly") {
			attr_sortie = calcul_barycentre_poly(m_corps);
		}
		else if (type_metrie == "centroïde_poly") {
			attr_sortie = calcul_centroide_poly(m_corps);
		}
		else if (type_metrie == "dist_barycentre") {
			attr_sortie = calcul_donnees_dist_barycentre(m_corps);
		}
		else if (type_metrie == "dist_centroïde") {
			attr_sortie = calcul_donnees_dist_centroide(m_corps);
		}
		else if (type_metrie == "tangeante") {
			attr_sortie = calcul_tangeantes(m_corps);
		}
		else if (dls::outils::est_element(type_metrie, "courbure_min", "courbure_max", "direction_min", "direction_max", "var_géom", "gaussien", "moyenne")) {
			auto attr_N = m_corps.attribut("N");

			if (attr_N == nullptr || attr_N->portee != portee_attr::POINT) {
				calcul_normaux(m_corps, false, false);
				attr_N = m_corps.attribut("N");
			}

			auto rayon = evalue_decimal("rayon");

			if (evalue_bool("limite_rayon")) {
				rayon *= calcul_arrete_plus_longues(m_corps);
			}

			if (evalue_bool("relatif")) {
				auto barycentre = calcul_barycentre(points);
				auto dist_max = -std::numeric_limits<float>::max();

				for (auto i = 0; i < points->taille(); ++i) {
					auto d = longueur(barycentre - points->point(i));

					if (d > dist_max) {
						dist_max = d;
					}
				}

				rayon *= dist_max;
			}

			auto donnees_ret = calcule_courbure(
						contexte.chef,
						m_corps,
						static_cast<double>(rayon));

			if (donnees_ret.nombre_instable > 0) {
				dls::flux_chaine ss;
				ss << "Il y a " << donnees_ret.nombre_instable
				   << " points instables. Veuillez modifier le rayon pour stabiliser l'algorithme.";
				this->ajoute_avertissement(ss.chn());
				return res_exec::ECHOUEE;
			}

			if (donnees_ret.nombre_impossible > 0) {
				dls::flux_chaine ss;
				ss << "Il y a " << donnees_ret.nombre_impossible
				   << " points impossibles à calculer. Veuillez modifier le rayon pour stabiliser l'algorithme.";
				this->ajoute_avertissement(ss.chn());
				return res_exec::ECHOUEE;
			}

			if (type_metrie == "direction_min") {
				attr_sortie = m_corps.attribut("direction_min");
			}
			else if (type_metrie == "direction_max") {
				attr_sortie = m_corps.attribut("direction_max");
			}
			else if (type_metrie == "gaussien") {
				auto attr_courbure_min = m_corps.attribut("courbure_min");
				auto attr_courbure_max = m_corps.attribut("courbure_max");

				attr_sortie = m_corps.ajoute_attribut("gaussien", type_attribut::R32, 1, portee_attr::POINT);

				for (auto i = 0; i < points->taille(); ++i) {
					attr_sortie->r32(i)[0] = attr_courbure_min->r32(i)[0] * attr_courbure_max->r32(i)[0];
				}
			}
			else if (type_metrie == "moyenne") {
				auto attr_courbure_min = m_corps.attribut("courbure_min");
				auto attr_courbure_max = m_corps.attribut("courbure_max");
				attr_sortie = m_corps.ajoute_attribut("moyenne", type_attribut::R32, 1, portee_attr::POINT);

				for (auto i = 0; i < points->taille(); ++i) {
					attr_sortie->r32(i)[0] = (attr_courbure_min->r32(i)[0] + attr_courbure_max->r32(i)[0]) * 0.5f;
				}
			}
			else if (type_metrie == "courbure_min") {
				attr_sortie = m_corps.attribut("courbure_min");
			}
			else if (type_metrie == "courbure_max") {
				attr_sortie = m_corps.attribut("courbure_max");
			}
			else if (type_metrie == "var_géom") {
				attr_sortie = m_corps.attribut("var_geom");
			}
		}
		else if (type_metrie == "valence") {
			attr_sortie = calcul_valence(m_corps);
		}
		else if (type_metrie == "angle_sommets") {
			attr_sortie = calcul_angle_sommets(m_corps);
		}
		else if (type_metrie == "angle_dièdre") {
			attr_sortie = calcul_angle_diedre(m_corps);
		}
		else if (type_metrie == "longueur_arêtes") {
			attr_sortie = calcul_longueur_aretes(m_corps);
		}
		else {
			this->ajoute_avertissement("Type métrie inconnu");
			return res_exec::ECHOUEE;
		}

		if (valeur_max != 0.0f) {
			restreint_attribut_max(attr_sortie, valeur_max);
		}

		visualise_attribut(attr_sortie);

		return res_exec::REUSSIE;
	}

	void visualise_attribut(Attribut *attr)
	{
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, attr->portee);

		if (attr->type() == type_attribut::R32) {
			if (attr->dimensions == 1) {
				auto min_donnees = std::numeric_limits<float>::max();
				auto max_donnees = -min_donnees;

				min_max_attribut(attr, min_donnees, max_donnees);

				for (auto i = 0; i < attr->taille(); ++i) {
					assigne(attr_C->r32(i), couleur_min_max(attr->r32(i)[0], min_donnees, max_donnees));
				}
			}
			else if (attr->dimensions == 3) {
				for (auto i = 0; i < attr->taille(); ++i) {
					copie_attribut(attr_C, i, attr, i);
				}
			}
		}
	}
};

/* ************************************************************************** */

class OpFonteMaillage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fonte Maillage";
	static constexpr auto AIDE = "Simule un effet de fonte du maillage d'entrée";

	OpFonteMaillage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_fonte_maillage.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto points = m_corps.points_pour_ecriture();

		auto quantite = evalue_decimal("quantité", contexte.temps_courant);
		auto etalement = evalue_decimal("étalement", contexte.temps_courant);
		auto epaisseur = evalue_decimal("épaisseur", contexte.temps_courant);
		auto direction = evalue_enum("direction");
		auto amplitude_bruit = evalue_decimal("amplitude", contexte.temps_courant);
		auto frequence_bruit = evalue_vecteur("fréquence", contexte.temps_courant);
		auto decalage_bruit = evalue_vecteur("décalage", contexte.temps_courant);

		auto chef = contexte.chef;
		chef->demarre_evaluation("fonte maillage");

		/* calcul la boîte englobante */
		auto limites = calcule_limites_locales_corps(m_corps);
		auto const &min = limites.min;
		auto const &max = limites.max;

		auto attr_N = static_cast<Attribut *>(nullptr);

		auto centroide = dls::math::vec3f(0.0f);

		if (direction == "normal") {
			attr_N = m_corps.attribut("N");

			if (attr_N == nullptr || attr_N->portee != portee_attr::POINT) {
				calcul_normaux(m_corps, false, false);
				attr_N = m_corps.attribut("N");
			}
		}
		else {
			centroide = centre_masse_maillage(m_corps);
		}

		auto distance = (max.y - min.y);
		auto param_bruit = bruit::parametres();
		bruit::construit(bruit::type::SIMPLEX, param_bruit, 0);

		boucle_parallele(tbb::blocked_range<long>(0, points->taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				if (chef->interrompu()) {
					break;
				}

				auto p = points->point(i);

				p.y -= distance * quantite;

				if (p.y < min.y) {
					auto fraction_depassement = (min.y - p.y) / (max.y - min.y);

					auto poussee = dls::math::vec3f(0.0f);

					if (direction == "normal") {
						extrait(attr_N->r32(i), poussee);
						poussee.y = 0.0f;
					}
					else if (direction == "radial") {
						poussee = p - centroide;
						poussee.y = 0.0f;
					}

					/* Éjecte le point avec un peu de bruit si besoin est. */
					float n = 0.0f;

					if (amplitude_bruit != 0.0f) {
						n = amplitude_bruit * bruit::evalue(param_bruit, p * frequence_bruit + decalage_bruit);
					}

					p += ((quantite * etalement) + n) * fraction_depassement * poussee;

					p.y = min.y;

					/* Donne une épaisseur au point. */
					p.y -= fraction_depassement * epaisseur;
				}

				/* Pousse le point pour compenser le décalage descendant du pool */
				p.y += quantite * epaisseur;

				points->point(i, p);
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			chef->indique_progression_parallele(delta / static_cast<float>(points->taille()) * 100.0f);
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Test pour comprendre les Couleurs de Maillage ("Mesh Colors").
 * Voir http://www.cemyuksel.com/research/meshcolors/meshcolors_tog.pdf
 */
class OpCouleurMaillage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Couleur Maillage";
	static constexpr auto AIDE = "Crée des couleurs sur un maillage";

	OpCouleurMaillage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, true, true)) {
			return res_exec::ECHOUEE;
		}

		// l'algorithme n'est réelemnt que défini pour des triangles, pour les
		// polygones il faudra considérer d'autres manières de placer les
		// échantillons

		// les données doivent être stockées dans un tableau, avec les index
		// stockés sur les éléments (point, coté, polygone)

		auto R = 6;
		//auto const couleurs_par_point = 1;
		//auto const couleurs_par_cote = R - 1;
		//auto const couleurs_par_polygone = (R - 1) * (R - 2) / 2;

		auto chef = contexte.chef;
		chef->demarre_evaluation("couleur maillage");

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);

		pour_chaque_polygone_ferme(*corps_entree, [&](Corps const &corps_, Polygone const *poly)
		{
			auto Rd = static_cast<float>(R);

			auto v0 = corps_.point_transforme(poly->index_point(0));
			auto v1 = corps_.point_transforme(poly->index_point(1));
			auto v2 = corps_.point_transforme(poly->index_point(3));

			dls::math::vec3f couleurs[3] = {
				dls::math::vec3f(0.0f, 0.0f, 1.0f),
				dls::math::vec3f(0.0f, 1.0f, 0.0f),
				dls::math::vec3f(1.0f, 0.0f, 0.0f)
			};

			// les couleurs sont parfaitements alignés dans l'espace barycentrique
			for (auto i = 0; i <= R; ++i) {
				/* NOTE: pour les triangles il faut aller jusque R - i
				 * si on utilise des quadrilatères, il faudra considérer les
				 * quadrilatères n'ayant pas des cotés uniformes
				 */
				for (auto j = 0; j <= R; ++j) {
					auto const id = static_cast<float>(i);
					auto const jd = static_cast<float>(j);
					auto const u = id / Rd;
					auto const v = jd / Rd;
					auto const w = 1.0f - (id + jd) / Rd;

					auto point = w * v0 + u * v1 + v * v2;

					auto idx_point = m_corps.ajoute_point(point);

					if ((i == 0 || i == R) && (j == 0 || j == R)) {
						// nous sommes sur un point
						assigne(attr_C->r32(idx_point), couleurs[0]);
					}
					else if (((i == 0 || i == R) && j < R) || ((j == 0 || j == R) && i < R)) {
						// nous sommes sur un coté
						assigne(attr_C->r32(idx_point),  couleurs[1]);
					}
					else {
						// nous sommes dans le polygone
						assigne(attr_C->r32(idx_point),  couleurs[2]);
					}
				}
			}
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

static void ajourne_label_groupe(mi_face *face, unsigned int groupe)
{
	auto file = dls::file<mi_face *>();
	auto visites = dls::ensemble<mi_face *>();

	file.enfile(face);

	while (!file.est_vide()) {
		face = file.defile();

		if (visites.trouve(face) != visites.fin()) {
			continue;
		}

		visites.insere(face);

		face->label1 = groupe;

		/* pour chaque face autour de la nôtre */
		auto a0 = face->arete;
		auto a1 = a0->suivante;
		auto fin = a1;

		do {
			if (a0->paire != nullptr) {
				file.enfile(a0->paire->face);
			}

			a0 = a1;
			a1 = a0->suivante;
		} while (a1 != fin);
	}
}

class OpPiecesDetachees final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Pièces Détachées";
	static constexpr auto AIDE = "Groupe les polygones du maillage d'entrée selon leur connectivité de sorte que les polygones ou ensemble de polygones séparées des autres soient dans des groupes distincts.";

	OpPiecesDetachees(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto polyedre = converti_corps_polyedre(m_corps);

		auto nombre_groupe = 0u;
		for (auto face : polyedre.faces) {
			if (face->label1 != 0) {
				continue;
			}

			ajourne_label_groupe(face, ++nombre_groupe);
		}

		auto groupes = dls::tableau<GroupePrimitive *>(nombre_groupe);

		for (auto i = 0u; i < nombre_groupe; ++i) {
			groupes[i] = m_corps.ajoute_groupe_primitive("pièce" + dls::vers_chaine(i));
		}

		for (auto face : polyedre.faces) {
			groupes[face->label1 - 1]->ajoute_index(face->label);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_maillage(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceLissageLaplacien>());
	usine.enregistre_type(cree_desc<OperatriceTriangulation>());
	usine.enregistre_type(cree_desc<OperatriceNormaliseCovariance>());
	usine.enregistre_type(cree_desc<OperatriceAligneCovariance>());
	usine.enregistre_type(cree_desc<OperatriceBruitTopologique>());
	usine.enregistre_type(cree_desc<OperatriceErosionMaillage>());
	usine.enregistre_type(cree_desc<OpGeometrieMaillage>());
	usine.enregistre_type(cree_desc<OpFonteMaillage>());
	usine.enregistre_type(cree_desc<OpCouleurMaillage>());
	usine.enregistre_type(cree_desc<OpPiecesDetachees>());
}

#pragma clang diagnostic pop
