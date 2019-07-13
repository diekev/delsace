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
#	include "biblinternes/structures/dico.hh"
#endif

#include <ostream>

#include "biblinternes/structures/tableau.hh"

namespace lng {

/**
 * Classe de base pour définir des analyseuses syntactique.
 */
template <typename TypeIdentifiant>
struct analyseuse {
protected:
	/* La référence n'est pas constante car l'analyseuse peut changer
	 * l'identifiant des morceaux
	 * (par exemple id_morceau::PLUS -> id_morceau::PLUS_UNAIRE). */
	dls::tableau<TypeIdentifiant> &m_identifiants;
	long m_position = 0;

#ifdef DEBOGUE_IDENTIFIANT
	dls::dico<typename TypeIdentifiant::type, int> m_tableau_identifiant;
#endif

public:
	using type_id = typename TypeIdentifiant::type;

	explicit analyseuse(dls::tableau<TypeIdentifiant> &identifiants)
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

#ifdef DEBOGUE_IDENTIFIANT
	void imprime_identifiants_plus_utilises(std::ostream &os, size_t nombre = 5);
#endif

protected:
	/**
	 * Vérifie que l'identifiant courant est égal à celui spécifié puis avance
	 * la position de l'analyseur sur le tableau d'identifiant.
	 */
	bool requiers_identifiant(type_id identifiant)
	{
		if (m_position >= m_identifiants.taille()) {
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
#ifdef DEBOGUE_IDENTIFIANT
		m_tableau_identifiant[identifiant]++;
#endif
		return identifiant == this->identifiant_courant();
	}

	/**
	 * Retourne vrai si l'identifiant courant et celui d'après sont égaux à ceux
	 * spécifiés dans le même ordre.
	 */
	bool sont_2_identifiants(type_id id1, type_id id2)
	{
		if ((m_position + 2) >= m_identifiants.taille()) {
			return false;
		}

#ifdef DEBOGUE_IDENTIFIANT
		m_tableau_identifiant[id1]++;
		m_tableau_identifiant[id2]++;
#endif

		return m_identifiants[m_position].identifiant == id1
				&& m_identifiants[m_position + 1].identifiant == id2;
	}

	/**
	 * Retourne vrai si l'identifiant courant et les deux d'après sont égaux à
	 * ceux spécifiés dans le même ordre.
	 */
	bool sont_3_identifiants(type_id id1, type_id id2, type_id id3)
	{
		if (m_position + 3 >= m_identifiants.taille()) {
			return false;
		}

#ifdef DEBOGUE_IDENTIFIANT
		m_tableau_identifiant[id1]++;
		m_tableau_identifiant[id2]++;
		m_tableau_identifiant[id3]++;
#endif

		return m_identifiants[m_position].identifiant == id1
				&& m_identifiants[m_position + 1].identifiant == id2
				&& m_identifiants[m_position + 2].identifiant == id3;
	}

	/**
	 * Retourne l'identifiant courant.
	 */
	type_id identifiant_courant() const
	{
		if (m_position >= m_identifiants.taille()) {
			return TypeIdentifiant::INCONNU;
		}

		return m_identifiants[m_position].identifiant;
	}

	bool fini() const
	{
		return m_position == m_identifiants.taille();
	}

	TypeIdentifiant &donnees()
	{
		return m_identifiants[position()];
	}

	TypeIdentifiant const &donnees() const
	{
		return m_identifiants[position()];
	}
};


#ifdef DEBOGUE_IDENTIFIANT
template <typename TypeIdentifiant>
void analyseuse<TypeIdentifiant>::imprime_identifiants_plus_utilises(std::ostream &os, size_t nombre)
{
	dls::tableau<std::pair<int, int>> tableau(m_tableau_identifiant.taille());
	std::copy(m_tableau_identifiant.debut(), m_tableau_identifiant.fin(), tableau.debut());

	std::sort(tableau.debut(), tableau.fin(),
			  [](const std::pair<int, int> &a, const std::pair<int, int> &b)
	{
		return a.second > b.second;
	});

	os << "--------------------------------\n";
	os << "Identifiant les plus comparées :\n";
	for (size_t i = 0; i < nombre; ++i) {
		//os << chaine_identifiant(tableau[i].first) << " : " << tableau[i].second << '\n';
	}
	os << "--------------------------------\n";
}
#endif

}  /* namespace lng */
