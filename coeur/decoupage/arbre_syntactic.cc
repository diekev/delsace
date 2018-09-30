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

#include "arbre_syntactic.h"

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

/* ************************************************************************** */

Noeud::Noeud(const std::string &chaine, int id)
	: m_chaine(chaine)
	, identifiant(id)
{}

Noeud::~Noeud()
{
	for (Noeud *noeud : m_enfants) {
		delete noeud;
	}
}

void Noeud::ajoute_noeud(Noeud *noeud)
{
	m_enfants.push_back(noeud);
}

/* ************************************************************************** */

NoeudRacine::NoeudRacine(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudRacine::imprime_code(std::ostream &os, int tab)
{
	os << "NoeudRacine\n";

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudAppelFonction::NoeudAppelFonction(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudAppelFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAppelFonction : " << m_chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudDeclarationFonction::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudDeclarationFonction : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudExpression::NoeudExpression(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudExpression::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudExpression : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudAssignationVariable::NoeudAssignationVariable(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudAssignationVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudAssignationVariable : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudNombreEntier::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreEntier : " << m_chaine << '\n';
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudNombreReel::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudNombreReel : " << m_chaine << '\n';
}

/* ************************************************************************** */

NoeudVariable::NoeudVariable(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudVariable::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudVariable : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudOperation::NoeudOperation(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudOperation::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudOperation : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const std::string &chaine, int id)
	: Noeud(chaine, id)
{}

void NoeudRetour::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << "NoeudRetour : " << m_chaine << '\n';
	for (auto noeud : m_enfants) {
		noeud->imprime_code(os, tab + 1);
	}
}
