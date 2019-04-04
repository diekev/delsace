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

#include <any>
#include <list>

#include "donnees_type.h"
#include "morceaux.h"

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

struct ContexteGenerationCode;

enum class type_noeud : char {
	RACINE,
	DECLARATION_FONCTION,
	APPEL_FONCTION,
	VARIABLE,
	ACCES_MEMBRE,
	ACCES_MEMBRE_POINT,
	CONSTANTE,
	DECLARATION_VARIABLE,
	ASSIGNATION_VARIABLE,
	NOMBRE_REEL,
	NOMBRE_ENTIER,
	OPERATION_BINAIRE,
	OPERATION_UNAIRE,
	RETOUR,
	CHAINE_LITTERALE,
	BOOLEEN,
	CARACTERE,
	SI,
	BLOC,
	POUR,
	CONTINUE_ARRETE,
	BOUCLE,
	TRANSTYPE,
	NUL,
	TAILLE_DE,
	PLAGE,
	DIFFERE,
	NONSUR,
	TABLEAU,
};

const char *chaine_type_noeud(type_noeud type);

/* ************************************************************************** */

/* Idée pour un réusinage du code pour supprimer les tables virtuelles des
 * noeuds. Ces tables virtuelles doivent à chaque fois être résolues ce qui
 * nous fait perdre du temps. Au lieu d'avoir un système d'héritage, nous
 * pourrions avoir un système plus manuel selon les observations suivantes :
 *
 * noeud racine : multiples enfants pouvant être dans des tableaux différents
 * -- noeud déclaration fonction
 * -- noeud déclaration structure
 * -- noeud déclaration énum
 *
 * noeud déclaration fonction : un seul enfant
 * -- noeud bloc
 *
 * noeud appel fonction : multiples enfants de mêmes types
 * -- noeud expression
 *
 * noeud assignation variable : deux enfants
 * -- noeud expression | noeud déclaration variable
 * -- noeud expression
 *
 * noeud retour : un seul enfant
 * -- noeud expression
 *
 * noeud opérateur binaire : 2 enfants de même type
 * -- noeud expression
 * -- noeud expression
 *
 * noeud opérateur binaire : un seul enfant
 * -- noeud expression
 *
 * noeud expression : un seul enfant, peut utiliser une énumeration pour choisir
 *                    le bon noeud
 * -- noeud (variable | opérateur | nombre entier | nombre réel | appel fonction)
 *
 * noeud accès membre : deux enfants de même type
 * -- noeud variable
 *
 * noeud boucle : un seul enfant
 * -- noeud bloc
 *
 * noeud pour : 4 enfants
 * -- noeud variable
 * -- noeud expression
 * -- noeud expression
 * -- noeud bloc
 *
 * noeud bloc : multiples enfants de types différents
 * -- déclaration variable / expression / retour / boucle pour
 *
 * noeud si : 2 ou 3 enfants
 * -- noeud expression
 * -- noeud bloc
 * -- noeud si | noeud bloc
 *
 * noeud déclaration variable : aucun enfant
 * noeud variable : aucun enfant
 * noeud nombre entier : aucun enfant
 * noeud nombre réel : aucun enfant
 * noeud booléen : aucun enfant
 * noeud chaine caractère : aucun enfant
 * noeud continue_arrête : aucun enfant
 * noeud pointeur nul : aucun enfant
 *
 * Le seul type de neoud posant problème est le noeud de déclaration de
 * fonction, mais nous pourrions avoir des tableaux séparés avec une structure
 * de données pour définir l'ordre d'apparition des noeuds des tableaux dans la
 * fonction. Tous les autres types de noeuds ont des enfants bien défini, donc
 * nous pourrions peut-être supprimer l'héritage, tout en forçant une interface
 * commune à tous les noeuds.
 *
 * Mais pour tester ce réusinage, ce vaudrait bien essayer d'attendre que le
 * langage soit un peu mieux défini.
 */

/* ************************************************************************** */

enum {
	DYNAMIC          = (1 << 0),
	VARIADIC         = (1 << 1),
	GLOBAL           = (1 << 2),
	CONVERTI_TABLEAU = (1 << 3),
};

namespace noeud {

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class base {
protected:
	std::list<base *> m_enfants{};
	DonneesMorceaux const &m_donnees_morceaux;

public:
	std::any valeur_calculee{};

	size_t donnees_type = -1ul;

	bool calcule = false;
	char drapeaux = false;
	bool est_externe = false;
	char pad{};
	int module_appel{}; // module pour les appels de fonctions importées

	explicit base(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	virtual ~base() = default;

	/**
	 * Ajoute un noeud à la liste des noeuds du noeud.
	 */
	void ajoute_noeud(base *noeud);

	/**
	 * Imprime le 'code' de ce noeud dans le flux de sortie 'os' précisé. C'est
	 * attendu que le noeud demande à ces enfants d'imprimer leurs 'codes' dans
	 * le bon ordre.
	 */
	void imprime_code(std::ostream &os, int tab);

	/**
	 * Génère le code pour LLVM.
	 */
	virtual llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) = 0;

	/**
	 * Retourne l'identifiant du morceau de ce noeud.
	 */
	id_morceau identifiant() const;

	/**
	 * Retourne vrai si le résultat du noeud peut-être évalué durant la
	 * compilation.
	 */
	virtual bool est_constant() const;

	/**
	 * Retourne une référence constante vers la chaine du morceau de ce noeud.
	 */
	std::string_view const &chaine() const;

	/**
	 * Retourne le type syntactic de noeud.
	 */
	virtual type_noeud type() const = 0;

	/**
	 * Retourne vrai si le noeud peut se trouver à gauche d'un opérateur '='.
	 */
	virtual bool peut_etre_assigne(ContexteGenerationCode &contexte) const;

	/**
	 * Retourne une référence constante vers les données du morceau de ce neoud.
	 */
	DonneesMorceaux const &donnees_morceau() const;

	/**
	 * Retourne un pointeur vers le dernier enfant de ce noeud. Si le noeud n'a
	 * aucun enfant, retourne nullptr.
	 */
	base *dernier_enfant() const;

	/**
	 * Performe la validation sémantique du noeud et de ses enfants.
	 */
	virtual void perfome_validation_semantique(ContexteGenerationCode &contexte);
};

/* ************************************************************************** */

class racine final : public base {
public:
	explicit racine(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class appel_fonction final : public base {
public:
	explicit appel_fonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	void ajoute_nom_argument(const std::string_view &nom);

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;

private:
	   void verifie_compatibilite(
			   ContexteGenerationCode &contexte,
			   DonneesType const &type_arg,
			   DonneesType const &type_enf,
			   base *enfant);
};

/* ************************************************************************** */

class declaration_fonction final : public base {
public:
	explicit declaration_fonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class assignation_variable final : public base {
public:
	explicit assignation_variable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class declaration_variable final : public base {
public:
	explicit declaration_variable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class constante final : public base {
public:
	explicit constante(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class nombre_entier final : public base {
public:
	explicit nombre_entier(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class booleen final : public base {
public:
	explicit booleen(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class caractere final : public base {
public:
	explicit caractere(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class nombre_reel final : public base {
public:
	explicit nombre_reel(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class chaine_litterale final : public base {
public:
	explicit chaine_litterale(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class tableau final : public base {
public:
	explicit tableau(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class variable final : public base {
public:
	explicit variable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class acces_membre_de final : public base {
public:
	explicit acces_membre_de(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class operation_binaire final : public base {
public:
	explicit operation_binaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class operation_unaire final : public base {
public:
	explicit operation_unaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class retourne final : public base {
public:
	explicit retourne(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class si final : public base {
public:
	explicit si(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class bloc final : public base {
public:
	explicit bloc(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class pour final : public base {
public:
	explicit pour(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class cont_arr final : public base {
public:
	explicit cont_arr(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class boucle final : public base {
public:
	explicit boucle(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class transtype final : public base {
public:
	explicit transtype(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class nul final : public base {
public:
	explicit nul(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class taille_de final : public base {
public:
	explicit taille_de(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class plage final : public base {
public:
	explicit plage(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class acces_membre_point final : public base {
public:
	explicit acces_membre_point(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class differe final : public base {
public:
	explicit differe(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class non_sur final : public base {
public:
	explicit non_sur(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

}  /* namespace noeud */
