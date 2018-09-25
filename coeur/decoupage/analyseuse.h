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

#include <vector>
#include "morceaux.h"

/**
 * Classe de base pour définir des analyseurs syntactique.
 */
class Analyseuse {
protected:
	std::vector<DonneesMorceaux> m_identifiants{};
	int m_position = 0;

public:
	Analyseuse() = default;
	virtual ~Analyseuse() = default;

	/**
	 * Lance l'analyse syntactique du vecteur d'identifiants spécifié.
	 *
	 * Si aucun assembleur n'est installé lors de l'appel de cette méthode,
	 * une exception est lancée.
	 */
	virtual void lance_analyse(const std::vector<DonneesMorceaux> &identifiants) = 0;

protected:
	/**
	 * Vérifie que l'identifiant courant est égal à celui spécifié puis avance
	 * la position de l'analyseur sur le vecteur d'identifiant.
	 */
	bool requiers_identifiant(int identifiant);

	/**
	 * Retourne vrai si l'identifiant courant est égal à celui spécifié.
	 */
	bool est_identifiant(int identifiant);

	/**
	 * Retourne vrai si l'identifiant courant et celui d'après sont égaux à ceux
	 * spécifiés dans le même ordre.
	 */
	bool sont_2_identifiants(int id1, int id2);

	/**
	 * Retourne vrai si l'identifiant courant et les deux d'après sont égaux à
	 * ceux spécifiés dans le même ordre.
	 */
	bool sont_3_identifiants(int id1, int id2, int id3);

	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	void lance_erreur(const std::string &quoi);

	/**
	 * Avance l'analyseur d'un cran sur le vecteur d'identifiants.
	 */
	void avance();

	/**
	 * Recule l'analyseur d'un cran sur le vecteur d'identifiants.
	 */
	void recule();

	/**
	 * Retourne la position courante sur le vecteur d'identifiants.
	 */
	int position();

	/**
	 * Retourne l'identifiant courant.
	 */
	int identifiant_courant() const;
};
