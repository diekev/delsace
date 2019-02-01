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

#include "operatrices_cheveux.h"

#include <random>

#include "bibliotheques/outils/definitions.hh"

#include "../corps/corps.h"

#include "../attribut.h"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static void init_min_max(dls::math::vec3f &min, dls::math::vec3f &max)
{
	for (size_t i = 0; i < 3; ++i) {
		min[i] = -std::numeric_limits<float>::max();
		max[i] =  std::numeric_limits<float>::max();
	}
}

struct Triangle {
	dls::math::vec3f p0{};
	dls::math::vec3f p1{};
	dls::math::vec3f p2{};
	dls::math::vec3f min{};
	dls::math::vec3f max{};
};

struct DonneesCollesion {
	dls::math::vec3f normal{};
	Arrete *segment{};
};

class ArbreOcternaire {
	std::vector<Triangle *> m_triangles{};

public:
	struct Octant {
		Octant *enfants[8] = {
			nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr
		};

		dls::math::vec3f min{}, max{};

		~Octant()
		{
			for (auto enfant : enfants) {
				delete enfant;
			}
		}
	};

	~ArbreOcternaire()
	{
		for (auto tri : m_triangles) {
			delete tri;
		}

		m_triangles.clear();
	}

	void ajoute_triangle(Triangle *tri)
	{
		m_triangles.push_back(tri);
	}

private:
	Octant m_racine{};
};

/* ************************************************************************** */

class OperatriceCreationCourbes final : public OperatriceCorps {
	enum {
		STYLE_CREATION_SEGMENT,
		STYLE_CREATION_LONGUEUR,
	};

	enum {
		DIRECTION_NORMAL,
		DIRECTION_PERSONALISEE,
	};

public:
	static constexpr auto NOM = "Création Courbes";
	static constexpr auto AIDE = "Crée des courbes.";

	explicit OperatriceCreationCourbes(Graphe &graphe_parent, Noeud *noeud)
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
		return "entreface/operatrice_3d_creation_courbes.jo";
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

		auto corps_particules = entree(0)->requiers_corps(rectangle, temps);

		if (corps_particules == nullptr) {
			ajoute_avertissement("Aucun corps trouvé en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto const liste_points = corps_particules->points();

		if (liste_points->taille() == 0ul) {
			ajoute_avertissement("Aucune particule trouvée dans le nuage de points !");
			return EXECUTION_ECHOUEE;
		}

		auto courbes = nullptr;

		INUTILISE(courbes);

		auto const nombre_courbes = liste_points->taille();
		INUTILISE(nombre_courbes);
		auto const segments = evalue_entier("segments");
		auto const inv_segments = 1.0f / static_cast<float>(segments);
		auto const taille_min = evalue_decimal("taille_min");
		auto const taille_max = evalue_decimal("taille_max");
		auto const multiplication_taille = evalue_decimal("multiplication_taille");
		auto const chaine_style_creation = evalue_enum("style_création");
		auto const longueur_segment = evalue_decimal("taille_segment");
		auto const nor = evalue_vecteur("normal");
		auto normal = normalise(dls::math::vec3f(nor.x, nor.y, nor.z));
		auto const chaine_direction = evalue_enum("direction");

		/* À FAIRE : biais de longueur entre [-1.0, 1.0], où -1.0 = plus de
		 * petites courbes que de grandes, et 1.0 = plus de grandes courbes que
		 * de petites.
		 */

		int style_creation;

		if (chaine_style_creation == "segments") {
			style_creation = STYLE_CREATION_SEGMENT;
		}
		else {
			style_creation = STYLE_CREATION_LONGUEUR;
		}

		Attribut *attr_N = nullptr;
		int direction;

		if (chaine_direction == "normal") {
			direction = DIRECTION_NORMAL;

			attr_N = corps_particules->attribut("N");

			if (attr_N == nullptr) {
				ajoute_avertissement("Aucun attribut normal (N) trouvé sur les particules !");
				return EXECUTION_ECHOUEE;
			}
		}
		else {
			direction = DIRECTION_PERSONALISEE;
		}

//		courbes->nombre_courbes = nombre_courbes;
//		courbes->segments_par_courbe = segments;

//		courbes->liste_points()->reserve(nombre_courbes * (segments + 1));
//		courbes->liste_segments()->reserve(nombre_courbes * segments);

		std::uniform_real_distribution<float> dist_taille(taille_min, taille_max);
		std::mt19937 rng(19937);

		auto index_point = 0ul;
		for (Point3D *point : liste_points->points()) {
			auto const taille_courbe = dist_taille(rng) * multiplication_taille;

			size_t nombre_segment;
			float taille_segment;

			if (style_creation == STYLE_CREATION_SEGMENT) {
				nombre_segment = static_cast<size_t>(segments);
				taille_segment = taille_courbe * inv_segments;
			}
			else {
				taille_segment = longueur_segment * multiplication_taille;
				nombre_segment = static_cast<size_t>(taille_courbe / taille_segment);
			}

			if (direction == DIRECTION_NORMAL) {
				normal = attr_N->vec3(index_point++);
			}

			auto pos = dls::math::vec3f(point->x, point->y, point->z);

			auto index_npoint = m_corps.ajoute_point(pos.x, pos.y, pos.z);

			auto polygone = Polygone::construit(&m_corps, type_polygone::OUVERT, nombre_segment + 1);
			polygone->ajoute_sommet(index_npoint);

			for (size_t j = 0; j < nombre_segment; ++j) {
				pos += (taille_segment * normal);

				index_npoint = m_corps.ajoute_point(pos.x, pos.y, pos.z);
				polygone->ajoute_sommet(index_npoint);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCollisionCheveux : public OperatriceCorps {
public:
	static constexpr auto NOM = "Collision Cheveux";
	static constexpr auto AIDE = "Collèse des cheveux avec un maillage.";

	int execute(const Rectangle &rectangle, const int temps) override
	{
		auto const courbes = charge_courbes(rectangle, temps);

		if (courbes == nullptr) {
			ajoute_avertissement("Aucune courbe n'a été trouvée !");
			return EXECUTION_ECHOUEE;
		}

		/* obtiens le maillage de collision */
		auto const maillage_collision = charge_maillage_collesion(rectangle, temps);

		if (maillage_collision == nullptr) {
			ajoute_avertissement("Aucun maillage de collision n'a été trouvé !");
			return EXECUTION_ECHOUEE;
		}

		/* le maillage de collision est triangulé et chaque triangle est stocké
		 * dans un arbre octernaire */
		ArbreOcternaire *arbre = construit_arbre(maillage_collision);

		/* identifie les courbes dont la boite englobante se trouve dans la
		 * boite englobante du maillage de collision */
		auto liste_courbes = trouve_courbes_pres_maillage(maillage_collision, courbes);

		INUTILISE(liste_courbes);


		/* chaque segment de ces courbes sont testés pour une collision avec les
		 * triangles, jusqu'à ce qu'on trouve une collision :
		 * 1. d'abord les octants de l'arbre dans lequel se trouve les segments
		 * de la courbe sont identifiés en utilisant une entresection segment/
		 * boite englobante
		 */
		//ArbreOcternaire::Octant *octants = trouve_octants(courbes);

		/* 2. pour les triangles se trouvant dans les octants, une entresection
		 * boite englobante segment/triangle est performée
		 */

		/* 3. si le test précité passe, une entresection triangle/segment est
		 * performée
		 *
		 * le normal au point de collésion, et le segment collésé sont stocké
		 */

		delete arbre;

		return EXECUTION_REUSSIE;
	}

	Corps const *charge_maillage_collesion(const Rectangle &rectangle, const int temps)
	{
		return entree(0)->requiers_corps(rectangle, temps);
	}

	Corps const *charge_courbes(const Rectangle &rectangle, const int temps)
	{
		return entree(1)->requiers_corps(rectangle, temps);
	}

	ArbreOcternaire *construit_arbre(Corps const *maillage)
	{
		ArbreOcternaire *arbre = new ArbreOcternaire;

		auto const points = maillage->points();
		auto const prims  = maillage->prims();

		for (Primitive *prim : prims->prims()) {
			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			auto triangle = new Triangle();
			triangle->p0 = points->point(poly->index_point(0));
			triangle->p1 = points->point(poly->index_point(1));
			triangle->p2 = points->point(poly->index_point(2));

			init_min_max(triangle->min, triangle->max);
			dls::math::extrait_min_max(triangle->p0, triangle->min, triangle->max);
			dls::math::extrait_min_max(triangle->p1, triangle->min, triangle->max);
			dls::math::extrait_min_max(triangle->p2, triangle->min, triangle->max);

			arbre->ajoute_triangle(triangle);

			if (poly->nombre_sommets() > 3) {
				triangle = new Triangle();
				triangle->p0 = points->point(poly->index_point(0));
				triangle->p1 = points->point(poly->index_point(2));
				triangle->p2 = points->point(poly->index_point(3));

				init_min_max(triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p0, triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p1, triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p2, triangle->min, triangle->max);

				arbre->ajoute_triangle(triangle);
			}
		}

		return arbre;
	}

	Corps const *trouve_courbes_pres_maillage(Corps const *maillage, Corps const *courbes)
	{
		INUTILISE(courbes);
		INUTILISE(maillage);
//		for (size_t i = 0; i < courbes->nombre_courbes; ++i) {
//			for (size_t j = 0; j < courbes->segments_par_courbe; ++j) {

//			}
//		}

		return nullptr;
	}
};

/* ************************************************************************** */

struct DonneesSysteme {
	float masse = 0.0f;
	float masse_inverse = 0.0f;
	float rigidite = 0.0f;
	float amortissement = 0.0f;
	float temps_par_image = 0.0f;
	dls::math::vec3f gravite = dls::math::vec3f(0.0f);

	DonneesSysteme() = default;
};


class OperatriceMasseRessort : public OperatriceCorps {
public:
	static constexpr auto NOM = "Masse Ressort";
	static constexpr auto AIDE = "";

	explicit OperatriceMasseRessort(Graphe &graphe_parent, Noeud *noeud)
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

		if (m_corps.prims()->taille() == 0ul) {
			return EXECUTION_REUSSIE;
		}

		auto attr_V = m_corps.ajoute_attribut("mr_V", type_attribut::VEC3, portee_attr::POINT, m_corps.points()->taille());

		auto donnees = DonneesSysteme{};
		donnees.gravite = dls::math::vec3f{0.0f, -9.80665f, 0.0f};
		donnees.amortissement = 1.0f; //eval_float("amortissement");
		donnees.masse = 5.0f;// eval_float("masse");
		donnees.masse_inverse = 1.0f / donnees.masse;
		donnees.rigidite = 10.0f;// eval_float("rigidité");
		donnees.temps_par_image = 1.0f / 24.0f;

		auto liste_points = m_corps.points();

		for (Primitive *prim : m_corps.prims()->prims()) {
			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::OUVERT) {
				continue;
			}

			/* le premier point est la racine */
			for (size_t i = 1; i < polygone->nombre_sommets(); ++i) {
				auto const pos_precedent = liste_points->point(polygone->index_point(i - 1));
				auto pos = liste_points->point(polygone->index_point(i));
				auto vel = attr_V->vec3(polygone->index_point(i));

				/* force = masse * acceleration */
				auto force = donnees.masse * donnees.gravite;

				/* Ajout d'une force de ressort selon la loi de Hooke :
				 * f = -k * déplacement */
				auto force_ressort = -donnees.rigidite * (pos - pos_precedent);
				force += force_ressort;

				/* Amortissement : retrait de la vélocité selon le coefficient
				 * d'amortissement. */
				auto force_amortisseur = vel * donnees.amortissement;
				force -= force_amortisseur;

				/* acceleration = force / masse */
				auto acceleration = force * donnees.masse_inverse;

				vel = vel + acceleration * donnees.temps_par_image;
				pos = pos + vel * donnees.temps_par_image;

				liste_points->point(polygone->index_point(i), pos);
				attr_V->vec3(polygone->index_point(i), vel);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_cheveux(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationCourbes>());
	usine.enregistre_type(cree_desc<OperatriceMasseRessort>());
}

#pragma clang diagnostic pop
