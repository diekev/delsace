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
		DonneesTypeFinal &dt)
{
	if (dt.nom_broye == "") {
		auto flux = dls::flux_chaine();
		auto plage = dt.plage();

		while (!plage.est_finie()) {
			auto donnee = plage.front();
			plage.effronte();

			switch (donnee & 0xff) {
				case id_morceau::TROIS_POINTS:
				{
					flux << "Kv";
					break;
				}
				case id_morceau::POINTEUR:
				{
					flux << "KP";
					break;
				}
				case id_morceau::TABLEAU:
				{
					if (static_cast<size_t>(donnee >> 8) != 0) {
						flux << "KT" << static_cast<size_t>(donnee >> 8);
					}
					else {
						flux << "Kt";
					}

					break;
				}
				case id_morceau::N8:
				{
					flux << "Ksn8";
					break;
				}
				case id_morceau::N16:
				{
					flux << "Ksn16";
					break;
				}
				case id_morceau::N32:
				{
					flux << "Ksn32";
					break;
				}
				case id_morceau::N64:
				{
					flux << "Ksn64";
					break;
				}
				case id_morceau::N128:
				{
					flux << "Ksn128";
					break;
				}
				case id_morceau::R16:
				{
					flux << "Ksr16";
					break;
				}
				case id_morceau::R32:
				{
					flux << "Ksr32";
					break;
				}
				case id_morceau::R64:
				{
					flux << "Ksr64";
					break;
				}
				case id_morceau::R128:
				{
					flux << "Ksr128";
					break;
				}
				case id_morceau::Z8:
				{
					flux << "Ksz8";
					break;
				}
				case id_morceau::Z16:
				{
					flux << "Ksz16";
					break;
				}
				case id_morceau::Z32:
				{
					flux << "Ksz32";
					break;
				}
				case id_morceau::Z64:
				{
					flux << "Ksz64";
					break;
				}
				case id_morceau::Z128:
				{
					flux << "Ksz128";
					break;
				}
				case id_morceau::BOOL:
				{
					flux << "Ksbool";
					break;
				}
				case id_morceau::CHAINE:
				{
					flux << "Kschaine";
					break;
				}
				case id_morceau::FONC:
				{
					/* À FAIRE gestion des paramètres */
					flux << "Kf";
					break;
				}
				case id_morceau::COROUT:
				{
					flux << "Kc";
					break;
				}
				case id_morceau::PARENTHESE_OUVRANTE:
				case id_morceau::PARENTHESE_FERMANTE:
				case id_morceau::VIRGULE:
				case id_morceau::EINI:
				{
					flux << "Kseini";
					break;
				}
				case id_morceau::RIEN:
				{
					flux << "Ksrien";
					break;
				}
				case id_morceau::OCTET:
				{
					flux << "Ksoctet";
					break;
				}
				case id_morceau::NUL:
				{
					flux << "Ksnul";
					break;
				}
				case id_morceau::REFERENCE:
				{
					flux << "KR";
					break;
				}
				case id_morceau::CHAINE_CARACTERE:
				{
					auto id = static_cast<long>(donnee >> 8);
					flux << "Ks";
					flux << broye_nom_simple(contexte.nom_struct(id));
					break;
				}
				default:
				{
					flux << "INVALIDE";
					break;
				}
			}
		}

		dt.nom_broye = flux.chn();
	}

	return dt.nom_broye;
}

dls::chaine const &nom_broye_type(
		ContexteGenerationCode &contexte,
		long index_type)
{
	auto &dt = contexte.typeuse[index_type];
	return nom_broye_type(contexte, dt);
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

		auto const &nom_broye = nom_broye_type(contexte, arg.index_type);
		ret += dls::vers_chaine(nom_broye.taille());
		ret += nom_broye;
	}

	/* sorties */
	ret += "_S";
	ret += dls::vers_chaine(df.idx_types_retours.taille());
	ret += "_";

	for (auto const &idx : df.idx_types_retours) {
		auto const &nom_broye = nom_broye_type(contexte, idx);
		ret += dls::vers_chaine(nom_broye.taille());
		ret += nom_broye;
	}

	return ret;
}
