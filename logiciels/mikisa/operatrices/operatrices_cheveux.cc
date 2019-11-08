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
#include "biblinternes/phys/collision.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/contexte_evaluation.hh"
#include "coeur/delegue_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/attribut.h"
#include "corps/corps.h"
#include "corps/iteration_corps.hh"

#include "arbre_octernaire.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/**
 * Système de cheveux. Idées provenant de différentes sources (SPI, Yeti,
 * Ornatrix, Presto, Hair Farm, DreamWorks).
 *
 * Opératrices/fonctions à considérer :
 *
 * De Yeti :
 * Grow      | crée des cheveux selon des particules/points en entrées
 * Instance  | instancie une géométrie par point/cheveux
 * Scatter   | disperse des points sur une surface d'entrée
 *
 * Attribute | ajoute ou promeut des attributs sur les cheveux
 * Blend     | mélange 2 ensembles de cheveux
 * Displace  | pousse les cheveux dans une direction
 * Group     | crée des groupes pour les cheveux
 * Merge     | fusionne deux flux de cheveux dans un seul
 * Shader    | ajoute des attributs qui seront passé au moteur de rendu
 * Switch    | controle le flux du graphe
 * Texture   | ajoute des attributs selon une texture
 * Transform | applique une transformation aux éléments, translation sur $N est similaire à dispalice
 *
 * Bend      | attire ou répulse les courbes depuis la surface ou un plan
 * Clumping  | crée des touffes, en attirant les courbes d'un système vers celles d'un autre
 * Comb      | applique la forme d'une coupe aux cheveux
 * Curl      | crée des boucles dans les cheveux (aussi appelé Kink, Wave)
 * Direction | ajuste la direction des cheveux
 * Guide     | déformes les cheveux selon des guides
 * Motion    | ajoute des mouvements subtiles sans avoir à créer de simulations (vent)
 * Scraggle  | ajoute du bruit controlé le long des cheveux
 * Width     | controle le rayon des segments, pour chaque point | YETI, Ornatrix
 *
 * De Ornatrix :
 * Guides from Objet
 * Guides from Surface
 * Guides from Mesh
 * Hair from Guides
 * Hair Propagation | crée des cheveux sur des cheveux
 * Ground Strand    | crée des informations sur où les cheveux sont pour les faire suivre une surface (FurAttach)
 * Braid Pattern
 * Weaver Pattern
 * Edit Guides
 * Surface Comb     | crée des directions sur le scalp suivies par les cheveux
 * Frizz            | crée du bruit le long des cheveux
 * Gravity          | applique une force de gravité sur les cheveux (hors simulation)
 * Simmetry         | fait une copie mirroir des cheveux selon un plan
 * Strand Detail    | ajoute plus de points sur chaque segments pour mieux les approximer
 * Rotate Strand    | oriente les cheveux autour de leur axe
 * Strand Multiplier | crée des enfants autour des cheveux existants
 * Normalize Strands | utilise une direction normalisée pour déméler les cheveux
 *
 * De DreamWorks :
 * FurAttach
 * FurCollide
 * FurSampler
 * FurColliderDistance
 * Region of Influence
 * FollicleSampler (quickly get a spatially uniformly sampled set of follicles/curves)
 * CurveWind (procedural wind with gusts and wind shielding)
 * CurveJiggle (fast controllable jiggle/secondary motion)
 *
 * « Skunk: DreamWorks Fur Simulation System »
 * https://research.dreamworks.com/wp-content/uploads/2019/10/xtalk_skunk_abstract.pdf
 *
 * De SPI :
 * Wave (make hair wavy)
 * Length
 * GeoInstance
 * Bend
 * Orientation/Rotate (rotate around the base)
 * Wind
 * Push Away From Surface
 * Clumping (bunches a group of hairs together)
 * Clumping of wet hair (option dans clumping par groupe ou mappe)
 * Magnet (allow for hair to be deformed by external geometry, e.g. belts or straps)
 * Reshape (allow for hair to be deformed by external geometry, e.g. belts or straps)
 * Python (write custom code)
 * De SPI hair compositor :
 * Blend (mix 2 or more hair animations, weighted along the length, all or subsets, blend "balls" used as masks optionnaly)
 * Dynamic Hair Solver (takes a goal animation as input and produces a physically believable motion as output)
 * Offset nodes allow the artist to modify an animation using a simplified controller which follows along with the incoming animation
 *     controller can be a lower-resolution curve, "offset sock" (like NURBS) can be used instead for groups
 *
 * Possibilité de charge un cache disc.
 */

/* ************************************************************************** */

struct DonneesCollesion {
	dls::math::vec3f normal{};
	REMBOURRE(4);
	long idx_courbe = 0;
	long idx_segment = 0;
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

	OperatriceCreationCourbes(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps_particules = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_particules, true, false)) {
			return res_exec::ECHOUEE;
		}

		auto const liste_points = corps_particules->points_pour_lecture();

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
		 *
		 * méthode :
		 * ta = aléa(min, max)
		 * tb = aléa(influence)
		 * t = ta * (1 - biais) + tb * biais
		 */
//		auto const biais = (evalue_decimal("biais") + 1.0f) * 0.5f;
//		auto inf_min = taille_min;
//		auto inf_max = taille_max;

//		if (biais < 0.5f) {
//			inf_max = (taille_min + taille_max) * 0.5f;
//		}
//		else if (biais > 0.5f) {
//			inf_min = (taille_min + taille_max) * 0.5f;
//		}

		int style_creation;

		if (chaine_style_creation == "segments") {
			style_creation = STYLE_CREATION_SEGMENT;
		}
		else {
			style_creation = STYLE_CREATION_LONGUEUR;
		}

		Attribut const *attr_N = nullptr;
		int direction;

		if (chaine_direction == "normal") {
			direction = DIRECTION_NORMAL;

			attr_N = corps_particules->attribut("N");

			if (attr_N == nullptr) {
				ajoute_avertissement("Aucun attribut normal (N) trouvé sur les particules !");
				return res_exec::ECHOUEE;
			}

			if (attr_N->portee != portee_attr::POINT) {
				ajoute_avertissement("L'attribut normal (N) n'est pas sur les points !");
				return res_exec::ECHOUEE;
			}
		}
		else {
			direction = DIRECTION_PERSONALISEE;
		}

		auto gna = GNA();

		auto attr_L = m_corps.ajoute_attribut("longueur", type_attribut::R32, 1, portee_attr::PRIMITIVE, true);
		attr_L->reserve(liste_points.taille());

		auto points_sortie = m_corps.points_pour_ecriture();

		for (auto i = 0; i < liste_points.taille(); ++i) {
//			auto const ta = gna.uniforme(taille_min, taille_max);
//			auto const tb = gna.uniforme(inf_min, inf_max);
//			auto const taille_courbe = (tb * (1.0f - biais) + ta * biais) * multiplication_taille;
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
				extrait(attr_N->r32(i), normal);
			}

			auto pos = liste_points.point_local(i);

			auto index_npoint = points_sortie.ajoute_point(pos.x, pos.y, pos.z);

			auto polygone = m_corps.ajoute_polygone(type_polygone::OUVERT, nombre_segment + 1);
			m_corps.ajoute_sommet(polygone, index_npoint);

			for (long j = 0; j < nombre_segment; ++j) {
				pos += (taille_segment * normal);

				index_npoint = points_sortie.ajoute_point(pos.x, pos.y, pos.z);
				m_corps.ajoute_sommet(polygone, index_npoint);
			}

			attr_L->r32(polygone->index)[0] = taille_segment;
		}

		return res_exec::REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("biais") == nullptr) {
			this->ajoute_propriete("biais", danjo::TypePropriete::DECIMAL, 0.0f);
		}
	}
};

/* ************************************************************************** */

static void traverse_arbre(
		arbre_octernaire::noeud const *racine,
		dls::phys::rayond const &rayon,
		dls::tableau<arbre_octernaire::noeud const *> &noeuds)
{
	if (racine->est_feuille) {
		noeuds.pousse(racine);
		return;
	}

	for (auto i = 0; i < 8; ++i) {
		auto enfant = racine->enfants[i];

		if (enfant == nullptr) {
			continue;
		}

		auto mind = dls::math::converti_type_point<double>(enfant->limites.min);
		auto maxd = dls::math::converti_type_point<double>(enfant->limites.max);

		if (dls::phys::entresection_rapide_min_max(rayon, mind, maxd) > -0.5) {
			traverse_arbre(enfant, rayon, noeuds);
		}
	}
}

static auto entresecte_prim(Corps const &corps, Primitive *prim, const dls::phys::rayond &r)
{
	auto entresection = dls::phys::esectd{};

	if (prim->type_prim() != type_primitive::POLYGONE) {
		return entresection;
	}

	auto poly = dynamic_cast<Polygone *>(prim);
	auto attr_N = corps.attribut("N");

	if (poly->type != type_polygone::FERME) {
		return entresection;
	}

	auto points = corps.points_pour_lecture();

	for (auto j = 2; j < poly->nombre_sommets(); ++j) {
		auto i0 = poly->index_point(0);
		auto i1 = poly->index_point(j - 1);
		auto i2 = poly->index_point(j);

		auto const &v0 = points.point_monde(i0);
		auto const &v1 = points.point_monde(i1);
		auto const &v2 = points.point_monde(i2);

		auto const &v0_d = dls::math::converti_type_point<double>(v0);
		auto const &v1_d = dls::math::converti_type_point<double>(v1);
		auto const &v2_d = dls::math::converti_type_point<double>(v2);

		auto dist = 1000.0;
		auto ud = 0.0;
		auto vd = 0.0;

		if (entresecte_triangle(v0_d, v1_d, v2_d, r, dist, &ud, &vd)) {
			entresection.touche = true;
			entresection.point = r.origine + dist * r.direction;
			entresection.distance = dist;

			auto n0 = dls::math::vec3f();
			auto n1 = dls::math::vec3f();
			auto n2 = dls::math::vec3f();

			if (attr_N->portee == portee_attr::POINT) {
				extrait(attr_N->r32(i0), n0);
				extrait(attr_N->r32(i1), n1);
				extrait(attr_N->r32(i2), n2);
			}
			else if (attr_N->portee == portee_attr::PRIMITIVE) {
				extrait(attr_N->r32(prim->index), n0);
				n1 = n0;
				n2 = n0;
			}

			auto u = static_cast<float>(ud);
			auto v = static_cast<float>(vd);
			auto w = 1.0f - u - v;
			auto N = w * n0 + u * n1 + v * n2;
			entresection.normal = dls::math::converti_type<double>(N);

			break;
		}
	}

	return entresection;
}

/**
 * Tentative d'implémentation de
 * « FurCollide: Fast, Robust, and Controllable Fur Collisions with Meshes »
 * https://research.dreamworks.com/wp-content/uploads/2018/07/55-0210-somasundaram-Edited.pdf
 */
class OperatriceCollisionCheveux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Collision Cheveux";
	static constexpr auto AIDE = "Collèse des cheveux avec un maillage.";

	OperatriceCollisionCheveux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{}

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

		/* obtiens le maillage de collision */
		auto const maillage_collision = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, maillage_collision, true, true, 1)) {
			return res_exec::ECHOUEE;
		}

		/* le papier utilise une version triangulée du maillage dont chaque
		 * triangle est stocké dans un arbre octernaire, cette triangulation est
		 * ignorée ici */
		auto delegue = delegue_arbre_octernaire(*maillage_collision);
		auto arbre = arbre_octernaire::construit(delegue);

		/* identifie les courbes dont la boite englobante chevauche celle du
		 * maillage de collision
		 * NOTE : pour l'instant toutes les courbes sont considérées
		 */

		auto donnees_collesions = dls::tableau<DonneesCollesion>();

		pour_chaque_polygone_ouvert(m_corps, [&](Corps const &corps_courbe, Polygone *poly)
		{
			/* chaque segment de ces courbes sont testés pour une collision avec
			 * les primitives, jusqu'à ce qu'on trouve une collision :
			 * 1. d'abord les octants de l'arbre dans lesquels se trouvent les
			 * segments de la courbe sont identifiés en utilisant un test de
			 * chevauchement segment/boite englobante
			 */
			dls::tableau<arbre_octernaire::noeud const *> noeuds;

			auto points_courbes = corps_courbe.points_pour_lecture();

			for (auto i = 0; i < poly->nombre_segments(); ++i) {
				noeuds.efface();

				auto p0 = points_courbes.point_monde(poly->index_point(i));
				auto p1 = points_courbes.point_monde(poly->index_point(i + 1));

				auto rayon = dls::phys::rayond();
				rayon.origine = dls::math::converti_type_point<double>(p0);
				auto direction = normalise(p1 - p0);
				rayon.direction = dls::math::converti_type<double>(direction);
				dls::phys::calcul_direction_inverse(rayon);

				traverse_arbre(arbre.racine(), rayon, noeuds);

				//std::cerr << "Courbe " << poly->index << ", chevauche " << noeuds.taille() << " noeuds\n";

				/* 2. ensuite pour les primitives se trouvant dans les octants,
				 * un autre test de chevauchement est performé entre le segment
				 * et la boite englobante de la primitive
				 */
				for (auto noeud_hbe : noeuds) {
					for (auto idx : noeud_hbe->refs) {
						auto lmt = delegue.calcule_limites(idx);
						auto mind = dls::math::converti_type_point<double>(lmt.min);
						auto maxd = dls::math::converti_type_point<double>(lmt.max);

						if (dls::phys::entresection_rapide_min_max(rayon, mind, maxd) < 0.0) {
							continue;
						}

						/* 3. si le test précité passe, une entresection
						 * triangle/segment est performée
						 *
						 * le normal au point de collésion, et le segment
						 * collésé sont stocké
						 */
						auto prim = maillage_collision->prims()->prim(idx);

						auto esect = entresecte_prim(*maillage_collision, prim, rayon);

						if (!esect.touche) {
							continue;
						}

						auto dist = dls::math::longueur(p1 - p0);

						if (esect.distance > static_cast<double>(dist)) {
							continue;
						}

						auto donnees_collesion = DonneesCollesion();
						donnees_collesion.normal = dls::math::converti_type<float>(esect.normal);
						donnees_collesion.idx_courbe = idx;
						donnees_collesion.idx_segment = i;

						donnees_collesions.pousse(donnees_collesion);

						return;
					}
				}
			}
		});

		//std::cerr << "Il y a " << donnees_collesions.taille() << " collisions détectées\n";

		/* À FAIRE : réponse collision */

		return res_exec::REUSSIE;
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
class OperatriceMasseRessort final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Masse Ressort";
	static constexpr auto AIDE = "";

	OperatriceMasseRessort(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (m_corps.prims()->taille() == 0l) {
			return res_exec::REUSSIE;
		}

		auto attr_L = m_corps.attribut("longueur");

		if (attr_L == nullptr) {
			ajoute_avertissement("Aucun attribut de longueur trouvé !");
			return res_exec::ECHOUEE;
		}

		auto attr_V = m_corps.ajoute_attribut("mr_V", type_attribut::R32, 3, portee_attr::POINT);
		auto attr_P = m_corps.ajoute_attribut("mr_P", type_attribut::R32, 3, portee_attr::POINT);
		auto attr_D = m_corps.ajoute_attribut("mr_D", type_attribut::R32, 3, portee_attr::POINT);

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
			assigne(attr_P->r32(polygone->index_point(0)), liste_points.point_local(polygone->index_point(0)));

			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
				auto const pos_precedent = liste_points.point_local(polygone->index_point(i - 1));
				auto pos = liste_points.point_local(polygone->index_point(i));
				auto vel = dls::math::vec3f();
				extrait(attr_V->r32(polygone->index_point(i)), vel);

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

				assigne(attr_P->r32(polygone->index_point(i)), pos);
				assigne(attr_V->r32(polygone->index_point(i)), vel);
			}
		});

		/* résolution des contraintes */
		pour_chaque_polygone_ouvert(m_corps,
									[&](Corps const &, Polygone *polygone)
		{
			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
				auto pa = polygone->index_point(i - 1);
				auto pb = polygone->index_point(i);

				auto pos_precedent = dls::math::vec3f();
				extrait(attr_P->r32(pa), pos_precedent);
				auto cur_pos = dls::math::vec3f();
				extrait(attr_P->r32(pb), cur_pos);
				auto dir = normalise(cur_pos - pos_precedent);
				auto tmp_pos = pos_precedent + dir * attr_L->r32(polygone->index)[0];
				assigne(attr_P->r32(pb), tmp_pos);
				assigne(attr_D->r32(pb), cur_pos - tmp_pos);
			}
		});

		/* calcul des nouvelles positions et vélocités */
		pour_chaque_polygone_ouvert(m_corps,
									[&](Corps const &, Polygone *polygone)
		{
			for (long i = 1; i < polygone->nombre_sommets(); ++i) {
				auto pa = polygone->index_point(i - 1);
				auto pb = polygone->index_point(i);

				auto pos_pa = dls::math::vec3f();
				extrait(attr_P->r32(pa), pos_pa);

				auto d_pa = dls::math::vec3f();
				extrait(attr_D->r32(pb), pos_pa);

				auto vel_pa = ((pos_pa - liste_points.point_local(pa)) / dt) + 0.9f * (d_pa / dt);

				liste_points.point(pa, pos_pa);
				assigne(attr_V->r32(pa), vel_pa);
			}

			/* ajourne le dernier point */
			auto pa = polygone->index_point(polygone->nombre_sommets() - 1);
			auto pos_pa = dls::math::vec3f();
			extrait(attr_P->r32(pa), pos_pa);
			liste_points.point(pa, pos_pa);
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpTouffeCheveux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Touffe Cheveux";
	static constexpr auto AIDE = "";

	OpTouffeCheveux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto corps_guide = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_guide, true, true, 1)) {
			return res_exec::ECHOUEE;
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
				auto point_orig = points_entree.point_local(polygone->index_point(j));
				auto pt_guide = point_orig;//echantillone(guide, dist);

				auto dir = pt_guide - point_orig;

				auto point_final = point_orig + dir * poids * alea_poids/* * dist(rng)*/;
				points_entree.point(polygone->index_point(j), point_final);
			}
		}

		return res_exec::REUSSIE;
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

			auto point = points_entree.point_local(polygone->index_point(0));
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

class OpBruitCheveux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Bruit Cheveux";
	static constexpr auto AIDE = "";

	OpBruitCheveux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
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
				auto p = points.point_local(idx);

				auto bruit = quantite * mult * gna.uniforme(-alea, alea);
				auto bruit_x = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;
				auto bruit_y = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;
				auto bruit_z = (gna.uniforme(-0.5f, 0.5f) + decalage) * bruit;

				p.x += bruit_x;
				p.y += bruit_y;
				p.z += bruit_z;

				points.point(idx, p);
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Mélange de deux groupes de cheveux. Idée provenant de
 * "The Hair Motion Compositor : Compositing Dynamic Hair Animations in
 * a Production Environment"
 * http://library.imageworks.com/pdfs/imageworks-library-the-hair-motion-compositor-compositing-dynamic-hair-animations-production-environment.pdf
 */
class OpMelangeCheveux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Mélange Cheveux";
	static constexpr auto AIDE = "Fusionne les points de deux ensemble de cheveux.";

	OpMelangeCheveux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto corps2 = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps2, true, true, 1)) {
			return res_exec::ECHOUEE;
		}

		auto prims1 = m_corps.prims();
		auto prims2 = corps2->prims();

		if (prims1->taille() != prims2->taille()) {
			this->ajoute_avertissement("Les corps possèdent des nombres de primitives différents");
			return res_exec::ECHOUEE;
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
				return res_exec::ECHOUEE;
			}

			if (prim1->type_prim() != type_primitive::POLYGONE) {
				/* ERREUR ? */
				continue;
			}

			auto poly1 = dynamic_cast<Polygone *>(prim1);
			auto poly2 = dynamic_cast<Polygone *>(prim2);

			if (poly1->nombre_sommets() != poly2->nombre_sommets()) {
				this->ajoute_avertissement("Le nombre de points divergent sur les courbes");
				return res_exec::ECHOUEE;
			}

			for (auto j = 0; j < poly1->nombre_sommets(); ++j) {
				auto p1 = points1.point_local(poly1->index_point(j));
				auto p2 = points2.point_local(poly2->index_point(j));

				auto p = p1 * fac + p2 * (1.0f - fac);

				points1.point(poly1->index_point(j), p);
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpSimCheveux final : public OperatriceCorps {
	struct Spring {
		unsigned p1;
		unsigned p2;
		float rest_length;
		float stiffness;  // could be named spring_constant

		// Contructors
		Spring(unsigned point_1,
			   unsigned point_2,
			   float rest_length_ = 1.f,
			   float spring_constant = 25.f)
			: p1(point_1),
			  p2(point_2),
			  rest_length(rest_length_),
			  stiffness(spring_constant) {}
	};

	float hair_length = 2.0f;
	int num_hairs = 400;
	int num_hair_points = 16;
	float curviness = 1.5f;
	float curliness = 1.5f;
	float delta_time = 0.01f;
	int current_frame = 1;

	dls::math::vec3f position_sphere = dls::math::vec3f(0.0f);

	using tableau_vec3 = dls::tableau<dls::math::vec3f>;
	dls::tableau<tableau_vec3> velocities{};
	dls::tableau<tableau_vec3> forces{};

	using tableau_spring = dls::tableau<Spring *>;
	dls::tableau<tableau_spring> springs{};

public:
	static constexpr auto NOM = "Simulation Cheveux";
	static constexpr auto AIDE = "";

	OpSimCheveux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{}

	~OpSimCheveux()
	{
		for (auto tablb_spring : springs) {
			for (auto spring : tablb_spring) {
				delete spring;
			}
		}
	}

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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto const rayon_sphere = 1.0f;

		if (contexte.temps_courant == contexte.temps_debut) {
			position_sphere = dls::math::vec3f(0.0f);
			m_corps.reinitialise();

			for (auto tablb_spring : springs) {
				for (auto spring : tablb_spring) {
					delete spring;
				}
			}

			springs.efface();
			velocities.efface();
			forces.efface();

			auto gna = GNA();

			float segment_length = (hair_length / (static_cast<float>(num_hair_points) - 1.f));

			auto points = m_corps.points_pour_ecriture();

			for (int h = 0; h < num_hairs; h++) {
				auto poly = m_corps.ajoute_polygone(type_polygone::OUVERT, num_hair_points);

				auto point_base = normalise(echantillone_sphere<dls::math::vec3f>(gna));
				auto normal = point_base;

				tableau_vec3 point_velocities;
				tableau_vec3 point_forces;

				for (int p = 0; p < (num_hair_points); p++) {
					point_velocities.pousse(dls::math::vec3f(0.0f));
					point_forces.pousse(dls::math::vec3f(0.0f));

					auto idx_point = points.ajoute_point(point_base);
					m_corps.ajoute_sommet(poly, idx_point);

					point_base += normal * segment_length;
				}

				velocities.pousse(point_velocities);
				forces.pousse(point_forces);
			}

			for (auto i = 1u ; i < static_cast<unsigned>(num_hair_points); i++) {
				dls::tableau<Spring*> springs_attached_to_point;

				springs_attached_to_point.pousse(
							new Spring(i - 1, i, segment_length, 20 * static_cast<float>(static_cast<unsigned>(num_hair_points) - i)));
				if (i > 1) {
					springs_attached_to_point.pousse(
								new Spring(i - 2, i, segment_length * curviness , 35 * static_cast<float>((static_cast<unsigned>(num_hair_points) - i))));
					// if (i > 2) {
					//   springs_attached_to_point.push_back(
					//       new Spring(i - 3, i, segment_length * curliness , 60 * (num_hair_points - i)));
					// }
				}

				springs.pousse(springs_attached_to_point);
			}
		}
		else {
			auto prims = m_corps.prims();
			auto points = m_corps.points_pour_ecriture();

//			if (contexte.temps_courant < 100) {
//				position_sphere += dls::math::vec3f(0.0f, 0.1f, 0.0f);
//			}

			for (auto i = 0; i < num_hairs; ++i) {
				auto cvs = dynamic_cast<Polygone *>(prims->prim(i));

//				if (contexte.temps_courant < 100) {
//					auto point = points->point(cvs->index_point(0));
//					point += dls::math::vec3f(0.0f, 0.1f, 0.0f);
//					points->point(cvs->index_point(0), point);
//				}

				for (int p = 1; p < num_hair_points; p++) {
					// cout << " and point " << p << "\n";
					// cvs[p] += MPoint(p, 0, 0);

					forces[i][p] = dls::math::vec3f(0.0f); //reset forces
					auto current_position = points.point_local(cvs->index_point(p));

					// For every spring s whos endpoint is p
					for (int s = 0; s < springs[p - 1].taille(); s++) {
						// cout << "\tspring " << s << "\n";
						// INTERNAL FORCES
						dls::math::vec3f prev_position; //will be related to the spring point closer to the root
						int prev_point_id   = static_cast<int>(springs[p-1][s]->p1);
						float stiffness     = springs[p-1][s]->stiffness;
						float rest_length   = springs[p-1][s]->rest_length;
						prev_position = points.point_local(cvs->index_point(prev_point_id));

						auto spring_vector = prev_position - current_position;
						float current_length = longueur(spring_vector);
						// Hooke's law
						auto spring_force = stiffness * ( current_length / rest_length - 1.0f ) * (spring_vector / current_length);

						if (prev_point_id > 0) {
							forces[i][prev_point_id] += -spring_force;
						}

						forces[i][p] += spring_force;
					}

					// EXTERNAL FORCES
					forces[i][p] += dls::math::vec3f(0.0f, -9.82f, 0.0f); // assuming mass=1
					// Damping force
					forces[i][p] -= 0.9f * velocities[i][p];
				}
			}

			// Velocities and positions
			   // For every strand i
			for (int i = 0 ; i < num_hairs; i++ ) {
				auto cvs = dynamic_cast<Polygone *>(prims->prim(i));

				// for every point in each hair (excluding root point)
				for (int p = 1; p < num_hair_points; p++) {
					// cout << "\t\t\tVel_prev: " << velocities[p];
					velocities[i][p] += delta_time * forces[i][p] / 1.0f; // assumes mass = 1
					// cout << "\n\t\t\t\tVel_post: " << velocities[p] << "\n";
					auto prev_position = points.point_local(cvs->index_point(p)); // TODO: this is the error! Reason that it doesn't update
					// cout << "\t\t\tPos_prev: " << prev_position;
					auto current_velocity = velocities[i][p];
					auto new_position = prev_position + delta_time * current_velocity;

					{ // FOR COLLISION!!!!

						auto offset = new_position - position_sphere;
						auto normal = normalise(offset);
						auto magnitude = longueur(offset);

						{ // SIGNED DISTANCE METHOD - taken from
							// www.cs.ubc.ca/~rbridson/docs/cloth2003.pdf
							// Anticipates collision and tries to adjust velocities correctly
							float signed_distance = magnitude - rayon_sphere;
							float anticipated_signed_distance =
									signed_distance +
									delta_time * produit_scalaire(current_velocity /*- sphere_velocity*/,
									normal); // the sphere velocity should really be there if it is animated!

							// this all assumes that the sphere doesn't move!
							// Otherwise sphere_velocity is needed
							if (anticipated_signed_distance < 0) {
								float normal_vel = produit_scalaire(current_velocity, normal);
								auto tangential_vel = current_velocity - normal_vel * normal;
								float new_normal_vel =
										normal_vel - anticipated_signed_distance / delta_time;
								velocities[i][p] = new_normal_vel * normal + tangential_vel;

								// TODO: add the friction
							}
						}
					}

					points.point(cvs->index_point(p), new_position);
					// cout << "\n\t\t\t\tPos_post: " << new_position << "\n";
				}
			}
		}

		return res_exec::REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_cheveux(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationCourbes>());
	usine.enregistre_type(cree_desc<OperatriceMasseRessort>());
	usine.enregistre_type(cree_desc<OperatriceCollisionCheveux>());
	usine.enregistre_type(cree_desc<OpTouffeCheveux>());
	usine.enregistre_type(cree_desc<OpBruitCheveux>());
	usine.enregistre_type(cree_desc<OpMelangeCheveux>());
	usine.enregistre_type(cree_desc<OpSimCheveux>());
}

#pragma clang diagnostic pop
