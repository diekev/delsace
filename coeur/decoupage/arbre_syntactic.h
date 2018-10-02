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

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/LLVMContext.h>

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

enum {
	NOEUD_RACINE,
	NOEUD_DECLARATION_FONCTION,
	NOEUD_APPEL_FONCTION,
	NOEUD_EXPRESSION,
	NOEUD_VARIABLE,
	NOEUD_ASSIGNATION_VARIABLE,
	NOEUD_NOMBRE_REEL,
	NOEUD_NOMBRE_ENTIER,
	NOEUD_OPERATION,
	NOEUD_RETOUR,
};

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
 * noeud déclaration fonction : multiples enfants de types différents
 * -- déclaration variable / expression / retour / controle flux
 *
 * noeud appel fonction : multiples enfants de mêmes types
 * -- noeud expression
 *
 * noeud déclaration variable : un seul enfant
 * -- noeud expression
 *
 * noeud retour : un seul enfant
 * -- noeud expression
 *
 * noeud opérateur : 1 ou 2 enfants de même type
 * -- noeud expression
 *
 * noeud expression : un seul enfant, peut utiliser une énumeration pour choisir
 *                    le bon noeud
 * -- noeud (variable | opérateur | nombre entier | nombre réel | appel fonction)
 *
 * noeud variable : aucun enfant
 * noeud nombre entier : aucun enfant
 * noeud nombre réel : aucun enfant
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

struct ArgumentFonction {
	std::string chaine;
	int id_type;
};

struct DonneesArgument {
	size_t index;
	int type;
};

struct DonneesFonction {
	std::unordered_map<std::string, DonneesArgument> args;
	int type_retour;
	int pad;
};

struct DonneesVariable {
	llvm::Value *valeur;
	int type;
	int pad;
};

struct Block {
	llvm::BasicBlock *block;
	std::unordered_map<std::string, DonneesVariable> locals;
};

struct ContexteGenerationCode {
	llvm::Module *module;
	llvm::LLVMContext contexte;

	void pousse_block(llvm::BasicBlock *block);

	void jete_block();

	llvm::BasicBlock *block_courant() const;

	void pousse_locale(const std::string &nom, llvm::Value *valeur, int type);

	llvm::Value *valeur_locale(const std::string &nom);

	int type_locale(const std::string &nom);

	void ajoute_donnees_fonctions(const std::string &nom, const DonneesFonction &donnees);

	DonneesFonction donnees_fonction(const std::string &nom);

private:
	std::stack<Block> pile_block;
	std::unordered_map<std::string, DonneesFonction> fonctions;
};

/* ************************************************************************** */

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class Noeud {
protected:
	std::string m_chaine;
	std::vector<Noeud *> m_enfants;

public:
	int identifiant;
	int type = -1;

	Noeud(const std::string &chaine, int id);

	virtual ~Noeud() = default;

	/**
	 * Ajoute un noeud à la liste des noeuds du noeud.
	 */
	void ajoute_noeud(Noeud *noeud);

	/**
	 * Imprime le 'code' de ce noeud dans le flux de sortie 'os' précisé. C'est
	 * attendu que le noeud demande à ces enfants d'imprimer leurs 'codes' dans
	 * le bon ordre.
	 */
	virtual void imprime_code(std::ostream &os, int tab) = 0;

	/**
	 * Génère le code pour LLVM.
	 */
	virtual llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) = 0;

	/**
	 * Calcul le type de ce noeud en cherchant parmis ses enfants si nécessaire.
	 */
	virtual int calcul_type(ContexteGenerationCode &contexte);
};

/* ************************************************************************** */

class NoeudRacine final : public Noeud {
public:
	NoeudRacine(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudAppelFonction final : public Noeud {
	/* les noms des arguments s'il sont nommés */
	std::vector<std::string> m_noms_arguments;

public:
	NoeudAppelFonction(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;

	void ajoute_nom_argument(const std::string &nom);
};

/* ************************************************************************** */

class NoeudDeclarationFonction final : public Noeud {
	std::vector<ArgumentFonction> m_arguments;

public:
	int type_retour = -1;

	NoeudDeclarationFonction(const std::string &chaine, int id);

	void ajoute_argument(const ArgumentFonction &argument);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudExpression final : public Noeud {
public:
	NoeudExpression(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudAssignationVariable final : public Noeud {
public:
	NoeudAssignationVariable(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudNombreEntier final : public Noeud {
public:
	NoeudNombreEntier(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudNombreReel final : public Noeud {
public:
	NoeudNombreReel(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	NoeudVariable(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudOperation final : public Noeud {
public:
	NoeudOperation(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudRetour final : public Noeud {
public:
	NoeudRetour(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int calcul_type(ContexteGenerationCode &contexte) override;
};
