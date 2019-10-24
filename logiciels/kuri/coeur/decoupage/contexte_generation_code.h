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

#ifdef AVEC_LLVM
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

namespace llvm {
class BasicBlock;
class Type;
class Value;

namespace legacy {
class FunctionPassManager;
}
}  /* namespace llvm */
#endif

#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/liste.hh"

#include "donnees_type.h"

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
#ifdef AVEC_LLVM
	llvm::Value *valeur{nullptr};
#endif
	long donnees_type{-1l};
	bool est_dynamique = false;
	bool est_variadic = false;
	char drapeaux = 0;
	bool est_argument = false;
	bool est_membre_emploie = false;
	char pad[3] = {};

	/* nom de la structure pour l'accès des variables employées */
	dls::chaine structure = "";
};

struct DonneesMembre {
	long index_membre{};
	noeud::base *noeud_decl = nullptr;

	/* le décalage en octets dans la struct */
	unsigned int decalage = 0;
};

struct DonneesStructure {
	dls::dico_desordonne<dls::vue_chaine, DonneesMembre> donnees_membres{};
	dls::tableau<long> donnees_types{};

#ifdef AVEC_LLVM
	llvm::Type *type_llvm{nullptr};
#endif

	long id{0l};
	long index_type{-1l};
	noeud::base *noeud_decl = nullptr;
	bool est_enum = false;
	bool est_externe = false;
	char pad[2] = {};
	unsigned int taille_octet = 0;
};

struct DonneesFonction;

using conteneur_globales = dls::dico_desordonne<dls::vue_chaine, DonneesVariable>;
using conteneur_locales = dls::tableau<std::pair<dls::vue_chaine, DonneesVariable>>;

struct ContexteGenerationCode {
#ifdef AVEC_LLVM
	llvm::Module *module_llvm = nullptr;
	llvm::LLVMContext contexte{};
	llvm::Function *fonction = nullptr;
	llvm::legacy::FunctionPassManager *menageur_fonctions = nullptr;
#endif

	assembleuse_arbre *assembleuse = nullptr;

	dls::tableau<DonneesModule *> modules{};

	MagasinDonneesType magasin_types{};

	DonneesFonction *donnees_fonction = nullptr;

	/* magasin pour que les string_views des chaines temporaires soient toujours
	 * valides (notamment utilisé pour les variables des boucles dans les
	 * coroutines)
	 * utilisation d'une liste afin d'éviter les crashs quand on tient une
	 * référence à une chaine qui sera libéré */
	dls::liste<dls::chaine> magasin_chaines{};

	long index_type_ctx = -1;

	bool bit32 = false;

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
	DonneesModule *module(const dls::vue_chaine &nom) const;

	/**
	 * Retourne vrai si le module dont le nom est spécifié existe dans la liste
	 * de module de ce contexte.
	 */
	bool module_existe(const dls::vue_chaine &nom) const;

	/* ********************************************************************** */

#ifdef AVEC_LLVM
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
	void empile_bloc_continue(dls::vue_chaine chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_bloc_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_continue(dls::vue_chaine chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_bloc_arrete(dls::vue_chaine chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_bloc_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_arrete(dls::vue_chaine chaine);
#endif

	/* ********************************************************************** */

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs de continuation de boucle.
	 */
	void empile_goto_continue(dls::vue_chaine chaine, dls::chaine const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_goto_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	dls::chaine goto_continue(dls::vue_chaine chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_goto_arrete(dls::vue_chaine chaine, dls::chaine const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_goto_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	dls::chaine goto_arrete(dls::vue_chaine chaine);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la globale dont le nom est spécifié en paramètres
	 * à la table de globales de ce contexte.
	 */
	void pousse_globale(const dls::vue_chaine &nom, DonneesVariable const &donnees);

#ifdef AVEC_LLVM
	/**
	 * Retourne un pointeur vers la valeur LLVM de la globale dont le nom est
	 * spécifié en paramètre. Si aucune globale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_globale(const dls::vue_chaine &nom);
#endif

	/**
	 * Retourne vrai s'il existe une globale dont le nom correspond au spécifié.
	 */
	bool globale_existe(const dls::vue_chaine &nom);

	/**
	 * Retourne les données de la globale dont le nom est spécifié en
	 * paramètre. Si aucune globale ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	long type_globale(const dls::vue_chaine &nom);

	conteneur_globales::const_iteratrice iter_globale(const dls::vue_chaine &nom);

	conteneur_globales::const_iteratrice fin_globales();

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la locale dont le nom est spécifié en paramètres
	 * à la table de locales du block courant de ce contexte. Ajourne l'index
	 * de fin des variables locales pour être au nombre de variables dans le
	 * vecteur de variables. Voir 'pop_bloc_locales' pour la gestion des
	 * variables.
	 */
	void pousse_locale(
			const dls::vue_chaine &nom,
			DonneesVariable const &donnees);

	char drapeaux_variable(dls::vue_chaine const &nom);

	DonneesVariable &donnees_variable(const dls::vue_chaine &nom);

#ifdef AVEC_LLVM
	/**
	 * Retourne un pointeur vers la valeur LLVM de la locale dont le nom est
	 * spécifié en paramètre. Si aucune locale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_locale(const dls::vue_chaine &nom);
#endif

	/**
	 * Retourne vrai s'il existe une locale dont le nom correspond au spécifié.
	 */
	bool locale_existe(const dls::vue_chaine &nom);

	/**
	 * Retourne les données de la locale dont le nom est spécifié en paramètre.
	 * Si aucune locale ne portant ce nom n'existe, des données vides sont
	 * retournées.
	 */
	long type_locale(const dls::vue_chaine &nom);

	/**
	 * Retourne vrai si la variable locale dont le nom est spécifié peut être
	 * assignée.
	 */
	bool peut_etre_assigne(const dls::vue_chaine &nom);

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
	bool est_locale_variadique(const dls::vue_chaine &nom);

	conteneur_locales::const_iteratrice iter_locale(const dls::vue_chaine &nom) const;

	conteneur_locales::const_iteratrice debut_locales() const;

	conteneur_locales::const_iteratrice fin_locales() const;

	/* ********************************************************************** */

	/**
	 * Indique le début d'une nouvelle fonction. Le compteur et le vecteur de
	 * variables sont remis à zéro. Le pointeur passé en paramètre est celui de
	 * la fonction courant.
	 */
#ifdef AVEC_LLVM
	void commence_fonction(llvm::Function *f, DonneesFonction *df);
#endif
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
	bool structure_existe(const dls::vue_chaine &nom);

	/**
	 * Ajoute les données de la structure dont le nom est spécifié en paramètres
	 * à la table de structure de ce contexte. Retourne l'id de la structure
	 * ainsi ajoutée.
	 */
	long ajoute_donnees_structure(dls::vue_chaine const &nom, DonneesStructure &donnees);

	/**
	 * Retourne les données de la structure dont le nom est spécifié en
	 * paramètre. Si aucune structure ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const dls::vue_chaine &nom);

	/**
	 * Retourne les données de la structure dont l'id est spécifié en
	 * paramètre. Si aucune structure ne portant cette id n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const long id);

	dls::chaine nom_struct(const long id) const;

	/* ********************************************************************** */

	/**
	 * Ajoute un noeud à la pile des noeuds différés.
	 */
	void differe_noeud(noeud::base *noeud);

	/**
	 * Retourne une référence vers la pile constante de noeuds différés.
	 */
	dls::tableau<noeud::base *> const &noeuds_differes() const;

	/**
	 * Retourne une liste des noeuds différés du bloc courant.
	 */
	dls::tableau<noeud::base *> noeuds_differes_bloc() const;

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

	dls::dico_desordonne<dls::vue_chaine, DonneesStructure> structures{};

private:
#ifdef AVEC_LLVM
	llvm::BasicBlock *m_bloc_courant = nullptr;
#endif
	conteneur_globales globales{};
	dls::tableau<dls::vue_chaine> nom_structures{};

	conteneur_locales m_locales{};
	dls::pile<long> m_pile_nombre_locales{};
	long m_nombre_locales = 0;

	dls::pile<long> m_pile_nombre_differes{};
	long m_nombre_differes = 0;

#ifdef AVEC_LLVM
	using paire_bloc = std::pair<dls::vue_chaine, llvm::BasicBlock *>;

	dls::tableau<paire_bloc> m_pile_continue{};
	dls::tableau<paire_bloc> m_pile_arrete{};
#endif

	using paire_goto = std::pair<dls::vue_chaine, dls::chaine>;

	dls::tableau<paire_goto> m_pile_goto_continue{};
	dls::tableau<paire_goto> m_pile_goto_arrete{};

	dls::tableau<noeud::base *> m_noeuds_differes{};

	bool m_non_sur = false;

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};
