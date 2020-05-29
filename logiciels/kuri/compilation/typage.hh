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

#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tablet.hh"

#include "lexemes.hh"
#include "structures.hh"

struct GrapheDependance;
struct Operateurs;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudStruct;

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
	TYPE_DE_DONNEES,
	POLYMORPHIQUE,
};

enum {
	TYPEDEF_FUT_GENERE = 1,
	TYPE_EST_POLYMORPHIQUE = 2,
};

struct AtomeConstante;

struct Type {
	GenreType genre{};
	unsigned taille_octet = 0;
	unsigned alignement = 0;
	int drapeaux = 0;
	unsigned index_dans_table_types = 0;

	dls::chaine nom_broye{};

	AtomeConstante *info_type = nullptr;

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

		if (type_pointe && type_pointe->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

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

		if (type_pointe->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

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
		type->marque_polymorphique();

		return type;
	}

	void marque_polymorphique()
	{
		POUR (types_entrees) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
				return;
			}
		}

		POUR (types_sorties) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
				return;
			}
		}
	}
};

/* Type de base pour tous les types ayant des membres (structures, énumérations, etc.).
 */
struct TypeCompose : public Type {
	struct Membre {
		Type *type = nullptr;
		dls::vue_chaine_compacte nom = "";
		unsigned decalage = 0;
		int valeur = 0; // pour les énumérations
		NoeudExpression *expression_valeur_defaut = nullptr; // pour les membres des structures
	};

	kuri::tableau<Membre> membres{};
	dls::vue_chaine_compacte nom{};

	static TypeCompose *cree_eini()
	{
		auto type = memoire::loge<TypeCompose>("TypeCompose");
		type->genre = GenreType::EINI;
		type->taille_octet = 16;
		type->alignement = 8;
		return type;
	}

	static TypeCompose *cree_chaine()
	{
		auto type = memoire::loge<TypeCompose>("TypeCompose");
		type->genre = GenreType::CHAINE;
		type->taille_octet = 16;
		type->alignement = 8;
		return type;
	}
};

inline bool est_type_compose(Type *type)
{
	return dls::outils::est_element(
				type->genre,
				GenreType::CHAINE,
				GenreType::EINI,
				GenreType::ENUM,
				GenreType::ERREUR,
				GenreType::STRUCTURE,
				GenreType::TABLEAU_DYNAMIQUE,
				GenreType::TABLEAU_FIXE,
				GenreType::UNION,
				GenreType::VARIADIQUE);
}

struct TypeStructure final : public TypeCompose {
	TypeStructure() { genre = GenreType::STRUCTURE; }

	COPIE_CONSTRUCT(TypeStructure);

	kuri::tableau<TypeStructure *> types_employes{};

	NoeudStruct *decl = nullptr;

	bool deja_genere = false;
};

struct TypeUnion final : public TypeCompose {
	TypeUnion() { genre = GenreType::UNION; }

	COPIE_CONSTRUCT(TypeUnion);

	Type *type_le_plus_grand = nullptr;

	NoeudStruct *decl = nullptr;

	unsigned decalage_index = 0;
	bool deja_genere = false;
	bool est_nonsure = false;
	bool est_anonyme = false;
};

struct TypeEnum final : public TypeCompose {
	TypeEnum() { genre = GenreType::ENUM; }

	COPIE_CONSTRUCT(TypeEnum);

	Type *type_donnees{};

	NoeudEnum *decl = nullptr;
	bool est_drapeau = false;
	bool est_erreur = false;
};

struct TypeTableauFixe final : public TypeCompose {
	TypeTableauFixe() { genre = GenreType::TABLEAU_FIXE; }

	COPIE_CONSTRUCT(TypeTableauFixe);

	Type *type_pointe = nullptr;
	long taille = 0;

	static TypeTableauFixe *cree(Type*type_pointe, long taille, kuri::tableau<TypeCompose::Membre> &&membres)
	{
		auto type = memoire::loge<TypeTableauFixe>("TypeTableauFixe");
		type->membres = std::move(membres);
		type->type_pointe = type_pointe;
		type->taille = taille;
		type->alignement = type_pointe->alignement;
		type->taille_octet = type_pointe->taille_octet * static_cast<unsigned>(taille);

		if (type_pointe->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

		return type;
	}
};

struct TypeTableauDynamique final : public TypeCompose {
	TypeTableauDynamique() { genre = GenreType::TABLEAU_DYNAMIQUE; }

	COPIE_CONSTRUCT(TypeTableauDynamique);

	Type *type_pointe = nullptr;

	static TypeTableauDynamique *cree(Type *type_pointe, kuri::tableau<TypeCompose::Membre> &&membres)
	{
		assert(type_pointe);

		auto type = memoire::loge<TypeTableauDynamique>("TypeTableauDynamique");
		type->membres = std::move(membres);
		type->type_pointe = type_pointe;
		type->taille_octet = 24;
		type->alignement = 8;

		if (type_pointe->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

		return type;
	}
};

struct TypeVariadique final : public TypeCompose {
	TypeVariadique() { genre = GenreType::VARIADIQUE; }

	COPIE_CONSTRUCT(TypeVariadique);

	Type *type_pointe = nullptr;

	static TypeVariadique *cree(Type *type_pointe, kuri::tableau<TypeCompose::Membre> &&membres)
	{
		auto type = memoire::loge<TypeVariadique>("TypeVariadique");
		type->type_pointe = type_pointe;

		if (type_pointe && type_pointe->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

		type->membres = std::move(membres);
		type->taille_octet = 24;
		type->alignement = 8;
		return type;
	}
};

struct TypeTypeDeDonnees : public Type {
	TypeTypeDeDonnees() { genre = GenreType::TYPE_DE_DONNEES; }

	COPIE_CONSTRUCT(TypeTypeDeDonnees);

	// Non-nul si le type est connu lors de la compilation.
	Type *type_connu = nullptr;

	static TypeTypeDeDonnees *cree(Type *type_connu)
	{
		auto type = memoire::loge<TypeTypeDeDonnees>("TypeType");
		type->genre = GenreType::TYPE_DE_DONNEES;
		// un type 'type' est un genre de pointeur déguisé, donc donnons lui les mêmes caractéristiques
		type->taille_octet = 8;
		type->alignement = 8;
		type->type_connu = type_connu;
		return type;
	}
};

struct IdentifiantCode;

struct TypePolymorphique : public Type {
	TypePolymorphique()
	{
		genre = GenreType::POLYMORPHIQUE;
		drapeaux = TYPE_EST_POLYMORPHIQUE;
	}

	COPIE_CONSTRUCT(TypePolymorphique);

	IdentifiantCode *ident = nullptr;

	static TypePolymorphique *cree(IdentifiantCode *ident)
	{
		assert(ident);

		auto type = memoire::loge<TypePolymorphique>("TypePolymorphique");
		type->ident = ident;
		return type;
	}
};

dls::vue_chaine_compacte nom_type_polymorphique(Type *type);

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
	dls::tableau<TypeTypeDeDonnees *> types_type_de_donnees{};
	dls::tableau<TypePolymorphique *> types_polymorphiques{};

	// mise en cache de plusieurs types pour mieux les trouver
	TypeTypeDeDonnees *type_type_de_donnees_ = nullptr;
	TypeStructure *type_info_type_ = nullptr;
	Type *type_info_type_structure = nullptr;
	Type *type_info_type_union = nullptr;
	Type *type_info_type_membre_structure = nullptr;
	Type *type_info_type_entier = nullptr;
	Type *type_info_type_tableau = nullptr;
	Type *type_info_type_pointeur = nullptr;
	Type *type_info_type_enum = nullptr;
	Type *type_info_type_fonction = nullptr;
	Type *type_position_code_source = nullptr;
	Type *type_info_fonction_trace_appel = nullptr;
	Type *type_trace_appel = nullptr;
	Type *type_base_allocatrice = nullptr;
	Type *type_info_appel_trace_appel = nullptr;
	Type *type_stockage_temporaire = nullptr;
	// séparés car nous devons désalloué selon la bonne taille et ce sont plus des types « simples »
	TypeCompose *type_eini = nullptr;
	TypeCompose *type_chaine = nullptr;

	// -------------------------

	Typeuse(GrapheDependance &g, Operateurs &o);

	Typeuse(Typeuse const &) = delete;
	Typeuse &operator=(Typeuse const &) = delete;

	~Typeuse();

	Type *type_pour_lexeme(GenreLexeme lexeme);

	TypePointeur *type_pointeur_pour(Type *type);

	TypeReference *type_reference_pour(Type *type);

	TypeTableauFixe *type_tableau_fixe(Type *type_pointe, long taille);

	TypeTableauDynamique *type_tableau_dynamique(Type *type_pointe);

	TypeVariadique *type_variadique(Type *type_pointe);

	TypeFonction *discr_type_fonction(TypeFonction *it, kuri::tableau<Type *> const &entrees, kuri::tableau<Type *> const &sorties);

	TypeFonction *type_fonction(kuri::tableau<Type *> &&entrees, kuri::tableau<Type *> &&sorties);

	TypeTypeDeDonnees *type_type_de_donnees(Type *type_connu);

	TypeStructure *reserve_type_structure(NoeudStruct *decl);

	TypeEnum *reserve_type_enum(NoeudEnum *decl);

	TypeUnion *reserve_type_union(NoeudStruct *decl);

	TypeUnion *union_anonyme(kuri::tableau<TypeCompose::Membre> &&membres);

	TypeEnum *reserve_type_erreur(NoeudEnum *decl);

	TypePolymorphique *cree_polymorphique(IdentifiantCode *ident);

	inline Type *operator[](TypeBase type_base) const
	{
		return types_communs[static_cast<long>(type_base)];
	}

	size_t memoire_utilisee() const;

	long nombre_de_types() const;
};

/* ************************************************************************** */

dls::chaine chaine_type(Type const *type);

Type *type_dereference_pour(Type *type);

inline bool est_type_entier(Type const *type)
{
	return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
}

bool est_type_conditionnable(Type *type);
