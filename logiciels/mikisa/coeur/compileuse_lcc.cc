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

#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/operatrice_image.h"

#include "corps/corps.h"

#include "operatrice_graphe_detail.hh"

/* ************************************************************************** */

static auto stocke_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
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
			case type_attribut::INVALIDE:
			case type_attribut::CHAINE:
			{
				break;
			}
		}
	}
}

/* À FAIRE : déduplique. */
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

void CompileuseLCC::stocke_attributs(lcc::pile &donnees, long idx_attr)
{
	::stocke_attributs(m_gest_attrs, donnees, idx_attr);
}

void CompileuseLCC::charge_attributs(lcc::pile &donnees, long idx_attr)
{
	::charge_attributs(m_gest_attrs, donnees, idx_attr);
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
