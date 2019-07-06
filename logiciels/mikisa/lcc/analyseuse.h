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

#undef DEBOGUE_IDENTIFIANT

#ifdef DEBOGUE_IDENTIFIANT
#	include <map>
#endif

#include "biblinternes/structures/tableau.hh"

#include "contexte_generation_code.h"
#include "erreur.h"
#include "morceaux.hh"

/**
 * Classe de base pour définir des analyseurs syntactique.
 */
class Analyseuse {
protected:
	ContexteGenerationCode &m_contexte;
	/* La référence n'est pas constante car l'analyseuse peut changer
	 * l'identifiant des morceaux
	 * (par exemple id_morceau::PLUS -> id_morceau::PLUS_UNAIRE). */
	dls::tableau<DonneesMorceaux> &m_identifiants;
	long m_position = 0;

#ifdef DEBOGUE_IDENTIFIANT
	std::map<id_morceau, int> m_tableau_identifiant;
#endif

public:
	Analyseuse(dls::tableau<DonneesMorceaux> &identifiants, ContexteGenerationCode &contexte);
	virtual ~Analyseuse() = default;

	/**
	 * Lance l'analyse syntactique du vecteur d'identifiants spécifié.
	 *
	 * Si aucun assembleur n'est installé lors de l'appel de cette méthode,
	 * une exception est lancée.
	 */
	virtual void lance_analyse(std::ostream &os) = 0;

#ifdef DEBOGUE_IDENTIFIANT
	void imprime_identifiants_plus_utilises(std::ostream &os, size_t nombre = 5);
#endif

protected:
	/**
	 * Vérifie que l'identifiant courant est égal à celui spécifié puis avance
	 * la position de l'analyseur sur le vecteur d'identifiant.
	 */
	bool requiers_identifiant(id_morceau identifiant);

	/**
	 * Retourne vrai si l'identifiant courant est égal à celui spécifié.
	 */
	bool est_identifiant(id_morceau identifiant);

	/**
	 * Retourne vrai si l'identifiant courant et celui d'après sont égaux à ceux
	 * spécifiés dans le même ordre.
	 */
	bool sont_2_identifiants(id_morceau id1, id_morceau id2);

	/**
	 * Retourne vrai si l'identifiant courant et les deux d'après sont égaux à
	 * ceux spécifiés dans le même ordre.
	 */
	bool sont_3_identifiants(id_morceau id1, id_morceau id2, id_morceau id3);

	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(
			const std::string &quoi,
			erreur::type_erreur type = erreur::type_erreur::NORMAL);

	/**
	 * Avance l'analyseur du nombre de cran spécifié sur le vecteur de morceaux.
	 */
	void avance(long n = 1);

	/**
	 * Recule l'analyseur d'un cran sur le vecteur d'identifiants.
	 */
	void recule();

	/**
	 * Retourne la position courante sur le vecteur d'identifiants.
	 */
	long position();

	/**
	 * Retourne l'identifiant courant.
	 */
	id_morceau identifiant_courant() const;
};
