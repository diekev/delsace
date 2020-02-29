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

#include "broyage.hh"

#include "biblinternes/langage/unicode.hh"

#include "biblinternes/structures/flux_chaine.hh"

#include "contexte_generation_code.h"
#include "modules.hh"

static char char_depuis_hex(char hex)
{
	return "0123456789ABCDEF"[static_cast<int>(hex)];
}

dls::chaine broye_nom_simple(dls::vue_chaine_compacte const &nom)
{
	auto ret = dls::chaine{};

	auto debut = &nom[0];
	auto fin   = &nom[nom.taille()];

	while (debut < fin) {
		auto no = lng::nombre_octets(debut);

		switch (no) {
			case 0:
			{
				debut += 1;
				break;
			}
			case 1:
			{
				ret += *debut;
				break;
			}
			default:
			{
				for (int i = 0; i < no; ++i) {
					ret += 'x';
					ret += char_depuis_hex(static_cast<char>((debut[i] & 0xf0) >> 4));
					ret += char_depuis_hex(static_cast<char>(debut[i] & 0x0f));
				}

				break;
			}
		}

		debut += no;
	}

	return ret;
}

/* Broye le nom d'un type.
 *
 * Convention :
 * Kv : argument variadique
 * KP : pointeur
 * KR : référence
 * KT : tableau fixe, suivi de la taille
 * Kt : tableau dynamique
 * Ks : introduit un type scalaire, suivi de la chaine du type
 * Kf : fonction
 * Kc : coroutine
 *
 * Un type scalaire est un type de base, ou un type du programme.
 *
 * Exemples :
 * *z8 devient KPKsz8
 * &[]Foo devient KRKtKsFoo
 */
dls::chaine const &nom_broye_type(
		ContexteGenerationCode &contexte,
		Type *type,
		bool pour_generation_code_c)
{
	if (type->nom_broye != "" && type->genre != GenreType::ENUM) {
		return type->nom_broye;
	}

	auto flux = dls::flux_chaine();

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			flux << "Ksinvalide";
			break;
		}
		case GenreType::EINI:
		{
			flux << "Kseini";
			break;
		}
		case GenreType::CHAINE:
		{
			flux << "Kschaine";
			break;
		}
		case GenreType::RIEN:
		{
			flux << "Ksrien";
			break;
		}
		case GenreType::BOOL:
		{
			flux << "Ksbool";
			break;
		}
		case GenreType::OCTET:
		{
			flux << "Ksoctet";
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				flux << "Ksn8";
			}
			else if (type->taille_octet == 2) {
				flux << "Ksn16";
			}
			else if (type->taille_octet == 4) {
				flux << "Ksn32";
			}
			else if (type->taille_octet == 8) {
				flux << "Ksn64";
			}

			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				flux << "Ksz8";
			}
			else if (type->taille_octet == 2) {
				flux << "Ksz16";
			}
			else if (type->taille_octet == 4) {
				flux << "Ksz32";
			}
			else if (type->taille_octet == 8) {
				flux << "Ksz64";
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				flux << "Ksr16";
			}
			else if (type->taille_octet == 4) {
				flux << "Ksr32";
			}
			else if (type->taille_octet == 8) {
				flux << "Ksr64";
			}

			break;
		}
		case GenreType::REFERENCE:
		{
			flux << "KR";
			flux << nom_broye_type(contexte, static_cast<TypeReference *>(type)->type_pointe, pour_generation_code_c);
			break;
		}
		case GenreType::POINTEUR:
		{
			flux << "KP";

			auto type_pointe = static_cast<TypePointeur *>(type)->type_pointe;

			if (type_pointe == nullptr) {
				flux << "nul";
			}
			else {
				flux << nom_broye_type(contexte, type_pointe, pour_generation_code_c);
			}

			break;
		}
		case GenreType::UNION:
		{
			flux << "Ks";
			flux << broye_nom_simple(static_cast<TypeStructure const *>(type)->nom);
			break;
		}
		case GenreType::STRUCTURE:
		{
			flux << "Ks";
			flux << broye_nom_simple(static_cast<TypeStructure const *>(type)->nom);
			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_pointe = static_cast<TypeVariadique *>(type)->type_pointe;

			// les arguments variadiques sont transformés en tableaux, donc utilise Kt
			if (type_pointe != nullptr) {
				flux << "Kt";
				flux << nom_broye_type(contexte, type_pointe, pour_generation_code_c);
			}
			else {
				flux << "Kv";
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			flux << "Kt";
			flux << nom_broye_type(contexte, static_cast<TypeTableauDynamique *>(type)->type_pointe, pour_generation_code_c);
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = static_cast<TypeTableauFixe *>(type);

			flux << "KT";
			flux << type_tabl->taille;
			flux << nom_broye_type(contexte, static_cast<TypeTableauFixe *>(type)->type_pointe, pour_generation_code_c);
			break;
		}
		case GenreType::FONCTION:
		{
			// À FAIRE : types des paramètres, coroutine
			flux << "Kf";
			break;
		}
		case GenreType::ENUM:
		{
			auto type_enum = static_cast<TypeEnum const *>(type);

			if (pour_generation_code_c) {
				assert(type_enum->type_donnees);
				flux << nom_broye_type(contexte, type_enum->type_donnees, pour_generation_code_c);
			}
			else {
				flux << "Ks";
				flux << broye_nom_simple(static_cast<TypeEnum const *>(type)->nom);
			}

			break;
		}
	}

	type->nom_broye = flux.chn();

	return type->nom_broye;
}

/* format :
 * _K préfixe pour tous les noms de Kuri
 * C, E, F, S, U : coroutine, énum, fonction, structure, ou union
 * paire(s) : longueur + chaine ascii pour module et nom
 *
 * pour les fonctions :
 * _E[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 * paire(s) : noms des paramètres et de leurs types, préfixés par leurs tailles
 * _S[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 *
 * types des paramètres pour les fonctions
 *
 * fonc test(x : z32) : z32 (module Test)
 * -> _KF4Test4test_P2_E1_1x3z32_S1_3z32
 */
dls::chaine broye_nom_fonction(
		ContexteGenerationCode &contexte,
		DonneesFonction const &df,
		dls::vue_chaine_compacte const &nom_fonction,
		dls::chaine const &nom_module)
{
	auto type_fonc = df.type;
	auto ret = dls::chaine("_K");

	ret += df.est_coroutine ? "C" : "F";

	/* module */
	auto nom_ascii = broye_nom_simple(nom_module);

	ret += dls::vers_chaine(nom_ascii.taille());
	ret += nom_ascii;

	/* nom de la fonction */
	nom_ascii = broye_nom_simple(nom_fonction);

	ret += dls::vers_chaine(nom_ascii.taille());
	ret += nom_ascii;

	/* paramètres */

	/* entrées */
	ret += "_E";
	ret += dls::vers_chaine(df.args.taille());
	ret += "_";

	for (auto const &arg : df.args) {
		nom_ascii = broye_nom_simple(arg.nom);
		ret += dls::vers_chaine(nom_ascii.taille());
		ret += nom_ascii;

		auto const &nom_broye = nom_broye_type(contexte, arg.type, false);
		ret += dls::vers_chaine(nom_broye.taille());
		ret += nom_broye;
	}

	/* sorties */
	ret += "_S";
	ret += dls::vers_chaine(type_fonc->types_sorties.taille);
	ret += "_";

	POUR (type_fonc->types_sorties) {
		auto const &nom_broye = nom_broye_type(contexte, it, false);
		ret += dls::vers_chaine(nom_broye.taille());
		ret += nom_broye;
	}

	return ret;
}
