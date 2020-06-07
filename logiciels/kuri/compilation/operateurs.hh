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

#include "transformation_type.hh"

enum class GenreLexeme : unsigned int;
struct Compilatrice;
struct NoeudDeclarationFonction;
struct Type;

enum class IndiceTypeOp {
	ENTIER_NATUREL,
	ENTIER_RELATIF,
	REEL,
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

	NoeudDeclarationFonction *decl = nullptr;

	Genre genre{};
	bool est_basique = true;
};

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre);

struct OperateurBinaire {
	enum class Genre : char {
		Invalide,

		Addition,
		Addition_Reel,
		Soustraction,
		Soustraction_Reel,
		Multiplication,
		Multiplication_Reel,
		Division_Naturel,
		Division_Relatif,
		Division_Reel,
		Reste_Naturel,
		Reste_Relatif,

		Comp_Egal,
		Comp_Inegal,
		Comp_Inf,
		Comp_Inf_Egal,
		Comp_Sup,
		Comp_Sup_Egal,
		Comp_Inf_Nat,
		Comp_Inf_Egal_Nat,
		Comp_Sup_Nat,
		Comp_Sup_Egal_Nat,

		Comp_Egal_Reel,
		Comp_Inegal_Reel,
		Comp_Inf_Reel,
		Comp_Inf_Egal_Reel,
		Comp_Sup_Reel,
		Comp_Sup_Egal_Reel,

		Et_Logique,
		Ou_Logique,

		Et_Binaire,
		Ou_Binaire,
		Ou_Exclusif,

		Dec_Gauche,
		Dec_Droite_Arithm,
		Dec_Droite_Logique,
	};

	Type *type1{};
	Type *type2{};
	Type *type_resultat{};

	NoeudDeclarationFonction *decl = nullptr;

	Genre genre{};

	/* vrai si l'on peut sainement inverser les paramètres,
	 * vrai pour : +, *, !=, == */
	bool est_commutatif = false;

	/* faux pour les opérateurs définis par l'utilisateur */
	bool est_basique = true;
};

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre);

struct Operateurs {
	using type_conteneur_binaire = tableau_page<OperateurBinaire>;
	using type_conteneur_unaire = tableau_page<OperateurUnaire>;

	dls::dico_desordonne<GenreLexeme, type_conteneur_binaire> operateurs_binaires;
	dls::dico_desordonne<GenreLexeme, type_conteneur_unaire> operateurs_unaires;

	Type *type_bool = nullptr;

	OperateurBinaire *op_comp_egal_types = nullptr;
	OperateurBinaire *op_comp_diff_types = nullptr;

	~Operateurs();

	type_conteneur_binaire const &trouve_binaire(GenreLexeme id) const;
	type_conteneur_unaire const &trouve_unaire(GenreLexeme id) const;

	OperateurBinaire *ajoute_basique(GenreLexeme id, Type *type, Type *type_resultat, IndiceTypeOp indice_type);
	OperateurBinaire *ajoute_basique(GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, IndiceTypeOp indice_type);

	void ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat);

	void ajoute_perso(GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, NoeudDeclarationFonction *decl);

	void ajoute_perso_unaire(GenreLexeme id, Type *type, Type *type_resultat, NoeudDeclarationFonction *decl);

	void ajoute_operateur_basique_enum(Type *type);

	size_t memoire_utilisee() const;
};

OperateurUnaire const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		Type *type1,
		GenreLexeme type_op);

void enregistre_operateurs_basiques(
	Compilatrice &compilatrice,
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

bool cherche_candidats_operateurs(
		Compilatrice &compilatrice,
		ContexteValidationCode &contexte,
		Type *type1,
		Type *type2,
		GenreLexeme type_op,
		dls::tablet<OperateurCandidat, 10> &candidats);
