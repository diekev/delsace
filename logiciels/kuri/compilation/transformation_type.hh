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

#include "biblinternes/structures/vue_chaine_compacte.hh"

struct ContexteGenerationCode;
struct NoeudDeclarationFonction;
struct Type;

namespace noeud {
struct ContexteValidationCode;
}

enum class TypeTransformation {
	INUTILE,
	IMPOSSIBLE,

	CONSTRUIT_UNION,
	EXTRAIT_UNION,
	CONSTRUIT_EINI,
	EXTRAIT_EINI,
	CONSTRUIT_TABL_OCTET,
	CONVERTI_TABLEAU,
	FONCTION,
	PREND_REFERENCE,
	DEREFERENCE,
	AUGMENTE_TAILLE_TYPE,
	CONVERTI_VERS_BASE,
	CONVERTI_ENTIER_CONSTANT,
	CONVERTI_VERS_PTR_RIEN,
	CONVERTI_VERS_TYPE_CIBLE,
};

const char *chaine_transformation(TypeTransformation type);

struct TransformationType {
	TypeTransformation type{};
	NoeudDeclarationFonction const *fonction{};
	Type *type_cible = nullptr;
	long index_membre = 0;

	TransformationType() = default;

	TransformationType(TypeTransformation type_)
		: type(type_)
	{}

	TransformationType(TypeTransformation type_, Type *type_cible_, long index_membre_)
		: type(type_)
		, type_cible(type_cible_)
		, index_membre(index_membre_)
	{}

	TransformationType(TypeTransformation type_, Type *type_cible_)
		: type(type_)
		, type_cible(type_cible_)
	{}

	TransformationType(NoeudDeclarationFonction const *fonction_)
		: type(TypeTransformation::FONCTION)
		, fonction(fonction_)
	{}

	TransformationType(NoeudDeclarationFonction const *fonction_, Type *type_cible_)
		: type(TypeTransformation::FONCTION)
		, fonction(fonction_)
		, type_cible(type_cible_)
	{}
};

TransformationType cherche_transformation(
		ContexteGenerationCode &contexte,
		noeud::ContexteValidationCode &contexte_validation,
		Type *type_de,
		Type *type_vers);
