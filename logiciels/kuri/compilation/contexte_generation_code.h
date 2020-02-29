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

#include "operateurs.hh"
#include "expression.h"
#include "graphe_dependance.hh"
#include "typage.hh"

class assembleuse_arbre;

namespace noeud {
struct base;
}

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
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;
	double temps_validation = 0.0;
	double temps_generation = 0.0;
	double temps_fichier_objet = 0.0;
	double temps_executable = 0.0;
	double temps_nettoyage = 0.0;
};

struct DonneesVariable {
#ifdef AVEC_LLVM
	llvm::Value *valeur{nullptr};
#endif
	Type *type{nullptr};
	bool est_dynamique = false;
	bool est_variadic = false;
	bool est_argument = false;
	bool est_membre_emploie = false;
	bool est_externe = false;
	bool est_var_boucle = false;
	char pad[2] = {};

	/* nom de la structure pour l'accès des variables employées */
	dls::chaine structure = "";

	/* pour les évaluations des énums pour l'instant */
	ResultatExpression resultat_expression{};
};

struct DonneesMembre {
	long index_membre{};
	noeud::base *noeud_decl = nullptr;

	/* le décalage en octets dans la struct */
	unsigned int decalage = 0;

	/* pour les évaluations des énums pour l'instant */
	ResultatExpression resultat_expression{};
};

struct DonneesStructure {
	dls::dico_desordonne<dls::vue_chaine_compacte, DonneesMembre> donnees_membres{};
	dls::tableau<Type *> types{};

#ifdef AVEC_LLVM
	llvm::Type *type_llvm{nullptr};
#endif

	long id{0l};
	Type *type{nullptr};
	noeud::base *noeud_decl = nullptr;
	unsigned int taille_octet = 0;
	bool est_enum = false;
	bool est_drapeau = false;
	bool est_externe = false;
	bool est_union = false;
	bool est_nonsur = false;
	REMBOURRE(7);

	/* pour la prédéclaration des InfoType* */
	bool deja_genere = false;
	REMBOURRE(7);
};

/* petite ébauche pour un réusinage */
//#include "structures.hh"

//struct MembreStructure {
//	kuri::chaine nom;
//	/* le décalage en octets dans la struct */
//	int decalage = 0;
//	long index_type = -1;
//};

//struct DescriptionStructure {
//	kuri::tableau<MembreStructure> membres;
//	unsigned int taille_octet;
//	bool est_union;
//	bool est_nonsure;
//	bool est_externe;
//	/* pour la prédéclaration des InfoType* */
//	bool deja_genere;
//};

//struct DescriptionEnum {
//	kuri::tableau<kuri::chaine> noms;
//	kuri::tableau<int> valeurs;
//	long index_type;
//	bool est_drapeau;
//	/* pour la prédéclaration des InfoType* */
//	bool deja_genere;
//};

struct DonneesFonction;

using conteneur_globales = dls::dico_desordonne<dls::vue_chaine_compacte, DonneesVariable>;
using conteneur_locales = dls::tableau<std::pair<dls::vue_chaine_compacte, DonneesVariable>>;

struct ContexteGenerationCode {
#ifdef AVEC_LLVM
	llvm::Module *module_llvm = nullptr;
	llvm::LLVMContext contexte{};
	llvm::Function *fonction = nullptr;
	llvm::legacy::FunctionPassManager *menageur_fonctions = nullptr;
#endif

	assembleuse_arbre *assembleuse = nullptr;

	dls::tableau<DonneesModule *> modules{};
	dls::tableau<Fichier *> fichiers{};

	GrapheDependance graphe_dependance{};

	Operateurs operateurs{};

	Typeuse typeuse;

	DonneesFonction *donnees_fonction = nullptr;

	/* Les données des dépendances d'un noeud syntaxique, utilisée lors de la
	 * validation sémantique. */
	DonneesDependance donnees_dependance{};

	Type *type_contexte = nullptr;

	bool bit32 = false;

	dls::tableau<noeud::base *> noeuds_a_executer{};

	bool pour_gabarit = false;
	dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>> paires_expansion_gabarit{};

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
	void empile_bloc_continue(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_bloc_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_continue(dls::vue_chaine_compacte chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_bloc_arrete(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_bloc_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	llvm::BasicBlock *bloc_arrete(dls::vue_chaine_compacte chaine);
#endif

	/* ********************************************************************** */

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs de continuation de boucle.
	 */
	void empile_goto_continue(dls::vue_chaine_compacte chaine, dls::chaine const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs de continuation de boucle.
	 */
	void depile_goto_continue();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs de continuation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	dls::chaine goto_continue(dls::vue_chaine_compacte chaine);

	/**
	 * Ajoute le bloc spécifié sur la pile de blocs d'arrestation de boucle.
	 */
	void empile_goto_arrete(dls::vue_chaine_compacte chaine, dls::chaine const &bloc);

	/**
	 * Enlève le bloc spécifié de la pile de blocs d'arrestation de boucle.
	 */
	void depile_goto_arrete();

	/**
	 * Retourne le bloc se trouvant au sommet de la pile de blocs d'arrestation
	 * de boucle. Si la pile est vide, retourne un pointeur nul.
	 */
	dls::chaine goto_arrete(dls::vue_chaine_compacte chaine);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la globale dont le nom est spécifié en paramètres
	 * à la table de globales de ce contexte.
	 */
	void pousse_globale(const dls::vue_chaine_compacte &nom, DonneesVariable const &donnees);

#ifdef AVEC_LLVM
	/**
	 * Retourne un pointeur vers la valeur LLVM de la globale dont le nom est
	 * spécifié en paramètre. Si aucune globale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_globale(const dls::vue_chaine_compacte &nom);
#endif

	/**
	 * Retourne vrai s'il existe une globale dont le nom correspond au spécifié.
	 */
	bool globale_existe(const dls::vue_chaine_compacte &nom);

	conteneur_globales::const_iteratrice iter_globale(const dls::vue_chaine_compacte &nom);

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
			const dls::vue_chaine_compacte &nom,
			DonneesVariable const &donnees);

	DonneesVariable &donnees_variable(const dls::vue_chaine_compacte &nom);

#ifdef AVEC_LLVM
	/**
	 * Retourne un pointeur vers la valeur LLVM de la locale dont le nom est
	 * spécifié en paramètre. Si aucune locale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_locale(const dls::vue_chaine_compacte &nom);
#endif

	/**
	 * Retourne vrai s'il existe une locale dont le nom correspond au spécifié.
	 */
	bool locale_existe(const dls::vue_chaine_compacte &nom);

	/**
	 * Retourne vrai si la variable locale dont le nom est spécifié peut être
	 * assignée.
	 */
	bool peut_etre_assigne(const dls::vue_chaine_compacte &nom);

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
	bool est_locale_variadique(const dls::vue_chaine_compacte &nom);

	conteneur_locales::const_iteratrice iter_locale(const dls::vue_chaine_compacte &nom) const;

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
	bool structure_existe(const dls::vue_chaine_compacte &nom);

	/**
	 * Ajoute les données de la structure dont le nom est spécifié en paramètres
	 * à la table de structure de ce contexte. Retourne l'id de la structure
	 * ainsi ajoutée.
	 */
	long ajoute_donnees_structure(dls::vue_chaine_compacte const &nom, DonneesStructure &donnees);

	/**
	 * Retourne les données de la structure dont le nom est spécifié en
	 * paramètre. Si aucune structure ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const dls::vue_chaine_compacte &nom);
	DonneesStructure const &donnees_structure(const dls::vue_chaine_compacte &nom) const;

	/**
	 * Retourne les données de la structure dont l'id est spécifié en
	 * paramètre. Si aucune structure ne portant cette id n'existe, des données
	 * vides sont retournées.
	 */
	DonneesStructure &donnees_structure(const long id);
	DonneesStructure const &donnees_structure(const long id) const;

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

	dls::dico_desordonne<dls::vue_chaine_compacte, DonneesStructure> structures{};

	/* gestion des membres actifs des unions :
	 * cas à considérer :
	 * -- les portées des variables
	 * -- les unions dans les structures (accès par '.')
	 */
	dls::vue_chaine_compacte trouve_membre_actif(dls::vue_chaine_compacte const &nom_union);

	void renseigne_membre_actif(dls::vue_chaine_compacte const &nom_union, dls::vue_chaine_compacte const &nom_membre);

private:
#ifdef AVEC_LLVM
	llvm::BasicBlock *m_bloc_courant = nullptr;
#endif
public:
	conteneur_globales globales{};
	dls::tableau<dls::vue_chaine_compacte> nom_structures{};

	conteneur_locales m_locales{};
	dls::pile<long> m_pile_nombre_locales{};
	long m_nombre_locales = 0;

	dls::pile<long> m_pile_nombre_differes{};
	long m_nombre_differes = 0;

#ifdef AVEC_LLVM
	using paire_bloc = std::pair<dls::vue_chaine_compacte, llvm::BasicBlock *>;

	dls::tableau<paire_bloc> m_pile_continue{};
	dls::tableau<paire_bloc> m_pile_arrete{};
#endif

	using paire_goto = std::pair<dls::vue_chaine_compacte, dls::chaine>;

	dls::tableau<paire_goto> m_pile_goto_continue{};
	dls::tableau<paire_goto> m_pile_goto_arrete{};

	dls::tableau<noeud::base *> m_noeuds_differes{};

	bool m_non_sur = false;

	using paire_union_membre = std::pair<dls::vue_chaine_compacte, dls::vue_chaine_compacte>;
	dls::tableau<paire_union_membre> membres_actifs{};

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;

	// pour les variables des boucles
	bool est_coulisse_llvm = false;
};
