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

#ifdef AVEC_LLVM
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <llvm/IR/LegacyPassManager.h>
#pragma GCC diagnostic pop
#endif

#include "broyage.hh"
#include "modules.hh"

ContexteGenerationCode::ContexteGenerationCode()
	: magasin_types(graphe_dependance)
{
	enregistre_operateurs_basiques(*this, this->operateurs);

	/* À FAIRE : type du pointeur de __contexte_global */
	auto dt = DonneesTypeFinal{};
	dt.pousse(id_morceau::POINTEUR);
	dt.pousse(id_morceau::CHAINE_CARACTERE);
	this->index_type_ctx = magasin_types.ajoute_type(dt);
}

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		memoire::deloge("DonneesModule", module);
	}

	for (auto fichier : fichiers) {
		memoire::deloge("Fichier", fichier);
	}

#ifdef AVEC_LLVM
	delete menageur_fonctions;
#endif
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	auto chemin_corrige = chemin;

	if (chemin_corrige[chemin_corrige.taille() - 1] != '/') {
		chemin_corrige.append('/');
	}

	for (auto module : modules) {
		if (module->chemin == chemin_corrige) {
			return module;
		}
	}

	auto module = memoire::loge<DonneesModule>("DonneesModule");
	module->id = static_cast<size_t>(modules.taille());
	module->nom = nom;
	module->chemin = chemin_corrige;

	modules.pousse(module);

	return module;
}

DonneesModule *ContexteGenerationCode::module(size_t index) const
{
	return modules[static_cast<long>(index)];
}

DonneesModule *ContexteGenerationCode::module(const dls::vue_chaine_compacte &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::module_existe(const dls::vue_chaine_compacte &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

Fichier *ContexteGenerationCode::cree_fichier(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	for (auto fichier : fichiers) {
		if (fichier->chemin == chemin) {
			return nullptr;
		}
	}

	auto fichier = memoire::loge<Fichier>("Fichier");
	fichier->id = static_cast<size_t>(fichiers.taille());
	fichier->nom = nom;
	fichier->chemin = chemin;

	fichiers.pousse(fichier);

	return fichier;
}

Fichier *ContexteGenerationCode::fichier(size_t index) const
{
	return fichiers[static_cast<long>(index)];
}

Fichier *ContexteGenerationCode::fichier(const dls::vue_chaine_compacte &nom) const
{
	for (auto fichier : fichiers) {
		if (fichier->nom == nom) {
			return fichier;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::fichier_existe(const dls::vue_chaine_compacte &nom) const
{
	for (auto fichier : fichiers) {
		if (fichier->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

#ifdef AVEC_LLVM
llvm::BasicBlock *ContexteGenerationCode::bloc_courant() const
{
	return m_bloc_courant;
}

void ContexteGenerationCode::bloc_courant(llvm::BasicBlock *bloc)
{
	m_bloc_courant = bloc;
}

void ContexteGenerationCode::empile_bloc_continue(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc)
{
	m_pile_continue.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_bloc_continue()
{
	m_pile_continue.pop_back();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_continue(dls::vue_chaine_compacte chaine)
{
	if (m_pile_continue.est_vide()) {
		return nullptr;
	}

	if (chaine.est_vide()) {
		return m_pile_continue.back().second;
	}

	for (auto const &paire : m_pile_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}

void ContexteGenerationCode::empile_bloc_arrete(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc)
{
	m_pile_arrete.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_bloc_arrete()
{
	m_pile_arrete.pop_back();
}

llvm::BasicBlock *ContexteGenerationCode::bloc_arrete(dls::vue_chaine_compacte chaine)
{
	if (m_pile_arrete.est_vide()) {
		return nullptr;
	}

	if (chaine.est_vide()) {
		return m_pile_arrete.back().second;
	}

	for (auto const &paire : m_pile_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}
#endif

void ContexteGenerationCode::empile_goto_continue(dls::vue_chaine_compacte chaine, dls::chaine const &bloc)
{
	m_pile_goto_continue.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_continue()
{
	m_pile_goto_continue.pop_back();
}

dls::chaine ContexteGenerationCode::goto_continue(dls::vue_chaine_compacte chaine)
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

void ContexteGenerationCode::empile_goto_arrete(dls::vue_chaine_compacte chaine, dls::chaine const &bloc)
{
	m_pile_goto_arrete.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_arrete()
{
	m_pile_goto_arrete.pop_back();
}

dls::chaine ContexteGenerationCode::goto_arrete(dls::vue_chaine_compacte chaine)
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

void ContexteGenerationCode::pousse_globale(const dls::vue_chaine_compacte &nom, DonneesVariable const &donnees)
{
	/* Nous utilisons ça plutôt que 'insert' car la valeur est poussée deux fois :
	 * - la première fois, nulle, lors de la validation sémantique,
	 * - la deuxième fois, correcte, lors de la génération du code.
	 *
	 * 'insert' n'insert pas si la valeur existe déjà, donc nous nous
	 * retrouvions avec un pointeur nul. */
	globales[nom] = donnees;
}

#ifdef AVEC_LLVM
llvm::Value *ContexteGenerationCode::valeur_globale(const dls::vue_chaine_compacte &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.fin()) {
		return nullptr;
	}

	return iter->second.valeur;
}
#endif

bool ContexteGenerationCode::globale_existe(const dls::vue_chaine_compacte &nom)
{
	auto iter = globales.trouve(nom);

	if (iter == globales.fin()) {
		return false;
	}

	return true;
}

long ContexteGenerationCode::type_globale(const dls::vue_chaine_compacte &nom)
{
	auto iter = globales.trouve(nom);

	if (iter == globales.fin()) {
		return -1l;
	}

	return iter->second.index_type;
}

conteneur_globales::const_iteratrice ContexteGenerationCode::iter_globale(const dls::vue_chaine_compacte &nom)
{
	return globales.trouve(nom);
}

conteneur_globales::const_iteratrice ContexteGenerationCode::fin_globales()
{
	return globales.fin();
}

void ContexteGenerationCode::pousse_locale(
		const dls::vue_chaine_compacte &nom,
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

char ContexteGenerationCode::drapeaux_variable(dls::vue_chaine_compacte const &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return 0;
	}

	return iter->second.drapeaux;
}

DonneesVariable &ContexteGenerationCode::donnees_variable(dls::vue_chaine_compacte const &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
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

#ifdef AVEC_LLVM
llvm::Value *ContexteGenerationCode::valeur_locale(const dls::vue_chaine_compacte &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return nullptr;
	}

	return iter->second.valeur;
}
#endif

bool ContexteGenerationCode::locale_existe(const dls::vue_chaine_compacte &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;

	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return false;
	}

	return true;
}

long ContexteGenerationCode::type_locale(const dls::vue_chaine_compacte &nom)
{
	auto iter_fin = m_locales.debut() + m_nombre_locales;
	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
	{
		return paire.first == nom;
	});

	if (iter == iter_fin) {
		return -1l;
	}

	return iter->second.index_type;
}

bool ContexteGenerationCode::peut_etre_assigne(const dls::vue_chaine_compacte &nom)
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
	m_pile_nombre_differes.empile(m_nombre_differes);
}

void ContexteGenerationCode::depile_nombre_locales()
{
	/* nous ne pouvons pas avoir moins de locales en sortant du bloc */
	assert(m_pile_nombre_locales.haut() <= m_nombre_locales);
	m_nombre_locales = m_pile_nombre_locales.depile();

	/* nous ne pouvons pas avoir moins de noeuds différés en sortant du bloc */
	assert(m_pile_nombre_differes.haut() <= m_nombre_differes);
	m_nombre_differes = m_pile_nombre_differes.depile();

	while (m_noeuds_differes.taille() > m_nombre_differes) {
		m_noeuds_differes.pop_back();
	}
}

void ContexteGenerationCode::imprime_locales(std::ostream &os)
{
	for (auto i = 0; i < m_nombre_locales; ++i) {
		os << '\t' << m_locales[i].first << '\n';
	}
}

bool ContexteGenerationCode::est_locale_variadique(const dls::vue_chaine_compacte &nom)
{
	auto iter = iter_locale(nom);

	if (iter == fin_locales()) {
		return false;
	}

	return iter->second.est_variadic;
}

conteneur_locales::const_iteratrice ContexteGenerationCode::iter_locale(const dls::vue_chaine_compacte &nom) const
{
	auto iter_fin = fin_locales();
	auto iter = std::find_if(m_locales.debut(), iter_fin,
							 [&](const std::pair<dls::vue_chaine_compacte, DonneesVariable> &paire)
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
	return m_locales.debut() + m_nombre_locales;
}

#ifdef AVEC_LLVM
void ContexteGenerationCode::commence_fonction(llvm::Function *f, DonneesFonction *df)
{
	this->fonction = f;
	commence_fonction(df);
}
#endif

void ContexteGenerationCode::commence_fonction(DonneesFonction *df)
{
	this->donnees_fonction = df;
	m_nombre_locales = 0;
	m_locales.efface();
}

void ContexteGenerationCode::termine_fonction()
{
#ifdef AVEC_LLVM
	fonction = nullptr;
	m_bloc_courant = nullptr;
#endif
	this->donnees_fonction = nullptr;
	m_nombre_locales = 0;
	m_locales.efface();

	while (!m_noeuds_differes.est_vide()) {
		m_noeuds_differes.pop_back();
	}
}

bool ContexteGenerationCode::structure_existe(const dls::vue_chaine_compacte &nom)
{
	return structures.trouve(nom) != structures.fin();
}

long ContexteGenerationCode::ajoute_donnees_structure(const dls::vue_chaine_compacte &nom, DonneesStructure &donnees)
{
	donnees.id = nom_structures.taille();
#ifdef AVEC_LLVM
	donnees.type_llvm = nullptr;
#endif

	auto dt = DonneesTypeFinal{};
	dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(donnees.id << 8));

	donnees.index_type = magasin_types.ajoute_type(dt);

	structures.insere({nom, donnees});
	nom_structures.pousse(nom);

	return donnees.id;
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const dls::vue_chaine_compacte &nom)
{
	return structures.trouve(nom)->second;
}

DonneesStructure const &ContexteGenerationCode::donnees_structure(const dls::vue_chaine_compacte &nom) const
{
	return structures.trouve(nom)->second;
}

DonneesStructure &ContexteGenerationCode::donnees_structure(const long id)
{
	return structures.trouve(nom_structures[id])->second;
}

DonneesStructure const &ContexteGenerationCode::donnees_structure(const long id) const
{
	return structures.trouve(nom_structures[id])->second;
}

dls::chaine ContexteGenerationCode::nom_struct(const long id) const
{
	return dls::chaine{nom_structures[id]};
}

/* ************************************************************************** */

void ContexteGenerationCode::differe_noeud(noeud::base *noeud)
{
	m_noeuds_differes.pousse(noeud);
	++m_nombre_differes;
}

dls::tableau<noeud::base *> const &ContexteGenerationCode::noeuds_differes() const
{
	return m_noeuds_differes;
}

dls::tableau<noeud::base *> ContexteGenerationCode::noeuds_differes_bloc() const
{
	auto idx_debut = m_pile_nombre_differes.haut();
	auto idx_fin = m_nombre_differes;

	if (idx_debut == idx_fin) {
		return {};
	}

	auto liste = dls::tableau<noeud::base *>{};
	liste.reserve(idx_fin - idx_debut);

	for (auto i = idx_debut; i < idx_fin; ++i) {
		liste.pousse(m_noeuds_differes[i]);
	}

	return liste;
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	auto memoire = sizeof(ContexteGenerationCode);

	/* globales */
	memoire += static_cast<size_t>(globales.taille()) * (sizeof(DonneesVariable) + sizeof(dls::vue_chaine_compacte));

	/* fonctions */

	/* structures */
	memoire += static_cast<size_t>(structures.taille()) * sizeof(DonneesStructure);

	for (auto const &structure : structures) {
		memoire += static_cast<size_t>(structure.second.donnees_membres.taille()) * (sizeof(DonneesMembre) + sizeof(dls::vue_chaine_compacte));
		memoire += static_cast<size_t>(structure.second.index_types.taille()) * sizeof(DonneesTypeFinal);
	}

	memoire += static_cast<size_t>(nom_structures.taille()) * sizeof(dls::vue_chaine_compacte);

	/* m_locales */
	memoire += static_cast<size_t>(m_locales.capacite()) * sizeof(std::pair<dls::vue_chaine_compacte, DonneesVariable>);
	memoire += static_cast<size_t>(m_pile_nombre_locales.taille()) * sizeof(size_t);

	/* m_pile_continue */
#ifdef AVEC_LLVM
	memoire += static_cast<size_t>(m_pile_continue.taille()) * sizeof(llvm::BasicBlock *);

	/* m_pile_arrete */
	memoire += static_cast<size_t>(m_pile_arrete.taille()) * sizeof(llvm::BasicBlock *);
#endif

	/* magasin_types */
	memoire += static_cast<size_t>(magasin_types.donnees_types.taille()) * sizeof(DonneesTypeFinal);
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

	for (auto fichier : fichiers) {
		metriques.nombre_lignes += fichier->tampon.nombre_lignes();
		metriques.memoire_tampons += fichier->tampon.taille_donnees();
		metriques.memoire_morceaux += static_cast<size_t>(fichier->morceaux.taille()) * sizeof(DonneesMorceau);
		metriques.nombre_morceaux += static_cast<size_t>(fichier->morceaux.taille());
		metriques.temps_analyse += fichier->temps_analyse;
		metriques.temps_chargement += fichier->temps_chargement;
		metriques.temps_tampon += fichier->temps_tampon;
		metriques.temps_decoupage += fichier->temps_decoupage;
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

dls::vue_chaine_compacte ContexteGenerationCode::trouve_membre_actif(const dls::vue_chaine_compacte &nom_union)
{
	for (auto const &paire : membres_actifs) {
		if (paire.first == nom_union) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::renseigne_membre_actif(const dls::vue_chaine_compacte &nom_union, const dls::vue_chaine_compacte &nom_membre)
{
	for (auto &paire : membres_actifs) {
		if (paire.first == nom_union) {
			paire.second = nom_membre;
			return;
		}
	}

	membres_actifs.pousse({ nom_union, nom_membre });
}
