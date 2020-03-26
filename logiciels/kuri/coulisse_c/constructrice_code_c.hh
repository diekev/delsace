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

#include "biblinternes/outils/enchaineuse.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "broyage.hh"
#include "contexte_generation_code.h"
#include "typage.hh"

struct ContexteGenerationCode;

struct ConstructriceCodeC {
	ContexteGenerationCode &contexte;
	Enchaineuse m_enchaineuse{};

	explicit ConstructriceCodeC(ContexteGenerationCode &ctx)
		: contexte(ctx)
	{}

	void declare_variable(Type *type, dls::chaine const &nom, dls::chaine const &expr)
	{
		m_enchaineuse.pousse(nom_broye_type(type));
		m_enchaineuse.pousse(" ");
		m_enchaineuse.pousse(nom);

		if (!expr.est_vide()) {
			m_enchaineuse.pousse(" = ");
			m_enchaineuse.pousse(expr);
		}

		m_enchaineuse.pousse(";\n");
	}

	dls::chaine declare_variable_temp(Type *type, int index_var)
	{
		auto nom_temp = "__var_temp" + dls::vers_chaine(index_var);

		m_enchaineuse.pousse(nom_broye_type(type));
		m_enchaineuse.pousse(" ");
		m_enchaineuse.pousse(nom_temp);
		m_enchaineuse.pousse(";\n");

		return nom_temp;
	}

	dls::chaine expression_malloc(Type *type, dls::chaine const &expr)
	{
		auto flux = dls::flux_chaine();
		flux << "(";
		flux << nom_broye_type(type);
		flux << ")(malloc(" << expr << "))";

		return flux.chn();
	}

	void imprime_dans_flux(std::ostream &flux);
};

template <typename T>
ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, T const &valeur)
{
	dls::flux_chaine flux;
	flux << valeur;

	for (auto c : flux.chn()) {
		constructrice.m_enchaineuse.pousse_caractere(c);
	}

	return constructrice;
}

template <size_t N>
ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, const char (&c)[N])
{
	constructrice.m_enchaineuse.pousse(c, static_cast<long>(N));
	return constructrice;
}

ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, dls::vue_chaine_compacte const &chn);

ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, dls::chaine const &chn);

ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, const char *chn);
