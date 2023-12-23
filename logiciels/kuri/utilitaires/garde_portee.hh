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
 * GNU General Public License for more détails.
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

#include "macros.hh"

#include <type_traits>
#include <utility>

namespace kuri {

/**
 * Stocke une fonction qui sera exécutée lors de la destruction de cette classe.
 */
template <typename TypeFonction>
class GardePortée {
    TypeFonction m_fonction;

  public:
    /**
     * Construit une instance à partir d'une fonction.
     */
    explicit GardePortée(const TypeFonction &fonction);

    /**
     * Construit une instance à partir d'une fonction.
     */
    explicit GardePortée(TypeFonction &&fonction);

    /**
     * Exécute la fonction contenu dans cette classe.
     */
    ~GardePortée();
};

template <typename TypeFonction>
GardePortée<TypeFonction>::GardePortée(const TypeFonction &fonction) : m_fonction(fonction)
{
}

template <typename TypeFonction>
GardePortée<TypeFonction>::GardePortée(TypeFonction &&fonction) : m_fonction(std::move(fonction))
{
}

template <typename TypeFonction>
GardePortée<TypeFonction>::~GardePortée()
{
    m_fonction();
}

namespace détail {

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePortéeSurSortie {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortée à partir
 * de la classe vide GardePortéeSurSortie et d'une fonction quelconque.
 *
 * La magie se passe dans le macro SORTIE_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePortéeSurSortie, TypeFonction &&fonction)
{
    return GardePortée<TypeFonction>(std::forward<TypeFonction>(fonction));
}

}  // namespace détail

/**
 * Crée une variable locale de type GardePortée qui contient une fonction qui
 * sera exécutée lors de la sortie de la portée où la variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     SUR_SORTIE_PORTEE { // fais quelque chose };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePortéeSurSortie.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define SUR_SORTIE_PORTEE                                                                         \
    auto VARIABLE_ANONYME(ETAT_SORTIE_PORTEE) = kuri::détail::GardePortéeSurSortie() + [&]()

/* ************************************************************************** */

class CompteurExceptionNonAttrapée {
    int m_compteur = 0;

  public:
    /**
     * Construit l'objet en enregistrant le nombre d'exception non attrapée qui
     * existe au moment de la construction.
     */
    CompteurExceptionNonAttrapée();

    /**
     * Retourne vrai si une exception a été lancé depuis la création de l'objet.
     */
    bool une_nouvelle_exception_existe() noexcept;
};

/**
 * Stocke une fonction qui sera exécutée lors de la destruction de cette classe
 * si une exception a été lancé et si EXÉCUTE_SI_EXCEPTION est vrai.
 */
template <typename TypeFonction, bool EXÉCUTE_SI_EXCEPTION>
class GardePortéePourNouvelleException {
    TypeFonction m_fonction{};
    CompteurExceptionNonAttrapée m_compteur{};

  public:
    /**
     * Construit une instance à partir d'une fonction.
     */
    explicit GardePortéePourNouvelleException(const TypeFonction &fonction);

    /**
     * Construit une instance à partir d'une fonction.
     */
    explicit GardePortéePourNouvelleException(TypeFonction &&fonction);

    /**
     * Exécute la fonction contenu dans cette classe si une exception a été
     * lancé et si EXÉCUTE_SI_EXCEPTION est vrai.
     */
    ~GardePortéePourNouvelleException() noexcept(EXÉCUTE_SI_EXCEPTION);
};

template <typename TypeFonction, bool EXÉCUTE_SI_EXCEPTION>
GardePortéePourNouvelleException<TypeFonction, EXÉCUTE_SI_EXCEPTION>::
    GardePortéePourNouvelleException(const TypeFonction &fonction)
    : m_fonction(fonction)
{
}

template <typename TypeFonction, bool EXÉCUTE_SI_EXCEPTION>
GardePortéePourNouvelleException<TypeFonction, EXÉCUTE_SI_EXCEPTION>::
    GardePortéePourNouvelleException(TypeFonction &&fonction)
    : m_fonction(std::move(fonction))
{
}

template <typename TypeFonction, bool EXÉCUTE_SI_EXCEPTION>
GardePortéePourNouvelleException<TypeFonction, EXÉCUTE_SI_EXCEPTION>::
    ~GardePortéePourNouvelleException() noexcept(EXÉCUTE_SI_EXCEPTION)
{
    if (EXÉCUTE_SI_EXCEPTION == m_compteur.une_nouvelle_exception_existe()) {
        m_fonction();
    }
}

namespace détail {

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePortéeSurEchec {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortée à partir
 * de la classe vide GardePortéeSurEchec et d'une fonction quelconque.
 *
 * La magie se passe dans le macro ECHEC_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePortéeSurEchec, TypeFonction &&fonction)
{
    return GardePortéePourNouvelleException<typename std::decay<TypeFonction>::type, true>(
        std::forward<TypeFonction>(fonction));
}

/**
 * Crée une variable locale de type GardePortéePourNouvelleException qui
 * contient une fonction qui sera exécutée lors de la sortie de la portée où la
 * variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     SUR_ECHEC_PORTEE { // fais quelque chose à la sortie de la portée };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePortéeSurEchec.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define SUR_ECHEC_PORTEE                                                                          \
    auto VARIABLE_ANONYME(ETAT_ECHEC_PORTEE) = kuri::détail::GardePortéeSurEchec() + [&]() noexcept

/* ************************************************************************** */

/**
 * Classe vide utilisée pour créer un type afin de surcharger l'opérateur + ci-
 * dessous.
 */
enum class GardePortéeSurSuccès {};

/**
 * Surcharge de l'opérateur + pour créer un objet de type GardePortée à partir
 * de la classe vide GardePortéeSurSuccès et d'une fonction quelconque.
 *
 * La magie se passe dans le macro SUR_REUSSITE_PORTEE ci-dessous.
 */
template <typename TypeFonction>
auto operator+(GardePortéeSurSuccès, TypeFonction &&fonction)
{
    return GardePortéePourNouvelleException<typename std::decay<TypeFonction>::type, false>(
        std::forward<TypeFonction>(fonction));
}

}  // namespace détail

/**
 * Crée une variable locale de type GardePortéePourNouvelleException qui
 * contient une fonction qui sera exécutée lors de la sortie de la portée où la
 * variable est créée.
 *
 * L'idiome à utiliser est le suivant :
 *     SUR_REUSSITE_PORTEE { // fais quelque chose à la sortie de la portée };
 *
 * Les accolades servent à délimiter la partie interne d'un lambda dont le début
 * est contenu dans ce macro avant l'opérateur +. L'objet est créé par addition
 * du lambda et d'une instance de la classe GardePortéeSurSuccès.
 *
 * Le lambda capture les variables locales par référence donc il est possible
 * d'utiliser des variables ayant été déclaré avant sa création.
 *
 * Pour éviter les conflits, chaque instance de la classe a un nom unique,
 * obtenu à travers le macro VARIABLE_ANONYME. De fait, plusieurs instances
 * peuvent être créées dans la même portée.
 */
#define SUR_REUSSITE_PORTEE                                                                       \
    auto VARIABLE_ANONYME(                                                                        \
        ETAT_REUSSITE_PORTEE) = kuri::détail::GardePortéeSurSuccès() + [&]() noexcept

} /* namespace kuri */
