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

#include "broyage.hh"
#include "contexte_generation_code.h"
#include "typage.hh"

struct ContexteGenerationCode;

#undef UTILISE_ENCHAINEUSE

#ifdef UTILISE_ENCHAINEUSE
struct Enchaineuse {
	static constexpr auto TAILLE_TAMPON = 16 * 1024;

	using type_tampon = dls::tableau<char>;

	dls::tableau<type_tampon> m_tampons;
	long tampon_courant = 0;

	Enchaineuse()
	{
		ajoute_tampon();
	}

	void pousse(dls::vue_chaine const &chn)
	{
		auto &tampon = m_tampons[tampon_courant];

		for (auto c : chn) {
			tampon.pousse(c);

			if (tampon.taille() == TAILLE_TAMPON) {
				ajoute_tampon();
				tampon = m_tampons[tampon_courant];
			}
		}
	}

private:
	void ajoute_tampon()
	{
		m_tampons.pousse(type_tampon());
		m_tampons.back().reserve(TAILLE_TAMPON);
		tampon_courant = m_tampons.taille() - 1;
	}
};
#endif

struct GeneratriceCodeC {
	ContexteGenerationCode &contexte;
	dls::flux_chaine &os;

#ifdef UTILISE_ENCHAINEUSE
	Enchaineuse m_enchaineuse{};
#endif

	GeneratriceCodeC(ContexteGenerationCode &ctx, dls::flux_chaine &flux)
		: contexte(ctx)
		, os(flux)
	{}

	void declare_variable(Type *type, dls::chaine const &nom, dls::chaine const &expr)
	{
		os << nom_broye_type(type, true) << " " << nom;

		if (!expr.est_vide()) {
			os << " = " << expr;
		}

		os << ";\n";

#ifdef UTILISE_ENCHAINEUSE
		m_enchaineuse.pousse(nom_broye_type(type));
		m_enchaineuse.pousse(" ");
		m_enchaineuse.pousse(nom);

		if (!expr.est_vide()) {
			m_enchaineuse.pousse(" = ");
			m_enchaineuse.pousse(expr);
		}

		m_enchaineuse.pousse(";\n");
#endif
	}

	dls::chaine declare_variable_temp(Type *type, int index_var)
	{
		auto nom_temp = "__var_temp" + dls::vers_chaine(index_var);
		os << nom_broye_type(type, true) << " " << nom_temp << ";\n";

#ifdef UTILISE_ENCHAINEUSE
		m_enchaineuse.pousse(nom_broye_type(type));
		m_enchaineuse.pousse(" ");
		m_enchaineuse.pousse(nom_temp);
		m_enchaineuse.pousse(";\n");
#endif

		return nom_temp;
	}

	dls::chaine expression_malloc(Type *type, dls::chaine const &expr)
	{
		auto flux = dls::flux_chaine();
		flux << "(";
		os << nom_broye_type(type, true);
		flux << ")(malloc(" << expr << "))";

		return flux.chn();
	}
};

template <typename T>
GeneratriceCodeC &operator << (GeneratriceCodeC &generatrice, T const &valeur)
{
	generatrice.os << valeur;
	return generatrice;
}
