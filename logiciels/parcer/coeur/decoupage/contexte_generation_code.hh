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

#include <stack>
#include <unordered_map>

#include "donnees_type.hh"

class assembleuse_arbre;

namespace noeud {
struct base;
}

struct DonneesModule;

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

enum {
	BESOIN_DEREF = (1 << 0),
};

struct DonneesVariable {
	size_t donnees_type{-1ul};
	bool est_dynamique = false;
	bool est_variadic = false;
	char drapeaux = 0;
	bool est_argument = false;
	bool est_membre_emploie = false;
	char pad[3] = {};

	/* nom de la structure pour l'accès des variables employées */
	std::string structure = "";
};

struct DonneesMembre {
	size_t index_membre{};
	noeud::base *noeud_decl = nullptr;
};

struct DonneesStructure {
	std::unordered_map<std::string_view, DonneesMembre> donnees_membres{};
	std::vector<size_t> donnees_types{};

	size_t id{0ul};
	size_t index_type{-1ul};
	noeud::base *noeud_decl = nullptr;
	bool est_enum = false;
	bool est_externe = false;
	char pad[6] = {};
};

struct DonneesFonction;

using conteneur_globales = std::unordered_map<std::string_view, DonneesVariable>;
using conteneur_locales = std::vector<std::pair<std::string_view, DonneesVariable>>;

struct ContexteGenerationCode {
	assembleuse_arbre *assembleuse = nullptr;

	std::vector<DonneesModule *> modules{};

	MagasinDonneesType magasin_types{};

	DonneesFonction *donnees_fonction = nullptr;

	/* magasin pour que les string_views des chaines temporaires soient toujours
	 * valides (notamment utilisé pour les variables des boucles dans les
	 * coroutines) */
	std::vector<std::string> magasin_chaines{};

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
	DonneesModule *cree_module(std::string const &nom, std::string const &chemin);

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
	 * Ajoute le bloc spécifié sur la pile de blocs de continuation de boucle.
	 */
	void empile_goto_continue(std::string_view chaine, std::string const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_goto_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	std::string goto_continue(std::string_view chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_goto_arrete(std::string_view chaine, std::string const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_goto_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	std::string goto_arrete(std::string_view chaine);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la globale dont le nom est spécifié en paramètres
	 * à la table de globales de ce contexte.
	 */
	void pousse_globale(const std::string_view &nom, DonneesVariable const &donnees);

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

	conteneur_globales::const_iterator iter_globale(const std::string_view &nom);

	conteneur_globales::const_iterator fin_globales();

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
			DonneesVariable const &donnees);

	char drapeaux_variable(std::string_view const &nom);

	DonneesVariable &donnees_variable(const std::string_view &nom);

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

	conteneur_locales::const_iterator iter_locale(const std::string_view &nom);

	conteneur_locales::const_iterator debut_locales();

	conteneur_locales::const_iterator fin_locales();

	/* ********************************************************************** */

	/**
	 * Indique le début d'une nouvelle fonction. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le pointeur passé en paramètre est celui de
	 * la fonction courant.
	 */
	void commence_fonction(DonneesFonction *df);

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
	 * Ajoute un noeud à la pile des noeuds différés.
	 */
	void differe_noeud(noeud::base *noeud);

	/**
	 * Retourne une référence vers la pile constante de noeuds différés.
	 */
	std::vector<noeud::base *> const &noeuds_differes() const;

	/**
	 * Retourne une liste des noeuds différés du bloc courant.
	 */
	std::vector<noeud::base *> noeuds_differes_bloc() const;

	/* ********************************************************************** */

	size_t memoire_utilisee() const;

	/**
	 * Retourne les métriques de ce contexte. Les métriques sont calculées à
	 * chaque appel à cette fonction, et une structure neuve est retournée à
	 * chaque fois.
	 */
	Metriques rassemble_metriques() const;

	/* ********************************************************************** */

	/**
	 * Définie si oui ou non le contexte est non-sûr, c'est-à-dire que l'on peut
	 * manipuler des objets dangereux.
	 */
	void non_sur(bool ouinon);

	/**
	 * Retourne si oui ou non le contexte est non-sûr.
	 */
	bool non_sur() const;

	std::unordered_map<std::string_view, DonneesStructure> structures{};

private:
	conteneur_globales globales{};
	std::vector<std::string_view> nom_structures{};

	conteneur_locales m_locales{};
	std::stack<size_t> m_pile_nombre_locales{};
	size_t m_nombre_locales = 0;

	std::stack<size_t> m_pile_nombre_differes{};
	size_t m_nombre_differes = 0;

	using paire_goto = std::pair<std::string_view, std::string>;

	std::vector<paire_goto> m_pile_goto_continue{};
	std::vector<paire_goto> m_pile_goto_arrete{};

	std::vector<noeud::base *> m_noeuds_differes{};

	bool m_non_sur = false;

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};
