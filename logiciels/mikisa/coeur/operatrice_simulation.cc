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

#include "operatrice_simulation.hh"

#include "contexte_evaluation.hh"
#include "donnees_simulation.hh"
#include "noeud.hh"
#include "noeud_image.h"

OperatriceSimulation::OperatriceSimulation(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceCorps(graphe_parent, noeud_)
{
	noeud.peut_avoir_graphe = true;
}

const char *OperatriceSimulation::nom_classe() const
{
	return NOM;
}

const char *OperatriceSimulation::texte_aide() const
{
	return AIDE;
}

const char *OperatriceSimulation::chemin_entreface() const
{
	return "entreface/operatrice_simulation.jo";
}

int OperatriceSimulation::type() const
{
	return OPERATRICE_SIMULATION;
}

res_exec OperatriceSimulation::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	auto sortie_graphe = noeud.graphe.dernier_noeud_sortie;

	if (sortie_graphe == nullptr) {
		ajoute_avertissement("Aucune sortie trouvée dans le graphe !");
		return res_exec::ECHOUEE;
	}

	auto const temps_debut = evalue_entier("temps_début");
	auto const temps_fin   = evalue_entier("temps_fin");

	/* Si nous sommes au début, réinitialise. */
	if (contexte.temps_courant == temps_debut) {
		m_corps.reinitialise();
		m_corps1.reinitialise();
		m_corps2.reinitialise();

		noeud.graphe.entrees.efface();
		noeud.graphe.entrees.pousse(&m_corps1);
		noeud.graphe.entrees.pousse(&m_corps2);

		noeud.graphe.donnees.efface();
		noeud.graphe.donnees.pousse(&m_corps);

		/* copie l'état de base */
		auto corps = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps) {
			corps->copie_vers(&m_corps1);
		}

		corps = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps) {
			corps->copie_vers(&m_corps2);
		}
	}
	/* Ne simule que si l'on a avancé d'une image. */
	else if (contexte.temps_courant != m_dernier_temps + 1) {
		return res_exec::REUSSIE;
	}
	else if (contexte.temps_courant > temps_fin) {
		return res_exec::REUSSIE;
	}

	m_dernier_temps = contexte.temps_courant;

	/* Renseigne les données de simulation aux noeuds du graphe. */
	auto donnees_sim = DonneesSimulation{};
	donnees_sim.dt = 0.1; //static_cast<double>(evalue_decimal("dt"));
	donnees_sim.temps_fin = temps_fin;
	donnees_sim.temps_debut = temps_debut;
	donnees_sim.sous_etape = 0;
	donnees_sim.dernier_temps = m_dernier_temps;

	for (auto &noeud_graphe : noeud.graphe.noeuds()) {
		auto op = extrait_opimage(noeud_graphe->donnees);

		if (op->type() == OPERATRICE_CORPS) {
			auto op_corps = dynamic_cast<OperatriceCorps *>(op);
			op_corps->donnees_simulation(&donnees_sim);
		}
	}

	/* exécute graphe */
	execute_noeud(*sortie_graphe, contexte, donnees_aval);

	auto op_sortie = extrait_opimage(sortie_graphe->donnees);

	m_corps.reinitialise();
	op_sortie->corps()->copie_vers(&m_corps);

	return res_exec::REUSSIE;
}

bool OperatriceSimulation::depend_sur_temps() const
{
	return true;
}

void OperatriceSimulation::amont_change(PriseEntree *entree)
{
	INUTILISE(entree);

	for (auto noeud_graphe : noeud.graphe.noeuds()) {
		noeud_graphe->besoin_execution = true;
		auto op = extrait_opimage(noeud_graphe->donnees);
		op->amont_change(nullptr);
	}
}

void OperatriceSimulation::renseigne_dependance(
		ContexteEvaluation const &contexte,
		CompilatriceReseau &compilatrice,
		NoeudReseau *noeud_res)
{
	for (auto noeud_graphe : noeud.graphe.noeuds()) {
		auto op = extrait_opimage(noeud_graphe->donnees);
		op->renseigne_dependance(contexte, compilatrice, noeud_res);
	}
}
