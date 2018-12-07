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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <tbb/parallel_for.h>

/**
 * Enveloppes autour des outils de Intel TBB.
 * Inspiré par "Multithreading for Visual Effects", chapter 2.
 */

template <typename TypePlage>
inline auto taille_plage(const TypePlage &plage)
{
	return plage.end() - plage.begin();
}

template <typename TypePlage, typename TypeOp>
inline void boucle_serie(TypePlage &&plage, TypeOp &&operation)
{
	operation(plage);
}

template <typename TypePlage, typename TypeOp>
void boucle_parallele(
		TypePlage &&plage,
		TypeOp &&operation,
		typename TypePlage::const_iterator taille_grain = 1)
{
	const auto taille = taille_plage(plage);

	if (taille == 0) {
		return;
	}

	/* Vérifie si une seule tache peut être créée. */
	if (taille <= taille_grain) {
		boucle_serie(plage, operation);
		return;
	}

	tbb::parallel_for(TypePlage(plage.begin(), plage.end(), static_cast<typename TypePlage::size_type>(taille_grain)), operation);
}

template <typename TypePlage, typename TypeOp>
void boucle_parallele_legere(TypePlage &&plage, TypeOp &&operation)
{
	boucle_parallele(plage, operation, 1024);
}

template <typename TypePlage, typename TypeOp>
void boucle_parallele_lourde(TypePlage &&plage, TypeOp &&operation)
{
	boucle_parallele(plage, operation, 1);
}
