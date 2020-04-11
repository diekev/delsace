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
#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/structures/tablet.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/InstrTypes.h>
#pragma GCC diagnostic pop

#include "transformation_type.hh"

enum class GenreLexeme : unsigned int;
struct ContexteGenerationCode;
struct Type;

enum class IndiceTypeOp {
	ENTIER_NATUREL,
	ENTIER_RELATIF,
	REEL,
};

enum class RaisonOp {
	POUR_COMPARAISON,
	POUR_ARITHMETIQUE,
};

struct OperateurUnaire {
	enum class Genre : char {
		Invalide,

		Positif,
		Complement,
		Non_Logique,
		Non_Binaire,
		Prise_Adresse,
	};

	Type *type_operande = nullptr;
	Type *type_resultat = nullptr;

	Genre genre{};
	bool est_basique = true;
	dls::chaine nom_fonction = "";
};

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre);

struct OperateurBinaire {
	enum class Genre : char {
		Invalide,

		Addition,
		Soustraction,
		Multiplication,
		Division,
		Reste,

		Comp_Egal,
		Comp_Inegal,
		Comp_Inf,
		Comp_Inf_Egal,
		Comp_Sup,
		Comp_Sup_Egal,

		Et_Logique,
		Ou_Logique,

		Et_Binaire,
		Ou_Binaire,
		Ou_Exclusif,

		Dec_Gauche,
		Dec_Droite,
	};

	Type *type1{};
	Type *type2{};
	Type *type_resultat{};

	Genre genre{};

	/* vrai si l'on peut sainement inverser les paramètres,
	 * vrai pour : +, *, !=, == */
	bool est_commutatif = false;

	/* faux pour les opérateurs définis par l'utilisateur */
	bool est_basique = true;

	bool est_comp_entier = false;
	bool est_comp_reel = false;

	llvm::Instruction::BinaryOps instr_llvm{};
	llvm::CmpInst::Predicate predicat_llvm{};

	dls::chaine nom_fonction = "";
};

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre);

struct Operateurs {
	using type_conteneur_binaire = tableau_page<OperateurBinaire>;
	using type_conteneur_unaire = tableau_page<OperateurUnaire>;

	dls::dico_desordonne<GenreLexeme, type_conteneur_binaire> operateurs_binaires;
	dls::dico_desordonne<GenreLexeme, type_conteneur_unaire> operateurs_unaires;

	Type *type_bool = nullptr;

	~Operateurs();

	type_conteneur_binaire const &trouve_binaire(GenreLexeme id) const;
	type_conteneur_unaire const &trouve_unaire(GenreLexeme id) const;

	void ajoute_basique(GenreLexeme id, Type *type, Type *type_resultat, IndiceTypeOp indice_type, RaisonOp raison);
	void ajoute_basique(GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, IndiceTypeOp indice_type, RaisonOp raison);

	void ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat);

	void ajoute_perso(GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, dls::chaine const &nom_fonction);

	void ajoute_perso_unaire(GenreLexeme id, Type *type, Type *type_resultat, dls::chaine const &nom_fonction);

	void ajoute_operateur_basique_enum(Type *type);

	size_t memoire_utilisee() const;
};

OperateurUnaire const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		Type *type1,
		GenreLexeme type_op);

void enregistre_operateurs_basiques(
	ContexteGenerationCode &contexte,
	Operateurs &operateurs);

struct OperateurCandidat {
	OperateurBinaire const *op = nullptr;
	TransformationType transformation_type1{};
	TransformationType transformation_type2{};
	double poids = 0.0;
	bool inverse_operandes = false;

	OperateurCandidat() = default;

	COPIE_CONSTRUCT(OperateurCandidat);
};

dls::tablet<OperateurCandidat, 10> cherche_candidats_operateurs(
		ContexteGenerationCode const &contexte,
		Type *type1,
		Type *type2,
		GenreLexeme type_op);
