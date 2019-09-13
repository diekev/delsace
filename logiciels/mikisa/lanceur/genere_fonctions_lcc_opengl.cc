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

/**
 * Outil qui a servi à générer la première version des fichiers d'implémentation
 * GLSL des fonctions LCC. Gardé pour l'historique.
 *
 * Ceci ne fait que générer les déclarations des fonctions dans deux fichiers
 * différents : un pour les fonctions « normales », et un autre pour les
 * fonctions « polymorphiques ».
 */

#include <cstring>
#include <fstream>

#include "biblinternes/outils/conditions.h"
#include "biblinternes/langage/unicode.hh"

#include "lcc/lcc.hh"

static auto type_var_opengl(lcc::type_var type_lcc)
{
	switch (type_lcc) {
		case lcc::type_var::DEC:
			return "float";
		case lcc::type_var::VEC2:
			return "vec2";
		case lcc::type_var::VEC3:
			return "vec3";
		case lcc::type_var::VEC4:
			return "vec4";
		case lcc::type_var::MAT3:
			return "mat3";
		case lcc::type_var::MAT4:
			return "mat4";
		case lcc::type_var::ENT32:
			return "int";
		case lcc::type_var::CHAINE:
			return "void";
		case lcc::type_var::INVALIDE:
			return "void";
		case lcc::type_var::TABLEAU:
			return "void";
		case lcc::type_var::POLYMORPHIQUE:
			return "void";
		case lcc::type_var::COULEUR:
			return "vec4";
	}

	return "erreur";
}

static std::pair<const char *, char> paires_caracteres_sans_accent[] = {
	{ "à", 'a' },
	{ "â", 'a' },
	{ "é", 'e' },
	{ "è", 'e' },
	{ "ê", 'e' },
	{ "î", 'i' },
	{ "ô", 'o' },
};

static auto supprime_accents(dls::chaine const &chaine)
{
	auto res = dls::chaine();

	for (auto i = 0; i < chaine.taille();) {
		auto n = lng::nombre_octets(&chaine[i]);

		if (n == 1) {
			res += chaine[i];
		}
		else {
			for (auto const &paire : paires_caracteres_sans_accent) {
				if (strncmp(&chaine[i], paire.first, static_cast<size_t>(n)) == 0) {
					res += paire.second;
					break;
				}
			}
		}

		i += n;
	}

	return res;
}

int main()
{
	std::ios::sync_with_stdio(false);

	auto lcc = lcc::LCC();
	lcc::initialise(lcc);

	auto fonc_normales = dls::tableau<std::pair<dls::chaine, lcc::donnees_fonction const *>>();
	auto fonc_polymorphiques = dls::tableau<std::pair<dls::chaine, lcc::donnees_fonction const *>>();

	for (auto const &paire_df : lcc.fonctions.table) {
		for (auto const &df : paire_df.second) {
			if (!dls::outils::possede_drapeau(df.ctx, lcc::ctx_script::detail)) {
				continue;
			}

			auto est_polymorphique = false;

			for (auto i = 0; i < df.seing.entrees.taille(); ++i) {
				if (df.seing.entrees.type(i) == lcc::type_var::POLYMORPHIQUE) {
					est_polymorphique = true;
					break;
				}
			}

			if (est_polymorphique) {
				fonc_polymorphiques.pousse({ paire_df.first, &df });
			}
			else {
				fonc_normales.pousse({ paire_df.first, &df });
			}
		}
	}

	auto os = std::ofstream("/tmp/fonctions_normales.glsl");

	for (auto paire_df : fonc_normales) {
		auto df = paire_df.second;
		auto const &entrees = df->seing.entrees;
		auto const &sorties = df->seing.sorties;

		os << "void " << supprime_accents(paire_df.first);

		auto virgule = '(';

		for (auto i = 0; i < entrees.taille(); ++i) {
			os << virgule << "in " << type_var_opengl(entrees.type(i)) << ' ' << supprime_accents(entrees.nom(i));
			virgule = ',';
		}

		for (auto i = 0; i < sorties.taille(); ++i) {
			os << virgule << "out " << type_var_opengl(sorties.type(i)) << ' ' << supprime_accents(sorties.nom(i));
		}

		os << ")\n{\n}\n\n";
	}

	os = std::ofstream("/tmp/fonctions_polymorphiques.glsl");

	for (auto paire_df : fonc_polymorphiques) {
		auto df = paire_df.second;
		auto const &entrees = df->seing.entrees;
		auto const &sorties = df->seing.sorties;

		os << "void " << supprime_accents(paire_df.first);

		auto virgule = '(';

		for (auto i = 0; i < entrees.taille(); ++i) {
			os << virgule << "in TYPE_POLY" << ' ' << supprime_accents(entrees.nom(i));
			virgule = ',';
		}

		for (auto i = 0; i < sorties.taille(); ++i) {
			os << virgule << "out TYPE_POLY" << ' ' << supprime_accents(sorties.nom(i));
		}

		os << ")\n{\n}\n\n";
	}

	return 0;
}
