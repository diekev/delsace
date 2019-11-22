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

#include "objet.hh"

namespace tori {

std::shared_ptr<Objet> construit_objet(type_objet type)
{
	auto objet = static_cast<Objet *>(nullptr);

	switch (type) {
		case type_objet::NUL:
			objet = new Objet{};
			break;
		case type_objet::DICTIONNAIRE:
			objet = new ObjetDictionnaire{};
			break;
		case type_objet::TABLEAU:
			objet = new ObjetTableau{};
			break;
		case type_objet::CHAINE:
			objet = new ObjetChaine{};
			break;
		case type_objet::NOMBRE_ENTIER:
			objet = new ObjetNombreEntier{};
			break;
		case type_objet::NOMBRE_REEL:
			objet = new ObjetNombreReel{};
			break;
	}

	objet->type = type;

	return std::shared_ptr<Objet>(objet);
}

std::shared_ptr<Objet> construit_objet(long v)
{
	auto objet = std::make_shared<ObjetNombreEntier>();
	objet->valeur = v;
	objet->type = type_objet::NOMBRE_ENTIER;
	return objet;
}

std::shared_ptr<Objet> construit_objet(double v)
{
	auto objet = std::make_shared<ObjetNombreReel>();
	objet->valeur = v;
	objet->type = type_objet::NOMBRE_REEL;
	return objet;
}

std::shared_ptr<Objet> construit_objet(dls::chaine const &v)
{
	auto objet = std::make_shared<ObjetChaine>();
	objet->valeur = v;
	objet->type = type_objet::CHAINE;
	return objet;
}

std::shared_ptr<Objet> construit_objet(char const *v)
{
	auto objet = std::make_shared<ObjetChaine>();
	objet->valeur = v;
	objet->type = type_objet::CHAINE;
	return objet;
}

}  /* namespace tori */
