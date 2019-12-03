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

#include "operatrices_fracture.hh"

#include "biblexternes/voro/voro++.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/arbre_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/iteration_corps.hh"
#include "corps/limites_corps.hh"
#include "corps/polyedre.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

#define OPBOOLEEN

#ifdef OPBOOLEEN

/**
 * Tentative d'implémentation de
 * « Exact and Efficient Booleans for Polyhedra »
 * https://liris.cnrs.fr/Documents/Liris-4883.pdf
 * https://github.com/MEPP-team/MEPP/tree/f8a4cf58f5c83fdd6e9509461ce4b116c2dca6a1/src/components/Tools/Boolean_Operations/src
 *
 * Voir aussi :
 * https://github.com/openscad/openscad/wiki/Project:-Survey-of-CSG-algorithms
 */

struct mi_triangle_coupe {
	/*! \brief true if the facet belongs to the first polyhedron*/
	bool								Facet_from_A{};
	/*! \brief An exact vector giving the direction of the normal*/
	dls::math::vec3f norm_dir{};
	/*! \brief A list of segments (the intersections with the facets of the other polyhedron)*/
	dls::tableau<dls::tableau<unsigned> >	CutList{};
	/*! \brief A list of points (when the intersection is a point)*/
	dls::ensemble<unsigned>					PtList{};
	/*! \brief The list of the intersections*/
	dls::dico<unsigned, unsigned>		RefInter{};

	/*! \brief Default constructor*/
	mi_triangle_coupe() {}
	/*! \brief Constructor
				 \param V : The normal direction
				 \param ffA : Must be true if the facet belongs to the first polyhedron*/
	mi_triangle_coupe(dls::math::vec3f V, bool ffA)
	{
		norm_dir=V; Facet_from_A=ffA;
	} // MT
};

struct DonneesBooleen {
	dls::tableau<mi_face *> triangles{};
	dls::tableau<mi_triangle_coupe> triangles_esect{};
	dls::dico<unsigned int, dls::ensemble<unsigned int>> couples{};
};

struct delegue_polyedre_hbe {
	Polyedre const &polyedre;
	DonneesBooleen &donnees;
	unsigned j = 0;
	unsigned i = 0;
	bool est_A = false;

	delegue_polyedre_hbe(Polyedre const &c, DonneesBooleen &db)
		: polyedre(c)
		, donnees(db)
	{}

	int nombre_elements() const
	{
		return static_cast<int>(polyedre.faces.taille());
	}

	void coords_element(int idx, dls::tableau<dls::math::vec3f> &cos) const
	{
		auto tri = polyedre.faces[idx];

		cos.efface();
		cos.reserve(3);

		auto arete = tri->arete;
		cos.pousse(arete->sommet->p);

		arete = arete->suivante;
		cos.pousse(arete->sommet->p);

		arete = arete->suivante;
		cos.pousse(arete->sommet->p);
	}

	void element_chevauche(int idx, mi_face *triangle)
	{
		if (triangle->label == 0xFFFFFFFF) {
			this->donnees.triangles.pousse(triangle);

			triangle->label = j++;
			triangle->arete->label = i++;
			triangle->arete->suivante->label = i++;
			triangle->arete->suivante->suivante->label = i++;

			this->donnees.triangles_esect.pousse(
						mi_triangle_coupe(calcul_direction_normal(triangle->arete), !est_A));
		}

		auto prim = polyedre.faces[idx];

		if (prim->label == 0xFFFFFFFF) {
			this->donnees.triangles.pousse(prim);

			prim->label = j++;
			prim->arete->label = i++;
			prim->arete->suivante->label = i++;
			prim->arete->suivante->suivante->label = i++;

			this->donnees.triangles_esect.pousse(
						mi_triangle_coupe(calcul_direction_normal(prim->arete), est_A));
		}

		this->donnees.couples[prim->label].insere(triangle->label);
	}
};

static auto chevauche(float *bv, limites3f const &limites)
{
	auto limbv = limites3f();
	limbv.min = dls::math::vec3f(bv[0], bv[2], bv[4]);
	limbv.max = dls::math::vec3f(bv[1], bv[3], bv[5]);
	return limbv.chevauchent(limites);
}

template <typename TypeDelegue>
auto cherche_chevauchement(
		bli::BVHTree *arbre,
		TypeDelegue &delegue,
		mi_face *triangle,
		limites3f const &limites)
{
	auto const &racine = arbre->nodes[arbre->totleaf];

	dls::pile<bli::BVHNode *> noeuds;

	if (chevauche(racine->bv, limites)) {
		noeuds.empile(racine);
	}

	while (!noeuds.est_vide()) {
		auto noeud = noeuds.depile();

		if (noeud->totnode == 0) {
			delegue.element_chevauche(noeud->index, triangle);
		}
		else {
			for (auto i = 0; i < noeud->totnode; ++i) {
				auto enfant = noeud->children[i];

				if (chevauche(enfant->bv, limites)) {
					noeuds.empile(enfant);
				}
			}
		}
	}
}

template <class K>
class Triangulation
{
	/*!
	 * \typedef typename Point_3
	 * \brief 3d point using exact number type
	 */
	typedef typename K::Point_3														Point_3;

	/*!
	 * \typedef typename Tri_vb
	 * \brief Vertex base
	 */
		typedef /*typename*/ Enriched_vertex_base<K>										Tri_vb;

	/*!
	 * \typedef typename Tri_fb
	 * \brief Face base
	 */
		typedef /*typename*/ Enriched_face_base<K>											Tri_fb;

	/*!
	 * \typedef typename Tri_DS
	 * \brief Data structure of the triangulation
	 */
	typedef typename CGAL::Triangulation_data_structure_2<Tri_vb,Tri_fb>			Tri_DS;

	/*!
	 * \typedef typename Itag
	 * \brief No intersection tag
	 */
	typedef typename CGAL::No_intersection_tag										Itag;

	/*!
	 * \typedef typename Constrained_Delaunay_tri
	 * \brief 2d constrained Delaunay triangulation
	 */
	typedef typename CGAL::Constrained_Delaunay_triangulation_2<K, Tri_DS, Itag>	Constrained_Delaunay_tri;

	/*!
	 * \typedef typename Vertex_handle_tri
	 * \brief Vertex handle for the triangulation
	 */
	typedef typename Constrained_Delaunay_tri::Vertex_handle						Vertex_handle_tri;

	/*!
	 * \typedef typename Face_handle_tri
	 * \brief Face handle for the triangulation
	 */
	typedef typename Constrained_Delaunay_tri::Face_handle							Face_handle_tri;

	/*!
	 * \typedef typename Point_tri
	 * \brief 2d point for the triangulation
	 */
	typedef typename Constrained_Delaunay_tri::Point								Point_tri;

	/*!
	 * \typedef typename Face_iterator_tri
	 * \brief Iterator for the faces of the triangulation
	 */
	typedef typename Constrained_Delaunay_tri::Face_iterator						Face_iterator_tri;

public:
	/*!
	 * \brief Constructor
	 * \param he : A halfedge incident to the facet
	 * \param norm_dir : The vector directing the normal of the facet
	 */
	Triangulation(mi_arete *he, dls::math::vec3f &norm_dir)
	{
		//find the longest coordinate of the normal vector, and its sign
		double x = static_cast<double>(norm_dir.x);
		double y = static_cast<double>(norm_dir.y);
		double z = static_cast<double>(norm_dir.z);
		double absx = std::abs(x);
		double absy = std::abs(y);
		double absz = std::abs(z);

		//this information is stored using a code :
		//0 : The coordinate X is the longest, and positive
		//1 : The coordinate Y is the longest, and positive
		//2 : The coordinate Z is the longest, and positive
		//3 : The coordinate X is the longest, and negative
		//4 : The coordinate Y is the longest, and negative
		//5 : The coordinate Z is the longest, and negative
		if (absx >= absy && absx >= absz) max_coordinate = (x>0)?0:3;
		else if (absy >= absx && absy >= absz) max_coordinate = (y>0)?1:4;
		else if (absz >= absx && absz >= absy) max_coordinate = (z>0)?2:5;

		//we add the three vertices of the facet to the triangulation
		//The label of these vertices is set for the corresponding point in the triangulation
		v1 = add_new_pt(he->sommet->p, he->sommet->label);
		v2 = add_new_pt(he->suivante->sommet->p, he->suivante->sommet->label);
		v3 = add_new_pt(he->suivante->suivante->sommet->p, he->suivante->suivante->sommet->label);

		//if the vertices does not have an Id (label = OxFFFFFFFF), the labels
		//of the points in the triangulation is set as follows :
		//0xFFFFFFFF for the first point
		//0xFFFFFFFE for the second point
		//0xFFFFFFFD for the third point
		if(v2->get_label() == 0xFFFFFFFF) v2->set_label(0xFFFFFFFE);
		if(v3->get_label() == 0xFFFFFFFF) v3->set_label(0xFFFFFFFD);
	}

	/*!
	 * \brief Compute the orthogonal projection of a point to the plane defined by the longest coordinate of the normal vector
	 * \param p : the point (in 3d)
	 * \return The projection as a 2d point
	 */
	Point_tri get_minvar_point_2(Point_3 &p)
	{
		switch(max_coordinate)
		{
		case 0:
			return Point_tri(p.y(),p.z());
			break;
		case 1:
			return Point_tri(p.z(),p.x());
			break;
		case 2:
			return Point_tri(p.x(),p.y());
			break;
		case 3:
			return Point_tri(p.z(),p.y());
			break;
		case 4:
			return Point_tri(p.x(),p.z());
			break;
		case 5:
			return Point_tri(p.y(),p.x());
			break;
		default:
			return Point_tri(p.y(),p.z());
		}
	}

	/*!
	 * \brief Adds a point in the triangulation
	 * \param p : The point (in 3d : the projection in 2d is done automatically)
	 * \param label : The label of the point
	 * \return The Vertex_handle of the point added
	 */
		Vertex_handle_tri add_new_pt(dls::math::vec3f p, unsigned long &label)   // MT: suppression référence
	{
		//if the point is not a new one, we verify that the point has not already been added
		if(label != 0xFFFFFFFF)
			for(unsigned int i = 0;i != pts_point.size();++i)
				if(label == pts_point[i])
					//if the point is already in the triangulation, we return its handle
					return pts_vertex[i];
		Vertex_handle_tri v;
		v = ct.insert(get_minvar_point_2(p));
		v->set_label(label);
		pts_point.pousse(label);
		pts_vertex.push_back(v);
		return v;
	}

	/*!
	 * \brief Adds a constrained segment in the triangulation
	 * \param p1 : The first point (in 3d)
	 * \param p2 : The second point (in 3d)
	 * \param label1 : The label of the first point
	 * \param label2 : The label of the second point
	 */
	void add_segment(Point_3 &p1, Point_3 &p2, unsigned long &label1, unsigned long &label2)
	{
		// we add the two points in the triangulation and store their handles in c1 and c2
		c1 = add_new_pt(p1, label1);
		c2 = add_new_pt(p2, label2);
		// we set a constrained segment between these two points
		ct.insert_constraint(c1, c2);
		// if an other segment is added, we will overwrite c1 and c2
		// what is important is to memorize the handles of the last segment added
	}

	/*!
	 * \brief Gets the triangles of the triangulation that belongs to the result
	 * and deduce for the three neighboring facets if they belong to the result or not
	 * \param inv_triangles : must be true if the orientation of the triangles must be inverted
	 * \param IsExt : Pointer on a three-case boolean table.
	 * \return The list of the triangles belonging to the result.
	 * each triangle is defined by a list of three labels
	 */
	dls::tableau<dls::tableau<unsigned long> > get_triangles(bool inv_triangles, bool *IsExt)
	{
		//init
		IsExt[0] = false;
		IsExt[1] = false;
		IsExt[2] = false;
				dls::tableau<dls::tableau<unsigned long> > tris;
		for(Face_iterator_tri fi = ct.faces_begin();fi != ct.faces_end();fi++)
			fi->set_OK(false);

		//the constrained segments are oriented, we search the triangle (c1, c2, X), (X, c1, c2) or (c2, X, c1)
		//where c1 and c2 are the two points related to the last constrained segment added, and X another point
		//this triangle belongs to the result (thanks to the orientation of the segments)
		Face_handle_tri f, f2 = c1->face();
				int i=0; // MT
		do {
			f = f2;
			f->has_vertex(c1,i);
			f2 = f->neighbor(f->ccw(i));
		} while( ! ( f->has_vertex(c2) && f2->has_vertex(c2) ) );

		//dans le cas particulier ou la frontiere se trouve exactement sur un bord de la triangulation,
		//et que ce triangle n'appartient pas a la triangulation, on d�marrera avec l'autre triangle
		//incluant le segment c1, c2 (et donc, n'appartenant pas au r�sultat

		//if the segment is exactly on the border of the triangulation, the triangle could be outside the triangulation
		//in that case, we will search the other triangle including the points c1 and c2
		//this triangle does not belong to the result
		if(f->has_vertex(ct.infinite_sommet))
		{
			f = f2;
			f->set_Ext(false);
		}
		else
		{
			f->set_Ext(true);
		}

		dls::pile<Face_handle_tri> sfh;
		f->set_OK(true);
		sfh.push(f);

		//we decide for all the triangles, if they belongs to the result, starting from the first triangle f,
		//by moving on the triangulation using the connectivity between the triangles.
		//If a constrained segment is crossed, the value of the tag "isext" is inverted
		while(!sfh.empty())
		{
			f = sfh.top();
			sfh.pop();

			if(f->get_Ext())
			{
				dls::tableau<unsigned long> tri;
				int i;
				tri.pousse(f->vertex(0)->get_label());

				//verify if the neighboring facets belongs to the result or not
				if(f->has_vertex(v1,i) && f->neighbor(f->ccw(i))->has_vertex(ct.infinite_sommet)) IsExt[0] = true;
				if(f->has_vertex(v2,i) && f->neighbor(f->ccw(i))->has_vertex(ct.infinite_sommet)) IsExt[1] = true;
				if(f->has_vertex(v3,i) && f->neighbor(f->ccw(i))->has_vertex(ct.infinite_sommet)) IsExt[2] = true;

				if(inv_triangles)
				{
					tri.pousse(f->vertex(2)->get_label());
					tri.pousse(f->vertex(1)->get_label());
				}
				else
				{
					tri.pousse(f->vertex(1)->get_label());
					tri.pousse(f->vertex(2)->get_label());
				}
				tris.pousse(tri);
			}
			for(i = 0;i!=3;i++)
			{
				if(!(f->neighbor(i)->get_OK() || f->neighbor(i)->has_vertex(ct.infinite_sommet)))
				{
					f->neighbor(i)->set_OK(true);
					f->neighbor(i)->set_Ext((f->is_constrained(i))?!f->get_Ext():f->get_Ext());
					sfh.push(f->neighbor(i));
				}
			}
		}
		return tris;
	}

	/*!
	 * \brief Gets all the triangles of the triangulation
	 * \param inv_triangles : must be true if the orientation of the triangles must be inverted
	 * \return The list of the triangles belonging to the result.
	 * each triangle is defined by a list of three labels
	 */
		dls::tableau<dls::tableau<unsigned long> > get_all_triangles(bool inv_triangles)
	{
				dls::tableau<dls::tableau<unsigned long> > tris;
		for(Face_iterator_tri f = ct.faces_begin();f != ct.faces_end();f++)
		{
			dls::tableau<unsigned long> tri;
			tri.pousse(f->vertex(0)->get_label());
			if(inv_triangles)
			{
				tri.pousse(f->vertex(2)->get_label());
				tri.pousse(f->vertex(1)->get_label());
			}
			else
			{
				tri.pousse(f->vertex(1)->get_label());
				tri.pousse(f->vertex(2)->get_label());
			}
			tris.pousse(tri);
		}
		return tris;
	}

private:
	/*! \brief The triangulation*/
	Constrained_Delaunay_tri ct;
	/*! \brief List of the id of the points added in the triangulation*/
	dls::tableau<unsigned int> pts_point;
	/*! \brief List of the handles of the points added in the triangulation*/
	dls::tableau<Vertex_handle_tri> pts_vertex;
	/*! \brief Handle of the point corresponding to the first vertex of the facet*/
	Vertex_handle_tri v1;
	/*! \brief Handle of the point corresponding to the second vertex of the facet*/
	Vertex_handle_tri v2;
	/*! \brief Handle of the point corresponding to the third vertex of the facet*/
	Vertex_handle_tri v3;
	/*! \brief Handle of the point corresponding to the first vertex of the last segment added*/
	Vertex_handle_tri c1;
	/*! \brief Handle of the point corresponding to the second vertex of the last segment added*/
	Vertex_handle_tri c2;
	/*! \brief Code identifying the plane where the triangulation is done \n
	 * 0 : Plane (y, z) \n
	 * 1 : Plane (z, x) \n
	 * 2 : Plane (x, y) \n
	 * 3 : Plane (z, y) \n
	 * 4 : Plane (x, z) \n
	 * 5 : Plane (y, x)
	 */
	int max_coordinate;
};

struct Info_Inter {
	/*! \brief The facet*/
	mi_face		*f;
	/*! \brief The halfedge*/
	mi_arete	*he;
	/*! \brief true if the intersection is exactly on the vertex pointed by he*/
	bool				IsOnVertex;
	/*! \brief A code for the location of the intersection :\n\n
		 * 0 : Intersection is strictly in the facet\n
		 * 1 : Intersection is on the first edge of the facet\n
		 * 2 : Intersection is on the second edge of the facet\n
		 * 3 : Intersection is exactly on the first vertex of the facet\n
		 * 4 : Intersection is on the third edge of the facet\n
		 * 5 : Intersection is exactly on the third vertex of the facet\n
		 * 6 : Intersection is exactly on the second vertex of the facet\n
		 * 7 : There is no intersection */
	unsigned short		res;
	/*! \brief The intersection point (exact)*/
	dls::math::vec3f pt;
	/*! \brief The Id of the intersection point*/
	unsigned Id;
};

class OpBooleensMaillage final : public OperatriceCorps {
	enum {
		UNION,
		INTER,
		MINUS,
	};

	int m_BOOP = UNION;

	dls::tableau<dls::math::vec3f> InterPts{};

public:
	static constexpr auto NOM = "Booléens maillages";
	static constexpr auto AIDE = "";

	OpBooleensMaillage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps_a = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_a, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto corps_b = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_b, true, true, 1)) {
			return res_exec::ECHOUEE;
		}

		/* converti les deux maillages en polyèdres triangulés */

		auto poly_a = construit_corps_polyedre_triangle(*corps_a);

		if (!valide_polyedre(poly_a)) {
			this->ajoute_avertissement("Le polyèdre A n'est pas valide");
			return res_exec::ECHOUEE;
		}

		auto poly_b = construit_corps_polyedre_triangle(*corps_b);

		if (!valide_polyedre(poly_b)) {
			this->ajoute_avertissement("Le polyèdre B n'est pas valide");
			return res_exec::ECHOUEE;
		}

		/* construit un arbre HBE depuis le polyèdre avec le moins de triangles */
		// il va falloir trouver tous les noeuds qui intersectent avec celui du
		// triangle t du deuxième polyèdre

		initialise_donnees(poly_a);
		initialise_donnees(poly_b);

		auto donnees_booleen = DonneesBooleen{};

		if (poly_a.faces.taille() < poly_b.faces.taille()) {
			auto delegue = delegue_polyedre_hbe(poly_a, donnees_booleen);
			delegue.est_A = true;

			auto arbre = bli::cree_arbre_bvh(delegue);

			for (auto tri : poly_b.faces) {
				auto limites_tri = limites3f();

				auto arete = tri->arete;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				arete = arete->suivante;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				arete = arete->suivante;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				cherche_chevauchement(arbre, delegue, tri, limites_tri);
			}

			memoire::deloge("BVHTree", arbre);
		}
		else {
			auto delegue = delegue_polyedre_hbe(poly_b, donnees_booleen);
			delegue.est_A = false;

			auto arbre = bli::cree_arbre_bvh(delegue);

			for (auto tri : poly_a.faces) {
				auto limites_tri = limites3f();

				auto arete = tri->arete;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				arete = arete->suivante;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				arete = arete->suivante;
				dls::math::extrait_min_max(arete->sommet->p, limites_tri.min, limites_tri.max);

				cherche_chevauchement(arbre, delegue, tri, limites_tri);
			}

			memoire::deloge("BVHTree", arbre);
		}

		std::cerr << "Il y a " << donnees_booleen.couples.taille() << " couples.\n";
		std::cerr << "Il y a " << donnees_booleen.triangles.taille() << " triangles.\n";
		std::cerr << "Il y a " << donnees_booleen.triangles_esect.taille() << " triangles_esect.\n";

		if (donnees_booleen.couples.taille() == 0) {
			converti_polyedre_corps(poly_a, m_corps);
			return res_exec::REUSSIE;
		}

		calcul_intersections(donnees_booleen);

		converti_polyedre_corps(poly_a, m_corps);

		return res_exec::REUSSIE;
	}

	void calcul_intersections(DonneesBooleen &donnees_booleens)
	{
		while (!donnees_booleens.couples.est_vide()) {
			auto fA = donnees_booleens.couples.debut()->first;
			auto fB = *donnees_booleens.couples[fA].debut();

			intersecte_tri_tri(donnees_booleens, fA, fB);

			rmCouple(donnees_booleens, fA, fB);
		}
	}

	void intersecte_tri_tri(DonneesBooleen &donnees_booleens, unsigned &A, unsigned &B)
	{
		auto nA = donnees_booleens.triangles_esect[A].norm_dir;
		auto nB = donnees_booleens.triangles_esect[B].norm_dir;

		mi_face *fA2, *fB2;
		auto fA = donnees_booleens.triangles[A];
		auto fB = donnees_booleens.triangles[B];

		mi_arete *heA[3], *heB[3];
		heA[0] = fA->arete;
		heA[1] = heA[0]->suivante;
		heA[2] = heA[1]->suivante;
		heB[0] = fB->arete;
		heB[1] = heB[0]->suivante;
		heB[2] = heB[1]->suivante;

		dls::math::vec3f ptA[3], ptB[3];
		ptA[0] = heA[0]->sommet->p;
		ptA[1] = heA[1]->sommet->p;
		ptA[2] = heA[2]->sommet->p;
		ptB[0] = heB[0]->sommet->p;
		ptB[1] = heB[1]->sommet->p;
		ptB[2] = heB[2]->sommet->p;

		//compute the position of the three vertices of each triangle regarding the plane of the other
		//positive if the vertex is above
		//negative if the vertex is under
		//zero if the vertex is exactly on the triangle
		float posA[3], posB[3];
		posA[0] = produit_scalaire(nB, (ptA[0] - ptB[0]));
		posA[1] = produit_scalaire(nB, (ptA[1] - ptB[0]));
		posA[2] = produit_scalaire(nB, (ptA[2] - ptB[0]));
		posB[0] = produit_scalaire(nA, (ptB[0] - ptA[0]));
		posB[1] = produit_scalaire(nA, (ptB[1] - ptA[0]));
		posB[2] = produit_scalaire(nA, (ptB[2] - ptA[0]));

		//a code is computed on 6 bits using these results (two bits for each point)
		//10 -> above ; 01 -> under ; 00 -> on the plane
		unsigned short posAbin, posBbin;
		posAbin =	  ( (posA[0] > 0)? 32 : 0 )
				+ ( (posA[0] < 0)? 16 : 0 )
				+ ( (posA[1] > 0)? 8 : 0 )
				+ ( (posA[1] < 0)? 4 : 0 )
				+ ( (posA[2] > 0)? 2 : 0 )
				+ ( (posA[2] < 0)? 1 : 0 );

		posBbin =	  ( (posB[0] > 0)? 32 : 0 )
				+ ( (posB[0] < 0)? 16 : 0 )
				+ ( (posB[1] > 0)? 8 : 0 )
				+ ( (posB[1] < 0)? 4 : 0 )
				+ ( (posB[2] > 0)? 2 : 0 )
				+ ( (posB[2] < 0)? 1 : 0 );

		//if the intersection is not a segment, the intersection is not computed
		//the triangles intersects on a point (one vertex on the plane and the two others under or above
		if (posAbin == 5 || posAbin == 10 || posAbin == 17 || posAbin == 34 || posAbin == 20 || posAbin == 40
				|| posBbin == 5 || posBbin == 10 || posBbin == 17 || posBbin == 34 || posBbin == 20 || posBbin == 40) return;
		//no possible intersection (one of the triangle is completely under or above the other
		if (posAbin == 42 || posAbin == 21
				|| posBbin == 42 || posBbin == 21) return;
		//the triangles are coplanar
		if (posAbin == 0) return;

		//if an edge of a triangle is on the plane of the other triangle, it is necessary to verify if the
		//two polyhedra are intersecting on these edges, or if it only is a contact to know if the intersection
		//between the triangles must be computed or not.
		//"edgeA" and "edgeB" are codes
		//0 : the first edge is on the plane
		//1 : the second edge is on the plane
		//2 : the third edge is on the plane
		//3 : there is no edge on the plane
		unsigned short edgeA = 3, edgeB = 3;
		if (     posAbin == 1  || posAbin == 2 ) edgeA = 1; //points 0 and 1 on the plane
		else if (posAbin == 16 || posAbin == 32) edgeA = 2; //points 1 and 2 on the plane
		else if (posAbin == 4  || posAbin == 8 ) edgeA = 0; //points 2 and 0 on the plane
		if (     posBbin == 1  || posBbin == 2 ) edgeB = 1; //points 0 and 1 on the plane
		else if (posBbin == 16 || posBbin == 32) edgeB = 2; //points 1 and 2 on the plane
		else if (posBbin == 4  || posBbin == 8 ) edgeB = 0; //points 2 and 0 on the plane

		dls::math::vec3f nA2, nB2;
		float p;
		bool invert_direction = false;
		bool stop = false;

		//if an edge of the first triangle is on the plane
		if (edgeA != 3 && edgeB == 3)
		{
			fA2 = heA[edgeA]->paire->face;
			nA2 = donnees_booleens.triangles_esect[fA2->label].norm_dir;
			p = produit_scalaire(produit_croix(nA, nB), produit_croix(nA2, nB));
			//if p is negative, the two triangles of the first polyhedron (including edgeA) are on the same side
			//so there is no intersection
			if (p < 0) stop = true;
			//if p == 0, fA2 is coplanar with the plane of fB
			//in that case, it is necessary to consider the boolean
			//operator used to determine if there is a contact or not
			else if (p == 0.0f)
			{
				switch(m_BOOP)
				{
					case UNION:
						if (posA[(edgeA+1)%3] * produit_scalaire(nA2, nB) > 0)
							stop = true;
						break;
					case INTER:
						if (posA[(edgeA+1)%3] > 0)
							stop = true;
						break;
					case MINUS:
						if (posA[(edgeA+1)%3] * produit_scalaire(nA2, nB) < 0)
							stop = true;
						break;
				}
			}
			//the intersection between fA2 and fB is the same so this couple is removed from the list
			rmCouple(donnees_booleens, fA2->label, fB->label);
		}
		//if an edge of the second triangle is on the plane
		else if (edgeA == 3 && edgeB != 3)
		{
			fB2 = heB[edgeB]->paire->face;
			nB2 = donnees_booleens.triangles_esect[fB2->label].norm_dir;
			p = produit_scalaire(produit_croix(nA, nB), produit_croix(nA, nB2));
			//if p is negative, the two triangles of the second polyhedron (including edgeB) are on the same side
			//so there is no intersection
			if (p < 0) stop = true;
			//if p == 0, fB2 is coplanar with the plane of fA
			//in that case, it is necessary to consider the boolean
			//operator used to determine if there is a contact or not
			else if (p == 0.0f)
			{
				switch(m_BOOP)
				{
					case UNION:
						if (posB[(edgeB+1)%3] < 0)
							stop = true;
						break;
					case INTER:
						if (posB[(edgeB+1)%3] * produit_scalaire(nB2, nA) < 0)
							stop = true;
						break;
					case MINUS:
						if (posB[(edgeB+1)%3] > 0)
							stop = true;
						break;
				}
			}
			//the intersection between fA and fB2 is the same so this couple is removed from the list
			rmCouple(donnees_booleens, fA->label, fB2->label);
		}
		//if an edge of each triangle is on the plane of the other
		else if (edgeA != 3 && edgeB != 3)
		{
			//in this case, four triangles are concerned by the intersection
			//fA2 and fB2 are the two other concerned facets
			//we try to determine if fA and fA2 are inside or outside the second polyhedron, using fB and fB2
			bool Intersection = false;
			dls::math::vec3f nAcnB2, nA2cnB;
			float nAnB2, nA2nB, nA2nB2;
			float posA2_A, posB_A, posB2_A, posB_B2, posA_B, posB2_B, posB_A2, posB2_A2, posA2_B, posA2_B2;
			dls::math::vec3f ptA2, ptB2;

			fA2 = heA[edgeA]->paire->face;
			fB2 = heB[edgeB]->paire->face;
			nA2 = donnees_booleens.triangles_esect[fA2->label].norm_dir;
			nB2 = donnees_booleens.triangles_esect[fB2->label].norm_dir;

			nAcnB2 = produit_croix(nA, nB2);
			nA2cnB = produit_croix(nA2, nB);

			nAnB2 = produit_scalaire(nA, nB2);
			nA2nB = produit_scalaire(nA2, nB);
			nA2nB2 = produit_scalaire(nA2, nB2);

			ptA2 = heA[edgeA]->paire->suivante->sommet->p;
			ptB2 = heB[edgeB]->paire->suivante->sommet->p;

			posA_B = posA[(edgeA+1)%3];
			posB_A = posB[(edgeB+1)%3];
			posB_A2 = produit_scalaire(nA2, (ptB[(edgeB+1)%3] - ptA[edgeA]));
			posB_B2 = produit_scalaire(nB2, (ptB[(edgeB+1)%3] - ptA[edgeA]));
			posA2_A = produit_scalaire(nA, (ptA2 - ptA[edgeA]));
			posA2_B = produit_scalaire(nB, (ptA2 - ptA[edgeA]));
			posA2_B2 = produit_scalaire(nB2, (ptA2 - ptA[edgeA]));
			posB2_A = produit_scalaire(nA, (ptB2 - ptA[edgeA]));
			posB2_A2 = produit_scalaire(nA2, (ptB2 - ptA[edgeA]));
			posB2_B = produit_scalaire(nB, (ptB2 - ptA[edgeA]));

			if (nAcnB2 == dls::math::vec3f(0.0f) && nA2cnB == dls::math::vec3f(0.0f) && nAnB2 * nA2nB > 0)
				stop = true;

			//firstly, we search the position of fA
			//if fA is inside the poyhedron, Intersection = true
			if (posB_A * posB2_A > 0) //fB and fB2 on the same side
			{
				if (posB_B2 > 0) Intersection = true;
			}
			else if (posB_A * posB2_A < 0) //fB and fB2 on opposite side
			{
				if (posA_B < 0) Intersection = true;
			}
			else  //fA and fB2 coplanar
			{
				if (posA_B * posB2_B < 0)
				{
					if (posB_B2 > 0) Intersection = true;
				}
				else
				{
					if (nAnB2 < 0)
					{
						if (m_BOOP == UNION) Intersection = true;
					}
					else
					{
						if (m_BOOP == MINUS) Intersection = true;
					}
				}
			}

			//secondly, we search the position of fA2
			//if fA2 is inside the poyhedron, "Intersection" is inverted
			if (posB_A2 * posB2_A2 > 0) //fB and fB2 on the same side
			{
				if (posB_B2 > 0) Intersection = !Intersection;
			}
			else if (posB_A2 * posB2_A2 < 0) //fB and fB2 on opposite side
			{
				if (posA2_B < 0) Intersection = !Intersection;
			}
			else if (posB2_A2 == 0.0f) //fA2 and fB2 coplanar
			{
				if (posA2_B * posB2_B < 0)
				{
					if (posB_B2 > 0) Intersection = !Intersection;
				}
				else
				{
					if (nA2nB2 < 0)
					{
						if (m_BOOP == UNION) Intersection = !Intersection;
					}
					else
					{
						if (m_BOOP == MINUS) Intersection = !Intersection;
					}
				}
			}
			else //fA2 and fB coplanar
			{
				if (posA2_B2 * posB_B2 < 0) {
					if (posB_B2 > 0) Intersection = !Intersection;
				}
				else {
					if (nA2nB < 0) {
						if (m_BOOP == UNION) Intersection = !Intersection;
					}
					else {
						if (m_BOOP == MINUS) Intersection = !Intersection;
					}
				}
			}

			//if Intersection == false, fA and fA2 are both inside or outside the second polyhedron.
			if (!Intersection) {
				stop = true;
			}

			//the intersection between (fA, fB2), (fA2, fB) and (fA2, fB2) are the same so these couples are removed from the list
			rmCouple(donnees_booleens, fA->label, fB2->label);
			rmCouple(donnees_booleens, fA2->label, fB->label);
			rmCouple(donnees_booleens, fA2->label, fB2->label);

			//it is possible that the direction of the intersection have to be inverted
			if (posB_A * posA2_A > 0 && posB_A * posB2_A >= 0 && posB2_B * posA_B > 0)
				invert_direction = true;
		}

		//if the intersection must not be compute
		if (stop) {
			return;
		}

		Info_Inter inter[4];
		inter[0].f = fA;
		inter[1].f = fA;
		inter[2].f = fB;
		inter[3].f = fB;

		//the two intersection points between the edges of a triangle and the
		//other triangle are computed for the two triangles
		switch(posBbin)
		{
			//common intersections : one point one one side of the plane and the two other points on the other side
			case 26:
			case 37:
				inter[0].he = heB[0];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[1];
				InterTriangleSegment(donnees_booleens, &inter[1]);
				break;
			case 25:
			case 38:
				inter[0].he = heB[1];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[2];
				InterTriangleSegment(donnees_booleens, &inter[1]);
				break;
			case 22:
			case 41:
				inter[0].he = heB[2];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[0];
				InterTriangleSegment(donnees_booleens, &inter[1]);
				break;
				//particular cases : one point on the plane, one point one one side and one point on the other side
			case 6:
			case 9:
				inter[0].he = heB[2];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[0];
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
			case 18:
			case 33:
				inter[0].he = heB[0];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[1];
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
			case 24:
			case 36:
				inter[0].he = heB[1];
				InterTriangleSegment(donnees_booleens, &inter[0]);
				inter[1].he = heB[2];
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
				//particular case : two points on the plane
			case 1:
			case 2:
				inter[0].he = heB[0];
				IsInTriangle(donnees_booleens, &inter[0]);
				inter[1].he = heB[2]->paire;
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
			case 16:
			case 32:
				inter[0].he = heB[1];
				IsInTriangle(donnees_booleens, &inter[0]);
				inter[1].he = heB[0]->paire;
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
			case 4:
			case 8:
				inter[0].he = heB[2];
				IsInTriangle(donnees_booleens, &inter[0]);
				inter[1].he = heB[1]->paire;
				IsInTriangle(donnees_booleens, &inter[1]);
				break;
			default:
				return;
		}

		switch(posAbin)
		{
			//common intersections : one point one one side of the plane and the two other points on the other side
			case 26:
			case 37:
				inter[2].he = heA[0];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[1];
				InterTriangleSegment(donnees_booleens, &inter[3]);
				break;
			case 25:
			case 38:
				inter[2].he = heA[1];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[2];
				InterTriangleSegment(donnees_booleens, &inter[3]);
				break;
			case 22:
			case 41:
				inter[2].he = heA[2];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[0];
				InterTriangleSegment(donnees_booleens, &inter[3]);
				break;
				//particular cases : one point on the plane, one point one one side and one point on the other side
			case 6:
			case 9:
				inter[2].he = heA[2];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[0];
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
			case 18:
			case 33:
				inter[2].he = heA[0];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[1];
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
			case 24:
			case 36:
				inter[2].he = heA[1];
				InterTriangleSegment(donnees_booleens, &inter[2]);
				inter[3].he = heA[2];
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
				//particular case : two points on the plane
			case 1:
			case 2:
				inter[2].he = heA[0];
				IsInTriangle(donnees_booleens, &inter[2]);
				inter[3].he = heA[2]->paire;
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
			case 16:
			case 32:
				inter[2].he = heA[1];
				IsInTriangle(donnees_booleens, &inter[2]);
				inter[3].he = heA[0]->paire;
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
			case 4:
			case 8:
				inter[2].he = heA[2];
				IsInTriangle(donnees_booleens, &inter[2]);
				inter[3].he = heA[1]->paire;
				IsInTriangle(donnees_booleens, &inter[3]);
				break;
			default:
				return;
		}

		//if two distincts points belongs to the two triangles
		if (IsSegment(inter))
		{
			//we get this segment in ptInter
			dls::tableau<unsigned> ptInter;
			Get_Segment(donnees_booleens, inter, ptInter);
			//and we build the opposite segment in ptInterInv
			dls::tableau<unsigned> ptInterInv;
			ptInterInv.pousse(ptInter[1]);
			ptInterInv.pousse(ptInter[0]);

			//the segments are stored in the concerned triangles, and oriented
			if (produit_scalaire(produit_croix(nA, nB), (InterPts[ptInter[1]] - InterPts[ptInter[0]])) * ((invert_direction == true)?-1:1) > 0)
			{
				switch(m_BOOP)
				{
					case UNION:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInter);
						if (edgeA != 3)
							donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInter);

						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInterInv);

						if (edgeB != 3)
							donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInterInv);
						break;
					case INTER:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInterInv);
						if (edgeA != 3) donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInterInv);
						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInter);
						if (edgeB != 3) donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInter);
						break;
					case MINUS:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInter);
						if (edgeA != 3) donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInter);
						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInter);
						if (edgeB != 3) donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInter);
						break;
				}
			}
			else
			{
				switch(m_BOOP)
				{
					case UNION:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInterInv);
						if (edgeA != 3) donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInterInv);
						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInter);
						if (edgeB != 3) donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInter);
						break;
					case INTER:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInter);
						if (edgeA != 3) donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInter);
						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInterInv);
						if (edgeB != 3) donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInterInv);
						break;
					case MINUS:
						donnees_booleens.triangles_esect[fA->label].CutList.pousse(ptInterInv);
						if (edgeA != 3) donnees_booleens.triangles_esect[fA2->label].CutList.pousse(ptInterInv);
						donnees_booleens.triangles_esect[fB->label].CutList.pousse(ptInterInv);
						if (edgeB != 3) donnees_booleens.triangles_esect[fB2->label].CutList.pousse(ptInterInv);
						break;
				}
			}
		}
	}

	void rmCouple(DonneesBooleen &donnees_booleens, unsigned &A, unsigned &B)
	{
		if (donnees_booleens.couples[A].compte(B) != 0) {
			donnees_booleens.couples[A].efface(B);
		}

		if (donnees_booleens.couples[A].taille() == 0) {
			donnees_booleens.couples.efface(A);
		}
	}

	void InterTriangleSegment(DonneesBooleen &donnees_booleens, Info_Inter* inter)
	{
		mi_face *f = inter->f;
		mi_arete *he = inter->he;
		//if the intersection has been computed, the function returns directly the Id of the intersection
		if (donnees_booleens.triangles_esect[f->label].RefInter.compte(he->label) != 0)
		{
			inter->Id = donnees_booleens.triangles_esect[f->label].RefInter[he->label];
			return;
		}
		//else, the calculation is done

		//this method is called when the intersection is not on the vertex pointed by the halfedge
		inter->IsOnVertex = false;
		//the intersection does not have an Id. 0xFFFFFFFF is set (this value means "no Id")
		inter->Id = 0xFFFFFFFF;

		dls::math::vec3f e1, e2, dir, p, s, q;
		float u, v, tmp;

		dls::math::vec3f s1 = he->paire->sommet->p;
		dls::math::vec3f s2 = he->sommet->p;
		dls::math::vec3f v0 = f->arete->sommet->p;
		dls::math::vec3f v1 = f->arete->suivante->sommet->p;
		dls::math::vec3f v2 = f->arete->suivante->suivante->sommet->p;

		//computation of the intersection (exact numbers)
		e1 = v1 - v0;
		e2 = v2 - v0;
		dir = s2 - s1;
		p = produit_croix(dir, e2);
		tmp = 1.0f / produit_scalaire(p, e1);
		s = s1 - v0;
		u = tmp * produit_scalaire(s, p);
		if (u < 0 || u > 1)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}
		q = produit_croix(s, e1);
		v = tmp * produit_scalaire(dir, q);
		if (v < 0 || v > 1)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}
		if (u + v > 1)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}

		//the result is stored in inter->pt
		inter->pt = s1+(tmp*e2*q)*dir;

		//creation of the code for the location of the intersection
		inter->res = 0;
		if (u == 0.0f) inter->res += 1;	//intersection on he(0)
		if (v == 0.0f) inter->res += 2;	//intersection on he(1)
		if (u+v == 1.0f) inter->res += 4;	//intersection on he(2)
	}


	/*! \brief Finds the position of a point in a 3d triangle
		 * \param inter : A pointer to an Info_Inter structure*/
	void IsInTriangle(DonneesBooleen &donnees_booleens, Info_Inter* inter)
	{
		mi_face *f = inter->f;
		mi_arete *he = inter->he;
		//if the intersection has been computed, the function returns directly the Id of the intersection
		if (donnees_booleens.triangles_esect[f->label].RefInter.compte(he->label) != 0)
		{
			inter->Id = donnees_booleens.triangles_esect[f->label].RefInter[he->label];
			return;
		}
		//else, the calculation is done

		//this method is called when the intersection is exactly on the vertex pointed by the halfedge
		inter->IsOnVertex = true;
		//the intersection does not have an Id. 0xFFFFFFFF is set (this value means "no Id")
		inter->Id = 0xFFFFFFFF;

		dls::math::vec3f p = he->sommet->p;
		dls::math::vec3f v0 = f->arete->sommet->p;
		dls::math::vec3f v1 = f->arete->suivante->sommet->p;
		dls::math::vec3f v2 = f->arete->suivante->suivante->sommet->p;

		dls::math::vec3f N = donnees_booleens.triangles_esect[f->label].norm_dir;
		float u, v, w;

		u = produit_scalaire(N, produit_croix(v0 - v2, p - v2));
		if (u < 0)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}
		v = produit_scalaire(N, produit_croix(v1 - v0, p - v0));
		if (v < 0)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}
		w = produit_scalaire(N, produit_croix(v2 - v1, p - v1));
		if (w < 0)
		{
			//the intersection is not in the triangle
			inter->res = 7;
			return;
		}

		//the point is in the triangle
		inter->pt = p;

		//creation of the code for the location of the intersection
		inter->res = 0;
		if (u == 0.0f) inter->res += 1;	//intersection on he(0)
		if (v == 0.0f) inter->res += 2;	//intersection on he(1)
		if (w == 0.0f) inter->res += 4;	//intersection on he(2)
	}

	/*! \brief Verify that the intersection is a segment
		 * \param inter : A pointer to four Info_Inter structures
		 * \return true if two distinct points are found in the four intersections computed*/
	bool IsSegment(Info_Inter *inter)
	{
		bool point = false; //true if a point is founded
		dls::math::vec3f pt; //the point founded
		bool id = false; //true if an Id is founded
		unsigned long Id = 0; //the Id founded // MT

		//each intersection is checked separately.
		//first intersection
		if (inter[0].Id != 0xFFFFFFFF)
		{
			//an Id different than 0xFFFFFFFF is founded
			//this intersection has already been computed and is valid
			id = true;
			Id = inter[0].Id;
		}
		else if (inter[0].res != 7)
		{
			//the intersection have no Id (0xFFFFFFFF)
			//but the intersection is in the triangle
			point = true;
			pt = inter[0].pt;
		}
		//second intersection
		if (inter[1].Id != 0xFFFFFFFF)
		{
			//an Id different than 0xFFFFFFFF is founded
			//this intersection has already been computed and is valid

			//if a point or an Id has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if (point || id) return true;
			id = true;
			Id = inter[1].Id;
		}
		else if (inter[1].res != 7)
		{
			//the intersection have no Id (0xFFFFFFFF)
			//but the intersection is in the triangle

			//if a point or an Id has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if (point || id) return true;
			point = true;
			pt = inter[1].pt;
		}
		//third intersection
		if (inter[2].Id != 0xFFFFFFFF)
		{
			//an Id different than 0xFFFFFFFF is founded
			//this intersection has already been computed and is valid

			//if a point or a different Id has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if (point || (id && Id != inter[2].Id)) return true;
			id = true;
			Id = inter[2].Id;
		}
		else if (inter[2].res != 7)
		{
			//the intersection have no Id (0xFFFFFFFF)
			//but the intersection is in the triangle

			//if an Id or a different point has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if ((point && pt != inter[2].pt) || id) return true;
			point = true;
			pt = inter[2].pt;
		}
		//fourth intersection
		if (inter[3].Id != 0xFFFFFFFF)
		{
			//an Id different than 0xFFFFFFFF is founded
			//this intersection has already been computed and is valid

			//if a point or a different Id has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if (point || (id && Id != inter[3].Id)) return true;
		}
		else if (inter[3].res != 7)
		{
			//the intersection have no Id (0xFFFFFFFF)
			//but the intersection is in the triangle

			//if an Id or a different point has already be founded, we founded the two distinct valid points (the intersection is a segment)
			//(it is not possible that the two first points are the same)
			if ((point && pt != inter[3].pt) || id) return true;
		}
		return false;
	}

	void Get_Segment(DonneesBooleen &donnees_booleens, Info_Inter *inter, dls::tableau<unsigned> &I)
	{
		for(unsigned int i = 0;i != 4;++i)
		{
			//if the point have an Id
			if (inter[i].Id != 0xFFFFFFFF)
			{
				//the Id is stored if it is not already done
				if (I.taille() == 0 || I[0] != inter[i].Id) I.pousse(inter[i].Id);
			}
			//else if the point is valid
			else if (inter[i].res != 7)
			{
				//the intersection point is stored in the list of the intersection points
				//and its new Id is stored in the output segment
				if (I.taille() == 0 || InterPts[I[0]] != inter[i].pt)
				{
					Store_Intersection(donnees_booleens, &inter[i]);
					I.pousse(inter[i].Id);
				}
			}
			//return if the two points are founded
			if (I.taille() == 2) return;
		}
	}

	void Store_Intersection(DonneesBooleen &donnees_booleens, Info_Inter *inter)
	{
		mi_face *f;
		mi_arete *he;
		f = inter->f;
		he = inter->he;
		unsigned I;

		//store the point to the list of the intersections and store its new Id
		inter->Id = static_cast<unsigned>(InterPts.taille());
		I = inter->Id;
		InterPts.pousse(inter->pt);

		//add this point as a vertex of the result
//		ppbuilder.add_vertex(inter->pt, inter->Id);

		//if the intersection is on the vertex pointed by the halfedge (he), we update the Id (label) of this vertex
		if (inter->IsOnVertex)
			he->sommet->label = I;

		//the intersection is memorized for each possible couple of (facet, halfedge) concerned by the intersection
		//if the intersection is exactly on the vertex pointed by the halfedge (he), it is necessary to take account
		//of every halfedge pointing to this vertex
		switch(inter->res) {
			case 0: //intersection on the facet
			{
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[f->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->label].RefInter[he->paire->label] = I;
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 1: //Intersection on the first halfedge of the facet
			{
				donnees_booleens.triangles_esect[f->label].PtList.insere(I);
				donnees_booleens.triangles_esect[f->arete->paire->face->label].PtList.insere(I);
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[f->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[f->arete->paire->face->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->arete->paire->face->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->paire->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->paire->label] = I;
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[f->arete->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->arete->paire->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 2: //Intersection on the second halfedge of the facet
			{
				donnees_booleens.triangles_esect[f->label].PtList.insere(I);
				donnees_booleens.triangles_esect[f->arete->suivante->paire->face->label].PtList.insere(I);
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[f->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[f->arete->suivante->paire->face->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->arete->suivante->paire->face->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->suivante->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->suivante->paire->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->suivante->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->suivante->paire->label] = I;
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[f->arete->suivante->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->arete->suivante->paire->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->suivante->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->suivante->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 3: //Intersection on the first and second halfedge of the facet (vertex pointed by the first halfedge)
			{
				//update the Id (label) of the first vertex of the facet
				f->arete->sommet->label = I;
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);

					auto H_circ = f->arete, H_end = f->arete;
					do {
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->paire->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						auto F_circ = f->arete, F_end = f->arete;
						do {
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->label] = I;
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->paire->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->paire->label] = I;
							F_circ = suivante_autour_point(F_circ);
						} while(F_circ != F_end);
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 4: //Intersection on the third halfedge of the facet
			{
				donnees_booleens.triangles_esect[f->label].PtList.insere(I);
				donnees_booleens.triangles_esect[f->arete->suivante->suivante->paire->face->label].PtList.insere(I);
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[f->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[f->arete->suivante->suivante->paire->face->label].RefInter[he->label] = I;
					donnees_booleens.triangles_esect[f->arete->suivante->suivante->paire->face->label].RefInter[he->paire->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->suivante->suivante->label] = I;
					donnees_booleens.triangles_esect[he->face->label].RefInter[f->arete->suivante->suivante->paire->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->suivante->suivante->label] = I;
					donnees_booleens.triangles_esect[he->paire->face->label].RefInter[f->arete->suivante->suivante->paire->label] = I;
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[f->arete->suivante->suivante->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[f->arete->suivante->suivante->paire->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->suivante->suivante->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[f->arete->suivante->suivante->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 5: //Intersection on the first and third halfedge of the facet (vertex pointed by the third halfedge)
			{
				//update the Id (label) of the third vertex of the facet
				f->arete->suivante->suivante->sommet->label = I;
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);

					auto H_circ = f->arete->suivante->suivante,
							H_end = f->arete->suivante->suivante;
					do {
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->paire->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						auto F_circ = f->arete->suivante->suivante,
								F_end = f->arete->suivante->suivante;
						do {
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->label] = I;
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->paire->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->paire->label] = I;
							F_circ = suivante_autour_point(F_circ);
						} while(F_circ != F_end);
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
			case 6: //Intersection on the second and third halfedge of the facet (vertex pointed by the second halfedge)
			{
				//update the Id (label) of the second vertex of the facet
				f->arete->suivante->sommet->label = I;
				if (!inter->IsOnVertex)
				{
					donnees_booleens.triangles_esect[he->face->label].PtList.insere(I);
					donnees_booleens.triangles_esect[he->paire->face->label].PtList.insere(I);

					auto H_circ = f->arete->suivante, H_end = f->arete->suivante;
					do {
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->label] = I;
						donnees_booleens.triangles_esect[H_circ->face->label].RefInter[he->paire->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->face->label].RefInter[H_circ->paire->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->label] = I;
						donnees_booleens.triangles_esect[he->paire->face->label].RefInter[H_circ->paire->label] = I;
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}
				else
				{
					auto H_circ = he, H_end = he;
					do {
						auto F_circ = f->arete->suivante, F_end = f->arete->suivante;
						do {
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->label] = I;
							donnees_booleens.triangles_esect[F_circ->face->label].RefInter[H_circ->paire->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->label] = I;
							donnees_booleens.triangles_esect[H_circ->face->label].RefInter[F_circ->paire->label] = I;
							F_circ = suivante_autour_point(F_circ);
						} while(F_circ != F_end);
						H_circ = suivante_autour_point(H_circ);
					} while(H_circ != H_end);
				}

				break;
			}
		}
	}

	/*! \brief Add a facet to the result
		 * \param pFacet : A handle to the facet to add
		 * \param facet_from_A : must be true if the facet belongs to the first polyhedron*/
//	void add_facet_to_solution(DonneesBooleen &donnees_booleens, mi_triangle *&pFacet, bool facet_from_A)
//	{
//		//if the facet contains an intersection point but no intersection segment, the facet must be triangulate before
//		if (pFacet->label < donnees_booleens.triangles_esect.taille()) {
//			auto TriCut = donnees_booleens.triangles_esect[pFacet->label];
//			mi_arete *he = pFacet->arete;
//			//creation of the triangulation
//			Triangulation<Exact_Kernel> T(he, TriCut.norm_dir);
//			//add the intersection points to the triangulation
//			for(std::set<unsigned>::iterator i = TriCut.PtList.begin();i != TriCut.PtList.end();++i)
//			{
//				T.add_new_pt(InterPts[*i], (unsigned long &)*i);    // MT: ajout cast
//			}
//			//get all the triangles of the triangulation
//			dls::tableau<dls::tableau<unsigned long> > Tri_set = T.get_all_triangles((m_BOOP == MINUS && !TriCut.Facet_from_A));
//			//add these triangles to the result
//			ppbuilder.add_triangle(Tri_set, he);
//		}
//		else {
//			//the facet is added to the result. If the facet belongs to the second polyhedron, and if the
//			//Boolean operation is a Subtraction, it is necessary to invert the orientation of the facet.
//			ppbuilder.add_triangle(pFacet, (m_BOOP == MINUS && !facet_from_A));
//		}
//		//the tag of the three neighboring facets is updated
//		pFacet->arete->paire->face->est_ext = true;
//		pFacet->arete->suivante->paire->face->est_ext = true;
//		pFacet->arete->suivante->suivante->paire->face->est_ext = true;
//	}
};
#endif // OPBOOLEEN

/* ************************************************************************** */

/* Necessary Voro++ data for fracture:
 * %i the particle/cell index
 *
 * %w number of vertices (totvert)
 * %P global vertex coordinates
 * v  vertex section delimiter
 *
 * %s number of faces (totpoly)
 * %a number of vertices in each face (sum is totloop)
 * %t the indices to the cell vertices, describes which vertices build each face
 * %n neighboring cell index for each face
 * f  face section delimiter
 *
 * %C the centroid of the voronoi cell
 * c  centroid section delimiter
 */

struct cell {
	dls::tableau<double> verts{};
	dls::tableau<int> poly_totvert{};
	dls::tableau<int> poly_indices{};
	dls::tableau<int> voisines{};

	float centroid[3] = { 0.0f, 0.0f, 0.0f };
	float volume = 0.0f;
	int index = 0;
	int totvert = 0;
	int totpoly = 0;
	int pad = 0;

	cell() = default;
};

class OperatriceFractureVoronoi final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fracture Voronoi";
	static constexpr auto AIDE = "";

	OperatriceFractureVoronoi(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_fracture_voronoi.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		/* À FAIRE : booléens, groupes. */
		this->ajoute_avertissement("Seuls les cubes sont supportés pour le moment !");

		m_corps.reinitialise();

		auto corps_maillage = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_maillage, true, false)) {
			return res_exec::ECHOUEE;
		}

		auto corps_points = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_points, true, false, 1)) {
			return res_exec::ECHOUEE;
		}

		/* création du conteneur */
		auto limites = calcule_limites_mondiales_corps(*corps_maillage);
		auto min = dls::math::converti_type<double>(limites.min);
		auto max = dls::math::converti_type<double>(limites.max);

		/* Divise le domaine de calcul. */
		auto nombre_block = dls::math::vec3i(8);

		auto const periodic_x = evalue_bool("périodic_x");
		auto const periodic_y = evalue_bool("périodic_y");
		auto const periodic_z = evalue_bool("périodic_z");

		auto points_entree = corps_points->points_pour_lecture();

		/* À FAIRE : rayon de particules : container_poly. */
		auto cont_voro = memoire::loge<voro::container>(
					"voro::container",
					min.x, max.x, min.y, max.y, min.z, max.z,
					nombre_block.x, nombre_block.y, nombre_block.z,
					periodic_x, periodic_y, periodic_z,
					static_cast<int>(points_entree.taille()));

		auto ordre_parts = memoire::loge<voro::particle_order>("voro::particle_order");

		/* ajout des particules */

		for (auto i = 0; i < points_entree.taille(); ++i) {
			auto point = points_entree.point_local(i);
			auto point_monde = corps_points->transformation(dls::math::point3d(point.x, point.y, point.z));
			cont_voro->put(*ordre_parts, i, point_monde.x, point_monde.y, point_monde.z);
		}

		/* création des cellules */
		auto cellules = dls::tableau<cell>();
		cellules.reserve(points_entree.taille());

		/* calcul des cellules */

		container_compute_cells(cont_voro, ordre_parts, cellules);

		/* conversion des données */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::PRIMITIVE);
		auto points = m_corps.points_pour_ecriture();
		auto gna = GNA();

		auto poly_index_offset = 0;

		for (auto &c : cellules) {
			for (auto i = 0; i < c.totvert; ++i) {
				auto px = static_cast<float>(c.verts[i * 3]);
				auto py = static_cast<float>(c.verts[i * 3 + 1]);
				auto pz = static_cast<float>(c.verts[i * 3 + 2]);
				points.ajoute_point(px, py, pz);
			}

			auto couleur = gna.uniforme_vec3(0.0f, 1.0f);
			auto skip = 0;

			for (auto i = 0; i < c.totpoly; ++i) {
				auto nombre_verts = c.poly_totvert[i];

				auto poly = m_corps.ajoute_polygone(type_polygone::FERME, nombre_verts);

				assigne(attr_C->r32(poly->index), couleur);
				for (auto j = 0; j < nombre_verts; ++j) {
					m_corps.ajoute_sommet(poly, poly_index_offset + c.poly_indices[skip + 1 + j]);
				}

				skip += (nombre_verts + 1);
			}

			poly_index_offset += c.totvert;
		}

		memoire::deloge("voro::container", cont_voro);
		memoire::deloge("voro::particle_order", ordre_parts);

		return res_exec::REUSSIE;
	}

	void container_compute_cells(voro::container* cn, voro::particle_order* po, dls::tableau<cell> &cells)
	{
		voro::voronoicell_neighbor vc;
		voro::c_loop_order vl(*cn, *po);

		if (!vl.start()) {
			return;
		}
		cell c;

		do {
			if (!cn->compute_cell(vc, vl)) {
				/* cellule invalide */
				continue;
			}

			/* adapté de voro++ */
			double *pp = vl.p[vl.ijk] + vl.ps * vl.q;

			/* index de la particule de la cellule */
			c.index = cn->id[vl.ijk][vl.q];

			/* sommets */
			vc.vertices(pp[0], pp[1], pp[2], c.verts);
			c.totvert = vc.p;

			/* polygones */
			c.totpoly = vc.number_of_faces();
			vc.face_orders(c.poly_totvert);
			vc.face_vertices(c.poly_indices);

			/* voisines */
			vc.neighbors(c.voisines);

			/* centroid */
			double centroid[3];
			vc.centroid(centroid[0], centroid[1], centroid[2]);
			c.centroid[0] = static_cast<float>(centroid[0] + pp[0]);
			c.centroid[1] = static_cast<float>(centroid[1] + pp[1]);
			c.centroid[2] = static_cast<float>(centroid[2] + pp[2]);

			/* volume */
			c.volume = static_cast<float>(vc.volume());

			cells.pousse(c);
		}
		while(vl.inc());
	}
};

/* ************************************************************************** */

void enregistre_operatrices_fracture(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceFractureVoronoi>());

#ifdef OPBOOLEEN
	usine.enregistre_type(cree_desc<OpBooleensMaillage>());
#endif
}

#pragma clang diagnostic pop
