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

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tablet.hh"

#include "lexemes.hh"
#include "structures.hh"

struct AtomeConstante;
struct GrapheDependance;
struct IdentifiantCode;
struct InfoType;
struct Operateurs;
struct NoeudDependance;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudStruct;
struct Typeuse;
struct TypeCompose;
struct TypeEnum;
struct TypeEnum;
struct TypeFonction;
struct TypePointeur;
struct TypePolymorphique;
struct TypeReference;
struct TypeStructure;
struct TypeTableauDynamique;
struct TypeTableauFixe;
struct TypeTypeDeDonnees;
struct TypeUnion;
struct TypeVariadique;

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

#define ENUMERE_GENRES_TYPES \
	ENUMERE_GENRE_TYPE_EX(INVALIDE) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_NATUREL) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_RELATIF) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_CONSTANT) \
	ENUMERE_GENRE_TYPE_EX(REEL) \
	ENUMERE_GENRE_TYPE_EX(POINTEUR) \
	ENUMERE_GENRE_TYPE_EX(STRUCTURE) \
	ENUMERE_GENRE_TYPE_EX(REFERENCE) \
	ENUMERE_GENRE_TYPE_EX(TABLEAU_FIXE) \
	ENUMERE_GENRE_TYPE_EX(TABLEAU_DYNAMIQUE) \
	ENUMERE_GENRE_TYPE_EX(EINI) \
	ENUMERE_GENRE_TYPE_EX(UNION) \
	ENUMERE_GENRE_TYPE_EX(CHAINE) \
	ENUMERE_GENRE_TYPE_EX(FONCTION) \
	ENUMERE_GENRE_TYPE_EX(RIEN) \
	ENUMERE_GENRE_TYPE_EX(BOOL) \
	ENUMERE_GENRE_TYPE_EX(OCTET) \
	ENUMERE_GENRE_TYPE_EX(ENUM) \
	ENUMERE_GENRE_TYPE_EX(VARIADIQUE) \
	ENUMERE_GENRE_TYPE_EX(ERREUR) \
	/* ENUMERE_GENRE_TYPE_EX(EINI_ERREUR) */ \
	ENUMERE_GENRE_TYPE_EX(TYPE_DE_DONNEES) \
	ENUMERE_GENRE_TYPE_EX(POLYMORPHIQUE)

enum class GenreType : int {
#define ENUMERE_GENRE_TYPE_EX(genre) genre,
		ENUMERE_GENRES_TYPES
#undef ENUMERE_GENRE_TYPE_EX
};

const char *chaine_genre_type(GenreType genre);
std::ostream &operator<<(std::ostream &os, GenreType genre);

enum {
	TYPEDEF_FUT_GENERE = 1,
	TYPE_EST_POLYMORPHIQUE = 2,
	TYPE_FUT_VALIDE = 4,
	RI_TYPE_FUT_GENEREE =  8,
	POSSEDE_TYPE_POINTEUR = 16,
	POSSEDE_TYPE_REFERENCE = 32,
	POSSEDE_TYPE_TABLEAU_FIXE = 64,
	POSSEDE_TYPE_TABLEAU_DYNAMIQUE = 128,
	POSSEDE_TYPE_TYPE_DE_DONNEES = 256,
	CODE_BINAIRE_TYPE_FUT_GENERE = 512,
	TYPE_EST_NORMALISE           = 1024,
};

struct Type {
	GenreType genre{};
	unsigned taille_octet = 0;
	unsigned alignement = 0;
	int drapeaux = 0;
	unsigned index_dans_table_types = 0;

	dls::chaine nom_broye{};

	InfoType *info_type = nullptr;
	AtomeConstante *atome_info_type = nullptr;
	NoeudDependance *noeud_dependance = nullptr;

	POINTEUR_NUL(Type)

	static Type *cree_entier(unsigned taille_octet, bool est_naturel);
	static Type *cree_entier_constant();
	static Type *cree_reel(unsigned taille_octet);
	static Type *cree_rien();
	static Type *cree_bool();
	static Type *cree_octet();

	inline bool est_bool() const { return genre == GenreType::BOOL; }
	inline bool est_chaine() const { return genre == GenreType::CHAINE; }
	inline bool est_eini() const { return genre == GenreType::EINI; }
	inline bool est_entier_constant() const { return genre == GenreType::ENTIER_CONSTANT; }
	inline bool est_entier_naturel() const { return genre == GenreType::ENTIER_NATUREL; }
	inline bool est_entier_relatif() const { return genre == GenreType::ENTIER_RELATIF; }
	inline bool est_enum() const { return genre == GenreType::ENUM; }
	inline bool est_erreur() const { return genre == GenreType::ERREUR; }
	inline bool est_fonction() const { return genre == GenreType::FONCTION; }
	inline bool est_invalide() const { return genre == GenreType::INVALIDE; }
	inline bool est_octet() const { return genre == GenreType::OCTET; }
	inline bool est_pointeur() const { return genre == GenreType::POINTEUR; }
	inline bool est_polymorphique() const { return genre == GenreType::POLYMORPHIQUE; }
	inline bool est_reel() const { return genre == GenreType::REEL; }
	inline bool est_reference() const { return genre == GenreType::REFERENCE; }
	inline bool est_rien() const { return genre == GenreType::RIEN; }
	inline bool est_structure() const { return genre == GenreType::STRUCTURE; }
	inline bool est_tableau_dynamique() const { return genre == GenreType::TABLEAU_DYNAMIQUE; }
	inline bool est_tableau_fixe() const { return genre == GenreType::TABLEAU_FIXE; }
	inline bool est_type_de_donnees() const { return genre == GenreType::TYPE_DE_DONNEES; }
	inline bool est_union() const { return genre == GenreType::UNION; }
	inline bool est_variadique() const { return genre == GenreType::VARIADIQUE; }

	inline TypeCompose *comme_compose();
	inline TypeEnum *comme_enum();
	inline TypeEnum *comme_erreur();
	inline TypeFonction *comme_fonction();
	inline TypePointeur *comme_pointeur();
	inline TypePolymorphique *comme_polymorphique();
	inline TypeReference *comme_reference();
	inline TypeStructure *comme_structure();
	inline TypeTableauDynamique *comme_tableau_dynamique();
	inline TypeTableauFixe *comme_tableau_fixe();
	inline TypeTypeDeDonnees *comme_type_de_donnees();
	inline TypeUnion *comme_union();
	inline TypeVariadique *comme_variadique();
};

struct TypePointeur : public Type {
	TypePointeur() { genre = GenreType::POINTEUR; }

	COPIE_CONSTRUCT(TypePointeur);

	Type *type_pointe = nullptr;

	static TypePointeur *cree(Type *type_pointe);
};

struct TypeReference : public Type {
	TypeReference() { genre = GenreType::REFERENCE; }

	COPIE_CONSTRUCT(TypeReference);

	Type *type_pointe = nullptr;

	static TypeReference *cree(Type *type_pointe);
};

struct TypeFonction : public Type {
	TypeFonction() { genre = GenreType::FONCTION; }

	kuri::tableau<Type *> types_entrees{};
	kuri::tableau<Type *> types_sorties{};

	static TypeFonction *cree(kuri::tableau<Type *> &&entrees, kuri::tableau<Type *> &&sorties);

	void marque_polymorphique();
};

/* Type de base pour tous les types ayant des membres (structures, énumérations, etc.).
 */
struct TypeCompose : public Type {
	struct Membre {
		enum {
			EST_CONSTANT  = 1, // si le membre est une constante
			EST_IMPLICITE = 2,
		};

		Type *type = nullptr;
		dls::vue_chaine_compacte nom = "";
		unsigned decalage = 0;
		int valeur = 0; // pour les énumérations
		NoeudExpression *expression_valeur_defaut = nullptr; // pour les membres des structures
		int drapeaux = 0;
	};

	kuri::tableau<Membre> membres{};
	dls::vue_chaine_compacte nom{};

	static TypeCompose *cree_eini();

	static TypeCompose *cree_chaine();
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
	bool est_anonyme = false;
};

struct TypeUnion final : public TypeCompose {
	TypeUnion() { genre = GenreType::UNION; }

	COPIE_CONSTRUCT(TypeUnion);

	Type *type_le_plus_grand = nullptr;
	TypeStructure *type_structure = nullptr;

	NoeudStruct *decl = nullptr;

	unsigned decalage_index = 0;
	bool deja_genere = false;
	bool est_nonsure = false;
	bool est_anonyme = false;

	void cree_type_structure(Typeuse &typeuse, unsigned alignement_membre_actif);
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

	static TypeTableauFixe *cree(Type*type_pointe, long taille, kuri::tableau<TypeCompose::Membre> &&membres);
};

struct TypeTableauDynamique final : public TypeCompose {
	TypeTableauDynamique() { genre = GenreType::TABLEAU_DYNAMIQUE; }

	COPIE_CONSTRUCT(TypeTableauDynamique);

	Type *type_pointe = nullptr;

	static TypeTableauDynamique *cree(Type *type_pointe, kuri::tableau<TypeCompose::Membre> &&membres);
};

struct TypeVariadique final : public TypeCompose {
	TypeVariadique() { genre = GenreType::VARIADIQUE; }

	COPIE_CONSTRUCT(TypeVariadique);

	Type *type_pointe = nullptr;

	static TypeVariadique *cree(Type *type_pointe, kuri::tableau<TypeCompose::Membre> &&membres);
};

struct TypeTypeDeDonnees : public Type {
	TypeTypeDeDonnees() { genre = GenreType::TYPE_DE_DONNEES; }

	COPIE_CONSTRUCT(TypeTypeDeDonnees);

	// Non-nul si le type est connu lors de la compilation.
	Type *type_connu = nullptr;

	static TypeTypeDeDonnees *cree(Type *type_connu);
};

struct TypePolymorphique : public Type {
	TypePolymorphique()
	{
		genre = GenreType::POLYMORPHIQUE;
		drapeaux = TYPE_EST_POLYMORPHIQUE;
	}

	COPIE_CONSTRUCT(TypePolymorphique);

	IdentifiantCode *ident = nullptr;

	static TypePolymorphique *cree(IdentifiantCode *ident);
};

/* ************************************************************************** */

inline TypePointeur *Type::comme_pointeur()
{
	assert(genre == GenreType::POINTEUR);
	return static_cast<TypePointeur *>(this);
}

inline TypeStructure *Type::comme_structure()
{
	assert(genre == GenreType::STRUCTURE);
	return static_cast<TypeStructure *>(this);
}

inline TypeCompose *Type::comme_compose()
{
	assert(est_type_compose(this));
	return static_cast<TypeCompose *>(this);
}

inline TypeReference *Type::comme_reference()
{
	assert(genre == GenreType::REFERENCE);
	return static_cast<TypeReference *>(this);
}

inline TypeTableauFixe *Type::comme_tableau_fixe()
{
	assert(genre == GenreType::TABLEAU_FIXE);
	return static_cast<TypeTableauFixe *>(this);
}

inline TypeTableauDynamique *Type::comme_tableau_dynamique()
{
	assert(genre == GenreType::TABLEAU_DYNAMIQUE);
	return static_cast<TypeTableauDynamique *>(this);
}

inline TypeUnion *Type::comme_union()
{
	assert(genre == GenreType::UNION);
	return static_cast<TypeUnion *>(this);
}

inline TypeFonction *Type::comme_fonction()
{
	assert(genre == GenreType::FONCTION);
	return static_cast<TypeFonction *>(this);
}

inline TypeEnum *Type::comme_enum()
{
	assert(genre == GenreType::ENUM || genre == GenreType::ERREUR);
	return static_cast<TypeEnum *>(this);
}

inline TypeEnum *Type::comme_erreur()
{
	assert(genre == GenreType::ERREUR);
	return static_cast<TypeEnum *>(this);
}

inline TypeVariadique *Type::comme_variadique()
{
	assert(genre == GenreType::VARIADIQUE);
	return static_cast<TypeVariadique *>(this);
}

inline TypeTypeDeDonnees *Type::comme_type_de_donnees()
{
	assert(genre == GenreType::TYPE_DE_DONNEES);
	return static_cast<TypeTypeDeDonnees *>(this);
}

inline TypePolymorphique *Type::comme_polymorphique()
{
	assert(genre == GenreType::POLYMORPHIQUE);
	return static_cast<TypePolymorphique *>(this);
}

/* ************************************************************************** */

void rassemble_noms_type_polymorphique(Type *type, kuri::tableau<dls::vue_chaine_compacte> &noms);

// À FAIRE : il peut y avoir une concurrence critique pour l'assignation d'index aux types
struct Typeuse {
	dls::outils::Synchrone<GrapheDependance> &graphe_;
	dls::outils::Synchrone<Operateurs> &operateurs_;

	template <typename T>
	using tableau_synchrone = dls::outils::Synchrone<dls::tableau<T>>;

	// NOTE : nous synchronisons les tableaux individuellement et non la Typeuse
	// dans son entièreté afin que différents threads puissent accéder librement
	// à différents types de types.
	dls::tableau<Type *> types_communs{};
	tableau_synchrone<Type *> types_simples{};
	tableau_synchrone<TypePointeur *> types_pointeurs{};
	tableau_synchrone<TypeReference *> types_references{};
	tableau_synchrone<TypeStructure *> types_structures{};
	tableau_synchrone<TypeEnum *> types_enums{};
	tableau_synchrone<TypeTableauFixe *> types_tableaux_fixes{};
	tableau_synchrone<TypeTableauDynamique *> types_tableaux_dynamiques{};
	tableau_synchrone<TypeFonction *> types_fonctions{};
	tableau_synchrone<TypeVariadique *> types_variadiques{};
	tableau_synchrone<TypeUnion *> types_unions{};
	tableau_synchrone<TypeTypeDeDonnees *> types_type_de_donnees{};
	tableau_synchrone<TypePolymorphique *> types_polymorphiques{};

	// mise en cache de plusieurs types pour mieux les trouver
	TypeTypeDeDonnees *type_type_de_donnees_ = nullptr;
	Type *type_contexte = nullptr;
	Type *type_info_type_ = nullptr;
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

	Typeuse(dls::outils::Synchrone<GrapheDependance> &g, dls::outils::Synchrone<Operateurs> &o);

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

	long memoire_utilisee() const;

	long nombre_de_types() const;

	void construit_table_types();
};

/* ************************************************************************** */

dls::chaine chaine_type(Type const *type);

Type *type_dereference_pour(Type *type);

inline bool est_type_entier(Type const *type)
{
	return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
}

bool est_type_conditionnable(Type *type);

Type *apparie_type_gabarit(Type *type, Type *type_polymorphique);

Type *resoud_type_polymorphique(Typeuse &typeuse, Type *type_gabarit, Type *pour_type);

Type *normalise_type(Typeuse &typeuse, Type *type);

void calcule_taille_type_compose(TypeCompose *type);
