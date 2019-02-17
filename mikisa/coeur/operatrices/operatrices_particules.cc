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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_particules.h"

#include <map>
#include <random>
#include <sstream>
#include <stack>

#include "../corps/corps.h"
#include "../corps/groupes.h"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceSuppressionPoints final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Suppression Points";
	static constexpr auto AIDE = "Supprime des points.";

	explicit OperatriceSuppressionPoints(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_suppression_point.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		auto corps = entree(0)->requiers_corps(rectangle, temps);

		if (corps == nullptr) {
			ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		/* À FAIRE : utilisation d'une 'liste' pour choisir le nom du groupe. */
		auto nom_groupe = evalue_chaine("nom_groupe");

		if (nom_groupe.empty()) {
			ajoute_avertissement("Le nom du groupe est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto points_corps = corps->points();
		auto groupe = corps->groupe_point(nom_groupe);

		if (groupe == nullptr) {
			ajoute_avertissement("Aucun groupe trouvé !");
			return EXECUTION_ECHOUEE;
		}

		std::vector<std::pair<Attribut *, Attribut *>> paires_attrs;
		paires_attrs.reserve(corps->attributs().size());

		for (auto attr : corps->attributs()) {
			auto attr2 = m_corps.ajoute_attribut(attr->nom(), attr->type(), attr->portee);

			if (attr->portee == portee_attr::POINT) {
				paires_attrs.push_back({ attr, attr2 });
			}

			/* À FAIRE : copie attributs restants. */
		}

		m_corps.points()->reserve(points_corps->taille() - groupe->taille());

		for (auto i = 0l; i < points_corps->taille(); ++i) {
			if (groupe->contiens(static_cast<size_t>(i))) {
				continue;
			}

			auto const point = points_corps->point(i);
			m_corps.ajoute_point(point.x, point.y, point.z);

			for (auto &paire : paires_attrs) {
				auto attr_Vorig = paire.first;
				auto attr_Vdest = paire.second;

				switch (attr_Vorig->type()) {
					case type_attribut::ENT8:
						attr_Vdest->pousse_ent8(attr_Vorig->ent8(i));
						break;
					case type_attribut::ENT32:
						attr_Vdest->pousse_ent32(attr_Vorig->ent32(i));
						break;
					case type_attribut::DECIMAL:
						attr_Vdest->pousse_decimal(attr_Vorig->decimal(i));
						break;
					case type_attribut::CHAINE:
						attr_Vdest->pousse_chaine(attr_Vorig->chaine(i));
						break;
					case type_attribut::VEC2:
						attr_Vdest->pousse_vec2(attr_Vorig->vec2(i));
						break;
					case type_attribut::VEC3:
						attr_Vdest->pousse_vec3(attr_Vorig->vec3(i));
						break;
					case type_attribut::VEC4:
						attr_Vdest->pousse_vec4(attr_Vorig->vec4(i));
						break;
					case type_attribut::MAT3:
						attr_Vdest->pousse_mat3(attr_Vorig->mat3(i));
						break;
					case type_attribut::MAT4:
						attr_Vdest->pousse_mat4(attr_Vorig->mat4(i));
						break;
					default:
						break;
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

struct Triangle {
	dls::math::vec3f v0 = dls::math::vec3f(0.0f, 0.0f, 0.0f);
	dls::math::vec3f v1 = dls::math::vec3f(0.0f, 0.0f, 0.0f);
	dls::math::vec3f v2 = dls::math::vec3f(0.0f, 0.0f, 0.0f);
	float aire = 0.0f;
	Triangle *precedent = nullptr, *suivant = nullptr;

	Triangle() = default;

	Triangle(dls::math::vec3f const &v_0, dls::math::vec3f const &v_1, dls::math::vec3f const &v_2)
		: Triangle()
	{
		v0 = v_0;
		v1 = v_1;
		v2 = v_2;
	}
};

std::vector<Triangle> convertis_maillage_triangles(Corps const *corps_entree, GroupePrimitive *groupe)
{
	std::vector<Triangle> triangles;
	auto const points = corps_entree->points();
	auto const prims  = corps_entree->prims();

	/* Convertis le maillage en triangles.
	 * Petit tableau pour comprendre le calcul du nombre de triangles.
	 * +----------------+------------------+
	 * | nombre sommets | nombre triangles |
	 * +----------------+------------------+
	 * | 3              | 1                |
	 * | 4              | 2                |
	 * | 5              | 3                |
	 * | 6              | 4                |
	 * | 7              | 5                |
	 * +----------------+------------------+
	 */

	auto nombre_triangles = 0l;

	iteratrice_index iter;

	if (groupe) {
		iter = iteratrice_index(groupe);
	}
	else {
		iter = iteratrice_index(prims->taille());
	}

	for (auto i : iter) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		nombre_triangles += poly->nombre_sommets() - 2;
	}

	triangles.reserve(static_cast<size_t>(nombre_triangles));

	for (auto ig : iter) {
		auto prim = prims->prim(ig);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (long i = 2; i < poly->nombre_sommets(); ++i) {
			Triangle triangle;

			triangle.v0 = points->point(poly->index_point(0));
			triangle.v1 = points->point(poly->index_point(i - 1));
			triangle.v2 = points->point(poly->index_point(i));

			triangles.push_back(triangle);
		}
	}

	return triangles;
}

/* À FAIRE : transfère attribut. */
class OperatriceCreationPoints : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Points";
	static constexpr auto AIDE = "Crée des points à partir des points ou des primitives d'un autre corps.";

	OperatriceCreationPoints(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dispersion_points.jo";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps_entree = entree(0)->requiers_corps(rectangle, temps);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Il n'y a pas de corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto origine = evalue_enum("origine");

		if (origine == "points") {
			return genere_points_depuis_points(corps_entree, temps);
		}

		if (origine == "primitives") {
			return genere_points_depuis_primitives(corps_entree, temps);
		}

		if (origine == "volume") {
			return genere_points_depuis_volume(corps_entree, temps);
		}

		ajoute_avertissement("Erreur : origine inconnue !");
		return EXECUTION_ECHOUEE;
	}

	int genere_points_depuis_points(Corps const *corps_entree, int temps)
	{
		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Il n'y a pas de points dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto nom_groupe_origine = evalue_chaine("groupe_origine");
		auto groupe_entree = static_cast<GroupePoint *>(nullptr);

		if (!nom_groupe_origine.empty()) {
			groupe_entree = m_corps.groupe_point(nom_groupe_origine);

			if (groupe_entree == nullptr) {
				this->ajoute_avertissement("Le groupe d'origine n'existe pas !");
				return EXECUTION_ECHOUEE;
			}
		}

		auto grouper_points = evalue_bool("grouper_points");
		auto groupe_sortie = static_cast<GroupePoint *>(nullptr);

		if (grouper_points) {
			auto nom_groupe = evalue_chaine("nom_groupe");

			if (nom_groupe.empty()) {
				this->ajoute_avertissement("Le nom du groupe de sortie est vide !");
				return EXECUTION_ECHOUEE;
			}

			groupe_sortie = m_corps.ajoute_groupe_point(nom_groupe);
		}

		auto points_sorties = m_corps.points();

		auto const nombre_points_emis = evalue_entier("nombre_points", temps);
		points_sorties->reserve(nombre_points_emis);

		auto iter = (groupe_entree != nullptr)
				? iteratrice_index(groupe_entree)
				: iteratrice_index(points_entree->taille());

		auto const nombre_points_par_points = (groupe_entree != nullptr)
				? nombre_points_emis / groupe_entree->taille()
				: nombre_points_emis / points_entree->taille();

		for (auto i : iter) {
			auto point = points_entree->point(i);
			auto const p_monde = corps_entree->transformation(
								dls::math::point3d(point.x, point.y, point.z));

			for (long j = 0; j < nombre_points_par_points; ++j) {
				auto index = m_corps.ajoute_point(
							static_cast<float>(p_monde.x),
							static_cast<float>(p_monde.y),
							static_cast<float>(p_monde.z));

				if (groupe_sortie) {
					groupe_sortie->ajoute_point(index);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}

	int genere_points_depuis_volume(Corps const *corps_entree, int temps)
	{
		/* création du conteneur */
		auto min = dls::math::vec3d( std::numeric_limits<double>::max());
		auto max = dls::math::vec3d(-std::numeric_limits<double>::max());

		auto points_maillage = corps_entree->points();

		for (auto i = 0; i < points_maillage->taille(); ++i) {
			auto point = points_maillage->point(i);
			auto point_monde = corps_entree->transformation(dls::math::point3d(point.x, point.y, point.z));

			for (size_t j = 0; j < 3; ++j) {
				if (point_monde[j] < min[j]) {
					min[j] = point_monde[j];
				}
				else if (point_monde[j] > max[j]) {
					max[j] = point_monde[j];
				}
			}
		}

		auto grouper_points = evalue_bool("grouper_points");
		auto groupe_sortie = static_cast<GroupePoint *>(nullptr);

		if (grouper_points) {
			auto nom_groupe = evalue_chaine("nom_groupe");

			if (nom_groupe.empty()) {
				this->ajoute_avertissement("Le nom du groupe de sortie est vide !");
				return EXECUTION_ECHOUEE;
			}

			groupe_sortie = m_corps.ajoute_groupe_point(nom_groupe);
		}

		auto const nombre_points = evalue_entier("nombre_points", temps);

		auto points_sorties = m_corps.points();
		points_sorties->reserve(nombre_points);

		auto const anime_graine = evalue_bool("anime_graine");
		auto const graine = evalue_entier("graine") + (anime_graine ? temps : 0);

		std::mt19937 rng(static_cast<size_t>(graine));
		std::uniform_real_distribution<double> dist_x(min.x, max.x);
		std::uniform_real_distribution<double> dist_y(min.y, max.y);
		std::uniform_real_distribution<double> dist_z(min.z, max.z);

		for (auto i = 0; i < nombre_points; ++i) {
			auto pos_x = dist_x(rng);
			auto pos_y = dist_y(rng);
			auto pos_z = dist_z(rng);

			auto index = m_corps.ajoute_point(
						static_cast<float>(pos_x),
						static_cast<float>(pos_y),
						static_cast<float>(pos_z));

			if (groupe_sortie) {
				groupe_sortie->ajoute_point(index);
			}
		}

		return EXECUTION_REUSSIE;
	}

	int genere_points_depuis_primitives(Corps const *corps_entree, int temps)
	{
		auto nom_groupe_origine = evalue_chaine("groupe_origine");
		auto groupe_entree = static_cast<GroupePrimitive *>(nullptr);

		if (!nom_groupe_origine.empty()) {
			groupe_entree = m_corps.groupe_primitive(nom_groupe_origine);

			if (groupe_entree == nullptr) {
				this->ajoute_avertissement("Le groupe d'origine n'existe pas !");
				return EXECUTION_ECHOUEE;
			}
		}

		auto triangles = convertis_maillage_triangles(corps_entree, groupe_entree);

		if (triangles.empty()) {
			this->ajoute_avertissement("Il n'y a pas de primitives dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto grouper_points = evalue_bool("grouper_points");
		auto groupe_sortie = static_cast<GroupePoint *>(nullptr);

		if (grouper_points) {
			auto nom_groupe = evalue_chaine("nom_groupe");

			if (nom_groupe.empty()) {
				this->ajoute_avertissement("Le nom du groupe de sortie est vide !");
				return EXECUTION_ECHOUEE;
			}

			groupe_sortie = m_corps.ajoute_groupe_point(nom_groupe);
		}

		auto const nombre_points = evalue_entier("nombre_points", temps);

		auto points_sorties = m_corps.points();
		points_sorties->reserve(nombre_points);

		/* À FAIRE : il faudrait un meilleur algorithme pour mieux distribuer
		 *  les points sur les maillages, avec nombre_points = max nombre
		 *  points. En ce moment, l'algorithme peut en mettre plus que prévu. */
		auto const nombre_points_triangle = static_cast<long>(std::ceil(
					static_cast<double>(nombre_points) / static_cast<double>(triangles.size())));

		auto const anime_graine = evalue_bool("anime_graine");
		auto const graine = evalue_entier("graine") + (anime_graine ? temps : 0);

		std::mt19937 rng(static_cast<size_t>(graine));
		std::uniform_real_distribution<double> dist(0.0, 1.0);

		for (Triangle const &triangle : triangles) {
			auto const v0 = corps_entree->transformation(dls::math::point3d(triangle.v0));
			auto const v1 = corps_entree->transformation(dls::math::point3d(triangle.v1));
			auto const v2 = corps_entree->transformation(dls::math::point3d(triangle.v2));

			auto const e0 = v1 - v0;
			auto const e1 = v2 - v0;

			for (long j = 0; j < nombre_points_triangle; ++j) {
				/* Génère des coordonnées barycentriques aléatoires. */
				auto r = dist(rng);
				auto s = dist(rng);

				if (r + s >= 1.0) {
					r = 1.0 - r;
					s = 1.0 - s;
				}

				auto pos = v0 + r * e0 + s * e1;

				auto index = m_corps.ajoute_point(
							static_cast<float>(pos.x),
							static_cast<float>(pos.y),
							static_cast<float>(pos.z));

				if (groupe_sortie) {
					groupe_sortie->ajoute_point(index);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * "Dart Throwing on Surfaces" (Cline et al. 2009)
 * http://peterwonka.net/Publications/pdfs/2009.EGSR.Cline.PoissonSamplingOnSurfaces.pdf
 */

/* La densité de l'arrangement de cercles ayant la plus grande densité, selon
 * Lagrange. C'est-à-dire le pourcentage d'aire qu'occuperait un arrangement de
 * cercle étant le plus compacte. */
static constexpr auto DENSITE_CERCLE = 0.9068996821171089f;

static constexpr auto NOMBRE_BOITE = 64;

/* ************************************************************************** */

struct HachageSpatial {
	std::unordered_map<std::size_t, std::vector<dls::math::vec3f>> m_tableau{};

	/**
	 * La taille maximum recommandée par la publication de Cline et al. est de
	 * 20 000. Cependant, les fonctions de hachage marche mieux quand la taille
	 * est un nombre premier ("Introduction to Algorithms", ISBN 0-262-03141-8),
	 * donc nous prenons le nombre premier le plus proche de 20 000.
	 */
	static constexpr auto TAILLE_MAX = 19997;

	/**
	 * Fonction de hachage repris de "Optimized Spatial Hashing for Collision
	 * Detection of Deformable Objects"
	 * http://www.beosil.com/download/CollisionDetectionHashing_VMV03.pdf
	 *
	 * Pour calculer l'empreinte d'une position, nous considérons la partie
	 * entière de celle-ci. Par exemple, le vecteur position <0.1, 0.2, 0.3>
	 * deviendra <0, 0, 0> ; de même pour le vecteur <0.4, 0.5, 0.6>. Ainsi,
	 * toutes les positions se trouvant entre <0, 0, 0> et
	 * <0.99.., 0.99.., 0.99..> seront dans la même alvéole.
	 */
	std::size_t fonction_empreinte(dls::math::vec3f const &position);

	/**
	 * Ajoute la posistion spécifiée dans le vecteur des positions ayant la même
	 * empreinte que celle-ci.
	 */
	void ajoute(dls::math::vec3f const &position);

	/**
	 * Retourne un vecteur contenant les positions ayant la même empreinte que
	 * la position passée en paramètre.
	 */
	std::vector<dls::math::vec3f> const &particules(dls::math::vec3f const &position);

	/**
	 * Retourne le nombre d'alvéoles présentes dans la table de hachage.
	 */
	size_t taille() const;
};

std::size_t HachageSpatial::fonction_empreinte(dls::math::vec3f const &position)
{
	return static_cast<std::size_t>(
				static_cast<int>(position.x) * 73856093
				^ static_cast<int>(position.y) * 19349663
				^ static_cast<int>(position.z) * 83492791) % TAILLE_MAX;
}

void HachageSpatial::ajoute(dls::math::vec3f const &position)
{
	auto const empreinte = fonction_empreinte(position);
	m_tableau[empreinte].push_back(position);
}

std::vector<dls::math::vec3f> const &HachageSpatial::particules(dls::math::vec3f const &position)
{
	auto const empreinte = fonction_empreinte(position);
	return m_tableau[empreinte];
}

size_t HachageSpatial::taille() const
{
	return m_tableau.size();
}

/* ************************************************************************** */

template <int O, typename T, int... Ns>
static auto calcule_aire(
		dls::math::vecteur<O, T, Ns...> const &v0,
		dls::math::vecteur<O, T, Ns...> const &v1,
		dls::math::vecteur<O, T, Ns...> const &v2)
{
	auto const c1 = v1 - v0;
	auto const c2 = v2 - v0;

	return longueur(produit_croix(c1, c2)) * static_cast<T>(0.5);
}

static float calcule_aire(Triangle const &triangle)
{
	return calcule_aire(triangle.v0, triangle.v1, triangle.v2);
}

class ListeTriangle {
	Triangle *m_premier_triangle = nullptr;
	Triangle *m_dernier_triangle = nullptr;

public:
	ListeTriangle() = default;
	~ListeTriangle() = default;

	Triangle *ajoute(dls::math::vec3f const &v0, dls::math::vec3f const &v1, dls::math::vec3f const &v2)
	{
		Triangle *triangle = new Triangle(v0, v1, v2);
		triangle->aire = calcule_aire(*triangle);
		triangle->precedent = nullptr;
		triangle->suivant = nullptr;

		if (m_premier_triangle == nullptr) {
			m_premier_triangle = triangle;
		}
		else {
			triangle->precedent = m_dernier_triangle;
			m_dernier_triangle->suivant = triangle;
		}

		m_dernier_triangle = triangle;

		return triangle;
	}

	void enleve(Triangle *triangle)
	{
		if (triangle->precedent) {
			triangle->precedent->suivant = triangle->suivant;
		}

		if (triangle->suivant) {
			triangle->suivant->precedent = triangle->precedent;
		}

		if (triangle == m_premier_triangle) {
			m_premier_triangle = triangle->suivant;

			if (m_premier_triangle) {
				m_premier_triangle->precedent = nullptr;
			}
		}

		if (triangle == m_dernier_triangle) {
			m_dernier_triangle = triangle->precedent;

			if (m_dernier_triangle) {
				m_dernier_triangle->precedent = nullptr;
			}
		}

		delete triangle;
	}

	Triangle *premier_triangle()
	{
		return m_premier_triangle;
	}

	bool vide() const
	{
		return m_premier_triangle == nullptr;
	}
};

struct BoiteTriangle {
	float aire_minimum = std::numeric_limits<float>::max();
	float aire_maximum = 0.0f; // = 2 * aire_minimum
	float aire_totale = 0.0f;
	float pad{};

	ListeTriangle triangles{};
};

void ajoute_triangle_boite(BoiteTriangle *boite, dls::math::vec3f const &v0, dls::math::vec3f const &v1, dls::math::vec3f const &v2)
{
	auto triangle = boite->triangles.ajoute(v0, v1, v2);
	boite->aire_minimum = std::min(boite->aire_minimum, triangle->aire);
	boite->aire_maximum = 2 * boite->aire_minimum;
	boite->aire_totale += triangle->aire;
}

bool verifie_distance_minimal(HachageSpatial &hachage_spatial, dls::math::vec3f const &point, float distance)
{
	auto const points = hachage_spatial.particules(point);

	for (auto p = 0ul; p < points.size(); ++p) {
		if (longueur(point - points[p]) < distance) {
			return false;
		}
	}

	return true;
}

bool triangle_couvert(Triangle const &triangle, HachageSpatial &hachage_spatial, const float radius)
{
	auto const &v0 = triangle.v0;
	auto const &v1 = triangle.v1;
	auto const &v2 = triangle.v2;

	auto const centre_triangle = (v0 + v1 + v2) / 3.0f;
	auto const points = hachage_spatial.particules(centre_triangle);

	for (auto p = 0ul; p < points.size(); ++p) {
		if (longueur(v0 - points[p]) <= radius
			&& longueur(v1 - points[p]) <= radius
			&& longueur(v2 - points[p]) <= radius)
		{
			return true;
		}
	}

	return false;
}

class OperatriceTirageFleche : public OperatriceCorps {
public:
	static constexpr auto NOM = "Tirage de flèche";
	static constexpr auto AIDE =
			"Crée des points sur une surface en l'échantillonnant par tirage de flèche.";

	OperatriceTirageFleche(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_tirage_fleche.jo";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps_maillage = entree(0)->requiers_corps(rectangle, temps);

		if (corps_maillage == nullptr) {
			this->ajoute_avertissement("Il n'y pas de corps en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto const prims_maillage = corps_maillage->prims();

		if (prims_maillage->taille() == 0) {
			this->ajoute_avertissement("Il n'y pas de primitives dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		/* Convertis le maillage en triangles. */
		auto const points = corps_maillage->points();

		auto aire_minimum = std::numeric_limits<float>::max();
		auto aire_maximum = 0.0f;
		auto aire_totale = 0.0f;

		/* Calcule les informations sur les aires. */
		for (auto ip = 0; ip < prims_maillage->taille(); ++ip) {
			auto prim = prims_maillage->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::FERME) {
				continue;
			}

			for (long i = 2; i < polygone->nombre_sommets(); ++i) {
				auto const v0 = points->point(polygone->index_point(0));
				auto const v1 = points->point(polygone->index_point(i - 1));
				auto const v2 = points->point(polygone->index_point(i));
				auto const v0_m = corps_maillage->transformation(dls::math::point3d(v0));
				auto const v1_m = corps_maillage->transformation(dls::math::point3d(v1));
				auto const v2_m = corps_maillage->transformation(dls::math::point3d(v2));

				auto aire = static_cast<float>(calcule_aire(v0_m, v1_m, v2_m));
				aire_minimum = std::min(aire_minimum, aire);
				aire_maximum = std::max(aire_maximum, aire);
				aire_totale += aire;
			}
		}

		/* Place les triangles dans les boites. */
		BoiteTriangle boites[NOMBRE_BOITE];

		for (auto ip = 0; ip < prims_maillage->taille(); ++ip) {
			auto prim = prims_maillage->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::FERME) {
				continue;
			}

			for (long i = 2; i < polygone->nombre_sommets(); ++i) {
				auto const v0 = points->point(polygone->index_point(0));
				auto const v1 = points->point(polygone->index_point(i - 1));
				auto const v2 = points->point(polygone->index_point(i));
				auto const v0_m = corps_maillage->transformation(dls::math::point3d(v0));
				auto const v1_m = corps_maillage->transformation(dls::math::point3d(v1));
				auto const v2_m = corps_maillage->transformation(dls::math::point3d(v2));

				auto aire = static_cast<float>(calcule_aire(v0_m, v1_m, v2_m));

				auto const index_boite = static_cast<int>(std::log2(aire_maximum / aire));

				if (index_boite < 0 || index_boite >= 64) {
					std::stringstream ss;
					ss << "Erreur lors de la génération de l'index d'une boîte !";
					ss << "\n   Index : " << index_boite;
					ss << "\n   Aire triangle : " << aire;
					ss << "\n   Aire totale : " << aire_maximum;
					this->ajoute_avertissement(ss.str());
					continue;
				}

				ajoute_triangle_boite(&boites[index_boite],
									  dls::math::vec3f(static_cast<float>(v0_m.x), static_cast<float>(v0_m.y), static_cast<float>(v0_m.z)),
									  dls::math::vec3f(static_cast<float>(v1_m.x), static_cast<float>(v1_m.y), static_cast<float>(v1_m.z)),
									  dls::math::vec3f(static_cast<float>(v2_m.x), static_cast<float>(v2_m.y), static_cast<float>(v2_m.z)));
			}
		}

		/* Ne considère que les triangles dont l'aire est supérieure à ce seuil. */
		auto const seuil_aire = aire_minimum / 10000.0f;
		auto const distance = evalue_decimal("distance");

		/* Calcule le nombre maximum de point. */
		auto const aire_cercle = static_cast<float>(M_PI) * (distance * 0.5f) * (distance * 0.5f);
		auto const nombre_points = static_cast<long>((aire_totale * DENSITE_CERCLE) / aire_cercle);
		std::cerr << "Nombre points prédits : " << nombre_points << '\n';

		auto points_nuage = m_corps.points();
		points_nuage->reserve(nombre_points);

		auto const graine = evalue_entier("graine");
		std::mt19937 rng(graine);
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		HachageSpatial hachage_spatial;

		/* Tant qu'il reste des triangles à remplir... */
		while (true) {
			/* Choisis une boîte avec une probabilité proportionnelle à l'aire
			 * total des fragments de la boîte. À FAIRE. */
			BoiteTriangle *boite = nullptr;
			auto boite_trouvee = false;
//			auto index_boite = 0;

			for (auto i = 0; i < NOMBRE_BOITE; ++i) {
				if (boites[i].triangles.vide()) {
					continue;
				}

				boite = &boites[i];
				boite_trouvee = true;
//				index_boite = i;
				break;

//				auto const probabilite_boite = boite->aire_totale / aire_totale;

//				if (dist(rng) <= probabilite_boite) {
//					boite_trouvee = true;
//					break;
//				}
			}

			/* Toutes les boites sont vides, arrêt de l'algorithme. */
			if (!boite_trouvee) {
				break;
			}

			/* Sélectionne un triangle proportionellement à son aire. */
			Triangle *triangle = boite->triangles.premier_triangle();
//			bool triangle_trouve = false;

//			for (auto tri : boite->triangles.triangles()) {
//				if (tri->jete) {
//					continue;
//				}

//				triangle = tri;
//				triangle_trouve = true;
//				break;

////				auto const probabilite_triangle = tri.aire / boite->aire_maximum;

////				if (dist(rng) <= probabilite_triangle) {
////					triangle = &tri;
////					triangle_trouve = true;
////					break;
////				}
//			}

//			if (!triangle_trouve) {
//				std::cerr << "Ne trouve pas de triangles !\n";
//				std::cerr << "Boîte : " << index_boite << '\n';
//				std::cerr << "Taille : " << boite->triangles.taille() << '\n';
//				continue;
//			}

			/* Choisis un point aléatoire p sur le triangle en prenant une
			 * coordonnée barycentrique aléatoire. */
			auto const v0 = triangle->v0;
			auto const v1 = triangle->v1;
			auto const v2 = triangle->v2;
			auto const e0 = v1 - v0;
			auto const e1 = v2 - v0;

			auto r = dist(rng);
			auto s = dist(rng);

			if (r + s >= 1.0f) {
				r = 1.0f - r;
				s = 1.0f - s;
			}

			auto point = v0 + r * e0 + s * e1;

			/* Vérifie que le point respecte la condition de distance minimal */
			auto ok = verifie_distance_minimal(hachage_spatial, point, distance);

			if (ok) {
				hachage_spatial.ajoute(point);
				m_corps.ajoute_point(point.x, point.y, point.z);
			}

			/* Vérifie si le triangle est complétement couvert par un point de
			 * l'ensemble. */
			auto couvert = triangle_couvert(*triangle, hachage_spatial, distance);

			if (couvert) {
				/* Si couvert, jète le triangle. */
				boite->aire_totale -= triangle->aire;
				boite->triangles.enleve(triangle);
			}
			else {
				/* Sinon, coupe le triangle en petit morceaux, et ajoute ceux
				 * qui ne ne sont pas totalement couvert à la liste, sauf si son
				 * aire est plus petite que le seuil d'acceptance. */

				/* On coupe le triangle en quatre en introduisant un point au
				 * centre de chaque coté. */
				auto const v01 = (v0 + v1) * 0.5f;
				auto const v12 = (v1 + v2) * 0.5f;
				auto const v20 = (v2 + v0) * 0.5f;

				Triangle triangle_fils[4] = {
					Triangle{ v0, v01, v20},
					Triangle{v01,  v1, v12},
					Triangle{v12,  v2, v20},
					Triangle{v20, v01, v12},
				};

				for (auto i = 0; i < 4; ++i) {

					auto const aire = calcule_aire(triangle_fils[i]);

					if (std::abs(aire - seuil_aire) <= std::numeric_limits<float>::epsilon()) {
						boite->aire_totale -= aire;
						continue;
					}

					couvert = triangle_couvert(triangle_fils[i], hachage_spatial, distance);

					if (couvert) {
						continue;
					}

					auto const index_boite0 = static_cast<int>(std::log2(aire_maximum / aire));

					if (index_boite0 >= 0 && index_boite0 < 64) {
						auto &b = boites[index_boite0];

						b.triangles.ajoute(triangle_fils[i].v0, triangle_fils[i].v1, triangle_fils[i].v2);
						b.aire_totale += aire;
					}
				}

				boite->triangles.enleve(triangle);
			}
		}

		std::cerr << "Nombre de points : " << points_nuage->taille() << "\n";
		std::cerr << "Nombre d'alvéoles : " << hachage_spatial.taille() << '\n';
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_particules(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationPoints>());
	usine.enregistre_type(cree_desc<OperatriceSuppressionPoints>());
	usine.enregistre_type(cree_desc<OperatriceTirageFleche>());
}

#pragma clang diagnostic pop
