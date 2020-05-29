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

#include "typage.hh"

#include "contexte_generation_code.h"
#include "operateurs.hh"
#include "outils_lexemes.hh"
#include "profilage.hh"

/* ************************************************************************** */

struct DonneesTypeCommun {
	TypeBase val_enum;
	GenreLexeme dt[2];
};

static DonneesTypeCommun donnees_types_communs[] = {
	{ TypeBase::PTR_N8, { GenreLexeme::POINTEUR, GenreLexeme::N8 } },
	{ TypeBase::PTR_N16, { GenreLexeme::POINTEUR, GenreLexeme::N16 } },
	{ TypeBase::PTR_N32, { GenreLexeme::POINTEUR, GenreLexeme::N32 } },
	{ TypeBase::PTR_N64, { GenreLexeme::POINTEUR, GenreLexeme::N64 } },
	{ TypeBase::PTR_Z8, { GenreLexeme::POINTEUR, GenreLexeme::Z8 } },
	{ TypeBase::PTR_Z16, { GenreLexeme::POINTEUR, GenreLexeme::Z16 } },
	{ TypeBase::PTR_Z32, { GenreLexeme::POINTEUR, GenreLexeme::Z32 } },
	{ TypeBase::PTR_Z64, { GenreLexeme::POINTEUR, GenreLexeme::Z64 } },
	{ TypeBase::PTR_R16, { GenreLexeme::POINTEUR, GenreLexeme::R16 } },
	{ TypeBase::PTR_R32, { GenreLexeme::POINTEUR, GenreLexeme::R32 } },
	{ TypeBase::PTR_R64, { GenreLexeme::POINTEUR, GenreLexeme::R64 } },
	{ TypeBase::PTR_EINI, { GenreLexeme::POINTEUR, GenreLexeme::EINI } },
	{ TypeBase::PTR_CHAINE, { GenreLexeme::POINTEUR, GenreLexeme::CHAINE } },
	{ TypeBase::PTR_RIEN, { GenreLexeme::POINTEUR, GenreLexeme::RIEN } },
	{ TypeBase::PTR_NUL, { GenreLexeme::POINTEUR, GenreLexeme::NUL } },
	{ TypeBase::PTR_BOOL, { GenreLexeme::POINTEUR, GenreLexeme::BOOL } },
	{ TypeBase::PTR_OCTET, { GenreLexeme::POINTEUR, GenreLexeme::OCTET } },

	{ TypeBase::REF_N8, { GenreLexeme::REFERENCE, GenreLexeme::N8 } },
	{ TypeBase::REF_N16, { GenreLexeme::REFERENCE, GenreLexeme::N16 } },
	{ TypeBase::REF_N32, { GenreLexeme::REFERENCE, GenreLexeme::N32 } },
	{ TypeBase::REF_N64, { GenreLexeme::REFERENCE, GenreLexeme::N64 } },
	{ TypeBase::REF_Z8, { GenreLexeme::REFERENCE, GenreLexeme::Z8 } },
	{ TypeBase::REF_Z16, { GenreLexeme::REFERENCE, GenreLexeme::Z16 } },
	{ TypeBase::REF_Z32, { GenreLexeme::REFERENCE, GenreLexeme::Z32 } },
	{ TypeBase::REF_Z64, { GenreLexeme::REFERENCE, GenreLexeme::Z64 } },
	{ TypeBase::REF_R16, { GenreLexeme::REFERENCE, GenreLexeme::R16 } },
	{ TypeBase::REF_R32, { GenreLexeme::REFERENCE, GenreLexeme::R32 } },
	{ TypeBase::REF_R64, { GenreLexeme::REFERENCE, GenreLexeme::R64 } },
	{ TypeBase::REF_EINI, { GenreLexeme::REFERENCE, GenreLexeme::EINI } },
	{ TypeBase::REF_CHAINE, { GenreLexeme::REFERENCE, GenreLexeme::CHAINE } },
	{ TypeBase::REF_RIEN, { GenreLexeme::REFERENCE, GenreLexeme::RIEN } },
	{ TypeBase::REF_BOOL, { GenreLexeme::REFERENCE, GenreLexeme::BOOL } },

	{ TypeBase::TABL_N8, { GenreLexeme::TABLEAU, GenreLexeme::N8 } },
	{ TypeBase::TABL_N16, { GenreLexeme::TABLEAU, GenreLexeme::N16 } },
	{ TypeBase::TABL_N32, { GenreLexeme::TABLEAU, GenreLexeme::N32 } },
	{ TypeBase::TABL_N64, { GenreLexeme::TABLEAU, GenreLexeme::N64 } },
	{ TypeBase::TABL_Z8, { GenreLexeme::TABLEAU, GenreLexeme::Z8 } },
	{ TypeBase::TABL_Z16, { GenreLexeme::TABLEAU, GenreLexeme::Z16 } },
	{ TypeBase::TABL_Z32, { GenreLexeme::TABLEAU, GenreLexeme::Z32 } },
	{ TypeBase::TABL_Z64, { GenreLexeme::TABLEAU, GenreLexeme::Z64 } },
	{ TypeBase::TABL_R16, { GenreLexeme::TABLEAU, GenreLexeme::R16 } },
	{ TypeBase::TABL_R32, { GenreLexeme::TABLEAU, GenreLexeme::R32 } },
	{ TypeBase::TABL_R64, { GenreLexeme::TABLEAU, GenreLexeme::R64 } },
	{ TypeBase::TABL_EINI, { GenreLexeme::TABLEAU, GenreLexeme::EINI } },
	{ TypeBase::TABL_CHAINE, { GenreLexeme::TABLEAU, GenreLexeme::CHAINE } },
	{ TypeBase::TABL_BOOL, { GenreLexeme::TABLEAU, GenreLexeme::BOOL } },
	{ TypeBase::TABL_OCTET, { GenreLexeme::TABLEAU, GenreLexeme::OCTET } },
};

/* ************************************************************************** */

static Type *cree_type_pour_lexeme(GenreLexeme lexeme)
{
	switch (lexeme) {
		case GenreLexeme::BOOL:
		{
			return Type::cree_bool();
		}
		case GenreLexeme::OCTET:
		{
			return Type::cree_octet();
		}
		case GenreLexeme::N8:
		{
			return Type::cree_entier(1, true);
		}
		case GenreLexeme::Z8:
		{
			return Type::cree_entier(1, false);
		}
		case GenreLexeme::N16:
		{
			return Type::cree_entier(2, true);
		}
		case GenreLexeme::Z16:
		{
			return Type::cree_entier(2, false);
		}
		case GenreLexeme::N32:
		{
			return Type::cree_entier(4, true);
		}
		case GenreLexeme::Z32:
		{
			return Type::cree_entier(4, false);
		}
		case GenreLexeme::N64:
		{
			return Type::cree_entier(8, true);
		}
		case GenreLexeme::Z64:
		{
			return Type::cree_entier(8, false);
		}
		case GenreLexeme::R16:
		{
			return Type::cree_reel(2);
		}
		case GenreLexeme::R32:
		{
			return Type::cree_reel(4);
		}
		case GenreLexeme::R64:
		{
			return Type::cree_reel(8);
		}
		case GenreLexeme::RIEN:
		{
			return Type::cree_rien();
		}
		default:
		{
			return nullptr;
		}
	}
}

Typeuse::Typeuse(GrapheDependance &g, Operateurs &o)
	: graphe(g)
	, operateurs(o)
{
	/* initialise les types communs */
	types_communs.redimensionne(static_cast<long>(TypeBase::TOTAL));

	type_eini = TypeCompose::cree_eini();
	type_chaine = TypeCompose::cree_chaine();

	types_communs[static_cast<long>(TypeBase::N8)] = cree_type_pour_lexeme(GenreLexeme::N8);
	types_communs[static_cast<long>(TypeBase::N16)] = cree_type_pour_lexeme(GenreLexeme::N16);
	types_communs[static_cast<long>(TypeBase::N32)] = cree_type_pour_lexeme(GenreLexeme::N32);
	types_communs[static_cast<long>(TypeBase::N64)] = cree_type_pour_lexeme(GenreLexeme::N64);
	types_communs[static_cast<long>(TypeBase::Z8)] = cree_type_pour_lexeme(GenreLexeme::Z8);
	types_communs[static_cast<long>(TypeBase::Z16)] = cree_type_pour_lexeme(GenreLexeme::Z16);
	types_communs[static_cast<long>(TypeBase::Z32)] = cree_type_pour_lexeme(GenreLexeme::Z32);
	types_communs[static_cast<long>(TypeBase::Z64)] = cree_type_pour_lexeme(GenreLexeme::Z64);
	types_communs[static_cast<long>(TypeBase::R16)] = cree_type_pour_lexeme(GenreLexeme::R16);
	types_communs[static_cast<long>(TypeBase::R32)] = cree_type_pour_lexeme(GenreLexeme::R32);
	types_communs[static_cast<long>(TypeBase::R64)] = cree_type_pour_lexeme(GenreLexeme::R64);
	types_communs[static_cast<long>(TypeBase::EINI)] = type_eini;
	types_communs[static_cast<long>(TypeBase::CHAINE)] = type_chaine;
	types_communs[static_cast<long>(TypeBase::RIEN)] = cree_type_pour_lexeme(GenreLexeme::RIEN);
	types_communs[static_cast<long>(TypeBase::BOOL)] = cree_type_pour_lexeme(GenreLexeme::BOOL);
	types_communs[static_cast<long>(TypeBase::OCTET)] = cree_type_pour_lexeme(GenreLexeme::OCTET);
	types_communs[static_cast<long>(TypeBase::ENTIER_CONSTANT)] = Type::cree_entier_constant();

	for (auto i = static_cast<long>(TypeBase::N8); i <= static_cast<long>(TypeBase::ENTIER_CONSTANT); ++i) {
		if (i == static_cast<long>(TypeBase::EINI) || i == static_cast<long>(TypeBase::CHAINE)) {
			continue;
		}

		types_simples.pousse(types_communs[i]);
	}

	type_type_de_donnees_ = TypeTypeDeDonnees::cree(nullptr);

	// nous devons créer le pointeur nul avant les autres types, car nous en avons besoin pour définir les opérateurs pour les pointeurs
	auto ptr_nul = TypePointeur::cree(nullptr);
	types_pointeurs.pousse(ptr_nul);

	types_communs[static_cast<long>(TypeBase::PTR_NUL)] = ptr_nul;

	for (auto &donnees : donnees_types_communs) {
		auto const idx = static_cast<long>(donnees.val_enum);
		auto type = this->type_pour_lexeme(donnees.dt[1]);

		if (donnees.dt[0] == GenreLexeme::TABLEAU) {
			type = this->type_tableau_dynamique(type);
		}
		else if (donnees.dt[0] == GenreLexeme::POINTEUR) {
			type = this->type_pointeur_pour(type);
		}
		else if (donnees.dt[0] == GenreLexeme::REFERENCE) {
			type = this->type_reference_pour(type);
		}
		else {
			assert(false);
		}

		types_communs[idx] = type;
	}

	type_info_type_ = reserve_type_structure(nullptr);

	auto membres_eini = kuri::tableau<TypeCompose::Membre>();
	membres_eini.pousse({ types_communs[static_cast<long>(TypeBase::PTR_RIEN)], "pointeur", 0 });
	membres_eini.pousse({ type_pointeur_pour(type_info_type_), "info", 8 });
	type_eini->membres = std::move(membres_eini);

	auto membres_chaine = kuri::tableau<TypeCompose::Membre>();
	membres_chaine.pousse({ types_communs[static_cast<long>(TypeBase::PTR_Z8)], "pointeur", 0 });
	membres_chaine.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	type_chaine->membres = std::move(membres_chaine);
}

Typeuse::~Typeuse()
{
#define DELOGE_TYPES(Type, Tableau) \
	for (auto ptr : Tableau) {\
	memoire::deloge(#Type, ptr); \
}

	DELOGE_TYPES(Type, types_simples);
	DELOGE_TYPES(TypePointeur, types_pointeurs);
	DELOGE_TYPES(TypeReference, types_references);
	DELOGE_TYPES(TypeStructure, types_structures);
	DELOGE_TYPES(TypeEnum, types_enums);
	DELOGE_TYPES(TypeTableauFixe, types_tableaux_fixes);
	DELOGE_TYPES(TypeTableauDynamique, types_tableaux_dynamiques);
	DELOGE_TYPES(TypeFonction, types_fonctions);
	DELOGE_TYPES(TypeVariadique, types_variadiques);
	DELOGE_TYPES(TypeUnion, types_unions);
	DELOGE_TYPES(TypeTypeDeDonnees, types_type_de_donnees);
	DELOGE_TYPES(TypePolymorphique, types_polymorphiques);

	memoire::deloge("TypeCompose", type_eini);
	memoire::deloge("TypeCompose", type_chaine);

#undef DELOGE_TYPES

	memoire::deloge("TypeType", type_type_de_donnees_);
}

Type *Typeuse::type_pour_lexeme(GenreLexeme lexeme)
{
	switch (lexeme) {
		case GenreLexeme::BOOL:
		{
			return types_communs[static_cast<long>(TypeBase::BOOL)];
		}
		case GenreLexeme::OCTET:
		{
			return types_communs[static_cast<long>(TypeBase::OCTET)];
		}
		case GenreLexeme::N8:
		{
			return types_communs[static_cast<long>(TypeBase::N8)];
		}
		case GenreLexeme::Z8:
		{
			return types_communs[static_cast<long>(TypeBase::Z8)];
		}
		case GenreLexeme::N16:
		{
			return types_communs[static_cast<long>(TypeBase::N16)];
		}
		case GenreLexeme::Z16:
		{
			return types_communs[static_cast<long>(TypeBase::Z16)];
		}
		case GenreLexeme::N32:
		{
			return types_communs[static_cast<long>(TypeBase::N32)];
		}
		case GenreLexeme::Z32:
		{
			return types_communs[static_cast<long>(TypeBase::Z32)];
		}
		case GenreLexeme::N64:
		{
			return types_communs[static_cast<long>(TypeBase::N64)];
		}
		case GenreLexeme::Z64:
		{
			return types_communs[static_cast<long>(TypeBase::Z64)];
		}
		case GenreLexeme::R16:
		{
			return types_communs[static_cast<long>(TypeBase::R16)];
		}
		case GenreLexeme::R32:
		{
			return types_communs[static_cast<long>(TypeBase::R32)];
		}
		case GenreLexeme::R64:
		{
			return types_communs[static_cast<long>(TypeBase::R64)];
		}
		case GenreLexeme::CHAINE:
		{
			return types_communs[static_cast<long>(TypeBase::CHAINE)];
		}
		case GenreLexeme::EINI:
		{
			return types_communs[static_cast<long>(TypeBase::EINI)];
		}
		case GenreLexeme::RIEN:
		{
			return types_communs[static_cast<long>(TypeBase::RIEN)];
		}
		case GenreLexeme::TYPE_DE_DONNEES:
		{
			return type_type_de_donnees_;
		}
		default:
		{
			return nullptr;
		}
	}
}

TypePointeur *Typeuse::type_pointeur_pour(Type *type)
{
	PROFILE_FONCTION;

	POUR (types_pointeurs) {
		if (it->type_pointe == type) {
			return it;
		}
	}

	auto resultat = TypePointeur::cree(type);

	graphe.connecte_type_type(resultat, type);

	auto indice = IndiceTypeOp::ENTIER_RELATIF;

	auto const &idx_dt_ptr_nul = types_communs[static_cast<long>(TypeBase::PTR_NUL)];
	auto const &idx_dt_bool = types_communs[static_cast<long>(TypeBase::BOOL)];

	operateurs.ajoute_basique(GenreLexeme::EGALITE, resultat, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, resultat, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::INFERIEUR, resultat, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::INFERIEUR_EGAL, resultat, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::SUPERIEUR, resultat, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::SUPERIEUR_EGAL, resultat, idx_dt_bool, indice);

	/* Pour l'arithmétique de pointeur nous n'utilisons que le type le plus
	 * gros, la résolution de l'opérateur ajoutera une transformation afin
	 * que le type plus petit soit transtyper à la bonne taille. */
	auto idx_type_entier = types_communs[static_cast<long>(TypeBase::Z64)];

	operateurs.ajoute_basique(GenreLexeme::PLUS, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::MOINS, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::MOINS, resultat, resultat, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::PLUS_EGAL, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::MOINS_EGAL, resultat, idx_type_entier, resultat, indice);

	idx_type_entier = types_communs[static_cast<long>(TypeBase::N64)];
	indice = IndiceTypeOp::ENTIER_NATUREL;

	operateurs.ajoute_basique(GenreLexeme::PLUS, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::MOINS, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::PLUS_EGAL, resultat, idx_type_entier, resultat, indice);
	operateurs.ajoute_basique(GenreLexeme::MOINS_EGAL, resultat, idx_type_entier, resultat, indice);

	types_pointeurs.pousse(resultat);

	return resultat;
}

TypeReference *Typeuse::type_reference_pour(Type *type)
{
	PROFILE_FONCTION;

	POUR (types_references) {
		if (it->type_pointe == type) {
			return it;
		}
	}

	auto resultat = TypeReference::cree(type);
	types_references.pousse(resultat);

	graphe.connecte_type_type(resultat, type);

	return resultat;
}

TypeTableauFixe *Typeuse::type_tableau_fixe(Type *type_pointe, long taille)
{
	PROFILE_FONCTION;

	POUR (types_tableaux_fixes) {
		if (it->type_pointe == type_pointe && it->taille == taille) {
			return it;
		}
	}

	// les décalages sont à zéros car ceci n'est pas vraiment une structure
	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 0 });

	auto type = TypeTableauFixe::cree(type_pointe, taille, std::move(membres));

	graphe.connecte_type_type(type, type_pointe);

	types_tableaux_fixes.pousse(type);

	return type;
}

TypeTableauDynamique *Typeuse::type_tableau_dynamique(Type *type_pointe)
{
	PROFILE_FONCTION;

	POUR (types_tableaux_dynamiques) {
		if (it->type_pointe == type_pointe) {
			return it;
		}
	}

	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "capacité", 16 });

	auto type = TypeTableauDynamique::cree(type_pointe, std::move(membres));

	graphe.connecte_type_type(type, type_pointe);

	types_tableaux_dynamiques.pousse(type);

	return type;
}

TypeVariadique *Typeuse::type_variadique(Type *type_pointe)
{
	PROFILE_FONCTION;

	POUR (types_variadiques) {
		if (it->type_pointe == type_pointe) {
			return it;
		}
	}

	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "capacité", 16 });

	auto type = TypeVariadique::cree(type_pointe, std::move(membres));

	graphe.connecte_type_type(type, type_pointe);

	types_variadiques.pousse(type);

	return type;
}

TypeFonction *Typeuse::discr_type_fonction(TypeFonction *it, const kuri::tableau<Type *> &entrees, const kuri::tableau<Type *> &sorties)
{
	if (it->types_entrees.taille != entrees.taille) {
		return nullptr;
	}

	if (it->types_sorties.taille != sorties.taille) {
		return nullptr;
	}

	for (int i = 0; i < it->types_entrees.taille; ++i) {
		if (it->types_entrees[i] != entrees[i]) {
			return nullptr;
		}
	}

	for (int i = 0; i < it->types_sorties.taille; ++i) {
		if (it->types_sorties[i] != sorties[i]) {
			return nullptr;
		}
	}

	return it;
}

TypeFonction *Typeuse::type_fonction(kuri::tableau<Type *> &&entrees, kuri::tableau<Type *> &&sorties)
{
	PROFILE_FONCTION;

	POUR (types_fonctions) {
		auto type = discr_type_fonction(it, entrees, sorties);

		if (type != nullptr) {
			return type;
		}
	}

	auto type = TypeFonction::cree(std::move(entrees), std::move(sorties));

	types_fonctions.pousse(type);

	auto indice = IndiceTypeOp::ENTIER_RELATIF;

	auto const &idx_dt_ptr_nul = types_communs[static_cast<long>(TypeBase::PTR_NUL)];
	auto const &idx_dt_bool = types_communs[static_cast<long>(TypeBase::BOOL)];

	operateurs.ajoute_basique(GenreLexeme::EGALITE, type, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, type, idx_dt_ptr_nul, idx_dt_bool, indice);

	POUR (type->types_entrees) {
		graphe.connecte_type_type(type, it);
	}

	POUR (type->types_sorties) {
		graphe.connecte_type_type(type, it);
	}

	return type;
}

TypeTypeDeDonnees *Typeuse::type_type_de_donnees(Type *type_connu)
{
	PROFILE_FONCTION;

	if (type_connu == nullptr) {
		return type_type_de_donnees_;
	}

	POUR (types_type_de_donnees) {
		if (it->type_connu == type_connu) {
			return it;
		}
	}

	auto type = TypeTypeDeDonnees::cree(type_connu);

	types_type_de_donnees.pousse(type);

	return type;
}

TypeStructure *Typeuse::reserve_type_structure(NoeudStruct *decl)
{
	auto type = memoire::loge<TypeStructure>("TypeStructure");
	type->decl = decl;

	// decl peut être nulle pour la réservation du type pour InfoType
	if (type->decl) {
		type->nom = decl->lexeme->chaine;
	}

	types_structures.pousse(type);

	return type;
}

TypeEnum *Typeuse::reserve_type_enum(NoeudEnum *decl)
{
	auto type = memoire::loge<TypeEnum>("TypeEnum");
	type->nom = decl->lexeme->chaine;
	type->decl = decl;

	types_enums.pousse(type);

	return type;
}

TypeUnion *Typeuse::reserve_type_union(NoeudStruct *decl)
{
	auto type = memoire::loge<TypeUnion>("TypeUnion");
	type->nom = decl->lexeme->chaine;
	type->decl = decl;

	types_unions.pousse(type);

	return type;
}

TypeUnion *Typeuse::union_anonyme(kuri::tableau<TypeCompose::Membre> &&membres)
{
	PROFILE_FONCTION;

	POUR (types_unions) {
		if (!it->est_anonyme) {
			continue;
		}

		if (it->membres.taille != membres.taille) {
			continue;
		}

		auto type_apparie = true;

		for (auto i = 0; i < it->membres.taille; ++i) {
			if (it->membres[i].type != membres[i].type) {
				type_apparie = false;
				break;
			}
		}

		if (type_apparie) {
			return it;
		}
	}

	// À FAIRE : déduplique ça avec la validation sémantique.
	auto max_alignement = 0u;
	auto taille_union = 0u;
	auto type_le_plus_grand = static_cast<Type *>(nullptr);

	POUR (membres) {
		auto type_membre = it.type;
		auto taille = type_membre->taille_octet;
		max_alignement = std::max(taille, max_alignement);

		if (taille > taille_union) {
			type_le_plus_grand = type_membre;
			taille_union = taille;
		}
	}

	/* ajoute une marge d'alignement */
	auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
	taille_union += padding;

	auto decalage_index = taille_union;

	/* ajoute la taille du membre actif */
	taille_union += static_cast<unsigned>(sizeof(int));

	/* ajoute une marge d'alignement finale */
	padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
	taille_union += padding;

	auto type = memoire::loge<TypeUnion>("TypeUnion");
	type->nom = "anonyme";
	type->membres = std::move(membres);
	type->est_anonyme = true;
	type->type_le_plus_grand = type_le_plus_grand;
	type->decalage_index = decalage_index;
	type->taille_octet = taille_union;
	type->alignement = max_alignement;

	types_unions.pousse(type);

	return type;
}

TypeEnum *Typeuse::reserve_type_erreur(NoeudEnum *decl)
{
	auto type = reserve_type_enum(decl);
	type->genre = GenreType::ERREUR;

	return type;
}

TypePolymorphique *Typeuse::cree_polymorphique(IdentifiantCode *ident)
{
	PROFILE_FONCTION;

	POUR (types_polymorphiques) {
		if (it->ident == ident) {
			return it;
		}
	}

	auto type = TypePolymorphique::cree(ident);
	types_polymorphiques.pousse(type);
	return type;
}

size_t Typeuse::memoire_utilisee() const
{
	auto memoire = 0ul;

#define COMPTE_MEMOIRE(Type, Tableau) \
	memoire += static_cast<size_t>(Tableau.taille()) * (sizeof(Type *) + sizeof(Type))

	COMPTE_MEMOIRE(Type, types_simples);
	COMPTE_MEMOIRE(TypePointeur, types_pointeurs);
	COMPTE_MEMOIRE(TypeReference, types_references);
	COMPTE_MEMOIRE(TypeStructure, types_structures);
	COMPTE_MEMOIRE(TypeEnum, types_enums);
	COMPTE_MEMOIRE(TypeTableauFixe, types_tableaux_fixes);
	COMPTE_MEMOIRE(TypeTableauDynamique, types_tableaux_dynamiques);
	COMPTE_MEMOIRE(TypeFonction, types_fonctions);
	COMPTE_MEMOIRE(TypeVariadique, types_variadiques);
	COMPTE_MEMOIRE(TypeUnion, types_unions);
	COMPTE_MEMOIRE(TypeTypeDeDonnees, types_type_de_donnees);
	COMPTE_MEMOIRE(TypePolymorphique, types_polymorphiques);

#undef COMPTE_MEMOIRE

	memoire += 2 * (sizeof(TypeCompose) + sizeof(TypeCompose *)); // chaine et eini

	// les types communs sont dans les types simples, ne comptons que la mémoire du tableau
	memoire += static_cast<size_t>(types_communs.taille()) * sizeof(Type *);

	return memoire;
}

long Typeuse::nombre_de_types() const
{
	auto compte = 0l;
	compte += types_simples.taille();
	compte += types_pointeurs.taille();
	compte += types_references.taille();
	compte += types_structures.taille();
	compte += types_enums.taille();
	compte += types_tableaux_fixes.taille();
	compte += types_tableaux_dynamiques.taille();
	compte += types_fonctions.taille();
	compte += types_variadiques.taille();
	compte += types_unions.taille();
	compte += types_type_de_donnees.taille();
	compte += types_polymorphiques.taille();
	compte += 2; // eini et chaine
	return compte;
}

/* ************************************************************************** */

dls::chaine chaine_type(const Type *type)
{
	if (type == nullptr) {
		return "nul";
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			return "invalide";
		}
		case GenreType::EINI:
		{
			return "eini";
		}
		case GenreType::CHAINE:
		{
			return "chaine";
		}
		case GenreType::RIEN:
		{
			return "rien";
		}
		case GenreType::BOOL:
		{
			return "bool";
		}
		case GenreType::OCTET:
		{
			return "octet";
		}
		case GenreType::ENTIER_CONSTANT:
		{
			return "entier_constant";
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				return "n8";
			}

			if (type->taille_octet == 2) {
				return "n16";
			}

			if (type->taille_octet == 4) {
				return "n32";
			}

			if (type->taille_octet == 8) {
				return "n64";
			}

			return "invalide";
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				return "z8";
			}

			if (type->taille_octet == 2) {
				return "z16";
			}

			if (type->taille_octet == 4) {
				return "z32";
			}

			if (type->taille_octet == 8) {
				return "z64";
			}

			return "invalide";
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return "r16";
			}

			if (type->taille_octet == 4) {
				return "r32";
			}

			if (type->taille_octet == 8) {
				return "r64";
			}

			return "invalide";
		}
		case GenreType::REFERENCE:
		{
			return "&" + chaine_type(static_cast<TypeReference const *>(type)->type_pointe);
		}
		case GenreType::POINTEUR:
		{
			return "*" + chaine_type(static_cast<TypePointeur const *>(type)->type_pointe);
		}
		case GenreType::UNION:
		{
			return static_cast<TypeUnion const *>(type)->nom;
		}
		case GenreType::STRUCTURE:
		{
			return static_cast<TypeStructure const *>(type)->nom;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			return "[]" + chaine_type(static_cast<TypeTableauDynamique const *>(type)->type_pointe);
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = static_cast<TypeTableauFixe const *>(type);

			auto res = dls::chaine("[");
			res += dls::vers_chaine(type_tabl->taille);
			res += "]";

			return res + chaine_type(type_tabl->type_pointe);
		}
		case GenreType::VARIADIQUE:
		{
			return "..." + chaine_type(static_cast<TypeVariadique const *>(type)->type_pointe);
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction const *>(type);

			auto res = dls::chaine("fonc");

			auto virgule = '(';

			POUR (type_fonc->types_entrees) {
				res += virgule;
				res += chaine_type(it);
				virgule = ',';
			}

			if (type_fonc->types_entrees.taille == 0) {
				res += virgule;
			}

			res += ')';

			virgule = '(';

			POUR (type_fonc->types_sorties) {
				res += virgule;
				res += chaine_type(it);
				virgule = ',';
			}

			res += ')';

			return res;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			return static_cast<TypeEnum const *>(type)->nom;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			return "type_de_données";
		}
		case GenreType::POLYMORPHIQUE:
		{
			auto type_polymorphique = static_cast<TypePolymorphique const *>(type);
			auto res = dls::chaine("$");
			return res + type_polymorphique->ident->nom;
		}
	}

	return "";
}

Type *type_dereference_pour(Type *type)
{
	PROFILE_FONCTION;

	if (type->genre == GenreType::TABLEAU_FIXE) {
		return static_cast<TypeTableauFixe *>(type)->type_pointe;
	}

	if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
		return static_cast<TypeTableauDynamique *>(type)->type_pointe;
	}

	if (type->genre == GenreType::POINTEUR) {
		return static_cast<TypePointeur *>(type)->type_pointe;
	}

	if (type->genre == GenreType::REFERENCE) {
		return static_cast<TypeReference *>(type)->type_pointe;
	}

	if (type->genre == GenreType::VARIADIQUE) {
		return static_cast<TypeVariadique *>(type)->type_pointe;
	}

	return nullptr;
}

void rassemble_noms_type_polymorphique(Type *type, kuri::tableau<dls::vue_chaine_compacte> &noms)
{
	if (type->genre == GenreType::FONCTION) {
		auto type_fonction = static_cast<TypeFonction *>(type);

		POUR (type_fonction->types_entrees) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				rassemble_noms_type_polymorphique(it, noms);
			}
		}

		POUR (type_fonction->types_sorties) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				rassemble_noms_type_polymorphique(it, noms);
			}
		}

		return;
	}

	while (type->genre != GenreType::POLYMORPHIQUE) {
		type = type_dereference_pour(type);
	}

	noms.pousse(static_cast<TypePolymorphique *>(type)->ident->nom);
}

bool est_type_conditionnable(Type *type)
{
	return dls::outils::est_element(
				type->genre,
				GenreType::BOOL,
				GenreType::CHAINE,
				GenreType::ENTIER_CONSTANT,
				GenreType::ENTIER_NATUREL,
				GenreType::ENTIER_RELATIF,
				GenreType::FONCTION,
				GenreType::POINTEUR,
				GenreType::TABLEAU_DYNAMIQUE);
}
