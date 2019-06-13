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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tests.hh"

#include "../types/date.h"

void test_date(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::types;

	Date tanjoubi(Jour(30), Mois(Mois::OCTOBER), Annee(1991));
	Date kirimasu(Jour(25), Mois(Mois::DECEMBER), Annee(1991));

	CU_VERIFIE_CONDITION(controleur, tanjoubi < kirimasu);
	CU_VERIFIE_CONDITION(controleur, tanjoubi.jours_jusque(kirimasu) == kirimasu.jours_depuis(tanjoubi));

	CU_VERIFIE_CONDITION(controleur, tanjoubi - Jour(15) == Date(Jour(15), Mois(Mois::OCTOBER), Annee(1991)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi - Jour(31) == Date(Jour(29), Mois(Mois::SEPTEMBER), Annee(1991)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi - Jour(61) == Date(Jour(30), Mois(Mois::AUGUST), Annee(1991)));

	CU_VERIFIE_CONDITION(controleur, tanjoubi + Jour(15) == Date(Jour(14), Mois(Mois::NOVEMBER), Annee(1991)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi + Jour(31) == Date(Jour(30), Mois(Mois::NOVEMBER), Annee(1991)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi + Jour(61) == Date(Jour(30), Mois(Mois::DECEMBER), Annee(1991)));

	CU_VERIFIE_CONDITION(controleur, tanjoubi - Mois(7) == Date(Jour(30), Mois(Mois::MARCH), Annee(1991)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi - Mois(10) == Date(Jour(30), Mois(Mois::DECEMBER), Annee(1990)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi - Mois(24) == Date(Jour(30), Mois(Mois::OCTOBER), Annee(1989)));

	CU_VERIFIE_CONDITION(controleur, tanjoubi + Mois(7) == Date(Jour(30), Mois(Mois::MAY), Annee(1992)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi + Mois(10) == Date(Jour(30), Mois(Mois::AUGUST), Annee(1992)));
	CU_VERIFIE_CONDITION(controleur, tanjoubi + Mois(24) == Date(Jour(30), Mois(Mois::OCTOBER), Annee(1993)));

	/* Verifie les années bissextile. */
	Date annee_bissextile(Jour(29), Mois(Mois::FEBRUARY), Annee(1992));
	CU_VERIFIE_CONDITION(controleur, annee_bissextile.est_annee_bissextile());
	CU_VERIFIE_CONDITION(controleur, !tanjoubi.est_annee_bissextile());
}
