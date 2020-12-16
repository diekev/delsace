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
#include "biblinternes/structures/tableau.hh"

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

const char *chaine_type(type_objet type);

/* ************************************************************************** */

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

	bool possede(dls::chaine const &cle) const
	{
		return valeur.trouve(cle) != valeur.fin();
	}

	bool possede(dls::tableau<dls::chaine> const &cles) const
	{
		for (auto const &cle : cles) {
			if (!possede(cle)) {
				return false;
			}
		}

		return true;
	}

	Objet *objet(dls::chaine const &cle) const
	{
		auto iter = valeur.trouve(cle);

		if (iter == valeur.fin()) {
			return nullptr;
		}

		return iter->second.get();
	}
};

struct ObjetTableau final : public Objet {
	dls::tableau<std::shared_ptr<Objet>> valeur{};

	ObjetTableau()
		: Objet{ type_objet::TABLEAU }
	{}

	template <typename... Args>
	static std::shared_ptr<Objet> construit(Args &&... args)
	{
		auto tableau = std::make_shared<ObjetTableau>();
		(tableau->ajoute(args), ...);
		return tableau;
	}

	template <typename T>
	void ajoute(T const &v)
	{
		valeur.ajoute(construit_objet(v));
	}

	void ajoute(std::shared_ptr<Objet> const &v)
	{
		valeur.ajoute(v);
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

/* ************************************************************************** */

inline auto extrait_dictionnaire(Objet *objet)
{
	assert(objet->type == type_objet::DICTIONNAIRE);
	return static_cast<ObjetDictionnaire *>(objet);
}

inline auto extrait_dictionnaire(Objet const *objet)
{
	assert(objet->type == type_objet::DICTIONNAIRE);
	return static_cast<ObjetDictionnaire const *>(objet);
}

inline auto extrait_tableau(Objet *objet)
{
	assert(objet->type == type_objet::TABLEAU);
	return static_cast<ObjetTableau *>(objet);
}

inline auto extrait_tableau(Objet const *objet)
{
	assert(objet->type == type_objet::TABLEAU);
	return static_cast<ObjetTableau const *>(objet);
}

inline auto extrait_chaine(Objet *objet)
{
	assert(objet->type == type_objet::CHAINE);
	return static_cast<ObjetChaine *>(objet);
}

inline auto extrait_chaine(Objet const *objet)
{
	assert(objet->type == type_objet::CHAINE);
	return static_cast<ObjetChaine const *>(objet);
}

inline auto extrait_nombre_entier(Objet *objet)
{
	assert(objet->type == type_objet::NOMBRE_ENTIER);
	return static_cast<ObjetNombreEntier *>(objet);
}

inline auto extrait_nombre_entier(Objet const *objet)
{
	assert(objet->type == type_objet::NOMBRE_ENTIER);
	return static_cast<ObjetNombreEntier const *>(objet);
}

inline auto extrait_nombre_reel(Objet *objet)
{
	assert(objet->type == type_objet::NOMBRE_REEL);
	return static_cast<ObjetNombreReel *>(objet);
}

inline auto extrait_nombre_reel(Objet const *objet)
{
	assert(objet->type == type_objet::NOMBRE_REEL);
	return static_cast<ObjetNombreReel const *>(objet);
}

/* ************************************************************************** */

ObjetChaine *cherche_chaine(
		ObjetDictionnaire *dico,
		dls::chaine const &nom);

ObjetDictionnaire *cherche_dico(
		ObjetDictionnaire *dico,
		dls::chaine const &nom);

ObjetNombreEntier *cherche_nombre_entier(
		ObjetDictionnaire *dico,
		dls::chaine const &nom);

ObjetNombreReel *cherche_nombre_reel(
		ObjetDictionnaire *dico,
		dls::chaine const &nom);

ObjetTableau *cherche_tableau(
		ObjetDictionnaire *dico,
		dls::chaine const &nom);

}  /* namespace tori */
