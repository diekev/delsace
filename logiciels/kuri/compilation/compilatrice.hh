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

#include "espace_de_travail.hh"
#include "gerante_chaine.hh"
#include "identifiant.hh"
#include "message.hh"
#include "modules.hh"
#include "structures.hh"
#include "tacheronne.hh"

struct Statistiques;

struct Compilatrice {
	dls::outils::Synchrone<TableIdentifiant> table_identifiants{};

	dls::outils::Synchrone<OrdonnanceuseTache> ordonnanceuse;

	dls::outils::Synchrone<GeranteChaine> gerante_chaine{};

	dls::outils::Synchrone<Messagere> messagere{};

	/* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment. */
	bool importe_kuri = true;
	bool possede_erreur = false;
	bool active_tests = false;

	template <typename T>
	using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

	tableau_synchrone<kuri::chaine> bibliotheques_dynamiques{};

	tableau_synchrone<kuri::chaine> bibliotheques_statiques{};

	tableau_synchrone<dls::vue_chaine_compacte> chemins{};

	/* définitions passées au compilateur C pour modifier les fichiers d'entête */
	tableau_synchrone<dls::vue_chaine_compacte> definitions{};

	tableau_synchrone<kuri::chaine> chaines_ajoutees_a_la_compilation{};

	template <typename T>
	using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

	tableau_page_synchrone<EspaceDeTravail> espaces_de_travail{};
	EspaceDeTravail *espace_de_travail_defaut = nullptr;

	kuri::chaine racine_kuri{};

	dls::outils::Synchrone<SystemeModule> sys_module{};

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
	Module *importe_module(EspaceDeTravail *espace, kuri::chaine const &nom, NoeudExpression const *site);

	/* ********************************************************************** */

	void ajoute_fichier_a_la_compilation(EspaceDeTravail *espace, kuri::chaine const &chemin, Module *module, NoeudExpression const *site);

	/* ********************************************************************** */

	EspaceDeTravail *demarre_un_espace_de_travail(OptionsCompilation const &options, kuri::chaine const &nom);

	/* ********************************************************************** */

	long memoire_utilisee() const;

	void rassemble_statistiques(Statistiques &stats) const;
};

dls::chaine charge_fichier(
		dls::chaine const &chemin,
		EspaceDeTravail &espace,
		NoeudExpression const *site);

/* manipule les options de compilation pour l'espace de travail défaut */
OptionsCompilation *obtiens_options_compilation();
void ajourne_options_compilation(OptionsCompilation *options);

EspaceDeTravail *espace_defaut_compilation();
void compilatrice_ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine_statique c);
void compilatrice_ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine_statique c);
void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine_statique c);
int fonction_test_variadique_externe(int sentinel, ...);

EspaceDeTravail *demarre_un_espace_de_travail(kuri::chaine_statique nom, OptionsCompilation *options);

EspaceDeTravail *compilatrice_espace_courant();

Message const *compilatrice_attend_message();
void compilatrice_commence_interception(EspaceDeTravail *espace);
void compilatrice_termine_interception(EspaceDeTravail *espace);

void compilatrice_rapporte_erreur(EspaceDeTravail *espace, kuri::chaine_statique fichier, int ligne, kuri::chaine_statique message);

/* ATTENTION: le paramètre « site » ne fait pas partie de l'interface de la fonction !
 * Cette fonction n'est pas appelée via FFI, mais est manuellement détectée et appelée
 * avec le site renseigné.
 */
kuri::tableau<kuri::Lexeme> compilatrice_lexe_fichier(kuri::chaine_statique chemin_donne, NoeudExpression const *site);
