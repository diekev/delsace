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

#include "outils_lexemes.hh"

#include "lexemes.hh"

PositionLexeme position_lexeme(Lexeme const &lexeme)
{
	auto pos = PositionLexeme{};
	pos.pos = lexeme.colonne;
	pos.numero_ligne = lexeme.ligne + 1;
	pos.index_ligne = lexeme.ligne;
	return pos;
}

GenreLexeme operateur_pour_assignation_composee(GenreLexeme type)
{
	switch (type) {
		default:
		{
			return type;
		}
		case GenreLexeme::MOINS_EGAL:
		{
			return GenreLexeme::MOINS;
		}
		case GenreLexeme::PLUS_EGAL:
		{
			return GenreLexeme::PLUS;
		}
		case GenreLexeme::MULTIPLIE_EGAL:
		{
			return GenreLexeme::FOIS;
		}
		case GenreLexeme::DIVISE_EGAL:
		{
			return GenreLexeme::DIVISE;
		}
		case GenreLexeme::MODULO_EGAL:
		{
			return GenreLexeme::POURCENT;
		}
		case GenreLexeme::ET_EGAL:
		{
			return GenreLexeme::ESPERLUETTE;
		}
		case GenreLexeme::OU_EGAL:
		{
			return GenreLexeme::BARRE;
		}
		case GenreLexeme::OUX_EGAL:
		{
			return GenreLexeme::CHAPEAU;
		}
		case GenreLexeme::DEC_DROITE_EGAL:
		{
			return GenreLexeme::DECALAGE_DROITE;
		}
		case GenreLexeme::DEC_GAUCHE_EGAL:
		{
			return GenreLexeme::DECALAGE_GAUCHE;
		}
	}
}

