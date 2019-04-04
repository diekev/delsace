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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "contexte_generation_code.h"

#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <llvm/IR/LegacyPassManager.h>
#pragma GCC diagnostic pop

#include "modules.hh"

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		delete module;
	}

	delete menageur_fonctions;
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(const std::string &nom)
{
	auto module = new DonneesModule;
	module->id = modules.size();
	module->nom = nom;

	modules.push_back(module);

	return module;
}

DonneesModule *ContexteGenerationCode::module(size_t index) const
{
	return modules[index];
}

DonneesModule *ContexteGenerationCode::module(const std::string_view &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::module_existe(const std::string_view &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

llvm::BasicBlock *ContexteGenerationCode::bloc_courant() const
{
	return m_bloc_courant;
}

void ContexteGenerationCode::bloc_courant(llvm::BasicBlock *bloc)
{
	m_bloc_courant = bloc;
}

void ContexteGenerationCode::empile_bloc_continue(std::string_view chaine, llvm::BasicBlock *bloc)
{
	m_pile_continue.push_back({chaine, bloc});
}

void ContexteGenerationCode::depile_bloc_continue()
{
	m_pile_continue.pop_back();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_continue(std::string_view chaine)
{
	if (m_pile_continue.empty()) {
		return nullptr;
	}

	if (chaine.empty()) {
		return m_pile_continue.back().second;
	}

	for (auto const &paire : m_pile_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}

void ContexteGenerationCode::empile_bloc_arrete(std::string_view chaine, llvm::BasicBlock *bloc)
{
	m_pile_arrete.push_back({chaine, bloc});
}

void ContexteGenerationCode::depile_bloc_arrete()
{
	m_pile_arrete.pop_back();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_arrete(std::string_view chaine)
{
	if (m_pile_arrete.empty()) {
		return nullptr;
	}

	if (chaine.empty()) {
		return m_pile_arrete.back().second;
	}

	for (auto const &paire : m_pile_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}

void ContexteGenerationCode::pousse_globale(const std::string_view &nom, llvm::Value *valeur, const size_t index_type, bool est_dynamique)
{
	/* Nous utilisons ça plutôt que 'insert' car la valeur est poussée deux fois :
	 * - la première fois, nulle, lors de la validation sémantique,
	 * - la deuxième fois, correcte, lors de la génération du code.
	 *
	 * 'insert' n'insert pas si la valeur existe déjà, donc nous nous
	 * retrouvions avec un pointeur nul. */
	globales[nom] = { valeur, index_type, est_dynamique };
}

llvm::Value *ContexteGenerationCode::valeur_globale(const std::string_view &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return nullptr;
	}

	return iter->second.valeur;
}

bool ContexteGenerationCode::globale_existe(const std::string_view &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return false;
	}

	return true;
}

size_t ContexteGenerationCode::type_globale(const std::string_view &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return -1ul;
	}

	return iter->second.donnees_type;
}

conteneur_globales::const_iterator ContexteGenerationCode::iter_globale(const std::string_view &nom)
{
	return globales.find(nom);
}

conteneur_globales::const_iterator ContexteGenerationCode::fin_globales()
{
	return globales.end();
}

void ContexteGenerationCode::pousse_locale(
		const std::string_view &nom,
		llvm::Value *valeur,
		const size_t &index_type,
		const bool est_variable,
		const bool est_variadique)
{
	if (m_locales.size() > m_nombre_locales) {
		m_locales[m_nombre_locales] = {nom, {valeur, index_type, est_variable, est_variadique, {}}};
	}
	else {
		m_locales.push_back({nom, {valeur, index_type, est_variable, est_variadique, {}}});
	}

	++m_nombre_locales;
}

llvm::Value *ContexteGenerationCode::valeur_locale(const std::string_view &nom)
{
	auto iter_fin = m_locales.begin() + static_cast<long>(m_nombre_locales);

	auto iter = std::find_if(m_locales.begin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return nullptr;
	}

	return iter->second.valeur;
}

bool ContexteGenerationCode::locale_existe(const std::string_view &nom)
{
	auto iter_fin = m_locales.begin() + static_cast<long>(m_nombre_locales);

	auto iter = std::find_if(m_locales.begin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return false;
	}

	return true;
}

size_t ContexteGenerationCode::type_locale(const std::string_view &nom)
{
	auto iter_fin = m_locales.begin() + static_cast<long>(m_nombre_locales);
	auto iter = std::find_if(m_locales.begin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return -1ul;
	}

	return iter->second.donnees_type;
}

bool ContexteGenerationCode::peut_etre_assigne(const std::string_view &nom)
{
	auto iter = iter_locale(nom);

	if (iter == fin_locales()) {
		return false;
	}

	return iter->second.est_dynamique;
}

void ContexteGenerationCode::empile_nombre_locales()
{
	m_pile_nombre_locales.push(m_nombre_locales);
}

void ContexteGenerationCode::depile_nombre_locales()
{
	auto nombre_locales = m_pile_nombre_locales.top();
	/* nous ne pouvons pas avoir moins de locales en sortant du bloc */
	assert(nombre_locales <= m_nombre_locales);
	m_nombre_locales = nombre_locales;
	m_pile_nombre_locales.pop();
}

void ContexteGenerationCode::imprime_locales(std::ostream &os)
{
	for (size_t i = 0; i < m_nombre_locales; ++i) {
		os << '\t' << m_locales[i].first << '\n';
	}
}

bool ContexteGenerationCode::est_locale_variadique(const std::string_view &nom)
{
	auto iter = iter_locale(nom);

	if (iter == fin_locales()) {
		return false;
	}

	return iter->second.est_variadic;
}

conteneur_locales::const_iterator ContexteGenerationCode::iter_locale(const std::string_view &nom)
{
	auto iter_fin = fin_locales();
	auto iter = std::find_if(m_locales.cbegin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	return iter;
}

conteneur_locales::const_iterator ContexteGenerationCode::fin_locales()
{
	return m_locales.begin() + static_cast<long>(m_nombre_locales);
}

void ContexteGenerationCode::commence_fonction(llvm::Function *f)
{
	this->fonction = f;
	m_nombre_locales = 0;
	m_locales.clear();
}

void ContexteGenerationCode::termine_fonction()
{
	fonction = nullptr;
	m_bloc_courant = nullptr;
	m_nombre_locales = 0;
	m_locales.clear();
}

bool ContexteGenerationCode::structure_existe(const std::string_view &nom)
{
	return structures.find(nom) != structures.end();
}

size_t ContexteGenerationCode::ajoute_donnees_structure(const std::string_view &nom, DonneesStructure &donnees)
{
	donnees.id = nom_structures.size();
	donnees.type_llvm = nullptr;
	structures.insert({nom, donnees});
	nom_structures.push_back(nom);
	return donnees.id;
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const std::string_view &nom)
{
	return structures[nom];
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const size_t id)
{
	return structures[nom_structures[id]];
}

std::string ContexteGenerationCode::nom_struct(const size_t id) const
{
	return std::string{nom_structures[id]};
}

/* ************************************************************************** */

void ContexteGenerationCode::differe_noeud(noeud::base *noeud)
{
	m_noeuds_differes.push(noeud);
}

const std::stack<noeud::base *> &ContexteGenerationCode::noeuds_differes() const
{
	return m_noeuds_differes;
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	size_t memoire = sizeof(ContexteGenerationCode);

	/* globales */
	memoire += globales.size() * (sizeof(DonneesVariable) + sizeof(std::string_view));

	/* fonctions */

	/* structures */
	memoire += structures.size() * sizeof(DonneesStructure);

	for (auto const &structure : structures) {
		memoire += structure.second.index_membres.size() * (sizeof(size_t) + sizeof(std::string_view));
		memoire += structure.second.donnees_types.size() * sizeof(DonneesType);
	}

	memoire += nom_structures.size() * sizeof(std::string_view);

	/* m_locales */
	memoire += m_locales.capacity() * sizeof(std::pair<std::string_view, DonneesVariable>);
	memoire += m_pile_nombre_locales.size() * sizeof(size_t);

	/* m_pile_continue */
	memoire += m_pile_continue.size() * sizeof(llvm::BasicBlock *);

	/* m_pile_arrete */
	memoire += m_pile_arrete.size() * sizeof(llvm::BasicBlock *);

	/* magasin_types */
	memoire += magasin_types.donnees_types.size() * sizeof(DonneesType);
	memoire += magasin_types.donnees_type_index.size() * (sizeof(size_t) + sizeof(size_t));

	for (auto module : modules) {
		memoire += module->memoire_utilisee();
	}

	return memoire;
}

Metriques ContexteGenerationCode::rassemble_metriques() const
{
	auto metriques = Metriques{};
	metriques.nombre_modules  = modules.size();
	metriques.temps_validation = this->temps_validation;
	metriques.temps_generation = this->temps_generation;

	for (auto module : modules) {
		metriques.nombre_lignes += module->tampon.nombre_lignes();
		metriques.memoire_tampons += module->tampon.taille_donnees();
		metriques.memoire_morceaux += module->morceaux.size() * sizeof(DonneesMorceaux);
		metriques.nombre_morceaux += module->morceaux.size();
		metriques.temps_analyse += module->temps_analyse;
		metriques.temps_chargement += module->temps_chargement;
		metriques.temps_tampon += module->temps_tampon;
		metriques.temps_decoupage += module->temps_decoupage;
	}

	return metriques;
}

/* ************************************************************************** */

void ContexteGenerationCode::non_sur(bool ouinon)
{
	m_non_sur = ouinon;
}

bool ContexteGenerationCode::non_sur() const
{
	return m_non_sur;
}
