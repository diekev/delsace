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

#include "evenement.hh"

std::ostream &operator<<(std::ostream &os, Evenement const &evenement)
{
	switch (evenement.type) {
		case evenement_fenetre::NUL:
			os << "nul";
			break;
		case evenement_fenetre::SOURIS_BOUGEE:
			os << "souris bougée";
			break;
		case evenement_fenetre::SOURIS_PRESSEE:
			os << "souris pressée";
			break;
		case evenement_fenetre::SOURIS_RELACHEE:
			os << "souris relâchée";
			break;
		case evenement_fenetre::SOURIS_ROULETTE:
			os << "roulette";
			break;
		case evenement_fenetre::CLE_PRESSEE:
			os << "clé pressée";
			break;
		case evenement_fenetre::CLE_RELACHEE:
			os << "clé relâchée";
			break;
		case evenement_fenetre::CLE_REPETEE:
			os << "clé répétée";
			break;
		case evenement_fenetre::DOUBLE_CLIC:
			os << "double clic";
			break;
		case evenement_fenetre::REDIMENSION:
			os << "redimension fenêtre";
			break;
	}

	os << " (" << evenement.pos.x << ", " << evenement.pos.y << ')';

	return os;
}
