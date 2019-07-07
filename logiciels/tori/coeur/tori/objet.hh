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

#include <memory>
#include <vector>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"

namespace tori {

enum type_objet : unsigned {
	NUL,
	DICTIONNAIRE,
	TABLEAU,
	CHAINE,
	NOMBRE_ENTIER,
	NOMBRE_REEL,
};

struct Objet {
	type_objet type = type_objet::NUL;
	int pad{};
};

template <typename T>
std::shared_ptr<Objet> construit_objet(T const &valeur);

std::shared_ptr<Objet> construit_objet(long v);

std::shared_ptr<Objet> construit_objet(double v);

std::shared_ptr<Objet> construit_objet(dls::chaine const &v);
std::shared_ptr<Objet> construit_objet(char const *v);

struct ObjetDictionnaire final : public Objet {
	dls::dico<dls::chaine, std::shared_ptr<Objet>> valeur{};

	ObjetDictionnaire()
		: Objet{ type_objet::DICTIONNAIRE }
	{}

	template <typename T>
	void insere(dls::chaine const &cle, T const &v)
	{
		valeur[cle] = construit_objet(v);
	}

	void insere(dls::chaine const &cle, std::shared_ptr<Objet> const &v)
	{
		valeur[cle] = v;
	}
};

struct ObjetTableau final : public Objet {
	std::vector<std::shared_ptr<Objet>> valeur{};

	ObjetTableau()
		: Objet{ type_objet::TABLEAU }
	{}

	template <typename... Args>
	static std::shared_ptr<Objet> construit(Args &&... args)
	{
		auto tableau = std::make_shared<ObjetTableau>();
		(tableau->pousse(args), ...);
		return tableau;
	}

	template <typename T>
	void pousse(T const &v)
	{
		valeur.push_back(construit_objet(v));
	}

	void pousse(std::shared_ptr<Objet> const &v)
	{
		valeur.push_back(v);
	}
};

struct ObjetChaine final : public Objet {
	dls::chaine valeur{};

	ObjetChaine()
		: Objet{ type_objet::CHAINE }
	{}
};

struct ObjetNombreEntier final : public Objet {
	long valeur{};

	ObjetNombreEntier()
		: Objet{ type_objet::NOMBRE_ENTIER }
	{}
};

struct ObjetNombreReel final : public Objet {
	double valeur{};

	ObjetNombreReel()
		: Objet{ type_objet::NOMBRE_REEL }
	{}
};

std::shared_ptr<Objet> construit_objet(type_objet type);

}  /* namespace tori */
