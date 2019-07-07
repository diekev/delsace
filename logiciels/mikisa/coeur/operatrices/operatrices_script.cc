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

#include "biblinternes/outils/parallelisme.h"

#include "corps/attribut.h"

#include "lcc/analyseuse_grammaire.h"
#include "lcc/contexte_execution.hh"
#include "lcc/contexte_generation_code.h"
#include "lcc/decoupeuse.h"
#include "lcc/execution_pile.hh"
#include "lcc/modules.hh"

#include "../chef_execution.hh"
#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

static auto converti_type_lcc(lcc::type_var type)
{
	switch (type) {
		case lcc::type_var::DEC:
			return type_attribut::DECIMAL;
		case lcc::type_var::ENT32:
			return type_attribut::ENT32;
		case lcc::type_var::VEC2:
			return type_attribut::VEC2;
		case lcc::type_var::VEC3:
			return type_attribut::VEC3;
		case lcc::type_var::COULEUR:
		case lcc::type_var::VEC4:
			return type_attribut::VEC4;
		case lcc::type_var::MAT3:
			return type_attribut::MAT3;
		case lcc::type_var::MAT4:
			return type_attribut::MAT4;
		case lcc::type_var::CHAINE:
			return type_attribut::CHAINE;
		case lcc::type_var::INVALIDE:
		case lcc::type_var::TABLEAU:
		case lcc::type_var::POLYMORPHIQUE:
			return type_attribut::INVALIDE;
	}

	return type_attribut::INVALIDE;
}

static auto converti_type_attr(type_attribut type)
{
	switch (type) {
		case type_attribut::DECIMAL:
		{
			return lcc::type_var::DEC;
		}
		case type_attribut::ENT32:
		{
			return lcc::type_var::ENT32;
		}
		case type_attribut::ENT8:
		{
			return lcc::type_var::ENT32;
		}
		case type_attribut::VEC2:
		{
			return lcc::type_var::VEC2;
		}
		case type_attribut::VEC3:
		{
			return lcc::type_var::VEC3;
		}
		case type_attribut::VEC4:
		{
			return lcc::type_var::VEC4;
		}
		case type_attribut::MAT3:
		{
			return lcc::type_var::MAT3;
		}
		case type_attribut::MAT4:
		{
			return lcc::type_var::MAT4;
		}
		case type_attribut::INVALIDE:
		{
			return lcc::type_var::INVALIDE;
		}
		case type_attribut::CHAINE:
		{
			return lcc::type_var::CHAINE;
		}
	}

	return lcc::type_var::INVALIDE;
}

static auto taille_attr(type_attribut type)
{
	switch (type) {
		case type_attribut::DECIMAL:
		case type_attribut::ENT32:
		case type_attribut::ENT8:
		{
			return 1;
		}
		case type_attribut::VEC2:
		{
			return 2;
		}
		case type_attribut::VEC3:
		{
			return 3;
		}
		case type_attribut::VEC4:
		{
			return 4;
		}
		case type_attribut::MAT3:
		{
			return 9;
		}
		case type_attribut::MAT4:
		{
			return 16;
		}
		case type_attribut::INVALIDE:
		case type_attribut::CHAINE:
		{
			return 0;
		}
	}

	return 0;
}

static auto converti_type_prop(danjo::TypePropriete type)
{
	switch (type) {
		case danjo::TypePropriete::DECIMAL:
		{
			return lcc::type_var::DEC;
		}
		case danjo::TypePropriete::ENTIER:
		{
			return lcc::type_var::ENT32;
		}
		case danjo::TypePropriete::CHAINE_CARACTERE:
		case danjo::TypePropriete::ENUM:
		case danjo::TypePropriete::TEXTE:
		case danjo::TypePropriete::FICHIER_ENTREE:
		case danjo::TypePropriete::FICHIER_SORTIE:
		{
			return lcc::type_var::CHAINE;
		}
		case danjo::TypePropriete::BOOL:
		{
			return lcc::type_var::ENT32;
		}
		case danjo::TypePropriete::VECTEUR:
		{
			return lcc::type_var::VEC3;
		}
		case danjo::TypePropriete::COULEUR:
		{
			return lcc::type_var::COULEUR;
		}
		case danjo::TypePropriete::COURBE_VALEUR:
		case danjo::TypePropriete::COURBE_COULEUR:
		case danjo::TypePropriete::RAMPE_COULEUR:
		{
			return lcc::type_var::INVALIDE;
		}
	}

	return lcc::type_var::INVALIDE;
}

/* ************************************************************************** */

static auto ajoute_attributs_contexte(
		Corps const &corps,
		ContexteGenerationCode &ctx_gen,
		compileuse_lng &compileuse,
		lcc::ctx_script ctx)
{
	auto portee = portee_attr{};

	if (ctx == (lcc::ctx_script::detail | lcc::ctx_script::point)) {
		portee = portee_attr::POINT;
	}
	else if (ctx == (lcc::ctx_script::detail | lcc::ctx_script::primitive)) {
		portee = portee_attr::PRIMITIVE;
	}
	else {
		return;
	}

	auto &gest_attrs = ctx_gen.gest_attrs;

	for (auto &attr : corps.attributs()) {
		if (attr->portee == portee) {
			auto idx = compileuse.donnees().loge_donnees(taille_attr(attr->type()));
			gest_attrs.ajoute_propriete(attr->nom(), converti_type_attr(attr->type()), idx);
		}
	}
}

static auto ajoute_proprietes_extra(
		danjo::Manipulable *manipulable,
		ContexteGenerationCode &ctx_gen,
		compileuse_lng &compileuse,
		int temps)
{
	auto debut_props = manipulable->debut();
	auto fin_props = manipulable->fin();

	for (auto iter = debut_props; iter != fin_props; ++iter) {
		auto const &prop = iter->second;

		if (!prop.est_extra) {
			continue;
		}

		auto type_llc = converti_type_prop(prop.type);
		auto idx = compileuse.donnees().loge_donnees(taille_type(type_llc));
		ctx_gen.pousse_locale(iter->first, idx, type_llc);

		switch (prop.type) {
			case danjo::TypePropriete::DECIMAL:
			{
				auto var = manipulable->evalue_decimal(iter->first, temps);
				compileuse.donnees().stocke(idx, var);
				break;
			}
			case danjo::TypePropriete::ENTIER:
			{
				auto var = manipulable->evalue_entier(iter->first, temps);
				compileuse.donnees().stocke(idx, var);
				break;
			}
			case danjo::TypePropriete::VECTEUR:
			{
				auto var = manipulable->evalue_vecteur(iter->first, temps);
				compileuse.donnees().stocke(idx, var);
				break;
			}
			case danjo::TypePropriete::COULEUR:
			{
				auto var = manipulable->evalue_couleur(iter->first, temps);
				// À FAIRE
				auto clr = dls::phys::couleur32(var.r, var.v, var.b, var.a);
				compileuse.donnees().stocke(idx, clr);
				break;
			}
			case danjo::TypePropriete::BOOL:
			case danjo::TypePropriete::CHAINE_CARACTERE:
			case danjo::TypePropriete::ENUM:
			case danjo::TypePropriete::TEXTE:
			case danjo::TypePropriete::FICHIER_ENTREE:
			case danjo::TypePropriete::FICHIER_SORTIE:
			case danjo::TypePropriete::COURBE_VALEUR:
			case danjo::TypePropriete::COURBE_COULEUR:
			case danjo::TypePropriete::RAMPE_COULEUR:
			{
				/* À FAIRE */
			}
		}
	}
}

static auto stocke_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		long idx_attr)
{
	for (auto const &requete : gest_attrs.donnees) {
		/* ne stocke pas les attributs créés pour ne pas effacer les
		 * données d'assignation de constante à la position du
		 * pointeur */
		if (requete->est_requis) {
			continue;
		}

		auto idx_pile = requete->ptr;
		auto attr = std::any_cast<Attribut *>(requete->ptr_donnees);

		switch (attr->type()) {
			case type_attribut::ENT8:
			{
				donnees.stocke(idx_pile, static_cast<float>(attr->ent8(idx_attr)));
				break;
			}
			case type_attribut::ENT32:
			{
				donnees.stocke(idx_pile, attr->ent32(idx_attr));
				break;
			}
			case type_attribut::DECIMAL:
			case type_attribut::VEC2:
			case type_attribut::VEC3:
			case type_attribut::VEC4:
			case type_attribut::MAT3:
			case type_attribut::MAT4:
			{
				auto taille = taille_octet_type_attribut(attr->type());

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_pile, ptr_attr, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::INVALIDE:
			case type_attribut::CHAINE:
			{
				break;
			}
		}
	}
}

static auto charge_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		long idx_attr)
{
	for (auto const &requete : gest_attrs.donnees) {
		auto idx_pile = requete->ptr;
		auto attr = std::any_cast<Attribut *>(requete->ptr_donnees);

		switch (attr->type()) {
			case type_attribut::ENT8:
			{
				auto v = donnees.charge_decimal(idx_pile);
				attr->valeur(idx_attr, static_cast<char>(v));
				break;
			}
			case type_attribut::ENT32:
			{
				auto v = donnees.charge_entier(idx_pile);
				attr->valeur(idx_attr, v);
				break;
			}
			case type_attribut::DECIMAL:
			case type_attribut::VEC2:
			case type_attribut::VEC3:
			case type_attribut::VEC4:
			case type_attribut::MAT3:
			case type_attribut::MAT4:
			{
				auto taille = taille_octet_type_attribut(attr->type());

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_attr, ptr_pile, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::INVALIDE:
			case type_attribut::CHAINE:
			{
				break;
			}
		}
	}
}

static auto ajoute_proprietes_contexte(
		lcc::ctx_script contexte,
		compileuse_lng &compileuse,
		gestionnaire_propriete &gest_props)
{
	auto idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::DEC));
	gest_props.ajoute_propriete("index", lcc::type_var::DEC, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete("temps", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete("temps_début", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete("temps_fin", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::DEC));
	gest_props.ajoute_propriete("cadence", lcc::type_var::DEC, idx);

	if (contexte == (lcc::ctx_script::detail | lcc::ctx_script::point)) {
		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::VEC3));
		gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);
	}
}

template <typename T>
static auto remplis_donnees(
		lcc::pile &donnees,
		gestionnaire_propriete &gest_props,
		dls::chaine const &nom,
		T const &v)
{
	auto idx = gest_props.pointeur_donnees(nom);

	if (idx == -1) {
		/* À FAIRE : erreur */
		return;
	}

	donnees.stocke(idx, v);
}

class OpScriptTopologie : public OperatriceCorps {
public:
	static constexpr auto NOM = "Script Topologie";
	static constexpr auto AIDE = "";

	OpScriptTopologie(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_script_detail.jo";
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
	//	auto corps_ref = entree(0)->requiers_corps(contexte, donnees_aval);

		auto texte = evalue_chaine("script");

		if (texte.est_vide()) {
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("script détail");

		/* ****************************************************************** */

		auto ctx_gen = ContexteGenerationCode{};

		auto donnees_module = ctx_gen.cree_module("racine");
		donnees_module->tampon = lng::tampon_source(texte);

		try {
			auto decoupeuse = decoupeuse_texte(donnees_module);
			decoupeuse.genere_morceaux();

			auto assembleuse = assembleuse_arbre(ctx_gen);

			auto analyseuse = analyseuse_grammaire(ctx_gen, donnees_module);

			analyseuse.lance_analyse(std::cerr);

			auto &gest_props = ctx_gen.gest_props;

			auto compileuse = compileuse_lng{};

			/* ajout des propriétés selon le contexte */
			auto ctx_script = (lcc::ctx_script::topologie | lcc::ctx_script::point);

			ajoute_proprietes_contexte(ctx_script, compileuse, gest_props);

			/* ajout des attributs selon le contexte */
			auto &gest_attrs = ctx_gen.gest_attrs;

			ajoute_attributs_contexte(m_corps, ctx_gen, compileuse, ctx_script);

			/* ajout des variables extras */
			ajoute_proprietes_extra(this, ctx_gen, compileuse, contexte.temps_courant);

			assembleuse.genere_code(ctx_gen, compileuse);

			for (auto &requete : gest_attrs.donnees) {
				requete->ptr_donnees = m_corps.attribut(requete->nom);
			}

			for (auto &requete : gest_attrs.requetes) {
				auto attr = m_corps.attribut(requete->nom);

				if (attr != nullptr) {
					/* L'attribut n'a pas la portée point */
					assert(attr->portee != portee_attr::POINT);

					m_corps.supprime_attribut(requete->nom);
				}

				attr = m_corps.ajoute_attribut(
							requete->nom,
							converti_type_lcc(requete->type),
							portee_attr::POINT);

				requete->ptr_donnees = attr;
			}

			auto ctx_exec = lcc::ctx_exec{};
			ctx_exec.ptr_corps = &m_corps;

			auto ctx_local = lcc::ctx_local{};

			/* données générales */
			remplis_donnees(compileuse.donnees(), gest_props, "temps", contexte.temps_courant);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_début", contexte.temps_debut);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_fin", contexte.temps_fin);
			remplis_donnees(compileuse.donnees(), gest_props, "cadence", static_cast<float>(contexte.cadence));

			for (auto i = 0; i < 1; ++i) {
				if (chef->interrompu()) {
					break;
				}

				remplis_donnees(compileuse.donnees(), gest_props, "index", i);

				/* stocke les attributs */
				stocke_attributs(gest_attrs, compileuse.donnees(), i);

				lcc::execute_pile(
							ctx_exec,
							ctx_local,
							compileuse.donnees(),
							compileuse.instructions(),
							i);

				/* charge les attributs */
				charge_attributs(gest_attrs, compileuse.donnees(), i);
			}
		}
		catch (erreur::frappe const &e) {
			this->ajoute_avertissement(e.message());
			chef->indique_progression(100.0f);
			return EXECUTION_ECHOUEE;
		}
		catch (std::exception const &e) {
			this->ajoute_avertissement(e.what());
			chef->indique_progression(100.0f);
			return EXECUTION_ECHOUEE;
		}

		/* ******************* */

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpScriptDetail : public OperatriceCorps {
public:
	static constexpr auto NOM = "Script Détail";
	static constexpr auto AIDE = "";

	OpScriptDetail(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_script_detail.jo";
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

		auto points = m_corps.points();

		if (points->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de points !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps.prims();

		if (prims->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée n'a pas de primitives !");
			return EXECUTION_ECHOUEE;
		}

		auto texte = evalue_chaine("script");

		if (texte.est_vide()) {
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("script détail");

		/* ****************** */

		auto ctx_gen = ContexteGenerationCode{};

		auto donnees_module = ctx_gen.cree_module("racine");
		donnees_module->tampon = lng::tampon_source(texte);

		try {
			auto decoupeuse = decoupeuse_texte(donnees_module);
			decoupeuse.genere_morceaux();

			auto assembleuse = assembleuse_arbre(ctx_gen);

			auto analyseuse = analyseuse_grammaire(ctx_gen, donnees_module);

			analyseuse.lance_analyse(std::cerr);

			auto &gest_props = ctx_gen.gest_props;

			auto compileuse = compileuse_lng{};

			/* ajout des propriétés selon le contexte */
			auto ctx_script = (lcc::ctx_script::detail | lcc::ctx_script::point);

			ajoute_proprietes_contexte(ctx_script, compileuse, gest_props);

			/* ajout des attributs selon le contexte */
			auto &gest_attrs = ctx_gen.gest_attrs;

			ajoute_attributs_contexte(m_corps, ctx_gen, compileuse, ctx_script);

			/* ajout des variables extras */
			ajoute_proprietes_extra(this, ctx_gen, compileuse, contexte.temps_courant);

			assembleuse.genere_code(ctx_gen, compileuse);

			for (auto &requete : gest_attrs.donnees) {
				requete->ptr_donnees = m_corps.attribut(requete->nom);
			}

			for (auto &requete : gest_attrs.requetes) {
				auto attr = m_corps.attribut(requete->nom);

				if (attr != nullptr) {
					/* L'attribut n'a pas la portée point */
					assert(attr->portee != portee_attr::POINT);

					m_corps.supprime_attribut(requete->nom);
				}

				attr = m_corps.ajoute_attribut(
							requete->nom,
							converti_type_lcc(requete->type),
							portee_attr::POINT);

				requete->ptr_donnees = attr;
			}

			auto ctx_exec = lcc::ctx_exec{};

			/* À FAIRE : vérifie que les données sont bel et bien requises avant
			 * d'en faire des copies. */
			points->detache();

			auto taille_donnees = compileuse.donnees().taille();
			auto taille_instructions = compileuse.instructions().taille();

			/* Retourne si le script est vide, notons qu'il y a forcément une
			 * intruction : code_inst::TERMINE. */
			if ((taille_donnees == 0) || (taille_instructions == 1)) {
				return EXECUTION_REUSSIE;
			}

			/* données générales */
			remplis_donnees(compileuse.donnees(), gest_props, "temps", contexte.temps_courant);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_début", contexte.temps_debut);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_fin", contexte.temps_fin);
			remplis_donnees(compileuse.donnees(), gest_props, "cadence", static_cast<float>(contexte.cadence));

			boucle_serie(tbb::blocked_range<long>(0, points->taille()),
							 [&](tbb::blocked_range<long> const &plage)
			{
				if (chef->interrompu()) {
					return;
				}

				/* fait une copie locale */
				auto donnees = compileuse.donnees();

				auto ctx_local = lcc::ctx_local{};

				for (auto i = plage.begin(); i < plage.end(); ++i) {
					if (chef->interrompu()) {
						break;
					}

					auto point = points->point(i);

					remplis_donnees(donnees, gest_props, "P", point);
					remplis_donnees(donnees, gest_props, "index", static_cast<int>(i));

					/* stocke les attributs */
					stocke_attributs(gest_attrs, donnees, i);

					lcc::execute_pile(
								ctx_exec,
								ctx_local,
								donnees,
								compileuse.instructions(),
								static_cast<int>(i));

					auto idx_sortie = gest_props.pointeur_donnees("P");
					point = donnees.charge_vec3(idx_sortie);

					/* charge les attributs */
					charge_attributs(gest_attrs, donnees, i);

					points->point(i, point);
				}

				auto delta = static_cast<float>(plage.end() - plage.begin());
				chef->indique_progression_parallele(delta / static_cast<float>(points->taille()) * 100.0f);
			});
		}
		catch (erreur::frappe const &e) {
			this->ajoute_avertissement(e.message());
			chef->indique_progression(100.0f);
			return EXECUTION_ECHOUEE;
		}
		catch (std::exception const &e) {
			this->ajoute_avertissement(e.what());
			chef->indique_progression(100.0f);
			return EXECUTION_ECHOUEE;
		}

		/* ******************* */

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_script(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpScriptDetail>());
	usine.enregistre_type(cree_desc<OpScriptTopologie>());
}
