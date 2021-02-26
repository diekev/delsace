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

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/dico.hh"

#include "structures/table_hachage.hh"

#include "graphe_dependance.hh"
#include "message.hh"
#include "metaprogramme.hh"
#include "modules.hh"
#include "options.hh"
#include "operateurs.hh"
#include "typage.hh"

struct Atome;
struct AtomeGlobale;
struct Coulisse;
struct ConstructriceRI;

// Interface avec le module « Kuri », pour certaines fonctions intéressantes
struct InterfaceKuri {
	NoeudDeclarationEnteteFonction *decl_panique = nullptr;
	NoeudDeclarationEnteteFonction *decl_panique_tableau = nullptr;
	NoeudDeclarationEnteteFonction *decl_panique_chaine = nullptr;
	NoeudDeclarationEnteteFonction *decl_panique_membre_union = nullptr;
	NoeudDeclarationEnteteFonction *decl_panique_memoire = nullptr;
	NoeudDeclarationEnteteFonction *decl_panique_erreur = nullptr;
	NoeudDeclarationEnteteFonction *decl_rappel_panique_defaut = nullptr;
	NoeudDeclarationEnteteFonction *decl_dls_vers_r32 = nullptr;
	NoeudDeclarationEnteteFonction *decl_dls_vers_r64 = nullptr;
	NoeudDeclarationEnteteFonction *decl_dls_depuis_r32 = nullptr;
	NoeudDeclarationEnteteFonction *decl_dls_depuis_r64 = nullptr;
	NoeudDeclarationEnteteFonction *decl_creation_contexte = nullptr;
};

/* IPA :
 * - crée_un_espace_de_travail
 * - espace_de_travail_défaut
 *
 * Problèmes :
 * - les modules ne sont ouvert qu'une seule fois
 * - il faudra stocker les modules pour chaque espace de travail, et partager les données constantes des fichiers
 * - séparer les données constantes des données dynamiques
 * -- données constantes : tampon du fichier, lexèmes
 * -- données dynamiques : arbres syntaxiques, types, noeuds dépendances
 */
struct EspaceDeTravail {
private:
	std::atomic<int> nombre_taches_chargement = 0;
	std::atomic<int> nombre_taches_lexage = 0;
	std::atomic<int> nombre_taches_parsage = 0;
	std::atomic<int> nombre_taches_typage = 0;
	std::atomic<int> nombre_taches_ri = 0;
	std::atomic<int> nombre_taches_execution = 0;
	std::atomic<int> nombre_taches_optimisation = 0;

	PhaseCompilation phase = PhaseCompilation::PARSAGE_EN_COURS;

public:
	kuri::chaine nom{};
	OptionsCompilation options{};

	template <typename T>
	using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

	tableau_page_synchrone<Module> modules{};
	tableau_page_synchrone<Fichier> fichiers{};
	tableau_page_synchrone<MetaProgramme> metaprogrammes{};

	dls::outils::Synchrone<GrapheDependance> graphe_dependance{};

	dls::outils::Synchrone<Operateurs> operateurs{};

	Typeuse typeuse;

	dls::outils::Synchrone<InterfaceKuri> interface_kuri{};

	tableau_page<AtomeFonction> fonctions{};

	using TypeDicoGlobale = dls::dico<NoeudDeclaration *, AtomeGlobale *>;
	dls::outils::Synchrone<TypeDicoGlobale> table_globales{};
	tableau_page<AtomeGlobale> globales{};

	struct DonneesConstructeurGlobale {
		AtomeGlobale *atome = nullptr;
		NoeudExpression *expression = nullptr;
		TransformationType transformation{};
	};

	using ConteneurConstructeursGlobales = kuri::tableau<DonneesConstructeurGlobale, int>;
	dls::outils::Synchrone<ConteneurConstructeursGlobales> constructeurs_globaux{};

	using TableChaine = kuri::table_hachage<kuri::chaine_statique, AtomeConstante *>;
	dls::outils::Synchrone<TableChaine> table_chaines{};

	std::mutex mutex_atomes_fonctions{};

	/* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
	NoeudDeclarationEnteteFonction *fonction_principale = nullptr;

	NoeudDeclarationEnteteFonction *fonction_point_d_entree = nullptr;

	/* Le métaprogramme controlant la compilation dans cette espace. */
	MetaProgramme *metaprogramme = nullptr;

	Coulisse *coulisse = nullptr;

	/* pour activer ou désactiver les optimisations */
	bool optimisations = false;

	explicit EspaceDeTravail(OptionsCompilation opts);

	COPIE_CONSTRUCT(EspaceDeTravail);

	~EspaceDeTravail();

	POINTEUR_NUL(EspaceDeTravail)

	/**
	 * Retourne un pointeur vers le module avec le nom et le chemin spécifiés.
	 * Si un tel module n'existe pas, un nouveau module est créé.
	 */
	Module *trouve_ou_cree_module(dls::outils::Synchrone<SystemeModule> &sys_module, IdentifiantCode *nom_module, kuri::chaine_statique chemin);

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * module n'a ce nom, retourne nullptr.
	 */
	Module *module(const IdentifiantCode *nom_module) const;

	/**
	 * Crée un fichier avec le nom spécifié, et retourne un pointeur vers le
	 * fichier ainsi créé ou un pointeur vers un fichier existant.
	 */
	ResultatFichier trouve_ou_cree_fichier(dls::outils::Synchrone<SystemeModule> &sys_module, Module *module, kuri::chaine_statique nom_fichier, kuri::chaine_statique chemin, bool importe_kuri);

	/**
	 * Retourne un pointeur vers le fichier à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	Fichier *fichier(long index) const;

	/**
	 * Retourne un pointeur vers le module dont le chemin est spécifié. Si aucun
	 * fichier n'a ce nom, retourne nullptr.
	 */
	Fichier *fichier(const dls::vue_chaine_compacte &chemin) const;

	AtomeFonction *cree_fonction(Lexeme const *lexeme, kuri::chaine const &nom_fonction);
	AtomeFonction *cree_fonction(Lexeme const *lexeme, kuri::chaine const &nom_fonction, kuri::tableau<Atome *, int> &&params);
	AtomeFonction *trouve_ou_insere_fonction(ConstructriceRI &constructrice, NoeudDeclarationEnteteFonction *decl);
	AtomeFonction *trouve_fonction(kuri::chaine const &nom_fonction);
	AtomeFonction *trouve_ou_insere_fonction_init(ConstructriceRI &constructrice, Type *type);

	AtomeGlobale *cree_globale(Type *type, AtomeConstante *valeur, bool initialisateur, bool est_constante);
	void ajoute_globale(NoeudDeclaration *decl, AtomeGlobale *atome);
	AtomeGlobale *trouve_globale(NoeudDeclaration *decl);
	AtomeGlobale *trouve_ou_insere_globale(NoeudDeclaration *decl);

	long memoire_utilisee() const;

	void rassemble_statistiques(Statistiques &stats) const;

	MetaProgramme *cree_metaprogramme();

	void tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_execution_ajoutee(dls::outils::Synchrone<Messagere> &messagere);

	void tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere, Fichier *fichier);
	void tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_execution_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere);
	void tache_liaison_executable_terminee(dls::outils::Synchrone<Messagere> &messagere);

	bool peut_generer_code_final() const;
	bool parsage_termine() const;

	void change_de_phase(dls::outils::Synchrone<Messagere> &messagere, PhaseCompilation nouvelle_phase);
	PhaseCompilation phase_courante() const;

	/* Imprime la RI de toutes les fonctions de l'espace de travail. */
	void imprime_programme() const;
};
