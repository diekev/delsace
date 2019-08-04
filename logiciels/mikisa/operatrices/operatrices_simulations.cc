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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/moultfilage/synchronise.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_simulation.hh"
#include "coeur/usine_operatrice.h"

#include "corps/groupes.h"

#include "delegue_hbe.hh"
#include "ocean.hh"

#include "biblinternes/math/complexe.hh"

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
		INUTILISE(contexte);
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		if (m_graphe_parent.donnees.est_vide()) {
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

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attrf = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		auto gravite = evalue_vecteur("gravité", contexte.temps_courant);

		/* À FAIRE : f = m * a => multiplier par la masse? */
		boucle_parallele(
					tbb::blocked_range<long>(0, nombre_points),
					[&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				attrf->valeur(i, gravite);
			}
		});

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

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attrf = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		auto direction = evalue_vecteur("direction", contexte.temps_courant);
		auto amplitude = evalue_decimal("amplitude", contexte.temps_courant);

		auto force_max = direction * amplitude;

		/* À FAIRE : vent
		 * - intégration des forces dans le désordre (gravité après vent) par
		 *   exemple en utilisant un attribut extra.
		 * - turbulence
		 */
		boucle_parallele(
					tbb::blocked_range<long>(0, nombre_points),
					[&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto force = attrf->vec3(i);

				for (size_t j = 0; j < 3; ++j) {
					force[j] = std::min(force_max[j], force[j] + force_max[j]);
				}

				attrf->valeur(i, force);
			}
		});

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

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attr_P = m_corps.ajoute_attribut("pos_pre", type_attribut::VEC3, portee_attr::POINT);
		auto attrf = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);

		/* À FAIRE : passe le temps par image en paramètre. */
		auto const temps_par_image = 1.0f / 24.0f;
		/* À FAIRE : masse comme propriété des particules */
		auto const masse = 1.0f; // evalfloat("masse");
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

		liste_points->detache();

		boucle_parallele(
					tbb::blocked_range<long>(0, nombre_points),
					[&](tbb::blocked_range<long> const &plage)
		{
			for (long i = plage.begin(); i < plage.end(); ++i) {
				auto desactivee = attr_desactiv->ent8(i);

				if (desactivee == 1) {
					continue;
				}

				auto pos = liste_points->point(i);

				/* a = f / m */
				auto const acceleration = attrf->vec3(i) * masse_inverse;

				/* velocite = acceleration * temp_par_image + velocite */
				auto velocite = attr_V->vec3(i) + acceleration * temps_par_image;

				/* position = velocite * temps_par_image + position */
				auto npos = pos + velocite * temps_par_image;

				liste_points->point(i, npos);
				attr_V->valeur(i, velocite);
				attr_P->valeur(i, pos);
				attrf->valeur(i, dls::math::vec3f(0.0f));
			}
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

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
		auto corps_collision = entree(1)->requiers_corps(contexte, donnees_aval);

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
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();

		auto const elasticite = evalue_decimal("élasticité", contexte.temps_courant);
		/* À FAIRE : rayon comme propriété des particules */
		auto const rayon = evalue_decimal("rayon", contexte.temps_courant);

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
			dls::flux_chaine ss;
			ss << "Opération '" << chaine_reponse << "' inconnue\n";
			this->ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("maillage vers volume");

		auto groupe = m_corps.ajoute_groupe_point("collision");
		groupe->reinitialise();

		auto groupe_sync = dls::synchronise<GroupePoint *>();
		groupe_sync = groupe;

		auto attr_desactiv = m_corps.ajoute_attribut("part_desactiv",
												  type_attribut::ENT8,
												  portee_attr::POINT);

		auto delegue_prims = DeleguePrim(*corps_collision);
		auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

		m_corps.points()->detache();

		boucle_parallele(tbb::blocked_range<long>(0, nombre_points),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (long i = plage.begin(); i < plage.end(); ++i) {
				auto pos_cou = liste_points->point(i);
				auto vel = attr_V->vec3(i);
				auto pos_pre = attr_P->vec3(i);
				auto desactivee = attr_desactiv->ent8(i);

				if (desactivee == 1) {
					continue;
				}

				/* Calcul la position en espace objet. */
				auto pos_monde = m_corps.transformation(dls::math::point3d(pos_pre));

				auto rayon_part = dls::phys::rayond{};
				rayon_part.origine = pos_monde;

				auto dir = normalise(pos_cou - pos_pre);
				rayon_part.direction.x = static_cast<double>(dir.x);
				rayon_part.direction.y = static_cast<double>(dir.y);
				rayon_part.direction.z = static_cast<double>(dir.z);

				calcul_direction_inverse(rayon_part);

				for (size_t j = 0; j < 3; ++j) {
					rayon_part.direction_inverse[j] = 1.0 / rayon_part.direction[j];
				}

				auto accumulatrice = AccumulatriceTraverse(rayon_part.origine);
				traverse(arbre_hbe, delegue_prims, rayon_part, accumulatrice);

				auto const &esect = accumulatrice.intersection();

				if (!esect.touche) {
					continue;
				}

				if (esect.distance > static_cast<double>(rayon)) {
					continue;
				}

				auto index_prim = esect.idx;

				groupe_sync.accede_ecriture([&](GroupePoint *groupe_point)
				{
					groupe_point->ajoute_point(static_cast<size_t>(i));
				});

				switch (reponse) {
					case rep_collision::RIEN:
					{
						break;
					}
					case rep_collision::REBONDIS:
					{
						auto prim = prims_collision->prim(index_prim);
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
						attr_V->valeur(i, vel);
						break;
					}
					case rep_collision::COLLE:
					{
						pos_cou.x = static_cast<float>(esect.point.x);
						pos_cou.y = static_cast<float>(esect.point.y);
						pos_cou.z = static_cast<float>(esect.point.z);

						liste_points->point(i, pos_cou);
						attr_V->valeur(i, dls::math::vec3f(0.0f));
						attr_desactiv->valeur(i, char(1));
						break;
					}
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			auto total = static_cast<float>(nombre_points);

			chef->indique_progression_parallele(delta / total * 100.0f);
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class BarnesHutSummation {
public:
	struct Particule {
		dls::math::vec3f position = dls::math::vec3f(0.0f);
		float mass = 0.0f;

		Particule() = default;

		Particule(dls::math::vec3f const &pos, float m)
			: position(pos)
			, mass(m)
		{}

		Particule operator+(const Particule &o) const
		{
			Particule ret;
			ret.mass = mass + o.mass;
			ret.position =
					(position * mass + o.position * o.mass) * (1.0f / ret.mass);
			return ret;
		}
	};

protected:
	struct Node {
		Particule p{};
		int children[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // We do not use pointer here to save memory bandwidth(64
		// bit v.s. 32 bit)
		// There are so many ways to optimize and I'll consider them later...
		dls::math::vec3i bounds[2];  // À FAIRE: this is quite brute-force...

		Node() = default;

		bool is_leaf()
		{
			for (int i = 0; i < 8; i++) {
				if (children[i] != 0) {
					return false;
				}
			}

			return p.mass > 0;
		}
	};

	float resolution = 0.0f;
	float inv_resolution = 0.0f;
	int total_levels = 0;
	int margin = 0;

	dls::tableau<Node> nodes{};
	int node_end = 0;
	dls::math::vec3f lower_corner{};

	dls::math::vec3i coordonnees_grille(const dls::math::vec3f &position)
	{
		dls::math::vec3i u;
		dls::math::vec3f t = (position - lower_corner) * inv_resolution;
		for (size_t i = 0; i < 3; i++) {
			u[i] = int(t[i]);
		}
		return u;
	}

	int calcul_index_enfant(const dls::math::vec3i &u, int level)
	{
		int ret = 0;
		for (size_t i = 0; i < 3; i++) {
			ret += ((u[i] >> (total_levels - level - 1)) & 1) << i;
		}
		return ret;
	}

	void sommarise(int t)
	{
		auto &node = nodes[t];

		if (node.is_leaf()) {
			auto u = coordonnees_grille(node.p.position);
			node.bounds[0] = u - dls::math::vec3i(margin);
			node.bounds[1] = u + dls::math::vec3i(margin);
			return;
		}

		float mass = 0.0f;
		dls::math::vec3f total_position(0.0f);
		node.bounds[0] = dls::math::vec3i(std::numeric_limits<int>::max());
		node.bounds[1] = dls::math::vec3i(std::numeric_limits<int>::min());

		for (int c = 0; c < 8; c++) {
			if (node.children[c]) {
				sommarise(nodes[t].children[c]);
				auto const &ch = nodes[node.children[c]];
				mass += ch.p.mass;
				total_position += ch.p.mass * ch.p.position;

				for (size_t i = 0; i < 3; i++) {
					node.bounds[0][i] = std::min(node.bounds[0][i], ch.bounds[0][i]);
					node.bounds[1][i] = std::max(node.bounds[1][i], ch.bounds[1][i]);
				}
			}
		}

		total_position *= dls::math::vec3f(1.0f / mass);

		if (!dls::math::est_fini(total_position)) {
			/* sérialise les données */
		}

		node.p = Particule(total_position, mass);
	}

	int cree_enfant(int t, int child_index)
	{
		return cree_enfant(t, child_index, Particule(dls::math::vec3f(0.0f), 0.0f));
	}

	int cree_enfant(int t, int child_index, const Particule &p)
	{
		int nt = cree_nouveau_noeud();
		nodes[t].children[child_index] = nt;
		nodes[nt].p = p;
		return nt;
	}

	int cree_nouveau_noeud()
	{
		nodes[node_end] = Node();
		return node_end++;
	}

public:
	// We do not evaluate the weighted average of position and mass on the fly
	// for efficiency and accuracy
	void initialise(float res,
					float marginfloat,
					const dls::tableau<Particule> &particles)
	{
		this->resolution = res;
		this->inv_resolution = 1.0f / res;
		this->margin = static_cast<int>(std::ceil(marginfloat * inv_resolution));
		assert(particles.taille() != 0);
		dls::math::vec3f lower(1e30f);
		dls::math::vec3f upper(-1e30f);

		for (auto &p : particles) {
			for (size_t k = 0; k < 3; k++) {
				lower[k] = std::min(lower[k], p.position[k]);
				upper[k] = std::max(upper[k], p.position[k]);
			}
			// TC_P(p.position);
		}

		lower_corner = lower;
		int intervals = static_cast<int>(std::ceil(max(upper - lower) / res));
		total_levels = 0;
		for (int i = 1; i < intervals; i *= 2, total_levels++)
			;
		// We do not use the 0th node...
		node_end = 1;
		nodes.efface();
		nodes.redimensionne(particles.taille() * 2);
		int root = cree_nouveau_noeud();
		// Make sure that one leaf node contains only one particle.
		// Unless particles are too close and thereby merged.
		for (auto &p : particles) {
			if (p.mass == 0.0f) {
				continue;
			}

			dls::math::vec3i u = coordonnees_grille(p.position);
			auto t = root;

			if (nodes[t].is_leaf()) {
				// First node
				nodes[t].p = p;
				continue;
			}

			// Traverse down until there's no way...
			int k = 0;
			//TC_ERROR("cp maybe originally used without initialization");
			int cp = -1;
			for (; k < total_levels; k++) {
				cp = calcul_index_enfant(u, k);
				if (nodes[t].children[cp] != 0) {
					t = nodes[t].children[cp];
				} else {
					break;
				}
			}
			if (nodes[t].is_leaf()) {
				// Leaf node, containing one particle q
				// Split the node until p and q belong to different children.
				Particule q = nodes[t].p;
				nodes[t].p = Particule();
				dls::math::vec3i v = coordonnees_grille(q.position);
				int cq = calcul_index_enfant(v, k);
				while (cp == cq && k < total_levels) {
					t = cree_enfant(t, cp);
					k++;
					cp = calcul_index_enfant(u, k);
					cq = calcul_index_enfant(v, k);
				}
				if (k == total_levels) {
					// We have to merge two particles since they are too close...
					q = p + q;
					cree_enfant(t, cp, q);
				} else {
					nodes[t].p = Particule();
					cree_enfant(t, cp, p);
					cree_enfant(t, cq, q);
				}
			} else {
				// Non-leaf node, simply create a child.
				cree_enfant(t, cp, p);
			}
		}
		//TC_P(node_end);
		sommarise(root);
	}

	/*
  template<typename T>
  dls::math::vec3f summation(const Particle &p, const T &func) {
	  // À FAIRE: fine level
	  // À FAIRE: only one particle?
	  int t = 1;
	  dls::math::vec3f ret(0.0f);
	  dls::math::vec3f u = get_coord(p.position);
	  for (int k = 0; k < total_levels; k++) {
		  int cp = get_child_index(u, k);
		  for (int c = 0; c < 8; c++) {
			  if (c != cp && nodes[t].children[c]) {
				  const Node &n = nodes[nodes[t].children[c]];
				  auto tmp = func(p, n.p);
				  ret += tmp;
			  }
		  }
		  t = nodes[t].children[cp];
		  if (t == 0) {
			  break;
		  }
	  }
	  return ret;
  }
  */

	template <typename T>
	dls::math::vec3f summation(int t, const Particule &p, const T &func)
	{
		const Node &node = nodes[t];
		if (nodes[t].is_leaf()) {
			return func(p, node.p);
		}

		dls::math::vec3f ret(0.0f);
		dls::math::vec3i u = coordonnees_grille(p.position);

		for (size_t c = 0; c < 8; c++) {
			if (node.children[c]) {
				const Node &ch = nodes[node.children[c]];
				if (ch.bounds[0][0] <= u[0] && u[0] <= ch.bounds[1][0] &&
						ch.bounds[0][1] <= u[1] && u[1] <= ch.bounds[1][1] &&
						ch.bounds[0][2] <= u[2] && u[2] <= ch.bounds[1][2]) {
					ret += summation(node.children[c], p, func);
				} else {
					// Coarse summation
					ret += func(p, ch.p);
				}
			}
		}

		return ret;
	}
};

class OperatriceSolveurNCorps : public OperatriceCorps {
public:
	static constexpr auto NOM = "Solveur N-Corps";
	static constexpr auto AIDE = "";

	explicit OperatriceSolveurNCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_solveur_n_corps.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void initialise_attributs()
	{
		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();

		auto attr_V = m_corps.attribut("V");
		auto mult_vel = evalue_decimal("mult_vel");

		if (attr_V == nullptr) {
			attr_V = m_corps.ajoute_attribut("V", type_attribut::VEC3, portee_attr::POINT);

			for (auto i = 0; i < nombre_points; ++i) {
				auto pos = liste_points->point(i);
				attr_V->valeur(i, (pos - dls::math::vec3f(0.5f)) * mult_vel);
			}
		}

		m_corps.ajoute_attribut("pos_pre", type_attribut::VEC3, portee_attr::POINT);
	}

	void sous_etape(float gravitation, float dt)
	{
		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();
		auto attr_V = m_corps.attribut("V");
		auto attr_P = m_corps.attribut("pos_pre");

		using BHP = BarnesHutSummation::Particule;

		dls::tableau<BHP> bhps;
		bhps.reserve(nombre_points);

		for (auto i = 0; i < nombre_points; ++i) {
			auto pos = liste_points->point(i);
			bhps.pousse(BHP(pos, 1.0f));
		}

		BarnesHutSummation bhs;
		bhs.initialise(1e-4f, 1e-3f, bhps);

		auto f = [](const BHP &p, const BHP &q) {
			dls::math::vec3f d = p.position - q.position;
			float dist2 = produit_scalaire(d, d);
			dist2 += 1e-4f;
			d *= dls::math::vec3f(p.mass * q.mass / (dist2 * std::sqrt(dist2)));
			return d;
		};

		if (gravitation != 0.0f) {
			boucle_parallele(
						tbb::blocked_range<long>(0, nombre_points),
						[&](tbb::blocked_range<long> const &plage)
			{
				for (auto i = plage.begin(); i < plage.end(); ++i) {
					auto pos = liste_points->point(i);
					dls::math::vec3f totalf_bhs = bhs.summation(1, BHP(pos, 1.0f), f);

					attr_V->valeur(i, attr_V->vec3(i) + totalf_bhs * gravitation * dt);
				}
			});
		}

		for (auto i = 0; i < nombre_points; ++i) {
			auto pos = liste_points->point(i);

			liste_points->point(i, pos + dt * attr_V->vec3(i));
			attr_P->valeur(i, pos);
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto liste_points = m_corps.points();
		auto const nombre_points = liste_points->taille();

		if (nombre_points == 0) {
			return EXECUTION_REUSSIE;
		}

		initialise_attributs();

		auto const dt_config = 0.1f; // evalue_decimal("dt");
		auto const dt_simulation = evalue_decimal("dt_simulation");
		auto nb_etape = dt_config / dt_simulation;

		auto const gravitation = evalue_decimal("gravitation");

		for (auto i = 0; i < static_cast<int>(nb_etape); ++i) {
			sous_etape(gravitation, dt_simulation / nb_etape);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSimulationOcean : public OperatriceCorps {
	Ocean m_ocean{};
	bool m_reinit = false;

	calque_image m_ecume_precedente{};

public:
	static constexpr auto NOM = "Océan";
	static constexpr auto AIDE = "";

	explicit OperatriceSimulationOcean(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_ocean.jo";
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

		if (m_corps.points()->taille() == 0) {
			this->ajoute_avertissement("Le corps d'entrée est vide");
			return EXECUTION_ECHOUEE;
		}

		/* paramètres simulations */
		auto resolution = dls::math::restreint(evalue_entier("résolution"), 4, 11);
		auto velocite_vent = evalue_decimal("vélocité_vent");
		auto echelle_vague = evalue_decimal("échelle_vague");
		auto alignement_vague = evalue_decimal("alignement_vague");
		auto plus_petite_vague = evalue_decimal("plus_petite_vague");
		auto direction_vague = evalue_decimal("direction_vague");
		auto profondeur = evalue_decimal("profondeur");
		auto gravite = evalue_decimal("gravité");
		auto damp = evalue_decimal("damping");
		auto quantite_chop = evalue_decimal("quantité_chop");
		auto graine = evalue_entier("graine");
		auto temps = evalue_decimal("temps", contexte.temps_courant) / static_cast<float>(contexte.cadence);
		auto couverture_ecume = evalue_decimal("couverture_écume");
		auto attenuation_ecume = evalue_decimal("atténuation_écume");
		auto taille = evalue_decimal("taille");
		auto taille_spaciale = static_cast<float>(evalue_entier("taille_spaciale"));

		auto const taille_inverse = 1.0f / (taille * taille_spaciale);

		if (!std::isfinite(taille_inverse)) {
			this->ajoute_avertissement("La taille inverse n'est pas finie");
			return EXECUTION_ECHOUEE;
		}

		m_ocean.res_x = static_cast<int>(std::pow(2.0, resolution));
		m_ocean.res_y = m_ocean.res_x;
		m_ocean.calcul_deplacement_y = true;
		m_ocean.calcul_normaux = true;
		m_ocean.calcul_ecume = true;
		m_ocean.calcul_chop = (quantite_chop > 0.0f);
		m_ocean.l = plus_petite_vague;
		m_ocean.amplitude = 1.0f;
		m_ocean.reflections_damp = 1.0f - damp;
		m_ocean.alignement_vent = alignement_vague;
		m_ocean.profondeur = profondeur;
		m_ocean.taille_spaciale_x = taille_spaciale;
		m_ocean.taille_spaciale_z = taille_spaciale;
		m_ocean.vent_x = std::cos(direction_vague);
		m_ocean.vent_z = -std::sin(direction_vague);
		/* plus grosse vague pour une certaine vélocité */
		m_ocean.L = velocite_vent * velocite_vent / gravite;
		m_ocean.temps = temps;
		m_ocean.graine = graine;

		if (contexte.temps_courant == contexte.temps_debut || m_reinit) {
			deloge_donnees_ocean(&m_ocean);
			initialise_donnees_ocean(&m_ocean, gravite);

			auto desc = desc_grille_2d{};
			desc.etendue.min = dls::math::vec2f(0.0f);
			desc.etendue.max = dls::math::vec2f(1.0f);
			desc.fenetre_donnees = desc.etendue;
			desc.taille_pixel = 1.0 / (static_cast<double>(m_ocean.res_x));
			desc.type_donnees = type_grille::R32;

			m_ecume_precedente = calque_image::construit_calque(desc);

			m_reinit = false;
		}

		simule_ocean(&m_ocean, temps, echelle_vague, quantite_chop, gravite);

		auto grille_ecume = dynamic_cast<grille_dense_2d<float> *>(m_ecume_precedente.tampon);

		/* applique les déplacements à la géométrie d'entrée */
		auto points = m_corps.points();
		auto res_x = m_ocean.res_x;
		auto res_y = m_ocean.res_y;

		auto N = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);
		N->redimensionne(points->taille());

		auto C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		C->redimensionne(points->taille());

		OceanResult ocr;

		auto gna = GNA{graine};

		for (auto i = 0; i < points->taille(); ++i) {
			auto p = points->point(i);

			/* converti la position du point en espace grille */
			auto u = std::fmod(p.x * taille_inverse + 0.5f, 1.0f);
			auto v = std::fmod(p.z * taille_inverse + 0.5f, 1.0f);

			if (u < 0.0f) {
				u += 1.0f;
			}

			if (v < 0.0f) {
				v += 1.0f;
			}

			auto x = static_cast<int>(u * (static_cast<float>(res_x)));
			auto y = static_cast<int>(v * (static_cast<float>(res_y)));

			/* À FAIRE : échantillonage bilinéaire. */
			evalue_ocean_ij(&m_ocean, &ocr, x, y);

			p[0] += ocr.disp[0];
			p[1] += ocr.disp[1];
			p[2] += ocr.disp[2];

			points->point(i, p);
			N->vec3(i) = ocr.normal;

			if (m_ocean.calcul_ecume) {
				auto index = grille_ecume->calcul_index(dls::math::vec2i(x, y));
				auto ecume = ocean_jminus_vers_ecume(ocr.Jminus, couverture_ecume);

				/* accumule l'écume précédente pour cette cellule. */
				auto ecume_prec = grille_ecume->valeur(index);

				/* réduit aléatoirement l'écume */
				ecume_prec *= gna.uniforme(0.0f, 1.0f);

				if (ecume_prec < 1.0f) {
					ecume_prec *= ecume_prec;
				}

				/* brise l'écume là où la hauteur (Y) est basse (vallée), et le
				 * déplacement X et Z est au plus haut.
				 */

				auto eplus_neg = ocr.Eplus[2] < 0.0f ? 1.0f + ocr.Eplus[2] : 1.0f;
				eplus_neg = eplus_neg < 0.0f ? 0.0f : eplus_neg;

				ecume_prec *= attenuation_ecume * (0.75f + eplus_neg * 0.25f);

				/* Une restriction pleine ne devrait pas être nécessaire ! */
				auto resultat_ecume = std::min(ecume_prec + ecume, 1.0f);

				grille_ecume->valeur(index) = resultat_ecume;

				C->vec3(i) = dls::math::vec3f(resultat_ecume, resultat_ecume, 1.0f);
			}
		}

		return EXECUTION_REUSSIE;
	}

	void parametres_changes() override
	{
		m_reinit = true;
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
	usine.enregistre_type(cree_desc<OperatriceSolveurNCorps>());
	usine.enregistre_type(cree_desc<OperatriceSimulationOcean>());
}

#pragma clang diagnostic pop
