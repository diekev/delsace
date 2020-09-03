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
#include "lexeuse.hh"
#include "modules.hh"
#include "statistiques.hh"

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(OptionsCompilation opts)
	: options(opts)
	, assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", *this))
	, typeuse(graphe_dependance, this->operateurs)
{
	auto ops = operateurs.verrou_ecriture();
	enregistre_operateurs_basiques(*this, *ops);
}

EspaceDeTravail::~EspaceDeTravail()
{
	memoire::deloge("AssembleuseArbre", assembleuse);
}

Module *EspaceDeTravail::cree_module(dls::chaine const &nom_module, dls::chaine const &chemin)
{
	auto chemin_corrige = chemin;

	if (chemin_corrige.taille() > 0 && chemin_corrige[chemin_corrige.taille() - 1] != '/') {
		chemin_corrige.append('/');
	}

	auto modules_ = modules.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*modules_)) {
		if (it.chemin == chemin_corrige) {
			return &it;
		}
	}

	auto module = modules_->ajoute_element(*this);
	module->id = static_cast<size_t>(modules_->taille() - 1);
	module->nom = nom_module;
	module->chemin = chemin_corrige;

	return module;
}

Module *EspaceDeTravail::module(const dls::vue_chaine_compacte &nom_module) const
{
	auto modules_ = modules.verrou_lecture();
	POUR_TABLEAU_PAGE ((*modules_)) {
		if (it.nom == nom_module) {
			return const_cast<Module *>(&it);
		}
	}

	return nullptr;
}

Fichier *EspaceDeTravail::cree_fichier(dls::chaine const &nom_fichier, dls::chaine const &chemin, bool importe_kuri)
{
	auto fichiers_ = fichiers.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*fichiers_)) {
		if (it.chemin == chemin) {
			return nullptr;
		}
	}

	auto fichier = fichiers_->ajoute_element();
	fichier->id = static_cast<size_t>(fichiers_->taille() - 1);
	fichier->nom = nom_fichier;
	fichier->chemin = chemin;

	if (importe_kuri) {
		fichier->modules_importes.insere("Kuri");
	}

	return fichier;
}

Fichier *EspaceDeTravail::fichier(long index) const
{
	return const_cast<Fichier *>(&fichiers->a_l_index(index));
}

Fichier *EspaceDeTravail::fichier(const dls::vue_chaine_compacte &nom_fichier) const
{
	auto fichiers_ = fichiers.verrou_lecture();

	POUR_TABLEAU_PAGE ((*fichiers_)) {
		if (it.nom == nom_fichier) {
			return const_cast<Fichier *>(&it);
		}
	}

	return nullptr;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom_fichier)
{
	auto table = table_fonctions.verrou_ecriture();
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fichier);
	table->insere({ nom_fichier, atome_fonc });
	return atome_fonc;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom_fonction, kuri::tableau<Atome *> &&params)
{
	auto table = table_fonctions.verrou_ecriture();
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fonction, std::move(params));
	table->insere({ nom_fonction, atome_fonc });
	return atome_fonc;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction(ConstructriceRI &constructrice, NoeudDeclarationEnteteFonction const *decl)
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

	if (!decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX)) {
		auto atome = constructrice.cree_allocation(typeuse.type_contexte, ID::contexte);
		params.pousse(atome);
	}

	for (auto i = 0; i < decl->params.taille; ++i) {
		auto param = decl->parametre_entree(i);
		auto atome = constructrice.cree_allocation(param->type, param->ident);
		params.pousse(atome);
	}

	// À FAIRE : retours multiples

	auto atome_fonc = fonctions.ajoute_element(decl->lexeme, decl->nom_broye, std::move(params));
	atome_fonc->type = normalise_type(typeuse, decl->type);
	atome_fonc->est_externe = decl->est_externe;
	atome_fonc->sanstrace = decl->possede_drapeau(FORCE_SANSTRACE);
	atome_fonc->decl = decl;

	table->insere({ decl->nom_broye, atome_fonc });

	constructrice.fonction_courante = fonction_courante;

	return atome_fonc;
}

AtomeFonction *EspaceDeTravail::trouve_fonction(const dls::chaine &nom_fonction)
{
	auto table = table_fonctions.verrou_lecture();

	auto iter = table->trouve(nom_fonction);

	if (iter != table->fin()) {
		return iter->second;
	}

	return nullptr;
}

AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction_init(ConstructriceRI &constructrice, Type *type)
{
	auto table = table_fonctions.verrou_ecriture();

	auto nom_fonction = "initialise_" + dls::vers_chaine(type);

	auto iter_fonc = table->trouve(nom_fonction);

	if (iter_fonc != table->fin()) {
		return iter_fonc->second;
	}

	auto fonction_courante = constructrice.fonction_courante;
	constructrice.fonction_courante = nullptr;

	auto types_entrees = kuri::tableau<Type *>(2);
	types_entrees[0] = typeuse.type_contexte;
	types_entrees[1] = typeuse.type_pointeur_pour(normalise_type(typeuse, type));

	auto types_sorties = kuri::tableau<Type *>(1);
	types_sorties[0] = typeuse[TypeBase::RIEN];

	auto params = kuri::tableau<Atome *>(2);
	params[0] = constructrice.cree_allocation(types_entrees[0], ID::contexte);
	params[1] = constructrice.cree_allocation(types_entrees[1], ID::pointeur);

	auto atome_fonc = fonctions.ajoute_element(nullptr, nom_fonction, std::move(params));
	atome_fonc->type = typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));

	table->insere({ nom_fonction, atome_fonc });

	constructrice.fonction_courante = fonction_courante;

	return atome_fonc;
}

AtomeGlobale *EspaceDeTravail::cree_globale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante)
{
	return globales.ajoute_element(typeuse.type_pointeur_pour(type), initialisateur, est_externe, est_constante);
}

void EspaceDeTravail::ajoute_globale(NoeudDeclaration *decl, AtomeGlobale *atome)
{
	auto table = table_globales.verrou_ecriture();
	table->insere({ decl, atome });
}

AtomeGlobale *EspaceDeTravail::trouve_globale(NoeudDeclaration *decl)
{
	auto table = table_globales.verrou_lecture();
	auto iter = table->trouve(decl);

	if (iter != table->fin()) {
		return iter->second;
	}

	return nullptr;
}

AtomeGlobale *EspaceDeTravail::trouve_ou_insere_globale(NoeudDeclaration *decl)
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

long EspaceDeTravail::memoire_utilisee() const
{
	auto memoire = 0l;

	memoire += modules->taille() * taille_de(Module *);
	memoire += fichiers->taille() * taille_de(Fichier *);

	auto modules_ = modules.verrou_lecture();
	POUR_TABLEAU_PAGE ((*modules_)) {
		memoire += it.fichiers.taille() * taille_de(Fichier *);
		memoire += it.nom.taille();
		memoire += it.chemin.taille();

		if (!it.fonctions_exportees.est_stocke_dans_classe()) {
			memoire += it.fonctions_exportees.taille() * taille_de(dls::vue_chaine_compacte);
		}
	}

	auto fichiers_ = fichiers.verrou_lecture();
	POUR_TABLEAU_PAGE ((*fichiers_)) {
		// les autres membres sont gérés dans rassemble_statistiques()
		if (!it.modules_importes.est_stocke_dans_classe()) {
			memoire += it.modules_importes.taille() * taille_de(dls::vue_chaine_compacte);
		}
	}

	memoire += fonctions.memoire_utilisee();
	memoire += globales.memoire_utilisee();

	pour_chaque_element(fonctions, [&](AtomeFonction const &it)
	{
		memoire += it.params_entrees.taille * taille_de(Atome *);
		memoire += it.params_sorties.taille * taille_de(Atome *);
		memoire += it.chunk.capacite;
		memoire += it.chunk.locales.taille() * taille_de(Locale);
		memoire += it.chunk.decalages_labels.taille() * taille_de(int);
	});

	return memoire;
}

void EspaceDeTravail::rassemble_statistiques(Statistiques &stats) const
{
	stats.nombre_modules += modules->taille();

	allocatrice_noeud.rassemble_statistiques(stats);
	operateurs->rassemble_statistiques(stats);
	graphe_dependance->rassemble_statistiques(stats);
	typeuse.rassemble_statistiques(stats);

	auto &stats_fichiers = stats.stats_fichiers;
	auto fichiers_ = fichiers.verrou_lecture();
	POUR_TABLEAU_PAGE ((*fichiers_)) {
		auto entree = EntreeFichier();
		entree.nom = it.nom.c_str();
		entree.nombre_lignes = it.tampon.nombre_lignes();
		entree.memoire_tampons = it.tampon.taille_donnees();
		entree.memoire_lexemes = it.lexemes.taille() * taille_de(Lexeme);
		entree.nombre_lexemes = it.lexemes.taille();
		entree.temps_parsage = it.temps_analyse;
		entree.temps_chargement = it.temps_chargement;
		entree.temps_tampon = it.temps_tampon;
		entree.temps_lexage = it.temps_decoupage;

		stats_fichiers.ajoute_entree(entree);
	}
}

/* ************************************************************************** */

static Compilatrice *ptr_compilatrice = nullptr;

Compilatrice::Compilatrice()
	: constructrice_ri(*this)
	, ordonnanceuse(this)
{
	this->bibliotheques_dynamiques->pousse("pthread");
	this->definitions->pousse("_REENTRANT");

	ptr_compilatrice = this;
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace, const dls::chaine &nom, const Lexeme &lexeme)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						*espace,
						&lexeme,
						erreur::Genre::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					*espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
	auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
	auto nom_dossier = chemin_absolu.filename();

	// @concurrence critique
	auto module = espace->cree_module(nom_dossier.c_str(), chemin_absolu.c_str());

	if (module->importe) {
		return module;
	}

	module->importe = true;

	messagere->ajoute_message_module_ouvert(espace, module);

	for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
		auto chemin_entree = entree.path();

		if (!std::filesystem::is_regular_file(chemin_entree)) {
			continue;
		}

		if (chemin_entree.extension() != ".kuri") {
			continue;
		}

		ajoute_fichier_a_la_compilation(espace, chemin_entree.stem().c_str(), module, {});
	}

	if (module->nom == "Kuri") {
		auto fichier = espace->cree_fichier("constantes", "constantes.kuri", false);
		const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
		fichier->tampon = lng::tampon_source(source);
		fichier->module = module;
		module->fichiers.pousse(fichier);

		ordonnanceuse->cree_tache_pour_lexage(espace, fichier);
	}

	messagere->ajoute_message_module_ferme(espace, module);

	return module;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		EspaceDeTravail &espace,
		Lexeme const &lexeme)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
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

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace, const dls::chaine &nom, Module *module, const Lexeme &lexeme)
{
	auto chemin = module->chemin + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					*espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du fichier */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	// @concurrence critique
	auto fichier = espace->cree_fichier(nom.c_str(), chemin_absolu.c_str(), importe_kuri);

	if (fichier == nullptr) {
		/* le fichier a déjà été chargé */
		return;
	}

	messagere->ajoute_message_fichier_ouvert(espace, fichier->chemin);

	fichier->module = module;

	auto debut_chargement = dls::chrono::compte_seconde();
	auto tampon = charge_fichier(chemin, *espace, lexeme);
	fichier->temps_chargement = debut_chargement.temps();

	auto debut_tampon = dls::chrono::compte_seconde();
	fichier->tampon = lng::tampon_source(tampon);
	fichier->temps_tampon = debut_tampon.temps();

	ordonnanceuse->cree_tache_pour_lexage(espace, fichier);

	messagere->ajoute_message_fichier_ferme(espace, fichier->chemin);
}

/* ************************************************************************** */

long Compilatrice::memoire_utilisee() const
{
	auto memoire = taille_de(Compilatrice);

	memoire += bibliotheques_dynamiques->taille() * taille_de(dls::chaine);
	POUR (*bibliotheques_dynamiques.verrou_lecture()) {
		memoire += it.taille();
	}

	memoire += bibliotheques_statiques->taille() * taille_de(dls::chaine);
	POUR (*bibliotheques_statiques.verrou_lecture()) {
		memoire += it.taille();
	}

	memoire += chemins->taille() * taille_de(dls::vue_chaine_compacte);
	memoire += definitions->taille() * taille_de(dls::vue_chaine_compacte);

	memoire += ordonnanceuse->memoire_utilisee();
	memoire += table_identifiants->memoire_utilisee();

	memoire += gerante_chaine->m_table.taille() * taille_de(dls::chaine);
	POUR (gerante_chaine->m_table) {
		memoire += it.capacite;
	}

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		memoire += it.memoire_utilisee();
	}

	memoire += messagere->memoire_utilisee();

	return memoire;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
	stats.memoire_compilatrice = memoire_utilisee();

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		it.rassemble_statistiques(stats);
	}

	stats.nombre_identifiants = table_identifiants->taille();
}

/* ************************************************************************** */

EspaceDeTravail *Compilatrice::demarre_un_espace_de_travail(OptionsCompilation const &options, const dls::chaine &nom)
{
	auto espace = espaces_de_travail->ajoute_element(options);
	espace->nom = nom;

	importe_module(espace, "Kuri", {});

	return espace;
}

/* ************************************************************************** */

GeranteChaine::~GeranteChaine()
{
	POUR (m_table) {
		it.chaine.taille = it.capacite;
		kuri::detruit_chaine(it.chaine);
	}
}

void GeranteChaine::ajoute_chaine(const kuri::chaine &chaine, long capacite)
{
	m_table.pousse({ chaine, capacite });
}

/* ************************************************************************** */

OptionsCompilation *obtiens_options_compilation()
{
	return &ptr_compilatrice->espace_de_travail_defaut->options;
}

void ajourne_options_compilation(OptionsCompilation *options)
{
	ptr_compilatrice->espace_de_travail_defaut->options = *options;

	if (options->nom_sortie != kuri::chaine("a.out")) {
		// duplique la mémoire
		options->nom_sortie = copie_chaine(options->nom_sortie);
	}
}

void compilatrice_ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine c)
{
	auto chaine = dls::chaine(c.pointeur, c.taille);

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->pousse(chaine);

	auto module = espace->cree_module("", "");
	auto fichier = espace->cree_fichier("métaprogramme", "", ptr_compilatrice->importe_kuri);
	fichier->tampon = lng::tampon_source(chaine);
	fichier->module = module;
	module->fichiers.pousse(fichier);

	ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, fichier);
}

void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine c)
{
	auto chaine = dls::chaine(c.pointeur, c.taille);

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->pousse(chaine);

	auto fichier = espace->cree_fichier("métaprogramme", "", ptr_compilatrice->importe_kuri);
	fichier->tampon = lng::tampon_source(chaine);
	fichier->module = module;
	module->fichiers.pousse(fichier);

	ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, fichier);
}

void compilatrice_ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine c)
{
	auto vue = dls::chaine(c.pointeur, c.taille);
	auto chemin = std::filesystem::current_path() / vue.c_str();

	if (!std::filesystem::exists(chemin)) {
		std::cerr << "Le fichier " << chemin << " n'existe pas !\n";
		ptr_compilatrice->possede_erreur = true;
		return;
	}

	auto module = espace->cree_module("", "");
	auto tampon = charge_fichier(chemin.c_str(), *espace, {});
	auto fichier = espace->cree_fichier(vue, chemin.c_str(), ptr_compilatrice->importe_kuri);
	ptr_compilatrice->messagere->ajoute_message_fichier_ouvert(espace, fichier->chemin);

	fichier->tampon = lng::tampon_source(tampon);
	fichier->module = module;
	module->fichiers.pousse(fichier);

	ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, fichier);

	ptr_compilatrice->messagere->ajoute_message_fichier_ferme(espace, fichier->chemin);
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

EspaceDeTravail *demarre_un_espace_de_travail(kuri::chaine nom, OptionsCompilation *options)
{
	return ptr_compilatrice->demarre_un_espace_de_travail(*options, dls::chaine(nom.pointeur, nom.taille));
}

void compilatrice_commence_interception(EspaceDeTravail *espace)
{
	ptr_compilatrice->messagere->commence_interception(espace);
}

void compilatrice_termine_interception(EspaceDeTravail *espace)
{
	ptr_compilatrice->messagere->termine_interception(espace);
}

EspaceDeTravail *espace_defaut_compilation()
{
	return ptr_compilatrice->espace_de_travail_defaut;
}

void compilatrice_rapporte_erreur(EspaceDeTravail *espace, kuri::chaine fichier, int ligne, kuri::chaine message)
{
	::rapporte_erreur(espace, fichier, ligne, message);
}

static kuri::tableau<kuri::Lexeme> converti_tableau_lexemes(dls::tableau<Lexeme> const &lexemes)
{
	auto resultat = kuri::tableau<kuri::Lexeme>();
	resultat.reserve(lexemes.taille());

	POUR (lexemes) {
		resultat.pousse({ static_cast<int>(it.genre), it.chaine });
	}

	return resultat;
}

kuri::tableau<kuri::Lexeme> compilatrice_lexe_fichier(kuri::chaine chemin_donne)
{
	static Lexeme lexeme = {};

	auto espace = ptr_compilatrice->espace_de_travail_defaut;
	auto chemin = dls::chaine(chemin_donne.pointeur, chemin_donne.taille);

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au chemin",
					*espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					&lexeme,
					erreur::Genre::MODULE_INCONNU);
	}

	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto fichier = espace->cree_fichier(chemin_absolu.stem().c_str(), chemin_absolu.c_str(), false);

	if (fichier == nullptr) {
		fichier = espace->fichier(chemin_absolu.stem().c_str());
		return converti_tableau_lexemes(fichier->lexemes);
	}

	auto tampon = charge_fichier(chemin.c_str(), *espace, {});
	fichier->tampon = lng::tampon_source(tampon);

	auto lexeuse = Lexeuse(*ptr_compilatrice, fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
	lexeuse.performe_lexage();

	return converti_tableau_lexemes(fichier->lexemes);
}
