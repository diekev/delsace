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

#include "assembleuse_arbre.h"
#include "modules.hh"

ContexteGenerationCode::ContexteGenerationCode()
	: assembleuse(memoire::loge<assembleuse_arbre>("assembleuse_arbre", *this))
	, typeuse(graphe_dependance, this->operateurs)
{
	enregistre_operateurs_basiques(*this, this->operateurs);

	/* Pour fprintf dans les messages d'erreurs, nous incluons toujours "stdio.h". */
	this->ajoute_inclusion("stdio.h");
	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	this->ajoute_inclusion("stdlib.h");
	/* Pour strlen, nous incluons toujours "string.h". */
	this->ajoute_inclusion("string.h");
	/* Pour les coroutines nous incluons toujours pthread */
	this->ajoute_inclusion("pthread.h");
	this->bibliotheques_dynamiques.pousse("pthread");
	this->definitions.pousse("_REENTRANT");
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

void ContexteGenerationCode::ajoute_inclusion(const dls::chaine &fichier)
{
	if (deja_inclus.trouve(fichier) != deja_inclus.fin()) {
		return;
	}

	deja_inclus.insere(fichier);
	inclusions.pousse(fichier);
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	auto memoire = sizeof(ContexteGenerationCode);

	memoire += static_cast<size_t>(deja_inclus.taille()) * sizeof(dls::chaine);
	POUR (deja_inclus) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(inclusions.taille()) * sizeof(dls::chaine);
	POUR (inclusions) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(bibliotheques_dynamiques.taille()) * sizeof(dls::chaine);
	POUR (bibliotheques_dynamiques) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(bibliotheques_statiques.taille()) * sizeof(dls::chaine);
	POUR (bibliotheques_statiques) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(chemins.taille()) * sizeof(dls::vue_chaine_compacte);
	memoire += static_cast<size_t>(definitions.taille()) * sizeof(dls::vue_chaine_compacte);
	memoire += static_cast<size_t>(modules.taille()) * sizeof(DonneesModule *);
	memoire += static_cast<size_t>(fichiers.taille()) * sizeof(Fichier *);

	POUR (modules) {
		memoire += static_cast<size_t>(it->fichiers.taille()) * sizeof(Fichier *);
		memoire += static_cast<size_t>(it->nom.taille());
		memoire += static_cast<size_t>(it->chemin.taille());

		if (!it->fonctions_exportees.est_stocke_dans_classe()) {
			memoire += static_cast<size_t>(it->fonctions_exportees.taille()) * sizeof(dls::vue_chaine_compacte);
		}
	}

	POUR (fichiers) {
		// les autres membres sont gérés dans rassemble_metriques()
		if (!it->modules_importes.est_stocke_dans_classe()) {
			memoire += static_cast<size_t>(it->modules_importes.taille()) * sizeof(dls::vue_chaine_compacte);
		}
	}

	memoire += static_cast<size_t>(file_typage.taille()) * sizeof(NoeudExpression *);
	memoire += static_cast<size_t>(noeuds_a_executer.taille()) * sizeof(NoeudExpression *);
	memoire += table_identifiants.memoire_utilisee();

	memoire += static_cast<size_t>(gerante_chaine.m_table.taille()) * sizeof(dls::chaine);
	POUR (gerante_chaine.m_table) {
		memoire += static_cast<size_t>(it.taille);
	}

	memoire += static_cast<size_t>(paires_expansion_gabarit.taille()) * (sizeof (Type *) + sizeof (dls::vue_chaine_compacte));

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
	metriques.memoire_graphe = this->graphe_dependance.memoire_utilisee();
	metriques.memoire_arbre += this->allocatrice_noeud.memoire_utilisee();
	metriques.nombre_noeuds += this->allocatrice_noeud.nombre_noeuds();

	metriques.nombre_noeuds_deps = static_cast<size_t>(this->graphe_dependance.noeuds.taille());
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
