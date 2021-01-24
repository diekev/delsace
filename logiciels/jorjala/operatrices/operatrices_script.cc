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

#include "operatrices_script.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/langage/tampon_source.hh"

#include "coeur/chef_execution.hh"
#include "coeur/compileuse_lcc.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OpScriptTopologie final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Script Topologie";
	static constexpr auto AIDE = "";

	OpScriptTopologie(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_script_detail.jo"};
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
		m_corps.reinitialise();
	//	auto corps_ref = entree(0)->requiers_corps(contexte, donnees_aval);

		auto portee_script = evalue_enum("portée_script");

		auto ctx_script = lcc::ctx_script::topologie;

		if (portee_script == "points") {
			ctx_script |= lcc::ctx_script::point;
		}
		else if (portee_script == "primitives") {
			ctx_script |= lcc::ctx_script::primitive;
		}
		else if (portee_script == "fichier") {
			ctx_script |= lcc::ctx_script::fichier;
		}
		else {
			this->ajoute_avertissement("La portée du script est invalide");
			return res_exec::ECHOUEE;
		}

		auto texte = evalue_chaine("script");

		if (texte.est_vide()) {
			return res_exec::ECHOUEE;
		}

		auto contenu_fichier = dls::contenu_fichier(evalue_fichier_entree("source_entrée"));

		if (contenu_fichier.est_vide()) {
			return res_exec::ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("script détail");

		/* ****************************************************************** */

		auto compileuse = CompileuseScriptLCC();

		if (!compileuse.compile_script(*this, m_corps, contexte, texte, ctx_script)) {
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}

		auto taille_donnees = compileuse.donnees().taille();
		auto taille_instructions = compileuse.m_compileuse.instructions().taille();

		/* Retourne si le script est vide, notons qu'il y a forcément une
		 * intruction : code_inst::TERMINE. */
		if ((taille_donnees == 0) || (taille_instructions == 1)) {
			return res_exec::REUSSIE;
		}

		/* données générales */
		compileuse.remplis_donnees(compileuse.donnees(), "temps", contexte.temps_courant);
		compileuse.remplis_donnees(compileuse.donnees(), "temps_début", contexte.temps_debut);
		compileuse.remplis_donnees(compileuse.donnees(), "temps_fin", contexte.temps_fin);
		compileuse.remplis_donnees(compileuse.donnees(), "cadence", static_cast<float>(contexte.cadence));

		if (portee_script == "fichier") {
			execute_script_pour_fichier(
						chef, compileuse, contenu_fichier);
		}
		else {
			execute_script(chef, compileuse);
		}

		/* ****************************************************************** */

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}

	void execute_script_pour_fichier(
			ChefExecution *chef,
			CompileuseScriptLCC &compileuse,
			dls::chaine const &contenu_fichier)
	{
		auto texte_ = contenu_fichier;
		auto tampon_source = lng::tampon_source(std::move(texte_));

		auto decalage_chn = compileuse.m_ctx_global.chaines.taille();
		compileuse.m_ctx_global.chaines.ajoute("");

		for (auto i = 0ul; i < tampon_source.nombre_lignes(); ++i) {
			if (chef->interrompu()) {
				break;
			}

			auto ctx_local = lcc::ctx_local{};

			compileuse.m_ctx_global.chaines[decalage_chn] = tampon_source[static_cast<int>(i)];

			compileuse.remplis_donnees(compileuse.donnees(), "index", static_cast<int>(i));
			compileuse.remplis_donnees(compileuse.donnees(), "ligne", static_cast<int>(decalage_chn));

			/* stocke les attributs */
			compileuse.stocke_attributs(ctx_local, compileuse.donnees(), static_cast<int>(i));

			compileuse.execute_pile(ctx_local, compileuse.donnees());

			/* charge les attributs */
			compileuse.charge_attributs(ctx_local, compileuse.donnees(), static_cast<int>(i));
		}
	}

	void execute_script(
			ChefExecution *chef,
			CompileuseScriptLCC &compileuse)
	{
		for (auto i = 0; i < 1; ++i) {
			if (chef->interrompu()) {
				break;
			}

			auto ctx_local = lcc::ctx_local{};

			compileuse.remplis_donnees(compileuse.donnees(), "index", i);

			/* stocke les attributs */
			compileuse.stocke_attributs(ctx_local, compileuse.donnees(), i);

			compileuse.execute_pile(ctx_local, compileuse.donnees());

			/* charge les attributs */
			compileuse.charge_attributs(ctx_local, compileuse.donnees(), i);
		}
	}
};

/* ************************************************************************** */

class OpScriptDetail final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Script Détail";
	static constexpr auto AIDE = "";

	OpScriptDetail(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_script_detail.jo"};
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

		if (!valide_corps_entree(*this, &m_corps, true, false)) {
			return res_exec::ECHOUEE;
		}

		auto portee_script = evalue_enum("portée_script");

		auto ctx_script = lcc::ctx_script::detail;

		if (portee_script == "points") {
			ctx_script |= lcc::ctx_script::point;
		}
		else if (portee_script == "primitives") {
			ctx_script |= lcc::ctx_script::primitive;
		}
		else if (portee_script == "fichier") {
			ctx_script |= lcc::ctx_script::fichier;
		}
		else {
			this->ajoute_avertissement("La portée du script est invalide");
			return res_exec::ECHOUEE;
		}

		auto texte = evalue_chaine("script");

		if (texte.est_vide()) {
			return res_exec::ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("script détail");

		/* ****************************************************************** */

		auto compileuse = CompileuseScriptLCC();

		if (!compileuse.compile_script(*this, m_corps, contexte, texte, ctx_script)) {
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}

		auto taille_donnees = compileuse.donnees().taille();
		auto taille_instructions = compileuse.m_compileuse.instructions().taille();

		/* Retourne si le script est vide, notons qu'il y a forcément une
		 * intruction : code_inst::TERMINE. */
		if ((taille_donnees == 0) || (taille_instructions == 1)) {
			return res_exec::REUSSIE;
		}

		/* données générales */
		compileuse.remplis_donnees(compileuse.donnees(), "temps", contexte.temps_courant);
		compileuse.remplis_donnees(compileuse.donnees(), "temps_début", contexte.temps_debut);
		compileuse.remplis_donnees(compileuse.donnees(), "temps_fin", contexte.temps_fin);
		compileuse.remplis_donnees(compileuse.donnees(), "cadence", static_cast<float>(contexte.cadence));

		if (portee_script == "points") {
			auto points_entree = m_corps.points_pour_lecture();

			auto donnees_prop = compileuse.m_gest_attrs.donnees_pour_propriete("P");
			if (donnees_prop->est_modifiee) {
				auto points_sortie = m_corps.points_pour_ecriture();
				execute_script_sur_points(chef, compileuse, points_entree, &points_sortie);
			}
			else {
				execute_script_sur_points(chef, compileuse, points_entree, nullptr);
			}

		}
		else if (portee_script == "primitives") {
			execute_script_sur_primitives(chef, compileuse);
		}

		/* ******************* */

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}

	void execute_script_sur_points(
			ChefExecution *chef,
			CompileuseScriptLCC &compileuse,
			AccesseusePointLecture const &points_entree,
			AccesseusePointEcriture *points_sortie)
	{
		boucle_serie(tbb::blocked_range<long>(0, points_entree.taille()),
					 [&](tbb::blocked_range<long> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			/* fait une copie locale */
			auto donnees = compileuse.donnees();

			for (auto i = plage.begin(); i < plage.end(); ++i) {
				if (chef->interrompu()) {
					break;
				}

				auto ctx_local = lcc::ctx_local{};

				auto point = points_entree.point_local(i);

				compileuse.remplis_donnees(donnees, "P", point);
				compileuse.remplis_donnees(donnees, "index", static_cast<int>(i));

				/* stocke les attributs */
				compileuse.stocke_attributs(ctx_local, donnees, i);

				compileuse.execute_pile(ctx_local, donnees);

				auto idx_sortie = compileuse.pointeur_donnees("P");
				point = donnees.charge_vec3(idx_sortie);

				/* charge les attributs */
				compileuse.charge_attributs(ctx_local, donnees, i);

				if (points_sortie) {
					points_sortie->point(i, point);
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			chef->indique_progression_parallele(delta / static_cast<float>(points_entree.taille()) * 100.0f);
		});
	}

	void execute_script_sur_primitives(
			ChefExecution *chef,
			CompileuseScriptLCC &compileuse)
	{
		auto prims = m_corps.prims();

		boucle_serie(tbb::blocked_range<long>(0, prims->taille()),
					 [&](tbb::blocked_range<long> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			/* fait une copie locale */
			auto donnees = compileuse.donnees();

			for (auto i = plage.begin(); i < plage.end(); ++i) {
				if (chef->interrompu()) {
					break;
				}

				auto ctx_local = lcc::ctx_local{};

				compileuse.remplis_donnees(donnees, "index", static_cast<int>(i));

				/* stocke les attributs */
				compileuse.stocke_attributs(ctx_local, donnees, i);

				compileuse.execute_pile(ctx_local, donnees);

				/* charge les attributs */
				compileuse.charge_attributs(ctx_local, donnees, i);
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			chef->indique_progression_parallele(delta / static_cast<float>(prims->taille()) * 100.0f);
		});
	}
};

/* ************************************************************************** */

void enregistre_operatrices_script(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpScriptDetail>());
	usine.enregistre_type(cree_desc<OpScriptTopologie>());
}

#pragma clang diagnostic pop
