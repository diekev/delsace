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

#include "operatrices_simulations.hh"

#include "bibliotheques/outils/definitions.hh"

#include "../corps/groupes.h"

#include "../operatrice_simulation.hh"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceEntreeSimulation : public OperatriceCorps {
public:
	static constexpr auto NOM = "Entrée Simulation";
	static constexpr auto AIDE = "";

	explicit OperatriceEntreeSimulation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
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

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);
		INUTILISE(temps);

		m_corps.reinitialise();

		if (m_graphe_parent.donnees.empty()) {
			ajoute_avertissement("Les données du graphe sont vides !");
			return EXECUTION_ECHOUEE;
		}

		auto corps = std::any_cast<Corps *>(m_graphe_parent.donnees[0]);
		corps->copie_vers(&m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceGravite : public OperatriceCorps {
public:
	static constexpr auto NOM = "Gravité";
	static constexpr auto AIDE = "";

	explicit OperatriceGravite(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_gravite.jo";
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

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attr_F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		auto gravite = evalue_vecteur("gravité", temps);

		/* À FAIRE : f = m * a => multiplier par la masse? */
		for (long i = 0; i < nombre_points; ++i) {
			attr_F->vec3(i, gravite);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* À FAIRE : manipulatrice dédiée pour la position/orientation. */
class OperatriceVent : public OperatriceCorps {
public:
	static constexpr auto NOM = "Vent";
	static constexpr auto AIDE = "";

	explicit OperatriceVent(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_vent.jo";
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

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attr_F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		auto direction = evalue_vecteur("direction", temps);
		auto amplitude = evalue_decimal("amplitude", temps);

		auto force_max = direction * amplitude;

		/* À FAIRE : vent
		 * - intégration des forces dans le désordre (gravité après vent) par
		 *   exemple en utilisant un attribut extra.
		 * - turbulence
		 */
		for (long i = 0; i < nombre_points; ++i) {
			auto force = attr_F->vec3(i);

			for (size_t j = 0; j < 3; ++j) {
				force[j] = std::min(force_max[j], force[j] + force_max[j]);
			}

			attr_F->vec3(i, force);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSolveurParticules : public OperatriceCorps {
public:
	static constexpr auto NOM = "Solveur Particules";
	static constexpr auto AIDE = "";

	explicit OperatriceSolveurParticules(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
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

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attr_P = m_corps.ajoute_attribut("pos_pre", type_attribut::VEC3, portee_attr::POINT);
		auto attr_F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		/* À FAIRE : passe le temps par image en paramètre. */
		auto const temps_par_image = 1.0f / 24.0f;
		/* À FAIRE : masse comme propriété des particules */
		auto const masse = 1.0f; // eval_float("masse");
		auto const masse_inverse = 1.0f / masse;

		/* ajoute attribut vélocité */
		auto attr_V = m_corps.ajoute_attribut("V", type_attribut::VEC3, portee_attr::POINT);

		/* Ajourne la position des particules selon les équations :
		 * v(t) = F(t)dt
		 * x(t) = v(t)dt
		 *
		 * Voir :
		 * Physically Based Modeling: Principles and Practice
		 * https://www.cs.cmu.edu/~baraff/sigcourse/
		 */

		auto attr_desactiv = m_corps.ajoute_attribut("part_desactiv",
												  type_attribut::ENT8,
												  portee_attr::POINT);

		for (long i = 0; i < nombre_points; ++i) {
			auto desactivee = attr_desactiv->ent8(i);

			if (desactivee == 1) {
				continue;
			}

			auto pos = liste_points->point(i);

			/* a = f / m */
			auto const acceleration = attr_F->vec3(i) * masse_inverse;

			/* velocite = acceleration * temp_par_image + velocite */
			auto velocite = attr_V->vec3(i) + acceleration * temps_par_image;

			/* position = velocite * temps_par_image + position */
			auto npos = pos + velocite * temps_par_image;

			liste_points->point(i, npos);
			attr_V->vec3(i, velocite);
			attr_P->vec3(i, pos);
			attr_F->vec3(i, dls::math::vec3f(0.0f));
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

#if 0
/* collision avec un plan infini */
static auto verifie_collision(
		dls::math::vec3f const &pos_plan,
		dls::math::vec3f const &nor_plan,
		dls::math::vec3f const &pos,
		dls::math::vec3f const &vel,
		float rayon)
{
	const auto &XPdotN = dls::math::produit_scalaire(pos - pos_plan, nor_plan);

	/* Est-on à une distance epsilon du plan ? */
	if (XPdotN >= rayon + std::numeric_limits<float>::epsilon()) {
		return false;
	}

	/* Va-t-on vers le plan ? */
	if (dls::math::produit_scalaire(nor_plan, vel) >= 0.0f) {
		return false;
	}

	return true;
}
#endif

struct Triangle {
	using type_vec = dls::math::vec3f;
	type_vec v0;
	type_vec v1;
	type_vec v2;
};

struct Rayon {
	Triangle::type_vec direction;
	Triangle::type_vec origine;
};

/**
 * Algorithme de Möller-Trumbore.
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_entresection_algorithm
 */
static bool entresecte_triangle(Triangle const &triangle, Rayon const &rayon, float &distance)
{
	constexpr auto epsilon = 0.000001f;

	auto const &vertex0 = triangle.v0;
	auto const &vertex1 = triangle.v1;
	auto const &vertex2 = triangle.v2;

	auto const &cote1 = vertex1 - vertex0;
	auto const &cote2 = vertex2 - vertex0;
	auto const &h = dls::math::produit_croix(rayon.direction, cote2);
	auto const angle = dls::math::produit_scalaire(cote1, h);

	if (angle > -epsilon && angle < epsilon) {
		return false;
	}

	auto const f = 1.0f / angle;
	auto const &s = Triangle::type_vec(rayon.origine) - vertex0;
	auto const angle_u = f * dls::math::produit_scalaire(s, h);

	if (angle_u < 0.0f || angle_u > 1.0f) {
		return false;
	}

	auto const q = dls::math::produit_croix(s, cote1);
	auto const angle_v = f * dls::math::produit_scalaire(rayon.direction, q);

	if (angle_v < 0.0f || angle_u + angle_v > 1.0f) {
		return false;
	}

	/* À cette étape on peut calculer t pour trouver le point d'entresection sur
	 * la ligne. */
	auto const t = f * dls::math::produit_scalaire(cote2, q);

	/* Entresection avec le rayon. */
	if (t > epsilon) {
		distance = t;
		return true;
	}

	/* Cela veut dire qu'il y a une entresection avec une ligne, mais pas avec
	 * le rayon. */
	return false;
}

static long cherche_collision(
		Corps const *corps_collision,
		Rayon const &rayon_part,
		float &dist)
{
	auto const prims_collision = corps_collision->prims();
	auto const points_collision = corps_collision->points();

	/* À FAIRE : collision particules
	 * - structure accélération
	 */
	for (Primitive *prim : prims_collision->prims()) {
		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto const &v0 = points_collision->point(poly->index_point(0));
			auto const &v1 = points_collision->point(poly->index_point(j - 1));
			auto const &v2 = points_collision->point(poly->index_point(j));

			auto const &v0_d = corps_collision->transformation(dls::math::point3d(v0));
			auto const &v1_d = corps_collision->transformation(dls::math::point3d(v1));
			auto const &v2_d = corps_collision->transformation(dls::math::point3d(v2));

			auto triangle = Triangle{};
			triangle.v0 = dls::math::vec3f(
							  static_cast<float>(v0_d.x),
							  static_cast<float>(v0_d.y),
							  static_cast<float>(v0_d.z));
			triangle.v1 = dls::math::vec3f(
							  static_cast<float>(v1_d.x),
							  static_cast<float>(v1_d.y),
							  static_cast<float>(v1_d.z));
			triangle.v2 = dls::math::vec3f(
							  static_cast<float>(v2_d.x),
							  static_cast<float>(v2_d.y),
							  static_cast<float>(v2_d.z));

			if (entresecte_triangle(triangle, rayon_part, dist)) {
				return static_cast<long>(prim->index);
			}
		}
	}

	return -1;
}

enum rep_collision {
	RIEN,
	REBONDIS,
	COLLE,
};

class OperatriceCollision : public OperatriceCorps {
public:
	static constexpr auto NOM = "Collision";
	static constexpr auto AIDE = "";

	explicit OperatriceCollision(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_collision.jo";
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

	int execute(const Rectangle &rectangle, const int temps) override
	{
		auto corps_collision = entree(1)->requiers_corps(rectangle, temps);

		if (corps_collision == nullptr) {
			ajoute_avertissement("Aucun Corps pour la collision trouvé !");
			return EXECUTION_ECHOUEE;
		}

		auto const prims_collision = corps_collision->prims();
		auto const points_collision = corps_collision->points();

		if (prims_collision->taille() == 0l) {
			ajoute_avertissement("Aucune primitive trouvé dans le Corps collision !");
			return EXECUTION_ECHOUEE;
		}

		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();

		auto const elasticite = evalue_decimal("élasticité", temps);
		/* À FAIRE : rayon comme propriété des particules */
		auto const rayon = evalue_decimal("rayon", temps);

		/* ajoute attribut vélocité */
		auto attr_V = m_corps.attribut("V");

		if (attr_V == nullptr) {
			ajoute_avertissement("Aucune attribut de vélocité trouvé !");
			return EXECUTION_ECHOUEE;
		}

		auto attr_P = m_corps.attribut("pos_pre");

		if (attr_P == nullptr) {
			ajoute_avertissement("Aucune attribut de position trouvé !");
			return EXECUTION_ECHOUEE;
		}

		auto const chaine_reponse = evalue_enum("réponse_collision");
		rep_collision reponse;

		if (chaine_reponse == "rien") {
			reponse = rep_collision::RIEN;
		}
		else if (chaine_reponse == "rebondis") {
			reponse = rep_collision::REBONDIS;
		}
		else if (chaine_reponse == "colle") {
			reponse = rep_collision::COLLE;
		}
		else {
			std::stringstream ss;
			ss << "Opération '" << chaine_reponse << "' inconnue\n";
			this->ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		auto groupe = m_corps.ajoute_groupe_point("collision");
		groupe->reinitialise();

		auto attr_desactiv = m_corps.ajoute_attribut("part_desactiv",
												  type_attribut::ENT8,
												  portee_attr::POINT);

		for (long i = 0; i < nombre_points; ++i) {
			auto pos_cou = liste_points->point(i);
			auto vel = attr_V->vec3(i);
			auto pos_pre = attr_P->vec3(i);
			auto desactivee = attr_desactiv->ent8(i);

			if (desactivee == 1) {
				continue;
			}

			/* Calcul la position en espace objet. */
			auto pos_monde_d = m_corps.transformation(dls::math::point3d(pos_pre));
			auto pos_monde = dls::math::vec3f(
								 static_cast<float>(pos_monde_d.x),
								 static_cast<float>(pos_monde_d.y),
								 static_cast<float>(pos_monde_d.z));

			auto rayon_part = Rayon{};
			rayon_part.origine = pos_monde;
			rayon_part.direction = normalise(pos_cou - pos_pre);

			auto dist = 1000.0f;
			auto index_prim = cherche_collision(corps_collision, rayon_part, dist);

			if (index_prim < 0) {
				continue;
			}

			if (dist > rayon) {
				continue;
			}

			groupe->ajoute_point(static_cast<size_t>(i));

			switch (reponse) {
				case rep_collision::RIEN:
				{
					break;
				}
				case rep_collision::REBONDIS:
				{
					auto prim = prims_collision->prim(static_cast<size_t>(index_prim));
					auto poly = dynamic_cast<Polygone *>(prim);
					auto const &v0 = points_collision->point(poly->index_point(0));
					auto const &v1 = points_collision->point(poly->index_point(1));
					auto const &v2 = points_collision->point(poly->index_point(2));

					auto const e1 = v1 - v0;
					auto const e2 = v2 - v0;
					auto nor_poly = normalise(produit_croix(e1, e2));

					/* Trouve le normal de la vélocité au point de collision. */
					auto nv = dls::math::produit_scalaire(nor_poly, vel) * nor_poly;

					/* Trouve la tangente de la vélocité. */
					auto tv = vel - nv;

					/* Le normal de la vélocité est multiplité par le coefficient
					 * d'élasticité. */
					vel = -elasticite * nv + tv;
					attr_V->vec3(i, vel);
					break;
				}
				case rep_collision::COLLE:
				{
					pos_cou = pos_pre + dist * rayon_part.direction;
					liste_points->point(i, pos_cou);
					attr_V->vec3(i, dls::math::vec3f(0.0f));
					attr_desactiv->ent8(i, 1);
					break;
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_simulations(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSimulation>());

	usine.enregistre_type(cree_desc<OperatriceEntreeSimulation>());
	usine.enregistre_type(cree_desc<OperatriceGravite>());
	usine.enregistre_type(cree_desc<OperatriceSolveurParticules>());
	usine.enregistre_type(cree_desc<OperatriceCollision>());
	usine.enregistre_type(cree_desc<OperatriceVent>());
}

#pragma clang diagnostic pop
