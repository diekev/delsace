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

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/attribut.h"
#include "corps/corps.h"
#include "corps/iteration_corps.hh"

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
	dls::tableau<Triangle *> m_triangles{};

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
				memoire::deloge("ArbreOcternaire::Octant", enfant);
			}
		}
	};

	~ArbreOcternaire()
	{
		for (auto tri : m_triangles) {
			memoire::deloge("Triangle", tri);
		}

		m_triangles.efface();
	}

	void ajoute_triangle(Triangle *tri)
	{
		m_triangles.pousse(tri);
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps_particules = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_particules == nullptr) {
			ajoute_avertissement("Aucun corps trouvé en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto const liste_points = corps_particules->points_pour_lecture();

		if (liste_points->taille() == 0l) {
			ajoute_avertissement("Aucune particule trouvée dans le nuage de points !");
			return EXECUTION_ECHOUEE;
		}

		auto const segments = evalue_entier("segments");
		auto const inv_segments = 1.0f / static_cast<float>(segments);
		auto const taille_min = evalue_decimal("taille_min");
		auto const taille_max = evalue_decimal("taille_max");
		auto const multiplication_taille = evalue_decimal("multiplication_taille");
		auto const chaine_style_creation = evalue_enum("style_création");
		auto const longueur_segment = evalue_decimal("taille_segment");
		auto normal = normalise(evalue_vecteur("normal"));
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

			if (attr_N->portee != portee_attr::POINT) {
				ajoute_avertissement("L'attribut normal (N) n'est pas sur les points !");
				return EXECUTION_ECHOUEE;
			}
		}
		else {
			direction = DIRECTION_PERSONALISEE;
		}

		auto gna = GNA();

		auto attr_L = m_corps.ajoute_attribut("longueur", type_attribut::DECIMAL, portee_attr::PRIMITIVE, true);
		attr_L->reserve(liste_points->taille());

		for (auto i = 0; i < liste_points->taille(); ++i) {
			auto const taille_courbe = gna.uniforme(taille_min, taille_max) * multiplication_taille;

			long nombre_segment;
			float taille_segment;

			if (style_creation == STYLE_CREATION_SEGMENT) {
				nombre_segment = segments;
				taille_segment = taille_courbe * inv_segments;
			}
			else {
				taille_segment = longueur_segment * multiplication_taille;
				nombre_segment = static_cast<long>(taille_courbe / taille_segment);
			}

			if (direction == DIRECTION_NORMAL) {
				normal = attr_N->vec3(i);
			}

			auto pos = liste_points->point(i);

			auto index_npoint = m_corps.ajoute_point(pos.x, pos.y, pos.z);

			auto polygone = Polygone::construit(&m_corps, type_polygone::OUVERT, nombre_segment + 1);
			polygone->ajoute_sommet(index_npoint);

			for (long j = 0; j < nombre_segment; ++j) {
				pos += (taille_segment * normal);

				index_npoint = m_corps.ajoute_point(pos.x, pos.y, pos.z);
				polygone->ajoute_sommet(index_npoint);
			}

			attr_L->pousse(taille_segment);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCollisionCheveux : public OperatriceCorps {
public:
	static constexpr auto NOM = "Collision Cheveux";
	static constexpr auto AIDE = "Collèse des cheveux avec un maillage.";

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto const courbes = charge_courbes(contexte, donnees_aval);

		if (courbes == nullptr) {
			ajoute_avertissement("Aucune courbe n'a été trouvée !");
			return EXECUTION_ECHOUEE;
		}

		/* obtiens le maillage de collision */
		auto const maillage_collision = charge_maillage_collesion(contexte, donnees_aval);

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

		memoire::deloge("ArbreOcternaire", arbre);

		return EXECUTION_REUSSIE;
	}

	Corps const *charge_maillage_collesion(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
	{
		return entree(0)->requiers_corps(contexte, donnees_aval);
	}

	Corps const *charge_courbes(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
	{
		return entree(1)->requiers_corps(contexte, donnees_aval);
	}

	ArbreOcternaire *construit_arbre(Corps const *maillage)
	{
		auto arbre = memoire::loge<ArbreOcternaire>("ArbreOcternaire");

		pour_chaque_polygone_ferme(*maillage,
								   [&](Corps const &corps, Polygone *poly)
		{
			for (long i = 2; i < poly->nombre_sommets(); ++i) {
				auto triangle = memoire::loge<Triangle>("Triangle");
				triangle->p0 = corps.point_transforme(poly->index_point(0));
				triangle->p1 = corps.point_transforme(poly->index_point(i - 1));
				triangle->p2 = corps.point_transforme(poly->index_point(i));

				init_min_max(triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p0, triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p1, triangle->min, triangle->max);
				dls::math::extrait_min_max(triangle->p2, triangle->min, triangle->max);

				arbre->ajoute_triangle(triangle);
			}
		});

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

/**
 * Sources :
 * - Khan Academy, Pixar in a Box
 * - http://roxlu.com/2013/006/hair-simulation
 * - http://matthias-mueller-fischer.ch/publications/FTLHairFur.pdf
 */
class OperatriceMasseRessort : public OperatriceCorps {
public:
	static constexpr auto NOM = "Masse Ressort";
	static constexpr auto AIDE = "";

	explicit OperatriceMasseRessort(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
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

		if (m_corps.prims()->taille() == 0l) {
			return EXECUTION_REUSSIE;
		}

		auto attr_L = m_corps.attribut("longueur");

		if (attr_L == nullptr) {
			ajoute_avertissement("Aucun attribut de longueur trouvé !");
			return EXECUTION_ECHOUEE;
		}

		auto attr_V = m_corps.ajoute_attribut("mr_V", type_attribut::VEC3, portee_attr::POINT);
		auto attr_P = m_corps.ajoute_attribut("mr_P", type_attribut::VEC3, portee_attr::POINT);
		auto attr_D = m_corps.ajoute_attribut("mr_D", type_attribut::VEC3, portee_attr::POINT);

		auto donnees = DonneesSysteme{};
		donnees.gravite = dls::math::vec3f{0.0f, -9.80665f, 0.0f};
		donnees.amortissement = 1.0f; //eval_float("amortissement");
		donnees.masse = 5.0f;// eval_float("masse");
		donnees.masse_inverse = 1.0f / donnees.masse;
		donnees.rigidite = 10.0f;// eval_float("rigidité");
		donnees.temps_par_image = 1.0f / 24.0f;

		auto liste_points = m_corps.points_pour_ecriture();

		auto dt = 0.1f;

		/* ajourne vélocités */
		pour_chaque_polygone_ouvert(m_corps,
									[&](Corps const &, Polygone *polygone)
		{
			/* le premier point est la racine */
			attr_P->valeur(polygone->index_point(0), liste_points->point(polygone->index_point(0)));

			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
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

				attr_P->valeur(polygone->index_point(i), pos);
				attr_V->valeur(polygone->index_point(i), vel);
			}
		});

		/* résolution des contraintes */
		pour_chaque_polygone_ouvert(m_corps,
									[&](Corps const &, Polygone *polygone)
		{
			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
				auto pa = polygone->index_point(i - 1);
				auto pb = polygone->index_point(i);

				auto const pos_precedent = attr_P->vec3(pa);
				auto cur_pos = attr_P->vec3(pb);
				auto dir = normalise(cur_pos - pos_precedent);
				auto tmp_pos = pos_precedent + dir * attr_L->decimal(polygone->index);
				attr_P->valeur(pb, tmp_pos);
				attr_D->valeur(pb, cur_pos - tmp_pos);
			}
		});

		/* calcul des nouvelles positions et vélocités */
		pour_chaque_polygone_ouvert(m_corps,
									[&](Corps const &, Polygone *polygone)
		{
			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
				auto pa = polygone->index_point(i - 1);
				auto pb = polygone->index_point(i);

				auto pos_pa = attr_P->vec3(pa);
				auto vel_pa = ((pos_pa - liste_points->point(pa)) / dt) + 0.9f * (attr_D->vec3(pb) / dt);

				liste_points->point(pa, pos_pa);
				attr_V->valeur(pa, vel_pa);
			}

			/* ajourne le dernier point */
			auto pa = polygone->index_point(polygone->nombre_sommets() - 1);
			liste_points->point(pa, attr_P->vec3(pa));
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpTouffeCheveux : public OperatriceCorps {
public:
	static constexpr auto NOM = "Touffe Cheveux";
	static constexpr auto AIDE = "";

	explicit OpTouffeCheveux(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
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

		if (m_corps.prims()->taille() == 0l) {
			this->ajoute_avertissement("Aucune primitive en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto corps_guide = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps_guide == nullptr) {
			this->ajoute_avertissement("Aucun guide connecté");
			return EXECUTION_ECHOUEE;
		}

		/* poids de l'effet */
		auto const poids = evalue_decimal("poids");
		/* randomisation du poids de l'effet */
		auto const alea_poids = evalue_decimal("aléa_poids");
		/* comment l'effet affecte la base ou le bout
		 *  1 = attraction complète
		 *  0 = attraction nulle
		 * -1 = répulsion complète */
	//	auto const attraction_bout = evalue_decimal("attraction_bout");
	//	auto const attraction_base = evalue_decimal("attraction_base");
		/* controle où la transition entre la base et le bout se fait
		 * 0   = base
		 * 0.5 = milieu
		 * 1   = bout  */
	//	auto const biais_attraction = evalue_decimal("biais_attraction");
		/* défini les limites de l'effet */
	//	auto const distance_min = evalue_decimal("distance_min");
	//	auto const distance_max = evalue_decimal("distance_max");
		/* cause les fibres de tournoier autour de la touffe */
	//	auto const tournoiement = evalue_decimal("tournoiement");
		/* ajout d'un effet de frizz, plus la valeur est élevé, plus les
		 * fibres s'envole loin de la touffe */
	//	auto const envole = evalue_decimal("envole");

		auto points_entree = m_corps.points_pour_ecriture();

		for (auto ip = 0; ip < m_corps.prims()->taille(); ++ip) {
			auto prim = m_corps.prims()->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::OUVERT) {
				continue;
			}

		//	auto pos = points_entree->point(polygone->index_point(0));
		//	auto guide = trouve_courbe_plus_proche(corps_guide->prims(), pos);

			/* guide ne peut-être nul, donc ne vérifie pas la validité */

			/* attire la courbe vers le guide :
			 * - pour chaque point de la courbe, calcul un point équivalent
			 *   le long du guide
			 * - calcul un vecteur d'attraction entre les points
			 * - applique le poids selon les paramètres */
			for (auto j = 0; j < polygone->nombre_sommets(); ++j) {
				auto point_orig = points_entree->point(polygone->index_point(j));
				auto pt_guide = point_orig;//echantillone(guide, dist);

				auto dir = pt_guide - point_orig;

				auto point_final = point_orig + dir * poids * alea_poids/* * dist(rng)*/;
				points_entree->point(polygone->index_point(j), point_final);
			}
		}

		return EXECUTION_REUSSIE;
	}

	Polygone *trouve_courbe_plus_proche(ListePrimitives const *prims, dls::math::vec3f const &pos) const
	{
		auto poly_ret = static_cast<Polygone *>(nullptr);
		auto dist_min = std::numeric_limits<float>::max();
		auto points_entree = m_corps.points_pour_lecture();

		for (auto ip = 0; ip < prims->taille(); ++ip) {
			auto prim = prims->prim(ip);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto polygone = dynamic_cast<Polygone *>(prim);

			if (polygone->type != type_polygone::OUVERT) {
				continue;
			}

			auto point = points_entree->point(polygone->index_point(0));
			auto dist = longueur(point - pos);

			if (dist_min < dist) {
				dist = dist_min;
				poly_ret = polygone;
			}
		}

		return poly_ret;
	}
};

/* ************************************************************************** */

class OpBruitCheveux : public OperatriceCorps {
public:
	static constexpr auto NOM = "Bruit Cheveux";
	static constexpr auto AIDE = "";

	explicit OpBruitCheveux(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_bruit_cheveux.jo";
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

		if (m_corps.prims()->taille() == 0l) {
			this->ajoute_avertissement("Aucune primitive en entrée");
			return EXECUTION_REUSSIE;
		}

		auto prims = m_corps.prims();
		auto points = m_corps.points_pour_ecriture();

		/* quantité de bruit */
		auto const quantite = evalue_decimal("quantité");
		/* modulation de l'effet le long de la fibre */
//		auto const quantite_base = evalue_decimal("quantité_base");
//		auto const quantite_bout = evalue_decimal("quantité_bout");
		/* multiplie la quantité */
		auto const mult = evalue_decimal("mult");
		/* randomise la quantité */
		auto const alea = evalue_decimal("alea");
		/* décalage le long de la fibre */
		auto const decalage = evalue_decimal("décalage");
		/* fréquence du bruit */
//		auto const frequence = evalue_decimal("fréquence");
		/* probabilité/pourcentage qu'une fibre soit affectée */
		auto const prob = evalue_decimal("probabilité");
		/* maintiens la longueur de la fibre */
//		auto const maintiens_longueur = evalue_bool("maintiens_longueur");

		auto gna = GNA();

		for (auto i = 0; i < prims->taille(); ++i) {
			auto prim = prims->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				/* ERREUR ? */
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type != type_polygone::OUVERT) {
				/* ERREUR ? */
				continue;
			}

			if (prob != 1.0f && gna.uniforme(0.0f, 1.0f) >= prob) {
				continue;
			}

			for (auto j = 1; j < poly->nombre_sommets(); ++j) {
				auto idx = poly->index_point(j);
				auto p = points->point(idx);

				auto bruit = quantite * mult * gna.uniforme(-alea, alea);
				auto bruit_x = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;
				auto bruit_y = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;
				auto bruit_z = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;

				p.x += bruit_x;
				p.y += bruit_y;
				p.z += bruit_z;

				points->point(idx, p);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Mélange de deux groupes de cheveux. Idée provenant de
 * "The Hair Motion Compositor : Compositing Dynamic Hair Animations in
 * a Production Environment"
 * http://library.imageworks.com/pdfs/imageworks-library-the-hair-motion-compositor-compositing-dynamic-hair-animations-production-environment.pdf
 */
class OpMelangeCheveux : public OperatriceCorps {
public:
	static constexpr auto NOM = "Mélange Cheveux";
	static constexpr auto AIDE = "Fusionne les points de deux ensemble de cheveux.";

	explicit OpMelangeCheveux(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_melange_cheveux.jo";
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

		if (m_corps.prims()->taille() == 0l) {
			this->ajoute_avertissement("Aucune primitive en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto corps2 = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps2 == nullptr) {
			this->ajoute_avertissement("Aucun corps connecté à la seconde entrée");
			return EXECUTION_ECHOUEE;
		}

		auto prims1 = m_corps.prims();
		auto prims2 = corps2->prims();

		if (prims1->taille() != prims2->taille()) {
			this->ajoute_avertissement("Les corps possèdent des nombres de primitives différents");
			return EXECUTION_ECHOUEE;
		}

		auto points1 = m_corps.points_pour_ecriture();
		auto points2 = corps2->points_pour_lecture();

		auto const fac = evalue_decimal("facteur");

		/* À FAIRE :
		 * - interpolation linéaire/rotationelle (quaternion?)
		 * - définition d'un masque ou d'un groupe pour choisir quels
		 *   cheveux affecter, le masque peut-être des PrimSphères à
		 *   venir
		 * - poids de l'effet selon la position sur la courbe (courbe
		 *   de mappage), pour plus affecter le bout ou la base
		 */

		for (auto i = 0; i < prims1->taille(); ++i) {
			auto prim1 = prims1->prim(i);
			auto prim2 = prims2->prim(i);

			if (prim1->type_prim() != prim2->type_prim()) {
				this->ajoute_avertissement("Les types des primitives sont différents !");
				return EXECUTION_ECHOUEE;
			}

			if (prim1->type_prim() != type_primitive::POLYGONE) {
				/* ERREUR ? */
				continue;
			}

			auto poly1 = dynamic_cast<Polygone *>(prim1);
			auto poly2 = dynamic_cast<Polygone *>(prim2);

			if (poly1->nombre_sommets() != poly2->nombre_sommets()) {
				this->ajoute_avertissement("Le nombre de points divergent sur les courbes");
				return EXECUTION_ECHOUEE;
			}

			for (auto j = 0; j < poly1->nombre_sommets(); ++j) {
				auto p1 = points1->point(poly1->index_point(j));
				auto p2 = points2->point(poly2->index_point(j));

				auto p = p1 * fac + p2 * (1.0f - fac);

				points1->point(poly1->index_point(j), p);
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
	usine.enregistre_type(cree_desc<OpTouffeCheveux>());
	usine.enregistre_type(cree_desc<OpBruitCheveux>());
	usine.enregistre_type(cree_desc<OpMelangeCheveux>());
}

#pragma clang diagnostic pop
