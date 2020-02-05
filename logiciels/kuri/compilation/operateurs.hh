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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

#include <llvm/IR/InstrTypes.h>

#include "transformation_type.hh"

enum class TypeLexeme : unsigned int;
struct ContexteGenerationCode;

enum class IndiceTypeOp {
	ENTIER_NATUREL,
	ENTIER_RELATIF,
	REEL,
};

enum class RaisonOp {
	POUR_COMPARAISON,
	POUR_ARITHMETIQUE,
};

struct DonneesOperateur {
	long index_type1{};
	long index_type2{};
	long index_resultat{};

	/* vrai si l'on peut sainement inverser les paramètres,
	 * vrai pour : +, *, !=, == */
	bool est_commutatif = false;

	bool inverse_parametres = false;

	/* faux pour les opérateurs définis par l'utilisateur */
	bool est_basique = true;

	bool est_comp_entier = false;
	bool est_comp_reel = false;

	llvm::Instruction::BinaryOps instr_llvm{};
	llvm::CmpInst::Predicate predicat_llvm{};

	dls::chaine nom_fonction = "";
};

struct Operateurs {
	using type_conteneur = dls::tableau<DonneesOperateur *>;

	dls::dico_desordonne<TypeLexeme, type_conteneur> donnees_operateurs;

	long type_bool = 0;

	~Operateurs();

	type_conteneur const &trouve(TypeLexeme id) const;

	void ajoute_basique(TypeLexeme id, long index_type, long index_type_resultat, IndiceTypeOp indice_type, RaisonOp raison);
	void ajoute_basique(TypeLexeme id, long index_type1, long index_type2, long index_type_resultat, IndiceTypeOp indice_type, RaisonOp raison);

	void ajoute_basique_unaire(TypeLexeme id, long index_type, long index_type_resultat);

	void ajoute_perso(TypeLexeme id, long index_type1, long index_type2, long index_type_resultat, dls::chaine const &nom_fonction);

	void ajoute_perso_unaire(TypeLexeme id, long index_type, long index_type_resultat, dls::chaine const &nom_fonction);

	void ajoute_operateur_basique_enum(long index_type);
};

DonneesOperateur const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		long index_type1,
		TypeLexeme type_op);

void enregistre_operateurs_basiques(
	ContexteGenerationCode &contexte,
	Operateurs &operateurs);

struct OperateurCandidat {
	DonneesOperateur const *op = nullptr;
	TransformationType transformation_type1{};
	TransformationType transformation_type2{};
	double poids = 0.0;
	bool inverse_operandes = false;

	COPIE_CONSTRUCT(OperateurCandidat);
};

dls::tableau<OperateurCandidat> cherche_candidats_operateurs(
		ContexteGenerationCode const &contexte,
		long index_type1,
		long index_type2,
		TypeLexeme type_op);
