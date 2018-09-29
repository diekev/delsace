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
#include <string>
#include <vector>

enum {
	NOEUD_RACINE,
	NOEUD_DECLARATION_FONCTION,
	NOEUD_APPEL_FONCTION,
	NOEUD_EXPRESSION,
	NOEUD_VARIABLE,
	NOEUD_ASSIGNATION_VARIABLE,
	NOEUD_NOMBRE_REEL,
	NOEUD_NOMBRE_ENTIER,
	NOEUD_OPERATION,
	NOEUD_RETOUR,
};

/* ************************************************************************** */

/**
 * Classe de base représentant un noeud dans l'arbre.
 */
class Noeud {
protected:
	std::string m_chaine;
	std::vector<Noeud *> m_enfants;

public:
	Noeud(const std::string &chaine);

	virtual ~Noeud();

	/**
	 * Ajoute un noeud à la liste des noeuds du noeud.
	 */
	void ajoute_noeud(Noeud *noeud);

	/**
	 * Imprime le 'code' de ce noeud dans le flux de sortie 'os' précisé. C'est
	 * attendu que le noeud demande à ces enfants d'imprimer leurs 'codes' dans
	 * le bon ordre.
	 */
	virtual void imprime_code(std::ostream &os) = 0;
};

/* ************************************************************************** */

class NoeudRacine final : public Noeud {
public:
	NoeudRacine(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudAppelFonction final : public Noeud {
public:
	NoeudAppelFonction(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudDeclarationFonction final : public Noeud {
public:
	NoeudDeclarationFonction(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudExpression final : public Noeud {
public:
	NoeudExpression(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudAssignationVariable final : public Noeud {
public:
	NoeudAssignationVariable(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudNombreEntier final : public Noeud {
public:
	NoeudNombreEntier(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudNombreReel final : public Noeud {
public:
	NoeudNombreReel(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudVariable final : public Noeud {
public:
	NoeudVariable(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudOperation final : public Noeud {
public:
	NoeudOperation(const std::string &chaine);

	void imprime_code(std::ostream &os);
};

/* ************************************************************************** */

class NoeudRetour final : public Noeud {
public:
	NoeudRetour(const std::string &chaine);

	void imprime_code(std::ostream &os);
};
