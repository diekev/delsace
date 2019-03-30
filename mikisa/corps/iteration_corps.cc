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

#include "iteration_corps.hh"

#include "corps.h"
#include "volume.hh"

void pour_chaque_polygone(const Corps &corps, type_fonc_rap_poly fonction_rappel)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto polygone = dynamic_cast<Polygone *>(prim);
		fonction_rappel(corps, polygone);
	}
}

void pour_chaque_polygone_ferme(Corps const &corps, type_fonc_rap_poly fonction_rappel)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto polygone = dynamic_cast<Polygone *>(prim);

		if (polygone->type != type_polygone::FERME) {
			continue;
		}

		fonction_rappel(corps, polygone);
	}
}

void pour_chaque_polygone_ouvert(Corps const &corps, type_fonc_rap_poly fonction_rappel)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto polygone = dynamic_cast<Polygone *>(prim);

		if (polygone->type != type_polygone::OUVERT) {
			continue;
		}

		fonction_rappel(corps, polygone);
	}
}

void pour_chaque_primitive(Corps const &corps, type_fonc_rap_prim fonction_rappel)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);
		fonction_rappel(corps, prim);
	}
}

void pour_chaque_volume(Corps const &corps, type_fonc_rap_volume fonction_rappel)
{
	auto prims = corps.prims();

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() != type_primitive::VOLUME) {
			continue;
		}

		auto volume = dynamic_cast<Volume *>(prim);

		fonction_rappel(corps, volume);
	}
}
