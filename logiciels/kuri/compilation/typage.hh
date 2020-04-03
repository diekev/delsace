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

#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tablet.hh"

#include "lexemes.hh"
#include "structures.hh"

#ifdef AVEC_LLVM
namespace llvm {
class Constant;
class Type;
}
#endif

struct GrapheDependance;
struct Operateurs;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudStruct;

/**
 * Système de type.
 *
 * Puisque qu'il est possible d'associer des expressions aux types, le système
 * de type est basé autour de deux structures : DonneesTypeDeclare et Type.
 * La première stocke les lexèmes des types et l'arbre syntaxique de leurs
 * expressions tel qu'ils ont été écris dans le programme, alors que la seconde
 * est la représentation des types finaux, qui sont résolus lors de la valiation
 * sémantique selon le contexte où ils apparaissent.
 *
 * Par exemple, voici quelques cas où les types déclarés ont des éléments
 * différents mais pointent vers un même type final :
 * - [1024]z32 et [2 * 512]z32
 * - a : z32; et b : type_de(a);
 *
 * Ce niveau d'indirection nous permet également d'avoir un système de gabarit
 * où les types déclarés possèdent les informations sur les gabarits et nous
 * aident à résoudre leurs types finaux lors des appels.
 */

/* ************************************************************************** */

using type_plage_donnees_type = dls::plage_continue<const GenreLexeme>;

struct DonneesTypeDeclare {
	// 5 lexème par défaut est amplement suffisant, la plupart des types
	// complexes sont de forme *[]T
	dls::tablet<GenreLexeme, 5> donnees{};

	// 2 expresssions par défaut, pour les types de matrices [..][..]T
	dls::tablet<NoeudExpression *, 2> expressions{};

	// nous ne pouvons pas utiliser tablet ici... car DonneesTypeDeclare est
	// utilisé par valeur, ce qui nous une récursion pour déterminer la taille
	// final de la structure
	dls::tableau<DonneesTypeDeclare> types_entrees{};
	dls::tableau<DonneesTypeDeclare> types_sorties{};

	dls::vue_chaine_compacte nom_struct = "";
	dls::vue_chaine_compacte nom_gabarit = "";
	bool est_gabarit = false;

	using type_plage = type_plage_donnees_type;

	DonneesTypeDeclare() = default;
	DonneesTypeDeclare(GenreLexeme i0);
	DonneesTypeDeclare(GenreLexeme i0, GenreLexeme i1);

	GenreLexeme type_base() const;

	long taille() const;

	GenreLexeme operator[](long idx) const;

	void pousse(GenreLexeme id);

	void pousse(DonneesTypeDeclare const &dtd);

	type_plage plage() const;

	/**
	 * Retourne des données pour un type correspondant au déréférencement de ce
	 * type. Si le type n'est ni un pointeur, ni un tableau, retourne des
	 * données invalides.
	 */
	type_plage dereference() const;
};

inline bool est_invalide(type_plage_donnees_type p)
{
	if (p.est_finie()) {
		return true;
	}

	return false;
}

/* ************************************************************************** */

enum class TypeBase : char {
	N8,
	N16,
	N32,
	N64,
	Z8,
	Z16,
	Z32,
	Z64,
	R16,
	R32,
	R64,
	EINI,
	CHAINE,
	RIEN,
	BOOL,
	OCTET,
	ENTIER_CONSTANT,

	PTR_N8,
	PTR_N16,
	PTR_N32,
	PTR_N64,
	PTR_Z8,
	PTR_Z16,
	PTR_Z32,
	PTR_Z64,
	PTR_R16,
	PTR_R32,
	PTR_R64,
	PTR_EINI,
	PTR_CHAINE,
	PTR_RIEN,
	PTR_NUL,
	PTR_BOOL,
	PTR_OCTET,

	REF_N8,
	REF_N16,
	REF_N32,
	REF_N64,
	REF_Z8,
	REF_Z16,
	REF_Z32,
	REF_Z64,
	REF_R16,
	REF_R32,
	REF_R64,
	REF_EINI,
	REF_CHAINE,
	REF_RIEN,
	REF_BOOL,

	TABL_N8,
	TABL_N16,
	TABL_N32,
	TABL_N64,
	TABL_Z8,
	TABL_Z16,
	TABL_Z32,
	TABL_Z64,
	TABL_R16,
	TABL_R32,
	TABL_R64,
	TABL_EINI,
	TABL_CHAINE,
	TABL_BOOL,
	TABL_OCTET,

	TOTAL,
};

enum class GenreType : int {
	INVALIDE,
	ENTIER_NATUREL,
	ENTIER_RELATIF,
	ENTIER_CONSTANT,
	REEL,
	POINTEUR,
	STRUCTURE,
	REFERENCE,
	TABLEAU_FIXE,
	TABLEAU_DYNAMIQUE,
	EINI,
	UNION,
	CHAINE,
	FONCTION,
	RIEN,
	BOOL,
	OCTET,
	ENUM,
	VARIADIQUE,
	ERREUR,
	//EINI_ERREUR,
};

enum {
	TYPEDEF_FUT_GENERE = 1
};

struct Type {
	GenreType genre{};
	unsigned taille_octet = 0;
	unsigned alignement = 0;
	int drapeaux = 0;

	dls::chaine nom_broye{};
	dls::chaine ptr_info_type{};

#ifdef AVEC_LLVM
	llvm::Type *type_llvm = nullptr;
	llvm::Constant *info_type_llvm = nullptr;
#endif

	static Type *cree_entier(unsigned taille_octet, bool est_naturel)
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = est_naturel ? GenreType::ENTIER_NATUREL : GenreType::ENTIER_RELATIF;
		type->taille_octet = taille_octet;
		type->alignement = taille_octet;
		return type;
	}

	static Type *cree_entier_constant()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::ENTIER_CONSTANT;

		return type;
	}

	static Type *cree_reel(unsigned taille_octet)
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::REEL;
		type->taille_octet = taille_octet;
		type->alignement = taille_octet;
		return type;
	}

	static Type *cree_eini()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::EINI;
		type->taille_octet = 16;
		type->alignement = 8;
		return type;
	}

	static Type *cree_chaine()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::CHAINE;
		type->taille_octet = 16;
		type->alignement = 8;
		return type;
	}

	static Type *cree_rien()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::RIEN;
		type->taille_octet = 0;
		return type;
	}

	static Type *cree_bool()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::BOOL;
		type->taille_octet = 1;
		type->alignement = 1;
		return type;
	}

	static Type *cree_octet()
	{
		auto type = memoire::loge<Type>("Type");
		type->genre = GenreType::OCTET;
		type->taille_octet = 1;
		type->alignement = 1;
		return type;
	}
};

struct TypePointeur : public Type {
	TypePointeur() { genre = GenreType::POINTEUR; }

	COPIE_CONSTRUCT(TypePointeur);

	Type *type_pointe = nullptr;

	static TypePointeur *cree(Type *type_pointe)
	{
		auto type = memoire::loge<TypePointeur>("TypePointeur");
		type->type_pointe = type_pointe;
		type->taille_octet = 8;
		type->alignement = 8;
		return type;
	}
};

struct TypeReference : public Type {
	TypeReference() { genre = GenreType::REFERENCE; }

	COPIE_CONSTRUCT(TypeReference);

	Type *type_pointe = nullptr;

	static TypeReference *cree(Type *type_pointe)
	{
		assert(type_pointe);

		auto type = memoire::loge<TypeReference>("TypeReference");
		type->type_pointe = type_pointe;
		type->taille_octet = 8;
		type->alignement = 8;
		return type;
	}
};

#if 0 // ébauche pour déplacer les données des types dans ceux-ci, et ne plus les stocker dans les noeuds des arbres syntaxiques
struct TypeCompose : public Type {
	struct Membre {
		Type *type = nullptr;
		dls::vue_chaine_compacte nom = "";
		unsigned decalage = 0;
	};

	kuri::tableau<Membre> membres{};
	dls::vue_chaine_compacte nom{};
};

inline bool est_type_compose(Type *type)
{
	return dls::outils::est_element(
				type->genre,
				GenreType::EINI,
				GenreType::ENUM,
				GenreType::ERREUR,
				GenreType::STRUCTURE,
				GenreType::TABLEAU_DYNAMIQUE,
				GenreType::TABLEAU_FIXE,
				GenreType::UNION,
				GenreType::VARIADIQUE);
}
#endif

struct TypeStructure : public Type {
	TypeStructure() { genre = GenreType::STRUCTURE; }

	COPIE_CONSTRUCT(TypeStructure);

	kuri::tableau<Type *> types{};

	kuri::tableau<TypeStructure *> types_employes{};

	NoeudStruct *decl = nullptr;

	bool deja_genere = false;

	dls::vue_chaine_compacte nom{};
};

struct TypeUnion : public Type {
	TypeUnion() { genre = GenreType::UNION; }

	COPIE_CONSTRUCT(TypeUnion);

	kuri::tableau<Type *> types{};

	Type *type_le_plus_grand = nullptr;

	NoeudStruct *decl = nullptr;

	bool deja_genere = false;
	bool est_nonsure = false;

	dls::vue_chaine_compacte nom{};
};

struct TypeEnum : public Type {
	TypeEnum() { genre = GenreType::ENUM; }

	COPIE_CONSTRUCT(TypeEnum);

	Type *type_donnees{};

	NoeudEnum *decl = nullptr;

	dls::vue_chaine_compacte nom{};
};

struct TypeTableauFixe : public Type {
	TypeTableauFixe() { genre = GenreType::TABLEAU_FIXE; }

	COPIE_CONSTRUCT(TypeTableauFixe);

	Type *type_pointe = nullptr;
	long taille = 0;

	static TypeTableauFixe *cree(Type *type_pointe, long taille)
	{
		assert(type_pointe);

		auto type = memoire::loge<TypeTableauFixe>("TypeTableauFixe");
		type->type_pointe = type_pointe;
		type->taille = taille;
		type->alignement = type_pointe->alignement;
		type->taille_octet = type_pointe->taille_octet * static_cast<unsigned>(taille);
		return type;
	}
};

struct TypeTableauDynamique : public Type {
	TypeTableauDynamique() { genre = GenreType::TABLEAU_DYNAMIQUE; }

	COPIE_CONSTRUCT(TypeTableauDynamique);

	Type *type_pointe = nullptr;

	static TypeTableauDynamique *cree(Type *type_pointe)
	{
		assert(type_pointe);

		auto type = memoire::loge<TypeTableauDynamique>("TypeTableauDynamique");
		type->type_pointe = type_pointe;
		type->taille_octet = 24;
		type->alignement = 8;
		return type;
	}
};

struct TypeFonction : public Type {
	TypeFonction() { genre = GenreType::FONCTION; }

	kuri::tableau<Type *> types_entrees{};
	kuri::tableau<Type *> types_sorties{};

	static TypeFonction *cree(kuri::tableau<Type *> &&entrees, kuri::tableau<Type *> &&sorties)
	{
		auto type = memoire::loge<TypeFonction>("TypeFonction");
		type->types_entrees = std::move(entrees);
		type->types_sorties = std::move(sorties);
		type->taille_octet = 8;
		type->alignement = 8;

		return type;
	}
};

struct TypeVariadique : public Type {
	TypeVariadique() { genre = GenreType::VARIADIQUE; }

	COPIE_CONSTRUCT(TypeVariadique);

	Type *type_pointe = nullptr;

	static TypeVariadique *cree(Type *type_pointe_)
	{
		auto type = memoire::loge<TypeVariadique>("TypeVariadique");
		type->type_pointe = type_pointe_;

		return type;
	}
};

struct Typeuse {
	GrapheDependance &graphe;
	Operateurs &operateurs;

	dls::tableau<Type *> types_communs{};
	dls::tableau<Type *> types_simples{};
	dls::tableau<TypePointeur *> types_pointeurs{};
	dls::tableau<TypeReference *> types_references{};
	dls::tableau<TypeStructure *> types_structures{};
	dls::tableau<TypeEnum *> types_enums{};
	dls::tableau<TypeTableauFixe *> types_tableaux_fixes{};
	dls::tableau<TypeTableauDynamique *> types_tableaux_dynamiques{};
	dls::tableau<TypeFonction *> types_fonctions{};
	dls::tableau<TypeVariadique *> types_variadiques{};
	dls::tableau<TypeUnion *> types_unions{};

	// -------------------------

	Typeuse(GrapheDependance &g, Operateurs &o);

	~Typeuse();

	Type *type_pour_lexeme(GenreLexeme lexeme);

	TypePointeur *type_pointeur_pour(Type *type);

	TypeReference *type_reference_pour(Type *type);

	Type *type_dereference_pour(Type *type);

	TypeTableauFixe *type_tableau_fixe(Type *type_pointe, long taille);

	TypeTableauDynamique *type_tableau_dynamique(Type *type_pointe);

	TypeVariadique *type_variadique(Type *type_pointe);

	Type *type_pour_nom(dls::vue_chaine_compacte const &chaine);

	TypeFonction *discr_type_fonction(TypeFonction *it, kuri::tableau<Type *> const &entrees, kuri::tableau<Type *> const &sorties);

	TypeFonction *type_fonction(kuri::tableau<Type *> &&entrees, kuri::tableau<Type *> &&sorties);

	TypeStructure *reserve_type_structure(NoeudStruct *decl);

	TypeEnum *reserve_type_enum(NoeudEnum *decl);

	TypeUnion *reserve_type_union(NoeudStruct *decl);

	TypeEnum *reserve_type_erreur(NoeudEnum *decl);

	inline Type *operator[](TypeBase type_base) const
	{
		return types_communs[static_cast<long>(type_base)];
	}

	size_t memoire_utilisee() const;

	long nombre_de_types() const;
};

/* ************************************************************************** */

dls::chaine chaine_type(Type const *type);

inline bool est_type_entier(Type const *type)
{
	return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
}
