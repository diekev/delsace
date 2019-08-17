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

#include "biblinternes/math/bruit.hh"
#include <eigen3/Eigen/Eigenvalues>

#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/moultfilage/boucle.hh"

#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "corps/iteration_corps.hh"
#include "corps/limites_corps.hh"

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
				auto const &n = attr_N->vec3(i);
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

class OperatriceLissageLaplacien : public OperatriceCorps {
public:
	static constexpr auto NOM = "Lissage Laplacien";
	static constexpr auto AIDE = "Performe un lissage laplacien des points du corps d'entrée.";

	OperatriceLissageLaplacien(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points_entree = m_corps.points_pour_ecriture();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = m_corps.prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

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

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceTriangulation : public OperatriceCorps {
public:
	static constexpr auto NOM = "Triangulation";
	static constexpr auto AIDE = "Performe une triangulation des polygones du corps d'entrée.";

	OperatriceTriangulation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps trouvé en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Aucune primitves en entrée !");
			return EXECUTION_ECHOUEE;
		}

		corps_entree->copie_vers(&m_corps);

		/* À FAIRE : attributs vertex */
		auto paires_attrs = dls::tableau<std::pair<Attribut const *, Attribut *>>();

		for (auto const &attr : corps_entree->attributs()) {
			if (attr.portee == portee_attr::PRIMITIVE) {
				auto nattr = m_corps.attribut(attr.nom());
				nattr->reinitialise();

				paires_attrs.pousse({ &attr, nattr });
			}
			else if (attr.portee == portee_attr::VERTEX) {
				m_corps.supprime_attribut(attr.nom());
			}
		}

		m_corps.supprime_primitives();

		auto paires_grps = dls::tableau<std::pair<GroupePrimitive const *, GroupePrimitive *>>();

		for (auto const &grp : corps_entree->groupes_prims()) {
			auto ngrp = m_corps.ajoute_groupe_primitive(grp.nom);

			paires_grps.pousse({ &grp, ngrp });
		}

		pour_chaque_polygone(*corps_entree,
							 [&](Corps const &corps_entree_, Polygone *poly)
		{
			INUTILISE(corps_entree_);

			if (poly->type == type_polygone::OUVERT) {
				auto npoly = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

				for (auto j = 0; j < poly->nombre_sommets(); ++j) {
					npoly->ajoute_sommet(poly->index_point(j));
				}

				copie_donnees(poly, npoly, paires_attrs, paires_grps);
			}
			else {
				for (auto j = 2; j < poly->nombre_sommets(); ++j) {
					auto npoly = Polygone::construit(&m_corps, poly->type, 3);
					npoly->ajoute_sommet(poly->index_point(0));
					npoly->ajoute_sommet(poly->index_point(j - 1));
					npoly->ajoute_sommet(poly->index_point(j));

					copie_donnees(poly, npoly, paires_attrs, paires_grps);
				}
			}
		});

		return EXECUTION_REUSSIE;
	}

	void copie_donnees(
			Polygone *poly,
			Polygone *npoly,
			dls::tableau<std::pair<Attribut const *, Attribut *>> const &paires_attrs,
			dls::tableau<std::pair<GroupePrimitive const *, GroupePrimitive *>> const &paires_grps)
	{
		for (auto paire : paires_attrs) {
			paire.second->redimensionne(paire.second->taille() + 1);
			copie_attribut(paire.first, poly->index, paire.second, npoly->index);
		}

		for (auto paire : paires_grps) {
			if (!paire.first->contient(poly->index)) {
				continue;
			}

			paire.second->ajoute_primitive(npoly->index);
		}
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

class OperatriceNormaliseCovariance : public OperatriceCorps {
public:
	static constexpr auto NOM = "Normalise Covariance";
	static constexpr auto AIDE = "Modifie les points pour que la covariance moyenne soit égale à 1.";

	OperatriceNormaliseCovariance(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points = m_corps.points_pour_ecriture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

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

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceAligneCovariance : public OperatriceCorps {
public:
	static constexpr auto NOM = "Aligne Covariance";
	static constexpr auto AIDE = "Tourne l'objet de sorte que l'axe locale de "
								 "covariance le plus grand soit aligné avec "
								 "l'axe des X, et le second avec l'axe des Y.";

	OperatriceAligneCovariance(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points = m_corps.points_pour_ecriture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

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

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceBruitTopologique : public OperatriceCorps {
public:
	static constexpr auto NOM = "Bruit Topologique";
	static constexpr auto AIDE = "";

	OperatriceBruitTopologique(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points = m_corps.points_pour_ecriture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

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
			deplacement[i] += gna.uniforme(0.0f, poids_normaux) * attr_N->vec3(i);
		}

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			point += poids * deplacement[i];
			points->point(i, point);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceErosionMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Érosion Maillage";
	static constexpr auto AIDE = "";

	OperatriceErosionMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points = corps_entree->points_pour_lecture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto points_elimines = dls::tableau<char>(points->taille(), 0);
		auto index_voisins = cherche_index_voisins(*corps_entree);
		auto index_adjacents = cherche_index_adjacents(*corps_entree);

		/* Un point est sur une bordure si le nombre de points voisins est
		 * différents du nombre des primitives voisins. */
		auto nombre_elimines = 0;
		for (auto i = 0; i < points_elimines.taille(); ++i) {
			if (index_voisins[i].taille() != index_adjacents[i].taille()) {
				points_elimines[i] = 1;
				++nombre_elimines;
			}
		}

		if (nombre_elimines == 0) {
			return EXECUTION_REUSSIE;
		}

		/* trouve les primitives à supprimer */
		auto prims_eliminees = dls::tableau<char>(prims->taille(), 0);

		for (auto i = 0; i < points_elimines.taille(); ++i) {
			if (points_elimines[i] == 0) {
				continue;
			}

			for (auto j : index_adjacents[i]) {
				prims_eliminees[j] = 1;
			}
		}

		/* À FAIRE : transfère attributs, paramètres (iters, inverse) */

		/* ne copie que ce qui n'a pas été supprimé */

		/* les nouveaux index des points, puisque certains sont supprimés, il
		 * faut réindexer */
		auto nouveaux_index = dls::tableau<long>(points->taille(), -1);
		auto index = 0;

		for (auto i = 0; i < points_elimines.taille(); ++i) {
			if (points_elimines[i]) {
				continue;
			}

			nouveaux_index[i] = index++;

			auto point = points->point(i);
			m_corps.ajoute_point(point.x, point.y, point.z);
		}

		for (auto i = 0; i < prims_eliminees.taille(); ++i) {
			if (prims_eliminees[i]) {
				continue;
			}

			auto prim = prims->prim(i);
			auto poly = dynamic_cast<Polygone *>(prim);

			auto nprim = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				nprim->ajoute_sommet(nouveaux_index[poly->index_point(j)]);
			}
		}

		auto attr_N = corps_entree->attribut("N");

		if (attr_N != nullptr) {
			calcul_normaux(m_corps, attr_N->portee == portee_attr::PRIMITIVE, true);
		}

		return EXECUTION_REUSSIE;
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

	auto aires = corps.ajoute_attribut("aire", type_attribut::DECIMAL, portee_attr::PRIMITIVE);

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

		aires->valeur(poly->index, aire_poly);
	});

	return aires;
}

static auto calcul_donnees_perimetres(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto perimetres = corps.ajoute_attribut("aire", type_attribut::DECIMAL, portee_attr::PRIMITIVE);

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

		perimetres->valeur(poly->index, peri_poly);
	});

	return perimetres;
}

static auto calcul_barycentre_poly(Corps &corps)
{
	auto points = corps.points_pour_lecture();

	auto barycentres = corps.ajoute_attribut("barycentre", type_attribut::VEC3, portee_attr::PRIMITIVE);

	pour_chaque_polygone_ferme(corps,
							   [&](Corps const &corps_entree, Polygone *poly)
	{
		INUTILISE(corps_entree);

		auto barycentre = dls::math::vec3f(0.0f);

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			barycentre += points->point(poly->index_point(j));
		}

		barycentre /= static_cast<float>(poly->nombre_sommets());

		barycentres->valeur(poly->index, barycentre);
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
			aire += aires_poly->decimal(voisin);
		}

		aires_sommets[i] = aire;
	}

	auto centroides = corps.ajoute_attribut("centroide", type_attribut::VEC3, portee_attr::PRIMITIVE);

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

		centroides->valeur(poly->index, centroide);
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
	auto tangeantes = corps.ajoute_attribut("tangeantes", type_attribut::VEC3, portee_attr::POINT);
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

		tangeantes->valeur(i, tangeante);
	}

	return tangeantes;
}

static auto calcul_donnees_dist_point(Corps &corps, dls::math::vec3f const &centre)
{
	auto points = corps.points_pour_lecture();

	auto dist = corps.ajoute_attribut("distance", type_attribut::DECIMAL, portee_attr::POINT);

	for (auto i = 0; i < points->taille(); ++i) {
		auto d = longueur(points->point(i) - centre);

		dist->valeur(i, d);
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
		auto v = attr->decimal(i);

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
	if (attr->type() != type_attribut::DECIMAL) {
		return;
	}

	for (auto i = 0; i < attr->taille(); ++i) {
		auto v = attr->decimal(i);

		if (v > valeur_max) {
			attr->valeur(i, valeur_max);
		}
	}
}

/* +---------------+----------+---------+---------------------------------------+
 * | AIRE          | MAILLAGE | FAIT    | aire de chaque polygone               |
 * | PÉRIMÈTRE     | MAILLAGE | FAIT    | périmètre de chaque polygone          |
 * | VALENCE       | MAILLAGE | À FAIRE | nombre de voisins pour chaque sommets |
 * | ANGLE         | MAILLAGE | À FAIRE | angle de chaque vertex                |
 * | ANGLE DIEDRE  | MAILLAGE | À FAIRE | angle dièdre de chaque arrête         |
 * | LONGUEUR COTE | MAILLAGE | À FAIRE | longueur de chaque coté               |
 * | CENTROIDE     | MAILLAGE | FAIT    | centre de masse de chaque polygone    |
 * | BARYCENTRE    | MAILLAGE | FAIT    | barycentre de chaque polygone         |
 * | COURBURE      | MAILLAGE | FAIT    | courbures du maillage                 |
 * | GAUSSIEN      | MAILLAGE | FAIT    | produit des courbures principales     |
 * | MOYENNE       | MAILLAGE | FAIT    | moyenne des courbures principales     |
 * | GRADIENT      | ATTRIBUT | À FAIRE |                                       |
 * | LAPLACIEN     | ATTRIBUT | À FAIRE |                                       |
 * +---------------+----------+---------+---------------------------------------+
 */

class OpGeometrieMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Géométrie Maillage";
	static constexpr auto AIDE = "";

	OpGeometrieMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points = m_corps.points_pour_lecture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

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
				return EXECUTION_ECHOUEE;
			}

			if (donnees_ret.nombre_impossible > 0) {
				dls::flux_chaine ss;
				ss << "Il y a " << donnees_ret.nombre_impossible
				   << " points impossibles à calculer. Veuillez modifier le rayon pour stabiliser l'algorithme.";
				this->ajoute_avertissement(ss.chn());
				return EXECUTION_ECHOUEE;
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

				attr_sortie = m_corps.ajoute_attribut("gaussien", type_attribut::DECIMAL, portee_attr::POINT);

				for (auto i = 0; i < points->taille(); ++i) {
					attr_sortie->valeur(i, attr_courbure_min->decimal(i) * attr_courbure_max->decimal(i));
				}
			}
			else if (type_metrie == "moyenne") {
				auto attr_courbure_min = m_corps.attribut("courbure_min");
				auto attr_courbure_max = m_corps.attribut("courbure_max");
				attr_sortie = m_corps.ajoute_attribut("moyenne", type_attribut::DECIMAL, portee_attr::POINT);

				for (auto i = 0; i < points->taille(); ++i) {
					attr_sortie->valeur(i, (attr_courbure_min->decimal(i) + attr_courbure_max->decimal(i)) * 0.5f);
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
		else {
			this->ajoute_avertissement("Type métrie inconnu");
			return EXECUTION_ECHOUEE;
		}

		if (valeur_max != 0.0f) {
			restreint_attribut_max(attr_sortie, valeur_max);
		}

		visualise_attribut(attr_sortie);

		return EXECUTION_REUSSIE;
	}

	void visualise_attribut(Attribut *attr)
	{
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, attr->portee);
		attr_C->reinitialise();
		attr_C->reserve(attr->taille());

		if (attr->type() == type_attribut::DECIMAL) {
			auto min_donnees = std::numeric_limits<float>::max();
			auto max_donnees = -min_donnees;

			min_max_attribut(attr, min_donnees, max_donnees);

			for (auto i = 0; i < attr->taille(); ++i) {
				attr_C->pousse(couleur_min_max(attr->decimal(i), min_donnees, max_donnees));
			}
		}
		else if (attr->type() == type_attribut::VEC3) {
			for (auto i = 0; i < attr->taille(); ++i) {
				attr_C->pousse(attr->vec3(i));
			}
		}
	}
};

/* ************************************************************************** */

/* À FAIRE : bibliothèque de bruit. */
static auto bruit(dls::math::vec3f const &p)
{
	return dls::math::bruit_simplex_3d(p.x, p.y, p.z);
}

class OpFonteMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fonte Maillage";
	static constexpr auto AIDE = "Simule un effet de fonte du maillage d'entrée";

	OpFonteMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto points = m_corps.points_pour_ecriture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de points !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de primitives !");
			return EXECUTION_ECHOUEE;
		}

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
						auto Nn = attr_N->vec3(i);
						poussee.x = Nn.x;
						poussee.z = Nn.z;
					}
					else if (direction == "radial") {
						poussee = p - centroide;
						poussee.y = 0.0f;
					}

					/* Éjecte le point avec un peu de bruit si besoin est. */
					float n = 0.0f;

					if (amplitude_bruit != 0.0f) {
						n = amplitude_bruit * bruit(p * frequence_bruit + decalage_bruit);
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

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Test pour comprendre les Couleurs de Maillage ("Mesh Colors").
 * Voir http://www.cemyuksel.com/research/meshcolors/meshcolors_tog.pdf
 */
class OpCouleurMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Couleur Maillage";
	static constexpr auto AIDE = "Crée des couleurs sur un maillage";

	OpCouleurMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Le corps d'entrée est nul, rien n'est connecté");
			return EXECUTION_ECHOUEE;
		}

		auto points = corps_entree->points_pour_lecture();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de points !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de primitives !");
			return EXECUTION_ECHOUEE;
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

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

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

					m_corps.ajoute_point(point);

					if ((i == 0 || i == R) && (j == 0 || j == R)) {
						// nous sommes sur un point
						attr_C->pousse(couleurs[0]);
					}
					else if (((i == 0 || i == R) && j < R) || ((j == 0 || j == R) && i < R)) {
						// nous sommes sur un coté
						attr_C->pousse(couleurs[1]);
					}
					else {
						// nous sommes dans le polygone
						attr_C->pousse(couleurs[2]);
					}
				}
			}
		});

		return EXECUTION_REUSSIE;
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
}

#pragma clang diagnostic pop
