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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "definitions.h"

#include <type_traits>
#include <utility>

namespace dls {
namespace outils {

/**
 * Stocke une fonction qui sera exécutée lors de la destruction de cette classe.
 */
template <typename TypeFonction>
class GardePortee {
	TypeFonction m_fonction;

public:
	/**
	 * Construit une instance à partir d'une fonction.
	 */
	explicit GardePortee(const TypeFonction &fonction);

	/**
	 * Construit une instance à partir d'une fonction.
	 */
	explicit GardePortee(TypeFonction &&fonction);

	/**
	 * Exécute la fonction contenu dans cette classe.
	 */
	~GardePortee();
};

template <typename TypeFonction>
GardePortee<TypeFonction>::GardePortee(const TypeFonction &fonction)
	: m_fonction(fonction)
{}

template <typename TypeFonction>
GardePortee<TypeFonction>::GardePortee(TypeFonction &&fonction)
	: m_fonction(std::move(fonction))
{}

template <typename TypeFonction>
GardePortee<TypeFonction>::~GardePortee()
{
	m_fonction();
}

namespace detail {

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePorteeSurSortie {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortee à partir
 * de la classe vide GardePorteeSurSortie et d'une fonction quelconque.
 *
 * La magie se passe dans le macro SORTIE_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePorteeSurSortie, TypeFonction &&fonction)
{
	return GardePortee<TypeFonction>(std::forward<TypeFonction>(fonction));
}

}  /* namespace detail */

/**
 * Crée une variable locale de type GardePortee qui contient une fonction qui
 * sera exécutée lors de la sortie de la portée où la variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     SORTIE_PORTEE { // fais quelque chose };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePorteeSurSortie.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define SORTIE_PORTEE \
	auto VARIABLE_ANONYME(ETAT_SORTIE_PORTEE) = ::detail::GardePorteeSurSortie() + [&]()

/* ************************************************************************** */

class CompteurExceptionNonAttrapee {
	int m_compteur;

public:
	/**
	 * Construit l'objet en enregistrant le nombre d'exception non attrapée qui
	 * existe au moment de la construction.
	 */
	CompteurExceptionNonAttrapee();

	/**
	 * Retourne si une exception a été lancé depuis la création de l'objet.
	 */
	bool existe_nouvelle_exception() noexcept;
};

/**
 * Stocke une fonction qui sera exécutée lors de la destruction de cette classe
 * si une exception a été lancé et si EXECUTE_SI_EXCEPTION est vrai.
 */
template <typename TypeFonction, bool EXECUTE_SI_EXCEPTION>
class GardePorteePourNouvelleException {
	TypeFonction m_fonction;
	CompteurExceptionNonAttrapee m_compteur;

public:
	/**
	 * Construit une instance à partir d'une fonction.
	 */
	explicit GardePorteePourNouvelleException(const TypeFonction &fonction);

	/**
	 * Construit une instance à partir d'une fonction.
	 */
	explicit GardePorteePourNouvelleException(TypeFonction &&fonction);

	/**
	 * Exécute la fonction contenu dans cette classe si une exception a été
	 * lancé et si EXECUTE_SI_EXCEPTION est vrai.
	 */
	~GardePorteePourNouvelleException() noexcept(EXECUTE_SI_EXCEPTION);
};

template <typename TypeFonction, bool EXECUTE_SI_EXCEPTION>
GardePorteePourNouvelleException<TypeFonction, EXECUTE_SI_EXCEPTION>::
GardePorteePourNouvelleException(const TypeFonction &fonction)
	: m_fonction(fonction)
{}

template <typename TypeFonction, bool EXECUTE_SI_EXCEPTION>
GardePorteePourNouvelleException<TypeFonction, EXECUTE_SI_EXCEPTION>::
GardePorteePourNouvelleException(TypeFonction &&fonction)
	: m_fonction(std::move(fonction))
{}

template <typename TypeFonction, bool EXECUTE_SI_EXCEPTION>
GardePorteePourNouvelleException<TypeFonction, EXECUTE_SI_EXCEPTION>::
~GardePorteePourNouvelleException() noexcept(EXECUTE_SI_EXCEPTION)
{
	if (EXECUTE_SI_EXCEPTION == m_compteur.existe_nouvelle_exception()) {
		m_fonction();
	}
}

namespace detail {

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePorteeSurEchec {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortee à partir
 * de la classe vide GardePorteeSurEchec et d'une fonction quelconque.
 *
 * La magie se passe dans le macro ECHEC_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePorteeSurEchec, TypeFonction &&fonction)
{
	return GardePorteePourNouvelleException<
			typename std::decay<TypeFonction>::type, true>(
				std::forward<TypeFonction>(fonction));
}

/**
 * Crée une variable locale de type GardePorteePourNouvelleException qui
 * contient une fonction qui sera exécutée lors de la sortie de la portée où la
 * variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     ECHEC_PORTEE { // fais quelque chose à la sortie de la portée };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePorteeSurEchec.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define ECHEC_PORTEE \
	auto VARIABLE_ANONYME(ETAT_ECHEC_PORTEE) = ::detail::GardePorteeSurEchec() + [&]() noexcept

/* ************************************************************************** */

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePorteeSurSucces {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortee à partir
 * de la classe vide GardePorteeSurSucces et d'une fonction quelconque.
 *
 * La magie se passe dans le macro REUSSITE_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePorteeSurSucces, TypeFonction &&fonction)
{
	return GardePorteePourNouvelleException<
			typename std::decay<TypeFonction>::type, false>(
				std::forward<TypeFonction>(fonction));
}

}  /* namespace detail */

/**
 * Crée une variable locale de type GardePorteePourNouvelleException qui
 * contient une fonction qui sera exécutée lors de la sortie de la portée où la
 * variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     REUSSITE_PORTEE { // fais quelque chose à la sortie de la portée };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePorteeSurSucces.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define REUSSITE_PORTEE \
	auto VARIABLE_ANONYME(ETAT_REUSSITE_PORTEE) = ::detail::GardePorteeSurSucces() + [&]() noexcept

}  /* namespace outils */
}  /* namespace dls */
