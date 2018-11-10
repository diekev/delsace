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

#include "contexte_generation_code.h"  // pour DonneesModule
#include "decoupeuse.h"
#include "erreur.h"
#include "modules.hh"
#include "morceaux.h"
#include "nombres.h"

void test_decoupage_nombre_decimal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0.5 ";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 3);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0.5"));

		auto nombre = converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleur, nombre, 0.5);
	}

	{
		const auto tampon = "0.559_57";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 8);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0.559_57"));

		auto nombre = converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleur, nombre, 0.55957);
	}

	{
		const auto tampon = "100000+";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 6);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("100000"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 100000l);
	}

	{
		const auto tampon = "1_234_567_890 ";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("1_234_567_890"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 1234567890l);
	}
}

void test_decoupage_nombre_binaire(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0b1001100+";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 9);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0b1001100"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 76l);
	}

	{
		const auto tampon = "0B1001_1011 ";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 11);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0B1001_1011"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 155l);
	}
}

void test_decoupage_nombre_octal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0o1234567+";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 9);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0o1234567"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 342391l);
	}

	{
		const auto tampon = "0O01_23_45_67 ";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0O01_23_45_67"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 342391l);
	}
}

void test_decoupage_nombre_hexadecimal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0xff38ce+";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 8);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0xff38ce"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 0xff38cel);
	}

	{
		const auto tampon = "0XFF_c9_45_AB ";
		id_morceau id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), id_nombre);
		auto chaine = std::string_view{ tampon, compte };

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == id_morceau::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string_view("0XFF_c9_45_AB"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 0xFFc945ABl);
	}
	{
		auto module = DonneesModule{};
		module.tampon = TamponSource("0xff38ce");

		decoupeuse_texte decoupeuse(&module);
		decoupeuse.genere_morceaux();

		const auto &morceaux = module.morceaux;

		CU_VERIFIE_CONDITION(controleur, morceaux.size() == 1);
		CU_VERIFIE_CONDITION(controleur, morceaux[0].identifiant == id_morceau::NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, morceaux[0].chaine, std::string_view("0xff38ce"));
	}
}

void test_surchage_binaire(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		auto nombre = converti_chaine_nombre_entier(
						  "0b10000000000000000000000000000000000000000000000000000000000000000",
						  id_morceau::NOMBRE_BINAIRE);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "0o2777777777777777777777",
						  id_morceau::NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());

		nombre = converti_chaine_nombre_entier(
					 "0o17777777777777777777777",
					 id_morceau::NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "9223372036854775808",
						  id_morceau::NOMBRE_ENTIER);

		/* Dans ce cas-ci, les bits débordent, et nous nous retrouvons avec une
		 * valeur négative. */
		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::min());

		nombre = converti_chaine_nombre_entier(
					 "9223372036854775807456",
					 id_morceau::NOMBRE_ENTIER);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "0x1ffffffffffffffff",
						  id_morceau::NOMBRE_HEXADECIMAL);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
}

void test_decoupage_nombres(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	test_decoupage_nombre_decimal(controleur);
	test_decoupage_nombre_binaire(controleur);
	test_decoupage_nombre_octal(controleur);
	test_decoupage_nombre_hexadecimal(controleur);
	test_surchage_binaire(controleur);
}
