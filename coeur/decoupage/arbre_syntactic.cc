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

Noeud::Noeud(const std::string &chaine)
	: m_chaine(chaine)
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

NoeudRacine::NoeudRacine(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudRacine::imprime_code(std::ostream &os)
{
	os << "NoeudRacine\n";

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os);
	}
}

/* ************************************************************************** */

NoeudAppelFonction::NoeudAppelFonction(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudAppelFonction::imprime_code(std::ostream &os)
{
	os << "NoeudAppelFonction\n";
}

/* ************************************************************************** */

NoeudDeclarationFonction::NoeudDeclarationFonction(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudDeclarationFonction::imprime_code(std::ostream &os)
{
	os << "NoeudDeclarationFonction : " << m_chaine << '\n';

	for (auto noeud : m_enfants) {
		noeud->imprime_code(os);
	}
}

/* ************************************************************************** */

NoeudExpression::NoeudExpression(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudExpression::imprime_code(std::ostream &os)
{
	os << "NoeudExpression\n";
}

/* ************************************************************************** */

NoeudAssignationVariable::NoeudAssignationVariable(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudAssignationVariable::imprime_code(std::ostream &os)
{
	os << "NoeudAssignationVariable\n";
}

/* ************************************************************************** */

NoeudNombreEntier::NoeudNombreEntier(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudNombreEntier::imprime_code(std::ostream &os)
{
	os << "NoeudNombreEntier\n";
}

/* ************************************************************************** */

NoeudNombreReel::NoeudNombreReel(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudNombreReel::imprime_code(std::ostream &os)
{
	os << "NoeudNombreReel\n";
}

/* ************************************************************************** */

NoeudVariable::NoeudVariable(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudVariable::imprime_code(std::ostream &os)
{
	os << "NoeudVariable\n";
}

/* ************************************************************************** */

NoeudOperation::NoeudOperation(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudOperation::imprime_code(std::ostream &os)
{
	os << "NoeudOperation\n";
}

/* ************************************************************************** */

NoeudRetour::NoeudRetour(const std::string &chaine)
	: Noeud(chaine)
{}

void NoeudRetour::imprime_code(std::ostream &os)
{
	os << "NoeudRetour\n";
}
