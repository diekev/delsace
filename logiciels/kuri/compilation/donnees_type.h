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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/tableau_simple_compact.hh"

#include "lexemes.hh"

/**
 * Système de type.
 *
 * Puisque qu'il est possible d'associer des expressions aux types, le système
 * de type est basé autour de deux structures : DonneesTypeDeclare et
 * DonneesTypeFinal. La première stocke les morceaux des types et leurs
 * expressions tel qu'ils ont été écris dans le programme, alors que la seconde
 * stocke ceux des types finaux. Les types finaux sont résolus selon le contexte
 * lors de l'analyse syntactique.
 *
 * Par exemple, voici quelques cas où les types déclarés ont des éléments
 * différents mais pointent vers un même type final :
 * - [1024]z32 et [2 * 512]z32
 * - a : z32; et b : type_de(a);
 *
 * Ce niveau d'indirection nous permet également d'avoir un système de gabarit
 * où les types déclarés possèdent les informations sur les gabarits et nous
 * aident à résoudre leurs types finaux lors des appels.
 */

namespace noeud {
struct base;
}

struct ContexteGenerationCode;

using type_plage_donnees_type = dls::plage_continue<const GenreLexeme>;

/* ************************************************************************** */

struct DonneesTypeDeclare {
	dls::tableau_simple_compact<GenreLexeme> donnees{};
	dls::tableau_simple_compact<noeud::base *> expressions{};

	dls::vue_chaine nom_gabarit = "";

	using type_plage = type_plage_donnees_type;

	GenreLexeme type_base() const;

	long taille() const;

	GenreLexeme operator[](long idx) const;

	void pousse(GenreLexeme id);

	void pousse(DonneesTypeDeclare const &dtd);

	type_plage plage() const;

	/**
	 * Retourne des données pour un type correspondant au déréférencement de ce
	 * type. Si le type n'est ni un pointeur, ni un tableau, retourne des
	 * données invalides.
	 */
	type_plage dereference() const;
};

/* ************************************************************************** */

#ifdef AVEC_LLVM
namespace llvm {
class Type;
}
#endif

/**
 * Classe pour gérer les données du type d'une variable ou d'une constante. En
 * l'espèce, la classe contient un vecteur qui peut contenir un nombre variable
 * de pointeurs, de références, et de tableaux, mais le dernier éléments du
 * vecteur devra forcément être vers un type connu (entier, réel, booléen,
 * structure, etc...).
 */
struct DonneesTypeFinal {
private:
	/* À FAIRE : type similaire à llvm::SmallVector. */
	dls::tableau<GenreLexeme> m_donnees{};

#ifdef AVEC_LLVM
	llvm::Type *m_type{nullptr};
#endif

public:
	using type_plage = type_plage_donnees_type;

	dls::chaine ptr_info_type{};
	dls::chaine nom_broye{};

	using iterateur_const = dls::tableau<GenreLexeme>::const_iteratrice_inverse;

	DonneesTypeFinal() = default;

	explicit DonneesTypeFinal(GenreLexeme i0);

	DonneesTypeFinal(GenreLexeme i0, GenreLexeme i1);

	DonneesTypeFinal(type_plage autre);

	DonneesTypeFinal(const DonneesTypeFinal &) = default;
	DonneesTypeFinal &operator=(const DonneesTypeFinal &) = default;

	/**
	 * Ajoute un identifiant à ces données. Il ne sera pas possible de supprimer
	 * l'identifiant poussé, donc il vaut mieux faire en sorte de pousser des
	 * données correctes dans un ordre correcte.
	 */
	void pousse(GenreLexeme identifiant);

	/**
	 * Pousse les identifiants d'un autre vecteur de données dans celui-ci.
	 * Cette fonction est principalement là pour générer les données relatives
	 * à la prise de l'addresse d'une variable. Il ne sera pas possible de
	 * supprimer les identifiants poussés, donc il vaut mieux faire en sorte de
	 * pousser des données correctes dans un ordre correcte.
	 */
	void pousse(const DonneesTypeFinal &autre);

	/**
	 * Pousse les identifiants d'un autre vecteur de données dans celui-ci.
	 * Cette fonction est principalement là pour générer les données relatives
	 * à la prise de l'addresse d'une variable. Il ne sera pas possible de
	 * supprimer les identifiants poussés, donc il vaut mieux faire en sorte de
	 * pousser des données correctes dans un ordre correcte.
	 */
	void pousse(type_plage_donnees_type autre);

	/**
	 * Retourne le type de base, à savoir le premier élément déclaré. Par
	 * exemple si nous déclarons '**n8', le type de base sera '*' (ID_POINTEUR),
	 * alors que pour 'n32', ce sera 'n32' (ID_E32).
	 *
	 * Cette fonction ne vérifie pas que les données sont valide, donc l'appeler
	 * sur des données invalide (vide) crashera le programme.
	 */
	GenreLexeme type_base() const;

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

	type_plage plage() const;

	/**
	 * Retourne des données pour un type correspondant au déréférencement de ce
	 * type. Si le type n'est ni un pointeur, ni un tableau, retourne des
	 * données invalides.
	 */
	type_plage dereference() const;

	long taille() const;

#ifdef AVEC_LLVM
	llvm::Type *type_llvm() const
	{
		return m_type;
	}

	void type_llvm(llvm::Type *tllvm)
	{
		m_type = tllvm;
	}
#endif
};

/**
 * Compare deux DonneesType et retourne vrai s'ils sont égaux.
 */
[[nodiscard]] inline bool operator==(type_plage_donnees_type type_a, type_plage_donnees_type type_b) noexcept
{
	/* Petite optimisation. */
	if (type_a.taille() != type_b.taille()) {
		return false;
	}

	while (!type_a.est_finie()) {
		if (type_a.front() != type_b.front()) {
			return false;
		}

		type_a.effronte();
		type_b.effronte();
	}

	return true;
}

[[nodiscard]] inline bool operator==(const DonneesTypeFinal &type_a, const DonneesTypeFinal &type_b) noexcept
{
	return (type_a.plage() == type_b.plage());
}

[[nodiscard]] inline bool operator==(const DonneesTypeDeclare &type_a, const DonneesTypeDeclare &type_b) noexcept
{
	return (type_a.plage() == type_b.plage());
}

[[nodiscard]] inline bool operator==(DonneesTypeFinal const &type_a, type_plage_donnees_type type_b) noexcept
{
	return (type_a.plage() == type_b);
}

[[nodiscard]] inline bool operator==(type_plage_donnees_type type_a, DonneesTypeFinal const &type_b) noexcept
{
	return (type_a == type_b.plage());
}

/**
 * Compare deux DonneesType et retourne vrai s'ils sont inégaux.
 */
[[nodiscard]] inline bool operator!=(DonneesTypeFinal const &type_a, DonneesTypeFinal const &type_b) noexcept
{
	return !(type_a == type_b);
}

[[nodiscard]] inline bool operator!=(DonneesTypeFinal const &type_a, type_plage_donnees_type type_b) noexcept
{
	return !(type_a == type_b);
}

[[nodiscard]] inline bool operator!=(type_plage_donnees_type type_a, DonneesTypeFinal const &type_b) noexcept
{
	return !(type_a == type_b);
}

[[nodiscard]] inline bool operator!=(type_plage_donnees_type type_a, type_plage_donnees_type type_b) noexcept
{
	return !(type_a == type_b);
}

dls::chaine chaine_type(DonneesTypeFinal const &donnees_type, ContexteGenerationCode const &contexte);

inline bool est_type_tableau_fixe(GenreLexeme id)
{
	return (id != GenreLexeme::TABLEAU) && ((id & 0xff) == GenreLexeme::TABLEAU);
}

inline bool est_type_tableau_fixe(DonneesTypeFinal const &dt)
{
	return est_type_tableau_fixe(dt.type_base());
}

inline bool est_type_tableau_fixe(type_plage_donnees_type dt)
{
	return est_type_tableau_fixe(dt.front());
}

inline bool est_invalide(type_plage_donnees_type p)
{
	if (p.est_finie()) {
		return true;
	}

	return false;
}

/* ************************************************************************** */

namespace std {

template <>
struct hash<DonneesTypeFinal> {
	/* Utilisation de l'algorithme DJB2 pour générer une empreinte. */
	size_t operator()(const DonneesTypeFinal &donnees) const
	{
		auto empreinte = 5381ul;

		for (auto const &id : donnees) {
			empreinte = ((empreinte << 5) + empreinte) + static_cast<size_t>(id);
		}

		return empreinte;
	}
};

}

/* ************************************************************************** */

[[nodiscard]] unsigned alignement(
		ContexteGenerationCode &contexte,
		const DonneesTypeFinal &donnees_type);

void ajoute_contexte_programme(
		ContexteGenerationCode &contexte,
		DonneesTypeDeclare &dt);

/* ************************************************************************** */

unsigned int taille_octet_type(ContexteGenerationCode const &contexte, DonneesTypeFinal const &donnees_type);
