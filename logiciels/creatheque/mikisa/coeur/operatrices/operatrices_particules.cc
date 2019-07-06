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

#include <eigen3/Eigen/Eigenvalues>

#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/gna.hh"
#include "bibliotheques/outils/parallelisme.h"
#include "bibliotheques/outils/temps.hh"
#include "bibliotheques/structures/arbre_kd.hh"
#include "bibliotheques/structures/grille_particules.hh"

#include "bibloc/logeuse_memoire.hh"
#include "bibloc/tableau.hh"

#include "corps/corps.h"
#include "corps/groupes.h"
#include "corps/iteration_corps.hh"
#include "corps/triangulation.hh"

#include "../contexte_evaluation.hh"
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps == nullptr) {
			ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

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

		if (groupe->taille() == 0) {
			corps->copie_vers(&m_corps);
			return EXECUTION_REUSSIE;
		}

		dls::tableau<std::pair<Attribut *, Attribut *>> paires_attrs;
		paires_attrs.reserve(static_cast<long>(corps->attributs().size()));

		for (auto attr : corps->attributs()) {
			auto attr2 = m_corps.ajoute_attribut(attr->nom(), attr->type(), attr->portee);

			if (attr->portee == portee_attr::POINT) {
				paires_attrs.pousse({ attr, attr2 });
			}

			/* À FAIRE : copie attributs restants. */
		}

		m_corps.points()->reserve(points_corps->taille() - groupe->taille());

		/* Utilisation d'un tableau pour définir plus rapidement si un point est
		 * à garder ou non. Ceci donne une accélération de 10x avec des
		 * centaines de miliers de points à traverser. Peut-être pourrions nous
		 * également trier les groupes et utiliser une recherche binaire pour
		 * chercher plus rapidement les points, mais si dans le futur nous
		 * supporterons également la suppression des primitives, alors les
		 * points des primitives ne seront pas dans un groupe, et il faudra une
		 * structure de données séparée pour étiquetter les points à retirer.
		 * D'où l'utilisation d'un vecteur booléen. */
		dls::tableau<char> dans_le_groupe(points_corps->taille(), 0);

		for (auto i = 0; i < groupe->taille(); ++i) {
			dans_le_groupe[static_cast<long>(groupe->index(i))] = 1;
		}

		for (auto i = 0l; i < points_corps->taille(); ++i) {
			if (dans_le_groupe[i]) {
				continue;
			}

			auto const point = points_corps->point(i);
			m_corps.ajoute_point(point.x, point.y, point.z);

			for (auto &paire : paires_attrs) {
				auto attr_Vorig = paire.first;
				auto attr_Vdest = paire.second;

				switch (attr_Vorig->type()) {
					case type_attribut::ENT8:
						attr_Vdest->pousse(attr_Vorig->ent8(i));
						break;
					case type_attribut::ENT32:
						attr_Vdest->pousse(attr_Vorig->ent32(i));
						break;
					case type_attribut::DECIMAL:
						attr_Vdest->pousse(attr_Vorig->decimal(i));
						break;
					case type_attribut::CHAINE:
						attr_Vdest->pousse(attr_Vorig->chaine(i));
						break;
					case type_attribut::VEC2:
						attr_Vdest->pousse(attr_Vorig->vec2(i));
						break;
					case type_attribut::VEC3:
						attr_Vdest->pousse(attr_Vorig->vec3(i));
						break;
					case type_attribut::VEC4:
						attr_Vdest->pousse(attr_Vorig->vec4(i));
						break;
					case type_attribut::MAT3:
						attr_Vdest->pousse(attr_Vorig->mat3(i));
						break;
					case type_attribut::MAT4:
						attr_Vdest->pousse(attr_Vorig->mat4(i));
						break;
					default:
						break;
				}
			}
		}

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_groupe") {
			entree(0)->obtiens_liste_groupes_points(chaines);
		}
	}
};

/* ************************************************************************** */

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
			this->ajoute_avertissement("Il n'y a pas de corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto origine = evalue_enum("origine");

		if (origine == "points") {
			return genere_points_depuis_points(corps_entree, contexte.temps_courant);
		}

		if (origine == "primitives") {
			return genere_points_depuis_primitives(corps_entree, contexte.temps_courant);
		}

		if (origine == "volume") {
			return genere_points_depuis_volume(corps_entree, contexte.temps_courant);
		}

		if (origine == "attribut") {
			return genere_points_depuis_attribut(corps_entree);
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
			groupe_entree = corps_entree->groupe_point(nom_groupe_origine);

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

		auto gna = GNA(graine);

		for (auto i = 0; i < nombre_points; ++i) {
			auto pos_x = gna.uniforme(min.x, max.x);
			auto pos_y = gna.uniforme(min.y, max.y);
			auto pos_z = gna.uniforme(min.z, max.z);

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
			groupe_entree = corps_entree->groupe_primitive(nom_groupe_origine);

			if (groupe_entree == nullptr) {
				this->ajoute_avertissement("Le groupe d'origine n'existe pas !");
				return EXECUTION_ECHOUEE;
			}
		}

		auto triangles = convertis_maillage_triangles(corps_entree, groupe_entree);

		if (triangles.est_vide()) {
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

		auto attr_N = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);

		/* À FAIRE : il faudrait un meilleur algorithme pour mieux distribuer
		 *  les points sur les maillages, avec nombre_points = max nombre
		 *  points. En ce moment, l'algorithme peut en mettre plus que prévu. */
		auto const nombre_points_triangle = static_cast<long>(std::ceil(
					static_cast<double>(nombre_points) / static_cast<double>(triangles.taille())));

		auto const anime_graine = evalue_bool("anime_graine");
		auto const graine = evalue_entier("graine") + (anime_graine ? temps : 0);

		auto gna = GNA(graine);

		for (Triangle const &triangle : triangles) {
			auto const v0 = corps_entree->transformation(dls::math::point3d(triangle.v0));
			auto const v1 = corps_entree->transformation(dls::math::point3d(triangle.v1));
			auto const v2 = corps_entree->transformation(dls::math::point3d(triangle.v2));

			auto const e0 = v1 - v0;
			auto const e1 = v2 - v0;

			auto const nor_triangle_d = normalise(produit_croix(e0, e1));
			auto const nor_triangle = dls::math::vec3f(
						static_cast<float>(nor_triangle_d.x),
						static_cast<float>(nor_triangle_d.y),
						static_cast<float>(nor_triangle_d.z));

			for (long j = 0; j < nombre_points_triangle; ++j) {
				/* Génère des coordonnées barycentriques aléatoires. */
				auto r = gna.uniforme(0.0, 1.0);
				auto s = gna.uniforme(0.0, 1.0);

				if (r + s >= 1.0) {
					r = 1.0 - r;
					s = 1.0 - s;
				}

				auto pos = v0 + r * e0 + s * e1;

				auto index = m_corps.ajoute_point(
							static_cast<float>(pos.x),
							static_cast<float>(pos.y),
							static_cast<float>(pos.z));

				/* À FAIRE : échantillone proprement selon le type de normaux */
				attr_N->pousse(nor_triangle);

				if (groupe_sortie) {
					groupe_sortie->ajoute_point(index);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}

	int genere_points_depuis_attribut(Corps const *corps_entree)
	{
		auto nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut.empty()) {
			this->ajoute_avertissement("L'attribut n'est pas spécifié");
			return EXECUTION_ECHOUEE;
		}

		auto attr_source = corps_entree->attribut(nom_attribut);

		if (attr_source == nullptr) {
			std::stringstream ss;
			ss << "L'attribut '" << nom_attribut << "' n'existe pas !";
			this->ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		if (attr_source->type() != type_attribut::VEC3) {
			std::stringstream ss;
			ss << "L'attribut '" << nom_attribut << "' n'est pas de type vecteur !";
			this->ajoute_avertissement(ss.str());
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

		for (auto i = 0; i < attr_source->taille(); ++i) {
			auto p = attr_source->vec3(i);
			auto index = m_corps.ajoute_point(p.x, p.y, p.z);

			if (groupe_sortie) {
				groupe_sortie->ajoute_point(index);
			}
		}

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "groupe_origine") {
			auto origine = evalue_enum("origine");

			if (origine == "points") {
				entree(0)->obtiens_liste_groupes_points(chaines);
			}
			else if (origine == "primitives") {
				entree(0)->obtiens_liste_groupes_prims(chaines);
			}
		}
		else if (attache == "nom_attribut") {
			entree(0)->obtiens_liste_attributs(chaines);
		}
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

class ListeTriangle {
	Triangle *m_premier_triangle = nullptr;
	Triangle *m_dernier_triangle = nullptr;

public:
	ListeTriangle() = default;
	~ListeTriangle() = default;

	Triangle *ajoute(dls::math::vec3f const &v0, dls::math::vec3f const &v1, dls::math::vec3f const &v2)
	{
		auto triangle = memoire::loge<Triangle>("Triangle", v0, v1, v2);
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

		memoire::deloge("Triangle", triangle);
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

static void ajoute_triangle_boite(BoiteTriangle *boite, dls::math::vec3f const &v0, dls::math::vec3f const &v1, dls::math::vec3f const &v2)
{
	auto triangle = boite->triangles.ajoute(v0, v1, v2);
	boite->aire_minimum = std::min(boite->aire_minimum, triangle->aire);
	boite->aire_maximum = 2 * boite->aire_minimum;
	boite->aire_totale += triangle->aire;
}

static BoiteTriangle *choisis_boite(BoiteTriangle boites[], GNA &gna)
{
	auto aire_totale_boites = 0.0f;

	auto nombre_boite_vide = 0;
	for (auto i = 0; i < NOMBRE_BOITE; ++i) {
		if (boites[i].triangles.vide()) {
			++nombre_boite_vide;
			continue;
		}

		aire_totale_boites += boites[i].aire_totale;
	}

	if (nombre_boite_vide == NOMBRE_BOITE) {
		return nullptr;
	}

	auto debut = compte_tick_ms();

	while (true) {
		for (auto i = 0; i < NOMBRE_BOITE; ++i) {
			if (boites[i].triangles.vide()) {
				continue;
			}

			auto const probabilite_boite = boites[i].aire_totale / aire_totale_boites;

			if (gna.uniforme(0.0f, 1.0f) <= probabilite_boite) {
				return &boites[i];
			}
		}

		/* Évite les boucles infinies. */
		if ((compte_tick_ms() - debut) > 1000) {
			break;
		}
	}

	return nullptr;
}

static Triangle *choisis_triangle(BoiteTriangle *boite, GNA &gna)
{
#if 1
	static_cast<void>(gna);
	return boite->triangles.premier_triangle();
#else
	if (false) { // cause un crash
		auto tri = boite->triangles.premier_triangle();
		boite->aire_totale = 0.0f;

		while (tri != nullptr) {
			boite->aire_totale += tri->aire;
			tri = tri->suivant;
		}
	}

	auto debut = compte_tick_ms();

	while (true) {
		auto tri = boite->triangles.premier_triangle();

		while (tri != nullptr) {
			auto const probabilite_triangle = tri->aire / boite->aire_totale;

			if (gna.uniforme(0.0f, 1.0f) <= probabilite_triangle) {
				return tri;
			}

			tri = tri->suivant;
		}

		/* Évite les boucles infinies. */
		if ((compte_tick_ms() - debut) > 1000) {
			break;
		}
	}

	return boite->triangles.premier_triangle();
#endif
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

		auto corps_maillage = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_maillage == nullptr) {
			this->ajoute_avertissement("Il n'y pas de corps en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto const prims_maillage = corps_maillage->prims();

		if (prims_maillage->taille() == 0) {
			this->ajoute_avertissement("Il n'y pas de primitives dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto nom_groupe = evalue_chaine("nom_groupe");
		auto groupe_prim = static_cast<GroupePrimitive *>(nullptr);

		if (!nom_groupe.empty()) {
			groupe_prim = corps_maillage->groupe_primitive(nom_groupe);

			if (groupe_prim == nullptr) {
				std::stringstream ss;
				ss << "Aucun groupe de primitives nommé '" << nom_groupe
				   << "' trouvé sur le corps d'entrée !";

				this->ajoute_avertissement(ss.str());
				return EXECUTION_ECHOUEE;
			}
		}

		/* Convertis le maillage en triangles. */
		auto triangles_entree = convertis_maillage_triangles(corps_maillage, groupe_prim);

		if (triangles_entree.est_vide()) {
			this->ajoute_avertissement("Il n'y pas de polygones dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto aire_minimum = std::numeric_limits<float>::max();
		auto aire_maximum = 0.0f;
		auto aire_totale = 0.0f;

		auto limites_min = dls::math::point3d( std::numeric_limits<double>::max());
		auto limites_max = dls::math::point3d(-std::numeric_limits<double>::max());

		/* Calcule les informations sur les aires. */
		for (auto const &triangle : triangles_entree) {
			auto const v0_m = corps_maillage->transformation(dls::math::point3d(triangle.v0));
			auto const v1_m = corps_maillage->transformation(dls::math::point3d(triangle.v1));
			auto const v2_m = corps_maillage->transformation(dls::math::point3d(triangle.v2));

			auto aire = static_cast<float>(calcule_aire(v0_m, v1_m, v2_m));
			aire_minimum = std::min(aire_minimum, aire);
			aire_maximum = std::max(aire_maximum, aire);
			aire_totale += aire;

			extrait_min_max(v0_m, limites_min, limites_max);
			extrait_min_max(v1_m, limites_min, limites_max);
			extrait_min_max(v2_m, limites_min, limites_max);
		}

		/* Place les triangles dans les boites. */
		BoiteTriangle boites[NOMBRE_BOITE];

		for (auto const &triangle : triangles_entree) {
			auto const v0_m = corps_maillage->transformation(dls::math::point3d(triangle.v0));
			auto const v1_m = corps_maillage->transformation(dls::math::point3d(triangle.v1));
			auto const v2_m = corps_maillage->transformation(dls::math::point3d(triangle.v2));

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

		/* Ne considère que les triangles dont l'aire est supérieure à ce seuil. */
		auto const seuil_aire = aire_minimum / 10000.0f;
		auto const distance = evalue_decimal("distance");

		/* Calcule le nombre maximum de point. */
		auto const aire_cercle = constantes<float>::PI * (distance * 0.5f) * (distance * 0.5f);
		auto const nombre_points = static_cast<long>((aire_totale * DENSITE_CERCLE) / aire_cercle);
		std::cerr << "Nombre points prédits : " << nombre_points << '\n';

		auto points_nuage = m_corps.points();
		points_nuage->reserve(nombre_points);

		auto const graine = evalue_entier("graine");

		auto gna = GNA(graine);

		auto grille_particule = GrilleParticules(limites_min, limites_max, distance);

		auto debut = compte_tick_ms();

		auto attr_N = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);

		/* Tant qu'il reste des triangles à remplir... */
		while (true) {
			/* Choisis une boîte avec une probabilité proportionnelle à l'aire
			 * total des fragments de la boîte. À FAIRE. */
			BoiteTriangle *boite = choisis_boite(boites, gna);

			/* Toutes les boites sont vides, arrêt de l'algorithme. */
			if (boite == nullptr) {
				break;
			}

			/* Sélectionne un triangle proportionellement à son aire. */
			auto triangle = choisis_triangle(boite, gna);

			/* Choisis un point aléatoire p sur le triangle en prenant une
			 * coordonnée barycentrique aléatoire. */
			auto const v0 = triangle->v0;
			auto const v1 = triangle->v1;
			auto const v2 = triangle->v2;
			auto const e0 = v1 - v0;
			auto const e1 = v2 - v0;

			auto r = gna.uniforme(0.0f, 1.0f);
			auto s = gna.uniforme(0.0f, 1.0f);

			if (r + s >= 1.0f) {
				r = 1.0f - r;
				s = 1.0f - s;
			}

			auto point = v0 + r * e0 + s * e1;

			/* Vérifie que le point respecte la condition de distance minimal */
			//auto ok = verifie_distance_minimal(hachage_spatial, point, distance);
			auto ok = grille_particule.verifie_distance_minimal(point, distance);

			if (ok) {
				//chage_spatial.ajoute(point);
				grille_particule.ajoute(point);
				m_corps.ajoute_point(point.x, point.y, point.z);
				/* À FAIRE : échantillone proprement selon le type de normaux */
				auto nor = normalise(produit_croix(e0, e1));
				attr_N->pousse(nor);
				debut = compte_tick_ms();
			}

			/* Vérifie si le triangle est complétement couvert par un point de
			 * l'ensemble. */
			auto couvert = grille_particule.triangle_couvert(
						triangle->v0,
						triangle->v1,
						triangle->v2,
						distance);

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

					couvert = grille_particule.triangle_couvert(
								triangle_fils[i].v0,
								triangle_fils[i].v1,
								triangle_fils[i].v2,
								distance);

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

			/* Évite les boucles infinies. */
			if ((compte_tick_ms() - debut) > 1000) {
				break;
			}
		}

		std::cerr << "Nombre de points : " << points_nuage->taille() << "\n";

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_groupe") {
			entree(0)->obtiens_liste_groupes_prims(chaines);
		}
	}
};

/* ************************************************************************** */

/**
 * Implémentation partielle de l'algorithme de génération de maillage alpha de
 * "Enhancing Particle Methods for Fluid Simulation in Computer Graphics",
 * Hagit Schechter, 2013
 */

static bool construit_sphere(
		dls::math::vec3f const &x0,
		dls::math::vec3f const &x1,
		dls::math::vec3f const &x2,
		float const rayon,
		dls::math::vec3f &centre)
{
	auto const x0x1 = x0 - x1;
	auto const lx0x1 = longueur(x0x1);

	auto const x1x2 = x1 - x2;
	auto const lx1x2 = longueur(x1x2);

	auto const x2x0 = x2 - x0;
	auto const lx2x0 = longueur(x2x0);

	auto n = produit_croix(x0x1, x1x2);
	auto ln = longueur(n);

	auto radius_x = (lx0x1 * lx1x2 * lx2x0) / (2.0f * ln);

	if (radius_x > rayon) {
		return false;
	}

	auto const abs_n_sqr = (ln * ln);
	auto const inv_abs_n_sqr = 1.0f / abs_n_sqr;
	auto const inv_abs_n_sqr2 = 1.0f / (2.0f * abs_n_sqr);

	auto alpha = (longueur_carree(x1x2) * produit_scalaire(x0x1, x0 - x2)) * inv_abs_n_sqr2;
	auto beta = (longueur_carree(x0 - x2) * produit_scalaire(x1 - x0, x1x2)) * inv_abs_n_sqr2;
	auto gamma = (longueur_carree(x0x1) * produit_scalaire(x2x0, x2 - x1)) * inv_abs_n_sqr2;

	auto l = alpha * x0 + beta * x1 + gamma * x2;

	/* NOTE : selon le papier, c'est censé être
	 * (radius_x * radius_x - radius * radius)
	 * mais cela donne un nombre négatif, résultant en un NaN... */
	auto t = std::sqrt((radius_x - rayon) * (radius_x - rayon) * inv_abs_n_sqr);

	centre = l + t * n;

	if (est_nan(centre)) {
		return false;
	}

	return true;
}

static void trouve_points_voisins(
		ListePoints3D const &points,
		ListePoints3D &rpoints,
		dls::math::vec3f const &point,
		const float radius)
{
	for (auto i = 0; i < points.taille(); ++i) {
		const auto &pi = points.point(i);

		if (pi == point) {
			continue;
		}

		if (longueur(point - pi) < radius) {
			auto p3d = memoire::loge<Point3D>("Point3D");
			p3d->x = pi.x;
			p3d->y = pi.y;
			p3d->z = pi.z;

			rpoints.pousse(p3d);
		}
	}
}

static void construit_triangle(
		Corps &corps,
		int &tri_offset,
		float const radius,
		ListePoints3D const &N1,
		dls::math::vec3f const &pi)
{
	for (auto j = 0; j < N1.taille() - 1; ++j) {
		auto pj = N1.point(j), pk = N1.point(j + 1);
		dls::math::vec3f center;

		if (!construit_sphere(pi, pj, pk, radius, center)) {
			continue;
		}

		bool clear = true;

		for (auto k(0); k < N1.taille(); ++k) {
			if (k == j || k == (j + 1)) {
				continue;
			}

			if (longueur(N1.point(k) - center) < radius) {
				clear = false;
				break;
			}
		}

		if (!clear) {
			continue;
		}

		corps.ajoute_point(pi.x, pi.y, pi.z);
		corps.ajoute_point(pj.x, pj.y, pj.z);
		corps.ajoute_point(pk.x, pk.y, pk.z);

		auto poly = Polygone::construit(&corps, type_polygone::FERME, 3);
		poly->ajoute_sommet(tri_offset + 0);
		poly->ajoute_sommet(tri_offset + 1);
		poly->ajoute_sommet(tri_offset + 2);

		tri_offset += 3;
	}
}

static void construit_maillage_alpha(
		Corps const &corps_entree,
		float const radius,
		Corps &sortie)
{
	auto points_entree = corps_entree.points();
	auto tri_offset = 0;

	for (auto i = 0; i < points_entree->taille(); ++i) {
		auto point = points_entree->point(i);

		ListePoints3D N1;
		trouve_points_voisins(*points_entree, N1, point, 2.0f * radius);
		construit_triangle(sortie, tri_offset, radius, N1, point);
	}
}

class OperatriceMaillageAlpha : public OperatriceCorps {
public:
	static constexpr auto NOM = "Maillage Alpha";
	static constexpr auto AIDE =
			"Crée une surface à partir de points.";

	OperatriceMaillageAlpha(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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
			this->ajoute_avertissement("L'entrée n'est pas connectée !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Il n'y pas de points dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		construit_maillage_alpha(*corps_entree, 0.1f, m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceEnleveDoublons : public OperatriceCorps {
public:
	static constexpr auto NOM = "Enlève Doublons";
	static constexpr auto AIDE = "";

	OperatriceEnleveDoublons(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_enleve_doublons.jo";
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

		auto points_entree = corps_entree->points();

		auto dist = evalue_decimal("distance", contexte.temps_courant);

		/* À FAIRE : liste de points à garder : doublons[i] = i. */
		auto doublons = std::vector<int>(static_cast<size_t>(points_entree->taille()), -1);

#if 1
		auto arbre = ArbreKD(points_entree->taille());

		for (auto i = 0; i < points_entree->taille(); ++i) {
			arbre.insert(i, points_entree->point(i));
		}

		arbre.balance();

		auto doublons_trouves = arbre.calc_doublons_rapide(dist, false, doublons);
#else
		auto doublons_trouves = 0;

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto const &p1 = points_entree->point(i);

			for (auto j = i + 1; j < points_entree->taille(); ++j) {
				auto const &p2 = points_entree->point(j);

				auto d = longueur(p1 - p2);

				if (d <= dist && doublons[static_cast<size_t>(i)] == -1) {
					++doublons_trouves;
					doublons[static_cast<size_t>(i)] = j;
				}
			}
		}
#endif

		std::cerr << "Il y a " << points_entree->taille() << " points.\n";
		std::cerr << "Il y a " << doublons_trouves << " doublons.\n";

		if (doublons_trouves == 0) {
			corps_entree->copie_vers(&m_corps);
			return EXECUTION_REUSSIE;
		}

#if 1
		corps_entree->copie_vers(&m_corps);

		pour_chaque_polygone(m_corps,
							 [&](Corps const &, Polygone *poly)
		{
			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				auto index = static_cast<size_t>(poly->index_point(j));

				if (doublons[index] != -1) {
					poly->ajourne_index(j, doublons[index]);
				}
			}
		});

#else	/* À FAIRE : le réindexage n'est pas correcte. */
		/* Supprime les points */

		auto tamis_point = dls::tableau<bool>(doublons.size(), false);
		auto reindexage = dls::tableau<long>(doublons.size(), -1);
		auto nouvel_index = 0;

		std::cerr << "Calcul tamis, reindexage\n";

		for (auto i = 0ul; i < doublons.size(); ++i) {
			auto supprime = (doublons[i] != -1) && (doublons[i] != static_cast<int>(i));
			tamis_point[i] = supprime;

			if (supprime) {
				reindexage[i] = std::min(static_cast<int>(i), doublons[i]);
			}
			else {
				reindexage[i] = nouvel_index++;
			}
		}

		std::cerr << "Copie des points non supprimés\n";
		m_corps.points()->reserve(points_entree->taille() - doublons_trouves);

		for (auto i = 0; i < points_entree->taille(); ++i) {
			if (tamis_point[static_cast<size_t>(i)]) {
				continue;
			}

			auto p = corps_entree->point_transforme(i);

			m_corps.ajoute_point(p.x, p.y, p.z);
		}

		std::cerr << "Il y a '" << m_corps.points()->taille() << "' points de créés !\n";

		/* Copie les primitives. */

		std::cerr << "Création des primitives\n";
		pour_chaque_polygone(*corps_entree,
							 [&](Corps const &, Polygone *poly)
		{
			auto npoly = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				auto index = static_cast<size_t>(poly->index_point(j));

				if (reindexage[index] == -1 || reindexage[index] > m_corps.points()->taille()) {
					//std::cerr << "Ajout d'un index invalide !!!\n";
				}

				npoly->ajoute_sommet(reindexage[index]);
			}
		});

		std::cerr << "Copie des attributs\n";
		/* Copie les attributs */
		for (auto const attr : corps_entree->attributs()) {
			if (attr->portee != portee_attr::POINT) {
				auto copie_attr = memoire::loge<Attribut>(*attr);
				m_corps.ajoute_attribut(copie_attr);
				continue;
			}

			auto attr_point = m_corps.ajoute_attribut(attr->nom(), attr->type(), attr->portee);

			for (auto i = 0, j = 0; i < points_entree->taille(); ++i) {
				if (tamis_point[static_cast<size_t>(i)] == true) {
					continue;
				}

				copie_attribut(attr, i, attr_point, j++);
			}
		}

		std::cerr << "Fin de l'algorithme\n";
#endif

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceGiguePoints final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Gigue Points";
	static constexpr auto AIDE = "Gigue les points d'entrée.";

	explicit OperatriceGiguePoints(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_gigue_points.jo";
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
			this->ajoute_avertissement("Il n'y a pas de points en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto const graine = evalue_entier("graine");
		auto const taille = evalue_decimal("taille");
		auto const taille_par_axe = evalue_vecteur("taille_par_axe");

		/* À FAIRE : pondérer selon un attribut, genre taille de point d'une
		 * opératrice de création de points. */

		auto gna = GNA(graine);
		auto min = -0.5f * taille;
		auto max =  0.5f * taille;

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto p = points_entree->point(i);

			p.x += gna.uniforme(min, max) * taille_par_axe.x;
			p.y += gna.uniforme(min, max) * taille_par_axe.y;
			p.z += gna.uniforme(min, max) * taille_par_axe.z;

			points_entree->point(i, p);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationTrainee final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Trainée";
	static constexpr auto AIDE = "Crée une trainée derrière des particules selon leurs vélocités.";

	explicit OperatriceCreationTrainee(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_trainee_points.jo";
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

		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Il n'y a pas de points en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			this->ajoute_avertissement("L'attribut n'est pas nommé !");
			return EXECUTION_ECHOUEE;
		}

		auto attr_V = corps_entree->attribut(nom_attribut);

		if (attr_V == nullptr || attr_V->type() != type_attribut::VEC3 || attr_V->portee != portee_attr::POINT) {
			this->ajoute_avertissement("Aucun attribut vecteur trouvé sur les points !");
			return EXECUTION_ECHOUEE;
		}

		/* À FAIRE : transfère d'attributs */

		m_corps.points()->reserve(points_entree->taille() * 2);

		auto const dt = evalue_decimal("dt", contexte.temps_courant);
		auto const taille = evalue_decimal("taille", contexte.temps_courant) * dt;
		auto const inverse_direction = evalue_bool("inverse_direction");

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto p = points_entree->point(i);
			auto v = attr_V->vec3(i) * taille;

			m_corps.ajoute_point(p.x, p.y, p.z);

			/* Par défaut nous utilisons la vélocité, donc la direction normale
			 * est celle d'où nous venons. */
			if (inverse_direction) {
				p += v;
			}
			else {
				p -= v;
			}

			m_corps.ajoute_point(p.x, p.y, p.z);

			auto seg = Polygone::construit(&m_corps, type_polygone::OUVERT, 2);
			seg->ajoute_sommet(i * 2);
			seg->ajoute_sommet(i * 2 + 1);
		}

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_attribut") {
			entree(0)->obtiens_liste_attributs(chaines);
		}
	}
};

/* ************************************************************************** */

static auto calcul_centre_masse(
	dls::tableau<dls::math::vec3f> const &points)
{
#if 0
		/* calcul la masse totale */
		auto attr_M = m_corps.attribut("M");
		auto masse_totale = 0.0f;

		if (attr_M != nullptr) {
			/* À FAIRE : fonction pour accumuler les valeurs des attributs */
			for (auto i = 0; i < points_entree->taille(); ++i) {
				masse_totale += attr_M->valeur(i);
			}
		}
		else {
			masse_totale = static_cast<float>(points_entree->taille());
		}
#endif

	auto centre = dls::math::vec3f(0.0f);

	for (auto const &pi : points) {
		centre += pi; // * attr_M->valeur(i);
	}

	centre /= static_cast<float>(points.taille());
	//centre_masse /= masse_totale;

	return centre;
}

static auto calcul_covariance(
	dls::tableau<dls::math::vec3f> const &points,
	dls::math::vec3f const &centre)
{
	auto mat_covariance = dls::math::mat3x3f(0.0f);

	for (auto const &pi : points) {
		//auto mi = (attr_M != nullptr) ? attr_M->valeur(i) : 1.0f;
		auto vi = pi - centre;

		/* | x*x x*y x*z |
		 * | x*y y*y y*z |
		 * | x*z z*y z*z |
		 */
		mat_covariance[0][0] += vi.x * vi.x;// * mi;
		mat_covariance[0][1] += vi.x * vi.y;// * mi;
		mat_covariance[0][2] += vi.x * vi.z;// * mi;

		mat_covariance[1][0] += vi.y * vi.x;// * mi;
		mat_covariance[1][1] += vi.y * vi.y;// * mi;
		mat_covariance[1][2] += vi.y * vi.z;// * mi;

		mat_covariance[2][0] += vi.z * vi.x;// * mi;
		mat_covariance[2][1] += vi.z * vi.y;// * mi;
		mat_covariance[2][2] += vi.z * vi.z;// * mi;
	}

	//mat_covariance /= masse_totale;
	mat_covariance[0] /= static_cast<float>(points.taille());
	mat_covariance[1] /= static_cast<float>(points.taille());
	mat_covariance[2] /= static_cast<float>(points.taille());

	return mat_covariance;
}

/* calcul la force pour faire tourner point autour de l'axe */
static auto force_rotation(
		dls::math::vec3f const &point,
		dls::math::vec3f const &centre_masse,
		dls::math::vec3f const &normal,
		float const poids)
{
	auto dir = point - centre_masse;

	auto temp = produit_croix(normal, dir);
	temp *= poids;

	auto f = produit_croix(normal, temp);
	f *= poids;

	return f + temp;
}

/* calcul la force pour attirer point vers un axe */
static auto force_attirance(
		dls::math::vec3f const &point,
		dls::math::vec3f const &axe,
		float const poids)
{
	auto proj = produit_scalaire(point, axe) * axe;
	return (proj - point) * poids;
}

/**
 * https://research.dreamworks.com/wp-content/uploads/2018/07/talk_flow_field_revised-Edited.pdf
 * http://number-none.com/product/My%20Friend,%20the%20Covariance%20Body/
 */
class OpForceInteraction final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Force d'Interaction";
	static constexpr auto AIDE = "Influence les particules selon une distribution locale de particules voisines.";

	explicit OpForceInteraction(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_force_interaction.jo";
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
			this->ajoute_avertissement("Il n'y a pas de points en entrée");
			return EXECUTION_ECHOUEE;
		}

		/* À FAIRE :
		 * - calcul selon les particules se trouvant dans un rayon de la
		 *   particule courante (kd-tree).
		 * - fusion des particules se trouvant dans une fraction du rayon
		 *   afin d'optimiser le calcul des corps de covariance
		 */

		/* pousse ou tire la particule le long des axes locaux */
		auto const force_dir = evalue_vecteur("force_dir");
		/* attire la particule vers les axes locaux */
		auto const force_axe = evalue_vecteur("force_axe");
		/* rotationne la particule autour des axes locaux */
		auto const force_rot = evalue_vecteur("force_rot");
		/* attire la particule vers le centre de masse */
		auto const force_centre = evalue_decimal("force_centre");

		auto rayon = evalue_decimal("rayon");
		auto poids = evalue_decimal("poids");

		auto attr_F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		/* À FAIRE : il y a un effet yo-yo. */

		boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto p = points_entree->point(i);

#if 0
				auto force = attr_F->valeur(i);
				auto centre_masse = dls::math::vec3f(0.0f);
				auto vpx = dls::math::vec3f(1.0f, 0.0f, 0.0f);
				auto vpy = dls::math::vec3f(0.0f, 1.0f, 0.0f);
				auto vpz = dls::math::vec3f(0.0f, 0.0f, 1.0f);

				force += force_dir.x * vpx;
				force += force_dir.y * vpy;
				force += force_dir.z * vpz;

				/* attire la particule vers les axes locaux */
				if (force_axe.x != 0.0f) {
					force += force_attirance(p, vpx, force_axe.x);
				}

				if (force_axe.y != 0.0f) {
					force += force_attirance(p, vpy, force_axe.y);
				}

				if (force_axe.z != 0.0f) {
					force += force_attirance(p, vpz, force_axe.z);
				}

				/* rotationne la particule autour des axes locaux */
				if (force_rot.x != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpx), force_rot.x);
				}

				if (force_rot.y != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpy), force_rot.y);
				}

				if (force_rot.z != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpz), force_rot.z);
				}

				/* attire la particule vers le centre de masse */
				force += (centre_masse - p) * force_centre;

#else
				// À FAIRE : arbre k-d, simplifie points
				auto points_voisins = points_autour(p, i, rayon);

				if (points_voisins.est_vide()) {
					continue;
				}

				auto centre_masse = calcul_centre_masse(points_voisins);

				auto MC = calcul_covariance(points_voisins, centre_masse);

				/* calcul des vecteurs propres à la matrice */
				auto A = Eigen::Matrix<float, 3, 3>();
				A(0, 0) = MC[0][0];
				A(0, 1) = MC[0][1];
				A(0, 2) = MC[0][2];
				A(1, 0) = MC[1][0];
				A(1, 1) = MC[1][1];
				A(1, 2) = MC[1][2];
				A(2, 0) = MC[2][0];
				A(2, 1) = MC[2][1];
				A(2, 2) = MC[2][2];

				auto s = Eigen::SelfAdjointEigenSolver<Eigen::Matrix<float, 3, 3>>(A);

				auto vpx_e = s.eigenvectors().col(0) * s.eigenvalues()(0);
				auto vpy_e = s.eigenvectors().col(1) * s.eigenvalues()(1);
				auto vpz_e = s.eigenvectors().col(2) * s.eigenvalues()(2);

				auto vpx = dls::math::vec3f(vpx_e[0], vpx_e[1], vpx_e[2]);
				auto vpy = dls::math::vec3f(vpy_e[0], vpy_e[1], vpy_e[2]);
				auto vpz = dls::math::vec3f(vpz_e[0], vpz_e[1], vpz_e[2]);

				/* calcul la force pour la particule */
				auto force = attr_F->vec3(i);

				/* pousse ou tire la particule le long des axes locaux */
				force += force_dir.x * vpx;
				force += force_dir.y * vpy;
				force += force_dir.z * vpz;

				/* attire la particule vers les axes locaux */
				if (force_axe.x != 0.0f) {
					force += force_attirance(p, vpx, force_axe.x);
				}

				if (force_axe.y != 0.0f) {
					force += force_attirance(p, vpy, force_axe.y);
				}

				if (force_axe.z != 0.0f) {
					force += force_attirance(p, vpz, force_axe.z);
				}

				/* rotationne la particule autour des axes locaux */
				if (force_rot.x != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpx), force_rot.x);
				}

				if (force_rot.y != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpy), force_rot.y);
				}

				if (force_rot.z != 0.0f) {
					force += force_rotation(p, centre_masse, normalise(vpz), force_rot.z);
				}

				/* attire la particule vers le centre de masse */
				force += (centre_masse - p) * force_centre;
#endif

				attr_F->valeur(i, force * poids);
			}
		});

		return EXECUTION_REUSSIE;
	}

	dls::tableau<dls::math::vec3f> points_autour(
		dls::math::vec3f const &centre,
		long index,
		float rayon)
	{
		dls::tableau<dls::math::vec3f> res;
		auto points_entree = m_corps.points();

		for (auto i = 0; i < points_entree->taille(); ++i) {
			if (i == index) {
				continue;
			}

			auto p = points_entree->point(i);

			if (longueur(p - centre) <= rayon) {
				res.pousse(p);
			}
		}

		return res;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_particules(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationPoints>());
	usine.enregistre_type(cree_desc<OperatriceSuppressionPoints>());
	usine.enregistre_type(cree_desc<OperatriceTirageFleche>());
	usine.enregistre_type(cree_desc<OperatriceMaillageAlpha>());
	usine.enregistre_type(cree_desc<OperatriceEnleveDoublons>());
	usine.enregistre_type(cree_desc<OperatriceGiguePoints>());
	usine.enregistre_type(cree_desc<OperatriceCreationTrainee>());
	usine.enregistre_type(cree_desc<OpForceInteraction>());
}

#pragma clang diagnostic pop
