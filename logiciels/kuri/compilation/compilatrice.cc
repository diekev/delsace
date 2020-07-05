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

#include "compilatrice.hh"

#include <filesystem>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/flux/outils.h"

#include "assembleuse_arbre.h"
#include "erreur.h"
#include "modules.hh"

Compilatrice::Compilatrice()
	: assembleuse(memoire::loge<assembleuse_arbre>("assembleuse_arbre", *this))
	, typeuse(graphe_dependance, this->operateurs)
	, constructrice_ri(*this)
{
	auto table = table_identifiants.verrou_ecriture();
	initialise_identifiants(*table);

	auto ops = operateurs.verrou_ecriture();
	enregistre_operateurs_basiques(*this, *ops);

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

Compilatrice::~Compilatrice()
{
	auto modules_ = modules.verrou_lecture();
	for (auto module : *modules_) {
		memoire::deloge("Module", module);
	}

	auto fichiers_ = fichiers.verrou_lecture();
	for (auto fichier : *fichiers_) {
		memoire::deloge("Fichier", fichier);
	}

	memoire::deloge("assembleuse_arbre", assembleuse);
}

Module *Compilatrice::importe_module(const dls::chaine &nom, const Lexeme &lexeme)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						*this,
						&lexeme,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					*this,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
	auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
	auto nom_dossier = chemin_absolu.filename();

	auto module = cree_module(nom_dossier.c_str(), chemin_absolu.c_str());

	if (module->importe) {
		return module;
	}

	module->importe = true;

	std::cout << "Importation du module : " << nom_dossier << " (" << chemin_absolu << ")" << std::endl;

	for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
		auto chemin_entree = entree.path();

		if (!std::filesystem::is_regular_file(chemin_entree)) {
			continue;
		}

		if (chemin_entree.extension() != ".kuri") {
			continue;
		}

		ajoute_fichier_a_la_compilation(chemin_entree.stem().c_str(), module, {});
	}

	return module;
}

/* ************************************************************************** */

Module *Compilatrice::cree_module(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	auto chemin_corrige = chemin;

	if (chemin_corrige[chemin_corrige.taille() - 1] != '/') {
		chemin_corrige.append('/');
	}

	auto modules_ = modules.verrou_ecriture();
	for (auto module : *modules_) {
		if (module->chemin == chemin_corrige) {
			return module;
		}
	}

	auto module = memoire::loge<Module>("Module", *this);
	module->id = static_cast<size_t>(modules_->taille());
	module->nom = nom;
	module->chemin = chemin_corrige;

	modules_->pousse(module);

	return module;
}

Module *Compilatrice::module(size_t index) const
{
	return modules->a(static_cast<long>(index));
}

Module *Compilatrice::module(const dls::vue_chaine_compacte &nom) const
{
	auto modules_ = modules.verrou_lecture();
	for (auto module : *modules_) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool Compilatrice::module_existe(const dls::vue_chaine_compacte &nom) const
{
	auto modules_ = modules.verrou_lecture();
	for (auto module : *modules_) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		Compilatrice &compilatrice,
		Lexeme const &lexeme)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					compilatrice,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = fichier.tellg();
	fichier.seekg(0, fichier.beg);

	dls::chaine res;
	res.reserve(taille_fichier);

	dls::flux::pour_chaque_ligne(fichier, [&](dls::chaine const &ligne)
	{
		res += ligne;
		res.pousse('\n');
	});

	return res;
}

void Compilatrice::ajoute_fichier_a_la_compilation(const dls::chaine &nom, Module *module, const Lexeme &lexeme)
{
	auto chemin = module->chemin + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					*this,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*this,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du fichier */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto fichier = cree_fichier(nom.c_str(), chemin_absolu.c_str());

	if (fichier == nullptr) {
		/* le fichier a déjà été chargé */
		return;
	}

	std::cout << "Chargement du fichier : " << chemin << std::endl;

	fichier->module = module;

	auto debut_chargement = dls::chrono::compte_seconde();
	auto tampon = charge_fichier(chemin, *this, lexeme);
	fichier->temps_chargement = debut_chargement.temps();

	auto debut_tampon = dls::chrono::compte_seconde();
	fichier->tampon = lng::tampon_source(tampon);
	fichier->temps_tampon = debut_tampon.temps();

	auto unite = UniteCompilation();
	unite.fichier = fichier;
	unite.etat = UniteCompilation::Etat::PARSAGE_ATTENDU;

	file_compilation->pousse(unite);
}

Fichier *Compilatrice::cree_fichier(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	auto fichiers_ = fichiers.verrou_ecriture();

	for (auto fichier : *fichiers_) {
		if (fichier->chemin == chemin) {
			return nullptr;
		}
	}

	auto fichier = memoire::loge<Fichier>("Fichier");
	fichier->id = static_cast<size_t>(fichiers_->taille());
	fichier->nom = nom;
	fichier->chemin = chemin;

	if (importe_kuri) {
		fichier->modules_importes.insere("Kuri");
	}

	fichiers_->pousse(fichier);

	return fichier;
}

Fichier *Compilatrice::fichier(size_t index) const
{
	return fichiers->a(static_cast<long>(index));
}

Fichier *Compilatrice::fichier(const dls::vue_chaine_compacte &nom) const
{
	auto fichiers_ = fichiers.verrou_lecture();

	for (auto fichier : *fichiers_) {
		if (fichier->nom == nom) {
			return fichier;
		}
	}

	return nullptr;
}

bool Compilatrice::fichier_existe(const dls::vue_chaine_compacte &nom) const
{
	auto fichiers_ = fichiers.verrou_lecture();

	for (auto fichier : *fichiers_) {
		if (fichier->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

void Compilatrice::ajoute_inclusion(const dls::chaine &fichier)
{
	if (deja_inclus.trouve(fichier) != deja_inclus.fin()) {
		return;
	}

	deja_inclus.insere(fichier);
	inclusions.pousse(fichier);
}

bool Compilatrice::compilation_terminee() const
{
	return possede_erreur || (file_compilation->est_vide() && file_execution->est_vide());
}

/* ************************************************************************** */

void Compilatrice::ajoute_unite_compilation_pour_typage(NoeudExpression *expression)
{
	auto unite = UniteCompilation();
	unite.noeud = expression;
	unite.etat = UniteCompilation::Etat::TYPAGE_ATTENDU;
	unite.etat_original = UniteCompilation::Etat::TYPAGE_ATTENDU;

	file_compilation->pousse(unite);
}

void Compilatrice::ajoute_unite_compilation_entete_fonction(NoeudDeclarationFonction *decl)
{
	auto unite = UniteCompilation();
	unite.noeud = decl;
	unite.etat = UniteCompilation::Etat::TYPAGE_ENTETE_FONCTION_ATTENDU;
	unite.etat_original = UniteCompilation::Etat::TYPAGE_ENTETE_FONCTION_ATTENDU;

	file_compilation->pousse(unite);
}

/* ************************************************************************** */

size_t Compilatrice::memoire_utilisee() const
{
	auto memoire = sizeof(Compilatrice);

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
	memoire += static_cast<size_t>(modules->taille()) * sizeof(Module *);
	memoire += static_cast<size_t>(fichiers->taille()) * sizeof(Fichier *);

	auto modules_ = modules.verrou_lecture();
	POUR (*modules_) {
		memoire += static_cast<size_t>(it->fichiers.taille()) * sizeof(Fichier *);
		memoire += static_cast<size_t>(it->nom.taille());
		memoire += static_cast<size_t>(it->chemin.taille());

		if (!it->fonctions_exportees.est_stocke_dans_classe()) {
			memoire += static_cast<size_t>(it->fonctions_exportees.taille()) * sizeof(dls::vue_chaine_compacte);
		}
	}

	auto fichiers_ = fichiers.verrou_lecture();
	POUR (*fichiers_) {
		// les autres membres sont gérés dans rassemble_metriques()
		if (!it->modules_importes.est_stocke_dans_classe()) {
			memoire += static_cast<size_t>(it->modules_importes.taille()) * sizeof(dls::vue_chaine_compacte);
		}
	}

	memoire += static_cast<size_t>(file_compilation->taille()) * sizeof(UniteCompilation);
	memoire += static_cast<size_t>(file_execution->taille()) * sizeof(NoeudDirectiveExecution *);
	memoire += table_identifiants->memoire_utilisee();

	memoire += static_cast<size_t>(gerante_chaine.m_table.taille()) * sizeof(dls::chaine);
	POUR (gerante_chaine.m_table) {
		memoire += static_cast<size_t>(it.taille);
	}

	return memoire;
}

Metriques Compilatrice::rassemble_metriques() const
{
	auto operateurs_ = operateurs.verrou_lecture();
	auto graphe = graphe_dependance.verrou_lecture();

	auto metriques = Metriques{};
	metriques.nombre_modules  = static_cast<size_t>(modules->taille());
	metriques.temps_validation = this->temps_validation;
	metriques.temps_generation = this->temps_generation;
	metriques.memoire_types = this->typeuse.memoire_utilisee();
	metriques.memoire_operateurs = operateurs_->memoire_utilisee();
	metriques.memoire_graphe = graphe->memoire_utilisee();
	metriques.memoire_arbre += this->allocatrice_noeud.memoire_utilisee();
	metriques.nombre_noeuds += this->allocatrice_noeud.nombre_noeuds();

	metriques.nombre_noeuds_deps = static_cast<size_t>(graphe->noeuds.taille());
	metriques.nombre_types = typeuse.nombre_de_types();

	POUR (operateurs_->operateurs_unaires) {
		metriques.nombre_operateurs += it.second.taille();
	}

	POUR (operateurs_->operateurs_binaires) {
		metriques.nombre_operateurs += it.second.taille();
	}

	auto fichiers_ = fichiers.verrou_lecture();
	for (auto fichier : *fichiers_) {
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
