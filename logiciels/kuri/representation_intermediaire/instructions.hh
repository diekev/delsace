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

#include "operateurs.hh"
#include "structures.hh"

struct IdentifiantCode;
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

	Atome() = default;

	COPIE_CONSTRUCT(Atome);
};

struct AtomeConstante : public Atome {
	AtomeConstante() { genre_atome = Atome::Genre::CONSTANTE; }

	struct Valeur {
		union {
			unsigned long long valeur_entiere;
			double valeur_reelle;
			bool valeur_booleenne;
			struct { char *pointeur; long taille; } valeur_chaine;
			struct { AtomeConstante **pointeur; long taille; } valeur_structure;
		};

		enum class Genre {
			INDEFINIE,
			ENTIERE,
			REELLE,
			BOOLEENNE,
			NULLE,
			CHAINE,
			CARACTERE,
			STRUCTURE,
		};

		Genre genre{};

		~Valeur()
		{
			if (genre == Genre::STRUCTURE) {
				memoire::deloge_tableau("kuri::tableau", valeur_structure.pointeur, valeur_structure.taille);
			}
		}
	};

	Valeur valeur{};

	static AtomeConstante *cree(Type *type, unsigned long long valeur);
	static AtomeConstante *cree(Type *type, double valeur);
	static AtomeConstante *cree(Type *type, bool valeur);
	static AtomeConstante *cree(Type *type);
	static AtomeConstante *cree(Type *type, kuri::chaine const &chaine);
	static AtomeConstante *cree(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
};

struct AtomeGlobale : public Atome {
	AtomeGlobale() { genre_atome = Atome::Genre::GLOBALE; }

	AtomeConstante *initialisateur{};

	COPIE_CONSTRUCT(AtomeGlobale);

	static AtomeGlobale *cree(Type *type, AtomeConstante *initialisateur);
};

struct AtomeFonction : public Atome {
	dls::chaine nom{};

	kuri::tableau<Atome *> params_entrees{};
	kuri::tableau<Atome *> params_sorties{};

	kuri::tableau<Atome *> instructions{};

	static AtomeFonction *cree(dls::chaine const &nom, kuri::tableau<Atome *> &&params);
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

	Genre genre = Genre::INVALIDE;
	int numero = 0;

	Instruction() { genre_atome = Atome::Genre::INSTRUCTION; }
};

struct InstructionAppel : public Instruction {
	InstructionAppel() { genre = Instruction::Genre::APPEL; }

	Atome *appele = nullptr;
	kuri::tableau<Atome *> args{};

	COPIE_CONSTRUCT(InstructionAppel);

	static InstructionAppel *cree(Type *type, Atome *appele, kuri::tableau<Atome *> &&args);
};

struct InstructionAllocation : public Instruction {
	InstructionAllocation() { genre = Instruction::Genre::ALLOCATION; }

	static InstructionAllocation *cree(Type *type, IdentifiantCode *ident);
};

struct InstructionRetour : public Instruction {
	InstructionRetour() { genre = Instruction::Genre::RETOUR; }

	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionRetour);

	static InstructionRetour *cree(Atome *valeur);
};

struct InstructionOpBinaire : public Instruction {
	InstructionOpBinaire() { genre = Instruction::Genre::OPERATION_BINAIRE; }

	OperateurBinaire::Genre op{};
	Atome *valeur_gauche = nullptr;
	Atome *valeur_droite = nullptr;

	COPIE_CONSTRUCT(InstructionOpBinaire);

	static InstructionOpBinaire *cree(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite);
};

struct InstructionOpUnaire : public Instruction {
	InstructionOpUnaire() { genre = Instruction::Genre::OPERATION_UNAIRE; }

	OperateurUnaire::Genre op{};
	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionOpUnaire);

	static InstructionOpUnaire *cree(Type *type, OperateurUnaire::Genre op, Atome *valeur);
};

struct InstructionChargeMem : public Instruction {
	InstructionChargeMem() { genre = Instruction::Genre::CHARGE_MEMOIRE; }

	Instruction *inst_chargee = nullptr;

	COPIE_CONSTRUCT(InstructionChargeMem);

	static InstructionChargeMem *cree(Type *type, Instruction *inst_chargee);
};

struct InstructionStockeMem : public Instruction {
	InstructionStockeMem() { genre = Instruction::Genre::STOCKE_MEMOIRE; }

	Instruction *ou = nullptr;
	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionStockeMem);

	static InstructionStockeMem *cree(Type *type, Instruction *ou, Atome *valeur);
};

struct InstructionLabel : public Instruction {
	InstructionLabel() { genre = Instruction::Genre::LABEL; }

	int id = 0;

	static InstructionLabel *cree(int id);
};

struct InstructionBranche : public Instruction {
	InstructionBranche() { genre = Instruction::Genre::BRANCHE; }

	InstructionLabel *label = nullptr;

	COPIE_CONSTRUCT(InstructionBranche);

	static InstructionBranche *cree(InstructionLabel *label);
};

struct InstructionBrancheCondition : public Instruction {
	InstructionBrancheCondition() { genre = Instruction::Genre::BRANCHE_CONDITION; }

	Atome *condition = nullptr;
	InstructionLabel *label_si_vrai = nullptr;
	InstructionLabel *label_si_faux = nullptr;

	COPIE_CONSTRUCT(InstructionBrancheCondition);

	static InstructionBrancheCondition *cree(Atome *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
};

struct InstructionAccedeMembre : public Instruction {
	InstructionAccedeMembre() { genre = Instruction::Genre::ACCEDE_MEMBRE; }

	Atome *accede = nullptr;
	Atome *index = nullptr;

	COPIE_CONSTRUCT(InstructionAccedeMembre);

	static InstructionAccedeMembre *cree(Type *type, Atome *accede, Atome *index);
};

struct InstructionAccedeIndex : public Instruction {
	InstructionAccedeIndex() { genre = Instruction::Genre::ACCEDE_INDEX; }

	Atome *accede = nullptr;
	Atome *index = nullptr;

	COPIE_CONSTRUCT(InstructionAccedeIndex);

	static InstructionAccedeIndex *cree(Type *type, Atome *accede, Atome *index);
};

struct InstructionTranstype : public Instruction {
	InstructionTranstype() { genre = Instruction::Genre::TRANSTYPE; }

	Atome *valeur = nullptr;

	COPIE_CONSTRUCT(InstructionTranstype);

	static InstructionTranstype *cree(Type *type, Atome *valeur);
};
