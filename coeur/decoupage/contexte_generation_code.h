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

#include <llvm/IR/LLVMContext.h>

#include <stack>
#include <unordered_map>

#include "donnees_type.h"

struct TamponSource;

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

struct DonneesArgument {
	size_t index = 0;
	DonneesType donnees_type{};
};

struct DonneesFonction {
	std::unordered_map<std::string_view, DonneesArgument> args{};
	DonneesType donnees_type{};
};

struct DonneesVariable {
	llvm::Value *valeur;
	DonneesType donnees_type{};
};

struct Block {
	llvm::BasicBlock *block;
	std::unordered_map<std::string_view, DonneesVariable> locals;
};

struct DonneesStructure {
	std::unordered_map<std::string_view, size_t> index_membres;
	std::vector<DonneesType> donnees_types;
};

struct ContexteGenerationCode {
	const TamponSource &tampon;
	llvm::Module *module;
	llvm::LLVMContext contexte;

	ContexteGenerationCode(const TamponSource &tampon_source);

	/* ********************************************************************** */

	/**
	 * Pousse le block sur la pile de blocks. Ceci construit un Block qui
	 * contiendra le block LLVM passé en paramètre. Routes les locales poussées
	 * après un tel appel seront ajoutées à ce block.
	 */
	void pousse_block(llvm::BasicBlock *block);

	/**
	 * Enlève le block du haut de la pile de blocks.
	 */
	void jete_block();

	/**
	 * Retourne un pointeur vers le block LLVM du block courant.
	 */
	llvm::BasicBlock *block_courant() const;

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la globale dont le nom est spécifié en paramètres
	 * à la table de globales de ce contexte.
	 */
	void pousse_globale(const std::string_view &nom, llvm::Value *valeur, const DonneesType &type);

	/**
	 * Retourne un pointeur vers la valeur LLVM de la globale dont le nom est
	 * spécifié en paramètre. Si aucune globale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_globale(const std::string_view &nom);

	/**
	 * Retourne les données de la globale dont le nom est spécifié en
	 * paramètre. Si aucune globale ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	const DonneesType &type_globale(const std::string_view &nom);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la locale dont le nom est spécifié en paramètres
	 * à la table de locales du block courant de ce contexte.
	 */
	void pousse_locale(const std::string_view &nom, llvm::Value *valeur, const DonneesType &type);

	/**
	 * Retourne un pointeur vers la valeur LLVM de la locale dont le nom est
	 * spécifié en paramètre. Si aucune locale de ce nom n'existe, retourne
	 * nullptr.
	 */
	llvm::Value *valeur_locale(const std::string_view &nom);

	/**
	 * Retourne les données de la locale dont le nom est spécifié en paramètre.
	 * Si aucune locale ne portant ce nom n'existe, des données vides sont
	 * retournées.
	 */
	const DonneesType &type_locale(const std::string_view &nom);

	/* ********************************************************************** */

	/**
	 * Ajoute les données de la fonction dont le nom est spécifié en paramètres
	 * à la table de fonctions de ce contexte.
	 */
	void ajoute_donnees_fonctions(const std::string_view &nom, const DonneesFonction &donnees);

	/**
	 * Retourne les données de la fonction dont le nom est spécifié en
	 * paramètre. Si aucune fonction ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	const DonneesFonction &donnees_fonction(const std::string_view &nom);

	/**
	 * Retourne vrai si le nom spécifié en paramètre est celui d'une fonction
	 * ayant déjà été ajouté à la liste de fonctions de ce contexte.
	 */
	bool fonction_existe(const std::string_view &nom);

	/* ********************************************************************** */

	/**
	 * Retourne vrai si le nom spécifié en paramètre est celui d'une structure
	 * ayant déjà été ajouté à la liste de structures de ce contexte.
	 */
	bool structure_existe(const std::string_view &nom);

	/**
	 * Ajoute les données de la structure dont le nom est spécifié en paramètres
	 * à la table de structure de ce contexte.
	 */
	void ajoute_donnees_structure(const std::string_view &nom, const DonneesStructure &donnees);

	/**
	 * Retourne les données de la structure dont le nom est spécifié en
	 * paramètre. Si aucune structure ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	const DonneesStructure &donnees_structure(const std::string_view &nom);

private:
	std::stack<Block> pile_block;
	std::unordered_map<std::string_view, DonneesVariable> globales;
	std::unordered_map<std::string_view, DonneesFonction> fonctions;
	std::unordered_map<std::string_view, DonneesStructure> structures;

	/* Utilisé au cas où nous ne pouvons trouver une variable locale ou globale. */
	DonneesType m_donnees_type_invalide;
};
