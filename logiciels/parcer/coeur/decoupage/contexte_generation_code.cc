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

#include "contexte_generation_code.hh"

#include "modules.hh"

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		delete module;
	}
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	for (auto module : modules) {
		if (module->chemin == chemin) {
			return nullptr;
		}
	}

	auto module = new DonneesModule;
	module->id = modules.taille();
	module->nom = nom;
	module->chemin = chemin;

	modules.pousse(module);

	return module;
}

DonneesModule *ContexteGenerationCode::module(long index) const
{
	return modules[index];
}

DonneesModule *ContexteGenerationCode::module(const dls::vue_chaine &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::module_existe(const dls::vue_chaine &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

void ContexteGenerationCode::empile_goto_continue(dls::vue_chaine chaine, dls::chaine const &bloc)
{
	m_pile_goto_continue.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_continue()
{
	m_pile_goto_continue.pop_back();
}

dls::chaine ContexteGenerationCode::goto_continue(dls::vue_chaine chaine)
{
	if (m_pile_goto_continue.est_vide()) {
		return "";
	}

	if (chaine.est_vide()) {
		return m_pile_goto_continue.back().second;
	}

	for (auto const &paire : m_pile_goto_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::empile_goto_arrete(dls::vue_chaine chaine, dls::chaine const &bloc)
{
	m_pile_goto_arrete.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_arrete()
{
	m_pile_goto_arrete.pop_back();
}

dls::chaine ContexteGenerationCode::goto_arrete(dls::vue_chaine chaine)
{
	if (m_pile_goto_arrete.est_vide()) {
		return "";
	}

	if (chaine.est_vide()) {
		return m_pile_goto_arrete.back().second;
	}

	for (auto const &paire : m_pile_goto_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::pousse_globale(const dls::vue_chaine &nom, DonneesVariable const &donnees)
{
	/* Nous utilisons ça plutôt que 'insert' car la valeur est poussée deux fois :
	 * - la première fois, nulle, lors de la validation sémantique,
	 * - la deuxième fois, correcte, lors de la génération du code.
	 *
	 * 'insert' n'insert pas si la valeur existe déjà, donc nous nous
	 * retrouvions avec un pointeur nul. */
	globales[nom] = donnees;
}

bool ContexteGenerationCode::globale_existe(const dls::vue_chaine &nom)
{
	auto iter = globales.trouve(nom);

	if (iter == globales.fin()) {
		return false;
	}

	return true;
}

size_t ContexteGenerationCode::type_globale(const dls::vue_chaine &nom)
{
	auto iter = globales.trouve(nom);

	if (iter == globales.fin()) {
		return -1ul;
	}

	return iter->second.donnees_type;
}

conteneur_globales::const_iteratrice ContexteGenerationCode::iter_globale(const dls::vue_chaine &nom)
{
	return globales.trouve(nom);
}

conteneur_globales::const_iteratrice ContexteGenerationCode::fin_globales()
{
	return globales.fin();
}

void ContexteGenerationCode::pousse_locale(
		const dls::vue_chaine &nom,
		DonneesVariable const &donnees)
{
	if (m_locales.taille() > m_nombre_locales) {
		m_locales[m_nombre_locales] = { nom, donnees };
	}
	else {
		m_locales.pousse({nom, donnees});
	}

	++m_nombre_locales;
}

char ContexteGenerationCode::drapeaux_variable(dls::vue_chaine const &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return 0;
	}

	return iter->second.drapeaux;
}

DonneesVariable &ContexteGenerationCode::donnees_variable(dls::vue_chaine const &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter != iter_fin) {
		return iter->second;
	}

	auto iter_g = globales.trouve(nom);

	if (iter_g != globales.fin()) {
		return iter_g->second;
	}

	std::cerr << "Échec recherche " << nom << '\n';
	throw "";
}

bool ContexteGenerationCode::locale_existe(const dls::vue_chaine &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return false;
	}

	return true;
}

size_t ContexteGenerationCode::type_locale(const dls::vue_chaine &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;
	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return -1ul;
	}

	return iter->second.donnees_type;
}

bool ContexteGenerationCode::peut_etre_assigne(const dls::vue_chaine &nom)
{
	auto iter = iter_locale(nom);

	if (iter == fin_locales()) {
		return false;
	}

	return iter->second.est_dynamique;
}

void ContexteGenerationCode::empile_nombre_locales()
{
	m_pile_nombre_locales.empile(m_nombre_locales);
}

void ContexteGenerationCode::depile_nombre_locales()
{
	auto nombre_locales = m_pile_nombre_locales.haut();
	/* nous ne pouvons pas avoir moins de locales en sortant du bloc */
	assert(nombre_locales <= m_nombre_locales);
	m_nombre_locales = nombre_locales;
	m_pile_nombre_locales.depile();
}

void ContexteGenerationCode::imprime_locales(std::ostream &os)
{
	for (long i = 0; i < m_nombre_locales; ++i) {
		os << '\t' << m_locales[i].first << '\n';
	}
}

bool ContexteGenerationCode::est_locale_variadique(const dls::vue_chaine &nom)
{
	auto iter = iter_locale(nom);

	if (iter == fin_locales()) {
		return false;
	}

	return iter->second.est_variadic;
}

conteneur_locales::const_iteratrice ContexteGenerationCode::iter_locale(const dls::vue_chaine &nom) const
{
	auto iter_fin = fin_locales();
	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	return iter;
}

conteneur_locales::const_iteratrice ContexteGenerationCode::debut_locales() const
{
	return m_locales.debut();
}

conteneur_locales::const_iteratrice ContexteGenerationCode::fin_locales() const
{
	return m_locales.debut() + static_cast<long>(m_nombre_locales);
}

void ContexteGenerationCode::commence_fonction(DonneesFonction *df)
{
	this->donnees_fonction = df;
	m_nombre_locales = 0;
	m_locales.efface();
}

void ContexteGenerationCode::termine_fonction()
{
	this->donnees_fonction = nullptr;
	m_nombre_locales = 0;
	m_locales.efface();

	while (!m_noeuds_differes.est_vide()) {
		m_noeuds_differes.pop_back();
	}
}

bool ContexteGenerationCode::structure_existe(const dls::vue_chaine &nom)
{
	return structures.trouve(nom) != structures.fin();
}

long ContexteGenerationCode::ajoute_donnees_structure(const dls::vue_chaine &nom, DonneesStructure &donnees)
{
	donnees.id = nom_structures.taille();

	auto dt = DonneesType{};
	dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(donnees.id << 8));

	donnees.index_type = magasin_types.ajoute_type(dt);

	structures.insere({nom, donnees});
	nom_structures.pousse(nom);

	return donnees.id;
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const dls::vue_chaine &nom)
{
	return structures[nom];
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const long id)
{
	return structures[nom_structures[id]];
}

dls::chaine ContexteGenerationCode::nom_struct(const long id) const
{
	return dls::chaine{nom_structures[id]};
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	size_t memoire = sizeof(ContexteGenerationCode);

	/* globales */
	memoire += static_cast<size_t>(globales.taille()) * (sizeof(DonneesVariable) + sizeof(dls::vue_chaine));

	/* fonctions */

	/* structures */
	memoire += static_cast<size_t>(structures.taille()) * sizeof(DonneesStructure);

	for (auto const &structure : structures) {
		memoire += static_cast<size_t>(structure.second.donnees_membres.taille()) * (sizeof(DonneesMembre) + sizeof(dls::vue_chaine));
		memoire += static_cast<size_t>(structure.second.donnees_types.taille()) * sizeof(DonneesType);
	}

	memoire += static_cast<size_t>(nom_structures.taille()) * sizeof(dls::vue_chaine);

	/* m_locales */
	memoire += static_cast<size_t>(m_locales.capacite()) * sizeof(std::pair<dls::vue_chaine, DonneesVariable>);
	memoire += static_cast<size_t>(m_pile_nombre_locales.taille()) * sizeof(size_t);

	/* magasin_types */
	memoire += static_cast<size_t>(magasin_types.donnees_types.taille()) * sizeof(DonneesType);
	memoire += static_cast<size_t>(magasin_types.donnees_type_index.taille()) * (sizeof(size_t) + sizeof(size_t));

	for (auto module : modules) {
		memoire += module->memoire_utilisee();
	}

	return memoire;
}

Metriques ContexteGenerationCode::rassemble_metriques() const
{
	auto metriques = Metriques{};
	metriques.nombre_modules  = static_cast<size_t>(modules.taille());
	metriques.temps_validation = this->temps_validation;
	metriques.temps_generation = this->temps_generation;

	for (auto module : modules) {
		metriques.nombre_lignes += module->tampon.nombre_lignes();
		metriques.memoire_tampons += module->tampon.taille_donnees();
		metriques.memoire_morceaux += static_cast<size_t>(module->morceaux.taille()) * sizeof(DonneesMorceaux);
		metriques.nombre_morceaux += static_cast<size_t>(module->morceaux.taille());
		metriques.temps_analyse += module->temps_analyse;
		metriques.temps_chargement += module->temps_chargement;
		metriques.temps_tampon += module->temps_tampon;
		metriques.temps_decoupage += module->temps_decoupage;
	}

	return metriques;
}
