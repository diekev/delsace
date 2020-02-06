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

enum class TypeTransformation {
	INUTILE,
	IMPOSSIBLE,

	CONSTRUIT_EINI,
	CONSTRUIT_EINI_TABLEAU,
	EXTRAIT_EINI,
	CONSTRUIT_TABL_OCTET,
	PREND_PTR_RIEN,
	CONVERTI_TABLEAU,
	FONCTION,
	PREND_REFERENCE,
	DEREFERENCE,
	AUGMENTE_TAILLE_TYPE,
};

const char *chaine_transformation(TypeTransformation type);

struct TransformationType {
	TypeTransformation type{};
	dls::vue_chaine_compacte nom_fonction{};
	long index_type_cible = -1;

	TransformationType() = default;

	TransformationType(TypeTransformation type_)
		: type(type_)
	{}

	TransformationType(TypeTransformation type_, long idx_type)
		: type(type_)
		, index_type_cible(idx_type)
	{}

	TransformationType(dls::vue_chaine_compacte nom_fonction_)
		: type(TypeTransformation::FONCTION)
		, nom_fonction(nom_fonction_)
	{}

	TransformationType(const char *nom_fonction_)
		: type(TypeTransformation::FONCTION)
		, nom_fonction(nom_fonction_)
	{}
};

TransformationType cherche_transformation(
		ContexteGenerationCode const &contexte,
		long type_de,
		long type_vers);
