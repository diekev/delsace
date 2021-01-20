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
#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "erreur.h"
#include "lexeuse.hh"
#include "modules.hh"
#include "statistiques.hh"

#include "representation_intermediaire/impression.hh"

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(OptionsCompilation opts)
	: options(opts)
	, typeuse(graphe_dependance, this->operateurs)
{
	auto ops = operateurs.verrou_ecriture();
	enregistre_operateurs_basiques(*this, *ops);
}

EspaceDeTravail::~EspaceDeTravail()
{
}

Module *EspaceDeTravail::trouve_ou_cree_module(dls::outils::Synchrone<SystemeModule> &sys_module, IdentifiantCode *nom_module, dls::vue_chaine chemin)
{
	auto donnees_module = sys_module->trouve_ou_cree_module(nom_module, chemin);

	auto modules_ = modules.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*modules_)) {
		if (it.donnees_constantes == donnees_module) {
			return &it;
		}
	}

	return modules_->ajoute_element(donnees_module);
}

Module *EspaceDeTravail::module(const IdentifiantCode *nom_module) const
{
	auto modules_ = modules.verrou_lecture();
	POUR_TABLEAU_PAGE ((*modules_)) {
		if (it.nom() == nom_module) {
			return const_cast<Module *>(&it);
		}
	}

	return nullptr;
}

ResultatFichier EspaceDeTravail::trouve_ou_cree_fichier(dls::outils::Synchrone<SystemeModule> &sys_module, Module *module, dls::vue_chaine nom_fichier, dls::vue_chaine chemin, bool importe_kuri)
{
	auto donnees_fichier = sys_module->trouve_ou_cree_fichier(nom_fichier, chemin);

	auto fichiers_ = fichiers.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*fichiers_)) {
		if (it.donnees_constantes == donnees_fichier) {
			return FichierExistant(it);
		}
	}

	auto fichier = fichiers_->ajoute_element(donnees_fichier);

	if (importe_kuri) {
		fichier->modules_importes.insere(ID::Kuri);
	}

	fichier->module = module;
	module->fichiers.ajoute(fichier);

	return FichierNeuf(*fichier);
}

Fichier *EspaceDeTravail::fichier(long index) const
{
	auto fichiers_ = fichiers.verrou_lecture();

	POUR_TABLEAU_PAGE ((*fichiers_)) {
		if (it.id() == index) {
			return const_cast<Fichier *>(&it);
		}
	}

	return nullptr;
}

Fichier *EspaceDeTravail::fichier(const dls::vue_chaine_compacte &chemin) const
{
	auto fichiers_ = fichiers.verrou_lecture();

	POUR_TABLEAU_PAGE ((*fichiers_)) {
		if (it.chemin() == chemin) {
			return const_cast<Fichier *>(&it);
		}
	}

	return nullptr;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom_fichier)
{
	std::unique_lock lock(mutex_atomes_fonctions);
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fichier);
	return atome_fonc;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const dls::chaine &nom_fonction, kuri::tableau<Atome *> &&params)
{
	std::unique_lock lock(mutex_atomes_fonctions);
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fonction, std::move(params));
	return atome_fonc;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction(ConstructriceRI &constructrice, NoeudDeclarationEnteteFonction *decl)
{
	std::unique_lock lock(mutex_atomes_fonctions);

	if (decl->atome) {
		return static_cast<AtomeFonction *>(decl->atome);
	}

	SAUVEGARDE_ETAT(constructrice.fonction_courante);

	auto params = kuri::tableau<Atome *>();
	params.reserve(decl->params.taille);

	if (!decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX)) {
		auto atome = constructrice.cree_allocation(decl, typeuse.type_contexte, ID::contexte);
		params.ajoute(atome);
	}

	for (auto i = 0; i < decl->params.taille; ++i) {
		auto param = decl->parametre_entree(i);
		auto atome = constructrice.cree_allocation(decl, param->type, param->ident);
		param->atome = atome;
		params.ajoute(atome);
	}

	auto params_sortie = kuri::tableau<Atome *>();
	if (decl->params_sorties.taille == 1) {
		auto param_sortie = decl->params_sorties[0];
		auto atome = constructrice.cree_allocation(decl, param_sortie->type, param_sortie->ident);
		param_sortie->atome = atome;
		params_sortie.ajoute(atome);
	}
	else {
		POUR (decl->params_sorties) {
			auto atome = constructrice.cree_allocation(decl, typeuse.type_pointeur_pour(it->type, false), it->ident);
			it->atome = atome;
			params.ajoute(atome);
		}
	}

	auto atome_fonc = fonctions.ajoute_element(decl->lexeme, decl->nom_broye(constructrice.espace()), std::move(params));
	atome_fonc->type = normalise_type(typeuse, decl->type);
	atome_fonc->est_externe = decl->est_externe;
	atome_fonc->sanstrace = decl->possede_drapeau(FORCE_SANSTRACE);
	atome_fonc->decl = decl;
	atome_fonc->params_sorties = std::move(params_sortie);
	atome_fonc->enligne = decl->possede_drapeau(FORCE_ENLIGNE);

	decl->atome = atome_fonc;

	return atome_fonc;
}

AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction_init(ConstructriceRI &constructrice, Type *type)
{
	std::unique_lock lock(mutex_atomes_fonctions);

	if (type->fonction_init) {
		return type->fonction_init;
	}

	auto nom_fonction = "initialise_" + dls::vers_chaine(type);

	SAUVEGARDE_ETAT(constructrice.fonction_courante);

	auto types_entrees = dls::tablet<Type *, 6>(1);
	types_entrees[0] = typeuse.type_pointeur_pour(normalise_type(typeuse, type), false);

	auto types_sorties = dls::tablet<Type *, 6>(1);
	types_sorties[0] = typeuse[TypeBase::RIEN];

	auto params = kuri::tableau<Atome *>(1);
	params[0] = constructrice.cree_allocation(nullptr, types_entrees[0], ID::pointeur);

	auto params_sortie = kuri::tableau<Atome *>();
	auto atome = constructrice.cree_allocation(nullptr, typeuse[TypeBase::RIEN], nullptr);
	params_sortie.ajoute(atome);

	auto atome_fonc = fonctions.ajoute_element(nullptr, nom_fonction, std::move(params));
	atome_fonc->type = typeuse.type_fonction(types_entrees, types_sorties, false);
	atome_fonc->params_sorties = std::move(params_sortie);
	atome_fonc->enligne = true;
	atome_fonc->sanstrace = true;

	type->fonction_init = atome_fonc;

	return atome_fonc;
}

AtomeGlobale *EspaceDeTravail::cree_globale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante)
{
	return globales.ajoute_element(typeuse.type_pointeur_pour(type, false), initialisateur, est_externe, est_constante);
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

	memoire += modules->memoire_utilisee();
	memoire += fichiers->memoire_utilisee();

	auto modules_ = modules.verrou_lecture();
	POUR_TABLEAU_PAGE ((*modules_)) {
		memoire += it.fichiers.taille() * taille_de(Fichier *);
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
	operateurs->rassemble_statistiques(stats);
	graphe_dependance->rassemble_statistiques(stats);
	typeuse.rassemble_statistiques(stats);

	auto &stats_fichiers = stats.stats_fichiers;
	auto fichiers_ = fichiers.verrou_lecture();
	POUR_TABLEAU_PAGE ((*fichiers_)) {
		auto entree = EntreeFichier();
		entree.nom = it.nom().c_str();
		entree.temps_parsage = it.temps_analyse;

		stats_fichiers.fusionne_entree(entree);
	}
}

MetaProgramme *EspaceDeTravail::cree_metaprogramme()
{
	return metaprogrammes->ajoute_element();
}

void EspaceDeTravail::tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
	change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
	nombre_taches_chargement += 1;
}

void EspaceDeTravail::tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
	change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
	nombre_taches_lexage += 1;
}

void EspaceDeTravail::tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
	change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
	nombre_taches_parsage += 1;
}

void EspaceDeTravail::tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
	if (phase > PhaseCompilation::PARSAGE_TERMINE) {
		change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
	}

	nombre_taches_typage += 1;
}

void EspaceDeTravail::tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
	if (phase > PhaseCompilation::TYPAGE_TERMINE) {
		change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);
	}

	nombre_taches_ri += 1;
}

void EspaceDeTravail::tache_execution_ajoutee(dls::outils::Synchrone<Messagere> &/*messagere*/)
{
	nombre_taches_execution += 1;
}

void EspaceDeTravail::tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere, Fichier *fichier)
{
	messagere->ajoute_message_fichier_ferme(this, fichier->chemin());

	/* Une fois que nous avons fini de charger un fichier, il faut le lexer. */
	tache_lexage_ajoutee(messagere);

	nombre_taches_chargement -= 1;
}

void EspaceDeTravail::tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	/* Une fois que nous lexer quelque chose, il faut le parser. */
	tache_parsage_ajoutee(messagere);
	nombre_taches_lexage -= 1;
}

void EspaceDeTravail::tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	nombre_taches_parsage -= 1;

	if (parsage_termine()) {
		change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
	}
}

void EspaceDeTravail::tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	nombre_taches_typage -= 1;

	if (nombre_taches_typage == 0 && phase == PhaseCompilation::PARSAGE_TERMINE) {
		change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);
	}
}

void EspaceDeTravail::tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	nombre_taches_ri -= 1;

	if (nombre_taches_ri == 0 && phase == PhaseCompilation::TYPAGE_TERMINE) {
		change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
	}
}

void EspaceDeTravail::tache_execution_terminee(dls::outils::Synchrone<Messagere> &/*messagere*/)
{
	nombre_taches_execution -= 1;
}

void EspaceDeTravail::tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	change_de_phase(messagere, PhaseCompilation::APRES_GENERATION_OBJET);
}

void EspaceDeTravail::tache_liaison_executable_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	change_de_phase(messagere, PhaseCompilation::APRES_LIAISON_EXECUTABLE);
}

bool EspaceDeTravail::peut_generer_code_final() const
{
	if (phase != PhaseCompilation::GENERATION_CODE_TERMINEE) {
		return false;
	}

	if (nombre_taches_execution == 0) {
		return true;
	}

	if (nombre_taches_execution == 1 && metaprogramme) {
		return true;
	}

	return false;
}

bool EspaceDeTravail::parsage_termine() const
{
	return nombre_taches_chargement == 0 && nombre_taches_lexage == 0 && nombre_taches_parsage == 0;
}

void EspaceDeTravail::change_de_phase(dls::outils::Synchrone<Messagere> &messagere, PhaseCompilation nouvelle_phase)
{
	phase = nouvelle_phase;
	messagere->ajoute_message_phase_compilation(this);
}

PhaseCompilation EspaceDeTravail::phase_courante() const
{
	return phase;
}

void EspaceDeTravail::imprime_programme() const
{
	std::ofstream os;
	os.open("/tmp/ri_programme.kr");

	POUR_TABLEAU_PAGE(fonctions) {
		imprime_fonction(&it, os);
	}
}

/* ************************************************************************** */

static Compilatrice *ptr_compilatrice = nullptr;

Compilatrice::Compilatrice()
	: ordonnanceuse(this)
{
	this->bibliotheques_dynamiques->ajoute("pthread");
	this->definitions->ajoute("_REENTRANT");

	ptr_compilatrice = this;
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace, const dls::chaine &nom, NoeudExpression const *site)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						*espace,
						site,
						erreur::Genre::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
	auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
	auto nom_dossier = chemin_absolu.filename();

	// @concurrence critique
	auto module = espace->trouve_ou_cree_module(sys_module, ptr_compilatrice->table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier.c_str()), chemin_absolu.c_str());

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

		auto resultat = espace->trouve_ou_cree_fichier(sys_module, module, chemin_entree.stem().c_str(), chemin_entree.c_str(), importe_kuri);

		if (resultat.tag_type() == FichierNeuf::tag) {
			ordonnanceuse->cree_tache_pour_chargement(espace, resultat.t2().fichier);
		}
	}

	if (module->nom() == ID::Kuri) {
		auto resultat = espace->trouve_ou_cree_fichier(sys_module, module, "constantes", "constantes.kuri", false);

		if (resultat.tag_type() == FichierNeuf::tag) {
			auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
			if (!donnees_fichier->fut_charge) {
				const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
				donnees_fichier->charge_tampon(lng::tampon_source(source));
			}

			ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
		}
	}

	messagere->ajoute_message_module_ferme(espace, module);

	return module;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		EspaceDeTravail &espace,
		NoeudExpression const *site)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	return dls::chaine(std::istreambuf_iterator<char>(fichier), std::istreambuf_iterator<char>());
}

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace, const dls::chaine &nom, Module *module, NoeudExpression const *site)
{
	auto chemin = module->chemin() + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du fichier */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, nom.c_str(), chemin_absolu.c_str(), importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		ordonnanceuse->cree_tache_pour_chargement(espace, resultat.t2().fichier);
	}
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

	memoire += gerante_chaine->memoire_utilisee();

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		memoire += it.memoire_utilisee();
	}

	memoire += messagere->memoire_utilisee();

	memoire += sys_module->memoire_utilisee();

	return memoire;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
	stats.memoire_compilatrice = memoire_utilisee();

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		it.rassemble_statistiques(stats);
	}

	stats.nombre_identifiants = table_identifiants->taille();

	sys_module->rassemble_stats(stats);
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

long GeranteChaine::ajoute_chaine(const dls::chaine &chaine)
{
	if ((enchaineuse.tampon_courant->occupe + chaine.taille()) >= Enchaineuse::TAILLE_TAMPON) {
		enchaineuse.ajoute_tampon();
	}

	// calcul l'adresse de la chaine
	auto adresse = (enchaineuse.nombre_tampons() - 1) * Enchaineuse::TAILLE_TAMPON + enchaineuse.tampon_courant->occupe;

	adresse_et_taille.ajoute(adresse);
	adresse_et_taille.ajoute(static_cast<int>(chaine.taille()));

	enchaineuse.ajoute(chaine);

	return adresse;
}

kuri::chaine GeranteChaine::chaine_pour_adresse(long adresse) const
{
	assert(adresse >= 0);

	auto taille = 0;

	for (auto i = 0; i < adresse_et_taille.taille(); i += 2) {
		if (adresse_et_taille[i] == adresse) {
			taille = adresse_et_taille[i + 1];
			break;
		}
	}

	auto tampon_courant = &enchaineuse.m_tampon_base;

	while (adresse >= Enchaineuse::TAILLE_TAMPON) {
		adresse -= Enchaineuse::TAILLE_TAMPON;
		tampon_courant = tampon_courant->suivant;
	}

	assert(tampon_courant);

	auto resultat = kuri::chaine();
	resultat.taille = taille;
	resultat.pointeur = const_cast<char *>(&tampon_courant->donnees[adresse]);
	return resultat;
}

long GeranteChaine::memoire_utilisee() const
{
	return enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON + adresse_et_taille.taille() * taille_de(int);
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

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->ajoute(chaine);

	auto module = espace->trouve_ou_cree_module(ptr_compilatrice->sys_module, ID::chaine_vide, "");
	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, "métaprogramme", "", ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
		if (!donnees_fichier->fut_charge) {
			donnees_fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
		}
		ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
	}
}

void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine c)
{
	auto chaine = dls::chaine(c.pointeur, c.taille);

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->ajoute(chaine);

	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, "métaprogramme", "", ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
		if (!donnees_fichier->fut_charge) {
			donnees_fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
		}
		ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
	}
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

	auto module = espace->trouve_ou_cree_module(ptr_compilatrice->sys_module, ID::chaine_vide, "");
	ptr_compilatrice->ajoute_fichier_a_la_compilation(espace, chemin.stem().c_str(), module, {});
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

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y retourner le message disponible */
Message const *compilatrice_attend_message()
{
	assert(false);
	return nullptr;
}

EspaceDeTravail *demarre_un_espace_de_travail(kuri::chaine nom, OptionsCompilation *options)
{
	return ptr_compilatrice->demarre_un_espace_de_travail(*options, dls::chaine(nom.pointeur, nom.taille));
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y renseigner dans l'espace le métaprogramme en cours
 * d'exécution */
void compilatrice_commence_interception(EspaceDeTravail * /*espace*/)
{
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y vérifier que le métaprogramme terminant l'interception
 * est bel et bien celui l'ayant commencé */
void compilatrice_termine_interception(EspaceDeTravail * /*espace*/)
{
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
		resultat.ajoute({ static_cast<int>(it.genre), it.chaine });
	}

	return resultat;
}

kuri::tableau<kuri::Lexeme> compilatrice_lexe_fichier(kuri::chaine chemin_donne)
{
	auto espace = ptr_compilatrice->espace_de_travail_defaut;
	auto chemin = dls::chaine(chemin_donne.pointeur, chemin_donne.taille);

	if (!std::filesystem::exists(chemin.c_str())) {
		// À FAIRE(erreur) : site
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au chemin",
					*espace,
					nullptr,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		// À FAIRE(erreur) : site
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					nullptr,
					erreur::Genre::MODULE_INCONNU);
	}

	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto module = espace->module(ID::chaine_vide);

	auto resultat = espace->trouve_ou_cree_fichier(
				ptr_compilatrice->sys_module,
				module,
				chemin_absolu.stem().c_str(),
				chemin_absolu.c_str(),
				ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierExistant::tag) {
		auto donnees_fichier = resultat.t1().fichier->donnees_constantes;
		return converti_tableau_lexemes(donnees_fichier->lexemes);
	}

	auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
	// À FAIRE(erreur) : site
	auto tampon = charge_fichier(chemin.c_str(), *espace, nullptr);
	donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

	auto lexeuse = Lexeuse(*ptr_compilatrice, donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
	lexeuse.performe_lexage();

	return converti_tableau_lexemes(donnees_fichier->lexemes);
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les Machines Virtuelles, et y retourner l'espace du métaprogramme */
EspaceDeTravail *compilatrice_espace_courant()
{
	assert(false);
	return nullptr;
}
