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
	DEFERE,
	NONSUR,
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
	DYNAMIC  = (1 << 0),
	VARIADIC = (1 << 1),
	GLOBAL   = (1 << 2),
};

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class Noeud {
protected:
	std::list<Noeud *> m_enfants{};
	DonneesMorceaux const &m_donnees_morceaux;

public:
	std::any valeur_calculee{};

	size_t donnees_type = -1ul;

	bool calcule = false;
	char drapeaux = false;
	bool est_externe = false;
	char pad{};
	int module_appel{}; // module pour les appels de fonctions importées

	explicit Noeud(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

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
	Noeud *dernier_enfant() const;

	/**
	 * Performe la validation sémantique du noeud et de ses enfants.
	 */
	virtual void perfome_validation_semantique(ContexteGenerationCode &contexte);
};

/* ************************************************************************** */

class NoeudRacine final : public Noeud {
public:
	explicit NoeudRacine(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudAppelFonction final : public Noeud {
public:
	explicit NoeudAppelFonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	void ajoute_nom_argument(const std::string_view &nom);

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudDeclarationFonction final : public Noeud {
public:
	explicit NoeudDeclarationFonction(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudAssignationVariable final : public Noeud {
public:
	explicit NoeudAssignationVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudDeclarationVariable final : public Noeud {
public:
	explicit NoeudDeclarationVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudConstante final : public Noeud {
public:
	explicit NoeudConstante(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudNombreEntier final : public Noeud {
public:
	explicit NoeudNombreEntier(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudBooleen final : public Noeud {
public:
	explicit NoeudBooleen(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudCaractere final : public Noeud {
public:
	explicit NoeudCaractere(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudNombreReel final : public Noeud {
public:
	explicit NoeudNombreReel(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudChaineLitterale final : public Noeud {
public:
	explicit NoeudChaineLitterale(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	bool est_constant() const override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	explicit NoeudVariable(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudAccesMembre final : public Noeud {
public:
	explicit NoeudAccesMembre(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudOperationBinaire final : public Noeud {
public:
	explicit NoeudOperationBinaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	bool peut_etre_assigne(ContexteGenerationCode &contexte) const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudOperationUnaire final : public Noeud {
public:
	explicit NoeudOperationUnaire(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudRetour final : public Noeud {
public:
	explicit NoeudRetour(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudSi final : public Noeud {
public:
	explicit NoeudSi(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudBloc final : public Noeud {
public:
	explicit NoeudBloc(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudPour final : public Noeud {
public:
	explicit NoeudPour(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudContArr final : public Noeud {
public:
	explicit NoeudContArr(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudBoucle final : public Noeud {
public:
	explicit NoeudBoucle(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudTranstype final : public Noeud {
public:
	explicit NoeudTranstype(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudNul final : public Noeud {
public:
	explicit NoeudNul(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudTailleDe final : public Noeud {
public:
	explicit NoeudTailleDe(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudPlage final : public Noeud {
public:
	explicit NoeudPlage(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudAccesMembrePoint final : public Noeud {
public:
	explicit NoeudAccesMembrePoint(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};

/* ************************************************************************** */

class NoeudDefere final : public Noeud {
public:
	explicit NoeudDefere(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;
};

/* ************************************************************************** */

class NoeudNonSur final : public Noeud {
public:
	explicit NoeudNonSur(ContexteGenerationCode &contexte, DonneesMorceaux const &morceau);

	void imprime_code(std::ostream &os, int tab) override;

	llvm::Value *genere_code_llvm(ContexteGenerationCode &contexte, bool const expr_gauche = false) override;

	type_noeud type() const override;

	void perfome_validation_semantique(ContexteGenerationCode &contexte) override;
};
