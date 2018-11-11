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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#include <llvm/ADT/SmallVector.h>
#pragma GCC diagnostic pop

#include "morceaux.h"

/**
 * Classe pour gérer les données du type d'une variable ou d'une constante. En
 * l'espèce, la classe contient un vecteur qui peut contenir un nombre variable
 * de pointeurs, de références, et de tableaux, mais le dernier éléments du
 * vecteur devra forcément être vers un type connu (entier, réel, booléen,
 * structure, etc...).
 */
class DonneesType {
	/* Petite optimisation pour les types simples. */
	llvm::SmallVector<id_morceau, 1> m_donnees{};

public:
	using iterateur_const = llvm::SmallVectorImpl<id_morceau>::const_reverse_iterator;

	DonneesType() = default;

	/**
	 * Ajoute un identifiant à ces données. Il ne sera pas possible de supprimer
	 * l'identifiant poussé, donc il vaut mieux faire en sorte de pousser des
	 * données correctes dans un ordre correcte.
	 */
	void pousse(id_morceau identifiant);

	/**
	 * Pousse les identifiants d'un autre vecteur de données dans celui-ci.
	 * Cette fonction est principalement là pour générer les données relatives
	 * à la prise de l'addresse d'une variable. Il ne sera pas possible de
	 * supprimer les identifiants poussés, donc il vaut mieux faire en sorte de
	 * pousser des données correctes dans un ordre correcte.
	 */
	void pousse(const DonneesType &autre);

	/**
	 * Retourne le type de base, à savoir le premier élément déclaré. Par
	 * exemple si nous déclarons '**n8', le type de base sera '*' (ID_POINTEUR),
	 * alors que pour 'n32', ce sera 'n32' (ID_E32).
	 *
	 * Cette fonction ne vérifie pas que les données sont valide, donc l'appeler
	 * sur des données invalide (vide) crashera le programme.
	 */
	id_morceau type_base() const;

	/**
	 * Retourne vrai si les données sont vides, ou si le dernier élément du
	 * tableau n'est ni un pointeur, ni un tableau, ni une référence.
	 */
	bool est_invalide() const;

	/**
	 * Retourne un itérateur constante vers le début de ces données. Puisque les
	 * données sont poussées dans l'ordre de déclaration du code (ex: **n8) et
	 * que nous avons besoin de l'ordre inverse pour construire le type LLVM
	 * (ex: n8**), l'itérateur est en fait un itérateur inverse et part de la
	 * fin des données.
	 */
	iterateur_const begin() const;

	/**
	 * Retourne un itérateur constante vers la fin de ces données. Puisque les
	 * données sont poussées dans l'ordre de déclaration du code (ex: **n8) et
	 * que nous avons besoin de l'ordre inverse pour construire le type LLVM
	 * (ex: n8**), l'itérateur est en fait un itérateur inverse et part du début
	 * des données.
	 */
	iterateur_const end() const;

	/**
	 * Retourne des données pour un type correspondant au déréférencement de ce
	 * type. Si le type n'est ni un pointeur, ni un tableau, retourne des
	 * données invalides.
	 */
	DonneesType derefence() const;
};

/**
 * Compare deux DonneesType et retourne vrai s'ils sont égaux. L'égalité est
 * déterminée par le type de base des données.
 */
inline bool operator==(const DonneesType &type_a, const DonneesType &type_b)
{
	return type_a.type_base() == type_b.type_base();
}

/**
 * Compare deux DonneesType et retourne vrai s'ils sont inégaux. L'inégalité est
 * déterminée par le type de base des données.
 */
inline bool operator!=(const DonneesType &type_a, const DonneesType &type_b)
{
	return !(type_a == type_b);
}

std::ostream &operator<<(std::ostream &os, const DonneesType &donnees_type);
