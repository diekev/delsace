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
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1201, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "../concepts.hh"

#include "dimensions.hh"

#include <cstring>  /* Pour std::memcpy, std::memcmp. */
#include <iostream>

#include "biblinternes/structures/chaine.hh"

namespace dls {
namespace math {

/**
 * Type d'exception lancée quand il y a une erreur de valeur.
 */
class ExceptionValeur {
	const dls::chaine &m_quoi;

public:
	/**
	 * Force la création d'une exception avec un message.
	 */
	ExceptionValeur() = delete;

	/**
	 * Crée une exception avec le message d'erreur.
	 */
	explicit ExceptionValeur(const dls::chaine &quoi)
		: m_quoi(quoi)
	{}

	/**
	 * Retourne le message d'erreur.
	 */
	const dls::chaine &quoi() const
	{
		return m_quoi;
	}
};

template <ConceptNombre Nombre>
class matrice_dyn;

/**
 * Copie les données d'une matrice vers celles d'une autre.
 *
 * Si les matrices ont des dimensions différentes, une exception de type
 * ExceptionValeur est lancée.
 */
template <ConceptNombre Nombre>
static auto copie(const matrice_dyn<Nombre> &de, matrice_dyn<Nombre> &vers)
{
	if (de.dimensions() != vers.dimensions()) {
		throw ExceptionValeur(
					"Impossible de copier des matrices qui ont des dimensions"
					" différentes.");
	}

	/* cppcheck-suppress syntaxError */
	if constexpr (std::is_trivially_copyable<Nombre>::value) {
		std::memcpy(vers.donnees(),
					de.donnees(),
		            sizeof(Nombre) * static_cast<unsigned>(de.dimensions().nombre_elements()));
	}
	else {
		for (int i = 0; i < de.nombre_lignes(); ++i) {
			for (int j = 0; j < de.nombre_colonnes(); ++j) {
				vers[i][j] = de[i][j];
			}
		}
	}
}

/**
 * Classe représentant une matrice. Les données de la matrice sont stockées
 * sous forme d'un tableau unidimensionnelle, et l'indexation dans ce tableau
 * se fait en ligne-majeure, c'est-à-dire matrice[0][2] accède la valeur à la
 * première ligne, troisième colonne.
 */
template <ConceptNombre Nombre>
class matrice_dyn {
	Dimensions m_dimensions{};

	Nombre *m_donnees = nullptr;

public:
	/**
	 * Construit une matrice par défaut. Le contenu et les dimensions de la
	 * matrice ne sont pas initialisées.
	 */
	matrice_dyn() = default;

	/**
	 * Construit une matrice avec les dimensions spécifiés. Le contenu de la
	 * matrice n'est pas initialié.
	 */
	explicit matrice_dyn(const Dimensions &dimensions);

	/**
	 * Construit une matrice avec la hauteur et la largeur spécifiées. Le
	 * contenu de la matrice n'est pas initialié.
	 */
	matrice_dyn(const Hauteur &hauteur, const Largeur &largeur);

	/**
	 * Construit une matrice avec la hauteur et la largeur spécifiées. Le
	 * contenu de la matrice n'est pas initialié.
	 */
	matrice_dyn(const Largeur &largeur, const Hauteur &hauteur);

	/**
	 * Construit une matrice à partir d'une autre. Après cette opération, la
	 * condition *this == autre est vraie.
	 */
	matrice_dyn(const matrice_dyn &autre);

	/**
	 * Construit une matrice en prenant les valeurs d'une autre. Après cette
	 * opération, l'autre matrice est laissée dans un état indéfini.
	 */
	matrice_dyn(matrice_dyn &&autre);

	/**
	 * Construit une matrice bidimensionnelle depuis une liste de listes.
	 */
	explicit matrice_dyn(const std::initializer_list<std::initializer_list<Nombre>> &listes_valeurs);

	/**
	 * Détruit la matrice. La mémoire pointée par donnees() est libéré, et
	 * l'utilisation du pointeur est indéfini.
	 */
	~matrice_dyn();

	/**
	 * Remplie la matrice avec la valeur passée en argument.
	 */
	void remplie(const Nombre valeur);

	/**
	 * Retourne les dimensions de la matrice.
	 */
	const Dimensions &dimensions() const;

	/**
	 * Retourne le nombre de colonnes ou la largeur de la matrice.
	 */
	int nombre_colonnes() const;

	/**
	 * Retourne le nombre de lignes ou la hauteur de la matrice.
	 */
	int nombre_lignes() const;

	/**
	 * Redimensionne la matrice selon les nouvelles dimensions spécifiées. Les
	 * nouvelles dimensions de la matrice doivent être compatibles avec les
	 * anciennes. Si les dimensions ne sont pas compatibles, lance une instance
	 * de la classe ExceptionValeur.
	 */
	void redimensionne(const Dimensions &nouvelles_dimensions);

	/**
	 * Échange les valeurs de cette matrice avec celle de l'autre.
	 */
	void swap(matrice_dyn &autre);

	/**
	 * Retourne un pointeur vers les données de cette matrice.
	 */
	Nombre *donnees() const;

	/**
	 * Retourne un pointeur vers le début de la ligne qui a pour index i.
	 * Aucune vérification sur les bornes des dimensions n'est effectuée, et
	 * accéder une ligne en dehors des bornes de la matrice est indéfini.
	 */
	Nombre *operator[](int i);

	/**
	 * Retourne un pointeur const vers le début de la ligne qui a pour index i.
	 * Aucune vérification sur les bornes des dimensions n'est effectuée, et
	 * accéder une ligne en dehors des bornes de la matrice est indéfini.
	 */
	Nombre *operator[](int i) const;

	/**
	 * Crée une matrice identité à partir des dimensions spécifiées. Si les
	 * dimensions ne sont pas carré, une exception est lancée.
	 */
	static matrice_dyn identite(const Dimensions &dimensions);

	/**
	 * Assigne les valeurs d'une autre matrice dans celle-ci. Après cette
	 * opération, la condition *this == autre est vraie.
	 */
	matrice_dyn &operator=(const matrice_dyn &autre);

	/**
	 * Assigne les valeurs d'une autre matrice dans celle-ci. Après cette
	 * opération, l'autre matrice est laissée dans un état indéfini.
	 */
	matrice_dyn &operator=(matrice_dyn &&autre);

	/**
	 * Additionne les valeurs d'une autre matrice à celle-ci.
	 *
	 * Si les matrice ont des dimensions différentes, une exception de type
	 * ExceptionValeur est lancée.
	 */
	matrice_dyn &operator+=(const matrice_dyn &autre);

	/**
	 * Soustrait les valeurs d'une autre matrice à celle-ci.
	 *
	 * Si les matrice ont des dimensions différentes, une exception de type
	 * ExceptionValeur est lancée.
	 */
	matrice_dyn &operator-=(const matrice_dyn &autre);

	/**
	 * Multiplie cette matrice avec une autre.
	 *
	 * Si les matrice ont des dimensions incompatibles, une exception de type
	 * ExceptionValeur est lancée.
	 */
	matrice_dyn &operator*=(const matrice_dyn &autre);

	/**
	 * Multiplie chaque valeur de cette matrice avec la valeur spécificiée.
	 */
	matrice_dyn &operator*=(const Nombre valeur);
};

/* ***************************** Implémentation ***************************** */

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(const Dimensions &dimensions)
	: m_dimensions(dimensions)
{
	m_donnees = new Nombre[dimensions.nombre_elements()];
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(const Hauteur &hauteur, const Largeur &largeur)
	: matrice_dyn(Dimensions(hauteur, largeur))
{}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(const Largeur &largeur, const Hauteur &hauteur)
	: matrice_dyn(Dimensions(hauteur, largeur))
{}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(const matrice_dyn &autre)
	: matrice_dyn(autre.dimensions())
{
	copie(autre, *this);
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(matrice_dyn &&autre)
	: matrice_dyn()
{
	swap(autre);
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::matrice_dyn(const std::initializer_list<std::initializer_list<Nombre> > &listes_valeurs)
	: matrice_dyn()
{
	const auto lignes = listes_valeurs.size();
	const auto colonnes = listes_valeurs.begin()->size();

	m_dimensions = Dimensions(Hauteur(static_cast<int>(lignes)), Largeur(static_cast<int>(colonnes)));
	m_donnees = new Nombre[lignes * colonnes];

	auto index = 0;

	for (const auto &liste : listes_valeurs) {
		for (const auto &valeur : liste) {
			m_donnees[index++] = valeur;
		}
	}
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre>::~matrice_dyn()
{
	if (!m_donnees) {
		return;
	}

	delete [] m_donnees;
}

template<ConceptNombre Nombre>
void matrice_dyn<Nombre>::remplie(const Nombre valeur)
{
	for (int i = 0, ie = dimensions().nombre_elements(); i < ie; ++i) {
		m_donnees[i] = valeur;
	}
}

template<ConceptNombre Nombre>
const Dimensions &matrice_dyn<Nombre>::dimensions() const
{
	return m_dimensions;
}

template<ConceptNombre Nombre>
int matrice_dyn<Nombre>::nombre_colonnes() const
{
	return m_dimensions.largeur;
}

template<ConceptNombre Nombre>
int matrice_dyn<Nombre>::nombre_lignes() const
{
	return m_dimensions.hauteur;
}

template<ConceptNombre Nombre>
void matrice_dyn<Nombre>::redimensionne(const Dimensions &nouvelles_dimensions)
{
	if (this->dimensions() == nouvelles_dimensions) {
		return;
	}

	const auto elements = this->dimensions().nombre_elements();
	const auto nouveaux_elements = nouvelles_dimensions.nombre_elements();

	/* Les dimensions sont différentes, mais le nombre d'éléments reste le
	 * même. */
	if (elements != nouveaux_elements) {
		throw ExceptionValeur(
					"Les nouvelles dimensions ne sont pas compatibles avec les"
					" anciennes !");
	}

	m_dimensions = nouvelles_dimensions;
}

template<ConceptNombre Nombre>
void matrice_dyn<Nombre>::swap(matrice_dyn &autre)
{
	std::swap(m_dimensions, autre.m_dimensions);
	std::swap(m_donnees, autre.m_donnees);
}

template<ConceptNombre Nombre>
Nombre *matrice_dyn<Nombre>::donnees() const
{
	return m_donnees;
}

template<ConceptNombre Nombre>
Nombre *matrice_dyn<Nombre>::operator[](int i)
{
	return &m_donnees[i * m_dimensions.largeur];
}

template<ConceptNombre Nombre>
Nombre *matrice_dyn<Nombre>::operator[](int i) const
{
	return &m_donnees[i * m_dimensions.largeur];
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> matrice_dyn<Nombre>::identite(const Dimensions &dimensions)
{
	if (dimensions.hauteur != dimensions.largeur) {
		throw ExceptionValeur("Une matrice identité doit être carré !");
	}

	auto resultat = matrice_dyn<Nombre>(dimensions);
	resultat.remplie(0);

	for (int i = 0, ie = resultat.nombre_lignes(); i < ie; ++i) {
		resultat[i][i] = static_cast<Nombre>(1);
	}

	return resultat;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator=(const matrice_dyn<Nombre> &autre)
{
	if (this->dimensions().nombre_elements() != autre.dimensions().nombre_elements()) {
		delete m_donnees;
		m_donnees = new Nombre[autre.dimensions().nombre_elements()];
	}

	this->m_dimensions = autre.dimensions();
	copie(autre, *this);

	return *this;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator=(matrice_dyn<Nombre> &&autre)
{
	swap(autre);
	return *this;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator+=(const matrice_dyn<Nombre> &autre)
{
	if (this->dimensions() != autre.dimensions()) {
		throw ExceptionValeur(
					"La somme matricielle n'est définie que si les"
					" dimensions des deux matrices sont égales !");
	}

	for (int i = 0; i < this->dimensions().nombre_elements(); ++i) {
		m_donnees[i] += autre.donnees()[i];
	}

	return *this;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator-=(const matrice_dyn<Nombre> &autre)
{
	if (this->dimensions() != autre.dimensions()) {
		throw ExceptionValeur(
					"La différence matricielle n'est définie que si les"
					" dimensions des deux matrices sont égales !");
	}

	for (int i = 0; i < this->dimensions().nombre_elements(); ++i) {
		m_donnees[i] -= autre.donnees()[i];
	}

	return *this;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator*=(const matrice_dyn &autre)
{
	if (this->nombre_colonnes() != autre.nombre_lignes()) {
		throw ExceptionValeur(
					"Le produit matricielle n'est défini que si le nombre de"
					" colonnes de la première matrice est égale au nombre de "
					"lignes de la seconde.");
	}

	auto resultat = matrice_dyn(Hauteur(nombre_lignes()), Largeur(autre.nombre_colonnes()));

	for (int l = 0, fl = resultat.nombre_lignes(); l < fl; ++l) {
		for (int c = 0, fc = resultat.nombre_colonnes(); c < fc; ++c) {
			Nombre valeur = 0;

			for (int m = 0, fm = autre.nombre_lignes(); m < fm; ++m) {
				valeur += (*this)[l][m] * autre[m][c];
			}

			resultat[l][c] = valeur;
		}
	}

	swap(resultat);
	return *this;
}

template<ConceptNombre Nombre>
matrice_dyn<Nombre> &matrice_dyn<Nombre>::operator*=(const Nombre valeur)
{
	for (int i = 0; i < this->dimensions().nombre_elements(); ++i) {
		m_donnees[i] *= valeur;
	}

	return *this;
}

/* ******************************* Opérateurs ******************************* */

/**
 * Retourne une matrice dont les valeurs résultent de l'addition des deux
 * matrices spécifiées.
 */
template <ConceptNombre Nombre>
auto operator+(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	auto tmp = matrice_dyn<Nombre>(cote_gauche);
	tmp += cote_droit;
	return tmp;
}

/**
 * Retourne une matrice dont les valeurs résultent de la soustraction des
 * valeurs de la matrice cote_droit à celles de cote_gauche.
 */
template <ConceptNombre Nombre>
auto operator-(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	auto tmp = matrice_dyn<Nombre>(cote_gauche);
	tmp -= cote_droit;
	return tmp;
}

/**
 * Retourne une matrice dont les valeurs résultent de la multiplication des deux
 * matrices spécifiées.
 */
template <ConceptNombre Nombre>
auto operator*(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	auto tmp = matrice_dyn<Nombre>(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

/**
 * Retourne une matrice dont les valeurs résultent de la multiplication de la
 * matrice spécifiée avec le nombre spécifié.
 */
template <ConceptNombre Nombre>
auto operator*(const matrice_dyn<Nombre> &cote_gauche, const Nombre cote_droit)
{
	auto tmp = matrice_dyn<Nombre>(cote_gauche);
	tmp *= cote_droit;
	return tmp;
}

/**
 * Retourne une matrice dont les valeurs résultent de la multiplication de la
 * matrice spécifiée avec le nombre spécifié.
 */
template <ConceptNombre Nombre>
auto operator*(const Nombre cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	auto tmp = matrice_dyn<Nombre>(cote_droit);
	tmp *= cote_gauche;
	return tmp;
}

/**
 * Vérifie que les deux matrices spécifiées sont égales.
 */
template <ConceptNombre Nombre>
bool operator==(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	if (cote_gauche.dimensions() != cote_droit.dimensions()) {
		return false;
	}

	/* cppcheck-suppress syntaxError */
	if constexpr (std::is_trivially_copyable<Nombre>::value) {
		return std::memcmp(
					cote_gauche.donnees(),
					cote_droit.donnees(),
					sizeof(Nombre) * static_cast<unsigned>(cote_gauche.dimensions().nombre_elements())) == 0;
	}
	else {
		for (int i = 0; i < cote_gauche.dimensions().nombre_elements(); ++i) {
			if (cote_gauche.donnees()[i] != cote_droit.donnees()[i]) {
				return false;
			}
		}

		return true;
	}
}

/**
 * Vérifie que les deux matrices spécifiées sont différentes.
 */
template <ConceptNombre Nombre>
bool operator!=(const matrice_dyn<Nombre> &cote_gauche, const matrice_dyn<Nombre> &cote_droit)
{
	return !(cote_gauche == cote_droit);
}

/**
 * Imprime les données de la matrice spécifiée dans le flux donné.
 */
template <ConceptNombre Nombre>
auto &operator<<(std::ostream &os, const matrice_dyn<Nombre> &mat)
{
	for (int i = 0; i < mat.nombre_lignes(); ++i) {
		os << "[";

		for (int j = 0; j < mat.nombre_colonnes(); ++j) {
			os << mat[i][j] << " ";
		}

		os << "]\n";
	}

	return os;
}

}  /* namespace math */
}  /* namespace dls */
