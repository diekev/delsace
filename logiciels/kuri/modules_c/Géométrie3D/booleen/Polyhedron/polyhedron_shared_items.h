#ifndef HEADER_POLYHEDRON_SHARED_ITEMS
#define HEADER_POLYHEDRON_SHARED_ITEMS

#ifdef _MSC_VER
#    if !defined(NOMINMAX)
#        define NOMINMAX
#    endif
#    include <windows.h>
#    pragma warning(disable : 4996)
#endif

#include <fstream>
#include <list>

#include "CGAL/exceptions.h"

// For Polyhedron copy
#include "Polyhedron_Copy.h"

using namespace std;
using namespace CGAL;

struct Texture {
    QString m_name;
    QImage m_data;
    GLuint m_id;
};

// compute facet normal
struct Facet_normal {
    template <class Facet>
    void operator()(Facet &f)
    {
        typename Facet::Normal_3 sum = CGAL::NULL_VECTOR;
        typename Facet::Halfedge_around_facet_circulator h = f.facet_begin();
        do {
            typename Facet::Normal_3 normal = CGAL::cross_product(
                h->next()->vertex()->point() - h->vertex()->point(),
                h->next()->next()->vertex()->point() - h->next()->vertex()->point());
            double sqnorm = to_double(normal * normal);
            if (sqnorm != 0)
                normal = normal / (float)std::sqrt(sqnorm);
            sum = sum + normal;
        } while (++h != f.facet_begin());
        double sqnorm = to_double(sum * sum);
        if (sqnorm != 0.0)
            f.normal() = sum / std::sqrt(sqnorm);
        else {
            f.normal() = CGAL::NULL_VECTOR;
            // TRACE("degenerate face\n");
        }
    }
};
// from Hichem
/*struct Facet_normal
{
    template <class Facet>
    void operator()(Facet& f)
    {
        typedef typename Facet::Normal_3		Vector_3;
        Vector_3								facet_normal;
        typename Facet::Halfedge_around_facet_const_circulator h = f.facet_begin();
        do
        {
            facet_normal = CGAL::cross_product(h->next()->vertex()->point() - h->vertex()->point(),
                h->next()->next()->vertex()->point() - h->next()->vertex()->point());
        }
        while (facet_normal != CGAL::NULL_VECTOR && ++h	!= f.facet_begin());
        if(facet_normal != CGAL::NULL_VECTOR)
        {
            f.normal() = facet_normal;
        }
        else // All consecutive facet edges are collinear --> degenerate (0 area) facet
        {
            std::cerr <<std::endl << "Degenerate polyhedron facet" << std::endl << std::flush;
            f.normal() = CGAL::NULL_VECTOR;
            assert(facet_normal != CGAL::NULL_VECTOR);
        }
    }
};*/

// compute vertex normal
struct Vertex_normal {
    template <class Vertex>
    void operator()(Vertex &v)
    {
        typename Vertex::Normal_3 normal = CGAL::NULL_VECTOR;
        typename Vertex::Halfedge_around_vertex_const_circulator pHalfedge = v.vertex_begin();
        typename Vertex::Halfedge_around_vertex_const_circulator begin = pHalfedge;
        CGAL_For_all(pHalfedge, begin) if (!pHalfedge->is_border())
            normal = normal + pHalfedge->facet()->normal();
        double sqnorm = to_double(normal * normal);
        if (sqnorm != 0.0f)
            v.normal() = normal / (float)std::sqrt(sqnorm);
        else
            v.normal() = CGAL::NULL_VECTOR;
    }
};

#if (0)
// compute halfedge normal : ajout Céline
template <class kernel>
struct Halfedge_normal  // (functor)
{
    typedef typename kernel::FT FT;

    template <class Halfedge>
    void operator()(Halfedge &h)
    {
        typename Halfedge::Normal_3 normal = CGAL::NULL_VECTOR;

        if (!h.is_border())
            normal = normal + (h.facet()->normal());
        if (!(h.opposite()->is_border()))
            normal = normal + (h.opposite()->facet()->normal());

        FT sqnorm = normal * normal;
        if (sqnorm != 0.0f)
            h.normal() = normal / (FT)std::sqrt(sqnorm);
        else
            h.normal() = CGAL::NULL_VECTOR;
    }
};
#endif

template <class Refs, class T, class Norm>
class MEPP_Common_Facet : public CGAL::HalfedgeDS_face_base<Refs, T> {
  protected:
    // tag
    int m_tag;

    // normal
    Norm m_normal;

    // color
    float m_color[3];

  public:
    // life cycle
    MEPP_Common_Facet()
    {
        color(0.5f, 0.5f, 0.5f);
    }

    // tag
    const int &tag() const
    {
        return m_tag;
    }
    int &tag()
    {
        return m_tag;
    }
    void tag(const int &t)
    {
        m_tag = t;
    }

    // normal
    typedef Norm Normal_3;
    Normal_3 &normal()
    {
        return m_normal;
    }
    const Normal_3 &normal() const
    {
        return m_normal;
    }

    // color
    float color(int index)
    {
        return m_color[index];
    }
    void color(float r, float g, float b)
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
    }
};

template <class Refs, class Tprev, class Tvertex, class Tface, class Norm>
class MEPP_Common_Halfedge : public CGAL::HalfedgeDS_halfedge_base<Refs, Tprev, Tvertex, Tface> {
  protected:
    // tag
    int m_tag;

#if (0)
    // normal
    Norm m_normal;  // AJOUT Céline : halfedge normal (sum of 2 incident facet normals)
#endif

    // option for edge superimposing
    bool m_control_edge;

    // texture coordinates : AJOUT Laurent Chevalier
    float m_texture_coordinates[2];

  public:
    // life cycle
    MEPP_Common_Halfedge()
    {
        m_control_edge = true;

        // texture coordinates : AJOUT Laurent Chevalier
        texture_coordinates(0.0f, 0.0f);
    }

    // tag
    const int &tag() const
    {
        return m_tag;
    }
    int &tag()
    {
        return m_tag;
    }
    void tag(const int &t)
    {
        m_tag = t;
    }

#if (0)
    // normal : AJOUT Céline
    typedef Norm Normal_3;
    Normal_3 &normal()
    {
        return m_normal;
    }
    const Normal_3 &normal() const
    {
        return m_normal;
    }
#endif

    // texture coordinates : AJOUT Laurent Chevalier
    float texture_coordinates(int index)
    {
        return m_texture_coordinates[index];
    }
    void texture_coordinates(float u, float v)
    {
        m_texture_coordinates[0] = u;
        m_texture_coordinates[1] = v;
    }

    // control edge
    bool &control_edge()
    {
        return m_control_edge;
    }
    const bool &control_edge() const
    {
        return m_control_edge;
    }
    void control_edge(const bool &flag)
    {
        m_control_edge = flag;
    }
};

template <class Refs, class T, class P, class Norm>
class MEPP_Common_Vertex : public CGAL::HalfedgeDS_vertex_base<Refs, T, P> {
  protected:
    // tag
    int m_tag;

    // normal
    Norm m_normal;

    // color
    float m_color[3];

    // texture coordinates
    float m_texture_coordinates[2];

  public:
    // life cycle
    MEPP_Common_Vertex()
    {
        color(0.5f, 0.5f, 0.5f);
        texture_coordinates(0.0f, 0.0f);
    }
    // repeat mandatory constructors
    MEPP_Common_Vertex(const P &pt) : CGAL::HalfedgeDS_vertex_base<Refs, T, P>(pt)
    {
        color(0.5f, 0.5f, 0.5f);
        texture_coordinates(0.0f, 0.0f);
    }

    // color
    float color(int index)
    {
        return m_color[index];
    }
    void color(float r, float g, float b)
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
    }

    // texture coordinates
    float texture_coordinates(int index)
    {
        return m_texture_coordinates[index];
    }
    void texture_coordinates(float u, float v)
    {
        m_texture_coordinates[0] = u;
        m_texture_coordinates[1] = v;
    }

    // normal
    typedef Norm Normal_3;
    // typedef Norm Vector;

    Normal_3 &normal()
    {
        return m_normal;
    }
    const Normal_3 &normal() const
    {
        return m_normal;
    }

    // tag
    int &tag()
    {
        return m_tag;
    }
    const int &tag() const
    {
        return m_tag;
    }
    void tag(const int &t)
    {
        m_tag = t;
    }
};

template <class kernel, class items>
class MEPP_Common_Polyhedron : public CGAL::Polyhedron_3<kernel, items> {
  public:
    typedef typename kernel::FT FT;
    typedef typename kernel::Point_3 Point;
    typedef typename kernel::Vector_3 Vector;
    typedef typename kernel::Iso_cuboid_3 Iso_cuboid;
    typedef typename MEPP_Common_Polyhedron::Facet_handle Facet_handle;
    typedef typename MEPP_Common_Polyhedron::Vertex_handle Vertex_handle;
    typedef typename MEPP_Common_Polyhedron::Halfedge_handle Halfedge_handle;
    typedef typename MEPP_Common_Polyhedron::Facet_iterator Facet_iterator;
    typedef typename MEPP_Common_Polyhedron::Vertex_iterator Vertex_iterator;
    typedef typename MEPP_Common_Polyhedron::Halfedge_iterator Halfedge_iterator;
    typedef typename MEPP_Common_Polyhedron::Halfedge_around_vertex_circulator
        Halfedge_around_vertex_circulator;
    typedef typename MEPP_Common_Polyhedron::Halfedge_around_facet_circulator
        Halfedge_around_facet_circulator;
    typedef typename MEPP_Common_Polyhedron::Point_iterator Point_iterator;
    typedef typename MEPP_Common_Polyhedron::Edge_iterator Edge_iterator;
    typedef typename MEPP_Common_Polyhedron::HalfedgeDS HalfedgeDS;
    typedef typename HalfedgeDS::Face Facet;
    typedef typename Facet::Normal_3 Normal;
    typedef Aff_transformation_3<kernel> Affine_transformation;

  protected:
    // tag : AJOUT Céline
    int m_tag;

    // bounding box
    Iso_cuboid m_bbox;

    bool m_pure_quad;
    bool m_pure_triangle;

    bool m_has_color;
    bool m_has_texture_coordinates;

    // MT
    unsigned int m_nb_components;
    unsigned int m_nb_boundaries;

  public:
    // life cycle
    MEPP_Common_Polyhedron()
    {
        m_pure_quad = false;
        m_pure_triangle = false;
        m_has_color = false;
        m_has_texture_coordinates = false;

        // MT
        m_nb_components = 0;
        m_nb_boundaries = 0;
    }

    virtual ~MEPP_Common_Polyhedron()
    {
    }

    // MT
    unsigned int nb_components()
    {
        return m_nb_components;
    }
    unsigned int nb_boundaries()
    {
        return m_nb_boundaries;
    }
    string pName;

    // tag : AJOUT Céline
    int &tag()
    {
        return m_tag;
    }
    const int &tag() const
    {
        return m_tag;
    }
    void tag(const int &t)
    {
        m_tag = t;
    }

    // type
    bool is_pure_triangle()
    {
        return m_pure_triangle;
    }
    bool is_pure_quad()
    {
        return m_pure_quad;
    }

    // normals (per facet, then per vertex)
    void compute_normals_per_facet()
    {
        std::for_each(this->facets_begin(), this->facets_end(), Facet_normal());
    }
    void compute_normals_per_vertex()
    {
        std::for_each(this->vertices_begin(), this->vertices_end(), Vertex_normal());
    }
#if (0)
    // ajout Céline :
    void compute_normals_per_halfedge()
    {
        std::for_each(this->halfedges_begin(), this->halfedges_end(), Halfedge_normal<kernel>());
    }
#endif

    void compute_normals()
    {
        compute_normals_per_facet();
        compute_normals_per_vertex();
#if (0)
        compute_normals_per_halfedge();  // ajout Céline
#endif
    }

    // bounding box
    Iso_cuboid &bbox()
    {
        return m_bbox;
    }
    const Iso_cuboid bbox() const
    {
        return m_bbox;
    }

    // compute bounding box
    void compute_bounding_box()
    {
        if (this->size_of_vertices() == 0) {
            return;
        }

        FT xmin, xmax, ymin, ymax, zmin, zmax;
        Vertex_iterator pVertex = this->vertices_begin();
        xmin = xmax = pVertex->point().x();
        ymin = ymax = pVertex->point().y();
        zmin = zmax = pVertex->point().z();

        for (; pVertex != this->vertices_end(); pVertex++) {
            const Point &p = pVertex->point();

            xmin = std::min(xmin, p.x());
            ymin = std::min(ymin, p.y());
            zmin = std::min(zmin, p.z());

            xmax = std::max(xmax, p.x());
            ymax = std::max(ymax, p.y());
            zmax = std::max(zmax, p.z());
        }

        m_bbox = Iso_cuboid(xmin, ymin, zmin, xmax, ymax, zmax);
    }

    // bounding box
    FT xmin()
    {
        return m_bbox.xmin();
    }
    FT xmax()
    {
        return m_bbox.xmax();
    }
    FT ymin()
    {
        return m_bbox.ymin();
    }
    FT ymax()
    {
        return m_bbox.ymax();
    }
    FT zmin()
    {
        return m_bbox.zmin();
    }
    FT zmax()
    {
        return m_bbox.zmax();
    }

    // copy bounding box
    void copy_bounding_box(MEPP_Common_Polyhedron<kernel, items> *pMesh)
    {
        m_bbox = pMesh->bbox();
    }

    // degree of a face
    static size_t degree(Facet_handle pFace)
    {
        return CGAL::circulator_size(pFace->facet_begin());
    }

    // valence of a vertex
    static unsigned int valence(Vertex_handle pVertex)
    {
        return CGAL::circulator_size(pVertex->vertex_begin());
    }

    // check wether a vertex is on a boundary or not
    static bool is_border(Vertex_handle pVertex)
    {
        Halfedge_around_vertex_circulator pHalfEdge = pVertex->vertex_begin();
        if (pHalfEdge == NULL)  // isolated vertex
            return true;
        Halfedge_around_vertex_circulator d = pHalfEdge;
        CGAL_For_all(pHalfEdge, d) if (pHalfEdge->is_border()) return true;
        return false;
    }

    // get any border halfedge attached to a vertex
    Halfedge_handle get_border_halfedge(Vertex_handle pVertex)
    {
        Halfedge_around_vertex_circulator pHalfEdge = pVertex->vertex_begin();
        Halfedge_around_vertex_circulator d = pHalfEdge;
        CGAL_For_all(pHalfEdge, d) if (pHalfEdge->is_border()) return pHalfEdge;
        return NULL;
    }

    // tag all halfedges
    void tag_halfedges(const int tag)
    {
        for (Halfedge_iterator pHalfedge = this->halfedges_begin();
             pHalfedge != this->halfedges_end();
             pHalfedge++)
            pHalfedge->tag(tag);
    }

    // tag all facets
    void tag_facets(const int tag)
    {
        for (Facet_iterator pFacet = this->facets_begin(); pFacet != this->facets_end(); pFacet++)
            pFacet->tag(tag);
    }

    // set index for all vertices
    void set_index_vertices()
    {
        int index = 0;
        for (Vertex_iterator pVertex = this->vertices_begin(); pVertex != this->vertices_end();
             pVertex++)
            pVertex->tag(index++);
    }

    // is pure degree ?
    bool is_pure_degree(unsigned int d)
    {
        for (Facet_iterator pFace = this->facets_begin(); pFace != this->facets_end(); pFace++)
            if (degree(pFace) != d)
                return false;
        return true;
    }

    // compute type
    void compute_type()
    {
        m_pure_quad = is_pure_degree(4);
        m_pure_triangle = is_pure_degree(3);
    }

    // compute facet center
    void compute_facet_center(Facet_handle pFace, Point &center)
    {
        Halfedge_around_facet_circulator pHalfEdge = pFace->facet_begin();
        Halfedge_around_facet_circulator end = pHalfEdge;
        Vector vec(0.0, 0.0, 0.0);
        int degree = 0;
        CGAL_For_all(pHalfEdge, end)
        {
            vec = vec + (pHalfEdge->vertex()->point() - CGAL::ORIGIN);
            degree++;
        }
        center = CGAL::ORIGIN + (vec / (FT)degree);
    }

    // compute average edge length around a vertex
    FT average_edge_length_around(Vertex_handle pVertex)
    {
        FT sum = 0.0;
        Halfedge_around_vertex_circulator pHalfEdge = pVertex->vertex_begin();
        Halfedge_around_vertex_circulator end = pHalfEdge;
        Vector vec(0.0, 0.0, 0.0);
        int degree = 0;
        CGAL_For_all(pHalfEdge, end)
        {
            Vector vec = pHalfEdge->vertex()->point() - pHalfEdge->opposite()->vertex()->point();
            sum += std::sqrt(to_double(vec * vec));
            degree++;
        }
        return sum / (FT)degree;
    }

    // count #boundaries
    unsigned int calc_nb_boundaries()
    {
        unsigned int nb = 0;
        tag_halfedges(0);
        for (Halfedge_iterator he = this->halfedges_begin(); he != this->halfedges_end(); he++) {
            if (he->is_border() && he->tag() == 0) {
                nb++;
                Halfedge_handle curr = he;
                do {
                    curr = curr->next();
                    curr->tag(1);
                } while (curr != he);
            }
        }
        m_nb_boundaries = nb;
        return nb;
    }

    // tag component
    void tag_component(Facet_handle pSeedFacet, const int tag_free, const int tag_done)
    {
        pSeedFacet->tag(tag_done);
        std::list<Facet_handle> facets;
        facets.push_front(pSeedFacet);
        while (!facets.empty()) {
            Facet_handle pFacet = facets.front();
            facets.pop_front();
            pFacet->tag(tag_done);
            Halfedge_around_facet_circulator pHalfedge = pFacet->facet_begin();
            Halfedge_around_facet_circulator end = pHalfedge;
            CGAL_For_all(pHalfedge, end)
            {
                Facet_handle pNFacet = pHalfedge->opposite()->facet();
                if (pNFacet != NULL && pNFacet->tag() == tag_free) {
                    facets.push_front(pNFacet);
                    pNFacet->tag(tag_done);
                }
            }
        }
    }

    // count #components
    unsigned int calc_nb_components()
    {
        unsigned int nb = 0;
        tag_facets(0);
        for (Facet_iterator pFacet = this->facets_begin(); pFacet != this->facets_end();
             pFacet++) {
            if (pFacet->tag() == 0) {
                nb++;
                tag_component(pFacet, 0, 1);
            }
        }
        m_nb_components = nb;
        return nb;
    }

    // compute the genus
    // V - E + F + B = 2 (C - G)
    // C -> #connected components
    // G : genus
    // B : #boundaries
    int genus()
    {
        int c = this->nb_components();
        int b = this->nb_boundaries();
        int v = this->size_of_vertices();
        int e = this->size_of_halfedges() / 2;
        int f = this->size_of_facets();
        return genus(c, v, f, e, b);
    }

    int genus(int c, int v, int f, int e, int b)
    {
        return (2 * c + e - b - f - v) / 2;
    }

    void copy_from(MEPP_Common_Polyhedron<kernel, items> *new_mesh)
    {
        this->clear();

        CCopyPoly<MEPP_Common_Polyhedron<kernel, items>, kernel> copier;
        copier.copy(new_mesh, this);

        this->compute_bounding_box();

        this->compute_normals();
        this->compute_type();

        (void)this->calc_nb_components();
        (void)this->calc_nb_boundaries();

        copy_textures(new_mesh);
    }

    bool has_color()
    {
        return m_has_color;
    }
    void has_color(bool has_col)
    {
        m_has_color = has_col;
    }

    void fit_mesh()
    {
        // Transform mesh for standard size and position

        this->compute_bounding_box();

        // Translate
        FT xaverage = (this->xmax() + this->xmin()) / 2;
        FT yaverage = (this->ymax() + this->ymin()) / 2;
        FT zaverage = (this->zmax() + this->zmin()) / 2;

        Vector transl(-xaverage, -yaverage, -zaverage);

        // Scale
        FT scale = this->xmax() - this->xmin();
        if (scale < (this->ymax() - this->ymin())) {
            scale = this->ymax() - this->ymin();
        }
        if (scale < (this->zmax() - this->zmin())) {
            scale = this->zmax() - this->zmin();
        }
        scale = 10 / scale;

        Affine_transformation t(CGAL::TRANSLATION, transl);
        std::transform(this->points_begin(), this->points_end(), this->points_begin(), t);

        Affine_transformation s(CGAL::SCALING, scale);
        std::transform(this->points_begin(), this->points_end(), this->points_begin(), s);

        // Compute new bounding box
        this->compute_bounding_box();
    }

    void triangulate()
    {
        Facet_iterator f = this->facets_begin();
        Facet_iterator f2 = this->facets_begin();
        do  // for (; f != this->facets_end(); f++)
        {
            f = f2;
            if (f == this->facets_end()) {
                break;
            }
            f2++;

            if (!(f->is_triangle())) {
                int num = (int)(f->facet_degree() - 3);
                Halfedge_handle h = f->halfedge();

                h = this->make_hole(h);

                Halfedge_handle g = h->next();
                g = g->next();
                Halfedge_handle new_he = this->add_facet_to_border(h, g);
                new_he->texture_coordinates(h->texture_coordinates(0), h->texture_coordinates(1));
                new_he->opposite()->texture_coordinates(g->texture_coordinates(0),
                                                        g->texture_coordinates(1));
                g = new_he;

                num--;
                while (num != 0) {
                    g = g->opposite();
                    g = g->next();
                    Halfedge_handle new_he = this->add_facet_to_border(h, g);
                    new_he->texture_coordinates(h->texture_coordinates(0),
                                                h->texture_coordinates(1));
                    new_he->opposite()->texture_coordinates(g->texture_coordinates(0),
                                                            g->texture_coordinates(1));
                    g = new_he;

                    num--;
                }

                this->fill_hole(h);
            }

        } while (true);

        this->compute_normals();
        this->compute_type();
    }
};

#endif
