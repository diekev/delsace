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

#include "moteur_rendu_ratissage.hh"

#include "biblinternes/opengl/contexte_rendu.h"

#include "coeur/conversion_types.hh"
#include "coeur/jorjala.hh"
#include "coeur/objet.h"

#undef RATISSAGE

#ifdef RATISSAGE
struct Image {
    float *tampon = nullptr;
    int hauteur = 0;
    int largeur = 0;
    int cannaux = 0;
    int pad = 0;
};

static auto dessine_ligne(
    Image &image, int x1, int y1, int x2, int y2, dls::math::vec4f const &couleur)
{
    auto tampon = image.tampon;

    auto dx = static_cast<float>(x2 - x1);
    auto dy = static_cast<float>(y2 - y1);
    float pas;

    if (std::abs(dx) >= std::abs(dy)) {
        pas = std::abs(dx);
    }
    else {
        pas = std::abs(dy);
    }

    dx /= pas;
    dy /= pas;
    auto x = static_cast<float>(x1);
    auto y = static_cast<float>(y1);
    auto i = 1.0f;

    // auto nombre_pixels = image.hauteur * image.largeur * image.cannaux;

    while (i <= pas) {
        auto const xi = static_cast<int>(x);
        auto const yi = static_cast<int>(y);

        if (xi >= 0 && xi < image.largeur && yi >= 0 && yi < image.hauteur) {
            auto const index = (xi + yi * image.largeur) * image.cannaux;

            //			if (index < 0 || index >= nombre_pixels) {
            //				break;
            //			}

            tampon[index + 0] = couleur.x;
            tampon[index + 1] = couleur.y;
            tampon[index + 2] = couleur.z;
            tampon[index + 3] = couleur.w;
        }

        x += dx;
        y += dy;
        i += 1.0f;
    }
}

static auto dessine_ligne(Image &image,
                          dls::math::point2f const &v0,
                          dls::math::point2f const &v1,
                          dls::math::vec4f const &couleur)
{
    dessine_ligne(image,
                  static_cast<int>(v0.x),
                  static_cast<int>(v0.y),
                  static_cast<int>(v1.x),
                  static_cast<int>(v1.y),
                  couleur);
}

// http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html

/* Remplis un triangle qui pointe vers le haut avec une base horizontale :
 *      /\
 *     /  \
 *     ----
 */
static auto remplis_triangle_bas_plat(Image &image,
                                      dls::math::vec2d const &v1,
                                      dls::math::vec2d const &v2,
                                      dls::math::vec2d const &v3,
                                      dls::math::vec4f const &couleur)
{
    auto const ld = static_cast<double>(image.largeur - 1);
    auto const hd = static_cast<double>(image.hauteur - 1);

    auto const taille_pixel = 1.0 / std::min(ld, hd);

    auto const cd1_inv = (v2.x - v1.x) / (v2.y - v1.y) * taille_pixel;
    auto const cd2_inv = (v3.x - v1.x) / (v3.y - v1.y) * taille_pixel;

    auto cur_x1 = v1.x;
    auto cur_x2 = v1.x;

    auto y1 = static_cast<int>(v1.y * hd);
    auto y2 = static_cast<int>(v2.y * hd);

    y1 = dls::math::restreint(y1, 0, image.hauteur - 1);
    y2 = dls::math::restreint(y2, 0, image.hauteur - 1);

    auto scanlineY = y1;

    while (scanlineY <= y2) {
        auto x1i = static_cast<int>(cur_x1 * ld);
        auto x2i = static_cast<int>(cur_x2 * ld);

        x1i = dls::math::restreint(x1i, 0, image.largeur - 1);
        x2i = dls::math::restreint(x2i, 0, image.largeur - 1);

        dessine_ligne(image, x1i, scanlineY, x2i, scanlineY, couleur);

        cur_x1 += cd1_inv;
        cur_x2 += cd2_inv;

        scanlineY += 1;
    }
}

// Remplis un triangle qui pointe vers le bas avec une base horizontale :
//     ----
//     \  /
//      \/
static auto remplis_triangle_haut_plat(Image &image,
                                       dls::math::vec2d const &v1,
                                       dls::math::vec2d const &v2,
                                       dls::math::vec2d const &v3,
                                       dls::math::vec4f const &couleur)
{
    auto const ld = static_cast<double>(image.largeur - 1);
    auto const hd = static_cast<double>(image.hauteur - 1);

    auto const taille_pixel = 1.0 / std::min(ld, hd);

    auto const cd1_inv = (v3.x - v1.x) / (v3.y - v1.y) * taille_pixel;
    auto const cd2_inv = (v3.x - v2.x) / (v3.y - v2.y) * taille_pixel;

    auto cur_x1 = v3.x;
    auto cur_x2 = v3.x;

    auto y1 = static_cast<int>(v1.y * hd);
    auto y3 = static_cast<int>(v3.y * hd);

    y1 = dls::math::restreint(y1, 0, image.hauteur - 1);
    y3 = dls::math::restreint(y3, 0, image.hauteur - 1);

    auto scanlineY = y3;

    while (scanlineY > y1) {
        auto x1i = static_cast<int>(cur_x1 * ld);
        auto x2i = static_cast<int>(cur_x2 * ld);

        x1i = dls::math::restreint(x1i, 0, image.largeur - 1);
        x2i = dls::math::restreint(x2i, 0, image.largeur - 1);

        dessine_ligne(image, x1i, scanlineY, x2i, scanlineY, couleur);

        cur_x1 -= cd1_inv;
        cur_x2 -= cd2_inv;
        scanlineY -= 1;
    }
}

static auto permute(dls::math::vec2d &v1, dls::math::vec2d &v2)
{
    std::swap(v1.x, v2.x);
    std::swap(v1.y, v2.y);
}

// Pour dessiner par ratissage un triangle quelconque, nous le divisons au besoin
// en deux triangles avec bases horizontales dont un pointe vers le haut et
// l'autre le bas :
//
//   /|
//  / |
// /__|
// \  |
//  \ |
//   \|
//
static auto ratisse_triangle(Image &image,
                             double x1,
                             double y1,
                             double x2,
                             double y2,
                             double x3,
                             double y3,
                             dls::math::vec4f const &couleur)
{
    auto v1 = dls::math::vec2d{x1, y1};
    auto v2 = dls::math::vec2d{x2, y2};
    auto v3 = dls::math::vec2d{x3, y3};

    while (true) {
        if ((v1.y <= v2.y) && (v2.y <= v3.y)) {
            break;
        }

        if (v2.y < v1.y) {
            permute(v1, v2);
        }

        if (v3.y < v2.y) {
            permute(v2, v3);
        }
    }

    if (v2.y == v3.y) {
        remplis_triangle_bas_plat(image, v1, v2, v3, couleur);
    }
    else if (v1.y == v2.y) {
        remplis_triangle_haut_plat(image, v1, v2, v3, couleur);
    }
    else {
        auto x4 = v1.x + ((v2.y - v1.y) / (v3.y - v1.y)) * (v3.x - v1.x);
        auto y4 = v2.y;

        auto v4 = dls::math::vec2d{x4, y4};

        remplis_triangle_bas_plat(image, v1, v2, v4, couleur);
        remplis_triangle_haut_plat(image, v2, v4, v3, couleur);
    }
}

static auto ratisse_triangle(Image &image,
                             dls::math::point2f p0,
                             dls::math::point2f p1,
                             dls::math::point2f p2,
                             dls::math::vec4f const &couleur)
{
    auto v1 = dls::math::vec2d{static_cast<double>(p0.x), static_cast<double>(p0.y)};
    auto v2 = dls::math::vec2d{static_cast<double>(p1.x), static_cast<double>(p1.y)};
    auto v3 = dls::math::vec2d{static_cast<double>(p2.x), static_cast<double>(p2.y)};

    while (true) {
        if ((v1.y <= v2.y) && (v2.y <= v3.y)) {
            break;
        }

        if (v2.y < v1.y) {
            permute(v1, v2);
        }

        if (v3.y < v2.y) {
            permute(v2, v3);
        }
    }

    v1.x /= static_cast<double>(image.largeur);
    v1.y /= static_cast<double>(image.hauteur);
    v2.x /= static_cast<double>(image.largeur);
    v2.y /= static_cast<double>(image.hauteur);
    v3.x /= static_cast<double>(image.largeur);
    v3.y /= static_cast<double>(image.hauteur);

    if (v2.y == v3.y) {
        remplis_triangle_bas_plat(image, v1, v2, v3, couleur);
    }
    else if (v1.y == v2.y) {
        remplis_triangle_haut_plat(image, v1, v2, v3, couleur);
    }
    else {
        auto x4 = v1.x + ((v2.y - v1.y) / (v3.y - v1.y)) * (v3.x - v1.x);
        auto y4 = v2.y;

        auto v4 = dls::math::vec2d{x4, y4};

        remplis_triangle_bas_plat(image, v1, v2, v4, couleur);
        remplis_triangle_haut_plat(image, v2, v4, v3, couleur);
    }
}
#endif

/* ************************************************************************** */

MoteurRenduRatissage::~MoteurRenduRatissage()
{
}

const char *MoteurRenduRatissage::id() const
{
    return "ratisseuse";
}

void MoteurRenduRatissage::calcule_rendu(
    StatistiquesRendu &stats, float *tampon, int hauteur, int largeur, bool rendu_final)
{
    auto contexte = crée_contexte_rendu();

#ifdef RATISSAGE
    /* ****************************************************************** */

    /* Met en place le contexte. */
    auto pile = PileMatrice{};
    contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

    /* couleur d'arrière plan */
    auto const arriere_plan = dls::math::vec4f(0.5f, 0.5f, 0.5f, 1.0f);

    auto const nombre_pixels = static_cast<size_t>(largeur) * static_cast<size_t>(hauteur) * 4;

    for (auto x = 0ul; x < nombre_pixels; x += 4) {
        for (auto i = 0ul; i < 4; ++i) {
            tampon[x + i] = arriere_plan[i];
        }
    }

    /* ********************* */

    auto image = Image{};
    image.tampon = tampon;
    image.hauteur = hauteur;
    image.largeur = largeur;
    image.cannaux = 4;

    /* dessine grille */
    auto const largeur_grille = 20;
    auto const hauteur_grille = 20;

    auto const moitie_largeur = static_cast<float>(largeur_grille) * 0.5f;
    auto const moitie_hauteur = static_cast<float>(hauteur_grille) * 0.5f;

    for (auto i = -moitie_hauteur; i <= moitie_hauteur; i += 1.0f) {
        auto s0 = dls::math::vec3f(i, 0.0f, -moitie_hauteur);
        auto s1 = dls::math::vec3f(i, 0.0f, moitie_hauteur);

        auto p0 = m_camera->pos_ecran(dls::math::point3f(s0));
        auto p1 = m_camera->pos_ecran(dls::math::point3f(s1));

        auto couleur = dls::math::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

        dessine_ligne(image, p0, p1, couleur);

        s0 = dls::math::vec3f(-moitie_largeur, 0.0f, i);
        s1 = dls::math::vec3f(moitie_largeur, 0.0f, i);

        p0 = m_camera->pos_ecran(dls::math::point3f(s0));
        p1 = m_camera->pos_ecran(dls::math::point3f(s1));

        couleur = dls::math::vec4f(0.0f, 0.0f, 1.0f, 1.0f);

        dessine_ligne(image, p0, p1, couleur);
    }

    if (m_delegue->scene != nullptr) {
        for (auto i = 0; i < m_delegue->nombre_objets(); ++i) {
            auto objet = m_delegue->objet(i);

            pile.ajoute(objet->transformation.matrice());

            objet->corps.accede_lecture([&pile, &contexte, this, &image](Corps const &corps) {
                pile.ajoute(corps.transformation.matrice());

                contexte.matrice_objet(math::matf_depuis_matd(pile.sommet()));

                auto liste_points = corps.points();
                auto liste_prims = corps.prims();

                for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
                    auto prim = liste_prims->prim(ip);

                    if (prim->type_prim() == type_primitive::POLYGONE) {
                        auto polygone = dynamic_cast<Polygone *>(prim);

                        auto couleur = dls::math::vec4f(1.0f, 1.0f, 1.0f, 1.0f);

                        if (polygone->type == type_polygone::FERME) {
                            for (long idx_poly = 2; idx_poly < polygone->nombre_sommets();
                                 ++idx_poly) {
                                auto p0 = liste_points->point(polygone->index_point(0));
                                auto p1 = liste_points->point(polygone->index_point(idx_poly - 1));
                                auto p2 = liste_points->point(polygone->index_point(idx_poly));

                                auto s0 = m_camera->pos_ecran(dls::math::point3f(p0));
                                auto s1 = m_camera->pos_ecran(dls::math::point3f(p1));
                                auto s2 = m_camera->pos_ecran(dls::math::point3f(p2));

                                ratisse_triangle(image, s0, s1, s2, couleur);
                            }
                        }
                        // else if (polygone->type == type_polygone::OUVERT) {
                        //	ajoute_polygone_segment(polygone, liste_points, attr_C, points_segment,
                        // couleurs_segment);
                        //}

                        // for (auto i = 0; i < polygone->nombre_sommets(); ++i) {
                        //	point_utilise[polygone->index_point(i)] = 1;
                        //}
                    }
                }

                pile.enleve_sommet();
            });

            pile.enleve_sommet();
        }
    }
#endif
}

ContexteRendu MoteurRenduRatissage::crée_contexte_rendu()
{
    m_camera.ajourne();

    auto résultat = ContexteRendu{};

    auto const &MV = convertis_matrice(m_camera.MV());
    auto const &P = convertis_matrice(m_camera.P());
    auto const &MVP = P * MV;

    résultat.vue(convertis_vecteur(m_camera.direction()));
    résultat.modele_vue(MV);
    résultat.projection(P);
    résultat.MVP(MVP);
    résultat.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
    résultat.pour_surlignage(false);

    return résultat;
}
