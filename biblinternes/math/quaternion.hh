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

#include "matrice.hh"
#include "vecteur.hh"

namespace dls::math {

/**
 * Classe représentant un quaternion sous la forme d'un vecteur et d'un poids.
 *
 * Les données de la classe sont accessibles publiquement afin de permettre au
 * compileur de mieux optimiser le code : si nous utilisions des méthodes
 * d'accès et de modifications, étant donné que C++ transforme les méthodes en
 * passant un pointeur 'this' à celles-ci, le compileur serait obligé de passer
 * l'addresse d'un objet en mémoire pour satisfaire le besoin du pointeur 'this'
 * ce qui ne peut se faire dans le cache du processeur.
 */
template <ConceptNombre Nombre>
class quaternion {
public:
	vec3<Nombre> vecteur = vec3<Nombre>(static_cast<Nombre>(0), static_cast<Nombre>(0), static_cast<Nombre>(0));
	Nombre poids = static_cast<Nombre>(1);

	/**
	 * Construit un quaternion avec des valeurs par défaut : le vecteur est
	 * égal à 0, et le poids à 1.
	 */
	quaternion() = default;

	/**
	 * Construit un quaternion à partir d'un autre.
	 */
	quaternion(const quaternion &autre) = default;

	/**
	 * Construit un quaternion à partir d'un vecteur et d'un poids.
	 */
	quaternion(const vec3<Nombre> &v, const Nombre &p);

	/**
	 * Ajoute les valeurs du quaternion spécifié à celui-ci.
	 */
	quaternion &operator+=(const quaternion &autre);

	/**
	 * Soustrait les valeurs du quaternion spécifié de celui-ci.
	 */
	quaternion &operator-=(const quaternion &autre);

	/**
	 * Multiplie les valeurs de ce quaternion par celui spécifié.
	 */
	quaternion &operator*=(const quaternion &autre);

	/**
	 * Multiplie les valeurs de ce quaternion par la valeur spécifiée.
	 */
	quaternion &operator*=(const Nombre &valeur);

	/**
	 * Divise les valeurs de ce quaternion par la valeur spécifiée.
	 */
	quaternion &operator/=(const Nombre &valeur);
};

/* ************************************************************************** */

template <ConceptNombre Nombre>
quaternion<Nombre>::quaternion(const vec3<Nombre> &v, const Nombre &p)
	: vecteur(v)
	, poids(p)
{}

template <ConceptNombre Nombre>
quaternion<Nombre> &quaternion<Nombre>::operator+=(const quaternion &autre)
{
	vecteur += autre.vecteur;
	poids += autre.poids;

	return *this;
}

template <ConceptNombre Nombre>
quaternion<Nombre> &quaternion<Nombre>::operator-=(const quaternion &autre)
{
	vecteur -= autre.vecteur;
	poids -= autre.poids;

	return *this;
}

template <ConceptNombre Nombre>
quaternion<Nombre> &quaternion<Nombre>::operator*=(const quaternion &autre)
{
	auto vp = produit_vectorielle(vecteur, autre.vecteur);
	vp += vecteur * autre.poids;
	vp += autre.vecteur * poids;

	auto pp = poids * autre.poids - produit_scalaire(vecteur, autre.vecteur);

	vecteur = vp;
	poids = pp;

	return *this;
}

template <ConceptNombre Nombre>
quaternion<Nombre> &quaternion<Nombre>::operator*=(const Nombre &valeur)
{
	vecteur *= valeur;
	poids *= valeur;

	return *this;
}

template <ConceptNombre Nombre>
quaternion<Nombre> &quaternion<Nombre>::operator/=(const Nombre &valeur)
{
	vecteur /= valeur;
	poids /= valeur;

	return *this;
}

/* ************************************************************************** */

/**
 * Retourne le quaternion résultant de l'addition des deux quaternions spécifiés.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator+(const quaternion<Nombre> &a, const quaternion<Nombre> &b)
{
	quaternion<Nombre> resultat(a);
	resultat += b;
	return resultat;
}

/**
 * Retourne le quaternion résultant de l'addition des deux quaternions spécifiés.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator-(const quaternion<Nombre> &a, const quaternion<Nombre> &b)
{
	quaternion<Nombre> resultat(a);
	resultat -= b;
	return resultat;
}

/**
 * Retourne le quaternion résultant de la négation du quaternion spécifié.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator-(const quaternion<Nombre> &a)
{
	return quaternion<Nombre>(a);
}

/**
 * Retourne le quaternion résultant de la multiplication des deux quaternions
 * spécifiés.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator*(const quaternion<Nombre> &a, const quaternion<Nombre> &b)
{
	quaternion<Nombre> resultat(a);
	resultat *= b;
	return resultat;
}

/**
 * Retourne le quaternion résultant de la multiplication du quaternion par la
 * valeur spécifiée.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator*(const Nombre &a, const quaternion<Nombre> &b)
{
	quaternion<Nombre> resultat(b);
	resultat *= a;
	return resultat;
}

/**
 * Retourne le quaternion résultant de la multiplication du quaternion par la
 * valeur spécifiée.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator*(const quaternion<Nombre> &a, const Nombre &b)
{
	quaternion<Nombre> resultat(a);
	resultat *= b;
	return resultat;
}

/**
 * Retourne le quaternion résultant de la réciproque du quaternion par la valeur
 * spécifiée.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator/(const Nombre &a, const quaternion<Nombre> &b)
{
	quaternion<Nombre> resultat;
	resultat.vecteur = vec3<Nombre>(a, a, a);
	resultat.vecteur /= b.vecteur;
	resultat.poids = (b.poids == 0) ? 0 : a / b.poids;
	return resultat;
}

/**
 * Retourne le quaternion résultant de la division du quaternion par la valeur
 * spécifiée.
 */
template <ConceptNombre Nombre>
quaternion<Nombre> operator/(const quaternion<Nombre> &a, const Nombre &b)
{
	quaternion<Nombre> resultat(a);
	resultat /= b;
	return resultat;
}

/* ************************************************************************** */

/**
 * Retourne vrai si les valeurs des deux quaternions sont égales entre elles.
 */
template <ConceptNombre Nombre>
inline bool operator==(const quaternion<Nombre> &cote_gauche, const quaternion<Nombre> &cote_droit)
{
	return cote_gauche.vecteur == cote_droit.vecteur && cote_gauche.poids == cote_droit.poids;
}

/**
 * Retourne vrai si les valeurs des deux quaternions sont différentes.
 */
template <ConceptNombre Nombre>
inline bool operator!=(const quaternion<Nombre> &cote_gauche, const quaternion<Nombre> &cote_droit)
{
	return !(cote_gauche == cote_droit);
}

/* ************************************************************************** */

/**
 * Retourne le produit scalaire des deux quaternions, à savoir le cosinus de
 * l'angle entre eux.
 */
template <ConceptNombre Nombre>
inline auto produit_scalaire(
		const quaternion<Nombre> &cote_gauche,
		const quaternion<Nombre> &cote_droit)
{
	return produit_scalaire(cote_gauche.vecteur, cote_droit.vecteur)
			+ cote_gauche.poids * cote_droit.poids;
}

/**
 * Retourne un quaternion dont les valeurs correspondent à celles du quaternion
 * spécifié mais ayant une longueur égale à un.
 */
template <ConceptNombre Nombre>
inline auto normalise(const quaternion<Nombre> &q)
{
	auto lon = std::sqrt(produit_scalaire(q, q));

	if (lon != static_cast<Nombre>(0)) {
		return q / lon;
	}

	return quaternion<Nombre>(
	            vec3<Nombre>(static_cast<Nombre>(0),
	                         static_cast<Nombre>(1),
	                         static_cast<Nombre>(0)),
	            static_cast<Nombre>(0));
}

/**
 * Retourne un quaternion dont les valeurs correspondent à celles du quaternion
 * spécifié mais ayant une longueur égale à un.
 */
template <ConceptNombre Nombre>
inline auto interp_spherique(const quaternion<Nombre> &a, const quaternion<Nombre> &b, const Nombre &t)
{
	constexpr auto un = static_cast<Nombre>(1);
	const auto cos_theta = produit_scalaire(a, b);

	/* Si les deux quaternions sont presques parallèle, retourne une
	 * interpolation linéaire de ceux-ci pour éviter une instabilité numérique.
	 */
	if (cos_theta >= static_cast<Nombre>(0.9995)) {
		return normalise((un - t) * a + t * b);
	}

	const auto theta = std::acos(restreint(cos_theta, -un, un));
	const auto thetap = theta * t;
	const auto qperp = normalise(b - a * thetap);

	return a * std::cos(thetap) + qperp * std::sin(thetap);
}

template <ConceptNombre Nombre>
[[nodiscard]] auto quat_depuis_mat3(mat3x3<Nombre> const &mat)
{
	auto UN = static_cast<Nombre>(1.0);
	auto DEUX = static_cast<Nombre>(2.0);

	dls::math::quaternion<Nombre> q;

	/* À FAIRE : assert que la longueur des vecteurs lignes = 1.0 */

	auto tr = 0.25 * static_cast<double>(UN + mat[0][0] + mat[1][1] + mat[2][2]);

	if (tr > 1e-4) {
		auto s = std::sqrt(tr);
		q.vecteur[0] = static_cast<Nombre>(s);
		s = 1.0 / (4.0 * s);
		q.vecteur[1] = static_cast<Nombre>(static_cast<double>(mat[1][2] - mat[2][1]) * s);
		q.vecteur[2] = static_cast<Nombre>(static_cast<double>(mat[2][0] - mat[0][2]) * s);
		q.poids = static_cast<Nombre>(static_cast<double>(mat[0][1] - mat[1][0]) * s);
	}
	else {
		if (mat[0][0] > mat[1][1] && mat[0][0] > mat[2][2]) {
			auto s = static_cast<double>(DEUX * std::sqrt(UN + mat[0][0] - mat[1][1] - mat[2][2]));
			q.vecteur[1] = static_cast<Nombre>(0.25 * s);

			s = 1.0 / s;
			q.vecteur[0] = static_cast<Nombre>(static_cast<double>(mat[1][2] - mat[2][1]) * s);
			q.vecteur[2] = static_cast<Nombre>(static_cast<double>(mat[1][0] + mat[0][1]) * s);
			q.poids = static_cast<Nombre>(static_cast<double>(mat[2][0] + mat[0][2]) * s);
		}
		else if (mat[1][1] > mat[2][2]) {
			auto s = static_cast<double>(DEUX * std::sqrt(UN + mat[1][1] - mat[0][0] - mat[2][2]));
			q.vecteur[2] = static_cast<Nombre>(0.25 * s);

			s = 1.0 / s;
			q.vecteur[0] = static_cast<Nombre>(static_cast<double>(mat[2][0] - mat[0][2]) * s);
			q.vecteur[1] = static_cast<Nombre>(static_cast<double>(mat[1][0] + mat[0][1]) * s);
			q.poids = static_cast<Nombre>(static_cast<double>(mat[2][1] + mat[1][2]) * s);
		}
		else {
			auto s = static_cast<double>(DEUX * std::sqrt(UN + mat[2][2] - mat[0][0] - mat[1][1]));
			q.poids = static_cast<Nombre>(0.25 * s);

			s = 1.0 / s;
			q.vecteur[0] = static_cast<Nombre>(static_cast<double>(mat[0][1] - mat[1][0]) * s);
			q.vecteur[1] = static_cast<Nombre>(static_cast<double>(mat[2][0] + mat[0][2]) * s);
			q.vecteur[2] = static_cast<Nombre>(static_cast<double>(mat[2][1] + mat[1][2]) * s);
		}
	}

	return normalise(q);
}

template <ConceptNombre Nombre>
void loc_quat_depuis_mat4(
        mat4x4<Nombre> const &mat,
        vec3<Nombre> &loc,
        quaternion<Nombre> &quat)
{
	auto mat3 = mat3_depuis_mat4(mat);
	auto mat3_n = normalise(mat3);

	/* Pour qu'une taille négative n'interfère pas avec la rotation.
	 * NOTE : ceci est une solution pour contourner le fait que les matrices
	 * négatives ne fonctionne pas pour les conversions de rotations. À FIXER.
	 */
	if (est_negative(mat3)) {
		nie(mat3_n);
	}

	quat = quat_depuis_mat3(mat3_n);
	loc[0] = mat[3][0];
	loc[1] = mat[3][1];
	loc[2] = mat[3][2];
}

template <ConceptNombre Nombre>
[[nodiscard]] auto mat3_depuis_quat(quaternion<Nombre> const &q)
{
	auto const q0 = M_SQRT2 * static_cast<double>(q.vecteur[0]);
	auto const q1 = M_SQRT2 * static_cast<double>(q.vecteur[1]);
	auto const q2 = M_SQRT2 * static_cast<double>(q.vecteur[2]);
	auto const q3 = M_SQRT2 * static_cast<double>(q.poids);

	auto const qda = q0 * q1;
	auto const qdb = q0 * q2;
	auto const qdc = q0 * q3;
	auto const qaa = q1 * q1;
	auto const qab = q1 * q2;
	auto const qac = q1 * q3;
	auto const qbb = q2 * q2;
	auto const qbc = q2 * q3;
	auto const qcc = q3 * q3;

	mat3x3<Nombre> m;
	m[0][0] = static_cast<Nombre>(1.0 - qbb - qcc);
	m[0][1] = static_cast<Nombre>(qdc + qab);
	m[0][2] = static_cast<Nombre>(-qdb + qac);

	m[1][0] = static_cast<Nombre>(-qdc + qab);
	m[1][1] = static_cast<Nombre>(1.0 - qaa - qcc);
	m[1][2] = static_cast<Nombre>(qda + qbc);

	m[2][0] = static_cast<Nombre>(qdb + qac);
	m[2][1] = static_cast<Nombre>(-qda + qbc);
	m[2][2] = static_cast<Nombre>(1.0 - qaa - qbb);

	return m;
}

/**
 * Extraction de la rotation d'une matrice selon l'algorithme présenté dans
 * "A Robust Method to Extract the Rotational Part of Deformations"
 * http://matthias-mueller-fischer.ch/publications/stablePolarDecomp.pdf
 */
template <ConceptNombre Nombre>
void extrait_rotation(
        mat3x3<Nombre> const &A,
        quaternion<Nombre> &q,
        const unsigned int iter_max)
{
	for (unsigned int iter = 0; iter < iter_max; iter++) {
		auto R = mat3_depuis_quat(q);

		auto omega = (produit_croix(R[0], A[0])
		        + produit_croix(R[1], A[1])
		        + produit_croix(R[2], A[2]))
		        * (1.0 / std::fabs(produit_scalaire(R[0], A[0])
		                           + produit_scalaire(R[1], A[1])
		                           + produit_scalaire(R[2], A[2])) + 1.0e-9);

		double w = longueur(omega);

		if (w < 1.0e-9) {
			break;
		}

		q = normalise(Quaterniond(AngleAxisd(w, (1.0 / w) * omega)) * q);
	}
}

}  /* dls::math */
