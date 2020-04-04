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

#pragma once

#include "biblinternes/outils/definitions.h"

struct ContexteGenerationCode;
struct NoeudBloc;
struct NoeudExpression;

/* ************************************************************************** */

enum class type_expression : char {
	ENTIER,
	REEL,
};

struct ResultatExpression {
	union {
		long entier = 0;
		double reel;
		bool condition;
	};

	type_expression type{};
	bool est_errone = false;
	NoeudExpression *noeud_erreur = nullptr;
	const char *message_erreur = nullptr;

	ResultatExpression() = default;

	COPIE_CONSTRUCT(ResultatExpression);
};

ResultatExpression evalue_expression(
		ContexteGenerationCode &contexte,
		NoeudBloc *bloc,
		NoeudExpression *b);
