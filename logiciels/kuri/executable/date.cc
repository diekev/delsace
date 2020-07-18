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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "date.hh"

#include <time.h>

TempsSysteme maintenant_systeme()
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME_COARSE, &t);

	TempsSysteme temps;
	temps.seconde = t.tv_sec;
	temps.nanoseconde = t.tv_nsec;

	return temps;
}

bool est_annee_bissextile(long annee)
{
	if (annee % 100 == 0) {
		if (annee % 400 == 0) {
			return true;
		}

		return false;
	}

	return (annee % 4 == 0);
}

Date hui_systeme()
{
	auto inst = maintenant_systeme();

	auto seconde = inst.seconde % 60;
	auto minutes = (inst.seconde / 60) % 60;
	auto heures  = (inst.seconde / 3600) % 24;

	auto annees = (inst.seconde / secondes_par_an());

	int jours_par_mois[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if (est_annee_bissextile(annees)) {
		jours_par_mois[1] = 29;
	}

	auto secondes_mois = (inst.seconde - (annees * secondes_par_an()));
	auto secondes_ecoulees_mois = 0;
	auto mois = 0;

	for (auto j : jours_par_mois) {
		if (secondes_mois < (28 * 24 * 60 * 60)) {
			break;
		}

		secondes_ecoulees_mois += j * 24 * 60 * 60;
		secondes_mois -= j * 24 * 60 * 60;
		mois += 1;
	}

	auto jours = (inst.seconde - (annees * secondes_par_an()) - (secondes_ecoulees_mois)) / (24 * 60 * 60);

	// On soustrait 3 car Epoch UTC commence un jeudi (4ème jour, index 3)
	auto jour_semaine = ((inst.seconde / (3600 * 24)) - 3) % 7;

	Date date;
	date.jour = static_cast<int>(jours);
	date.mois = mois + 1;
	date.annee = static_cast<int>(annees + 1970);
	date.heure = static_cast<int>(heures) + 2; // UTC
	date.minute = static_cast<int>(minutes);
	date.seconde = static_cast<int>(seconde);
	date.jour_semaine = static_cast<int>(jour_semaine);

	return date;
}
