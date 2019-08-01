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

	bool depend_sur_temps() const override
	{
		return true;
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

	bool depend_sur_temps() const override
	{
		return true;
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

		auto dt = evalue_decimal("dt");

		m_poseidon.dt = dt;

		psn::fill_grid(*m_poseidon.drapeaux, TypeFluid);

		/* init simulation */

		auto da = DonneesAval{};
		da.table.insere({ "poseidon", &m_poseidon });

		entree(0)->requiers_corps(contexte, &da);

		psn::ajourne_sources(m_poseidon, contexte.temps_courant);

		psn::ajourne_obstables(m_poseidon);

		/* lance simulation */
		entree(1)->requiers_corps(contexte, &da);

		/* sauve données */

		auto volume = memoire::loge<Volume>("Volume");
		volume->grille = m_poseidon.densite->copie();

		/* visualise domaine */
		auto etendu = m_poseidon.densite->etendu();
		auto taille_voxel = m_poseidon.densite->taille_voxel();

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
		dessine_boite(m_corps, attr_C, etendu.min, etendu.max, dls::math::vec3f(0.0f, 1.0f, 0.0f));
		dessine_boite(m_corps, attr_C, etendu.min, etendu.min + dls::math::vec3f(taille_voxel), dls::math::vec3f(0.0f, 1.0f, 0.0f));

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

		if (m_poseidon.densite != nullptr) {
			supprime_grilles();
		}

		auto res = evalue_entier("résolution");

		auto etendu = limites3f{};
		etendu.min = dls::math::vec3f(-5.0f, -1.0f, -5.0f);
		etendu.max = dls::math::vec3f( 5.0f,  9.0f,  5.0f);
		auto fenetre_donnees = etendu;
		auto taille_voxel = 10.0f / static_cast<float>(res);

		m_poseidon.densite = memoire::loge<Grille<float>>("grilles", etendu, fenetre_donnees, taille_voxel);

		if (m_poseidon.decouple) {
			taille_voxel *= 2.0f;
		}

		m_poseidon.pression = memoire::loge<Grille<float>>("grilles", etendu, fenetre_donnees, taille_voxel);
		m_poseidon.drapeaux = memoire::loge<Grille<int>>("grilles", etendu, fenetre_donnees, taille_voxel);
		m_poseidon.velocite = memoire::loge<GrilleMAC>("grilles", etendu, fenetre_donnees, taille_voxel);
	}

	void supprime_grilles()
	{
		memoire::deloge("grilles", m_poseidon.densite);
		memoire::deloge("grilles", m_poseidon.pression);
		memoire::deloge("grilles", m_poseidon.drapeaux);
		memoire::deloge("grilles", m_poseidon.velocite);
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

		auto vieille_vel = memoire::loge<GrilleMAC>("grilles", velocite->etendu(), velocite->fenetre_donnees(), velocite->taille_voxel());
		vieille_vel->copie_donnees(*velocite);

		psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *densite, poseidon_gaz->dt, ordre);

		psn::advecte_semi_lagrange(*drapeaux, *vieille_vel, *velocite, poseidon_gaz->dt, ordre);

		/* À FAIRE : plus de paramètres, voir MF. */
		psn::ajourne_conditions_bordures_murs(*drapeaux, *velocite);

		memoire::deloge("grilles", vieille_vel);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
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

	bool depend_sur_temps() const override
	{
		return true;
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

	bool depend_sur_temps() const override
	{
		return true;
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
