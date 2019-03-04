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
#include <set>

#include "bibliotheques/outils/gna.hh"
#include "bibliotheques/outils/parallelisme.h"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "courbure.hh"
#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCourbureMaillage : public OperatriceCorps {
public:
	static constexpr auto NOM = "Courbure Maillage";
	static constexpr auto AIDE = "Calcul la courbure du maillage d'entrée";

	OperatriceCourbureMaillage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_courbure_maillage.jo";
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

		auto points_entree = m_corps.points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto relatif = evalue_bool("relatif");
		auto courbure_max = evalue_decimal("courbure_max");
		auto rayon = evalue_decimal("rayon");

		auto donnees_ret = calcule_courbure(
					m_corps,
					relatif,
					static_cast<double>(rayon),
					static_cast<double>(courbure_max));

		if (donnees_ret.nombre_instable > 0) {
			std::stringstream ss;
			ss << "Il y a " << donnees_ret.nombre_instable
			   << " points instables. Veuillez modifier le rayon pour stabiliser l'algorithme.";
			this->ajoute_avertissement(ss.str());
		}

		if (donnees_ret.nombre_impossible > 0) {
			std::stringstream ss;
			ss << "Il y a " << donnees_ret.nombre_impossible
			   << " points impossibles à calculer. Veuillez modifier le rayon pour stabiliser l'algorithme.";
			this->ajoute_avertissement(ss.str());
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto cherche_index_voisins(Corps const &corps)
{
	auto points_entree = corps.points();
	auto prims_entree = corps.prims();

	std::vector<std::set<long>> voisins(static_cast<size_t>(points_entree->taille()));

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto polygone = dynamic_cast<Polygone *>(prim);

		if (polygone->type != type_polygone::FERME) {
			continue;
		}

		for (auto j = 0; j < polygone->nombre_sommets() - 1; ++j) {
			auto i0 = polygone->index_point(j);
			auto i1 = polygone->index_point(j + 1);

			voisins[static_cast<size_t>(i0)].insert(i1);
			voisins[static_cast<size_t>(i1)].insert(i0);
		}

		auto dernier = polygone->index_point(polygone->nombre_sommets() - 1);
		auto premier = polygone->index_point(0);

		voisins[static_cast<size_t>(premier)].insert(dernier);
		voisins[static_cast<size_t>(dernier)].insert(premier);
	}

	return voisins;
}

static auto cherche_index_adjacents(Corps const &corps)
{
	auto points_entree = corps.points();
	auto prims_entree = corps.prims();

	std::vector<std::set<long>> adjacents(static_cast<size_t>(points_entree->taille()));

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto polygone = dynamic_cast<Polygone *>(prim);

		if (polygone->type != type_polygone::FERME) {
			continue;
		}

		for (auto j = 0; j < polygone->nombre_sommets(); ++j) {
			auto i0 = polygone->index_point(j);

			adjacents[static_cast<size_t>(i0)].insert(i);
		}
	}

	return adjacents;
}

static auto cherche_index_bordures(
		std::vector<std::set<long>> const &voisins,
		std::vector<std::set<long>> const &adjacents)
{
	std::vector<bool> bordures(voisins.size());

	for (auto i = 0ul; i < voisins.size(); ++i) {
		bordures[i] = voisins[i].size() != adjacents[i].size();
	}

	return bordures;
}

/* ************************************************************************** */

static auto calcule_lissage_normal(
		ListePoints3D const *points_entree,
		std::vector<bool> const &bordures,
		std::vector<std::set<long>> const &voisins,
		std::vector<dls::math::vec3f> &deplacement,
		bool preserve_bordures)
{
	boucle_parallele(tbb::blocked_range<size_t>(0, static_cast<size_t>(points_entree->taille())),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); i++) {
			if (bordures[i]) {
				if (preserve_bordures) {
					deplacement[i] = dls::math::vec3f(0.0f);
				}
				else {
					auto nn = voisins[i].size();
					if (!nn) {
						continue;
					}

					int nnused = 0;

					for (auto j : voisins[i]) {
						if (bordures[static_cast<size_t>(j)]) {
							continue;
						}

						deplacement[i] += points_entree->point(j);
						nnused++;
					}

					deplacement[i] /= static_cast<float>(nnused);
					deplacement[i] -= points_entree->point(static_cast<long>(i));
				}
			}
			else {
				auto nn = voisins[i].size();

				if (!nn) {
					continue;
				}

				for (auto j : voisins[i]) {
					deplacement[i] += points_entree->point(j);
				}

				deplacement[i] /= static_cast<float>(nn);
				deplacement[i] -= points_entree->point(static_cast<long>(i));
			}
		}
	});
}

static auto calcule_lissage_pondere(
		ListePoints3D const *points_entree,
		std::vector<bool> const &bordures,
		std::vector<std::set<long>> const &voisins,
		std::vector<dls::math::vec3f> &deplacement,
		bool preserve_bordures)
{
	boucle_parallele(tbb::blocked_range<size_t>(0, static_cast<size_t>(points_entree->taille())),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); i++) {
			if (bordures[i]) {
				if (preserve_bordures) {
					deplacement[i] = dls::math::vec3f(0.0f);
				}
				else {
					auto nn = voisins[i].size();
					if (!nn) {
						continue;
					}

					auto p = dls::math::vec3f(0.0f);
					auto const &xi = points_entree->point(static_cast<long>(i));
					auto poids = 1.0f;

					for (auto j : voisins[i]) {
						if (bordures[static_cast<size_t>(j)]) {
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
				auto nn = voisins[i].size();

				if (!nn) {
					continue;
				}

				auto p = dls::math::vec3f(0.0f);
				auto const &xi = points_entree->point(static_cast<long>(i));
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
		std::vector<dls::math::vec3f> const &deplacement,
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
				auto const &d = deplacement[static_cast<size_t>(i)];
				p += poids_lissage * (d - n * produit_scalaire(d, n));
			}
			else {
				p += poids_lissage * deplacement[static_cast<size_t>(i)];
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

		auto points_entree = m_corps.points();

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
			}
		}

		std::vector<dls::math::vec3f> deplacement(static_cast<size_t>(points_entree->taille()));

		points_entree->detache();

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

		/* copie les points, À FAIRE : partage */
		auto points_entree = corps_entree->points();
		m_corps.points()->reserve(points_entree->taille());
		m_corps.transformation = corps_entree->transformation;

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto point = points_entree->point(i);
			m_corps.ajoute_point(point.x, point.y, point.z);
		}

		/* À FAIRE : attributs, groupes */

		for (auto i = 0; i < prims->taille(); ++i) {
			auto prim = prims->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				/* À FAIRE : copie primitive */
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type == type_polygone::OUVERT) {
				auto npoly = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

				for (auto j = 0; j < poly->nombre_sommets(); ++j) {
					npoly->ajoute_sommet(poly->index_point(j));
				}
			}
			else {
				for (auto j = 2; j < poly->nombre_sommets(); ++j) {
					auto npoly = Polygone::construit(&m_corps, poly->type, 3);
					npoly->ajoute_sommet(poly->index_point(0));
					npoly->ajoute_sommet(poly->index_point(j - 1));
					npoly->ajoute_sommet(poly->index_point(j));
				}
			}
		}

		/* À FAIRE : recrée les normaux au bon endroit */
		if (corps_entree->attribut("N") != nullptr) {
			calcul_normaux(m_corps, true, false);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto centre_masse_maillage(Corps const &corps)
{
	auto prims = corps.prims();
	auto points = corps.points();
	auto centre_masse = dls::math::vec3f(0.0f);
	auto masse_totale = 0.0f;

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

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
	}

	if (masse_totale != 0.0f) {
		centre_masse /= masse_totale;
	}

	return centre_masse;
}

static auto covariance_maillage(Corps const &corps)
{
	auto prims = corps.prims();
	auto points = corps.points();

	auto MC = dls::math::mat3x3f(0.0f);
	auto aire_totale = 0.0f;

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

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
	}

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

		auto points = m_corps.points();

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

		auto points = m_corps.points();

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

		auto points = m_corps.points();

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
		}

		auto graine = evalue_entier("graine");
		auto poids = evalue_decimal("poids");
		auto poids_normaux = evalue_decimal("poids_normaux");
		auto poids_tangeantes = evalue_decimal("poids_tangeantes");

		auto index_voisins = cherche_index_voisins(m_corps);

		auto gna = GNA(graine);

		auto deplacement = std::vector<dls::math::vec3f>(
					static_cast<size_t>(points->taille()),
					dls::math::vec3f(0.0f));

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);

			/* tangeante */
			auto const &voisins = index_voisins[static_cast<size_t>(i)];

			for (auto voisin : voisins) {
				auto const pv = points->point(voisin);
				auto echelle = poids_tangeantes / (poids_tangeantes + longueur(pv - point));
				deplacement[static_cast<size_t>(i)] += gna.uniforme(0.0f, echelle) * (pv - point);
			}

			if (voisins.size() != 0) {
				deplacement[static_cast<size_t>(i)] /= static_cast<float>(voisins.size());
			}

			/* normal */
			deplacement[static_cast<size_t>(i)] += gna.uniforme(0.0f, poids_normaux) * attr_N->vec3(i);
		}

		for (auto i = 0; i < points->taille(); ++i) {
			auto point = points->point(i);
			point += poids * deplacement[static_cast<size_t>(i)];
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

		auto points = corps_entree->points();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto points_elimines = std::vector<bool>(static_cast<size_t>(points->taille()), false);
		auto index_voisins = cherche_index_voisins(*corps_entree);
		auto index_adjacents = cherche_index_adjacents(*corps_entree);

		/* Un point est sur une bordure si le nombre de points voisins est
		 * différents du nombre des primitives voisins. */
		auto nombre_elimines = 0;
		for (auto i = 0ul; i < points_elimines.size(); ++i) {
			if (index_voisins[i].size() != index_adjacents[i].size()) {
				points_elimines[i] = true;
				++nombre_elimines;
			}
		}

		if (nombre_elimines == 0) {
			return EXECUTION_REUSSIE;
		}

		/* trouve les primitives à supprimer */
		auto prims_eliminees = std::vector<bool>(static_cast<size_t>(prims->taille()), false);

		for (auto i = 0ul; i < points_elimines.size(); ++i) {
			if (points_elimines[i] == false) {
				continue;
			}

			for (auto j : index_adjacents[i]) {
				prims_eliminees[static_cast<size_t>(j)] = true;
			}
		}

		/* À FAIRE : transfère attributs, paramètres (iters, inverse) */

		/* ne copie que ce qui n'a pas été supprimé */

		/* les nouveaux index des points, puisque certains sont supprimés, il
		 * faut réindexer */
		auto nouveaux_index = std::vector<long>(static_cast<size_t>(points->taille()), -1);
		auto index = 0;

		for (auto i = 0ul; i < points_elimines.size(); ++i) {
			if (points_elimines[i]) {
				continue;
			}

			nouveaux_index[i] = index++;

			auto point = points->point(static_cast<long>(i));
			m_corps.ajoute_point(point.x, point.y, point.z);
		}

		for (auto i = 0ul; i < prims_eliminees.size(); ++i) {
			if (prims_eliminees[i]) {
				continue;
			}

			auto prim = prims->prim(static_cast<long>(i));
			auto poly = dynamic_cast<Polygone *>(prim);

			auto nprim = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				nprim->ajoute_sommet(nouveaux_index[static_cast<size_t>(poly->index_point(j))]);
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

void enregistre_operatrices_maillage(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCourbureMaillage>());
	usine.enregistre_type(cree_desc<OperatriceLissageLaplacien>());
	usine.enregistre_type(cree_desc<OperatriceTriangulation>());
	usine.enregistre_type(cree_desc<OperatriceNormaliseCovariance>());
	usine.enregistre_type(cree_desc<OperatriceAligneCovariance>());
	usine.enregistre_type(cree_desc<OperatriceBruitTopologique>());
	usine.enregistre_type(cree_desc<OperatriceErosionMaillage>());
}

#pragma clang diagnostic pop
