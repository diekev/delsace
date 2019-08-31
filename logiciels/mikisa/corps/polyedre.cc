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

#include "polyedre.hh"

#include "biblinternes/structures/dico_desordonne.hh"

#include "corps.h"
#include "iteration_corps.hh"
#include "listes.h"

Polyedre::~Polyedre()
{
	for (auto s : sommets) {
		memoire::deloge("mi_sommet", s);
	}

	for (auto a : aretes) {
		memoire::deloge("mi_arete", a);
	}

	for (auto t : faces) {
		memoire::deloge("mi_face", t);
	}
}

mi_sommet *Polyedre::cree_sommet(const dls::math::vec3f &p)
{
	auto s = memoire::loge<mi_sommet>("mi_sommet");
	s->p = p;
	s->index = sommets.taille();

	sommets.pousse(s);

	return s;
}

mi_arete *Polyedre::cree_arete(mi_sommet *s, mi_face *f)
{
	auto a = memoire::loge<mi_arete>("mi_arete");
	a->sommet = s;
	a->face = f;
	a->index = aretes.taille();

	s->arete = a;

	aretes.pousse(a);

	return a;
}

mi_face *Polyedre::cree_face()
{
	auto f = memoire::loge<mi_face>("mi_face");
	f->index = faces.taille();
	faces.pousse(f);
	return f;
}

bool valide_polyedre(const Polyedre &polyedre)
{
	std::cerr << "polyedre :\n"
			  << "\tsommets   : " << polyedre.sommets.taille() << '\n'
			  << "\tarrètes   : " << polyedre.aretes.taille() << '\n'
			  << "\tfaces     : " << polyedre.faces.taille() << '\n'
			  << '\n';

	for (auto s : polyedre.sommets) {
		if (s->arete == nullptr) {
			return false;
		}
	}

	for (auto a : polyedre.aretes) {
		if (a->sommet == nullptr) {
			return false;
		}

		if (a->face == nullptr) {
			return false;
		}

		if (a->paire == nullptr) {
			return false;
		}

		if (a->suivante == nullptr) {
			return false;
		}
	}

	for (auto f : polyedre.faces) {
		if (f->arete == nullptr) {
			return false;
		}
	}

	return true;
}

inline auto index_arete(long i0, long i1)
{
	return static_cast<size_t>(i0 | (i1 << 32));
}

Polyedre construit_corps_polyedre_triangle(const Corps &corps)
{
	auto polyedre = Polyedre();

	auto dico_aretes = dls::dico_desordonne<size_t, mi_arete *>();

	pour_chaque_polygone_ferme(corps, [&](Corps const &c, Polygone const *poly)
	{
		for (auto i = 2; i < poly->nombre_sommets(); ++i) {
			auto i0 = poly->index_point(0);
			auto i1 = poly->index_point(i - 1);
			auto i2 = poly->index_point(i);

			auto p0 = c.point_transforme(i0);
			auto p1 = c.point_transforme(i1);
			auto p2 = c.point_transforme(i2);

			auto s0 = polyedre.cree_sommet(p0);
			auto s1 = polyedre.cree_sommet(p1);
			auto s2 = polyedre.cree_sommet(p2);

			auto t = polyedre.cree_face();

			auto a0 = polyedre.cree_arete(s0, t);
			auto a1 = polyedre.cree_arete(s1, t);
			auto a2 = polyedre.cree_arete(s2, t);

			a0->suivante = a1;
			a1->suivante = a2;
			a2->suivante = a0;

			t->arete = a0;

			/* insere les mi_aretes dans le dictionnaire */
			auto idxi0i1 = index_arete(i0, i1);
			auto idxi1i2 = index_arete(i1, i2);
			auto idxi2i0 = index_arete(i2, i0);

			dico_aretes.insere({idxi0i1, a0});
			dico_aretes.insere({idxi1i2, a1});
			dico_aretes.insere({idxi2i0, a2});

			/* cherches les mi_aretes opposées */
			auto idxi1i0 = index_arete(i1, i0);
			auto idxi2i1 = index_arete(i2, i1);
			auto idxi0i2 = index_arete(i0, i2);

			auto iter = dico_aretes.trouve(idxi1i0);

			if (iter != dico_aretes.fin()) {
				auto a = iter->second;
				a0->paire = a;
				a->paire = a0;
			}

			iter = dico_aretes.trouve(idxi2i1);

			if (iter != dico_aretes.fin()) {
				auto a = iter->second;
				a1->paire = a;
				a->paire = a1;
			}

			iter = dico_aretes.trouve(idxi0i2);

			if (iter != dico_aretes.fin()) {
				auto a = iter->second;
				a2->paire = a;
				a->paire = a2;
			}
		}
	});

	return polyedre;
}

Polyedre converti_corps_polyedre(const Corps &corps)
{
	auto polyedre = Polyedre();

	auto dico_aretes = dls::dico_desordonne<size_t, mi_arete *>();
	auto dico_sommets = dls::dico_desordonne<long, mi_sommet *>();

	auto trouve_sommet = [&](long idx0)
	{
		auto iter0 = dico_sommets.trouve(idx0);

		if (iter0 == dico_sommets.fin()) {
			auto s0 = polyedre.cree_sommet(corps.point_transforme(idx0));
			dico_sommets.insere({ idx0, s0 });
			return s0;
		}

		return iter0->second;
	};

	pour_chaque_polygone_ferme(corps, [&](Corps const &corps_entree, Polygone const *poly)
	{
		INUTILISE(corps_entree);

		auto f = polyedre.cree_face();
		f->label = static_cast<unsigned>(poly->index);

		auto a = static_cast<mi_arete *>(nullptr);

		for (auto i = 0; i < poly->nombre_sommets(); ++i) {
			auto idx0 = poly->index_point(i);
			auto idx1 = poly->index_point((i + 1) % poly->nombre_sommets());

			auto s0 = trouve_sommet(idx0);
			s0->label = static_cast<unsigned>(idx0);
			auto s1 = trouve_sommet(idx1);
			s1->label = static_cast<unsigned>(idx1);

			auto a0 = polyedre.cree_arete(s0, f);

			if (f->arete == nullptr) {
				f->arete = a0;
			}

			if (a != nullptr) {
				a->suivante = a0;
			}

			a = a0;

			auto idxi0i1 = index_arete(idx0, idx1);

			dico_aretes.insere({idxi0i1, a0});

			/* cherches la mi_arete opposée */
			auto idxi1i0 = index_arete(idx1, idx0);

			auto iter = dico_aretes.trouve(idxi1i0);

			if (iter != dico_aretes.fin()) {
				auto a1 = iter->second;
				a0->paire = a1;
				a1->paire = a0;
			}
		}

		/* clos la boucle */
		a->suivante = f->arete;
	});

	return polyedre;
}

void converti_polyedre_corps(const Polyedre &polyedre, Corps &corps)
{
	for (auto triangle : polyedre.faces) {
		auto arete = triangle->arete;

		auto poly = Polygone::construit(&corps, type_polygone::FERME, 3);

		do {
			auto idx = corps.ajoute_point(arete->sommet->p);
			poly->ajoute_sommet(idx);

			arete = arete->suivante;
		} while (arete != triangle->arete);
	}
}

void initialise_donnees(Polyedre &polyedre)
{
	for (auto s : polyedre.sommets) {
		s->label = 0xFFFFFFFF;
	}

	for (auto f : polyedre.faces) {
		f->label = 0xFFFFFFFF;
	}
}

mi_arete *suivante_autour_point(mi_arete *arete)
{
	return arete->suivante->paire;
}
