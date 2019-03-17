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

#include "operatrices_vetements.hh"

#include <set>

#include "bibliotheques/outils/parallelisme.h"

#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

/* Un simple solveur de dynamique de vêtement utilisant des dynamiques basées
 * sur les positions tiré des notes du cours de SIGGRAPH "Realtime Physics"
 * http://www.matthiasmueller.info/realtimephysics/coursenotes.pdf
 *
 * Tiré de "OpenCloth", voir discussion sur
 * http://www.bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=6&t=7013&p=24420#p24420
 */

/* ************************************************************************** */

#define EPSILON  0.0000001f

struct DistanceConstraint {
	/* index des points */
	long p1, p2;
	float longueur_repos;
	/* coefficient d'étirement */
	float k;
	/* 1 - coefficient d'étirement */
	float k_prime;

	float pad;
};

struct BendingConstraint {
	/* index des points */
	long p1, p2, p3;
	float longueur_repos;
	/* poids */
	float w;
	/* coefficient de courbure */
	float k;
	/* 1 - coefficient de courbure */
	float k_prime;
};

static auto ajoute_contrainte_distance(
		ListePoints3D *X,
		long a,
		long b,
		float k,
		int solver_iterations)
{
	DistanceConstraint c;
	c.p1 = a;
	c.p2 = b;
	c.k = k;
	c.k_prime = 1.0f - std::pow((1.0f - c.k), 1.0f / static_cast<float>(solver_iterations));

	if (c.k_prime > 1.0f) {
		c.k_prime = 1.0f;
	}

	auto const deltaP = X->point(c.p1) - X->point(c.p2);
	c.longueur_repos = longueur(deltaP);

	return c;
}

static auto ajoute_contrainte_courbure(
		ListePoints3D *X,
		Attribut *W,
		long pa,
		long pb,
		long pc,
		float k,
		int solver_iterations)
{
	BendingConstraint c;
	c.p1 = pa;
	c.p2 = pb;
	c.p3 = pc;

	c.w = W->decimal(pa) + W->decimal(pb) + 2.0f * W->decimal(pc);
	auto const centre = 0.3333f * (X->point(pa) + X->point(pb) + X->point(pc));
	c.longueur_repos = longueur(X->point(pc) - centre);
	c.k = k;
	c.k_prime = 1.0f - std::pow((1.0f - c.k), 1.0f / static_cast<float>(solver_iterations));

	if (c.k_prime > 1.0f) {
		c.k_prime = 1.0f;
	}

	return c;
}

static void calcul_forces(
		Attribut *F,
		Attribut *W,
		dls::math::vec3f const &gravity)
{
	for (auto i = 0; i < F->taille(); i++) {
		F->vec3(i, dls::math::vec3f(0.0f));

		if (W->decimal(i) > 0.0f) {
			F->vec3(i, gravity);
		}
	}
}

static void integre_explicitement_avec_attenuation(
		float deltaTime,
		Attribut *tmp_X,
		Attribut *V,
		Attribut *F,
		ListePoints3D *X,
		Attribut *Ri,
		Attribut *W,
		float mass,
		float kDamp,
		float global_dampening)
{
	auto Xcm = dls::math::vec3f(0.0f);
	auto Vcm = dls::math::vec3f(0.0f);
	auto sumM = 0.0f;

	for (auto i = 0; i < X->taille(); ++i) {
		auto Vi = (V->vec3(i) * global_dampening) + (F->vec3(i) * deltaTime) * W->decimal(i);
		V->vec3(i, Vi);

		/* calcul la position et la vélocité du centre de masse pour l'atténuation */
		Xcm += (X->point(i) * mass);
		Vcm += (Vi * mass);
		sumM += mass;
	}

	Xcm /= sumM;
	Vcm /= sumM;

	auto I = dls::math::mat3x3f{};
	auto L = dls::math::vec3f(0.0f);

	/* vélocité angulaire */
	auto w = dls::math::vec3f(0.0f);

	for (auto i = 0; i < X->taille(); ++i) {
		auto Ri_i = (X->point(i) - Xcm);
		Ri->vec3(i, Ri_i);

		L += produit_croix(Ri_i, mass*V->vec3(i));

		/* voir http://www.sccg.sk/~onderik/phd/ca2010/ca10_lesson11.pdf */
		auto tmp = dls::math::mat3x3f(
					   0.0f, -Ri_i.z,  Ri_i.y,
					 Ri_i.z,    0.0f, -Ri_i.x,
					-Ri_i.y,  Ri_i.x,  0.0f);

		tmp *= transpose(tmp);

		for (auto j = 0ul; j < 3ul; ++j) {
			for (auto k = 0ul; k < 3ul; ++k) {
				tmp[j][k] *= mass;
			}
		}

		I += tmp;
	}

	w = inverse(I) * L;

	/* applique l'atténuation du centre de masse */
	for (auto i = 0; i < X->taille(); ++i) {
		dls::math::vec3f delVi = Vcm + produit_croix(w,Ri->vec3(i)) - V->vec3(i);
		V->vec3(i, V->vec3(i) + kDamp*delVi);
	}

	/* calcul position prédite */
	for (auto i = 0; i < X->taille(); ++i) {
		if (W->decimal(i) <= 0.0f) {
			tmp_X->vec3(i, X->point(i)); //fixed points
		}
		else {
			tmp_X->vec3(i, X->point(i) + V->vec3(i) * deltaTime);
		}
	}
}

static void integre(
		float deltaTime,
		Attribut *V,
		ListePoints3D *X,
		Attribut *tmp_X)
{
	auto const inv_dt = 1.0f / deltaTime;

	for (auto i = 0; i < X->taille(); i++) {
		V->vec3(i, (tmp_X->vec3(i) - X->point(i)) * inv_dt);
		X->point(i, tmp_X->vec3(i));
	}
}

static void ajourne_contraintes_distance(
		int i,
		std::vector<DistanceConstraint> const &d_constraints,
		Attribut *tmp_X,
		Attribut *W)
{
	auto const &c = d_constraints[static_cast<size_t>(i)];
	auto const dir = tmp_X->vec3(c.p1) - tmp_X->vec3(c.p2);
	auto const len = longueur(dir);

	if (len <= EPSILON) {
		return;
	}

	auto const w1 = W->decimal(c.p1);
	auto const w2 = W->decimal(c.p2);
	auto const invMass = w1 + w2;

	if (invMass <= EPSILON) {
		return;
	}

	auto const dP = (1.0f / invMass) * (len-c.longueur_repos ) * (dir / len) * c.k_prime;

	if (w1 > 0.0f) {
		tmp_X->vec3(c.p1, tmp_X->vec3(c.p1) - dP * w1);
	}

	if (w2 > 0.0f) {
		tmp_X->vec3(c.p2, tmp_X->vec3(c.p2) + dP * w2);
	}
}

static void ajourne_contraintes_courbures(
		int index,
		std::vector<BendingConstraint> const &b_constraints,
		Attribut *tmp_X,
		Attribut *W,
		float global_dampening)
{
	auto const &c = b_constraints[static_cast<size_t>(index)];

	/* Utilisation de l'algorithme tiré du papier
	 * 'A Triangle Bending Constraint Model for Position-Based Dynamics'
	 * http://image.diku.dk/kenny/download/kelager.niebe.ea10.pdf
	 */

	auto const global_k = global_dampening * 0.01f;
	auto const centre = 0.3333f * (tmp_X->vec3(c.p1) + tmp_X->vec3(c.p2) + tmp_X->vec3(c.p3));
	auto const dir_centre = tmp_X->vec3(c.p3) - centre;
	auto const dist_centre = longueur(dir_centre);

	auto const diff = 1.0f - ((global_k + c.longueur_repos) / dist_centre);
	auto const dir_force = dir_centre * diff;
	auto const fa =  c.k_prime * ((2.0f * W->decimal(c.p1)) / c.w) * dir_force;
	auto const fb =  c.k_prime * ((2.0f * W->decimal(c.p2)) / c.w) * dir_force;
	auto const fc = -c.k_prime * ((4.0f * W->decimal(c.p3)) / c.w) * dir_force;

	if (W->decimal(c.p1) > 0.0f)  {
		tmp_X->vec3(c.p1, tmp_X->vec3(c.p1) + fa);
	}

	if (W->decimal(c.p2) > 0.0f) {
		tmp_X->vec3(c.p2, tmp_X->vec3(c.p2) + fb);
	}

	if (W->decimal(c.p3) > 0.0f) {
		tmp_X->vec3(c.p3, tmp_X->vec3(c.p3) + fc);
	}
}

static void ajourne_contraintes_internes(
		Attribut *tmp_X,
		std::vector<DistanceConstraint> const &d_constraints,
		std::vector<BendingConstraint> const &b_constraints,
		Attribut *W,
		int solver_iterations,
		float global_dampening)
{
	for (auto si=0;si<solver_iterations;++si) {
		for (size_t i=0;i<d_constraints.size();i++) {
			ajourne_contraintes_distance(static_cast<int>(i), d_constraints, tmp_X, W);
		}

		for (size_t i=0;i<b_constraints.size();i++) {
			ajourne_contraintes_courbures(static_cast<int>(i), b_constraints, tmp_X, W, global_dampening);
		}
	}
}

/* ************************************************************************** */

struct ContrainteDistance {
	float longueur_repos;
	float pad;
	long v0;
	long v1;
};

struct DonneesSimVerlet {
	std::vector<ContrainteDistance> contrainte_distance;
	float drag;
	int repetitions;
	float dt;
	float pad;
};

static void integre_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
	auto attr_P = corps.attribut("P_prev");
	auto attr_F = corps.attribut("F");
	auto points = corps.points();

	boucle_parallele(tbb::blocked_range<long>(0, points->taille()),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); ++i) {
			auto pos_cour = points->point(i);
			auto pos_prev = attr_P->vec3(i);
			auto force = attr_F->vec3(i);
			auto drag = 1.0f - donnees_sim.drag;

			/* integration */
			auto vel = (pos_cour - pos_prev) + force * donnees_sim.dt;
			auto pos_nouv = (pos_cour + vel * donnees_sim.dt * drag);

			/* ajourne données solveur */
			attr_P->vec3(i, pos_cour);
			points->point(i, pos_nouv);
		}
	});
}

static void contraintes_distance_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
	auto points = corps.points();

	for (auto const &contrainte : donnees_sim.contrainte_distance) {
		auto vec1 = points->point(contrainte.v0);
		auto vec2 = points->point(contrainte.v1);

		/* calcul nouvelles positions */
		auto const delta = vec2 - vec1;
		auto const longueur_delta = longueur(delta);
		auto const difference = (longueur_delta - contrainte.longueur_repos) / longueur_delta;
		vec1 = vec1 + delta * 0.5f * difference;
		vec2 = vec2 - delta * 0.5f * difference;

		/* ajourne positions */
		points->point(contrainte.v0, vec1);
		points->point(contrainte.v1, vec2);
	}
}

static void contraintes_position_verlet(Corps &corps, DonneesSimVerlet const &/*donnees_sim*/)
{
	auto attr_P = corps.attribut("P_prev");
	auto attr_W = corps.attribut("W");
	auto points = corps.points();

	boucle_parallele(tbb::blocked_range<long>(0, points->taille()),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); ++i) {
			if (attr_W->decimal(i) <= 0.0f) {
				points->point(i, attr_P->vec3(i));
			}
		}
	});
}

static void applique_contraintes_verlet(Corps &corps, DonneesSimVerlet const &donnees_sim)
{
	/* À FAIRE contraintes collision */
	contraintes_distance_verlet(corps, donnees_sim);
	contraintes_position_verlet(corps, donnees_sim);
}

/* ************************************************************************** */

static std::set<std::pair<long, long>> calcul_cote_unique(Corps &corps)
{
	auto prims_entree = corps.prims();
	std::set<std::pair<long, long>> ensemble_cote;

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		for (auto j = 1; j < poly->nombre_sommets(); ++j) {
			auto j0 = poly->index_point(j - 1);
			auto j1 = poly->index_point(j);

			/* Ordonne les index pour ne pas compter les cotés allant
			 * dans le sens opposé : (0, 1) = (1, 0). */
			ensemble_cote.insert(std::make_pair(std::min(j0, j1), std::max(j0, j1)));
		}

		/* diagonales, À FAIRE : + de 4 sommets */
		if (poly->nombre_sommets() == 4) {
			auto j0 = poly->index_point(0);
			auto j1 = poly->index_point(2);

			ensemble_cote.insert(std::make_pair(j0, j1));

			j0 = poly->index_point(1);
			j1 = poly->index_point(3);

			ensemble_cote.insert(std::make_pair(j0, j1));
		}
	}

	return ensemble_cote;
}

class OperatriceSimVetement final : public OperatriceCorps {
	std::vector<DistanceConstraint> d_constraints{};

	std::vector<BendingConstraint> b_constraints{};

	DonneesSimVerlet m_donnees_verlet{};

public:
	static constexpr auto NOM = "Simulation Vêtement";
	static constexpr auto AIDE = "Simule un vêtement selon l'algorithme de Dynamiques Basées Point.";

	explicit OperatriceSimVetement(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_simulation_vetement.jo";
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
			this->ajoute_avertissement("Il n'y a aucun point en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = m_corps.prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Il n'y a auncune primitive en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto integration = evalue_enum("intégration");

		if (integration == "verlet") {
			simule_verlet(contexte.temps_courant);
		}
		else {
			simule_dbp(contexte.temps_courant);
		}

		return EXECUTION_REUSSIE;
	}

	int simule_verlet(int temps)
	{
		auto points_entree = m_corps.points();

		auto const gravity = evalue_vecteur("gravité", temps);
		auto const mass = evalue_decimal("masse") / static_cast<float>(points_entree->taille());

		auto W = m_corps.ajoute_attribut("W", type_attribut::DECIMAL, portee_attr::POINT);
		auto F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);
		auto P = m_corps.ajoute_attribut("P_prev", type_attribut::VEC3, portee_attr::POINT);

		/* À FAIRE : réinitialisation */
		if (temps == 1) {
			for (auto i = 0; i < points_entree->taille(); ++i) {
				P->vec3(i, points_entree->point(i));
			}
		}

		for (auto i = 0; i < points_entree->taille(); ++i) {
			W->decimal(i, 1.0f / mass);
		}

		/* points fixes, À FAIRE : groupe ou attribut. */
		for (auto i = 0; i < 20; ++i) {
			W->decimal(i, 0.0f);
		}

		m_donnees_verlet.drag = evalue_decimal("atténuation", temps);
		m_donnees_verlet.repetitions = evalue_entier("itérations");
		m_donnees_verlet.dt = evalue_decimal("dt", temps);

		if (m_donnees_verlet.contrainte_distance.empty()) {
			auto ensemble_cote = calcul_cote_unique(m_corps);
			m_donnees_verlet.contrainte_distance.reserve(ensemble_cote.size());

			for (auto &cote : ensemble_cote) {
				auto c = ContrainteDistance{};
				c.v0 = cote.first;
				c.v1 = cote.second;

				c.longueur_repos = longueur(points_entree->point(c.v0) - points_entree->point(c.v1));

				m_donnees_verlet.contrainte_distance.push_back(c);
			}
		}

		/* lance la simulation */
		calcul_forces(F, W, gravity);

		integre_verlet(m_corps, m_donnees_verlet);

		for(int index = 0; index < m_donnees_verlet.repetitions; index++) {
			applique_contraintes_verlet(m_corps, m_donnees_verlet);
		}

		return EXECUTION_REUSSIE;
	}

	int simule_dbp(int temps)
	{
		auto points_entree = m_corps.points();
		auto prims_entree = m_corps.prims();

		auto total_points = static_cast<size_t>(points_entree->taille());

		std::vector<unsigned short> indices;
		std::vector<float> phi0; //initial dihedral angle between adjacent triangles

		auto X = points_entree;
		auto tmp_X = m_corps.ajoute_attribut("tmp_X", type_attribut::VEC3, portee_attr::POINT);
		auto V = m_corps.ajoute_attribut("V", type_attribut::VEC3, portee_attr::POINT);
		auto F = m_corps.ajoute_attribut("F", type_attribut::VEC3, portee_attr::POINT);
		auto W = m_corps.ajoute_attribut("W", type_attribut::DECIMAL, portee_attr::POINT);
		auto Ri = m_corps.ajoute_attribut("Ri", type_attribut::VEC3, portee_attr::POINT);

		/* paramètres */
		auto const attenuation_globale = evalue_decimal("atténuation_globale", temps);
		auto const dt = evalue_decimal("dt");
		auto const iterations = evalue_entier("itérations");
		auto const courbe = evalue_decimal("courbe", temps);
		auto const etirement = evalue_decimal("étirement", temps);
		auto const attenuation = evalue_decimal("atténuation", temps);
		auto const gravity = evalue_vecteur("gravité", temps) * 0.001f;
		auto const mass = evalue_decimal("masse") / static_cast<float>(total_points);

		/* prépare données */

		for (auto i = 0; i < points_entree->taille(); ++i) {
			W->decimal(i, 1.0f / mass);
		}

		/* points fixes, À FAIRE : groupe ou attribut. */
		for (auto i = 0; i < 20; ++i) {
			W->decimal(i, 0.0f);
		}

		if (d_constraints.empty()) {
			auto ensemble_cote = calcul_cote_unique(m_corps);
			d_constraints.reserve(ensemble_cote.size());

			for (auto &cote : ensemble_cote) {
				auto c = ajoute_contrainte_distance(
							X,
							cote.first,
							cote.second,
							etirement,
							iterations);

				d_constraints.push_back(c);
			}
		}

		if (b_constraints.empty()) {
			b_constraints.reserve(static_cast<size_t>(prims_entree->taille()));

			for (auto i = 0; i < prims_entree->taille(); ++i) {
				auto prim = prims_entree->prim(i);

				if (prim->type_prim() != type_primitive::POLYGONE) {
					continue;
				}

				auto poly = dynamic_cast<Polygone *>(prim);

				for (auto j = 2; j < poly->nombre_sommets(); ++j) {
					auto c = ajoute_contrainte_courbure(
								X,
								W,
								poly->index_point(0),
								poly->index_point(j - 1),
								poly->index_point(j),
								courbe,
								iterations);

					b_constraints.push_back(c);
				}
			}
		}

		if (d_constraints.empty() || b_constraints.empty()) {
			this->ajoute_avertissement("Aucune contrainte n'a pu être ajoutée, aucun polygone trouvé !");
			return EXECUTION_ECHOUEE;
		}

		/* lance simulation */

		calcul_forces(F, W, gravity);

		integre_explicitement_avec_attenuation(
					dt,
					tmp_X,
					V,
					F,
					X,
					Ri,
					W,
					mass,
					attenuation,
					attenuation_globale);

		ajourne_contraintes_internes(
					tmp_X,
					d_constraints,
					b_constraints,
					W,
					iterations,
					attenuation_globale);

		/* À FAIRE : collision (position tmp_X sur la surface, réinit V). */

		integre(dt, V, X, tmp_X);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_vetement(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSimVetement>());
}
