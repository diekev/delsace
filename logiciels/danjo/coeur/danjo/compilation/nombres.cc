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

#include "nombres.h"

#include <cmath>

#include "erreur.h"

/* Logique de découpage et de conversion de nombres.
 *
 */

namespace danjo {

enum class etat_nombre : char {
	POINT,
	EXPONENTIEL,
	DEBUT,
};

static auto extrait_nombre_decimal(const char *debut, const char *fin, id_morceau &id_nombre)
{
	auto compte = 0ul;
	auto etat = etat_nombre::DEBUT;
	id_nombre = id_morceau::NOMBRE;

	while (debut != fin) {
		if (!est_nombre_decimal(*debut) && (*debut != '_') && *debut != '.') {
			break;
		}

		if (*debut == '.') {
			if ((*(debut + 1) == '.') && (*(debut + 2) == '.')) {
				break;
			}

			if (etat == etat_nombre::POINT) {
				//throw erreur::frappe("Erreur ! Le nombre contient un point en trop !\n", erreur::type_erreur::DECOUPAGE);
			}

			etat = etat_nombre::POINT;
			id_nombre = id_morceau::NOMBRE_DECIMAL;
		}

		++debut;
		++compte;
	}

	return compte;
}

size_t extrait_nombre(const char *debut, const char *fin, id_morceau &id_nombre)
{
	return extrait_nombre_decimal(debut, fin, id_nombre);
}

/* ************************************************************************** */

static auto converti_nombre_entier(const dls::vue_chaine &chaine)
{
	long valeur = 0l;
	auto i = 0l;

	for (; i < chaine.taille(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur = valeur * 10 + static_cast<long>(c - '0');

		if (i > 19 /*|| chaine > "9223372036854775807"*/) {
			/* À FAIRE : erreur, surcharge. */
			return std::numeric_limits<long>::max();
		}
	}

	return valeur;
}

long converti_chaine_nombre_entier(const dls::vue_chaine &chaine, id_morceau /*identifiant*/)
{
	return converti_nombre_entier(chaine);
}

static auto converti_nombre_reel(const dls::vue_chaine &chaine)
{
	double valeur = 0.0;
	auto i = 0l;

	/* avant point */
	for (; i < chaine.taille(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (c == '.') {
			++i;
			break;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur = valeur * 10.0 + static_cast<double>(c - '0');
	}

	/* après point */
	auto dividende = 10.0;

	for (; i < chaine.taille(); ++i) {
		auto c = chaine[i];

		if (c == '_') {
			continue;
		}

		if (!est_nombre_decimal(c)) {
			return valeur;
		}

		valeur += static_cast<double>(c - '0') / dividende;
		dividende *= 10.0;
	}

	return valeur;
}

double converti_chaine_nombre_reel(const dls::vue_chaine &chaine, id_morceau /*identifiant*/)
{
	return converti_nombre_reel(chaine);
}

}  /* namespace danjo */
