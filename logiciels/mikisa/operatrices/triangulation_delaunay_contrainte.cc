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

#include "triangulation_delaunay_contrainte.hh"

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

/**
 * Tentative d'implémentation de
 * « Fully Dynamic Constrained Delaunay Triangulations »
 * http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.14.6477&rep=rep1&type=pdf
 */

using point = dls::math::point3f;
using vertex = dls::math::point3f;

struct arete {
	dls::tableau<int> crep;
	bool contraint;
	bool delaunay;
};

struct face {
	arete *a1;
	arete *a2;
	arete *a3;
};

struct triangulation_contrainte {
private:
	dls::tableau<point> m_points{};

	struct resultat_location {
		bool point_existe = false;
		bool point_sur_arete_existante = false;
		arete *a = nullptr;
		face *f = nullptr;
		point p{};
	};

	auto cherche_point(point const &p)
	{
		auto res = resultat_location{};

		for (auto const &pnt : m_points) {
			if (pnt == p) {

			}
		}

		return  res;
	}

public:
	/**
	 * Une contrainte peut avoir un ou plusieurs point. S'il y a plus qu'un seul
	 * point, la contrainte est considérer comme étant une polyligne (qui peut
	 * s'entresecter, être ouverte, ou fermée).
	 */
	void insere_contrainte(dls::tableau<point> const &contrainte, int idx)
	{
		auto liste_sommets = dls::tableau<vertex *>();

		for (auto const &p : contrainte) {
			auto rl = cherche_point(p);
			auto v = static_cast<vertex *>(nullptr);

			if (rl.point_existe) {
				v = &rl.p;
			}
			else if (rl.point_sur_arete_existante) {
				v = insere_point_sur_arete(rl.a, p);
			}
			else {
				v = insere_point_sur_face(rl.f, p);
			}

			liste_sommets.pousse(v);
		}

		for (auto i = 0; i < liste_sommets.taille() - 1; ++i) {
			insere_segment(liste_sommets[i], liste_sommets[i + 1], idx);
		}
	}

	vertex *insere_point_sur_arete(arete *a, point p)
	{
		if (non_exactement_sur_arete(a, p)) {
			p = projette_sur_arete(a, p);
		}

		auto orig_crep = a->crep;

		auto v = new vertex(p);

		auto sous_arete_a = new arete();
		auto sous_arete_b = new arete();
		sous_arete_a->crep = orig_crep;
		sous_arete_b->crep = orig_crep;

		auto aretes = dls::pile<arete *>();

		// trouve les 4 aretes correspondant au polygone où se trouve p

		flip_aretes(aretes, p);

		return v;
	}

	vertex *insere_point_sur_face(face *f, point p)
	{
		auto v = new vertex(p);

		auto aretes = dls::pile<arete *>();
		aretes.empile(f->a1);
		aretes.empile(f->a2);
		aretes.empile(f->a3);

		flip_aretes(aretes, p);

		return v;
	}

	void flip_aretes(dls::pile<arete *> &aretes, point p)
	{
		while (!aretes.est_vide()) {
			auto a = aretes.depile();

			if (!a->contraint && !a->delaunay) {
				// f = face incidente à a ne contenant pas p
				// pousse sur la pile les deux cotés n'étant pas e
				flip(a);
			}
		}
	}

	void insere_segment(vertex *v0, vertex *v1, int idx)
	{
		/* étape 1 */
		auto liste_arete = dls::tableau<arete *>();
		// liste_arete = trouve_aretes_contraintes_traversees(v0, v1);

		for (auto a : liste_arete) {
			auto p = point_entresection(a, v0, v1);
			insere_point_sur_arete(a, p);
		}

		/* étape 2 */
		liste_arete = dls::tableau<arete *>();
		// liste_arete = trouve_aretes_traversees(v0, v1);

		for (auto a : liste_arete) {
			enleve_du_resultat(a);
		}

		auto liste_vertex = dls::tableau<vertex *>();
		// liste_vertex = trouve_vertex_tranverses(v0, v1);

		for (auto i = 0; i < liste_vertex.taille() - 1; ++i) {
			auto v = liste_vertex[i];
			auto vs = liste_vertex[i + 1];

			if (sont_connectes(v, vs)) {
				auto a = connection(v, vs);
				a->crep.pousse(idx);
			}
			else {
				auto a = new arete();
				a->crep.pousse(idx);
				ajoute_au_resultat(a);

				retriangule_face_connectees(a);
			}
		}
	}

	void enleve_du_resultat(arete *a)
	{

	}

	void ajoute_au_resultat(arete *a)
	{

	}
};
