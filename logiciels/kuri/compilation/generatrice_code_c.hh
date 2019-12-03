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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "contexte_generation_code.h"
#include "donnees_type.h"

struct ContexteGenerationCode;

struct GeneratriceCodeC {
	ContexteGenerationCode &contexte;
	dls::flux_chaine &os;

	GeneratriceCodeC(ContexteGenerationCode &ctx, dls::flux_chaine &flux)
		: contexte(ctx)
		, os(flux)
	{}

	void declare_variable(long type, dls::chaine const &nom, dls::chaine const &expr)
	{
		declare_variable(contexte.magasin_types.donnees_types[type], nom, expr);
	}

	void declare_variable(DonneesTypeFinal const &type, dls::chaine const &nom, dls::chaine const &expr)
	{
		contexte.magasin_types.converti_type_C(
					contexte,
					"",
					type.plage(),
					os);

		os << " " << nom;

		if (!expr.est_vide()) {
			os << " = " << expr;
		}

		os << ";\n";
	}

	dls::chaine expression_malloc(DonneesTypeFinal const &type, dls::chaine const &expr)
	{
		auto flux = dls::flux_chaine();
		flux << "(";
		contexte.magasin_types.converti_type_C(
					contexte,
					"",
					type.plage(),
					flux);
		flux << ")(malloc(" << expr << "))";

		return flux.chn();
	}
};
