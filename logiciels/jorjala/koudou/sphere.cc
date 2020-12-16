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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "sphere.hh"

#include "types.hh"

namespace kdo {

template <typename T>
bool resoud_quadratique(const T &a, const T &b, const T &c, T &x0, T &x1)
{
	auto discr = dls::math::difference_de_produits(b, b, 4.0 * a, c);

	if (discr < 0) {
		return false;
	}

	if (discr == 0.0) {
		x0 = x1 = - 0.5 * b / a;
	}
	else {
		auto q = (b > 0.0) ?
			-0.5 * (b + std::sqrt(discr)) :
			-0.5 * (b - std::sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}

	if (x0 > x1) {
		std::swap(x0, x1);
	}

	return true;
}

/**
 * Entresection d'un rayon avec une sphère tirée de
 * https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
 */
template <typename T>
static bool entresecte_sphere(
		dls::math::point3<T> const &position,
		T const &rayon_sphere2,
		dls::phys::rayon<T> const &rayon,
		T &distance)
{
	/* solutions pour t s'il y a une entresection */
	T t0, t1;

#if 1
	/* solution géométrique */
	auto L = position - rayon.origine;
	auto tca = produit_scalaire(L, rayon.direction);
	// if (tca < 0) return false;
	auto d2 = produit_scalaire(L, L) - tca * tca;

	if (d2 > rayon_sphere2) {
		return false;
	}

	auto thc = std::sqrt(rayon_sphere2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;
#else
	/* solution analytique */
	auto L = rayon.origine - position;
	auto a = produit_scalaire(rayon.direction, rayon.direction);
	auto b = 2.0 * produit_scalaire(rayon.direction, L);
	auto c = produit_scalaire(L, L) - rayon_sphere2;

	if (!resoud_quadratique(a, b, c, t0, t1)) {
		return false;
	}
#endif
	if (t0 > t1) {
		std::swap(t0, t1);
	}

	if (t0 < 0) {
		/* si t0 est négatif, utilisons plutôt t1 */
		t0 = t1;

		if (t0 < 0) {
			/* t0 et t1 sont négatifs */
			return false;
		}
	}

	distance = t0;

	return true;
}

/* ************************************************************************** */

delegue_sphere::delegue_sphere(sphere const &s)
	: ptr_sphere(s)
{}

long delegue_sphere::nombre_elements() const
{
	return 1;
}

void delegue_sphere::coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
{
	INUTILISE(idx);
	cos.efface();
	cos.ajoute(ptr_sphere.point - dls::math::vec3f(ptr_sphere.rayon));
	cos.ajoute(ptr_sphere.point + dls::math::vec3f(ptr_sphere.rayon));
}

dls::phys::esectd delegue_sphere::intersecte_element(long idx, const dls::phys::rayond &rayon) const
{
	INUTILISE(idx);
	auto distance = 1000.0;
	auto entresection = dls::phys::esectd();
	entresection.type = ESECT_OBJET_TYPE_AUCUN;

	auto centre = dls::math::converti_type_point<double>(ptr_sphere.point);

	if (entresecte_sphere(centre, ptr_sphere.rayon2, rayon, distance)) {
		entresection.idx_objet = ptr_sphere.index;
		entresection.idx = idx;
		entresection.distance = distance;
		entresection.type = ESECT_OBJET_TYPE_SPHERE;
		entresection.touche = true;

		auto point = rayon.origine + distance * rayon.direction;
		entresection.normal = normalise(dls::math::vec3d(point) - dls::math::vec3d(ptr_sphere.point));
	}

	return entresection;
}

/* ************************************************************************** */

sphere::sphere()
	: noeud(type_noeud::SPHERE)
	, delegue(*this)
{}

void sphere::construit_arbre_hbe()
{
	this->rayon2 = static_cast<double>(this->rayon) * static_cast<double>(this->rayon);
	arbre_hbe = bli::cree_arbre_bvh(delegue);
}

dls::phys::esectd sphere::traverse_arbre(const dls::phys::rayond &r)
{
	return bli::traverse(arbre_hbe, delegue, r);
}

limites3d sphere::calcule_limites()
{
	auto lims = limites3d();
	lims.min = dls::math::converti_type<double>(this->point - dls::math::vec3f(this->rayon));
	lims.max = dls::math::converti_type<double>(this->point + dls::math::vec3f(this->rayon));
	return lims;
}

}  /* namespace kdo */
