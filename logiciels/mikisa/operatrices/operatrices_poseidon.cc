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
#include "biblinternes/outils/empreintes.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/volume.hh"

#include "evaluation/reseau.hh"

#include "poseidon/diffusion.hh"
#include "poseidon/fluide.hh"
#include "poseidon/incompressibilite.hh"
#include "poseidon/monde.hh"
#include "poseidon/simulation.hh"
#include "poseidon/vorticite.hh"

#include "wolika/filtre_3d.hh"
#include "wolika/grille_dense.hh"

#include "outils_visualisation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/**
 * Publications utilisées pour élaborer le système :
 *
 * « Scalable fluid simulation in linear time on shared memory multiprocessors »
 * https://www.deepdyve.com/lp/association-for-computing-machinery/scalable-fluid-simulation-in-linear-time-on-shared-memory-Fr9a4RSL2d
 *
 * « Capturing Thin Features in Smoke Simulations »
 * http://library.imageworks.com/pdfs/imageworks-library-capturing-thin-features-in-smoke-simulation.pdf
 *
 * « "Megamind" : Fire, Smoke and Data »
 * https://research.dreamworks.com/wp-content/uploads/2018/07/57-rost-Edited.pdf
 */

/**
 * Champs à considérer :
 * - divergence
 * - oxygène (pour controler là ou le feu se forme)
 * - couleur (https://research.dreamworks.com/wp-content/uploads/2018/07/a38-yoon-Edited.pdf)
 *
 * Opératrices à considérer :
 * - (FumeFX) turbulence vélocité
 * - réanimation cache (volume temporel, réadvection)
 * - visualisation (tranche des champs, etc.)
 * - (FumeFX, Blender) haute-résolution/turbulence ondelette, via cache
 *
 * Options simulation :
 * - (FumeFX, Blender) fioul.
 * - amortissement vélocité
 * - (Blender) dissipation des champs de simulations (linéaire/log)
 */

/* ************************************************************************** */

static inline auto extrait_poseidon(DonneesAval *da)
{
	return std::any_cast<psn::Poseidon *>(da->table["poseidon"]);
}

/* ************************************************************************** */

class OpEntreeGaz final : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Entrée Gaz";
	static constexpr auto AIDE = "";

	OpEntreeGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud_reseau, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->noeud->nom);
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
		params.temperature = evalue_decimal("température");
		params.fioul = evalue_decimal("fioul");
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

		if (propriete("température") == nullptr) {
			ajoute_propriete("température", danjo::TypePropriete::DECIMAL, 1.0f);
			ajoute_propriete("fioul", danjo::TypePropriete::DECIMAL, 1.0f);
		}
	}
};

/* ************************************************************************** */

class OpObstacleGaz final : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Obstacle Gaz";
	static constexpr auto AIDE = "";

	OpObstacleGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud_reseau, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_objet") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->noeud->nom);
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

class OpSimulationGaz final : public OperatriceCorps {
	psn::Poseidon m_poseidon{};
	int m_derniere_temps = -1;
	REMBOURRE(4);

public:
	static constexpr auto NOM = "Simulation Gaz";
	static constexpr auto AIDE = "";

	OpSimulationGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

		auto da = DonneesAval{};
		da.table.insere({ "poseidon", &m_poseidon });

		/* passe poseidon aux opératrices en aval par exemple pour les
		 * visualisations */
		if (donnees_aval) {
			donnees_aval->table.insere({ "poseidon", &m_poseidon });
		}

		if (contexte.temps_courant == temps_debut) {
			reinitialise();
		}		
		else if (contexte.temps_courant != m_derniere_temps + 1) {
			return EXECUTION_REUSSIE;
		}

		auto dt_adaptif = evalue_bool("dt_adaptif");

		m_poseidon.decouple = evalue_bool("découple");
		m_poseidon.dt_min = evalue_decimal("dt_min");
		m_poseidon.dt_max = evalue_decimal("dt_max");
		m_poseidon.cfl = evalue_decimal("cfl");
		m_poseidon.duree_frame = evalue_decimal("durée_frame");
		m_poseidon.dt = (m_poseidon.dt_min + m_poseidon.dt_max) * 0.5f;
		m_poseidon.solveur_flip = evalue_bool("solveur_flip");

		psn::fill_grid(*m_poseidon.drapeaux, TypeFluid);

		/* init simulation */

		entree(0)->requiers_corps(contexte, &da);

		if (m_poseidon.solveur_flip) {
			/* commence par trier les particules, car nous aurons besoin d'une
			 * grille triée pour vérifier l'insertion de particules */
			m_poseidon.grille_particule.tri(m_poseidon.parts);
		}

		psn::ajourne_sources(m_poseidon, contexte.temps_courant);

		psn::ajourne_obstables(m_poseidon);

		if (m_poseidon.solveur_flip) {
			psn::transfere_particules_grille(m_poseidon);
		}

		/* lance simulation */
		while (true) {
			if (dt_adaptif) {
				auto vel_max = psn::calcul_vel_max(*m_poseidon.velocite);
				calcul_dt(m_poseidon, vel_max);

				if (m_poseidon.dt <= (m_poseidon.dt_min / 2.0f)) {
					this->ajoute_avertissement("Dt invalide, ne devrait pas arriver !");
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

		m_derniere_temps = contexte.temps_courant;

		/* sauve données */

		auto volume = memoire::loge<Volume>("Volume", m_poseidon.densite->copie());

		/* visualise domaine */
		auto etendu = m_poseidon.densite->desc().etendue;
		auto taille_voxel = static_cast<float>(m_poseidon.densite->desc().taille_voxel);

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);
		dessine_boite(m_corps, attr_C, etendu.min, etendu.max, dls::math::vec3f(0.0f, 1.0f, 0.0f));
		dessine_boite(m_corps, attr_C, etendu.min, etendu.min + dls::math::vec3f(taille_voxel), dls::math::vec3f(0.0f, 1.0f, 0.0f));

		auto pos_parts = m_poseidon.parts.champs_vectoriel("position");

		for (auto i = 0; i < m_poseidon.parts.taille(); ++i) {
			auto idx_p = m_corps.ajoute_point(pos_parts[i]);
			assigne(attr_C->r32(idx_p), dls::math::vec3f(0.435f, 0.284f, 0.743f));
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
		m_poseidon.solveur_flip = evalue_bool("solveur_flip");

		if (m_poseidon.densite != nullptr) {
			supprime_grilles();
		}

		auto res = evalue_entier("résolution");
		m_poseidon.resolution = res;

		auto desc = wlk::desc_grille_3d{};
		desc.etendue.min = dls::math::vec3f(-5.0f, -1.0f, -5.0f);
		desc.etendue.max = dls::math::vec3f( 5.0f,  9.0f,  5.0f);
		desc.fenetre_donnees = desc.etendue;
		desc.taille_voxel = 10.0 / static_cast<double>(res);

		m_poseidon.densite = memoire::loge<wlk::grille_dense_3d<float>>("grilles", desc);
		m_poseidon.temperature = memoire::loge<wlk::grille_dense_3d<float>>("grilles", desc);

		if (m_poseidon.solveur_flip) {
			m_poseidon.grille_particule = psn::GrilleParticule(desc);
		}

		if (m_poseidon.decouple) {
			desc.taille_voxel *= 2.0;
		}

		m_poseidon.pression = memoire::loge<wlk::grille_dense_3d<float>>("grilles", desc);
		m_poseidon.drapeaux = memoire::loge<wlk::grille_dense_3d<int>>("grilles", desc);

		m_poseidon.velocite = memoire::loge<wlk::GrilleMAC>("grilles", desc);

		m_poseidon.parts = psn::particules::construit_systeme_gaz();
	}

	void supprime_grilles()
	{
		memoire::deloge("grilles", m_poseidon.densite);
		memoire::deloge("grilles", m_poseidon.pression);
		memoire::deloge("grilles", m_poseidon.drapeaux);
		memoire::deloge("grilles", m_poseidon.oxygene);
		memoire::deloge("grilles", m_poseidon.temperature);
		memoire::deloge("grilles", m_poseidon.fioul);
		memoire::deloge("grilles", m_poseidon.velocite);
		memoire::deloge("grilles", m_poseidon.densite_prev);
		memoire::deloge("grilles", m_poseidon.temperature_prev);
		memoire::deloge("grilles", m_poseidon.velocite_prev);

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

		if (propriete("solveur_flip") == nullptr) {
			ajoute_propriete("solveur_flip", danjo::TypePropriete::BOOL, false);
		}
	}
};

/* ************************************************************************** */

class OpAdvectionGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Advection Gaz";
	static constexpr auto AIDE = "";

	OpAdvectionGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
		auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		auto vieille_vel = memoire::loge<wlk::GrilleMAC>("grilles", velocite->desc());
		vieille_vel->copie_donnees(*velocite);

		if (poseidon_gaz->solveur_flip) {
			/* advecte particules */
			auto echant = wlk::Echantilloneuse(*velocite);
			auto mult = poseidon_gaz->dt * static_cast<float>(velocite->desc().taille_voxel);

			auto pos_parts = poseidon_gaz->parts.champs_vectoriel("position");

			tbb::parallel_for(0l, poseidon_gaz->parts.taille(),
							  [&](long i)
			{
				auto &p = pos_parts[i];
				auto pos = velocite->monde_vers_continu(p);

				p += echant.echantillone_trilineaire(pos) * mult;
			});
		}
		else {
			psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *densite, poseidon_gaz->dt, ordre);

			if (poseidon_gaz->temperature) {
				psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *poseidon_gaz->temperature, poseidon_gaz->dt, ordre);
			}

			if (poseidon_gaz->fioul) {
				psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *poseidon_gaz->fioul, poseidon_gaz->dt, ordre);
			}

			if (poseidon_gaz->oxygene) {
				psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *poseidon_gaz->oxygene, poseidon_gaz->dt, ordre);
			}
		}

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

class OpFlottanceGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Flottance Gaz";
	static constexpr auto AIDE = "";

	OpFlottanceGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
		auto alpha = evalue_decimal("alpha");
		auto beta = evalue_decimal("beta");
		auto temperature_ambiante = evalue_decimal("température");
		auto gravite = dls::math::vec3f(0.0f, -gravite_y, 0.0f);
		auto densite = poseidon_gaz->densite;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;
		auto temperature = poseidon_gaz->temperature;
		auto densite_basse = wlk::grille_dense_3d<float>();

		/* converti la gravité à l'espace domaine */
		// auto mag = longueur(gravite);
		// gravite = densite->monde_vers_unit(gravite);
		// gravite = normalise(gravite);
		// gravite *= magnitude;

		if (poseidon_gaz->decouple) {
			/* rééchantillone la densité pour être alignée avec la vélocité */
			densite_basse = reechantillonne(*densite, velocite->desc().taille_voxel);
			densite = &densite_basse;
		}

		psn::ajoute_flottance(
					*densite,
					*velocite,
					*drapeaux,
					temperature,
					gravite,
					alpha,
					beta,
					temperature_ambiante,
					poseidon_gaz->dt,
					coefficient);

		return EXECUTION_REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("gravité") == nullptr) {
			ajoute_propriete("gravité", danjo::TypePropriete::DECIMAL, 1.0f);
			ajoute_propriete("coefficient", danjo::TypePropriete::DECIMAL, 1.0f);
		}

		if (propriete("alpha") == nullptr) {
			ajoute_propriete("alpha", danjo::TypePropriete::DECIMAL, 0.001f);
			ajoute_propriete("beta", danjo::TypePropriete::DECIMAL, 0.1f);
			ajoute_propriete("température", danjo::TypePropriete::DECIMAL, 0.0f);
		}
	}
};

/* ************************************************************************** */

class OpIncompressibiliteGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Incompressibilité Gaz";
	static constexpr auto AIDE = "";

	OpIncompressibiliteGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_projection_gaz.jo";
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

		auto const iterations = evalue_entier("itérations");
		auto const precision = 1.0f / std::pow(10.0f, static_cast<float>(evalue_entier("précision_")));

		std::cerr << "------------------------------------------------\n";
		std::cerr << "Incompressibilité, image " << contexte.temps_courant << '\n';

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto pression = poseidon_gaz->pression;
		auto velocite = poseidon_gaz->velocite;
		auto drapeaux = poseidon_gaz->drapeaux;

		psn::projette_velocite(*velocite, *pression, *drapeaux, iterations, precision);

		return EXECUTION_REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("itérations") == nullptr) {
			ajoute_propriete("itérations", danjo::TypePropriete::ENTIER, 100);
			ajoute_propriete("précision_", danjo::TypePropriete::ENTIER, 6);
		}
	}
};

/* ************************************************************************** */

class OpVorticiteGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Vorticité Gaz";
	static constexpr auto AIDE = "";

	OpVorticiteGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_vorticite_gaz.jo";
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

		auto quantite_vorticite = evalue_decimal("quantité", contexte.temps_courant);
		auto vorticite_flamme = evalue_decimal("vort_flamme", contexte.temps_courant);

		psn::ajoute_vorticite(
					*poseidon_gaz,
					quantite_vorticite,
					vorticite_flamme,
					poseidon_gaz->dt);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* Idée tirée de FumeFX, à voir à quelle étape il faut faire cet affinage
 * XXX - instable? */
class OpAffinageGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Affinage Gaz";
	static constexpr auto AIDE = "";

	OpAffinageGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_affinage_gaz.jo";
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

		auto const quantite_vel = evalue_decimal("quantité_vel", contexte.temps_courant);
		auto const rayon_vel = evalue_decimal("rayon_vel", contexte.temps_courant);

		if (quantite_vel != 0.0f) {
			auto velocite = poseidon_gaz->velocite;
			wlk::affine_grille(*velocite, rayon_vel, quantite_vel);
		}

		auto const quantite_fum = evalue_decimal("quantité_fum", contexte.temps_courant);
		auto const rayon_fum = evalue_decimal("rayon_fum", contexte.temps_courant);

		if (quantite_fum != 0.0f) {
			auto densite = poseidon_gaz->densite;
			wlk::affine_grille(*densite, rayon_fum, quantite_fum);
		}

		auto const quantite_tmp = evalue_decimal("quantité_tmp", contexte.temps_courant);
		auto const rayon_tmp = evalue_decimal("rayon_tmp", contexte.temps_courant);

		if (quantite_tmp != 0.0f) {
			auto temperature = poseidon_gaz->temperature;
			wlk::affine_grille(*temperature, rayon_tmp, quantite_tmp);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpDiffusionGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Diffusion Gaz";
	static constexpr auto AIDE = "";

	OpDiffusionGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_diffusion_gaz.jo";
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
		auto drapeaux = poseidon_gaz->drapeaux;
		auto const dt = poseidon_gaz->dt;
		auto const iterations = evalue_entier("itérations");
		auto const precision = 1.0f / std::pow(10.0f, static_cast<float>(evalue_entier("précision_")));

		auto const diff_fum = evalue_decimal("diff_fum", contexte.temps_courant);

		if (diff_fum != 0.0f) {
			if (poseidon_gaz->densite_prev == nullptr) {
				poseidon_gaz->densite_prev = memoire::loge<wlk::grille_dense_3d<float>>("grilles", poseidon_gaz->densite->desc());
			}

			std::swap(poseidon_gaz->densite, poseidon_gaz->densite_prev);

			auto fumee = poseidon_gaz->densite;
			auto fumee_prev = poseidon_gaz->densite_prev;

			psn::diffuse(*fumee, *fumee_prev, *drapeaux, iterations, precision, diff_fum, dt);
		}

		auto const diff_tmp = evalue_decimal("diff_tmp", contexte.temps_courant);

		if (diff_tmp != 0.0f) {
			if (poseidon_gaz->temperature_prev == nullptr) {
				poseidon_gaz->temperature_prev = memoire::loge<wlk::grille_dense_3d<float>>("grilles", poseidon_gaz->temperature->desc());
			}

			std::swap(poseidon_gaz->temperature, poseidon_gaz->temperature_prev);

			auto temperature = poseidon_gaz->temperature;
			auto temperature_prev = poseidon_gaz->temperature_prev;

			psn::diffuse(*temperature, *temperature_prev, *drapeaux, iterations, precision, diff_tmp, dt);
		}

		auto const diff_oxy = evalue_decimal("diff_oxy", contexte.temps_courant);

		if (diff_oxy != 0.0f) {
			if (poseidon_gaz->oxygene_prev == nullptr) {
				poseidon_gaz->oxygene_prev = memoire::loge<wlk::grille_dense_3d<float>>("grilles", poseidon_gaz->oxygene->desc());
			}

			std::swap(poseidon_gaz->oxygene, poseidon_gaz->oxygene_prev);

			auto oxygene = poseidon_gaz->oxygene;
			auto oxygene_prev = poseidon_gaz->oxygene_prev;

			psn::diffuse(*oxygene, *oxygene_prev, *drapeaux, iterations, precision, diff_oxy, dt);
		}

#if 0  /* À FAIRE : sépare vélocité */
		auto const diff_vel = evalue_decimal("diff_vel", contexte.temps_courant);

		if (diff_tmp != 0.0f) {
			if (poseidon_gaz->velocite_prev == nullptr) {
				poseidon_gaz->velocite_prev = memoire::loge<wlk::GrilleMAC>("grilles", poseidon_gaz->velocite->desc());
			}

			std::swap(poseidon_gaz->velocite, poseidon_gaz->velocite_prev);

			auto temperature = poseidon_gaz->velocite;
			auto temperature_prev = poseidon_gaz->velocite_prev;

			psn::diffuse(*temperature, *temperature_prev, *drapeaux, iterations, precision, diff_tmp, dt);
		}
#endif

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpBruitCollisionGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Bruit Collision Gaz";
	static constexpr auto AIDE = "Supprime des quantités du fluide en simulant un bruit blanc dans le champs de collision.";

	OpBruitCollisionGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_bruit_collision_gaz.jo";
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

		auto const quantite = evalue_decimal("quantité");
		auto graine = evalue_entier("graine");
		auto const anime_graine = evalue_bool("anime_graine");

		if (anime_graine) {
			graine += contexte.temps_courant;
		}

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto fumee = poseidon_gaz->densite;

		for (auto i = 0; i < fumee->nombre_elements(); ++i) {
			fumee->valeur(i) *= (1.0f - quantite * empreinte_n32_vers_r32(static_cast<unsigned>(graine + i)));
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpDissipationGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Dissipation Gaz";
	static constexpr auto AIDE = "Supprime des quantités du fluide.";

	OpDissipationGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dissipation_gaz.jo";
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

		auto const vit_fumee = 1.0f - evalue_decimal("vit_fumée") / 100.0f;
		auto const min_fumee = evalue_decimal("min_fumée");

		auto const vit_theta = 1.0f - evalue_decimal("vit_theta") / 100.0f;
		auto const min_theta = evalue_decimal("min_theta");

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);
		auto fumee = poseidon_gaz->densite;
		auto theta = poseidon_gaz->temperature;

		for (auto i = 0; i < fumee->nombre_elements(); ++i) {
			if (fumee->valeur(i) < min_fumee) {
				fumee->valeur(i) *= vit_fumee;
			}

			if (theta && theta->valeur(i) < min_theta) {
				theta->valeur(i) *= vit_theta;
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpVisualisationGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualisation Gaz";
	static constexpr auto AIDE = "Visualise un champs donné de la simulation.";

	OpVisualisationGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_visualisation_gaz.jo";
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

		/* cherche la simulation. */
		auto da = DonneesAval{};
		entree(0)->requiers_corps(contexte, &da);

		if (!da.possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en amont.");
			return EXECUTION_ECHOUEE;
		}

		auto const champs = evalue_enum("champs");
		auto const taille_vel = evalue_decimal("taille_vel");

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(&da);

		auto volume = static_cast<Volume *>(nullptr);

		if (champs == "divergence") {
			if (poseidon_gaz->divergence == nullptr) {
				this->ajoute_avertissement("Le champs 'divergence' n'est pas actif");
				return EXECUTION_ECHOUEE;
			}

			volume = memoire::loge<Volume>("Volume", poseidon_gaz->divergence->copie());
		}
		else if (champs == "fumée") {
			volume = memoire::loge<Volume>("Volume", poseidon_gaz->densite->copie());
		}
		else if (champs == "fioul") {
			if (poseidon_gaz->fioul == nullptr) {
				this->ajoute_avertissement("Le champs 'fioul' n'est pas actif");
				return EXECUTION_ECHOUEE;
			}

			volume = memoire::loge<Volume>("Volume", poseidon_gaz->fioul->copie());
		}
		else if (champs == "oxygène") {
			if (poseidon_gaz->oxygene == nullptr) {
				this->ajoute_avertissement("Le champs 'oxygène' n'est pas actif");
				return EXECUTION_ECHOUEE;
			}

			volume = memoire::loge<Volume>("Volume", poseidon_gaz->oxygene->copie());
		}
		else if (champs == "pression") {
			volume = memoire::loge<Volume>("Volume", poseidon_gaz->pression->copie());
		}
		else if (champs == "température") {
			if (poseidon_gaz->temperature == nullptr) {
				this->ajoute_avertissement("Le champs 'température' n'est pas actif");
				return EXECUTION_ECHOUEE;
			}

			volume = memoire::loge<Volume>("Volume", poseidon_gaz->temperature->copie());
		}
		else if (champs == "vélocité") {
			auto velocite = poseidon_gaz->velocite;
			auto res = velocite->desc().resolution;

			auto max_lng = 0.0f;

			for (auto z = 0; z < res.z; ++z) {
				for (auto y = 0; y < res.y; ++y) {
					for (auto x = 0; x < res.x; ++x) {
						auto vel = velocite->valeur_centree(x, y, z);
						max_lng = std::max(max_lng, longueur(vel));
					}
				}
			}

			auto C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::PRIMITIVE);

			for (auto z = 0; z < res.z; ++z) {
				for (auto y = 0; y < res.y; ++y) {
					for (auto x = 0; x < res.x; ++x) {
						auto co = dls::math::vec3i(x, y, z);

						auto vel = velocite->valeur_centree(x, y, z);
						auto lng = longueur(vel);
						auto clr = dls::phys::couleur_depuis_poids(lng / max_lng);

						if (lng != 0.0f) {
							vel /= lng;
							lng *= taille_vel;
						}

						lng *= static_cast<float>(velocite->desc().taille_voxel);
						lng *= poseidon_gaz->dt;

						auto pos_monde = velocite->index_vers_monde(co);

						auto p0 = pos_monde;
						auto p1 = p0 + vel * lng;

						auto idx0 = m_corps.ajoute_point(p0);
						auto idx1 = m_corps.ajoute_point(p1);

						auto poly = m_corps.ajoute_polygone(type_polygone::OUVERT, 2);
						m_corps.ajoute_sommet(poly, idx0);
						m_corps.ajoute_sommet(poly, idx1);

						assigne(C->r32(poly->index), dls::math::vec3f(clr.r, clr.v, clr.b));
					}
				}
			}
		}

		if (volume) {
			m_corps.ajoute_primitive(volume);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpErosionGaz final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Érosion Gaz";
	static constexpr auto AIDE = "Érode les champs donnés de la simulation.";

	OpErosionGaz(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_poseidon_erosion.jo";
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
		if (!donnees_aval || !donnees_aval->possede("poseidon")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return EXECUTION_ECHOUEE;
		}

		/* accumule les entrées */
		entree(0)->requiers_corps(contexte, donnees_aval);

		/* passe à notre exécution */
		auto poseidon_gaz = extrait_poseidon(donnees_aval);

		auto rayon = evalue_entier("rayon");
		auto erode_densite = evalue_bool("érode_densité");
		auto erode_temperature = evalue_bool("érode_température");
		auto erode_fioul = evalue_bool("érode_fioul");
		auto erode_divergence = evalue_bool("érode_divergence");
		auto erode_oxygene = evalue_bool("érode_oxygène");
		auto erode_velocite = evalue_bool("érode_vélocité");

		auto chef = contexte.chef;
		auto chef_wolika = ChefWolika(chef, "érosion poséidon");

		if (erode_densite) {
			wlk::erode_grille(*poseidon_gaz->densite, rayon, &chef_wolika);
		}

		if (erode_temperature && poseidon_gaz->temperature != nullptr) {
			wlk::erode_grille(*poseidon_gaz->temperature, rayon, &chef_wolika);
		}

		if (erode_fioul && poseidon_gaz->fioul != nullptr) {
			wlk::erode_grille(*poseidon_gaz->fioul, rayon, &chef_wolika);
		}

		if (erode_divergence && poseidon_gaz->divergence != nullptr) {
			wlk::erode_grille(*poseidon_gaz->divergence, rayon, &chef_wolika);
		}

		if (erode_oxygene && poseidon_gaz->oxygene != nullptr) {
			wlk::erode_grille(*poseidon_gaz->oxygene, rayon, &chef_wolika);
		}

		if (erode_velocite) {
			wlk::erode_grille(*poseidon_gaz->velocite, rayon, &chef_wolika);
		}

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
	usine.enregistre_type(cree_desc<OpVorticiteGaz>());
	usine.enregistre_type(cree_desc<OpAffinageGaz>());
	usine.enregistre_type(cree_desc<OpDiffusionGaz>());
	usine.enregistre_type(cree_desc<OpBruitCollisionGaz>());
	usine.enregistre_type(cree_desc<OpDissipationGaz>());
	usine.enregistre_type(cree_desc<OpVisualisationGaz>());
	usine.enregistre_type(cree_desc<OpErosionGaz>());
}

#pragma clang diagnostic pop
