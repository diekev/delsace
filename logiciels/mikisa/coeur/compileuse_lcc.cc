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

#include "compileuse_lcc.hh"

#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/operatrice_image.h"

#include "corps/corps.h"

#include "lcc/analyseuse_grammaire.h"
#include "lcc/assembleuse_arbre.h"
#include "lcc/contexte_execution.hh"
#include "lcc/contexte_generation_code.h"
#include "lcc/decoupeuse.h"
#include "lcc/execution_pile.hh"
#include "lcc/lcc.hh"
#include "lcc/modules.hh"

#include "operatrice_graphe_detail.hh"

/* ************************************************************************** */

static auto stocke_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		lcc::ctx_local &ctx_local,
		long idx_attr)
{
	for (auto const &donnee : gest_attrs.donnees) {
		/* ne stocke pas les attributs créés pour ne pas effacer les
		 * données d'assignation de constante à la position du
		 * pointeur */
		if (donnee->est_requis || donnee->est_propriete) {
			continue;
		}

		auto idx_pile = donnee->ptr;
		auto attr = std::any_cast<Attribut *>(donnee->ptr_donnees);

		switch (attr->type()) {
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_pile, ptr_attr, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::CHAINE:
			{
				auto decalage_chn = ctx_local.chaines.taille();
				ctx_local.chaines.pousse(*attr->chaine(idx_attr));
				donnees.stocke(idx_pile, static_cast<int>(decalage_chn));
				break;
			}
			case type_attribut::INVALIDE:
			{
				break;
			}
		}
	}
}

static auto charge_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		lcc::ctx_exec const &ctx_exec,
		lcc::ctx_local const &ctx_local,
		long idx_attr)
{
	for (auto const &donnee : gest_attrs.donnees) {
		if (donnee->est_propriete || !donnee->est_modifiee) {
			continue;
		}

		auto idx_pile = donnee->ptr;
		auto attr = std::any_cast<Attribut *>(donnee->ptr_donnees);

		switch (attr->type()) {
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_attr, ptr_pile, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::CHAINE:
			{
				auto ptr_chn = donnees.charge_entier(idx_pile);

				auto decalage_chn = ctx_exec.chaines.taille();

				if (ptr_chn >= decalage_chn) {
					*attr->chaine(idx_attr) = ctx_local.chaines[ptr_chn - decalage_chn];
				}
				else {
					*attr->chaine(idx_attr) = ctx_exec.chaines[ptr_chn];
				}

				break;
			}
			case type_attribut::INVALIDE:
			{
				break;
			}
		}
	}
}

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
			gest_attrs.ajoute_attribut(attr.nom(), converti_type_attr(attr.type(), attr.dimensions), idx);
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

static auto ajoute_proprietes_contexte(
		lcc::ctx_script contexte,
		compileuse_lng &compileuse,
		gestionnaire_propriete &gest_props)
{
	auto idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::DEC));
	gest_props.ajoute_propriete_non_modifiable("index", lcc::type_var::DEC, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete_non_modifiable("temps", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete_non_modifiable("temps_début", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::ENT32));
	gest_props.ajoute_propriete_non_modifiable("temps_fin", lcc::type_var::ENT32, idx);

	idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::DEC));
	gest_props.ajoute_propriete_non_modifiable("cadence", lcc::type_var::DEC, idx);

	if (contexte == (lcc::ctx_script::detail | lcc::ctx_script::point)) {
		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::VEC3));
		gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);
	}

	if (contexte == (lcc::ctx_script::topologie | lcc::ctx_script::fichier)) {
		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::CHAINE));
		gest_props.ajoute_propriete_non_modifiable("ligne", lcc::type_var::CHAINE, idx);
	}
}

/* ************************************************************************** */

static auto trouve_noeuds_connectes_sortie(Graphe const &graphe)
{
	auto noeuds = dls::pile<Noeud *>();
	auto ensemble = dls::ensemble<Noeud *>();

	for (auto &noeud_graphe : graphe.noeuds()) {
		if (noeud_graphe->sorties.est_vide()) {
			noeuds.empile(noeud_graphe);
		}
	}

	while (!noeuds.est_vide()) {
		auto noeud = noeuds.depile();

		if (ensemble.trouve(noeud) != ensemble.fin()) {
			continue;
		}

		ensemble.insere(noeud);

		for (auto const &entree : noeud->entrees) {
			for (auto const &sortie : entree->liens) {
				noeuds.empile(sortie->parent);
			}
		}
	}

	return ensemble;
}

/* ************************************************************************** */

lcc::pile &CompileuseLCC::donnees()
{
	return m_compileuse.donnees();
}

void CompileuseLCC::stocke_attributs(lcc::ctx_local &ctx_local, lcc::pile &donnees, long idx_attr)
{
	::stocke_attributs(m_gest_attrs, donnees, ctx_local, idx_attr);
}

void CompileuseLCC::charge_attributs(lcc::ctx_local &ctx_local, lcc::pile &donnees, long idx_attr)
{
	::charge_attributs(m_gest_attrs, donnees, m_ctx_global, ctx_local, idx_attr);
}

void CompileuseLCC::execute_pile(lcc::ctx_local &ctx_local, lcc::pile &donnees_pile)
{
	lcc::execute_pile(
				m_ctx_global,
				ctx_local,
				donnees_pile,
				m_compileuse.instructions(),
				0);
}

int CompileuseLCC::pointeur_donnees(const dls::chaine &nom)
{
	return m_gest_attrs.pointeur_donnees(nom);
}

/* ************************************************************************** */

CompileuseGrapheLCC::CompileuseGrapheLCC(Graphe &ptr_graphe)
	: graphe(ptr_graphe)
{}

bool CompileuseGrapheLCC::compile_graphe(ContexteEvaluation const &contexte, Corps *corps)
{
	m_compileuse = compileuse_lng();
	m_gest_attrs.reinitialise();
	m_ctx_global.reinitialise();

	if (graphe.besoin_ajournement) {
		tri_topologique(graphe);
		graphe.besoin_ajournement = false;
	}

	auto donnees_aval = DonneesAval{};
	donnees_aval.table.insere({"coulisse", dls::chaine("lcc")});
	donnees_aval.table.insere({"compileuse", &m_compileuse});
	donnees_aval.table.insere({"ctx_global", &m_ctx_global});
	donnees_aval.table.insere({"gest_attrs", &m_gest_attrs});

	auto type_detail = std::any_cast<int>(graphe.donnees[0]);

	/* fais de la place pour les entrées et sorties */
	auto &params_entree = params_noeuds_entree[type_detail];

	for (auto i = 0; i < params_entree.taille(); ++i) {
		auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(params_entree.type(i)));
		m_gest_attrs.ajoute_propriete(params_entree.nom(i), params_entree.type(i), idx);
	}

	auto &params_sortie = params_noeuds_sortie[type_detail];

	for (auto i = 0; i < params_sortie.taille(); ++i) {
		auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(params_sortie.type(i)));

		if (m_gest_attrs.propriete_existe(params_sortie.nom(i))) {
			continue;
		}

		m_gest_attrs.ajoute_propriete(params_sortie.nom(i), params_sortie.type(i), idx);
	}

	/* fais de la place pour les attributs */
	if (type_detail == DETAIL_POINTS && corps != nullptr) {
		for (auto &attr : corps->attributs()) {
			if (attr.portee != portee_attr::POINT) {
				continue;
			}

			auto idx = m_compileuse.donnees().loge_donnees(attr.dimensions);
			auto type_attr = converti_type_attr(attr.type(), attr.dimensions);
			m_gest_attrs.ajoute_attribut(attr.nom(), type_attr, idx);
		}
	}

	/* trouve les noeuds étant connectés à la sortie */
	auto ensemble = trouve_noeuds_connectes_sortie(graphe);

	/* performe la compilation */
	for (auto &noeud_graphe : graphe.noeuds()) {
		for (auto &sortie : noeud_graphe->sorties) {
			sortie->decalage_pile = 0;
		}

		if (ensemble.trouve(noeud_graphe) == ensemble.fin()) {
			continue;
		}

		auto operatrice = extrait_opimage(noeud_graphe->donnees);
		operatrice->reinitialise_avertisements();

		auto resultat = operatrice->execute(contexte, &donnees_aval);

		if (resultat == res_exec::ECHOUEE) {
			return false;
		}
	}

	/* prise en charge des requêtes */
	if (type_detail == DETAIL_POINTS && corps != nullptr) {
		for (auto &requete : m_gest_attrs.donnees) {
			requete->ptr_donnees = corps->attribut(requete->nom);
		}

		for (auto &requete : m_gest_attrs.requetes) {
			auto attr = corps->attribut(requete->nom);

			if (attr != nullptr) {
				/* L'attribut n'a pas la portée point */
				assert(attr->portee != portee_attr::POINT);

				corps->supprime_attribut(requete->nom);
			}

			auto dimensions = 0;
			auto type_attr = converti_type_lcc(requete->type, dimensions);

			attr = corps->ajoute_attribut(
						requete->nom,
						type_attr,
						dimensions,
						portee_attr::POINT);

			requete->ptr_donnees = attr;
		}
	}

	return true;
}

/* ************************************************************************** */

bool CompileuseScriptLCC::compile_script(
		OperatriceImage &op,
		Corps &corps,
		ContexteEvaluation const &contexte,
		dls::chaine const &texte,
		lcc::ctx_script ctx_script)
{
	ctx_gen = lcc::cree_contexte(*contexte.lcc);

	auto donnees_module = ctx_gen.cree_module("racine");
	donnees_module->tampon = lng::tampon_source(texte);

	try {
		auto decoupeuse = decoupeuse_texte(donnees_module);
		decoupeuse.genere_morceaux();

		auto assembleuse = assembleuse_arbre(ctx_gen);

		auto analyseuse = analyseuse_grammaire(ctx_gen, donnees_module);

		analyseuse.lance_analyse(std::cerr);

		/* ajout des propriétés et attributs selon le contexte */
		auto &gest_attrs = ctx_gen.gest_attrs;
		ajoute_proprietes_contexte(ctx_script, m_compileuse, gest_attrs);
		ajoute_attributs_contexte(corps, ctx_gen, m_compileuse, ctx_script);

		/* ajout des variables extras */
		cree_proprietes_parametres_declares(&op, ctx_gen);
		ajoute_proprietes_extra(&op, ctx_gen, m_compileuse, contexte.temps_courant);

		assembleuse.genere_code(ctx_gen, m_compileuse);

		for (auto &requete : gest_attrs.donnees) {
			requete->ptr_donnees = corps.attribut(requete->nom);
		}

		for (auto &requete : gest_attrs.requetes) {
			auto attr = corps.attribut(requete->nom);

			if (attr != nullptr) {
				/* L'attribut n'a pas la portée point */
				assert(attr->portee != portee_attr::POINT);

				corps.supprime_attribut(requete->nom);
			}

			auto dimensions = 0;
			auto type_attr = converti_type_lcc(requete->type, dimensions);
			attr = corps.ajoute_attribut(
						requete->nom,
						type_attr,
						dimensions,
						portee_attr::POINT);

			requete->ptr_donnees = attr;
		}
	}
	catch (erreur::frappe const &e) {
		op.ajoute_avertissement(e.message());
		return false;
	}
	catch (std::exception const &e) {
		op.ajoute_avertissement(e.what());
		return false;
	}

	auto taille_donnees = m_compileuse.donnees().taille();
	auto taille_instructions = m_compileuse.instructions().taille();

	/* Retourne si le script est vide, notons qu'il y a forcément une
		 * intruction : code_inst::TERMINE. */
	if ((taille_donnees == 0) || (taille_instructions == 1)) {
		return true;
	}

	for (auto req : ctx_gen.requetes) {
		if (req == lcc::req_fonc::polyedre) {
			m_ctx_global.polyedre = converti_corps_polyedre(corps);
		}
		else if (req == lcc::req_fonc::arbre_kd) {
			auto points_entree = corps.points_pour_lecture();

			m_ctx_global.arbre_kd.construit_avec_fonction(
						static_cast<int>(points_entree.taille()),
						[&](int i)
			{
				return points_entree.point_local(i);
			});
		}
	}

	for (auto donnee : ctx_gen.gest_attrs.donnees) {
		m_gest_attrs.donnees.pousse(donnee);
	}

	for (auto donnee : ctx_gen.gest_attrs.requetes) {
		m_gest_attrs.requetes.pousse(donnee);
	}

	ctx_gen.gest_attrs.donnees.efface();
	ctx_gen.gest_attrs.requetes.efface();

	m_ctx_global.chaines = ctx_gen.chaines;
	m_ctx_global.corps = &corps;

	return true;
}
