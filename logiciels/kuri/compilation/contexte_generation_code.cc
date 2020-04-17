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

#include "biblinternes/langage/unicode.hh"

#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "modules.hh"

ContexteGenerationCode::ContexteGenerationCode()
	: assembleuse(memoire::loge<assembleuse_arbre>("assembleuse_arbre", *this))
	, typeuse(graphe_dependance, this->operateurs)
{
	enregistre_operateurs_basiques(*this, this->operateurs);
}

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		memoire::deloge("DonneesModule", module);
	}

	for (auto fichier : fichiers) {
		memoire::deloge("Fichier", fichier);
	}

	memoire::deloge("assembleuse_arbre", assembleuse);
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

	auto module = memoire::loge<DonneesModule>("DonneesModule", *this);
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

	if (importe_kuri) {
		fichier->modules_importes.insere("Kuri");
	}

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

void ContexteGenerationCode::empile_controle_boucle(IdentifiantCode *ident_boucle)
{
	m_pile_controle_boucle.pousse(ident_boucle);
}

void ContexteGenerationCode::depile_controle_boucle()
{
	m_pile_controle_boucle.pop_back();
}

bool ContexteGenerationCode::possede_controle_boucle(IdentifiantCode *ident)
{
	if (m_pile_controle_boucle.est_vide()) {
		return false;
	}

	if (ident != nullptr) {
		for (auto ctrl : m_pile_controle_boucle) {
			if (ctrl == ident) {
				return true;
			}
		}
	}

	return ident == nullptr;
}

void ContexteGenerationCode::empile_goto_continue(IdentifiantCode *ident_boucle, dls::chaine const &label)
{
	m_goto_continue.pousse({ ident_boucle, label });
}

void ContexteGenerationCode::depile_goto_continue()
{
	m_goto_continue.pop_back();
}

dls::chaine const &ContexteGenerationCode::goto_continue(IdentifiantCode *ident_boucle)
{
	if (ident_boucle != nullptr) {
		POUR (m_goto_continue) {
			if (it.first == ident_boucle) {
				return it.second;
			}
		}
	}

	return m_goto_continue.back().second;
}

void ContexteGenerationCode::empile_goto_arrete(IdentifiantCode *ident_boucle, dls::chaine const &label)
{
	m_goto_arrete.pousse({ ident_boucle, label });
}

void ContexteGenerationCode::depile_goto_arrete()
{
	m_goto_arrete.pop_back();
}

dls::chaine const &ContexteGenerationCode::goto_arrete(IdentifiantCode *ident_boucle)
{
	if (ident_boucle != nullptr) {
		POUR (m_goto_arrete) {
			if (it.first == ident_boucle) {
				return it.second;
			}
		}
	}

	return m_goto_arrete.back().second;
}

/* ************************************************************************** */

void ContexteGenerationCode::commence_fonction(NoeudDeclarationFonction *df)
{
	this->donnees_fonction = df;
}

void ContexteGenerationCode::termine_fonction()
{
	this->donnees_fonction = nullptr;
	m_pile_controle_boucle.efface();
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	auto memoire = sizeof(ContexteGenerationCode);

	/* À FAIRE : réusinage arbre */
//	for (auto module : modules) {
//		memoire += module->memoire_utilisee();
//	}

	return memoire;
}

Metriques ContexteGenerationCode::rassemble_metriques() const
{
	auto metriques = Metriques{};
	metriques.nombre_modules  = static_cast<size_t>(modules.taille());
	metriques.temps_validation = this->temps_validation;
	metriques.temps_generation = this->temps_generation;
	metriques.memoire_types = this->typeuse.memoire_utilisee();
	metriques.memoire_operateurs = this->operateurs.memoire_utilisee();

	metriques.memoire_arbre += this->allocatrice_noeud.memoire_utilisee();
	metriques.nombre_noeuds += this->allocatrice_noeud.nombre_noeuds();

	metriques.nombre_types = typeuse.nombre_de_types();

	POUR (operateurs.operateurs_unaires) {
		metriques.nombre_operateurs += it.second.taille();
	}

	POUR (operateurs.operateurs_binaires) {
		metriques.nombre_operateurs += it.second.taille();
	}

	for (auto fichier : fichiers) {
		metriques.nombre_lignes += fichier->tampon.nombre_lignes();
		metriques.memoire_tampons += fichier->tampon.taille_donnees();
		metriques.memoire_lexemes += static_cast<size_t>(fichier->lexemes.taille()) * sizeof(Lexeme);
		metriques.nombre_lexemes += static_cast<size_t>(fichier->lexemes.taille());
		metriques.temps_analyse += fichier->temps_analyse;
		metriques.temps_chargement += fichier->temps_chargement;
		metriques.temps_tampon += fichier->temps_tampon;
		metriques.temps_decoupage += fichier->temps_decoupage;
	}

	return metriques;
}

/* ************************************************************************** */

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

/* ************************************************************************** */

GeranteChaine::~GeranteChaine()
{
	POUR (m_table) {
		kuri::detruit_chaine(it);
	}
}

void GeranteChaine::ajoute_chaine(const kuri::chaine &chaine)
{
	m_table.pousse(chaine);
}
