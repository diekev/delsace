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

#ifdef AVEC_LLVM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/ADT/SmallVector.h>
#pragma GCC diagnostic pop

namespace llvm {
class Type;
}
#endif

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/tableau_simple_compact.hh"

#include "morceaux.hh"

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

using type_plage_donnees_type = dls::plage_continue<const id_morceau>;

/* ************************************************************************** */

struct DonneesTypeDeclare {
	dls::tableau_simple_compact<id_morceau> donnees{};
	dls::tableau_simple_compact<noeud::base *> expressions{};

	dls::vue_chaine nom_gabarit = "";

	using type_plage = type_plage_donnees_type;

	id_morceau type_base() const;

	long taille() const;

	id_morceau operator[](long idx) const;

	void pousse(id_morceau id);

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
	dls::tableau<id_morceau> m_donnees{};

#ifdef AVEC_LLVM
	llvm::Type *m_type{nullptr};
#endif

public:
	using type_plage = type_plage_donnees_type;

	dls::chaine ptr_info_type{};

	using iterateur_const = dls::tableau<id_morceau>::const_iteratrice_inverse;

	DonneesTypeFinal() = default;

	explicit DonneesTypeFinal(id_morceau i0);

	DonneesTypeFinal(id_morceau i0, id_morceau i1);

	DonneesTypeFinal(type_plage autre);

	DonneesTypeFinal(const DonneesTypeFinal &) = default;
	DonneesTypeFinal &operator=(const DonneesTypeFinal &) = default;

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

inline bool est_type_tableau_fixe(id_morceau id)
{
	return (id != id_morceau::TABLEAU) && ((id & 0xff) == id_morceau::TABLEAU);
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

enum {
	TYPE_N8,
	TYPE_N16,
	TYPE_N32,
	TYPE_N64,
	TYPE_Z8,
	TYPE_Z16,
	TYPE_Z32,
	TYPE_Z64,
	TYPE_R16,
	TYPE_R32,
	TYPE_R64,
	TYPE_EINI,
	TYPE_CHAINE,
	TYPE_RIEN,
	TYPE_BOOL,
	TYPE_OCTET,

	TYPE_PTR_N8,
	TYPE_PTR_N16,
	TYPE_PTR_N32,
	TYPE_PTR_N64,
	TYPE_PTR_Z8,
	TYPE_PTR_Z16,
	TYPE_PTR_Z32,
	TYPE_PTR_Z64,
	TYPE_PTR_R16,
	TYPE_PTR_R32,
	TYPE_PTR_R64,
	TYPE_PTR_EINI,
	TYPE_PTR_CHAINE,
	TYPE_PTR_RIEN,
	TYPE_PTR_NUL,
	TYPE_PTR_BOOL,

	TYPE_REF_N8,
	TYPE_REF_N16,
	TYPE_REF_N32,
	TYPE_REF_N64,
	TYPE_REF_Z8,
	TYPE_REF_Z16,
	TYPE_REF_Z32,
	TYPE_REF_Z64,
	TYPE_REF_R16,
	TYPE_REF_R32,
	TYPE_REF_R64,
	TYPE_REF_EINI,
	TYPE_REF_CHAINE,
	TYPE_REF_RIEN,
	TYPE_REF_NUL,
	TYPE_REF_BOOL,

	TYPE_TABL_N8,
	TYPE_TABL_N16,
	TYPE_TABL_N32,
	TYPE_TABL_N64,
	TYPE_TABL_Z8,
	TYPE_TABL_Z16,
	TYPE_TABL_Z32,
	TYPE_TABL_Z64,
	TYPE_TABL_R16,
	TYPE_TABL_R32,
	TYPE_TABL_R64,
	TYPE_TABL_EINI,
	TYPE_TABL_CHAINE,
	TYPE_TABL_BOOL,
	TYPE_TABL_OCTET,

	TYPES_TOTAUX,
};

struct MagasinDonneesType {
	dls::dico_desordonne<DonneesTypeFinal, long> donnees_type_index{};
	dls::tableau<DonneesTypeFinal> donnees_types{};

	MagasinDonneesType();

	long ajoute_type(const DonneesTypeFinal &donnees);

	void converti_type_C(
			ContexteGenerationCode &contexte,
			dls::vue_chaine const &nom_variable,
			type_plage_donnees_type donnees,
			dls::flux_chaine &os,
			bool echappe = false,
			bool echappe_struct = false,
			bool echappe_tableau_fixe = false);

#ifdef AVEC_LLVM
	llvm::Type *converti_type(
			ContexteGenerationCode &contexte,
			DonneesType const &donnees);

	llvm::Type *converti_type(
			ContexteGenerationCode &contexte,
			size_t donnees);
#endif

	void declare_structures_C(
			ContexteGenerationCode &contexte,
			dls::flux_chaine &os);

	long operator[](int type);

private:
	dls::tableau<long> index_types_communs{};
};

/* ************************************************************************** */

[[nodiscard]] auto donnees_types_parametres(
		MagasinDonneesType &magasin,
		const DonneesTypeFinal &donnees_type,
		long &nombre_types_retour) noexcept(false) -> dls::tableau<long>;

#ifdef AVEC_LLVM
[[nodiscard]] llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		DonneesType &donnees_type);

[[nodiscard]] llvm::Type *converti_type_simple(
		ContexteGenerationCode &contexte,
		const id_morceau &identifiant,
		llvm::Type *type_entree);
#endif

[[nodiscard]] unsigned alignement(
		ContexteGenerationCode &contexte,
		const DonneesTypeFinal &donnees_type);

/* ************************************************************************** */

enum class type_noeud : char;

enum class niveau_compat : char {
	aucune                 = (     0),
	ok                     = (1 << 0),
	converti_tableau       = (1 << 1),
	converti_eini          = (1 << 2),
	extrait_eini           = (1 << 3),
	extrait_chaine_c       = (1 << 4),
	converti_tableau_octet = (1 << 5),
	prend_reference        = (1 << 6),
};

DEFINIE_OPERATEURS_DRAPEAU(niveau_compat, int)

/**
 * Retourne le niveau de compatibilité entre les deux types spécifiés.
 */
niveau_compat sont_compatibles(
		const DonneesTypeFinal &type1,
		const DonneesTypeFinal &type2,
		type_noeud type_droite);

/* ************************************************************************** */

bool est_type_entier(id_morceau type);

bool est_type_entier_naturel(id_morceau type);

bool est_type_entier_relatif(id_morceau type);

bool est_type_reel(id_morceau type);

unsigned int taille_type_octet(ContexteGenerationCode &contexte, DonneesTypeFinal const &donnees_type);
