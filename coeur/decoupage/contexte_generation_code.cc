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

ContexteGenerationCode::ContexteGenerationCode(const TamponSource &tampon_source)
	: tampon(tampon_source)
	, module(nullptr)
	, contexte{}
{}

llvm::BasicBlock *ContexteGenerationCode::bloc_courant() const
{
	return m_bloc_courant;
}

void ContexteGenerationCode::bloc_courant(llvm::BasicBlock *bloc)
{
	m_bloc_courant = bloc;
}

void ContexteGenerationCode::empile_bloc_continue(llvm::BasicBlock *bloc)
{
	m_pile_continue.push(bloc);
}

void ContexteGenerationCode::depile_bloc_continue()
{
	m_pile_continue.pop();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_continue()
{
	return m_pile_continue.empty() ? nullptr : m_pile_continue.top();
}

void ContexteGenerationCode::empile_bloc_arrete(llvm::BasicBlock *bloc)
{
	m_pile_arrete.push(bloc);
}

void ContexteGenerationCode::depile_bloc_arrete()
{
	m_pile_arrete.pop();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_arrete()
{
	return m_pile_arrete.empty() ? nullptr : m_pile_arrete.top();
}

void ContexteGenerationCode::pousse_globale(const std::string_view &nom, llvm::Value *valeur, const DonneesType &type)
{
	globales.insert({nom, {valeur, type}});
}

llvm::Value *ContexteGenerationCode::valeur_globale(const std::string_view &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return nullptr;
	}

	return iter->second.valeur;
}

const DonneesType &ContexteGenerationCode::type_globale(const std::string_view &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return this->m_donnees_type_invalide;
	}

	return iter->second.donnees_type;
}

void ContexteGenerationCode::pousse_locale(const std::string_view &nom, llvm::Value *valeur, const DonneesType &type, const bool est_variable)
{
	if (m_locales.size() > m_nombre_locales) {
		m_locales[m_nombre_locales] = {nom, {valeur, type, est_variable, {}}};
	}
	else {
		m_locales.push_back({nom, {valeur, type, est_variable, {}}});
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

const DonneesType &ContexteGenerationCode::type_locale(const std::string_view &nom)
{
	auto iter_fin = m_locales.begin() + static_cast<long>(m_nombre_locales);
	auto iter = std::find_if(m_locales.begin(), iter_fin,
							 [&](const std::pair<std::string_view, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return this->m_donnees_type_invalide;
	}

	return iter->second.donnees_type;
}

bool ContexteGenerationCode::peut_etre_assigne(const std::string_view &nom)
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

	return iter->second.est_variable;
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

void ContexteGenerationCode::ajoute_donnees_fonctions(const std::string_view &nom, const DonneesFonction &donnees)
{
	fonctions.insert({nom, donnees});
}

const DonneesFonction &ContexteGenerationCode::donnees_fonction(const std::string_view &nom)
{
	return fonctions[nom];
}

bool ContexteGenerationCode::fonction_existe(const std::string_view &nom)
{
	return fonctions.find(nom) != fonctions.end();
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

size_t ContexteGenerationCode::memoire_utilisee() const
{
	size_t memoire = sizeof(ContexteGenerationCode);

	/* globales */
	memoire += globales.size() * (sizeof(DonneesVariable) + sizeof(std::string_view));

	/* fonctions */
	memoire += fonctions.size() * (sizeof(DonneesFonction) + sizeof(std::string_view));

	for (const auto &fonction : fonctions) {
		memoire += fonction.second.args.size() * (sizeof(DonneesArgument) + sizeof(std::string_view));
	}

	/* structures */
	memoire += structures.size() * sizeof(DonneesStructure);

	for (const auto &structure : structures) {
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

	return memoire;
}
