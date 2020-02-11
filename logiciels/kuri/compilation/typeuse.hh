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

#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tableau.hh"

#include "donnees_type.h"
#include "graphe_dependance.hh"

struct Operateurs;

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
	REF_NUL,
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

struct Typeuse {
	GrapheDependance &graphe;
	Operateurs &operateurs;

	Typeuse(GrapheDependance &g, Operateurs &o);

	struct Indexeuse {
		dls::dico_desordonne<DonneesTypeFinal, long> donnees_type_index{};
		dls::tableau<DonneesTypeFinal> donnees_types{};
		dls::tableau<long> index_types_communs{};

		long trouve_index(DonneesTypeFinal const &dt);

		long index_type(DonneesTypeFinal const &dt);

		inline long operator[](TypeBase type_base) const
		{
			return index_types_communs[static_cast<long>(type_base)];
		}
	};

	Indexeuse indexeuse{};

	long ajoute_type(DonneesTypeFinal const &donnees);

	long type_tableau_pour(long index_type);

	long type_reference_pour(long index_type);

	long type_dereference_pour(long index_type);

	long type_pointeur_pour(long index_type);

	inline long nombre_de_types() const
	{
		return indexeuse.donnees_types.taille();
	}

	inline long operator[](TypeBase type_base) const
	{
		return indexeuse[type_base];
	}

	inline DonneesTypeFinal &operator[](long index_type)
	{
		return indexeuse.donnees_types[index_type];
	}

	inline DonneesTypeFinal const &operator[](long index_type) const
	{
		return indexeuse.donnees_types[index_type];
	}

private:
	void initialise_relations_pour_type(DonneesTypeFinal const &dt, long i);

	/* Puisque "ajoute_type" crée les relations dans le graphe entre le type et
	 * ses dérivées (pointeurs, références, etc.), et puisque la création des
	 * relations invoque à son tour "ajoute_type", nous pouvons nous retrouver
	 * dans une boucle infinie. Cette fonction ne fait qu'ajouter un type sans
	 * les relations.
	 */
	long ajoute_type_sans_relations(DonneesTypeFinal const &donnees);
};

/* ************************************************************************** */

[[nodiscard]] auto donnees_types_parametres(
		Typeuse &typeuse,
		const DonneesTypeFinal &donnees_type,
		long &nombre_types_retour) noexcept(false) -> dls::tableau<long>;
