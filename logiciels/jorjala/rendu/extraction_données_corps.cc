/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "extraction_données_corps.hh"

#include "coeur/conversion_types.hh"

/* ------------------------------------------------------------------------- */
/** \name AccesseuseAttribut
 * \{ */

struct AccesseuseAttribut {
    JJL::Attribut m_attribut;

    AccesseuseAttribut(JJL::Attribut attribut) : m_attribut(attribut)
    {
    }

    template <typename T>
    T donne_valeur_pour_index(int64_t index);
};

template <>
bool AccesseuseAttribut::donne_valeur_pour_index<bool>(int64_t index)
{
    return m_attribut.bool_pour_index(index);
}

template <>
int AccesseuseAttribut::donne_valeur_pour_index<int>(int64_t index)
{
    return m_attribut.entier_pour_index(index);
}

template <>
float AccesseuseAttribut::donne_valeur_pour_index<float>(int64_t index)
{
    return m_attribut.réel_pour_index(index);
}

template <>
dls::math::vec2f AccesseuseAttribut::donne_valeur_pour_index<dls::math::vec2f>(int64_t index)
{
    return convertis_vecteur(m_attribut.vec2_pour_index(index));
}

template <>
dls::math::vec3f AccesseuseAttribut::donne_valeur_pour_index<dls::math::vec3f>(int64_t index)
{
    return convertis_vecteur(m_attribut.vec3_pour_index(index));
}

template <>
dls::math::vec4f AccesseuseAttribut::donne_valeur_pour_index<dls::math::vec4f>(int64_t index)
{
    return convertis_vecteur(m_attribut.vec4_pour_index(index));
}

template <>
dls::phys::couleur32 AccesseuseAttribut::donne_valeur_pour_index<dls::phys::couleur32>(
    int64_t index)
{
    return convertis_couleur(m_attribut.couleur_pour_index(index));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires locales.
 * \{ */

static std::optional<JJL::Attribut> donne_attribut(
    JJL::tableau<JJL_Attribut *, JJL::Attribut> attributs,
    JJL::TypeAttribut type,
    std::string const &nom)
{
    for (auto attribut : attributs) {
        if (attribut.donne_type() == type && attribut.donne_nom().vers_std_string() == nom) {
            return attribut;
        }
    }

    return {};
}

static void extrait_points_polygone(JJL::Corps corps,
                                    JJL::PrimitivePolygone polygone,
                                    dls::tableau<dls::math::vec3f> &points,
                                    int64_t &décalage_triangle,
                                    dls::tableau<char> &points_utilisés)
{

    for (int64_t ip = 2; ip < polygone.nombre_de_sommets(); ++ip) {
        auto i0 = polygone.sommet_pour_index(0);
        auto i1 = polygone.sommet_pour_index(ip - 1);
        auto i2 = polygone.sommet_pour_index(ip);

        auto p0 = corps.donne_point_local(i0);
        auto p1 = corps.donne_point_local(i1);
        auto p2 = corps.donne_point_local(i2);

        points[décalage_triangle] = convertis_point(p0);
        points[décalage_triangle + 1] = convertis_point(p1);
        points[décalage_triangle + 2] = convertis_point(p2);
        décalage_triangle += 3;

        points_utilisés[i0] = 1;
        points_utilisés[i1] = 1;
        points_utilisés[i2] = 1;
    }
}

static void extrait_points_courbe(JJL::Corps corps,
                                  JJL::PrimitiveCourbe courbe,
                                  dls::tableau<dls::math::vec3f> &points,
                                  int64_t &décalage_segment,
                                  dls::tableau<char> &points_utilisés)
{

    for (int64_t ip = 0; ip < courbe.nombre_de_sommets() - 1; ++ip) {
        auto i0 = courbe.sommet_pour_index(ip);
        auto i1 = courbe.sommet_pour_index(ip + 1);

        auto p0 = corps.donne_point_local(i0);
        auto p1 = corps.donne_point_local(i1);

        points[décalage_segment] = convertis_point(p0);
        points[décalage_segment + 1] = convertis_point(p1);
        décalage_segment += 2;

        points_utilisés[i0] = 1;
        points_utilisés[i1] = 1;
    }
}

template <typename T>
static void extrait_attribut_point(JJL::PrimitivePolygone polygone,
                                   AccesseuseAttribut &convertisseuse_valeur,
                                   dls::tableau<T> &données_sortie,
                                   int64_t &décalage_triangle)
{
    for (int64_t ip = 2; ip < polygone.nombre_de_sommets(); ++ip) {
        auto i0 = polygone.sommet_pour_index(0);
        auto i1 = polygone.sommet_pour_index(ip - 1);
        auto i2 = polygone.sommet_pour_index(ip);

        données_sortie[décalage_triangle] = convertisseuse_valeur.donne_valeur_pour_index<T>(i0);
        données_sortie[décalage_triangle + 1] = convertisseuse_valeur.donne_valeur_pour_index<T>(
            i1);
        données_sortie[décalage_triangle + 2] = convertisseuse_valeur.donne_valeur_pour_index<T>(
            i2);
        décalage_triangle += 3;
    }
}

template <typename T>
static void extrait_attribut_point(JJL::PrimitiveCourbe courbe,
                                   AccesseuseAttribut &convertisseuse_valeur,
                                   dls::tableau<T> &données_sortie,
                                   int64_t &décalage_segment)
{
    for (int64_t ip = 0; ip < courbe.nombre_de_sommets() - 1; ++ip) {
        auto i0 = courbe.sommet_pour_index(ip);
        auto i1 = courbe.sommet_pour_index(ip + 1);

        données_sortie[décalage_segment] = convertisseuse_valeur.donne_valeur_pour_index<T>(i0);
        données_sortie[décalage_segment + 1] = convertisseuse_valeur.donne_valeur_pour_index<T>(
            i1);
        décalage_segment += 2;
    }
}

template <typename T>
static void extrait_attribut_primitive(JJL::PrimitivePolygone polygone,
                                       AccesseuseAttribut &convertisseuse_valeur,
                                       int64_t i,
                                       dls::tableau<T> &données_sortie,
                                       int64_t &décalage_triangle)
{
    auto valeur = convertisseuse_valeur.donne_valeur_pour_index<T>(i);

    for (int64_t ip = 2; ip < polygone.nombre_de_sommets(); ++ip) {
        données_sortie[décalage_triangle] = valeur;
        données_sortie[décalage_triangle + 1] = valeur;
        données_sortie[décalage_triangle + 2] = valeur;
        décalage_triangle += 3;
    }
}

template <typename T>
static void extrait_attribut_primitive(JJL::PrimitiveCourbe courbe,
                                       AccesseuseAttribut &convertisseuse_valeur,
                                       int64_t i,
                                       dls::tableau<T> &données_sortie,
                                       int64_t &décalage_segment)
{
    auto valeur = convertisseuse_valeur.donne_valeur_pour_index<T>(i);

    for (int64_t ip = 0; ip < courbe.nombre_de_sommets() - 1; ++ip) {
        données_sortie[décalage_segment] = valeur;
        données_sortie[décalage_segment + 1] = valeur;
        décalage_segment += 2;
    }
}

static void extrait_normal_polygone(JJL::Corps corps,
                                    JJL::PrimitivePolygone polygone,
                                    dls::tableau<dls::math::vec3f> &normaux,
                                    int64_t &décalage_triangle)
{
    auto normal = polygone.calcule_normal(corps);

    for (int64_t ip = 2; ip < polygone.nombre_de_sommets(); ++ip) {
        normaux[décalage_triangle] = convertis_vecteur(normal);
        normaux[décalage_triangle + 1] = convertis_vecteur(normal);
        normaux[décalage_triangle + 2] = convertis_vecteur(normal);

        décalage_triangle += 3;
    }
}

static void remplis_couleur_défaut(dls::tableau<dls::phys::couleur32> &couleurs,
                                   dls::phys::couleur32 couleur)
{
    for (auto i = 0; i < couleurs.taille(); i++) {
        couleurs[i] = couleur;
    }
}

static void remplis_couleur_défaut(dls::tableau<dls::phys::couleur32> &couleurs)
{
    remplis_couleur_défaut(couleurs, dls::phys::couleur32(0.8f, 0.8f, 0.8f, 1.0f));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsPolygonesSeuls
 * \{ */

ExtractriceCorpsPolygonesSeuls::ExtractriceCorpsPolygonesSeuls(JJL::Corps corps) : m_corps(corps)
{
}

void ExtractriceCorpsPolygonesSeuls::extrait_données(DonnéesTampon &données,
                                                     dls::tableau<char> &points_utilisés)
{
    auto nombre_de_triangles = calcule_nombre_de_triangles();
    if (nombre_de_triangles == 0) {
        return;
    }

    extrait_points_triangles(données.points_polys, nombre_de_triangles, points_utilisés);
    extrait_normaux_triangles(données.normaux, nombre_de_triangles);
    extrait_couleurs_triangles(données.couleurs_polys, nombre_de_triangles);
}

int64_t ExtractriceCorpsPolygonesSeuls::calcule_nombre_de_triangles()
{
    int64_t résultat = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        résultat += polygone.nombre_de_sommets() - 2;
    }

    return résultat;
}

void ExtractriceCorpsPolygonesSeuls::extrait_points_triangles(
    dls::tableau<dls::math::vec3f> &points,
    const int64_t nombre_de_triangles,
    dls::tableau<char> &points_utilisés)
{
    points.redimensionne(nombre_de_triangles * 3);

    int64_t décalage_triangle = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        extrait_points_polygone(m_corps, polygone, points, décalage_triangle, points_utilisés);
    }
}

void ExtractriceCorpsPolygonesSeuls::extrait_normaux_triangles(
    dls::tableau<dls::math::vec3f> &normaux, const int64_t nombre_de_triangles)
{
    normaux.redimensionne(nombre_de_triangles * 3);

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::VEC3, "N");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), normaux);
        return;
    }

    attribut = donne_attribut(
        m_corps.liste_des_attributs_primitives(), JJL::TypeAttribut::VEC3, "N");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_primitive(attribut.value(), normaux);
        return;
    }

    int64_t décalage_triangle = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        extrait_normal_polygone(m_corps, polygone, normaux, décalage_triangle);
    }
}

void ExtractriceCorpsPolygonesSeuls::extrait_couleurs_triangles(
    dls::tableau<dls::phys::couleur32> &couleurs, const int64_t nombre_de_triangles)
{
    couleurs.redimensionne(nombre_de_triangles * 3);

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), couleurs);
        return;
    }

    attribut = donne_attribut(
        m_corps.liste_des_attributs_primitives(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_primitive(attribut.value(), couleurs);
        return;
    }

    remplis_couleur_défaut(couleurs);
}

template <typename T>
void ExtractriceCorpsPolygonesSeuls::remplis_tampon_depuis_attribut_point(
    JJL::Attribut attribut, dls::tableau<T> &données_sortie)
{
    int64_t décalage_triangle = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        extrait_attribut_point(polygone, convertisseuse_valeur, données_sortie, décalage_triangle);
    }
}

template <typename T>
void ExtractriceCorpsPolygonesSeuls::remplis_tampon_depuis_attribut_primitive(
    JJL::Attribut attribut, dls::tableau<T> &données_sortie)
{
    int64_t décalage_triangle = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        extrait_attribut_primitive(
            polygone, convertisseuse_valeur, i, données_sortie, décalage_triangle);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsPolygonesSeuls
 * \{ */

ExtractriceCorpsCourbesSeules::ExtractriceCorpsCourbesSeules(JJL::Corps corps) : m_corps(corps)
{
}

void ExtractriceCorpsCourbesSeules::extrait_données(DonnéesTampon &données,
                                                    dls::tableau<char> &points_utilisés)
{
    auto nombre_de_segments = calcule_nombre_de_segments();
    if (nombre_de_segments == 0) {
        return;
    }

    extrait_points_segments(données.points_segments, nombre_de_segments, points_utilisés);
    extrait_couleurs_segments(données.couleurs_segments, nombre_de_segments);
}

int64_t ExtractriceCorpsCourbesSeules::calcule_nombre_de_segments()
{
    int64_t résultat = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
        résultat += courbe.nombre_de_sommets() - 1;
    }

    return résultat;
}

void ExtractriceCorpsCourbesSeules::extrait_points_segments(dls::tableau<dls::math::vec3f> &points,
                                                            const int64_t nombre_de_segments,
                                                            dls::tableau<char> &points_utilisés)
{
    points.redimensionne(nombre_de_segments * 2);

    int64_t décalage_segment = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
        extrait_points_courbe(m_corps, courbe, points, décalage_segment, points_utilisés);
    }
}

void ExtractriceCorpsCourbesSeules::extrait_couleurs_segments(
    dls::tableau<dls::phys::couleur32> &couleurs, const int64_t nombre_de_segments)
{
    couleurs.redimensionne(nombre_de_segments * 3);

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), couleurs);
        return;
    }

    attribut = donne_attribut(
        m_corps.liste_des_attributs_primitives(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_primitive(attribut.value(), couleurs);
        return;
    }

    remplis_couleur_défaut(couleurs);
}

template <typename T>
void ExtractriceCorpsCourbesSeules::remplis_tampon_depuis_attribut_point(
    JJL::Attribut attribut, dls::tableau<T> &données_sortie)
{
    int64_t décalage_segment = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
        extrait_attribut_point(courbe, convertisseuse_valeur, données_sortie, décalage_segment);
    }
}

template <typename T>
void ExtractriceCorpsCourbesSeules::remplis_tampon_depuis_attribut_primitive(
    JJL::Attribut attribut, dls::tableau<T> &données_sortie)
{
    int64_t décalage_segment = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
        extrait_attribut_primitive(
            courbe, convertisseuse_valeur, i, données_sortie, décalage_segment);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsMixte
 * \{ */

ExtractriceCorpsMixte::ExtractriceCorpsMixte(JJL::Corps corps) : m_corps(corps)
{
}

void ExtractriceCorpsMixte::extrait_données(DonnéesTampon &données,
                                            dls::tableau<char> &points_utilisés)
{
    calcule_compte_primitives();

    extrait_points(données.points_polys, données.points_segments, points_utilisés);
    extrait_normaux(données.normaux);
    extrait_couleurs(données.couleurs_polys, données.couleurs_segments);
}

void ExtractriceCorpsMixte::calcule_compte_primitives()
{
    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);

        switch (prim.type()) {
            case JJL::TypePrimitive::POLYGONE:
            {
                auto polygone = transtype<JJL::PrimitivePolygone>(prim);
                m_nombre_de_triangles += polygone.nombre_de_sommets() - 2;
                m_nombre_de_polygones += 1;
                break;
            }
            case JJL::TypePrimitive::COURBE:
            {
                auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
                m_nombre_de_segments += courbe.nombre_de_sommets() - 1;
                m_nombre_de_courbes += 1;
                break;
            }
            case JJL::TypePrimitive::VOLUME:
            {
                /* À FAIRE : volumes. */
                break;
            }
        }
    }
}

void ExtractriceCorpsMixte::extrait_points(dls::tableau<dls::math::vec3f> &points_polys,
                                           dls::tableau<dls::math::vec3f> &points_segments,
                                           dls::tableau<char> &points_utilisés)
{
    points_polys.redimensionne(m_nombre_de_triangles * 3);
    points_segments.redimensionne(m_nombre_de_segments * 2);

    int64_t décalage_points_polys = 0;
    int64_t décalage_points_segments = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);

        switch (prim.type()) {
            case JJL::TypePrimitive::POLYGONE:
            {
                auto polygone = transtype<JJL::PrimitivePolygone>(prim);
                extrait_points_polygone(
                    m_corps, polygone, points_polys, décalage_points_polys, points_utilisés);
                break;
            }
            case JJL::TypePrimitive::COURBE:
            {
                auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
                extrait_points_courbe(
                    m_corps, courbe, points_segments, décalage_points_segments, points_utilisés);
                break;
            }
            case JJL::TypePrimitive::VOLUME:
            {
                /* Rien à faire. */
                break;
            }
        }
    }
}

void ExtractriceCorpsMixte::extrait_normaux(dls::tableau<dls::math::vec3f> &normaux)
{
    if (m_nombre_de_triangles == 0) {
        return;
    }

    normaux.redimensionne(m_nombre_de_triangles * 3);

    /* Uniquement ici pour plaire à l'IPA. */
    dls::tableau<dls::math::vec3f> normaux_segments;

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::VEC3, "N");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), normaux, normaux_segments);
        return;
    }

    attribut = donne_attribut(
        m_corps.liste_des_attributs_primitives(), JJL::TypeAttribut::VEC3, "N");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_primitive(attribut.value(), normaux, normaux_segments);
        return;
    }

    int64_t décalage_triangle = 0;

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);
        if (prim.type() != JJL::TypePrimitive::POLYGONE) {
            continue;
        }
        auto polygone = transtype<JJL::PrimitivePolygone>(prim);
        extrait_normal_polygone(m_corps, polygone, normaux, décalage_triangle);
    }
}

void ExtractriceCorpsMixte::extrait_couleurs(dls::tableau<dls::phys::couleur32> &couleurs_polys,
                                             dls::tableau<dls::phys::couleur32> &couleurs_segments)
{
    couleurs_polys.redimensionne(m_nombre_de_triangles * 3);
    couleurs_segments.redimensionne(m_nombre_de_segments * 2);

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), couleurs_polys, couleurs_segments);
        return;
    }

    attribut = donne_attribut(
        m_corps.liste_des_attributs_primitives(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_primitive(
            attribut.value(), couleurs_polys, couleurs_segments);
        return;
    }

    remplis_couleur_défaut(couleurs_polys);
    remplis_couleur_défaut(couleurs_segments);
}

template <typename T>
void ExtractriceCorpsMixte::remplis_tampon_depuis_attribut_point(JJL::Attribut attribut,
                                                                 dls::tableau<T> &données_polys,
                                                                 dls::tableau<T> &données_segments)
{
    int64_t décalage_triangle = 0;
    int64_t décalage_segment = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);

        switch (prim.type()) {
            case JJL::TypePrimitive::POLYGONE:
            {
                if (!données_polys.est_vide()) {
                    auto polygone = transtype<JJL::PrimitivePolygone>(prim);
                    extrait_attribut_point(
                        polygone, convertisseuse_valeur, données_polys, décalage_triangle);
                }
                break;
            }
            case JJL::TypePrimitive::COURBE:
            {
                if (!données_segments.est_vide()) {
                    auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
                    extrait_attribut_point(
                        courbe, convertisseuse_valeur, données_segments, décalage_segment);
                }
                break;
            }
            case JJL::TypePrimitive::VOLUME:
            {
                /* Rien à faire. */
                break;
            }
        }
    }
}

template <typename T>
void ExtractriceCorpsMixte::remplis_tampon_depuis_attribut_primitive(
    JJL::Attribut attribut, dls::tableau<T> &données_polys, dls::tableau<T> &données_segments)
{
    int64_t décalage_triangle = 0;
    int64_t décalage_segment = 0;
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    for (int64_t i = 0; i < m_corps.nombre_de_primitives(); i++) {
        auto prim = m_corps.primitive_pour_index(i);

        switch (prim.type()) {
            case JJL::TypePrimitive::POLYGONE:
            {
                if (!données_polys.est_vide()) {
                    auto polygone = transtype<JJL::PrimitivePolygone>(prim);
                    extrait_attribut_primitive(
                        polygone, convertisseuse_valeur, i, données_polys, décalage_triangle);
                }
                break;
            }
            case JJL::TypePrimitive::COURBE:
            {
                if (!données_segments.est_vide()) {
                    auto courbe = transtype<JJL::PrimitiveCourbe>(prim);
                    extrait_attribut_primitive(
                        courbe, convertisseuse_valeur, i, données_segments, décalage_segment);
                }
                break;
            }
            case JJL::TypePrimitive::VOLUME:
            {
                /* Rien à faire. */
                break;
            }
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ExtractriceCorpsPoints
 * \{ */

ExtractriceCorpsPoints::ExtractriceCorpsPoints(JJL::Corps corps) : m_corps(corps)
{
}

void ExtractriceCorpsPoints::extrait_données(DonnéesTampon &données,
                                             dls::tableau<char> &points_utilisés)
{
    m_nombre_de_points = calcule_compte_points(points_utilisés);
    extrait_points(données.points, points_utilisés);
    extrait_couleurs(données.couleurs, points_utilisés);
}

int64_t ExtractriceCorpsPoints::calcule_compte_points(dls::tableau<char> &points_utilisés)
{
    if (m_corps.nombre_de_primitives() == 0) {
        return m_corps.nombre_de_points();
    }

    int64_t résultat = 0;

    for (auto i : points_utilisés) {
        résultat += bool(i);
    }

    /* Ne prenons en compte que les points inutilisés. */
    return m_corps.nombre_de_points() - résultat;
}

void ExtractriceCorpsPoints::extrait_points(dls::tableau<dls::math::vec3f> &points,
                                            dls::tableau<char> &points_utilisés)
{
    points.redimensionne(m_nombre_de_points);

    if (m_nombre_de_points == m_corps.nombre_de_points()) {
        /* Copie tous les points. */
        for (auto i = 0; i < m_corps.nombre_de_points(); i++) {
            points[i] = convertis_point(m_corps.donne_point_local(i));
        }
    }
    else {
        /* Ne copie que les points inutilisés. */
        int64_t décalage_point = 0;

        for (auto i = 0; i < m_corps.nombre_de_points(); i++) {
            if (points_utilisés[i]) {
                continue;
            }
            points[décalage_point++] = convertis_point(m_corps.donne_point_local(i));
        }
    }
}

void ExtractriceCorpsPoints::extrait_couleurs(dls::tableau<dls::phys::couleur32> &couleurs,
                                              dls::tableau<char> &points_utilisés)
{
    couleurs.redimensionne(m_nombre_de_points);

    auto attribut = donne_attribut(
        m_corps.liste_des_attributs_points(), JJL::TypeAttribut::COULEUR, "C");
    if (attribut.has_value()) {
        remplis_tampon_depuis_attribut_point(attribut.value(), couleurs, points_utilisés);
        return;
    }

    remplis_couleur_défaut(couleurs, dls::phys::couleur32(0.0f, 0.0f, 0.0f, 1.0f));
}

template <typename T>
void ExtractriceCorpsPoints::remplis_tampon_depuis_attribut_point(
    JJL::Attribut attribut, dls::tableau<T> &données, dls::tableau<char> &points_utilisés)
{
    auto convertisseuse_valeur = AccesseuseAttribut(attribut);

    if (m_nombre_de_points == m_corps.nombre_de_points()) {
        /* Copie tous les points. */
        for (auto i = 0; i < m_corps.nombre_de_points(); i++) {
            données[i] = convertisseuse_valeur.donne_valeur_pour_index<T>(i);
        }
    }
    else {
        /* Ne copie que les points inutilisés. */
        int64_t décalage_point = 0;

        for (auto i = 0; i < m_corps.nombre_de_points(); i++) {
            if (points_utilisés[i]) {
                continue;
            }
            données[décalage_point++] = convertisseuse_valeur.donne_valeur_pour_index<T>(i);
        }
    }
}

/** \} */
