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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <utility>

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"

#include "compilation/operateurs.hh"
#include "compilation/structures.hh"

#include "code_binaire.hh"

struct IdentifiantCode;
struct Instruction;
struct InstructionAccedeIndex;
struct InstructionAccedeMembre;
struct InstructionAllocation;
struct InstructionAppel;
struct InstructionBranche;
struct InstructionBrancheCondition;
struct InstructionChargeMem;
struct InstructionLabel;
struct InstructionOpBinaire;
struct InstructionOpUnaire;
struct InstructionRetour;
struct InstructionStockeMem;
struct InstructionTranstype;
struct Lexeme;
struct Type;

struct Atome {
	enum class Genre {
		CONSTANTE,
		FONCTION,
		INSTRUCTION,
		GLOBALE,
	};

	Type *type = nullptr;
	IdentifiantCode *ident = nullptr;

	Genre genre_atome{};
	// vrai si l'atome est celui d'une instruction chargeable
	bool est_chargeable = false;

	int nombre_utilisations = 0;

	// machine à état utilisée pour déterminer si un atome a été utilisé ou non
	int etat = 0;

	Atome() = default;

	COPIE_CONSTRUCT(Atome);

	inline Instruction *comme_instruction();
	inline Instruction const *comme_instruction() const;

	inline bool est_constante() const { return genre_atome == Genre::CONSTANTE; }
	inline bool est_fonction() const { return genre_atome == Genre::FONCTION; }
	inline bool est_globale() const { return genre_atome == Genre::GLOBALE; }
	inline bool est_instruction() const { return genre_atome == Genre::INSTRUCTION; }
};

struct AtomeConstante : public Atome {
	AtomeConstante() { genre_atome = Atome::Genre::CONSTANTE; }

	enum class Genre {
		GLOBALE,
		VALEUR,
		TRANSTYPE_CONSTANT,
		OP_BINAIRE_CONSTANTE,
		OP_UNAIRE_CONSTANTE,
		ACCES_INDEX_CONSTANT,
	};

	Genre genre{};
};

struct AtomeValeurConstante : public AtomeConstante {
	AtomeValeurConstante() { genre = Genre::VALEUR; }

	struct Valeur {
		union {
			unsigned long long valeur_entiere;
			double valeur_reelle;
			bool valeur_booleenne;
			struct { char *pointeur; long taille; } valeur_tdc;
			struct { AtomeConstante **pointeur; long taille; } valeur_structure;
			struct { AtomeConstante **pointeur; long taille; } valeur_tableau;
			Type *type;
		};

		enum class Genre {
			INDEFINIE,
			ENTIERE,
			REELLE,
			BOOLEENNE,
			NULLE,
			CARACTERE,
			STRUCTURE,
			TABLEAU_FIXE,
			TABLEAU_DONNEES_CONSTANTES,
			TYPE,
		};

		Genre genre{};

		~Valeur()
		{
			if (genre == Genre::STRUCTURE) {
				memoire::deloge_tableau("kuri::tableau", valeur_structure.pointeur, valeur_structure.taille);
			}

			if (genre == Genre::TABLEAU_FIXE) {
				memoire::deloge_tableau("kuri::tableau", valeur_tableau.pointeur, valeur_tableau.taille);
			}
		}
	};

	Valeur valeur{};

	AtomeValeurConstante(Type *type, unsigned long long valeur);
	AtomeValeurConstante(Type *type, double valeur);
	AtomeValeurConstante(Type *type, bool valeur);
	AtomeValeurConstante(Type *type);
	AtomeValeurConstante(Type *type, Type *pointeur_type);
	AtomeValeurConstante(Type *type, kuri::tableau<char> &&donnees_constantes);
	AtomeValeurConstante(Type *type, char *pointeur, long taille);
	AtomeValeurConstante(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
};

struct AtomeGlobale : public AtomeConstante {
	AtomeGlobale() { genre = Genre::GLOBALE; genre_atome = Atome::Genre::GLOBALE; est_chargeable = true; }

	AtomeConstante *initialisateur{};
	bool est_externe = false;
	bool est_constante = false;

	// index de la globale pour le code binaire
	int index = -1;

	COPIE_CONSTRUCT(AtomeGlobale);

	AtomeGlobale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante);
};

struct TranstypeConstant : public AtomeConstante {
	TranstypeConstant() { genre = Genre::TRANSTYPE_CONSTANT; }

	AtomeConstante *valeur = nullptr;

	COPIE_CONSTRUCT(TranstypeConstant);

	TranstypeConstant(Type *type, AtomeConstante *valeur);
};

struct OpBinaireConstant : public AtomeConstante {
	OpBinaireConstant() { genre = Genre::OP_BINAIRE_CONSTANTE; }

	OperateurBinaire::Genre op{};
	AtomeConstante *operande_gauche = nullptr;
	AtomeConstante *operande_droite = nullptr;

	COPIE_CONSTRUCT(OpBinaireConstant);

	OpBinaireConstant(Type *type, OperateurBinaire::Genre op, AtomeConstante *operande_gauche, AtomeConstante *operande_droite);
};

struct OpUnaireConstant : public AtomeConstante {
	OpUnaireConstant() { genre = Genre::OP_UNAIRE_CONSTANTE; }

	OperateurUnaire::Genre op{};
	AtomeConstante *operande = nullptr;

	COPIE_CONSTRUCT(OpUnaireConstant);

	OpUnaireConstant(Type *type, OperateurUnaire::Genre op, AtomeConstante *operande);
};

struct AccedeIndexConstant : public AtomeConstante {
	AccedeIndexConstant()
	{
		genre = Genre::ACCES_INDEX_CONSTANT;
	}

	AtomeConstante *accede = nullptr;
	AtomeConstante *index = nullptr;

	COPIE_CONSTRUCT(AccedeIndexConstant);

	AccedeIndexConstant(Type *type, AtomeConstante *accede, AtomeConstante *index);
};

struct AtomeFonction : public Atome {
	dls::chaine nom{};

	kuri::tableau<Atome *> params_entrees{};
	kuri::tableau<Atome *> params_sorties{};

	kuri::tableau<Instruction *> instructions{};

	/* pour les traces d'appels */
	Lexeme const *lexeme = nullptr;

	bool sanstrace = false;
	bool est_externe = false;
	NoeudDeclarationEnteteFonction const *decl = nullptr;

	// Pour les exécutions
	Chunk chunk{};

	long decalage_appel_init_globale = 0;

	struct DonneesFonctionExterne {
		dls::tablet<ffi_type *, 6> types_entrees{};
		ffi_cif cif{};
		void (*ptr_fonction)() = nullptr;
	};

	DonneesFonctionExterne donnees_externe{};

	AtomeFonction(Lexeme const *lexeme, dls::chaine const &nom);
	AtomeFonction(Lexeme const *lexeme, dls::chaine const &nom, kuri::tableau<Atome *> &&params);

	Instruction *derniere_instruction() const;

	COPIE_CONSTRUCT(AtomeFonction);
};

struct Instruction : public Atome {
	enum class Genre {
		INVALIDE,

		APPEL,
		ALLOCATION,
		OPERATION_BINAIRE,
		OPERATION_UNAIRE,
		CHARGE_MEMOIRE,
		STOCKE_MEMOIRE,
		LABEL,
		BRANCHE,
		BRANCHE_CONDITION,
		RETOUR,
		ACCEDE_MEMBRE,
		ACCEDE_INDEX,
		TRANSTYPE,
	};

	enum {
		SUPPRIME_INSTRUCTION = 1,
	};

	Genre genre = Genre::INVALIDE;
	int numero = 0;
	int drapeaux = 0;

	Instruction() { genre_atome = Atome::Genre::INSTRUCTION; }

#define COMME_INST(Type, Genre) \
	inline Type *comme_##Genre(); \
	inline Type const *comme_##Genre() const

	COMME_INST(InstructionAccedeIndex, acces_index);
	COMME_INST(InstructionAccedeMembre, acces_membre);
	COMME_INST(InstructionAllocation, alloc);
	COMME_INST(InstructionAppel, appel);
	COMME_INST(InstructionBranche, branche);
	COMME_INST(InstructionBrancheCondition, branche_cond);
	COMME_INST(InstructionChargeMem, charge);
	COMME_INST(InstructionLabel, label);
	COMME_INST(InstructionOpBinaire, op_binaire);
	COMME_INST(InstructionOpUnaire, op_unaire);
	COMME_INST(InstructionRetour, retour);
	COMME_INST(InstructionStockeMem, stocke_mem);
	COMME_INST(InstructionTranstype, transtype);

	inline bool est_acces_index() const { return genre == Genre::ACCEDE_INDEX; }
	inline bool est_acces_membre() const { return genre == Genre::ACCEDE_MEMBRE; }
	inline bool est_alloc() const { return genre == Genre::ALLOCATION; }
	inline bool est_appel() const { return genre == Genre::APPEL; }
	inline bool est_branche() const { return genre == Genre::BRANCHE; }
	inline bool est_branche_cond() const { return genre == Genre::BRANCHE_CONDITION; }
	inline bool est_charge() const { return genre == Genre::CHARGE_MEMOIRE; }
	inline bool est_label() const { return genre == Genre::LABEL; }
	inline bool est_op_binaire() const { return genre == Genre::OPERATION_BINAIRE; }
	inline bool est_op_unaire() const { return genre == Genre::OPERATION_UNAIRE; }
	inline bool est_retour() const { return genre == Genre::RETOUR; }
	inline bool est_stocke_mem() const { return genre == Genre::STOCKE_MEMOIRE; }
	inline bool est_transtype() const { return genre == Genre::TRANSTYPE; }

	inline bool est_branche_ou_retour() const { return est_branche() || est_branche_cond() || est_retour(); }

#undef COMME_INST
};

inline Instruction *Atome::comme_instruction()
{
	return static_cast<Instruction *>(this);
}

inline Instruction const *Atome::comme_instruction() const
{
	return static_cast<Instruction const *>(this);
}

struct InstructionAppel : public Instruction {
	InstructionAppel() { genre = Instruction::Genre::APPEL; }

	Atome *appele = nullptr;
	kuri::tableau<Atome *> args{};
	/* pour les traces d'appels */
	Lexeme const *lexeme = nullptr;

	InstructionAllocation *adresse_retour = nullptr;

	COPIE_CONSTRUCT(InstructionAppel);

	InstructionAppel(Lexeme const *lexeme, Atome *appele);
	InstructionAppel(Lexeme const *lexeme, Atome *appele, kuri::tableau<Atome *> &&args);
};

struct InstructionAllocation : public Instruction {
	InstructionAllocation()
	{
		genre = Instruction::Genre::ALLOCATION;
		est_chargeable = true;
	}

	// pour la génération de code binaire, mise en place lors de la génération de celle-ci
	int index_locale = 0;
	int profondeur_bloc = 0;

	InstructionAllocation(Type *type, IdentifiantCode *ident);
};

struct InstructionRetour : public Instruction {
	InstructionRetour() { genre = Instruction::Genre::RETOUR; }

	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionRetour);

	InstructionRetour(Atome *valeur);
};

struct InstructionOpBinaire : public Instruction {
	InstructionOpBinaire() { genre = Instruction::Genre::OPERATION_BINAIRE; }

	OperateurBinaire::Genre op{};
	Atome *valeur_gauche = nullptr;
	Atome *valeur_droite = nullptr;

	COPIE_CONSTRUCT(InstructionOpBinaire);

	InstructionOpBinaire(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite);
};

struct InstructionOpUnaire : public Instruction {
	InstructionOpUnaire() { genre = Instruction::Genre::OPERATION_UNAIRE; }

	OperateurUnaire::Genre op{};
	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionOpUnaire);

	InstructionOpUnaire(Type *type, OperateurUnaire::Genre op, Atome *valeur);
};

struct InstructionChargeMem : public Instruction {
	InstructionChargeMem()
	{
		genre = Instruction::Genre::CHARGE_MEMOIRE;
		est_chargeable = true;
	}

	Atome *chargee = nullptr;

	COPIE_CONSTRUCT(InstructionChargeMem);

	InstructionChargeMem(Type *type, Atome *chargee);
};

struct InstructionStockeMem : public Instruction {
	InstructionStockeMem() { genre = Instruction::Genre::STOCKE_MEMOIRE; }

	Atome *ou = nullptr;
	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionStockeMem);

	InstructionStockeMem(Type *type, Atome *ou, Atome *valeur);
};

struct InstructionLabel : public Instruction {
	InstructionLabel() { genre = Instruction::Genre::LABEL; }

	int id = 0;

	InstructionLabel(int id);
};

struct InstructionBranche : public Instruction {
	InstructionBranche() { genre = Instruction::Genre::BRANCHE; }

	InstructionLabel *label = nullptr;

	COPIE_CONSTRUCT(InstructionBranche);

	InstructionBranche(InstructionLabel *label);
};

struct InstructionBrancheCondition : public Instruction {
	InstructionBrancheCondition() { genre = Instruction::Genre::BRANCHE_CONDITION; }

	Atome *condition = nullptr;
	InstructionLabel *label_si_vrai = nullptr;
	InstructionLabel *label_si_faux = nullptr;

	COPIE_CONSTRUCT(InstructionBrancheCondition);

	InstructionBrancheCondition(Atome *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
};

struct InstructionAccedeMembre : public Instruction {
	InstructionAccedeMembre()
	{
		genre = Instruction::Genre::ACCEDE_MEMBRE;
		est_chargeable = true;
	}

	Atome *accede = nullptr;
	Atome *index = nullptr;

	COPIE_CONSTRUCT(InstructionAccedeMembre);

	InstructionAccedeMembre(Type *type, Atome *accede, Atome *index);
};

struct InstructionAccedeIndex : public Instruction {
	InstructionAccedeIndex()
	{
		genre = Instruction::Genre::ACCEDE_INDEX;
		est_chargeable = true;
	}

	Atome *accede = nullptr;
	Atome *index = nullptr;

	COPIE_CONSTRUCT(InstructionAccedeIndex);

	InstructionAccedeIndex(Type *type, Atome *accede, Atome *index);
};

enum TypeTranstypage {
	AUGMENTE_NATUREL,
	AUGMENTE_RELATIF,
	AUGMENTE_REEL,
	DIMINUE_NATUREL,
	DIMINUE_RELATIF,
	DIMINUE_REEL,
	POINTEUR_VERS_ENTIER,
	ENTIER_VERS_POINTEUR,
	REEL_VERS_ENTIER,
	ENTIER_VERS_REEL,
	BITS,
	DEFAUT, // À SUPPRIMER
};

struct InstructionTranstype : public Instruction {
	InstructionTranstype()
	{
		genre = Instruction::Genre::TRANSTYPE;
		est_chargeable = false; // À FAIRE : uniquement si la valeur est un pointeur
	}

	Atome *valeur = nullptr;
	TypeTranstypage op{};

	COPIE_CONSTRUCT(InstructionTranstype);

	InstructionTranstype(Type *type, Atome *valeur, TypeTranstypage op_);
};

#define COMME_INST(Type, Genre) \
	inline Type *Instruction::comme_##Genre() \
	{ \
		assert(est_##Genre()); \
		return static_cast<Type *>(this); \
	} \
	inline Type const *Instruction::comme_##Genre() const \
	{ \
		assert(est_##Genre()); \
		return static_cast<Type const *>(this); \
	}

	COMME_INST(InstructionAccedeIndex, acces_index)
	COMME_INST(InstructionAccedeMembre, acces_membre)
	COMME_INST(InstructionAllocation, alloc)
	COMME_INST(InstructionAppel, appel)
	COMME_INST(InstructionBranche, branche)
	COMME_INST(InstructionBrancheCondition, branche_cond)
	COMME_INST(InstructionChargeMem, charge)
	COMME_INST(InstructionLabel, label)
	COMME_INST(InstructionOpBinaire, op_binaire)
	COMME_INST(InstructionOpUnaire, op_unaire)
	COMME_INST(InstructionRetour, retour)
	COMME_INST(InstructionStockeMem, stocke_mem)
	COMME_INST(InstructionTranstype, transtype)

#undef COMME_INST
