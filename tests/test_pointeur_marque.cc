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

#include "tests.hh"

#include "biblinternes/structures/pointeur_marque.hh"
#include "biblinternes/structures/tableau_simple_compact.hh"

void test_pointeur_marque(dls::test_unitaire::Controleuse &controleuse)
{
	int a = 5;
	int b = 6;
	int c = 3;

	auto ptr_haut = dls::pointeur_marque_haut<int>(&a, b);

	CU_DEBUTE_PROPOSITION(controleuse, "Un pointeur marqué haut peut stocker un entier et un pointeur");
	{
		CU_VERIFIE_EGALITE(controleuse, ptr_haut.pointeur(), &a);
		CU_VERIFIE_EGALITE(controleuse, ptr_haut.marque(), b);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	auto ptr_bas = dls::pointeur_marque_bas<int, 4>(&a, c);

	CU_DEBUTE_PROPOSITION(controleuse, "Un pointeur marqué bas peut stocker un entier et un pointeur");
	{
		CU_VERIFIE_EGALITE(controleuse, ptr_bas.pointeur(), &a);
		CU_VERIFIE_EGALITE(controleuse, ptr_bas.marque(), c);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	auto ptr_mix = dls::pointeur_marque_haut_bas<int, 4>(&a, b, c);

	CU_DEBUTE_PROPOSITION(controleuse, "Un pointeur marqué haut-bas peut stocker deux entiers et un pointeur");
	{
		CU_VERIFIE_EGALITE(controleuse, ptr_mix.pointeur(), &a);
		CU_VERIFIE_EGALITE(controleuse, ptr_mix.marque_haut(), b);
		CU_VERIFIE_EGALITE(controleuse, ptr_mix.marque_bas(), c);
	}
	CU_TERMINE_PROPOSITION(controleuse);
}

void test_tableau_compact(dls::test_unitaire::Controleuse &controleuse)
{
	auto tabl = dls::tableau_simple_compact<int>();

	CU_DEBUTE_PROPOSITION(controleuse, "Un tableau compact est initialisé à zéro");
	{
		CU_VERIFIE_EGALITE(controleuse, tabl.donnees(), static_cast<int *>(nullptr));
		CU_VERIFIE_EGALITE(controleuse, tabl.taille(), 0);
	}
	CU_TERMINE_PROPOSITION(controleuse);

	CU_DEBUTE_PROPOSITION(controleuse, "Un tableau compact tient trace de sa taille et de ses valeurs");
	{
		for (auto i = 0; i < 256; ++i) {
			tabl.ajoute(i * 2);

			CU_VERIFIE_EGALITE(controleuse, tabl[i], i * 2);
			CU_VERIFIE_EGALITE(controleuse, tabl.taille(), i + 1);
		}
	}
	CU_TERMINE_PROPOSITION(controleuse);
}
