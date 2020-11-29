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
struct EspaceDeTravail;
struct NoeudDeclarationEnteteFonction;
struct Statistiques;
struct Type;
struct TypeEnum;

enum class IndiceTypeOp {
	ENTIER_NATUREL,
	ENTIER_RELATIF,
	REEL,
};

#define ENUMERE_OPERATEURS_UNAIRE \
	ENUMERE_GENRE_OPUNAIRE_EX(Invalide) \
	ENUMERE_GENRE_OPUNAIRE_EX(Positif) \
	ENUMERE_GENRE_OPUNAIRE_EX(Complement) \
	ENUMERE_GENRE_OPUNAIRE_EX(Non_Logique) \
	ENUMERE_GENRE_OPUNAIRE_EX(Non_Binaire) \
	ENUMERE_GENRE_OPUNAIRE_EX(Prise_Adresse)

struct OperateurUnaire {
	enum class Genre : char {
#define ENUMERE_GENRE_OPUNAIRE_EX(genre) genre,
		ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
	};

	Type *type_operande = nullptr;
	Type *type_resultat = nullptr;

	NoeudDeclarationEnteteFonction *decl = nullptr;

	Genre genre{};
	bool est_basique = true;
};

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre);

#define ENUMERE_OPERATEURS_BINAIRE \
	ENUMERE_GENRE_OPBINAIRE_EX(Invalide) \
	ENUMERE_GENRE_OPBINAIRE_EX(Addition) \
	ENUMERE_GENRE_OPBINAIRE_EX(Addition_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Soustraction) \
	ENUMERE_GENRE_OPBINAIRE_EX(Soustraction_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Multiplication) \
	ENUMERE_GENRE_OPBINAIRE_EX(Multiplication_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Division_Naturel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Division_Relatif) \
	ENUMERE_GENRE_OPBINAIRE_EX(Division_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Reste_Naturel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Reste_Relatif) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Egal) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inegal) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Nat) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal_Nat) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Nat) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal_Nat) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Egal_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inegal_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal_Reel) \
	ENUMERE_GENRE_OPBINAIRE_EX(Et_Logique) \
	ENUMERE_GENRE_OPBINAIRE_EX(Ou_Logique) \
	ENUMERE_GENRE_OPBINAIRE_EX(Et_Binaire) \
	ENUMERE_GENRE_OPBINAIRE_EX(Ou_Binaire) \
	ENUMERE_GENRE_OPBINAIRE_EX(Ou_Exclusif) \
	ENUMERE_GENRE_OPBINAIRE_EX(Dec_Gauche) \
	ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Arithm) \
	ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Logique)

struct OperateurBinaire {
	enum class Genre : char {
#define ENUMERE_GENRE_OPBINAIRE_EX(genre) genre,
		ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
	};

	Type *type1{};
	Type *type2{};
	Type *type_resultat{};

	NoeudDeclarationEnteteFonction *decl = nullptr;

	Genre genre{};

	/* vrai si l'on peut sainement inverser les paramètres,
	 * vrai pour : +, *, !=, == */
	bool est_commutatif = false;

	/* faux pour les opérateurs définis par l'utilisateur */
	bool est_basique = true;
};

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre);

// À FAIRE : considère synchroniser les conteneurs des opérateurs au lieu de la structure, il faudra sans doute revoir l'interface afin de ne pas avoir à trop prendre de verrous
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

	void ajoute_perso(GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, NoeudDeclarationEnteteFonction *decl);

	void ajoute_perso_unaire(GenreLexeme id, Type *type, Type *type_resultat, NoeudDeclarationEnteteFonction *decl);

	void ajoute_operateur_basique_enum(TypeEnum *type);

	void rassemble_statistiques(Statistiques &stats) const;
};

OperateurUnaire const *cherche_operateur_unaire(
		Operateurs const &operateurs,
		Type *type1,
		GenreLexeme type_op);

void enregistre_operateurs_basiques(EspaceDeTravail &espace, Operateurs &operateurs);

struct OperateurCandidat {
	OperateurBinaire const *op = nullptr;
	TransformationType transformation_type1{};
	TransformationType transformation_type2{};
	double poids = 0.0;
	bool inverse_operandes = false;

	OperateurCandidat() = default;

	COPIE_CONSTRUCT(OperateurCandidat);
	POINTEUR_NUL(OperateurCandidat)
};

bool cherche_candidats_operateurs(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type1,
		Type *type2,
		GenreLexeme type_op,
		dls::tablet<OperateurCandidat, 10> &candidats);
