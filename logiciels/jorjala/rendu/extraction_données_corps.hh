/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "coeur/jorjala.hh"

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/couleur.hh"
#include "biblinternes/structures/tableau.hh"

/* ------------------------------------------------------------------------- */
/** \name DonnéesTampon
 * \{ */

struct DonnéesTampon {
    dls::tableau<dls::math::vec3f> points{};
    dls::tableau<dls::phys::couleur32> couleurs{};

    dls::tableau<dls::math::vec3f> points_polys{};
    dls::tableau<dls::math::vec3f> normaux{};
    dls::tableau<dls::phys::couleur32> couleurs_polys{};

    dls::tableau<dls::math::vec3f> points_segments{};
    dls::tableau<dls::phys::couleur32> couleurs_segments{};
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsPolygonesSeuls
 * \{ */

/** Extraction des données de rendu depuis un corps ne possèdant que des polygones. */
class ExtractriceCorpsPolygonesSeuls {
    JJL::Corps m_corps;

  public:
    ExtractriceCorpsPolygonesSeuls(JJL::Corps corps);

    void extrait_données(DonnéesTampon &données, dls::tableau<char> &points_utilisés);

    int64_t donne_nombre_de_polygones()
    {
        return m_corps.nombre_de_primitives();
    }

  private:
    int64_t calcule_nombre_de_triangles();

    void extrait_points_triangles(dls::tableau<dls::math::vec3f> &points,
                                  const int64_t nombre_de_triangles,
                                  dls::tableau<char> &points_utilisés);

    void extrait_normaux_triangles(dls::tableau<dls::math::vec3f> &normaux,
                                   const int64_t nombre_de_triangles);

    void extrait_couleurs_triangles(dls::tableau<dls::phys::couleur32> &couleurs,
                                    const int64_t nombre_de_triangles);

    template <typename T>
    void remplis_tampon_depuis_attribut_point(JJL::Attribut attribut,
                                              dls::tableau<T> &données_sortie);

    template <typename T>
    void remplis_tampon_depuis_attribut_primitive(JJL::Attribut attribut,
                                                  dls::tableau<T> &données_sortie);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsCourbesSeules
 * \{ */

/** Extraction des données de rendu depuis un corps ne possèdant que des courbes. */
class ExtractriceCorpsCourbesSeules {
    JJL::Corps m_corps;

  public:
    ExtractriceCorpsCourbesSeules(JJL::Corps corps);

    void extrait_données(DonnéesTampon &données, dls::tableau<char> &points_utilisés);

    int64_t donne_nombre_de_courbes()
    {
        return m_corps.nombre_de_primitives();
    }

  private:
    int64_t calcule_nombre_de_segments();

    void extrait_points_segments(dls::tableau<dls::math::vec3f> &points,
                                 const int64_t nombre_de_segments,
                                 dls::tableau<char> &points_utilisés);

    void extrait_couleurs_segments(dls::tableau<dls::phys::couleur32> &couleurs,
                                   const int64_t nombre_de_segments);

    template <typename T>
    void remplis_tampon_depuis_attribut_point(JJL::Attribut attribut,
                                              dls::tableau<T> &données_sortie);

    template <typename T>
    void remplis_tampon_depuis_attribut_primitive(JJL::Attribut attribut,
                                                  dls::tableau<T> &données_sortie);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsMixte
 * \{ */

/** Extraction des données de rendu depuis un corps composé de plusieurs types de primitives. */
class ExtractriceCorpsMixte {
    JJL::Corps m_corps;
    int64_t m_nombre_de_segments = 0;
    int64_t m_nombre_de_triangles = 0;
    int64_t m_nombre_de_polygones = 0;
    int64_t m_nombre_de_courbes = 0;

  public:
    ExtractriceCorpsMixte(JJL::Corps corps);

    void extrait_données(DonnéesTampon &données, dls::tableau<char> &points_utilisés);

    int64_t donne_nombre_de_polygones() const
    {
        return m_nombre_de_polygones;
    }

    int64_t donne_nombre_de_courbes() const
    {
        return m_nombre_de_courbes;
    }

  private:
    void calcule_compte_primitives();

    void extrait_points(dls::tableau<dls::math::vec3f> &points_polys,
                        dls::tableau<dls::math::vec3f> &points_segments,
                        dls::tableau<char> &points_utilisés);

    void extrait_normaux(dls::tableau<dls::math::vec3f> &normaux);

    void extrait_couleurs(dls::tableau<dls::phys::couleur32> &couleurs_polys,
                          dls::tableau<dls::phys::couleur32> &couleurs_segments);

    template <typename T>
    void remplis_tampon_depuis_attribut_point(JJL::Attribut attribut,
                                              dls::tableau<T> &données_polys,
                                              dls::tableau<T> &données_segments);

    template <typename T>
    void remplis_tampon_depuis_attribut_primitive(JJL::Attribut attribut,
                                                  dls::tableau<T> &données_polys,
                                                  dls::tableau<T> &données_segments);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsPoints
 * \{ */

/** Extraction des données des non-utilisés du corps. */
class ExtractriceCorpsPoints {
    JJL::Corps m_corps;
    int64_t m_nombre_de_points = 0;

  public:
    ExtractriceCorpsPoints(JJL::Corps corps);

    void extrait_données(DonnéesTampon &données, dls::tableau<char> &points_utilisés);

  private:
    int64_t calcule_compte_points(dls::tableau<char> &points_utilisés);

    void extrait_points(dls::tableau<dls::math::vec3f> &points,
                        dls::tableau<char> &points_utilisés);

    void extrait_couleurs(dls::tableau<dls::phys::couleur32> &couleurs,
                          dls::tableau<char> &points_utilisés);

    template <typename T>
    void remplis_tampon_depuis_attribut_point(JJL::Attribut attribut,
                                              dls::tableau<T> &données,
                                              dls::tableau<char> &points_utilisés);
};

/** \} */
