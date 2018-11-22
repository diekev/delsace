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

#include <iostream>
#include <vector>

enum class type_noeud : unsigned int {
	CHAINE_CARACTERE,
	VARIABLE,
	BLOC,
	POUR,
	SI,
};

/* ************************************************************************** */

class Noeud {
protected:
	std::vector<Noeud *> enfants;

public:
	virtual ~Noeud() = default;

	void ajoute_enfant(Noeud *noeud);

	virtual type_noeud type() const = 0;

	virtual void imprime_arbre(std::ostream &os, int tab) const = 0;
};

/* ************************************************************************** */

class NoeudChaineCaractere final : public Noeud {
public:
	type_noeud type() const override;

	void imprime_arbre(std::ostream &os, int tab) const override;
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	type_noeud type() const override;

	void imprime_arbre(std::ostream &os, int tab) const override;
};

/* ************************************************************************** */

class NoeudBloc final : public Noeud {
public:
	type_noeud type() const override;

	void imprime_arbre(std::ostream &os, int tab) const override;
};

/* ************************************************************************** */

class NoeudSi final : public Noeud {
public:
	type_noeud type() const override;

	void imprime_arbre(std::ostream &os, int tab) const override;
};

/* ************************************************************************** */

class NoeudPour final : public Noeud {
public:
	type_noeud type() const override;

	void imprime_arbre(std::ostream &os, int tab) const override;
};
