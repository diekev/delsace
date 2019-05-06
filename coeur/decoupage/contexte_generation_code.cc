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

#include "broyage.hh"
#include "modules.hh"

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		delete module;
	}

	delete menageur_fonctions;
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(
		std::string const &nom,
		std::string const &chemin)
{
	for (auto module : modules) {
		if (module->chemin == chemin) {
			return nullptr;
		}
	}

	auto module = new DonneesModule;
	module->id = modules.size();
	module->nom = nom;
	module->chemin = chemin;

	/* La fonction memoire_utilisee est définie globalement donc doit être
	 * définie dans chaque module. */
	auto nom_fonction = "mémoire_utilisée";

	auto donnees_fonctions = DonneesFonction();
	auto dt = DonneesType{};
	dt.pousse(id_morceau::FONCTION);
	dt.pousse(id_morceau::PARENTHESE_OUVRANTE);
	dt.pousse(id_morceau::PARENTHESE_FERMANTE);
	dt.pousse(id_morceau::Z64);
	donnees_fonctions.index_type = magasin_types.ajoute_type(dt);
	donnees_fonctions.index_type_retour = magasin_types[TYPE_Z64];
	donnees_fonctions.nom_broye = broye_nom_fonction(nom_fonction, "");

	module->fonctions_exportees.insert(nom_fonction);
	module->ajoute_donnees_fonctions(nom_fonction, donnees_fonctions);

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

void ContexteGenerationCode::empile_goto_continue(std::string_view chaine, std::string const &bloc)
{
	m_pile_goto_continue.push_back({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_continue()
{
	m_pile_goto_continue.pop_back();
}

std::string ContexteGenerationCode::goto_continue(std::string_view chaine)
{
	if (m_pile_goto_continue.empty()) {
		return "";
	}

	if (chaine.empty()) {
		return m_pile_goto_continue.back().second;
	}

	for (auto const &paire : m_pile_goto_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::empile_goto_arrete(std::string_view chaine, std::string const &bloc)
{
	m_pile_goto_arrete.push_back({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_arrete()
{
	m_pile_goto_arrete.pop_back();
}

std::string ContexteGenerationCode::goto_arrete(std::string_view chaine)
{
	if (m_pile_goto_arrete.empty()) {
		return "";
	}

	if (chaine.empty()) {
		return m_pile_goto_arrete.back().second;
	}

	for (auto const &paire : m_pile_goto_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
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
		m_locales[m_nombre_locales] = {nom, {valeur, index_type, est_variable, est_variadique, 0, {}}};
	}
	else {
		m_locales.push_back({nom, {valeur, index_type, est_variable, est_variadique, 0, {}}});
	}

	++m_nombre_locales;
}

void ContexteGenerationCode::pousse_locale(
		std::string_view const &nom,
		size_t const &index_type,
		char drapeaux)
{
	if (m_locales.size() > m_nombre_locales) {
		m_locales[m_nombre_locales] = {nom, {nullptr, index_type, false, false, drapeaux, {}}};
	}
	else {
		m_locales.push_back({nom, {nullptr, index_type, false, false, drapeaux, {}}});
	}

	++m_nombre_locales;
}

char ContexteGenerationCode::drapeaux_variable(std::string_view const &nom)
{
	auto iter_fin = m_locales.begin() + static_cast<long>(m_nombre_locales);

	auto iter = std::find_if(m_locales.begin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return 0;
	}

	return iter->second.drapeaux;
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
	m_pile_nombre_differes.push(m_nombre_differes);
}

void ContexteGenerationCode::depile_nombre_locales()
{
	auto nombre_locales = m_pile_nombre_locales.top();
	/* nous ne pouvons pas avoir moins de locales en sortant du bloc */
	assert(nombre_locales <= m_nombre_locales);
	m_nombre_locales = nombre_locales;
	m_pile_nombre_locales.pop();

	auto nombre_differes = m_pile_nombre_differes.top();
	/* nous ne pouvons pas avoir moins de noeuds différés en sortant du bloc */
	assert(nombre_differes <= m_nombre_differes);
	m_nombre_differes = nombre_differes;
	m_pile_nombre_differes.pop();

	while (m_noeuds_differes.size() > nombre_differes) {
		m_noeuds_differes.pop_back();
	}
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

	while (!m_noeuds_differes.empty()) {
		m_noeuds_differes.pop_back();
	}
}

bool ContexteGenerationCode::structure_existe(const std::string_view &nom)
{
	return structures.find(nom) != structures.end();
}

size_t ContexteGenerationCode::ajoute_donnees_structure(const std::string_view &nom, DonneesStructure &donnees)
{
	donnees.id = nom_structures.size();
	donnees.type_llvm = nullptr;

	auto dt = DonneesType{};
	dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(donnees.id << 8));

	donnees.index_type = magasin_types.ajoute_type(dt);

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
	m_noeuds_differes.push_back(noeud);
	++m_nombre_differes;
}

std::vector<noeud::base *> const &ContexteGenerationCode::noeuds_differes() const
{
	return m_noeuds_differes;
}

std::vector<noeud::base *> ContexteGenerationCode::noeuds_differes_bloc() const
{
	auto idx_debut = m_pile_nombre_differes.top();
	auto idx_fin = m_nombre_differes;

	if (idx_debut == idx_fin) {
		return {};
	}

	auto liste = std::vector<noeud::base *>{};
	liste.reserve(idx_fin - idx_debut);

	for (auto i = idx_debut; i < idx_fin; ++i) {
		liste.push_back(m_noeuds_differes[i]);
	}

	return liste;
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
		memoire += structure.second.donnees_membres.size() * (sizeof(DonneesMembre) + sizeof(std::string_view));
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
