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

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"

#include "graphe_dependance.hh"
#include "identifiant.hh"
#include "message.hh"
#include "modules.hh"
#include "operateurs.hh"
#include "options.hh"
#include "tacheronne.hh"

struct Statistiques;

struct GeranteChaine {
	/* stocke la capacité réservée par la lexeuse en attendant d'avoir la capacité comme mebre de kuri::chaine */
	struct ChainteEtCapacite {
		kuri::chaine chaine;
		long capacite;
	};

	dls::tableau<ChainteEtCapacite> m_table{};

	~GeranteChaine();

	void ajoute_chaine(kuri::chaine const &chaine, long capacite);
};

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
	dls::chaine nom{};
	OptionsCompilation options{};

	template <typename T>
	using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

	tableau_page_synchrone<Module> modules{};
	tableau_page_synchrone<Fichier> fichiers{};

	dls::outils::Synchrone<GrapheDependance> graphe_dependance{};

	dls::outils::Synchrone<Operateurs> operateurs{};

	Typeuse typeuse;

	dls::outils::Synchrone<InterfaceKuri> interface_kuri{};

	using TypeDicoFonction = dls::dico<dls::chaine, AtomeFonction *>;
	tableau_page<AtomeFonction> fonctions{};

	using TypeDicoGlobale = dls::dico<NoeudDeclaration *, AtomeGlobale *>;
	dls::outils::Synchrone<TypeDicoGlobale> table_globales{};
	tableau_page<AtomeGlobale> globales{};

	struct DonneesConstructeurGlobale {
		AtomeGlobale *atome = nullptr;
		NoeudExpression *expression = nullptr;
		TransformationType transformation{};
	};

	using ConteneurConstructeursGlobales = dls::tableau<DonneesConstructeurGlobale>;
	dls::outils::Synchrone<ConteneurConstructeursGlobales> constructeurs_globaux{};

	using TableChaine = dls::dico<dls::chaine, AtomeConstante *>;
	dls::outils::Synchrone<TableChaine> table_chaines{};

	std::atomic<int> nombre_taches_parsage = 0;
	std::atomic<int> nombre_taches_typage = 0;
	std::atomic<int> nombre_taches_ri = 0;
	std::atomic<int> nombre_taches_execution = 0;

	std::mutex mutex_atomes_fonctions{};

	PhaseCompilation phase{};

	/* mise en cache de la fonction principale, si vue dans la Syntaxeuse */
	NoeudDeclarationEnteteFonction *fonction_principale = nullptr;

	explicit EspaceDeTravail(OptionsCompilation opts);

	COPIE_CONSTRUCT(EspaceDeTravail);

	~EspaceDeTravail();

	POINTEUR_NUL(EspaceDeTravail)

	/**
	 * Crée un module avec le nom spécifié, et retourne un pointeur vers le
	 * module ainsi créé. Si un module avec le même chemin existe, il est
	 * retourné sans qu'un nouveau module ne soit créé.
	 */
	Module *cree_module(dls::chaine const &nom_module, dls::chaine const &chemin);

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * module n'a ce nom, retourne nullptr.
	 */
	Module *module(const dls::vue_chaine_compacte &nom_module) const;

	/**
	 * Crée un fichier avec le nom spécifié, et retourne un pointeur vers le
	 * fichier ainsi créé. Aucune vérification n'est faite quant à la présence
	 * d'un fichier avec un nom similaire pour l'instant.
	 */
	Fichier *cree_fichier(dls::chaine const &nom_fichier, dls::chaine const &chemin, bool importe_kuri);

	/**
	 * Retourne un pointeur vers le fichier à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	Fichier *fichier(long index) const;

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * fichier n'a ce nom, retourne nullptr.
	 */
	Fichier *fichier(const dls::vue_chaine_compacte &nom_fichier) const;

	AtomeFonction *cree_fonction(Lexeme const *lexeme, dls::chaine const &nom_fonction);
	AtomeFonction *cree_fonction(Lexeme const *lexeme, dls::chaine const &nom_fonction, kuri::tableau<Atome *> &&params);
	AtomeFonction *trouve_ou_insere_fonction(ConstructriceRI &constructrice, NoeudDeclarationEnteteFonction *decl);
	AtomeFonction *trouve_fonction(dls::chaine const &nom_fonction);
	AtomeFonction *trouve_ou_insere_fonction_init(ConstructriceRI &constructrice, Type *type);

	AtomeGlobale *cree_globale(Type *type, AtomeConstante *valeur, bool initialisateur, bool est_constante);
	void ajoute_globale(NoeudDeclaration *decl, AtomeGlobale *atome);
	AtomeGlobale *trouve_globale(NoeudDeclaration *decl);
	AtomeGlobale *trouve_ou_insere_globale(NoeudDeclaration *decl);

	long memoire_utilisee() const;

	void rassemble_statistiques(Statistiques &stats) const;
};

struct Compilatrice {
	dls::outils::Synchrone<TableIdentifiant> table_identifiants{};

	dls::outils::Synchrone<OrdonnanceuseTache> ordonnanceuse;

	dls::outils::Synchrone<GeranteChaine> gerante_chaine{};

	dls::outils::Synchrone<Messagere> messagere{};

	/* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment. */
	bool importe_kuri = true;
	bool possede_erreur = false;

	template <typename T>
	using tableau_synchrone = dls::outils::Synchrone<dls::tableau<T>>;

	tableau_synchrone<dls::chaine> bibliotheques_dynamiques{};

	tableau_synchrone<dls::chaine> bibliotheques_statiques{};

	tableau_synchrone<dls::vue_chaine_compacte> chemins{};

	/* définitions passées au compilateur C pour modifier les fichiers d'entête */
	tableau_synchrone<dls::vue_chaine_compacte> definitions{};

	tableau_synchrone<dls::chaine> chaines_ajoutees_a_la_compilation{};

	template <typename T>
	using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

	tableau_page_synchrone<EspaceDeTravail> espaces_de_travail{};
	EspaceDeTravail *espace_de_travail_defaut = nullptr;

	dls::chaine racine_kuri{};

	/* ********************************************************************** */

	Compilatrice();

	/* ********************************************************************** */

	/* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
	 * compilation. */
	Compilatrice(const Compilatrice &) = delete;
	Compilatrice &operator=(const Compilatrice &) = delete;

	/* ********************************************************************** */

	/**
	 * Charge le module dont le nom est spécifié.
	 *
	 * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
	 * dans le dossier du module racine.
	 *
	 * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
	 * du module, sauf pour le module racine.
	 *
	 * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
	 * pour tenir compte de la progression de la compilation. Si un nom de module ne
	 * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
	 * exception est lancée.
	 *
	 * Les Lexeme doivent être celles du nom du module et sont utilisées
	 * pour les erreurs lancées.
	 *
	 * Le paramètre est_racine ne doit être vrai que pour le module racine.
	 */
	Module *importe_module(EspaceDeTravail *espace, dls::chaine const &nom, Lexeme const &lexeme);

	/* ********************************************************************** */

	void ajoute_fichier_a_la_compilation(EspaceDeTravail *espace, dls::chaine const &chemin, Module *module, Lexeme const &lexeme);

	/* ********************************************************************** */

	EspaceDeTravail *demarre_un_espace_de_travail(OptionsCompilation const &options, dls::chaine const &nom);

	/* ********************************************************************** */

	long memoire_utilisee() const;

	void rassemble_statistiques(Statistiques &stats) const;
};

dls::chaine charge_fichier(
		dls::chaine const &chemin,
		EspaceDeTravail &espace,
		Lexeme const &lexeme);

/* manipule les options de compilation pour l'espace de travail défaut */
OptionsCompilation *obtiens_options_compilation();
void ajourne_options_compilation(OptionsCompilation *options);

EspaceDeTravail *espace_defaut_compilation();
void compilatrice_ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine c);
void compilatrice_ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine c);
void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine c);
int fonction_test_variadique_externe(int sentinel, ...);

EspaceDeTravail *demarre_un_espace_de_travail(kuri::chaine nom, OptionsCompilation *options);

Message const *compilatrice_attend_message();
void compilatrice_commence_interception(EspaceDeTravail *espace);
void compilatrice_termine_interception(EspaceDeTravail *espace);

void compilatrice_rapporte_erreur(EspaceDeTravail *espace, kuri::chaine fichier, int ligne, kuri::chaine message);

kuri::tableau<kuri::Lexeme> compilatrice_lexe_fichier(kuri::chaine chemin_donne);
