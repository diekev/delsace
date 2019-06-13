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

#include "../types/temperature.h"

template <typename real>
void test_temperature_impl(dls::test_unitaire::Controleuse &controleur)
{
	using namespace dls::types;

	temperature<real> temp(real(0), TemperatureScale::CELSIUS);
	temperature<real> tempk(real(0), TemperatureScale::KELVIN);

	/* check that 0°C > 0°K (-273.15°C) */
	CU_VERIFIE_CONDITION(controleur, temp > tempk);

	temp = temperature<real>(real(-273.15), TemperatureScale::CELSIUS);

	CU_VERIFIE_CONDITION(controleur, temp.to_kelvin() == tempk);

	temperature<real> temp3(real(5), TemperatureScale::FAHRENHEIT);
	temperature<real> temp4(real(6), TemperatureScale::FAHRENHEIT);

	CU_VERIFIE_CONDITION(controleur, temp3 < temp4);

	temp4 = temp3;
	temp4 += real(78);

	CU_VERIFIE_CONDITION(controleur, temp3 != temp4);
}

void test_temperature(dls::test_unitaire::Controleuse &controleur)
{
	test_temperature_impl<float>(controleur);
	test_temperature_impl<double>(controleur);
}
