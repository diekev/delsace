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

#include "operatrices_ariel.hh"

#include "../corps/corps.h"

#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#ifdef WITH_ARIEL
#include "ariel/geom/cubegen.hpp"
#include "ariel/sim/flip.hpp"
#include "ariel/scene/sceneloader.hpp"
#endif

#include "outils_visualisation.hh"

/* ************************************************************************** */

struct Particule {
	dls::math::vec3f pos{};
	dls::math::vec3f vel{};
	dls::math::vec3f vel_pic{};
};

template <typename T>
class Grille {
	dls::tableau<T> m_donnees = {};

	dls::math::vec3<size_t> m_res = dls::math::vec3<size_t>(0ul, 0ul, 0ul);
	size_t m_nombre_voxels = 0;

	T m_arriere_plan = T(0);

	size_t calcul_index(size_t x, size_t y, size_t z) const
	{
		return x + (y + z * m_res[1]) * m_res[0];
	}

	bool hors_des_limites(size_t x, size_t y, size_t z) const
	{
		if (x >= m_res[0]) {
			return true;
		}

		if (y >= m_res[1]) {
			return true;
		}

		if (z >= m_res[2]) {
			return true;
		}

		return false;
	}

public:
	Grille() = default;

	void initialise(size_t res_x, size_t res_y, size_t res_z)
	{
		m_res[0] = res_x;
		m_res[1] = res_y;
		m_res[2] = res_z;

		m_nombre_voxels = res_x * res_y * res_z;

		m_donnees.redimensionne(static_cast<long>(m_nombre_voxels));
		std::fill(m_donnees.debut(), m_donnees.fin(), T(0));
	}

	dls::math::vec3<size_t> resolution() const
	{
		return m_res;
	}

	T valeur(size_t index) const
	{
		if (index >= m_nombre_voxels) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T valeur(size_t x, size_t y, size_t z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[calcul_index(x, y, z)];
	}

	void valeur(size_t index, T v)
	{
		if (index >= m_nombre_voxels) {
			return;
		}

		m_donnees[index] = v;
	}

	void valeur(size_t x, size_t y, size_t z, T v)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[calcul_index(x, y, z)] = v;
	}

	void copie(Grille<T> const &grille)
	{
		for (size_t i = 0; i < m_nombre_voxels; ++i) {
			m_donnees[i] = grille.m_donnees[i];
		}
	}

	void arriere_plan(T const &v)
	{
		m_arriere_plan = v;
	}

	void *donnees() const
	{
		return m_donnees.donnees();
	}

	size_t taille_octet() const
	{
		return m_nombre_voxels * sizeof(T);
	}
};

class GrilleParticule {
	dls::tableau<dls::tableau<Particule *>> m_donnees = {};
	dls::math::vec3<size_t> m_res = dls::math::vec3<size_t>(0ul, 0ul, 0ul);
	size_t m_nombre_voxels = 0;

	dls::tableau<Particule *> m_arriere_plan = {};

	size_t calcul_index(size_t x, size_t y, size_t z) const
	{
		return x + (y + z * m_res[1]) * m_res[0];
	}

	bool hors_des_limites(size_t x, size_t y, size_t z) const
	{
		if (x >= m_res[0]) {
			return true;
		}

		if (y >= m_res[1]) {
			return true;
		}

		if (z >= m_res[2]) {
			return true;
		}

		return false;
	}

public:
	void initialise(size_t res_x, size_t res_y, size_t res_z)
	{
		m_res.x = res_x;
		m_res.y = res_y;
		m_res.z = res_z;

		m_nombre_voxels = res_x * res_y * res_z;
		m_donnees.redimensionne(static_cast<long>(m_nombre_voxels));

		for (auto &donnees : m_donnees) {
			donnees.efface();
		}
	}

	void ajoute_particule(size_t x, size_t y, size_t z, Particule *particule)
	{
		if (hors_des_limites(x, y, z)) {
			return;
		}

		m_donnees[static_cast<long>(calcul_index(x, y, z))].pousse(particule);
	}

	const dls::tableau<Particule *> &particule(size_t x, size_t y, size_t z) const
	{
		if (hors_des_limites(x, y, z)) {
			return m_arriere_plan;
		}

		return m_donnees[static_cast<long>(calcul_index(x, y, z))];
	}
};

enum {
	CELLULE_VIDE   = 0,
	CELLULE_FLUIDE = 1,
};

struct Fluide {
	dls::math::vec3<size_t> res = dls::math::vec3<size_t>(32ul);

	Grille<char> drapeaux{};
	Grille<float> phi{};
	Grille<float> velocite_x{};
	Grille<float> velocite_y{};
	Grille<float> velocite_z{};
	Grille<float> velocite_x_ancienne{};
	Grille<float> velocite_y_ancienne{};
	Grille<float> velocite_z_ancienne{};
	Grille<dls::math::vec3f> velocite{};
	Grille<dls::math::vec3f> ancienne_velocites{};
	GrilleParticule grille_particules{};

	dls::tableau<Particule> particules{};

	/* géométrie domaine */
	dls::math::vec3f min = dls::math::vec3f(-8.0f, 0.0f, -8.0f);
	dls::math::vec3f max = dls::math::vec3f( 8.0f);

	dls::math::vec3f taille_voxel{};

	Fluide() = default;
	~Fluide() = default;

	/* pour faire taire cppcheck car source et domaine sont alloués dynamiquement */
	Fluide(Fluide const &autre) = default;
	Fluide &operator=(Fluide const &autre) = default;

	int temps_courant{};
	int temps_debut{};
	int temps_fin{};
	int temps_precedent{};

	void ajourne_pour_nouveau_temps();

private:
	void initialise();
	void construit_grille_particule();
	void soustrait_velocite();
	void sauvegarde_velocite_PIC();
	void transfert_velocite();
	void advecte_particules();
	void ajoute_acceleration();
	void conditions_bordure();
	void construit_champs_distance();
	void etend_champs_velocite();
};

void initialise_fluide(Fluide &fluide, dls::math::vec3i const &res)
{
	/* calcul la taille des voxels */
	auto dims = fluide.max - fluide.min;
	auto axe = dls::math::axe_dominant_abs(dims);

	auto res_x = static_cast<float>(res.x) * (dims.x / dims[axe]);
	auto res_y = static_cast<float>(res.y) * (dims.y / dims[axe]);
	auto res_z = static_cast<float>(res.z) * (dims.z / dims[axe]);

	fluide.res.x = static_cast<size_t>(res_x);
	fluide.res.y = static_cast<size_t>(res_y);
	fluide.res.z = static_cast<size_t>(res_z);

	fluide.taille_voxel.x = dims.x / res_x;
	fluide.taille_voxel.y = dims.y / res_y;
	fluide.taille_voxel.z = dims.z / res_z;

	/* initialise selon la résolution */
	fluide.phi.initialise(fluide.res.x, fluide.res.y, fluide.res.z);
	fluide.velocite_x.initialise(fluide.res.x, fluide.res.y, fluide.res.z);
	fluide.velocite_y.initialise(fluide.res.x, fluide.res.y, fluide.res.z);
	fluide.velocite_z.initialise(fluide.res.x, fluide.res.y, fluide.res.z);
}

void advecte_particules(Fluide &fluide, float dt)
{
	for (auto &particule : fluide.particules) {
		particule.pos.y += (-9.81f * dt);
	}
}

#include <random>

bool contiens(dls::math::vec3f const &min,
			  dls::math::vec3f const &max,
			  dls::math::vec3f const &pos)
{
	for (size_t i = 0; i < 3; ++i) {
		if (pos[i] < min[i] || pos[i] >= max[i]) {
			return false;
		}
	}

	return true;
}

void cree_particules_source(Fluide *fluide, size_t nombre)
{
	//CHRONOMETRE_PORTEE(__func__, std::cerr);
	fluide->particules.efface();

	auto const &min_source = dls::math::vec3f(-2.0f, 4.0f, -2.0f);
	auto const &max_source = dls::math::vec3f( 2.0f, 8.0f, 2.0f);

	auto const &min_domaine = fluide->min;
	auto const &taille_domaine = fluide->max - fluide->min;

	dls::math::vec3f dh_domaine(
		taille_domaine[0] / static_cast<float>(fluide->res.x),
		taille_domaine[1] / static_cast<float>(fluide->res.y),
		taille_domaine[2] / static_cast<float>(fluide->res.z)
	);

	auto dh_2 = dh_domaine * 0.50f;
	auto dh_4 = dh_domaine * 0.25f;

	Particule p;

	std::mt19937 rng(179541);
	std::uniform_real_distribution<float> dist_x(-dh_4.x, dh_4.x);
	std::uniform_real_distribution<float> dist_y(-dh_4.y, dh_4.y);
	std::uniform_real_distribution<float> dist_z(-dh_4.z, dh_4.z);

	dls::math::vec3f decalage[8] = {
		dls::math::vec3f(-dh_4.x, -dh_4.y, -dh_4.z),
		dls::math::vec3f(-dh_4.x,  dh_4.y, -dh_4.z),
		dls::math::vec3f(-dh_4.x, -dh_4.y,  dh_4.z),
		dls::math::vec3f(-dh_4.x,  dh_4.y,  dh_4.z),
		dls::math::vec3f( dh_4.x, -dh_4.y, -dh_4.z),
		dls::math::vec3f( dh_4.x,  dh_4.y, -dh_4.z),
		dls::math::vec3f( dh_4.x, -dh_4.y,  dh_4.z),
		dls::math::vec3f( dh_4.x,  dh_4.y,  dh_4.z),
	};

	/* trouve si les voxels entresectent la source, si oui, ajoute des
	 * particules */
	for (size_t z = 0; z < fluide->res.z; ++z) {
		for (size_t y = 0; y < fluide->res.y; ++y) {
			for (size_t x = 0; x < fluide->res.x; ++x) {
				auto pos = min_domaine;
				pos.x += static_cast<float>(x) * dh_domaine[0];
				pos.y += static_cast<float>(y) * dh_domaine[1];
				pos.z += static_cast<float>(z) * dh_domaine[2];

				if (!contiens(min_source, max_source, pos)) {
					continue;
				}

				auto centre_voxel = pos + dh_2;

				for (size_t i = 0; i < nombre; ++i) {
					auto pos_p = centre_voxel + decalage[i];

					p.pos.x = pos_p.x + dist_x(rng);
					p.pos.y = pos_p.y + dist_y(rng);
					p.pos.z = pos_p.z + dist_z(rng);
					p.vel = dls::math::vec3f(0.0f);

					fluide->particules.pousse(p);
				}
			}
		}
	}
}

class OperatriceAriel final : public OperatriceCorps {
#ifdef WITH_ARIEL
	sceneCore::Scene *m_scene = nullptr;
	fluidCore::FlipSim *m_flip = nullptr;
	objCore::Obj geom_ariel = objCore::Obj{};
#endif

	Fluide m_fluide{};

public:
	static constexpr auto NOM = "Ariel";
	static constexpr auto AIDE = "";

	explicit OperatriceAriel(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
	}

	~OperatriceAriel() override
	{
#ifdef WITH_ARIEL
		delete m_scene;
		delete m_flip;
#endif
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		/* paramètres */
		//auto densite = 0.5f;
		auto dimensions = dls::math::vec3i(32);
		//auto step_size = 0.005f;

		/* À FAIRE : réinitialisation. */
		if (contexte.temps_courant == 1) {
			initialise_fluide(m_fluide, dimensions);
			m_fluide.particules.efface();

			cree_particules_source(&m_fluide, 8);
		}

		/* performe simulation */
		advecte_particules(m_fluide, 1.0f / 24.0f);

		/* copie données */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

		for (auto particule : m_fluide.particules) {
			m_corps.ajoute_point(particule.pos.x, particule.pos.y, particule.pos.z);
			attr_C->pousse(dls::math::vec3f(0.0f, 0.0f, 1.0f));
		}

		/* visualise domaine */
		dessine_boite(m_corps, attr_C, m_fluide.min, m_fluide.max, dls::math::vec3f(0.0f, 1.0f, 0.0f));
		dessine_boite(m_corps, attr_C, m_fluide.min, m_fluide.min + m_fluide.taille_voxel, dls::math::vec3f(0.0f, 1.0f, 0.0f));

		return EXECUTION_REUSSIE;
	}

#ifdef WITH_ARIEL
	void simule_ariel()
	{
		/* paramètres */
		auto densite = 0.5f;
		auto dimensions = dls::math::vec3i(32);
		auto step_size = 0.005f;

		/* debogage */
		auto verbose = true;

		/* charge la scène */
		if (m_scene == nullptr) {
			m_scene = new sceneCore::Scene();
		}

		/* forces externes */
		m_scene->AddExternalForce(dls::math::vec3f(0.0f));

		/* À FAIRE : charges maillages */

		/* crée la simulation */
		if (m_flip == nullptr) {
			m_flip = new fluidCore::FlipSim(
						dimensions,
						densite,
						step_size,
						m_scene,
						verbose);

			m_flip->Init();
		}

		/* lance la simulation */
		m_flip->Step(false, false, false);

		auto particules = m_flip->GetParticles();

		for (auto particule : *particules) {
			m_corps.ajoute_point(particule->m_p.x, particule->m_p.y, particule->m_p.z);
		}
	}
#endif
};

/* ************************************************************************** */

void enregistre_operatrices_ariel(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceAriel>());
}
