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

#include "sdk/context.h"
#include "sdk/primitive.h"

class EvaluationContext;
class FenetrePrincipale;
class Scene;
class UsineOperatrice;
class BaseEditrice;

static constexpr auto MASQUE_CATEGORIE = 0x000000ff;
static constexpr auto MASQUE_ACTION    = 0x0000ff00;

enum class type_evenement : int {
	/* Catégorie. */
	objet = (1 << 0),
	noeud = (2 << 0),
	temps = (3 << 0),

	/* Action. */
	ajoute     = (1 << 8),
	enleve     = (2 << 8),
	selectione = (3 << 8),
	modifie    = (4 << 8),
	parente    = (5 << 8),
	traite     = (6 << 8),
};

constexpr type_evenement operator&(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

constexpr type_evenement operator&(type_evenement lhs, int rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) & rhs);
}

constexpr type_evenement operator|(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr type_evenement operator^(type_evenement lhs, type_evenement rhs)
{
	return static_cast<type_evenement>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

constexpr type_evenement operator~(type_evenement lhs)
{
	return static_cast<type_evenement>(~static_cast<int>(lhs));
}

type_evenement &operator|=(type_evenement &lhs, type_evenement rhs);
type_evenement &operator&=(type_evenement &lhs, type_evenement rhs);
type_evenement &operator^=(type_evenement &lhs, type_evenement rhs);

constexpr auto action_evenement(type_evenement evenement)
{
	return evenement & MASQUE_ACTION;
}

constexpr auto categorie_evenement(type_evenement evenement)
{
	return evenement & MASQUE_CATEGORIE;
}

template <typename type_char>
auto &operator<<(
		std::basic_ostream<type_char> &os,
		type_evenement evenement)
{
	switch (categorie_evenement(evenement)) {
		case type_evenement::objet:
			os << "objet, ";
			break;
		case type_evenement::noeud:
			os << "noeud, ";
			break;
		case type_evenement::temps:
			os << "temps, ";
			break;
		default:
			os << "inconnu, ";
			break;
	}

	switch (action_evenement(evenement)) {
		case type_evenement::ajoute:
			os << "ajouté";
			break;
		case type_evenement::modifie:
			os << "modifié";
			break;
		case type_evenement::parente:
			os << "parenté";
			break;
		case type_evenement::enleve:
			os << "enlevé";
			break;
		case type_evenement::selectione:
			os << "sélectioné";
			break;
		case type_evenement::traite:
			os << "traité";
			break;
		default:
			os << "inconnu";
			break;
	}

	return os;
}

class ContextListener {
protected:
	Context *m_context = nullptr;

public:
	virtual ~ContextListener();

	void listens(Context *ctx);

	virtual void update_state(type_evenement event) = 0;
};

class Listened {
	std::vector<ContextListener *> m_listeners{};

public:
	void add_listener(ContextListener *listener);

	void remove_listener(ContextListener *listener);

	void notify_listeners(type_evenement event);
};
