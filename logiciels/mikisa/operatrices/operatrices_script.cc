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

#include "lcc/analyseuse_grammaire.h"
#include "lcc/assembleuse_arbre.h"
#include "lcc/contexte_execution.hh"
#include "lcc/contexte_generation_code.h"
#include "lcc/decoupeuse.h"
#include "lcc/execution_pile.hh"
#include "lcc/lcc.hh"
#include "lcc/modules.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/attribut.h"
#include "corps/polyedre.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static auto converti_type_lcc(lcc::type_var type, int &dimensions)
{
	switch (type) {
		case lcc::type_var::DEC:
		{
			dimensions = 1;
			return type_attribut::R32;
		}
		case lcc::type_var::ENT32:
		{
			dimensions = 1;
			return type_attribut::Z32;
		}
		case lcc::type_var::VEC2:
		{
			dimensions = 2;
			return type_attribut::R32;
		}
		case lcc::type_var::VEC3:
		{
			dimensions = 3;
			return type_attribut::R32;
		}
		case lcc::type_var::COULEUR:
		case lcc::type_var::VEC4:
		{
			dimensions = 4;
			return type_attribut::R32;
		}
		case lcc::type_var::MAT3:
		{
			dimensions = 9;
			return type_attribut::R32;
		}
		case lcc::type_var::MAT4:
		{
			dimensions = 16;
			return type_attribut::R32;
		}
		case lcc::type_var::CHAINE:
		{
			dimensions = 1;
			return type_attribut::CHAINE;
		}
		case lcc::type_var::INVALIDE:
		case lcc::type_var::TABLEAU:
		case lcc::type_var::POLYMORPHIQUE:
			return type_attribut::INVALIDE;
	}

	return type_attribut::INVALIDE;
}

static auto converti_type_attr(type_attribut type, int dimensions)
{
	switch (type) {
		case type_attribut::R32:
		{
			if (dimensions == 1) {
				return lcc::type_var::DEC;
			}

			if (dimensions == 2) {
				return lcc::type_var::VEC2;
			}

			if (dimensions == 3) {
				return lcc::type_var::VEC3;
			}

			if (dimensions == 4) {
				return lcc::type_var::VEC4;
			}

			if (dimensions == 9) {
				return lcc::type_var::MAT3;
			}

			if (dimensions == 16) {
				return lcc::type_var::MAT4;
			}

			break;
		}
		case type_attribut::Z8:
		case type_attribut::Z32:
		{
			if (dimensions == 1) {
				return lcc::type_var::ENT32;
			}

			break;
		}
		case type_attribut::CHAINE:
		{
			return lcc::type_var::CHAINE;
		}
		default:
		{
			return lcc::type_var::INVALIDE;
		}
	}

	return lcc::type_var::INVALIDE;
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
		case danjo::TypePropriete::LISTE_MANIP:
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
		if (attr.portee == portee) {
			auto idx = compileuse.donnees().loge_donnees(attr.dimensions);
			gest_attrs.ajoute_propriete(attr.nom(), converti_type_attr(attr.type(), attr.dimensions), idx);
		}
	}
}

static auto cree_proprietes_parametres_declares(
		danjo::Manipulable *manipulable,
		ContexteGenerationCode &ctx_gen)
{
	for (auto &param_decl : ctx_gen.params_declare) {
		if (manipulable->propriete(param_decl.nom) != nullptr) {
			continue;
		}

		std::cerr << "Crée propriété pour " << param_decl.nom << "\n";

		auto prop = danjo::Propriete{};
		prop.est_extra = true;

		switch (param_decl.type) {
			case lcc::type_var::DEC:
			{
				prop.type = danjo::TypePropriete::DECIMAL;
				prop.valeur = param_decl.valeur[0];
				break;
			}
			case lcc::type_var::ENT32:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = static_cast<int>(param_decl.valeur[0]);
				break;
			}
			case lcc::type_var::VEC3:
			{
				prop.type = danjo::TypePropriete::VECTEUR;
				prop.valeur = param_decl.valeur;
				break;
			}
			case lcc::type_var::COULEUR:
			{
				prop.type = danjo::TypePropriete::COULEUR;
				prop.valeur = dls::phys::couleur32();
				break;
			}
			case lcc::type_var::CHAINE:
			{
				prop.type = danjo::TypePropriete::CHAINE_CARACTERE;
				prop.valeur = dls::chaine("");
				break;
			}
			case lcc::type_var::VEC2:
			case lcc::type_var::VEC4:
			case lcc::type_var::MAT3:
			case lcc::type_var::MAT4:
			case lcc::type_var::INVALIDE:
			case lcc::type_var::TABLEAU:
			case lcc::type_var::POLYMORPHIQUE:
			{
				continue;
			}
		}

		manipulable->ajoute_propriete_extra(param_decl.nom, prop);
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
				compileuse.donnees().stocke(idx, var);
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
			case danjo::TypePropriete::LISTE_MANIP:
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
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

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
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

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

	if (contexte == (lcc::ctx_script::topologie | lcc::ctx_script::fichier)) {
		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::CHAINE));
		gest_props.ajoute_propriete("ligne", lcc::type_var::CHAINE, idx);
	}
}

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

		auto ctx_gen = lcc::cree_contexte(*contexte.lcc);

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
			ajoute_proprietes_contexte(ctx_script, compileuse, gest_props);

			/* ajout des attributs selon le contexte */
			auto &gest_attrs = ctx_gen.gest_attrs;

			ajoute_attributs_contexte(m_corps, ctx_gen, compileuse, ctx_script);

			/* ajout des variables extras */
			cree_proprietes_parametres_declares(this, ctx_gen);
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

				auto dimensions = 0;
				auto type_attr = converti_type_lcc(requete->type, dimensions);
				attr = m_corps.ajoute_attribut(
							requete->nom,
							type_attr,
							dimensions,
							portee_attr::POINT);

				requete->ptr_donnees = attr;
			}

			auto ctx_exec = lcc::ctx_exec{};
			ctx_exec.ptr_corps = &m_corps;			
			ctx_exec.chaines = ctx_gen.chaines;

			/* données générales */
			remplis_donnees(compileuse.donnees(), gest_props, "temps", contexte.temps_courant);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_début", contexte.temps_debut);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_fin", contexte.temps_fin);
			remplis_donnees(compileuse.donnees(), gest_props, "cadence", static_cast<float>(contexte.cadence));

			if (portee_script == "fichier") {
				execute_script_pour_fichier(
							chef, compileuse, gest_props, gest_attrs, ctx_exec, contenu_fichier);
			}
			else {
				execute_script(chef, compileuse, gest_props, gest_attrs, ctx_exec);
			}
		}
		catch (erreur::frappe const &e) {
			this->ajoute_avertissement(e.message());
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}
		catch (std::exception const &e) {
			this->ajoute_avertissement(e.what());
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}

		/* ******************* */

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}

	void execute_script_pour_fichier(
			ChefExecution *chef,
			compileuse_lng &compileuse,
			gestionnaire_propriete &gest_props,
			gestionnaire_propriete &gest_attrs,
			lcc::ctx_exec &ctx_exec,
			dls::chaine const &contenu_fichier)
	{
		auto tampon_source = lng::tampon_source(contenu_fichier);

		auto decalage_chn = ctx_exec.chaines.taille();
		ctx_exec.chaines.pousse("");

		for (auto i = 0ul; i < tampon_source.nombre_lignes(); ++i) {
			if (chef->interrompu()) {
				break;
			}

			ctx_exec.chaines[decalage_chn] = tampon_source[static_cast<int>(i)];

			remplis_donnees(compileuse.donnees(), gest_props, "index", static_cast<int>(i));
			remplis_donnees(compileuse.donnees(), gest_props, "ligne", static_cast<int>(decalage_chn));

			/* stocke les attributs */
			stocke_attributs(gest_attrs, compileuse.donnees(), static_cast<int>(i));

			lcc::execute_pile(
						ctx_exec,
						compileuse.donnees(),
						compileuse.instructions(),
						static_cast<int>(i));

			/* charge les attributs */
			charge_attributs(gest_attrs, compileuse.donnees(), static_cast<int>(i));
		}
	}

	void execute_script(
			ChefExecution *chef,
			compileuse_lng &compileuse,
			gestionnaire_propriete &gest_props,
			gestionnaire_propriete &gest_attrs,
			lcc::ctx_exec &ctx_exec)
	{
		for (auto i = 0; i < 1; ++i) {
			if (chef->interrompu()) {
				break;
			}

			remplis_donnees(compileuse.donnees(), gest_props, "index", i);

			/* stocke les attributs */
			stocke_attributs(gest_attrs, compileuse.donnees(), i);

			lcc::execute_pile(
						ctx_exec,
						compileuse.donnees(),
						compileuse.instructions(),
						i);

			/* charge les attributs */
			charge_attributs(gest_attrs, compileuse.donnees(), i);
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
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

		/* ****************** */

		auto ctx_gen = lcc::cree_contexte(*contexte.lcc);

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

			ajoute_proprietes_contexte(ctx_script, compileuse, gest_props);

			/* ajout des attributs selon le contexte */
			auto &gest_attrs = ctx_gen.gest_attrs;

			ajoute_attributs_contexte(m_corps, ctx_gen, compileuse, ctx_script);

			/* ajout des variables extras */
			cree_proprietes_parametres_declares(this, ctx_gen);
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

				auto dimensions = 0;
				auto type_attr = converti_type_lcc(requete->type, dimensions);
				attr = m_corps.ajoute_attribut(
							requete->nom,
							type_attr,
							dimensions,
							portee_attr::POINT);

				requete->ptr_donnees = attr;
			}

			auto ctx_exec = lcc::ctx_exec{};
			ctx_exec.chaines = ctx_gen.chaines;
			ctx_exec.corps = &m_corps;

			auto taille_donnees = compileuse.donnees().taille();
			auto taille_instructions = compileuse.instructions().taille();

			/* Retourne si le script est vide, notons qu'il y a forcément une
			 * intruction : code_inst::TERMINE. */
			if ((taille_donnees == 0) || (taille_instructions == 1)) {
				return res_exec::REUSSIE;
			}

			for (auto req : ctx_gen.requetes) {
				if (req == lcc::req_fonc::polyedre) {
					ctx_exec.polyedre = converti_corps_polyedre(m_corps);
				}				
				else if (req == lcc::req_fonc::arbre_kd) {
					auto points_entree = m_corps.points_pour_lecture();

					ctx_exec.arbre_kd.construit_avec_fonction(
								static_cast<int>(points_entree->taille()),
								[&](int i)
					{
						return points_entree->point(i);
					});
				}
			}

			/* données générales */
			remplis_donnees(compileuse.donnees(), gest_props, "temps", contexte.temps_courant);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_début", contexte.temps_debut);
			remplis_donnees(compileuse.donnees(), gest_props, "temps_fin", contexte.temps_fin);
			remplis_donnees(compileuse.donnees(), gest_props, "cadence", static_cast<float>(contexte.cadence));

			if (portee_script == "points") {
				execute_script_sur_points(chef, compileuse, gest_props, gest_attrs, ctx_exec);
			}
			else if (portee_script == "primitives") {
				execute_script_sur_primitives(chef, compileuse, gest_props, gest_attrs, ctx_exec);
			}
		}
		catch (erreur::frappe const &e) {
			this->ajoute_avertissement(e.message());
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}
		catch (std::exception const &e) {
			this->ajoute_avertissement(e.what());
			chef->indique_progression(100.0f);
			return res_exec::ECHOUEE;
		}

		/* ******************* */

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}

	void execute_script_sur_points(
			ChefExecution *chef,
			compileuse_lng &compileuse,
			gestionnaire_propriete &gest_props,
			gestionnaire_propriete &gest_attrs,
			lcc::ctx_exec &ctx_exec)
	{
		/* À FAIRE : la copie est peut-être inutile, à vérifier si le script les
		 * modifie. */
		auto points = m_corps.points_pour_ecriture();

		boucle_serie(tbb::blocked_range<long>(0, points->taille()),
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

				auto point = points->point(i);

				remplis_donnees(donnees, gest_props, "P", point);
				remplis_donnees(donnees, gest_props, "index", static_cast<int>(i));

				/* stocke les attributs */
				stocke_attributs(gest_attrs, donnees, i);

				lcc::execute_pile(
							ctx_exec,
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

	void execute_script_sur_primitives(
			ChefExecution *chef,
			compileuse_lng &compileuse,
			gestionnaire_propriete &gest_props,
			gestionnaire_propriete &gest_attrs,
			lcc::ctx_exec &ctx_exec)
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

				remplis_donnees(donnees, gest_props, "index", static_cast<int>(i));

				/* stocke les attributs */
				stocke_attributs(gest_attrs, donnees, i);

				lcc::execute_pile(
							ctx_exec,
							donnees,
							compileuse.instructions(),
							static_cast<int>(i));

				/* charge les attributs */
				charge_attributs(gest_attrs, donnees, i);
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
