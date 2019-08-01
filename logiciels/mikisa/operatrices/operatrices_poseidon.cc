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

#include "operatrices_poseidon.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/echantillonnage_volume.hh"
#include "corps/iter_volume.hh"
#include "corps/volume.hh"

#include "evaluation/reseau.hh"

#include "poseidon/fluide.hh"
#include "poseidon/incompressibilite.hh"
#include "poseidon/monde.hh"
#include "poseidon/simulation.hh"

#include "outils_visualisation.hh"

/**
 * Publications utilisées pour élaborer le système :
 *
 * « Scalable fluid simulation in linear time on shared memory multiprocessors »
 * https://www.deepdyve.com/lp/association-for-computing-machinery/scalable-fluid-simulation-in-linear-time-on-shared-memory-Fr9a4RSL2d
 *
 * « Capturing Thin Features in Smoke Simulations »
 * http://library.imageworks.com/pdfs/imageworks-library-capturing-thin-features-in-smoke-simulation.pdf
 */

/* ************************************************************************** */

static inline auto extrait_poseidon(DonneesAval *da)
{
	return std::any_cast<psn::Poseidon *>(da->table["poseidon"]);
}

/* ************************************************************************** */

class OpEntreeGaz : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Entrée Gaz";
	static constexpr auto AIDE = "";

	explicit OpEntreeGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	OpEntreeGaz(OpEntreeGaz const &) = default;
	OpEntreeGaz &operator=(OpEntreeGaz const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_entree_gaz.jo";
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

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->nom);
			}
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Aucun objet sélectionné");
			return EXECUTION_ECHOUEE;
		}

		if (contexte.temps_courant > 100) {
			return EXECUTION_REUSSIE;
		}

		auto poseidon_gaz = extrait_poseidon(donnees_aval);

		if (poseidon_gaz->densite == nullptr) {
			this->ajoute_avertissement("La simulation n'est pas encore commencée");
			return EXECUTION_ECHOUEE;
		}

		for (auto const &params : poseidon_gaz->monde.sources) {
			if (params.objet == m_objet) {
				return EXECUTION_REUSSIE;
			}
		}

		auto dico_mode = dls::cree_dico(
					dls::paire{ dls::vue_chaine("addition"), psn::mode_fusion::ADDITION },
					dls::paire{ dls::vue_chaine("maximum"), psn::mode_fusion::MAXIMUM },
					dls::paire{ dls::vue_chaine("minimum"), psn::mode_fusion::MINIMUM },
					dls::paire{ dls::vue_chaine("multiplication"), psn::mode_fusion::MULTIPLICATION },
					dls::paire{ dls::vue_chaine("soustraction"), psn::mode_fusion::SOUSTRACTION },
					dls::paire{ dls::vue_chaine("superposition"), psn::mode_fusion::SUPERPOSITION }
					);

		auto params = psn::ParametresSource{};
		params.objet = m_objet;
		params.densite = evalue_decimal("densité");
		params.facteur = evalue_decimal("facteur");
		params.debut = evalue_entier("début");
		params.fin = evalue_entier("fin");

		auto plage_mode = dico_mode.trouve_binaire(evalue_enum("mode_fusion"));

		if (plage_mode.est_finie()) {
			this->ajoute_avertissement("Mode de fusion inconnu !");
			params.fusion = psn::mode_fusion::SUPERPOSITION;
		}
		else {
			params.fusion = plage_mode.front().second;
		}

		poseidon_gaz->monde.sources.pousse(params);

		return EXECUTION_REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("mode_fusion") == nullptr) {
			ajoute_propriete("mode_fusion", danjo::TypePropriete::ENUM, dls::chaine("superposition"));
			ajoute_propriete("facteur", danjo::TypePropriete::DECIMAL, 1.0f);
			ajoute_propriete("densité", danjo::TypePropriete::DECIMAL, 1.0f);
		}

		if (propriete("début") == nullptr) {
			ajoute_propriete("début", danjo::TypePropriete::ENTIER, 1);
			ajoute_propriete("fin", danjo::TypePropriete::ENTIER, 100);
		}
	}
};

/* ************************************************************************** */

class OpObstacleGaz : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Obstacle Gaz";
	static constexpr auto AIDE = "";

	explicit OpObstacleGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	OpObstacleGaz(OpObstacleGaz const &) = default;
	OpObstacleGaz &operator=(OpObstacleGaz const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_obstacle_gaz.jo";
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

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->nom);
			}
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Aucun objet sélectionné");
			return EXECUTION_ECHOUEE;
		}

		if (contexte.temps_courant > 100) {
			return EXECUTION_REUSSIE;
		}

		auto poseidon = extrait_poseidon(donnees_aval);

		if (poseidon->densite == nullptr) {
			this->ajoute_avertissement("La simulation n'est pas encore commencée");
			return EXECUTION_ECHOUEE;
		}

		poseidon->monde.obstacles.insere(m_objet);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

template <typename T>
auto est_vide(Grille<T> const &grille)
{
	for (auto i = 0; i < grille.nombre_voxels(); ++i) {
		if (grille.valeur(i) != T(0)) {
			return false;
		}
	}

	return true;
}

namespace psn {

/* Low order B-Spline. */
struct KernelBSP2 {
	static const int rayon = 1;

	static inline float poids(dls::math::vec3f const &v, float dx_inv)
	{
		auto r = longueur(v) * dx_inv;

		if (r > 1.0f) {
			return 0.0f;
		}

		return (1.0f - r);
	}
};

/* Fonction M'4 */
struct KernelMP4 {
	static const int rayon = 2;

	static inline float poids(dls::math::vec3f const &v, float dx_inv)
	{
		auto r = longueur(v) * dx_inv;

		if (r > 2.0f) {
			return 0.0f;
		}

		if (r >= 1.0f) {
			return 0.5f * (2.0f * r * r) * (1.0f - r);
		}

		return 1.0f - (2.5f * r * r) + (1.5f * r * r * r);
	}
};

void transfere_particules_grille(Poseidon &poseidon)
{
	auto densite = poseidon.densite;
	auto &grille_particules = poseidon.grille_particule;
	grille_particules = GrilleParticule(densite->desc());
	grille_particules.tri(poseidon.particules);

	for (auto i = 0; i < densite->nombre_voxels(); ++i) {
		densite->valeur(i) = 0.0f;
	}

	auto dx_inv = 1.0f / densite->taille_voxel();
	auto res = densite->resolution();

	using type_kernel = KernelBSP2;

	boucle_parallele(tbb::blocked_range<int>(0, res.z - 1),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto lims = limites3i{};
		lims.min = dls::math::vec3i(0, 0, plage.begin());
		lims.max = dls::math::vec3i(res.x - 1, res.y - 1, plage.end());

		auto iter = IteratricePosition(lims);

		while (!iter.fini()) {
			auto pos_index = iter.suivante();

			auto pos_monde = densite->index_vers_monde(pos_index);

			auto voisines = grille_particules.voisines_cellules(pos_index, dls::math::vec3i(type_kernel::rayon));

			/* utilise le filtre BSP2 */
			auto valeur = 0.0f;
			auto poids = 0.0f;

			for (auto pv : voisines) {
				auto r = type_kernel::poids(pos_monde - pv->pos, dx_inv);
				valeur += r * pv->densite;
				poids += r;
			}

			if (poids != 0.0f) {
				valeur /= poids;
			}

			densite->valeur(pos_index.x, pos_index.y, pos_index.z) = valeur;
		}
	});
}

auto calcul_vel_max(GrilleMAC const &vel)
{
	auto vel_max = 0.0f;

	for (auto i = 0; i < vel.nombre_voxels(); ++i) {
		vel_max = std::max(vel_max, longueur(vel.valeur(i)));
	}

	return vel_max;
}

auto calcul_dt(Poseidon &poseidon, float vel_max)
{
	/* Méthode de Mantaflow, limitant Dt selon le temps de l'image, où Dt se
	 * rapproche de 30 image/seconde (d'où les epsilon de 10^-4). */
	auto dt = poseidon.dt;
	auto cfl = poseidon.cfl;
	auto dt_min = poseidon.dt_min;
	auto dt_max = poseidon.dt_max;
	auto temps_par_frame = poseidon.temps_par_frame;
	auto duree_frame = poseidon.duree_frame;
	auto vel_max_dt = vel_max * poseidon.dt;

	if (!poseidon.verrouille_dt) {
		dt = std::max(std::min(dt * (cfl / (vel_max_dt + 1e-05f)), dt_max), dt_min);

		if ((temps_par_frame + dt * 1.05f) > duree_frame) {
			/* à 5% d'une durée d'image ? ajoute epsilon pour prévenir les
			 * erreurs d'arrondissement... */
			dt = (duree_frame - temps_par_frame) + 1e-04f;
		}
		else if ((temps_par_frame + dt + dt_min) > duree_frame || (temps_par_frame + (dt * 1.25f)) > duree_frame) {
			/* évite les petits pas ainsi que ceux avec beaucoup de variance,
			 * donc divisons par 2 pour en faire des moyens au besoin */
			dt = (duree_frame - temps_par_frame + 1e-04f) * 0.5f;
			poseidon.verrouille_dt = true;
		}
	}

	poseidon.dt = dt;

#if 0
	/* Méthode de Bridson, limitant Dt selon la vélocité max pour éviter qu'il
	 * n'y ait un déplacement de plus de 5 cellules lors des advections.
	 * "Fluid Simulation Course Notes", SIGGRAPH 2007.
	 * https://www.cs.ubc.ca/~rbridson/fluidsimulation/fluids_notes.pdf
	 *
	 * Ancien code gardé pour référence.
	 */
	auto const factor = poseidon.cfl;
	auto const dh = 5.0f / static_cast<float>(poseidon.resolution);
	auto const vel_max_ = std::max(1e-16f, std::max(dh, vel_max));

	/* dt <= (5dh / max_u) */
	poseidon.dt = std::min(poseidon.dt, factor * dh / std::sqrt(vel_max_));
#endif
}

}

class OpSimulationGaz : public OperatriceCorps {
	psn::Poseidon m_poseidon{};

public:
	static constexpr auto NOM = "Simulation Gaz";
	static constexpr auto AIDE = "";

	explicit OpSimulationGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
	}

	~OpSimulationGaz() override
	{
		supprime_grilles();
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_simulation_gaz.jo";
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
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		/* init domaine */
		auto temps_debut = evalue_entier("début");
		auto temps_fin = evalue_entier("fin");

		if (contexte.temps_courant < temps_debut || contexte.temps_courant > temps_fin) {
			return EXECUTION_REUSSIE;
		}

		m_poseidon.decouple = evalue_bool("découple");

		if (contexte.temps_courant == temps_debut) {
			reinitialise();
		}

		auto dt_adaptif = evalue_bool("dt_adaptif");

		m_poseidon.dt_min = evalue_decimal("dt_min");
		m_poseidon.dt_max = evalue_decimal("dt_max");
		m_poseidon.cfl = evalue_decimal("cfl");
		m_poseidon.duree_frame = evalue_decimal("durée_frame");
		m_poseidon.dt = (m_poseidon.dt_min + m_poseidon.dt_max) * 0.5f;

		psn::fill_grid(*m_poseidon.drapeaux, TypeFluid);

		/* init simulation */

		auto da = DonneesAval{};
		da.table.insere({ "poseidon", &m_poseidon });

		entree(0)->requiers_corps(contexte, &da);

		psn::ajourne_sources(m_poseidon, contexte.temps_courant);

		psn::ajourne_obstables(m_poseidon);

		psn::transfere_particules_grille(m_poseidon);

		/* lance simulation */
		while (true) {
			if (dt_adaptif) {
				auto vel_max = psn::calcul_vel_max(*m_poseidon.velocite);
				calcul_dt(m_poseidon, vel_max);

				if (m_poseidon.dt <= (m_poseidon.dt_min / 2.0f)) {
					this->ajoute_avertissement("Dt invalide, ne devrais pas arriver !");
				}
			}

			entree(1)->requiers_corps(contexte, &da);

			m_poseidon.temps_par_frame += m_poseidon.dt;
			m_poseidon.temps_total     += m_poseidon.dt;

			if ((m_poseidon.temps_par_frame + 1e-6f) > m_poseidon.duree_frame) {
				m_poseidon.image += 1;
				/* Recalcul le temps temps pour éviter les erreurs inhérentes à
				 * l'accumulation de nombre décimaux. */
				m_poseidon.temps_total = static_cast<float>(m_poseidon.image) * m_poseidon.duree_frame;
				m_poseidon.temps_par_frame = 0.0f;
				m_poseidon.verrouille_dt = false;
				break;
			}
		}

		/* sauve données */

		auto volume = memoire::loge<Volume>("Volume");
		volume->grille = m_poseidon.densite->copie();

		/* visualise domaine */
		auto etendu = m_poseidon.densite->etendu();
		auto taille_voxel = m_poseidon.densite->taille_voxel();

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		dessine_boite(m_corps, attr_C, etendu.min, etendu.max, dls::math::vec3f(0.0f, 1.0f, 0.0f));
		dessine_boite(m_corps, attr_C, etendu.min, etendu.min + dls::math::vec3f(taille_voxel), dls::math::vec3f(0.0f, 1.0f, 0.0f));

		for (auto p : m_poseidon.particules) {
			m_corps.ajoute_point(p->pos);
			attr_C->pousse(dls::math::vec3f(0.435f, 0.284f, 0.743f));
		}

		m_corps.ajoute_primitive(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}

	void parametres_changes() override
	{
		reinitialise();
	}

	void amont_change(PriseEntree *prise) override
	{
		if (prise == nullptr) {
			return;
		}

		if (prise == entree(0)->pointeur()) {
			reinitialise();
		}
	}

	void reinitialise()
	{
		m_poseidon.monde.sources.efface();
		m_poseidon.monde.obstacles.efface();
		m_poseidon.temps_par_frame = 0.0f;
		m_poseidon.temps_total = 0.0f;
		m_poseidon.verrouille_dt = false;
		m_poseidon.image = 0;

		if (m_poseidon.densite != nullptr) {
			supprime_grilles();
		}

		auto res = evalue_entier("résolution");
		m_poseidon.resolution = res;

		auto desc = description_volume{};
		desc.etendues.min = dls::math::vec3f(-5.0f, -1.0f, -5.0f);
		desc.etendues.max = dls::math::vec3f( 5.0f,  9.0f,  5.0f);
		desc.fenetre_donnees = desc.etendues;
		desc.taille_voxel = 10.0f / static_cast<float>(res);

		m_poseidon.densite = memoire::loge<Grille<float>>("grilles", desc);

		if (m_poseidon.decouple) {
			desc.taille_voxel *= 2.0f;
		}

		m_poseidon.pression = memoire::loge<Grille<float>>("grilles", desc);
		m_poseidon.drapeaux = memoire::loge<Grille<int>>("grilles", desc);
		m_poseidon.velocite = memoire::loge<GrilleMAC>("grilles", desc);
	}

	void supprime_grilles()
	{
		memoire::deloge("grilles", m_poseidon.densite);
		memoire::deloge("grilles", m_poseidon.pression);
		memoire::deloge("grilles", m_poseidon.drapeaux);
		memoire::deloge("grilles", m_poseidon.velocite);

		m_poseidon.supprime_particules();
	}

	void performe_versionnage() override
	{
		if (propriete("résolution") == nullptr) {
			ajoute_propriete("résolution", danjo::TypePropriete::ENTIER, 32);
			ajoute_propriete("début", danjo::TypePropriete::ENTIER, 1);
			ajoute_propriete("fin", danjo::TypePropriete::ENTIER, 250);
			ajoute_propriete("dt", danjo::TypePropriete::DECIMAL, 0.1f);
		}

		if (propriete("découple") == nullptr) {
			ajoute_propriete("découple", danjo::TypePropriete::BOOL, false);
		}

		if (propriete("dt_adaptif") == nullptr) {
			ajoute_propriete("dt_adaptif", danjo::TypePropriete::BOOL, false);
			ajoute_propriete("dt_min", danjo::TypePropriete::DECIMAL, 0.2f);
			ajoute_propriete("dt_max", danjo::TypePropriete::DECIMAL, 2.0f);
			ajoute_propriete("durée_frame", danjo::TypePropriete::DECIMAL, 1.0f);
			ajoute_propriete("cfl", danjo::TypePropriete::DECIMAL, 3.0f);
		}
	}
};

/* ************************************************************************** */

class OpAdvectionGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Advection Gaz";
	static constexpr auto AIDE = "";

	explicit OpAdvectionGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_advection_gaz.jo";
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

		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto ordre = (evalue_enum("ordre") == "semi_lagrangienne") ? 1 : 0;
		//auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		auto vieille_vel = memoire::loge<GrilleMAC>("grilles", velocite->desc());
		vieille_vel->copie_donnees(*velocite);

		/* advecte particules */
		auto mult = poseidon_gaz->dt * velocite->taille_voxel();
		tbb::parallel_for(0l, poseidon_gaz->particules.taille(),
						  [&](long i)
		{
			auto p = poseidon_gaz->particules[i];
			auto pos = velocite->monde_vers_index(p->pos);
			p->pos += velocite->valeur_centree(pos) * mult;
		});

		//psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *densite, poseidon_gaz->dt, ordre);

		psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *velocite, poseidon_gaz->dt, ordre);

		/* À FAIRE : plus de paramètres, voir MF. */
		psn::ajourne_conditions_bordures_murs(*drapeaux, *velocite);

		memoire::deloge("grilles", vieille_vel);

		return EXECUTION_REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("ordre") == nullptr) {
			ajoute_propriete("ordre", danjo::TypePropriete::ENUM, dls::chaine("semi_lagrangienne"));
		}
	}
};

/* ************************************************************************** */

class OpFlottanceGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Flottance Gaz";
	static constexpr auto AIDE = "";

	explicit OpFlottanceGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_flottance_gaz.jo";
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

		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto coefficient = evalue_decimal("coefficient");
		auto gravite_y = evalue_decimal("gravité");
		auto gravite = dls::math::vec3f(0.0f, -gravite_y, 0.0f);
		auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;
		auto densite_basse = Grille<float>();

		if (poseidon_gaz->decouple) {
			/* rééchantillone la densité pour être alignée avec la vélocité */
			densite_basse = reechantillonne(*densite, velocite->taille_voxel());
			densite = &densite_basse;
		}

		psn::ajoute_flottance(*densite, *velocite, *drapeaux, gravite, poseidon_gaz->dt, coefficient);

		return EXECUTION_REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("gravité") == nullptr) {
			ajoute_propriete("gravité", danjo::TypePropriete::DECIMAL, 1.0f);
			ajoute_propriete("coefficient", danjo::TypePropriete::DECIMAL, 1.0f);
		}
	}
};

/* ************************************************************************** */

class OpIncompressibiliteGaz : public OperatriceCorps {
public:
	static constexpr auto NOM = "Incompressibilité Gaz";
	static constexpr auto AIDE = "";

	explicit OpIncompressibiliteGaz(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		m_execute_toujours = true;
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

		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		std::cerr << "------------------------------------------------\n";
		std::cerr << "Incompressibilité, image " << contexte.temps_courant << '\n';

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto pression = poseidon_gaz->pression;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		psn::projette_velocite(*velocite, *pression, *drapeaux);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_poseidon(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpEntreeGaz>());
	usine.enregistre_type(cree_desc<OpObstacleGaz>());
	usine.enregistre_type(cree_desc<OpSimulationGaz>());
	usine.enregistre_type(cree_desc<OpAdvectionGaz>());
	usine.enregistre_type(cree_desc<OpFlottanceGaz>());
	usine.enregistre_type(cree_desc<OpIncompressibiliteGaz>());
}
