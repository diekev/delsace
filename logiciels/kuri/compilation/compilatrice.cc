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
#include <stdarg.h>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/flux/outils.h"

#include "assembleuse_arbre.h"
#include "erreur.h"
#include "modules.hh"

#include "../executable/options.hh"

static Compilatrice *ptr_compilatrice = nullptr;

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
	this->bibliotheques_dynamiques->pousse("pthread");
	this->definitions->pousse("_REENTRANT");

	ptr_compilatrice = this;
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

	ajoute_message_module_ouvert(module->chemin);

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

	ajoute_message_module_ferme(module->chemin);

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

	ajoute_message_fichier_ouvert(fichier->chemin);

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

	ajoute_message_fichier_ferme(fichier->chemin);

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
	auto infos = infos_inclusions.verrou_ecriture();

	if (infos->deja_inclus.trouve(fichier) != infos->deja_inclus.fin()) {
		return;
	}

	infos->deja_inclus.insere(fichier);
	infos->inclusions.pousse(fichier);
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

	auto infos = infos_inclusions.verrou_lecture();

	memoire += static_cast<size_t>(infos->deja_inclus.taille()) * sizeof(dls::chaine);
	POUR (infos->deja_inclus) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(infos->inclusions.taille()) * sizeof(dls::chaine);
	POUR (infos->inclusions) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(bibliotheques_dynamiques->taille()) * sizeof(dls::chaine);
	POUR (*bibliotheques_dynamiques.verrou_lecture()) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(bibliotheques_statiques->taille()) * sizeof(dls::chaine);
	POUR (*bibliotheques_statiques.verrou_lecture()) {
		memoire += static_cast<size_t>(it.taille());
	}

	memoire += static_cast<size_t>(chemins->taille()) * sizeof(dls::vue_chaine_compacte);
	memoire += static_cast<size_t>(definitions->taille()) * sizeof(dls::vue_chaine_compacte);
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

	memoire += static_cast<size_t>(gerante_chaine->m_table.taille()) * sizeof(dls::chaine);
	POUR (gerante_chaine->m_table) {
		memoire += static_cast<size_t>(it.taille);
	}

	memoire += fonctions.memoire_utilisee();
	memoire += globales.memoire_utilisee();

	pour_chaque_element(fonctions, [&](AtomeFonction const &it)
	{
		memoire += static_cast<size_t>(it.params_entrees.taille) * sizeof(Atome *);
		memoire += static_cast<size_t>(it.params_sorties.taille) * sizeof(Atome *);
		memoire += static_cast<size_t>(it.chunk.capacite);
		memoire += static_cast<size_t>(it.chunk.locales.taille()) * sizeof(Locale);
		memoire += static_cast<size_t>(it.chunk.decalages_labels.taille()) * sizeof(int);
	});

	memoire += messagere->memoire_utilisee();

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

	auto memoire_mv = 0ul;
	memoire_mv += static_cast<size_t>(mv.globales.taille()) * sizeof(Globale);
	memoire_mv += static_cast<size_t>(mv.donnees_constantes.taille());
	memoire_mv += static_cast<size_t>(mv.donnees_globales.taille());
	memoire_mv += static_cast<size_t>(mv.patchs_donnees_constantes.taille()) * sizeof(PatchDonneesConstantes);
	memoire_mv += static_cast<size_t>(mv.bibliotheques.taille()) * sizeof(BibliothequePartagee);

	metriques.memoire_mv = memoire_mv;

	return metriques;
}

AtomeFonction *Compilatrice::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom)
{
	auto table = table_fonctions.verrou_ecriture();
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom);
	table->insere({ nom, atome_fonc });
	return atome_fonc;
}

AtomeFonction *Compilatrice::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom, kuri::tableau<Atome *> &&params)
{
	auto table = table_fonctions.verrou_ecriture();
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom, std::move(params));
	table->insere({ nom, atome_fonc });
	return atome_fonc;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *Compilatrice::trouve_ou_insere_fonction(ConstructriceRI &constructrice, const NoeudDeclarationFonction *decl)
{
	auto table = table_fonctions.verrou_ecriture();
	auto iter_fonc = table->trouve(decl->nom_broye);

	if (iter_fonc != table->fin()) {
		return iter_fonc->second;
	}

	auto fonction_courante = constructrice.fonction_courante;
	constructrice.fonction_courante = nullptr;

	auto params = kuri::tableau<Atome *>();
	params.reserve(decl->params.taille);

	if (!decl->est_externe && !dls::outils::possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
		auto atome = constructrice.cree_allocation(typeuse.type_contexte, ID::contexte);
		params.pousse(atome);
	}

	POUR (decl->params) {
		auto atome = constructrice.cree_allocation(it->type, it->ident);
		params.pousse(atome);
	}

	// À FAIRE : retours multiples

	auto atome_fonc = fonctions.ajoute_element(decl->lexeme, decl->nom_broye, std::move(params));
	atome_fonc->type = normalise_type(typeuse, decl->type);
	atome_fonc->est_externe = decl->est_externe;

	if (dls::outils::possede_drapeau(decl->drapeaux, FORCE_SANSTRACE)) {
		atome_fonc->sanstrace = true;
	}

	atome_fonc->decl = decl;

	table->insere({ decl->nom_broye, atome_fonc });

	constructrice.fonction_courante = fonction_courante;

	return atome_fonc;
}

AtomeFonction *Compilatrice::trouve_fonction(const dls::chaine &nom)
{
	auto table = table_fonctions.verrou_lecture();

	auto iter = table->trouve(nom);

	if (iter != table->fin()) {
		return iter->second;
	}

	return nullptr;
}

AtomeGlobale *Compilatrice::cree_globale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante)
{
	return globales.ajoute_element(typeuse.type_pointeur_pour(type), initialisateur, est_externe, est_constante);
}

void Compilatrice::ajoute_globale(NoeudDeclaration *decl, AtomeGlobale *atome)
{
	auto table = table_globales.verrou_ecriture();
	table->insere({ decl, atome });
}

AtomeGlobale *Compilatrice::trouve_globale(NoeudDeclaration *decl)
{
	auto table = table_globales.verrou_lecture();
	auto iter = table->trouve(decl);

	if (iter != table->fin()) {
		return iter->second;
	}

	return nullptr;
}

AtomeGlobale *Compilatrice::trouve_ou_insere_globale(NoeudDeclaration *decl)
{
	auto table = table_globales.verrou_ecriture();
	auto iter = table->trouve(decl);

	if (iter != table->fin()) {
		return iter->second;
	}

	auto atome = cree_globale(decl->type, nullptr, false, false);
	table->insere({ decl, atome });

	return atome;
}

/* ************************************************************************** */

void Compilatrice::ajoute_message_fichier_ouvert(const kuri::chaine &chemin)
{
	messagere->ajoute_message_fichier_ouvert(chemin);
}

void Compilatrice::ajoute_message_fichier_ferme(const kuri::chaine &chemin)
{
	messagere->ajoute_message_fichier_ferme(chemin);
}

void Compilatrice::ajoute_message_module_ouvert(const kuri::chaine &chemin)
{
	messagere->ajoute_message_module_ouvert(chemin);
}

void Compilatrice::ajoute_message_module_ferme(const kuri::chaine &chemin)
{
	messagere->ajoute_message_module_ferme(chemin);
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

/* ************************************************************************** */

static OptionsCompilation *options_compilation = nullptr;

void initialise_options_compilation(OptionsCompilation &option)
{
	options_compilation = &option;
}

OptionsCompilation *obtiens_options_compilation()
{
	return options_compilation;
}

void ajourne_options_compilation(OptionsCompilation *options)
{
	*options_compilation = *options;

	if (options_compilation->nom_sortie != kuri::chaine("a.out")) {
		// duplique la mémoire
		options_compilation->nom_sortie = copie_chaine(options_compilation->nom_sortie);
	}
}

void compilatrice_ajoute_chaine_compilation(kuri::chaine c)
{
	auto chaine = dls::chaine(c.pointeur, c.taille);

	auto module = ptr_compilatrice->cree_module("", "");
	auto fichier = ptr_compilatrice->cree_fichier("métaprogramme", "");
	fichier->tampon = lng::tampon_source(chaine);
	fichier->module = module;
	module->fichiers.pousse(fichier);

	auto unite = UniteCompilation();
	unite.fichier = fichier;
	unite.etat = UniteCompilation::Etat::PARSAGE_ATTENDU;

	ptr_compilatrice->file_compilation->pousse(unite);
}

void compilatrice_ajoute_fichier_compilation(kuri::chaine c)
{
	auto vue = dls::chaine(c.pointeur, c.taille);
	auto chemin = std::filesystem::current_path() / vue.c_str();

	if (!std::filesystem::exists(chemin)) {
		std::cerr << "Le fichier " << chemin << " n'existe pas !\n";
		ptr_compilatrice->possede_erreur = true;
		return;
	}

	auto module = ptr_compilatrice->cree_module("", "");
	auto tampon = charge_fichier(chemin.c_str(), *ptr_compilatrice, {});
	auto fichier = ptr_compilatrice->cree_fichier(vue, chemin.c_str());
	ptr_compilatrice->ajoute_message_fichier_ouvert(fichier->chemin);

	fichier->tampon = lng::tampon_source(tampon);
	fichier->module = module;
	module->fichiers.pousse(fichier);

	auto unite = UniteCompilation();
	unite.fichier = fichier;
	unite.etat = UniteCompilation::Etat::PARSAGE_ATTENDU;

	ptr_compilatrice->ajoute_message_fichier_ferme(fichier->chemin);
	ptr_compilatrice->file_compilation->pousse(unite);
}

// fonction pour tester les appels de fonctions variadiques externe dans la machine virtuelle
int fonction_test_variadique_externe(int sentinel, ...)
{
	va_list ap;
	va_start(ap, sentinel);

	int i = 0;
	for (;; ++i) {
		int t = va_arg(ap, int);

		if (t == sentinel) {
			break;
		}
	}

	va_end(ap);

	return i;
}

Message const *compilatrice_attend_message()
{
	auto messagere = ptr_compilatrice->messagere.verrou_ecriture();

	if (!messagere->possede_message()) {
		return nullptr;
	}

	return messagere->defile();
}
