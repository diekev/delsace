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

#include "test_unicode.h"

#include "../coeur/decoupage/unicode.h"

void test_unicode(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	/* Quelques séquences valides. */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("a"), 1);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("é"), 2);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("ア"), 3);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("𠮟"), 4);

	/* aucune séquence de codage ne peut commencer par 0xC0 ou 0xC1 */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xC0"), 0);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xC1"), 0);

	/* les points allande de u+D800 à u+DFFF sont interdits */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xD8\x00"), 0);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xDF\xFF"), 0);

	/* le premier octet de contination d'une séquence commençant par ED ne peut
	 * prendre aucune des valeurs valeurs hexadécimales A0 à BF */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xED\xA0"), 0);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xED\xBF"), 0);

	/* le premier octet de continuation d’une séquence qui commence par l’octet
	 * hexadécimal F4 ne peut prendre aucune des valeurs hexadécimales 90 à BF */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xF4\x90"), 0);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xF4\xBF"), 0);

	/* aucune séquence d’octets ne contient des octets initiaux de valeur
	 * hexadécimale F5 à FF */
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xF5"), 0);
	CU_VERIFIE_EGALITE(controleur, nombre_octets("\xFF"), 0);
}
