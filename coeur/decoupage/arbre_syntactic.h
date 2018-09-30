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
#include <string>
#include <vector>

#include <llvm/IR/LLVMContext.h>

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

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class Noeud {
protected:
	std::string m_chaine;
	std::vector<Noeud *> m_enfants;

public:
	int identifiant;

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
	virtual void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) = 0;
};

/* ************************************************************************** */

class NoeudRacine final : public Noeud {
public:
	NoeudRacine(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudAppelFonction final : public Noeud {
public:
	NoeudAppelFonction(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudDeclarationFonction final : public Noeud {
public:
	NoeudDeclarationFonction(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudExpression final : public Noeud {
public:
	NoeudExpression(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudAssignationVariable final : public Noeud {
public:
	NoeudAssignationVariable(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudNombreEntier final : public Noeud {
public:
	NoeudNombreEntier(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudNombreReel final : public Noeud {
public:
	NoeudNombreReel(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	NoeudVariable(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudOperation final : public Noeud {
public:
	NoeudOperation(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};

/* ************************************************************************** */

class NoeudRetour final : public Noeud {
public:
	NoeudRetour(const std::string &chaine, int id);

	void imprime_code(std::ostream &os, int tab) override;

	void genere_code_llvm(llvm::LLVMContext &contexte, llvm::Module *module) override;
};
