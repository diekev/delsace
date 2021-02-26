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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "espace_de_travail.hh"

#include <fstream>

#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"
#include "representation_intermediaire/impression.hh"

#include "arbre_syntaxique.hh"
#include "coulisse.hh"
#include "coulisse_asm.hh"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"
#include "identifiant.hh"
#include "statistiques.hh"

EspaceDeTravail::EspaceDeTravail(OptionsCompilation opts)
	: options(opts)
	, typeuse(graphe_dependance, this->operateurs)
{
	auto ops = operateurs.verrou_ecriture();
	enregistre_operateurs_basiques(*this, *ops);

	if (options.type_coulisse == TypeCoulisse::C) {
		coulisse = memoire::loge<CoulisseC>("CoulisseC");
	}
	else if (options.type_coulisse == TypeCoulisse::LLVM) {
		coulisse = memoire::loge<CoulisseLLVM>("CoulisseLLVM");
	}
	else if (options.type_coulisse == TypeCoulisse::ASM) {
		coulisse = memoire::loge<CoulisseASM>("CoulisseASM");
	}
	else {
		assert(false);
	}
}

EspaceDeTravail::~EspaceDeTravail()
{
	if (options.type_coulisse == TypeCoulisse::C) {
		auto c = dynamic_cast<CoulisseC *>(coulisse);
		memoire::deloge("CoulisseC", c);
		coulisse = nullptr;
	}
	else if (options.type_coulisse == TypeCoulisse::LLVM) {
		auto c = dynamic_cast<CoulisseLLVM *>(coulisse);
		memoire::deloge("CoulisseLLVM", c);
		coulisse = nullptr;
	}
	else if (options.type_coulisse == TypeCoulisse::ASM) {
		auto c = dynamic_cast<CoulisseASM *>(coulisse);
		memoire::deloge("CoulisseASM", c);
		coulisse = nullptr;
	}
}

Module *EspaceDeTravail::trouve_ou_cree_module(dls::outils::Synchrone<SystemeModule> &sys_module, IdentifiantCode *nom_module, kuri::chaine_statique chemin)
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

ResultatFichier EspaceDeTravail::trouve_ou_cree_fichier(dls::outils::Synchrone<SystemeModule> &sys_module, Module *module, kuri::chaine_statique nom_fichier, kuri::chaine_statique chemin, bool importe_kuri)
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
		if (dls::vue_chaine_compacte(it.chemin()) == chemin) {
			return const_cast<Fichier *>(&it);
		}
	}

	return nullptr;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const kuri::chaine &nom_fichier)
{
	std::unique_lock lock(mutex_atomes_fonctions);
	auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fichier);
	return atome_fonc;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme, const kuri::chaine &nom_fonction, kuri::tableau<Atome *, int> &&params)
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

	auto params = kuri::tableau<Atome *, int>();
	params.reserve(decl->params.taille());

	if (!decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX)) {
		auto atome = constructrice.cree_allocation(decl, typeuse.type_contexte, ID::contexte);
		params.ajoute(atome);
	}

	for (auto i = 0; i < decl->params.taille(); ++i) {
		auto param = decl->parametre_entree(i);
		auto atome = constructrice.cree_allocation(decl, param->type, param->ident);
		param->atome = atome;
		params.ajoute(atome);
	}

	/* Pour les sorties multiples, les valeurs de sorties sont des accès de
	 * membres du tuple, ainsi nous n'avons pas à compliquer la génération de
	 * code ou sa simplification.
	 */

	auto param_sortie = decl->param_sortie;
	auto atome_param_sortie = constructrice.cree_allocation(decl, param_sortie->type, param_sortie->ident);
	param_sortie->atome = atome_param_sortie;

	if (decl->params_sorties.taille() > 1) {
		auto index_membre = 0;
		POUR (decl->params_sorties) {
			it->atome = constructrice.cree_acces_membre(it, atome_param_sortie, index_membre++, true);
		}
	}

	auto atome_fonc = fonctions.ajoute_element(decl->lexeme, decl->nom_broye(constructrice.espace()), std::move(params));
	atome_fonc->type = normalise_type(typeuse, decl->type);
	atome_fonc->est_externe = decl->est_externe;
	atome_fonc->sanstrace = decl->possede_drapeau(FORCE_SANSTRACE);
	atome_fonc->decl = decl;
	atome_fonc->param_sortie = atome_param_sortie;
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

	auto nom_fonction = enchaine("initialise_", type);

	SAUVEGARDE_ETAT(constructrice.fonction_courante);

	auto types_entrees = dls::tablet<Type *, 6>(1);
	types_entrees[0] = typeuse.type_pointeur_pour(normalise_type(typeuse, type), false);

	auto type_sortie = typeuse[TypeBase::RIEN];

	auto params = kuri::tableau<Atome *, int>(1);
	params[0] = constructrice.cree_allocation(nullptr, types_entrees[0], ID::pointeur);

	auto param_sortie = constructrice.cree_allocation(nullptr, typeuse[TypeBase::RIEN], nullptr);

	auto atome_fonc = fonctions.ajoute_element(nullptr, nom_fonction, std::move(params));
	atome_fonc->type = typeuse.type_fonction(types_entrees, type_sortie, false);
	atome_fonc->param_sortie = param_sortie;
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
		memoire += it.params_entrees.taille_memoire();
		memoire += it.chunk.capacite;
		memoire += it.chunk.locales.taille_memoire();
		memoire += it.chunk.decalages_labels.taille_memoire();
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
		entree.nom = it.nom();
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

void EspaceDeTravail::tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> &/*messagere*/)
{
	nombre_taches_optimisation += 1;
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

	if (optimisations) {
		tache_optimisation_ajoutee(messagere);
	}

	if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 && phase == PhaseCompilation::TYPAGE_TERMINE) {
		change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
	}
}

void EspaceDeTravail::tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
	nombre_taches_optimisation -= 1;

	if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 && phase == PhaseCompilation::TYPAGE_TERMINE) {
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
