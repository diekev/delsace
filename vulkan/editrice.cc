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

#include "editrice.hh"

#include "evenement.hh"

bool Editrice::accepte_evenement(Evenement const &evenement)
{
	if (evenement.type != evenement_fenetre::REDIMENSION) {
		if (evenement.pos.x < pos.x || evenement.pos.x > (pos.x + taille.x)) {
			return false;
		}

		if (evenement.pos.y < pos.y || evenement.pos.y > (pos.y + taille.y)) {
			return false;
		}
	}

	std::cerr << "Évènement accepté !\n";

	switch (evenement.type) {
		case evenement_fenetre::NUL:
		{
			break;
		}
		case evenement_fenetre::SOURIS_BOUGEE:
		{
			this->souris_bougee(evenement);
			break;
		}
		case evenement_fenetre::SOURIS_PRESSEE:
		{
			this->souris_pressee(evenement);
			break;
		}
		case evenement_fenetre::SOURIS_RELACHEE:
		{
			this->souris_relachee(evenement);
			break;
		}
		case evenement_fenetre::SOURIS_ROULETTE:
		{
			this->roulette(evenement);
			break;
		}
		case evenement_fenetre::CLE_PRESSEE:
		{
			this->cle_pressee(evenement);
			break;
		}
		case evenement_fenetre::CLE_RELACHEE:
		{
			this->cle_relachee(evenement);
			break;
		}
		case evenement_fenetre::CLE_REPETEE:
		{
			this->cle_repetee(evenement);
			break;
		}
		case evenement_fenetre::DOUBLE_CLIC:
		{
			this->double_clic(evenement);
			break;
		}
		case evenement_fenetre::REDIMENSION:
		{
			this->taille_norm = this->taille / evenement.pos;
			break;
		}
	}

	return true;
}
