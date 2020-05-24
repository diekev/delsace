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

#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/liste.hh"

#include "allocatrice_noeud.hh"
#include "operateurs.hh"
#include "expression.h"
#include "graphe_dependance.hh"
#include "typage.hh"

class assembleuse_arbre;

struct DonneesModule;
struct Fichier;

struct Metriques {
	size_t nombre_modules = 0ul;
	size_t nombre_lignes = 0ul;
	size_t nombre_lexemes = 0ul;
	size_t nombre_noeuds = 0ul;
	size_t memoire_tampons = 0ul;
	size_t memoire_lexemes = 0ul;
	size_t memoire_arbre = 0ul;
	size_t memoire_contexte = 0ul;
	size_t memoire_types = 0ul;
	size_t memoire_operateurs = 0ul;
	size_t memoire_ri = 0ul;
	long nombre_types = 0;
	long nombre_operateurs = 0;
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;
	double temps_validation = 0.0;
	double temps_generation = 0.0;
	double temps_fichier_objet = 0.0;
	double temps_executable = 0.0;
	double temps_nettoyage = 0.0;
	double temps_ri = 0.0;
};

struct IdentifiantCode {
	dls::vue_chaine_compacte nom{};
};

struct TableIdentifiant {
private:
	// À FAIRE : il serait bien d'utiliser un dico simple car plus rapide, ne
	// nécissitant pas de hachage, mais dico échoue lors des comparaisons de
	// vue_chaine_compacte par manque de caractère nul à la fin des chaines
	dls::dico_desordonne<dls::vue_chaine_compacte, IdentifiantCode *> table{};
	dls::tableau<IdentifiantCode *> identifiants{};

public:
	~TableIdentifiant()
	{
		for (auto ident : identifiants) {
			memoire::deloge("IdentifiantCode", ident);
		}
	}

	IdentifiantCode *identifiant_pour_chaine(dls::vue_chaine_compacte const &nom)
	{
		auto iter = table.trouve(nom);

		if (iter != table.fin()) {
			return iter->second;
		}

		auto ident = memoire::loge<IdentifiantCode>("IdentifiantCode");
		ident->nom = nom;

		table.insere({ nom, ident });
		identifiants.pousse(ident);

		return ident;
	}
};

struct GeranteChaine {
	dls::tableau<kuri::chaine> m_table{};

	~GeranteChaine();

	void ajoute_chaine(kuri::chaine const &chaine);
};

// Interface avec le module « Kuri », pour certaines fonctions intéressantes
struct InterfaceKuri {
	NoeudDeclarationFonction *decl_panique = nullptr;
	NoeudDeclarationFonction *decl_panique_tableau = nullptr;
	NoeudDeclarationFonction *decl_panique_chaine = nullptr;
	NoeudDeclarationFonction *decl_panique_membre_union = nullptr;
	NoeudDeclarationFonction *decl_panique_memoire = nullptr;
	NoeudDeclarationFonction *decl_panique_erreur = nullptr;
	NoeudDeclarationFonction *decl_rappel_panique_defaut = nullptr;
	NoeudDeclarationFonction *decl_dls_vers_r32 = nullptr;
	NoeudDeclarationFonction *decl_dls_vers_r64 = nullptr;
	NoeudDeclarationFonction *decl_dls_depuis_r32 = nullptr;
	NoeudDeclarationFonction *decl_dls_depuis_r64 = nullptr;
};

struct ContexteGenerationCode {
	AllocatriceNoeud allocatrice_noeud{};

	/* À FAIRE : supprime ceci */
	assembleuse_arbre *assembleuse = nullptr;

	dls::tableau<DonneesModule *> modules{};
	dls::tableau<Fichier *> fichiers{};

	GrapheDependance graphe_dependance{};

	Operateurs operateurs{};

	Typeuse typeuse;

	TableIdentifiant table_identifiants{};

	NoeudDeclarationFonction *donnees_fonction = nullptr;

	NoeudExpressionAppel *pour_appel = nullptr;

	InterfaceKuri interface_kuri{};

	/* Les données des dépendances d'un noeud syntaxique, utilisée lors de la
	 * validation sémantique. */
	DonneesDependance donnees_dependance{};

	Type *type_contexte = nullptr;

	bool bit32 = false;

	dls::tableau<NoeudExpression *> noeuds_a_executer{};

	bool pour_gabarit = false;
	dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>> paires_expansion_gabarit{};

	dls::liste<NoeudExpression *> file_typage{};

	GeranteChaine gerante_chaine{};

	/* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment. */
	bool importe_kuri = true;

	ContexteGenerationCode();

	~ContexteGenerationCode();

	/* ********************************************************************** */

	/* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
	 * compilation. */
	ContexteGenerationCode(const ContexteGenerationCode &) = delete;
	ContexteGenerationCode &operator=(const ContexteGenerationCode &) = delete;

	/* ********************************************************************** */

	/**
	 * Crée un module avec le nom spécifié, et retourne un pointeur vers le
	 * module ainsi créé. Si un module avec le même chemin existe, il est
	 * retourné sans qu'un nouveau module ne soit créé.
	 */
	DonneesModule *cree_module(dls::chaine const &nom, dls::chaine const &chemin);

	/**
	 * Retourne un pointeur vers le module à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	DonneesModule *module(size_t index) const;

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * module n'a ce nom, retourne nullptr.
	 */
	DonneesModule *module(const dls::vue_chaine_compacte &nom) const;

	/**
	 * Retourne vrai si le module dont le nom est spécifié existe dans la liste
	 * de module de ce contexte.
	 */
	bool module_existe(const dls::vue_chaine_compacte &nom) const;

	/* ********************************************************************** */

	/**
	 * Crée un fichier avec le nom spécifié, et retourne un pointeur vers le
	 * fichier ainsi créé. Aucune vérification n'est faite quant à la présence
	 * d'un fichier avec un nom similaire pour l'instant.
	 */
	Fichier *cree_fichier(dls::chaine const &nom, dls::chaine const &chemin);

	/**
	 * Retourne un pointeur vers le fichier à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	Fichier *fichier(size_t index) const;

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * fichier n'a ce nom, retourne nullptr.
	 */
	Fichier *fichier(const dls::vue_chaine_compacte &nom) const;

	/**
	 * Retourne vrai si le fichier dont le nom est spécifié existe dans la liste
	 * de fichier de ce contexte.
	 */
	bool fichier_existe(const dls::vue_chaine_compacte &nom) const;

	/* ********************************************************************** */

	void empile_controle_boucle(IdentifiantCode *ident_boucle);

	void depile_controle_boucle();

	bool possede_controle_boucle(IdentifiantCode *ident);

	/* ********************************************************************** */

	/**
	 * Indique le début d'une nouvelle fonction. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le pointeur passé en paramètre est celui de
	 * la fonction courant.
	 */
	void commence_fonction(NoeudDeclarationFonction *df);

	/**
	 * Indique la fin de la fonction courant. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le bloc courant et la fonction courant sont
	 * désormais égaux à 'nullptr'.
	 */
	void termine_fonction();

	/* ********************************************************************** */

	size_t memoire_utilisee() const;

	/**
	 * Retourne les métriques de ce contexte. Les métriques sont calculées à
	 * chaque appel à cette fonction, et une structure neuve est retournée à
	 * chaque fois.
	 */
	Metriques rassemble_metriques() const;

	/* gestion des membres actifs des unions :
	 * cas à considérer :
	 * -- les portées des variables
	 * -- les unions dans les structures (accès par '.')
	 */
	dls::vue_chaine_compacte trouve_membre_actif(dls::vue_chaine_compacte const &nom_union);

	void renseigne_membre_actif(dls::vue_chaine_compacte const &nom_union, dls::vue_chaine_compacte const &nom_membre);

	dls::tableau<IdentifiantCode *> pile_controle_boucle{};

private:
	using paire_union_membre = std::pair<dls::vue_chaine_compacte, dls::vue_chaine_compacte>;
	dls::tableau<paire_union_membre> membres_actifs{};

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;

	// pour les variables des boucles
	bool est_coulisse_llvm = false;
};
