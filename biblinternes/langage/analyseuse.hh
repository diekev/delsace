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

#include <ostream>
#include <vector>

namespace lng {

/**
 * Classe de base pour définir des analyseuses syntactique.
 */
template <typename TypeIdentifiant>
struct analyseuse {
protected:
	std::vector<TypeIdentifiant> &m_identifiants;
	long m_position = 0;

public:
	using type_id = typename TypeIdentifiant::type;

	explicit analyseuse(std::vector<TypeIdentifiant> &identifiants)
		: m_identifiants(identifiants)
	{}

	virtual ~analyseuse() = default;

	/**
	 * Lance l'analyse syntactique du tableau d'identifiants spécifié.
	 *
	 * Si aucun assembleur n'est installé lors de l'appel de cette méthode,
	 * une exception est lancée.
	 */
	virtual void lance_analyse(std::ostream &os) = 0;

protected:
	/**
	 * Vérifie que l'identifiant courant est égal à celui spécifié puis avance
	 * la position de l'analyseur sur le tableau d'identifiant.
	 */
	bool requiers_identifiant(type_id identifiant)
	{
		if (m_position >= static_cast<long>(m_identifiants.size())) {
			return false;
		}

		const auto est_bon = this->est_identifiant(identifiant);

		avance();

		return est_bon;
	}

	/**
	 * Avance l'analyseur du nombre de cran spécifié sur le tableau de morceaux.
	 */
	void avance(long n = 1)
	{
		m_position += n;
	}

	/**
	 * Recule l'analyseur d'un cran sur le tableau d'identifiants.
	 */
	void recule()
	{
		m_position -= 1;
	}

	/**
	 * Retourne la position courante sur le tableau d'identifiants.
	 */
	long position() const
	{
		return m_position - 1;
	}

	/**
	 * Retourne vrai si l'identifiant courant est égal à celui spécifié.
	 */
	bool est_identifiant(type_id identifiant)
	{
		return identifiant == this->identifiant_courant();
	}

	/**
	 * Retourne vrai si l'identifiant courant et celui d'après sont égaux à ceux
	 * spécifiés dans le même ordre.
	 */
	bool sont_2_identifiants(type_id id1, type_id id2)
	{
		if ((m_position + 2) >= static_cast<long>(m_identifiants.size())) {
			return false;
		}

		return m_identifiants[static_cast<size_t>(m_position)].identifiant == id1
				&& m_identifiants[static_cast<size_t>(m_position + 1)].identifiant == id2;
	}

	/**
	 * Retourne vrai si l'identifiant courant et les deux d'après sont égaux à
	 * ceux spécifiés dans le même ordre.
	 */
	bool sont_3_identifiants(type_id id1, type_id id2, type_id id3)
	{
		if (m_position + 3 >= static_cast<long>(m_identifiants.size())) {
			return false;
		}

		return m_identifiants[static_cast<size_t>(m_position)].identifiant == id1
				&& m_identifiants[static_cast<size_t>(m_position + 1)].identifiant == id2
				&& m_identifiants[static_cast<size_t>(m_position + 2)].identifiant == id3;
	}

	/**
	 * Retourne l'identifiant courant.
	 */
	type_id identifiant_courant() const
	{
		if (m_position >= static_cast<long>(m_identifiants.size())) {
			return TypeIdentifiant::INCONNU;
		}

		return m_identifiants[static_cast<size_t>(m_position)].identifiant;
	}

	bool fini() const
	{
		return m_position == static_cast<long>(m_identifiants.size());
	}

	TypeIdentifiant &donnees()
	{
		return m_identifiants[static_cast<size_t>(position())];
	}

	TypeIdentifiant const &donnees() const
	{
		return m_identifiants[static_cast<size_t>(position())];
	}
};

}  /* namespace lng */
