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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/LLVMContext.h>
#pragma GCC diagnostic pop

#include <stack>
#include <unordered_map>

#include "donnees_type.h"
#include "tampon_source.h"

class assembleuse_arbre;
class Noeud;

struct DonneesModule;

namespace llvm {
class BasicBlock;
class Type;
class Value;

namespace legacy {
class FunctionPassManager;
}
}  /* namespace llvm */

struct Metriques {
	size_t nombre_modules = 0ul;
	size_t nombre_lignes = 0ul;
	size_t nombre_morceaux = 0ul;
	size_t memoire_tampons = 0ul;
	size_t memoire_morceaux = 0ul;
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};

struct DonneesVariable {
	llvm::Value *valeur;
	size_t donnees_type{-1ul};
	bool est_variable = false;
	bool est_variadic = false;
	char pad[6] = {};
};

struct DonneesStructure {
	std::unordered_map<std::string_view, size_t> index_membres{};
	std::vector<size_t> donnees_types{};
	llvm::Type *type_llvm{nullptr};
	size_t id{0ul};
};

struct ContexteGenerationCode {
	llvm::Module *module_llvm = nullptr;
	llvm::LLVMContext contexte{};
	llvm::Function *fonction = nullptr;
	llvm::legacy::FunctionPassManager *menageur_fonctions = nullptr;
	assembleuse_arbre *assembleuse = nullptr;

	std::vector<DonneesModule *> modules{};

	MagasinDonneesType magasin_types{};

	ContexteGenerationCode() = default;

	~ContexteGenerationCode();

	/* ********************************************************************** */

	/* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
	 * compilation. */
	ContexteGenerationCode(const ContexteGenerationCode &) = delete;
	ContexteGenerationCode &operator=(const ContexteGenerationCode &) = delete;

	/* ********************************************************************** */

	/**
	 * Crée un module avec le nom spécifié, et retourne un pointeur vers le
	 * module ainsi créé. Aucune vérification n'est faite quant à la présence
	 * d'un module avec un nom similaire pour l'instant.
	 */
	DonneesModule *cree_module(const std::string &nom);

	/**
	 * Retourne un pointeur vers le module à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	DonneesModule *module(size_t index) const;

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * module n'a ce nom, retourne nullptr.
	 */
	DonneesModule *module(const std::string_view &nom) const;

	/**
	 * Retourne vrai si le module dont le nom est spécifié existe dans la liste
	 * de module de ce contexte.
	 */
	bool module_existe(const std::string_view &nom) const;

	/* ********************************************************************** */

	/**
	 * Retourne un pointeur vers le block LLVM du bloc courant.
	 */
	llvm::BasicBlock *bloc_courant() const;

	/**
	 * Met un place le pointeur vers le bloc courant LLVM.
	 */
	void bloc_courant(llvm::BasicBlock *bloc);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs de continuation de boucle.
	 */
	void empile_bloc_continue(std::string_view chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_bloc_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_continue(std::string_view chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_bloc_arrete(std::string_view chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_bloc_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_arrete(std::string_view chaine);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la globale dont le nom est spécifié en paramètres
	 * à la table de globales de ce contexte.
	 */
	void pousse_globale(const std::string_view &nom, llvm::Value *valeur, const size_t index_type);

	/**
	 * Retourne un pointeur vers la valeur LLVM de la globale dont le nom est
	 * spécifié en paramètre. Si aucune globale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_globale(const std::string_view &nom);

	/**
	 * Retourne vrai s'il existe une globale dont le nom correspond au spécifié.
	 */
	bool globale_existe(const std::string_view &nom);

	/**
	 * Retourne les données de la globale dont le nom est spécifié en
	 * paramètre. Si aucune globale ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	size_t type_globale(const std::string_view &nom);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la locale dont le nom est spécifié en paramètres
	 * à la table de locales du block courant de ce contexte. Ajourne l'index
	 * de fin des variables locales pour être au nombre de variables dans le
	 * vecteur de variables. Voir 'pop_bloc_locales' pour la gestion des
	 * variables.
	 */
	void pousse_locale(
			const std::string_view &nom,
			llvm::Value *valeur,
			const size_t &index_type,
			const bool est_variable,
			const bool est_variadique);

	/**
	 * Retourne un pointeur vers la valeur LLVM de la locale dont le nom est
	 * spécifié en paramètre. Si aucune locale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_locale(const std::string_view &nom);

	/**
	 * Retourne vrai s'il existe une locale dont le nom correspond au spécifié.
	 */
	bool locale_existe(const std::string_view &nom);

	/**
	 * Retourne les données de la locale dont le nom est spécifié en paramètre.
	 * Si aucune locale ne portant ce nom n'existe, des données vides sont
	 * retournées.
	 */
	size_t type_locale(const std::string_view &nom);

	/**
	 * Retourne vrai si la variable locale dont le nom est spécifié peut être
	 * assignée.
	 */
	bool peut_etre_assigne(const std::string_view &nom);

	/**
	 * Indique que l'on débute un nouveau bloc dans la fonction, et donc nous
	 * enregistrons le nombre de variables jusqu'ici. Voir 'pop_bloc_locales'
	 * pour la gestion des variables.
	 */
	void empile_nombre_locales();

	/**
	 * Indique que nous sortons d'un bloc, donc toutes les variables du bloc
	 * sont 'effacées'. En fait les variables des fonctions sont tenues dans un
	 * vecteur. À chaque fois que l'on rencontre un bloc, on pousse le nombre de
	 * variables sur une pile, ainsi toutes les variables se trouvant entre
	 * l'index de début du bloc et l'index de fin d'un bloc sont des variables
	 * locales à ce bloc. Quand on sort du bloc, l'index de fin des variables
	 * est réinitialisé pour être égal à ce qu'il était avant le début du bloc.
	 * Ainsi, nous pouvons réutilisé la mémoire du vecteur de variable d'un bloc
	 * à l'autre, d'une fonction à l'autre, tout en faisant en sorte que peut
	 * importe le bloc dans lequel nous nous trouvons, on a accès à toutes les
	 * variables jusqu'ici déclarées.
	 */
	void depile_nombre_locales();

	/**
	 * Imprime le nom des variables locales dans le flux précisé.
	 */
	void imprime_locales(std::ostream &os);

	/**
	 * Retourne vrai si la variable est un argument variadic. Autrement,
	 * retourne faux.
	 */
	bool est_locale_variadique(const std::string_view &nom);

	/* ********************************************************************** */

	/**
	 * Indique le début d'une nouvelle fonction. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le pointeur passé en paramètre est celui de
	 * la fonction courant.
	 */
	void commence_fonction(llvm::Function *f);

	/**
	 * Indique la fin de la fonction courant. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le bloc courant et la fonction courant sont
	 * désormais égaux à 'nullptr'.
	 */
	void termine_fonction();

	/* ********************************************************************** */

	/**
	 * Retourne vrai si le nom spécifié en paramètre est celui d'une structure
	 * ayant déjà été ajouté à la liste de structures de ce contexte.
	 */
	bool structure_existe(const std::string_view &nom);

	/**
	 * Ajoute les données de la structure dont le nom est spécifié en paramètres
	 * à la table de structure de ce contexte. Retourne l'id de la structure
	 * ainsi ajoutée.
	 */
	size_t ajoute_donnees_structure(const std::string_view &nom, DonneesStructure &donnees);

	/**
	 * Retourne les données de la structure dont le nom est spécifié en
	 * paramètre. Si aucune structure ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const std::string_view &nom);

	/**
	 * Retourne les données de la structure dont l'id est spécifié en
	 * paramètre. Si aucune structure ne portant cette id n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const size_t id);

	std::string nom_struct(const size_t id) const;

	/* ********************************************************************** */

	/**
	 * Ajoute un noeud à la pile des noeuds déférés.
	 */
	void defere_noeud(Noeud *noeud);

	/**
	 * Retourne une référence vers la pile constante de noeuds déférés.
	 */
	const std::stack<Noeud *> &noeuds_deferes() const;

	/* ********************************************************************** */

	size_t memoire_utilisee() const;

	/**
	 * Retourne les métriques de ce contexte. Les métriques sont calculées à
	 * chaque appel à cette fonction, et une structure neuve est retournée à
	 * chaque fois.
	 */
	Metriques rassemble_metriques() const;

private:
	llvm::BasicBlock *m_bloc_courant = nullptr;
	std::unordered_map<std::string_view, DonneesVariable> globales{};
	std::unordered_map<std::string_view, DonneesStructure> structures{};
	std::vector<std::string_view> nom_structures{};

	std::vector<std::pair<std::string_view, DonneesVariable>> m_locales{};
	std::stack<size_t> m_pile_nombre_locales{};
	size_t m_nombre_locales = 0;

	using paire_bloc = std::pair<std::string_view, llvm::BasicBlock *>;

	std::vector<paire_bloc> m_pile_continue{};
	std::vector<paire_bloc> m_pile_arrete{};

	std::stack<Noeud *> m_noeuds_deferes{};

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};
