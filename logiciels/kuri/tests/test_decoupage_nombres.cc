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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "test_decoupage_nombres.h"

#include <cstring>

#include "biblinternes/langage/nombres.hh"

#include "compilation/contexte_generation_code.h"  // pour DonneesModule
#include "compilation/lexeuse.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/lexemes.hh"

using denombreuse = lng::decoupeuse_nombre<GenreLexeme>;

void test_decoupage_nombre_decimal(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto const tampon = "0.5 ";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 3);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0.5"));

		auto nombre = denombreuse::converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleuse, nombre, 0.5);
	}

	{
		auto const tampon = "0.559_57";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 8);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0.559_57"));

		auto nombre = denombreuse::converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleuse, nombre, 0.55957);
	}

	{
		auto const tampon = "100000+";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 6);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("100000"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 100000l);
	}

	{
		auto const tampon = "1_234_567_890 ";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 13);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("1_234_567_890"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 1234567890l);
	}
}

void test_decoupage_nombre_binaire(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto const tampon = "0b1001100+";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 9);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0b1001100"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 76l);
	}

	{
		auto const tampon = "0B1001_1011 ";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 11);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0B1001_1011"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 155l);
	}
}

void test_decoupage_nombre_octal(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto const tampon = "0o1234567+";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 9);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0o1234567"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 342391l);
	}

	{
		auto const tampon = "0O01_23_45_67 ";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 13);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0O01_23_45_67"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 342391l);
	}
}

void test_decoupage_nombre_hexadecimal(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto const tampon = "0xff38ce+";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 8);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0xff38ce"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 0xff38cel);
	}

	{
		auto const tampon = "0XFF_c9_45_AB ";
		GenreLexeme id_nombre;

		auto compte = denombreuse::extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = dls::vue_chaine_compacte{ tampon, static_cast<long>(compte) };

		CU_VERIFIE_CONDITION(controleuse, compte == 13);
		CU_VERIFIE_CONDITION(controleuse, id_nombre == GenreLexeme::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleuse, chaine, dls::vue_chaine_compacte("0XFF_c9_45_AB"));

		auto nombre = denombreuse::converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleuse, nombre, 0xFFc945ABl);
	}
	{
		auto fichier = Fichier{};
		fichier.tampon = lng::tampon_source("0xff38ce");

		Lexeuse lexeuse(&fichier);
		lexeuse.performe_lexage();

		auto const &morceaux = fichier.morceaux;

		CU_VERIFIE_CONDITION(controleuse, morceaux.taille() == 1);
		CU_VERIFIE_CONDITION(controleuse, morceaux[0].genre == GenreLexeme::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleuse, morceaux[0].chaine, dls::vue_chaine_compacte("0xff38ce"));
	}
}

void test_surchage_binaire(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto nombre = denombreuse::converti_chaine_nombre_entier(
						  dls::vue_chaine_compacte("0b10000000000000000000000000000000000000000000000000000000000000000"),
						  GenreLexeme::NOMBRE_BINAIRE);

		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = denombreuse::converti_chaine_nombre_entier(
						  dls::vue_chaine_compacte("0o2777777777777777777777"),
						  GenreLexeme::NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::max());

		nombre = denombreuse::converti_chaine_nombre_entier(
					 dls::vue_chaine_compacte("0o17777777777777777777777"),
					 GenreLexeme::NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = denombreuse::converti_chaine_nombre_entier(
						  dls::vue_chaine_compacte("9223372036854775808"),
						  GenreLexeme::NOMBRE_ENTIER);

		/* Dans ce cas-ci, les bits débordent, et nous nous retrouvons avec une
		 * valeur négative. */
		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::min());

		nombre = denombreuse::converti_chaine_nombre_entier(
					 dls::vue_chaine_compacte("9223372036854775807456"),
					 GenreLexeme::NOMBRE_ENTIER);

		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = denombreuse::converti_chaine_nombre_entier(
						  dls::vue_chaine_compacte("0x1ffffffffffffffff"),
						  GenreLexeme::NOMBRE_HEXADECIMAL);

		CU_VERIFIE_EGALITE(controleuse, nombre, std::numeric_limits<long>::max());
	}
}

void test_decoupage_nombres(dls::test_unitaire::Controleuse &controleuse)
{
	test_decoupage_nombre_decimal(controleuse);
	test_decoupage_nombre_binaire(controleuse);
	test_decoupage_nombre_octal(controleuse);
	test_decoupage_nombre_hexadecimal(controleuse);
	test_surchage_binaire(controleuse);
}
