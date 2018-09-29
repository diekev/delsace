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

#include "decoupeuse.h"
#include "erreur.h"
#include "morceaux.h"
#include "nombres.h"

void test_decoupage_nombre_decimal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0.5 ";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 3);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("0.5"));

		auto nombre = converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleur, nombre, 0.5);
	}

	{
		const auto tampon = "0.559_57";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 8);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_REEL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("0.55957"));

		auto nombre = converti_chaine_nombre_reel(chaine, id_nombre);

		CU_VERIFIE_EGALITE_DECIMALE(controleur, nombre, 0.55957);
	}

	{
		const auto tampon = "100000+";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 6);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("100000"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 100000l);
	}

	{
		const auto tampon = "1_234_567_890 ";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_ENTIER);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("1234567890"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 1234567890l);
	}
}

void test_decoupage_nombre_binaire(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0b1001100+";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 9);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("1001100"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 76l);
	}

	{
		const auto tampon = "0B1001_1011 ";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 11);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_BINAIRE);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("10011011"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 155l);
	}
}

void test_decoupage_nombre_octal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0o1234567+";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 9);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("1234567"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 342391l);
	}

	{
		const auto tampon = "0O01_23_45_67 ";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_OCTAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("01234567"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 342391l);
	}
}

void test_decoupage_nombre_hexadecimal(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		const auto tampon = "0xff38ce+";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 8);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("ff38ce"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 0xff38cel);
	}

	{
		const auto tampon = "0XFF_c9_45_AB ";
		std::string chaine;
		int id_nombre;

		auto compte = extrait_nombre(tampon, tampon + std::strlen(tampon), chaine, id_nombre);

		CU_VERIFIE_CONDITION(controleur, compte == 13);
		CU_VERIFIE_CONDITION(controleur, id_nombre == ID_NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, chaine, std::string("FFc945AB"));

		auto nombre = converti_chaine_nombre_entier(chaine, id_nombre);

		CU_VERIFIE_EGALITE(controleur, nombre, 0xFFc945ABl);
	}
	{
		auto tampon = TamponSource("0xff38ce");

		decoupeuse_texte decoupeuse(tampon);
		decoupeuse.genere_morceaux();

		const auto &morceaux = decoupeuse.morceaux();

		CU_VERIFIE_CONDITION(controleur, morceaux.size() == 1);
		CU_VERIFIE_CONDITION(controleur, morceaux[0].identifiant == ID_NOMBRE_HEXADECIMAL);
		CU_VERIFIE_EGALITE(controleur, morceaux[0].chaine, std::string("ff38ce"));
	}
}

void test_surchage_binaire(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	{
		auto nombre = converti_chaine_nombre_entier(
						  "10000000000000000000000000000000000000000000000000000000000000000",
						  ID_NOMBRE_BINAIRE);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "2777777777777777777777",
						  ID_NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());

		nombre = converti_chaine_nombre_entier(
					 "17777777777777777777777",
					 ID_NOMBRE_OCTAL);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "9223372036854775808",
						  ID_NOMBRE_ENTIER);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());

		nombre = converti_chaine_nombre_entier(
					 "9223372036854775807456",
					 ID_NOMBRE_ENTIER);

		CU_VERIFIE_EGALITE(controleur, nombre, std::numeric_limits<long>::max());
	}
	{
		auto nombre = converti_chaine_nombre_entier(
						  "1ffffffffffffffff",
						  ID_NOMBRE_HEXADECIMAL);

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
