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

#include <vector>

#include "donnees_type.h"
#include "morceaux.h"

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

struct ContexteGenerationCode;

enum {
	NOEUD_RACINE,
	NOEUD_DECLARATION_FONCTION,
	NOEUD_APPEL_FONCTION,
	NOEUD_EXPRESSION,
	NOEUD_VARIABLE,
	NOEUD_CONSTANTE,
	NOEUD_ASSIGNATION_VARIABLE,
	NOEUD_NOMBRE_REEL,
	NOEUD_NOMBRE_ENTIER,
	NOEUD_OPERATION,
	NOEUD_RETOUR,
	NOEUD_CHAINE_LITTERALE,
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
	DonneesType donnees_type{};
};

/* ************************************************************************** */

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class Noeud {
protected:
	std::vector<Noeud *> m_enfants;

	DonneesMorceaux m_donnees_morceaux;

public:
	union {
		double valeur_reelle;
		long valeur_entiere;
		bool valeur_boolenne;
	};

	DonneesType donnees_type{};

	bool calcule = false;
	char pad[7];

	explicit Noeud(const DonneesMorceaux &morceau);

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
	virtual const DonneesType &calcul_type(ContexteGenerationCode &contexte);

	/**
	 * Retourne l'identifiant du morceau de ce noeud.
	 */
	int identifiant() const;
	void reserve_enfants(size_t n);

	/**
	 * Retourne vrai si le résultat du noeud peut-être évalué durant la
	 * compilation.
	 */
	virtual bool est_constant() const;

	/**
	 * Retourne une référence constante vers la chaine du morceau de ce noeud.
	 */
	const std::string_view &chaine() const;

	/**
	 * Retourne le type syntactic de noeud.
	 */
	virtual int type_noeud() const = 0;
};

/* ************************************************************************** */

class NoeudRacine final : public Noeud {
public:
	explicit NoeudRacine(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudAppelFonction final : public Noeud {
	/* les noms des arguments s'il sont nommés */
	std::vector<std::string_view> m_noms_arguments;

public:
	explicit NoeudAppelFonction(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	void ajoute_nom_argument(const std::string_view &nom);

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudDeclarationFonction final : public Noeud {
	std::vector<ArgumentFonction> m_arguments;

public:
	int type_retour = -1;
	int pad1;

	explicit NoeudDeclarationFonction(const DonneesMorceaux &morceau);

	void ajoute_argument(const ArgumentFonction &argument);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudExpression final : public Noeud {
public:
	explicit NoeudExpression(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudAssignationVariable final : public Noeud {
public:
	explicit NoeudAssignationVariable(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudConstante final : public Noeud {
public:
	explicit NoeudConstante(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudNombreEntier final : public Noeud {
public:
	explicit NoeudNombreEntier(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	bool est_constant() const override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudNombreReel final : public Noeud {
public:
	explicit NoeudNombreReel(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	bool est_constant() const override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudChaineLitterale final : public Noeud {
public:
	explicit NoeudChaineLitterale(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	explicit NoeudVariable(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudOperation final : public Noeud {
public:
	explicit NoeudOperation(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};

/* ************************************************************************** */

class NoeudRetour final : public Noeud {
public:
	explicit NoeudRetour(const DonneesMorceaux &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte) override;

	const DonneesType &calcul_type(ContexteGenerationCode &contexte) override;

	int type_noeud() const override;
};
