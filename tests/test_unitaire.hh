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

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

namespace dls {
namespace test_unitaire {

class Controleuse;

#if defined __cpp_concepts && __cpp_concepts >= 201507
/**
 * Concept pour les fonctions devant être passée à un controleur unitaire pour
 * vérifier les conditions y contenues.
 *
 * La fonction doit avoir la signature :
 * `void foo(Controleuse &controleur);`
 */
template <typename TypeFonction>
concept bool ConceptFonctionTest = requires(
		TypeFonction fonction,
		Controleuse &controleur)
{
	{ fonction(controleur) } -> void;
};

/**
 * Concept pour les comparaisons de valeurs.
 */
template <typename T>
concept bool ConceptComparaison = requires(T a, T b)
{
	{ a == b } -> bool;
	{ a != b } -> bool;
};

/**
 * Concept pour les valeurs décimales.
 */
template <typename T>
concept bool ConceptDecimal = std::is_floating_point<T>::value;
#else
#	define ConceptFonctionTest typename
#	define ConceptComparaison typename
#	define ConceptDecimal typename
#endif

class Controleuse {
	std::ostream &m_flux = std::cerr;
	size_t m_total = 0;

	using fonction_test = void(*)(Controleuse&);

	std::string m_proposition = "";

	std::string::size_type m_taille_max_erreur = 0ul;

	std::vector<fonction_test> m_fonctions = {};
	std::vector<std::string> m_echecs = {};

public:
	explicit Controleuse(std::ostream &os = std::cerr);

	/**
	 * Verifie que la condition est vraie. Si la condition n'est pas vraie, une
	 * note est ajoutée au flux de sortie contenant la cond_str, le fichier et
	 * la ligne passés en argument, autrement, un point est imprimé.
	 */
	void verifie(const bool condition, const char *cond_str, const char *fichier, const int ligne);

	/**
	 * Indique le début d'une proposition. Le paramètre est la raison de la
	 * proposition. Cette raison sera imprimée en introduction chaque erreur
	 * de la proposition
	 */
	void debute_proposition(const char *raison);

	/**
	 * Indique le début d'une proposition. Le paramètre est la raison de la
	 * proposition. Cette raison sera imprimée en introduction chaque erreur
	 * de la proposition
	 */
	void debute_proposition(const std::string &raison);

	/**
	 * Indique la fin d'une proposition. Si d'autres tests sont performés
	 * après cette opération, et qu'aucune autre proposition n'a été débuté,
	 * les messages d'erreur seront introduits par une raison nulle.
	 */
	void termine_proposition();

	/**
	 * Verifie que la fonction spécifiée jete ou execption. Si une exception
	 * n'est pas jetée, une note est ajoutée au flux de sortie contenant le
	 * nom_fonction, le fichier et la ligne passés en argument, autrement, un
	 * point est imprimé.
	 */
	template <typename Fonction>
	void verifie_exception_jetee(
			Fonction &&fonction,
			const char *nom_fonction,
			const char *fichier,
			const int ligne);

	/**
	 * Verifie que deux entités sont égales. S'il n'y pas d'égalité, une note
	 * est ajoutée au flux de sortie contenant le fichier et la ligne passés en
	 * argument, autrement, un point est imprimé.
	 */
	template <ConceptComparaison TypeComparable>
	void verifie_egalite(
			const TypeComparable a,
			const TypeComparable b,
			const char *fichier,
			const int ligne);

	/**
	 * Verifie que deux valeurs décimales sont égales. S'il n'y pas d'égalité,
	 * une note est ajoutée au flux de sortie contenant le fichier et la ligne
	 * passés en argument, autrement, un point
	 * est imprimé.
	 */
	template <ConceptDecimal TypeDecimal>
	void verifie_egalite_decimale(
			const TypeDecimal a,
			const TypeDecimal b,
			const char *fichier,
			const int ligne);

	/**
	 * Verifie que deux valeurs décimales sont égales dans un écart d'un epsilon
	 * donné. S'il n'y pas d'égalité, une note est ajoutée au flux de sortie
	 * contenant le fichier et la ligne passés en argument, autrement, un point
	 * est imprimé.
	 */
	template <ConceptDecimal TypeDecimal>
	void verifie_egalite_epsilon(
			const TypeDecimal a,
			const TypeDecimal b,
			const TypeDecimal epsilon,
			const char *fichier,
			const int ligne);

	/**
	 * Imprime les résultats du contrôle des tests dans le flux passé dans le
	 * constructeur.
	 */
	void imprime_resultat();

	/**
	 * Ajoute une fonction à ce contrôleur. La fonction doit prendre comme
	 * unique argument une référence vers un contrôleur et appeler la méthode
	 * Controleuse::verifie() soit directement, soit par
	 * CU_VERIFIE_CONDITION.
	 */
	template <ConceptFonctionTest FonctionTest>
	void ajoute_fonction(FonctionTest &&test)
	{
		m_fonctions.push_back(test);
	}

	/**
	 * Performe les tests présents dans les fonctions ajoutées à ce contrôleur.
	 */
	void performe_controles();

private:
	void pousse_erreur(const std::string &erreur);
};

template <ConceptComparaison TypeComparable>
void Controleuse::verifie_egalite(
		const TypeComparable a,
		const TypeComparable b,
		const char *fichier,
		const int ligne)
{
	if (!(a == b)) {
		std::stringstream ss;

		ss << fichier << ":" << ligne
		   << "\n\tLes valeurs ne sont pas égales : "
		   << a << " != " << b << " !";

		pousse_erreur(ss.str());
	}

	m_flux << ".";
	++m_total;
}

template <ConceptDecimal TypeDecimal>
void Controleuse::verifie_egalite_decimale(
		const TypeDecimal a,
		const TypeDecimal b,
		const char *fichier,
		const int ligne)
{
	if (std::abs(a - b) > std::numeric_limits<TypeDecimal>::epsilon()) {
		std::stringstream ss;

		ss << fichier << ":" << ligne
		   << "\n\tLes valeurs ne sont pas environ égales : "
		   << a << " != " << b << '!';

		pousse_erreur(ss.str());
	}

	m_flux << ".";
	++m_total;
}

template <ConceptDecimal TypeDecimal>
void Controleuse::verifie_egalite_epsilon(
		const TypeDecimal a,
		const TypeDecimal b,
		const TypeDecimal epsilon,
		const char *fichier,
		const int ligne)
{
	if (std::abs(a - b) > epsilon) {
		std::stringstream ss;

		ss << fichier << ":" << ligne
		   << "\n\tLes valeurs ne sont pas égales dans un écart d'epsilon : "
		   << a << " != " << b << " (epsilon : " << epsilon << ") !";

		pousse_erreur(ss.str());
	}

	m_flux << ".";
	++m_total;
}

template <typename Fonction>
void Controleuse::verifie_exception_jetee(
		Fonction &&fonction,
		const char *nom_fonction,
		const char *fichier,
		const int ligne)
{
	auto exception_jetee = false;

	try {
		fonction();
	}
	catch (...) {
		exception_jetee = true;
	}

	if (!exception_jetee) {
		std::stringstream ss;

		ss << fichier << ":" << ligne
		   << "\n\tLa fonction '" << nom_fonction
		   << "' n'a pas jeté d'exceptions !";

		pousse_erreur(ss.str());
	}

	m_flux << ".";
	++m_total;
}

}  /* namespace test_unitaire */
}  /* namespace dls */

#define CU_DEBUTE_PROPOSITION(__controleur, __proposition) \
	__controleur.debute_proposition(__proposition)

#define CU_TERMINE_PROPOSITION(__controleur) \
	__controleur.termine_proposition()

#define CU_VERIFIE_CONDITION(__controleur, __condition) \
	__controleur.verifie(__condition, #__condition, __FILE__, __LINE__)

#define CU_VERIFIE_EGALITE(__controleur, __a, __b) \
	__controleur.verifie_egalite(__a, __b, __FILE__, __LINE__)

#define CU_VERIFIE_EGALITE_DECIMALE(__controleur, __a, __b) \
	__controleur.verifie_egalite_decimale(__a, __b, __FILE__, __LINE__)

#define CU_VERIFIE_EGALITE_EPSILON(__controleur, __a, __b, __epsilon) \
	__controleur.verifie_egalite_epsilon(__a, __b, __epsilon, __FILE__, __LINE__)

#define CU_VERIFIE_EXCEPTION_JETEE(__controleur, __fonction) \
	__controleur.verifie_exception_jetee(__fonction, #__fonction, __FILE__, __LINE__)
