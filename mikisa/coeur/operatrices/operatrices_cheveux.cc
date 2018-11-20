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

#include "../corps/corps.h"
#include "../corps/courbes.h"
#include "../corps/maillage.h"
#include "../corps/nuage_points.h"

#include "../attribut.h"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

static void init_min_max(numero7::math::vec3f &min, numero7::math::vec3f &max)
{
	for (int i = 0; i < 3; ++i) {
		min[i] = -std::numeric_limits<float>::max();
		max[i] =  std::numeric_limits<float>::max();
	}
}

static void min_max_vecteur(numero7::math::vec3f &min, numero7::math::vec3f &max, const numero7::math::vec3f &v)
{
	for (int i = 0; i < 3; ++i) {
		if (v[i] < min[i]) {
			min[i] = v[i];
		}
		if (v[i] > max[i]) {
			max[i] = v[i];
		}
	}
}

struct Triangle {
	numero7::math::vec3f p0;
	numero7::math::vec3f p1;
	numero7::math::vec3f p2;
	numero7::math::vec3f min;
	numero7::math::vec3f max;
};

struct DonneesCollesion {
	numero7::math::vec3f normal;
	Arrete *segment;
};

class ArbreOcternaire {
	std::vector<Triangle *> m_triangles;

public:
	struct Octant {
		Octant *enfants[8] = {
			nullptr, nullptr, nullptr, nullptr,
			nullptr, nullptr, nullptr, nullptr
		};

		numero7::math::vec3f min, max;

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
	Octant m_racine;
};

/* ************************************************************************** */

static constexpr auto NOM_CREATION_COURBES = "Création Courbes";
static constexpr auto AIDE_CREATION_COURBES = "Crée des courbes.";

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
	explicit OperatriceCreationCourbes(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
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

	const char *class_name() const override
	{
		return NOM_CREATION_COURBES;
	}

	const char *help_text() const override
	{
		return AIDE_CREATION_COURBES;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps_particules = input(0)->requiers_corps(rectangle, temps);

		if (corps_particules == nullptr) {
			ajoute_avertissement("Aucun corps trouvé en entrée !");
			return EXECUTION_ECHOUEE;
		}

		const auto liste_points = corps_particules->points();

		if (liste_points->taille() == 0ul) {
			ajoute_avertissement("Aucune particule trouvée dans le nuage de points !");
			return EXECUTION_ECHOUEE;
		}

		auto courbes = m_collection.cree_courbes();

		const auto nombre_courbes = liste_points->taille();
		const auto segments = evalue_entier("segments");
		const auto inv_segments = 1.0f / segments;
		const auto taille_min = evalue_decimal("taille_min");
		const auto taille_max = evalue_decimal("taille_max");
		const auto multiplication_taille = evalue_decimal("multiplication_taille");
		const auto chaine_style_creation = evalue_enum("style_création");
		const auto longueur_segment = evalue_decimal("taille_segment");
		const auto nor = evalue_vecteur("normal");
		auto normal = normalise(numero7::math::vec3f(nor.x, nor.y, nor.z));
		const auto chaine_direction = evalue_enum("direction");

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

		Attribut *attr_N;
		int direction;

		if (chaine_direction == "normal") {
			direction = DIRECTION_NORMAL;

			attr_N = corps_particules->attribut("N");

			if (attr_N == nullptr) {
				ajoute_avertissement("Aucun attribut normal (N) trouvé sur les particules !");
				direction = DIRECTION_PERSONALISEE;
			}
		}
		else {
			direction = DIRECTION_PERSONALISEE;
		}

		courbes->nombre_courbes = nombre_courbes;
		courbes->segments_par_courbe = segments;

		courbes->liste_points()->reserve(nombre_courbes * (segments + 1));
		courbes->liste_segments()->reserve(nombre_courbes * segments);

		std::uniform_real_distribution<float> dist_taille(taille_min, taille_max);
		std::mt19937 rng(19937);

		auto index_point = 0;
		for (Point3D *point : liste_points->points()) {
			const auto taille_courbe = dist_taille(rng) * multiplication_taille;

			int nombre_segment;
			float taille_segment;

			if (style_creation == STYLE_CREATION_SEGMENT) {
				nombre_segment = segments;
				taille_segment = taille_courbe * inv_segments;
			}
			else {
				taille_segment = longueur_segment * multiplication_taille;
				nombre_segment = static_cast<int>(taille_courbe / taille_segment);
			}

			if (direction == DIRECTION_NORMAL) {
				normal = attr_N->vec3(index_point++);
			}

			auto pos = numero7::math::vec3f(point->x, point->y, point->z);

			auto index_npoint = m_corps.ajoute_point(pos.x, pos.y, pos.z);

			auto polygone = Polygone::construit(&m_corps, POLYGONE_OUVERT, nombre_segment + 1);
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
	int execute(const Rectangle &rectangle, const int temps) override
	{
		Courbes *courbes = charge_courbes(rectangle, temps);

		if (courbes == nullptr) {
			ajoute_avertissement("Aucune courbe n'a été trouvée !");
			return EXECUTION_ECHOUEE;
		}

		/* obtiens le maillage de collision */
		Maillage *maillage_collision = charge_maillage_collesion(rectangle, temps);

		if (maillage_collision == nullptr) {
			ajoute_avertissement("Aucun maillage de collision n'a été trouvé !");
			return EXECUTION_ECHOUEE;
		}

		/* le maillage de collision est triangulé et chaque triangle est stocké
		 * dans un arbre octernaire */
		ArbreOcternaire *arbre = construit_arbre(maillage_collision);

		/* identifie les courbes dont la boite englobante se trouve dans la
		 * boite englobante du maillage de collision */
		Courbes *liste_courbes = trouve_courbes_pres_maillage(maillage_collision, courbes);

		static_cast<void>(liste_courbes);


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

	Maillage *charge_maillage_collesion(const Rectangle &rectangle, const int temps)
	{
		Collection collection_collision;
		input(0)->requiers_collection(collection_collision, rectangle, temps);

		Maillage *maillage_collision = nullptr;

		for (Corps *corps : collection_collision.plage()) {
			if (corps->type != CORPS_MAILLAGE) {
				continue;
			}

			maillage_collision = dynamic_cast<Maillage *>(corps);
		}

		return maillage_collision;
	}

	Courbes *charge_courbes(const Rectangle &rectangle, const int temps)
	{
		input(1)->requiers_collection(m_collection, rectangle, temps);

		for (Corps *corps : m_collection.plage()) {
			if (corps->type != CORPS_COURBE) {
				continue;
			}

			return dynamic_cast<Courbes *>(corps);
		}

		return nullptr;
	}

	ArbreOcternaire *construit_arbre(Maillage *maillage)
	{
		ArbreOcternaire *arbre = new ArbreOcternaire;

		for (Polygone *poly : maillage->liste_polys()->polys()) {
			auto triangle = new Triangle();
			triangle->p0 = poly->s[0]->pos;
			triangle->p1 = poly->s[1]->pos;
			triangle->p2 = poly->s[2]->pos;

			init_min_max(triangle->min, triangle->max);
			min_max_vecteur(triangle->min, triangle->max, triangle->p0);
			min_max_vecteur(triangle->min, triangle->max, triangle->p1);
			min_max_vecteur(triangle->min, triangle->max, triangle->p2);

			arbre->ajoute_triangle(triangle);

			if (poly->s[3] != nullptr) {
				triangle = new Triangle();
				triangle->p0 = poly->s[0]->pos;
				triangle->p1 = poly->s[2]->pos;
				triangle->p2 = poly->s[3]->pos;

				init_min_max(triangle->min, triangle->max);
				min_max_vecteur(triangle->min, triangle->max, triangle->p0);
				min_max_vecteur(triangle->min, triangle->max, triangle->p1);
				min_max_vecteur(triangle->min, triangle->max, triangle->p2);

				arbre->ajoute_triangle(triangle);
			}
		}

		return arbre;
	}

	Courbes *trouve_courbes_pres_maillage(Maillage *maillage, Courbes *courbes)
	{
		for (size_t i = 0; i < courbes->nombre_courbes; ++i) {
			for (size_t j = 0; j < courbes->segments_par_courbe; ++j) {

			}
		}

		return nullptr;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_cheveux(UsineOperatrice *usine)
{
	usine->register_type(NOM_CREATION_COURBES,
						 cree_desc<OperatriceCreationCourbes>(
							 NOM_CREATION_COURBES,
							 AIDE_CREATION_COURBES));
}
