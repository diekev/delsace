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

#include "rendu_corps.h"

#include <numeric>

#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/texture/texture.h"

#include "biblinternes/structures/tableau.hh"

#include "corps/attribut.h"
#include "corps/corps.h"
#include "corps/sphere.hh"
#include "corps/volume.hh"

#include "coeur/conversion_types.hh"
#include "coeur/jorjala.hh"

#include "wolika/grille_dense.hh"
#include "wolika/grille_eparse.hh"
#include "wolika/iteration.hh"

#include "extraction_données_corps.hh"

/* ************************************************************************** */

static std::unique_ptr<TamponRendu> cree_tampon_surface(bool possede_uvs, bool instances)
{
    auto nom_fichier = (instances) ? "nuanceurs/diffus_instances.vert" : "nuanceurs/diffus.vert";
    auto source = crée_sources_glsl_depuis_fichier(nom_fichier, "nuanceurs/diffus.frag");
    if (!source.has_value()) {
        std::cerr << __func__ << " erreur : les sources sont invalides !\n";
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(source.value());

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("possede_uvs", possede_uvs);
    programme->desactive();

    return tampon;
}

/* ************************************************************************** */

static dls::math::vec3f points_cercle_XZ[32] = {
    dls::math::vec3f(0.000000f, 0.000000f, 1.000000f),
    dls::math::vec3f(-0.195090f, 0.000000f, 0.980785f),
    dls::math::vec3f(-0.382683f, 0.000000f, 0.92388f),
    dls::math::vec3f(-0.555570f, 0.000000f, 0.83147f),
    dls::math::vec3f(-0.707107f, 0.000000f, 0.707107f),
    dls::math::vec3f(-0.831470f, 0.000000f, 0.55557f),
    dls::math::vec3f(-0.923880f, 0.000000f, 0.382683f),
    dls::math::vec3f(-0.980785f, 0.000000f, 0.19509f),
    dls::math::vec3f(-1.000000f, 0.000000f, 0.000000f),
    dls::math::vec3f(-0.980785f, 0.000000f, -0.19509f),
    dls::math::vec3f(-0.923880f, 0.000000f, -0.382683f),
    dls::math::vec3f(-0.831470f, 0.000000f, -0.55557f),
    dls::math::vec3f(-0.707107f, 0.000000f, -0.707107f),
    dls::math::vec3f(-0.555570f, 0.000000f, -0.83147f),
    dls::math::vec3f(-0.382683f, 0.000000f, -0.92388f),
    dls::math::vec3f(-0.195090f, 0.000000f, -0.980785f),
    dls::math::vec3f(0.000000f, 0.000000f, -1.000000f),
    dls::math::vec3f(0.195091f, 0.000000f, -0.980785f),
    dls::math::vec3f(0.382684f, 0.000000f, -0.923879f),
    dls::math::vec3f(0.555571f, 0.000000f, -0.831469f),
    dls::math::vec3f(0.707107f, 0.000000f, -0.707106f),
    dls::math::vec3f(0.831470f, 0.000000f, -0.55557f),
    dls::math::vec3f(0.923880f, 0.000000f, -0.382683f),
    dls::math::vec3f(0.980785f, 0.000000f, -0.195089f),
    dls::math::vec3f(1.000000f, 0.000000f, 0.000000f),
    dls::math::vec3f(0.980785f, 0.000000f, 0.195091f),
    dls::math::vec3f(0.923879f, 0.000000f, 0.382684f),
    dls::math::vec3f(0.831469f, 0.000000f, 0.555571f),
    dls::math::vec3f(0.707106f, 0.000000f, 0.707108f),
    dls::math::vec3f(0.555569f, 0.000000f, 0.831470f),
    dls::math::vec3f(0.382682f, 0.000000f, 0.923880f),
    dls::math::vec3f(0.195089f, 0.000000f, 0.980786f),
};
static dls::math::vec3f points_cercle_XY[32] = {
    dls::math::vec3f(0.000000f, 1.000000f, 0.000000f),
    dls::math::vec3f(-0.195090f, 0.980785f, 0.000000f),
    dls::math::vec3f(-0.382683f, 0.923880f, 0.000000f),
    dls::math::vec3f(-0.555570f, 0.831470f, 0.000000f),
    dls::math::vec3f(-0.707107f, 0.707107f, 0.000000f),
    dls::math::vec3f(-0.831470f, 0.555570f, 0.000000f),
    dls::math::vec3f(-0.923880f, 0.382683f, 0.000000f),
    dls::math::vec3f(-0.980785f, 0.195090f, 0.000000f),
    dls::math::vec3f(-1.000000f, 0.000000f, 0.000000f),
    dls::math::vec3f(-0.980785f, -0.195090f, 0.000000f),
    dls::math::vec3f(-0.923880f, -0.382683f, 0.000000f),
    dls::math::vec3f(-0.831470f, -0.555570f, 0.000000f),
    dls::math::vec3f(-0.707107f, -0.707107f, 0.000000f),
    dls::math::vec3f(-0.555570f, -0.831470f, 0.000000f),
    dls::math::vec3f(-0.382683f, -0.923880f, 0.000000f),
    dls::math::vec3f(-0.195090f, -0.980785f, 0.000000f),
    dls::math::vec3f(0.000000f, -1.000000f, 0.000000f),
    dls::math::vec3f(0.195091f, -0.980785f, 0.000000f),
    dls::math::vec3f(0.382684f, -0.923879f, 0.000000f),
    dls::math::vec3f(0.555571f, -0.831469f, 0.000000f),
    dls::math::vec3f(0.707107f, -0.707106f, 0.000000f),
    dls::math::vec3f(0.831470f, -0.555570f, 0.000000f),
    dls::math::vec3f(0.923880f, -0.382683f, 0.000000f),
    dls::math::vec3f(0.980785f, -0.195089f, 0.000000f),
    dls::math::vec3f(1.000000f, 0.000000f, 0.000000f),
    dls::math::vec3f(0.980785f, 0.195091f, 0.000000f),
    dls::math::vec3f(0.923879f, 0.382684f, 0.000000f),
    dls::math::vec3f(0.831469f, 0.555571f, 0.000000f),
    dls::math::vec3f(0.707106f, 0.707108f, 0.000000f),
    dls::math::vec3f(0.555569f, 0.831470f, 0.000000f),
    dls::math::vec3f(0.382682f, 0.923880f, 0.000000f),
    dls::math::vec3f(0.195089f, 0.980786f, 0.000000f),
};
static dls::math::vec3f points_cercle_YZ[32] = {
    dls::math::vec3f(0.000000f, 0.000000f, 1.000000f),
    dls::math::vec3f(0.000000f, -0.195090f, 0.980785f),
    dls::math::vec3f(0.000000f, -0.382683f, 0.923880f),
    dls::math::vec3f(0.000000f, -0.555570f, 0.831470f),
    dls::math::vec3f(0.000000f, -0.707107f, 0.707107f),
    dls::math::vec3f(0.000000f, -0.831470f, 0.555570f),
    dls::math::vec3f(0.000000f, -0.923880f, 0.382683f),
    dls::math::vec3f(0.000000f, -0.980785f, 0.195090f),
    dls::math::vec3f(0.000000f, -1.000000f, 0.000000f),
    dls::math::vec3f(0.000000f, -0.980785f, -0.195090f),
    dls::math::vec3f(0.000000f, -0.923880f, -0.382683f),
    dls::math::vec3f(0.000000f, -0.831470f, -0.555570f),
    dls::math::vec3f(0.000000f, -0.707107f, -0.707107f),
    dls::math::vec3f(0.000000f, -0.555570f, -0.831470f),
    dls::math::vec3f(0.000000f, -0.382683f, -0.923880f),
    dls::math::vec3f(0.000000f, -0.195090f, -0.980785f),
    dls::math::vec3f(0.000000f, 0.000000f, -1.000000f),
    dls::math::vec3f(0.000000f, 0.195091f, -0.980785f),
    dls::math::vec3f(0.000000f, 0.382684f, -0.923879f),
    dls::math::vec3f(0.000000f, 0.555571f, -0.831469f),
    dls::math::vec3f(0.000000f, 0.707107f, -0.707106f),
    dls::math::vec3f(0.000000f, 0.831470f, -0.555570f),
    dls::math::vec3f(0.000000f, 0.923880f, -0.382683f),
    dls::math::vec3f(0.000000f, 0.980785f, -0.195089f),
    dls::math::vec3f(0.000000f, 1.000000f, 0.000000f),
    dls::math::vec3f(0.000000f, 0.980785f, 0.195091f),
    dls::math::vec3f(0.000000f, 0.923879f, 0.382684f),
    dls::math::vec3f(0.000000f, 0.831469f, 0.555571f),
    dls::math::vec3f(0.000000f, 0.707106f, 0.707108f),
    dls::math::vec3f(0.000000f, 0.555569f, 0.831470f),
    dls::math::vec3f(0.000000f, 0.382682f, 0.923880f),
    dls::math::vec3f(0.000000f, 0.195089f, 0.980786f),
};

static void ajoute_primitive_sphere(Sphere *sphere,
                                    AccesseusePointLecture const &liste_points,
                                    Attribut const *attr_couleurs,
                                    dls::tableau<dls::math::vec3f> &points,
                                    dls::tableau<dls::math::vec3f> &couleurs)
{
    auto pos_sphere = liste_points.point_local(sphere->idx_point);

    for (auto i = 0; i < 32; ++i) {
        points.ajoute(points_cercle_XZ[i] * sphere->rayon + pos_sphere);
        points.ajoute(points_cercle_XZ[(i + 1) % 32] * sphere->rayon + pos_sphere);
    }

    for (auto i = 0; i < 32; ++i) {
        points.ajoute(points_cercle_XY[i] * sphere->rayon + pos_sphere);
        points.ajoute(points_cercle_XY[(i + 1) % 32] * sphere->rayon + pos_sphere);
    }

    for (auto i = 0; i < 32; ++i) {
        points.ajoute(points_cercle_YZ[i] * sphere->rayon + pos_sphere);
        points.ajoute(points_cercle_YZ[(i + 1) % 32] * sphere->rayon + pos_sphere);
    }

    auto couleur = dls::math::vec3f(0.8f);

    if (attr_couleurs) {
        if (attr_couleurs->portee == portee_attr::POINT) {
            extrait(attr_couleurs->r32(sphere->idx_point), couleur);
        }
        else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
            extrait(attr_couleurs->r32(sphere->index), couleur);
        }
        else if (attr_couleurs->portee == portee_attr::CORPS) {
            extrait(attr_couleurs->r32(0), couleur);
        }
    }

    auto idx = couleurs.taille();
    couleurs.redimensionne(idx + 32 * 3 * 2);

    for (auto i = 0; i < 32 * 3 * 2; ++i) {
        couleurs[idx + i] = couleur;
    }
}

static std::unique_ptr<TamponRendu> cree_tampon_segments(bool instances)
{
    auto nom_fichier = (instances) ? "nuanceurs/simple_instances.vert" : "nuanceurs/simple.vert";
    auto source = crée_sources_glsl_depuis_fichier(nom_fichier, "nuanceurs/simple.frag");
    if (!source.has_value()) {
        std::cerr << __func__ << " erreur : les sources sont invalides !\n";
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(source.value());

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);
    parametres_dessin.taille_ligne(1.0);

    tampon->parametres_dessin(parametres_dessin);

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
    programme->desactive();

    return tampon;
}

/* ************************************************************************** */

static auto slice(dls::math::vec3f const &view_dir,
                  size_t m_axis,
                  TamponRendu *m_renderbuffer,
                  dls::math::vec3f const &m_min,
                  dls::math::vec3f const &m_max)
{
    auto axis = axe_dominant_abs(view_dir);

    if (m_axis == axis) {
        return;
    }

    auto m_dimensions = m_max - m_min;
    auto m_num_slices = 256l;
    auto m_elements = m_num_slices * 6;

    m_axis = axis;
    auto depth = m_min[m_axis];
    auto slice_size = m_dimensions[m_axis] / static_cast<float>(m_num_slices);

    /* always process slices in back to front order! */
    if (view_dir[m_axis] > 0.0f) {
        depth = m_max[m_axis];
        slice_size = -slice_size;
    }

    const dls::math::vec3f vertices[3][4] = {{dls::math::vec3f(0.0f, m_min[1], m_min[2]),
                                              dls::math::vec3f(0.0f, m_max[1], m_min[2]),
                                              dls::math::vec3f(0.0f, m_max[1], m_max[2]),
                                              dls::math::vec3f(0.0f, m_min[1], m_max[2])},
                                             {dls::math::vec3f(m_min[0], 0.0f, m_min[2]),
                                              dls::math::vec3f(m_min[0], 0.0f, m_max[2]),
                                              dls::math::vec3f(m_max[0], 0.0f, m_max[2]),
                                              dls::math::vec3f(m_max[0], 0.0f, m_min[2])},
                                             {dls::math::vec3f(m_min[0], m_min[1], 0.0f),
                                              dls::math::vec3f(m_min[0], m_max[1], 0.0f),
                                              dls::math::vec3f(m_max[0], m_max[1], 0.0f),
                                              dls::math::vec3f(m_max[0], m_min[1], 0.0f)}};

    dls::tableau<GLuint> indices(m_elements);
    unsigned idx = 0;
    auto idx_count = 0;

    dls::tableau<dls::math::vec3f> points;
    points.reserve(m_num_slices * 4);

    for (auto slice(0l); slice < m_num_slices; slice++) {
        dls::math::vec3f v0 = vertices[m_axis][0];
        dls::math::vec3f v1 = vertices[m_axis][1];
        dls::math::vec3f v2 = vertices[m_axis][2];
        dls::math::vec3f v3 = vertices[m_axis][3];

        v0[m_axis] = depth;
        v1[m_axis] = depth;
        v2[m_axis] = depth;
        v3[m_axis] = depth;

        points.ajoute(v0);  //  * glm::mat3(m_inv_matrix)
        points.ajoute(v1);
        points.ajoute(v2);
        points.ajoute(v3);

        indices[idx_count++] = idx + 0;
        indices[idx_count++] = idx + 1;
        indices[idx_count++] = idx + 2;
        indices[idx_count++] = idx + 0;
        indices[idx_count++] = idx + 2;
        indices[idx_count++] = idx + 3;

        depth += slice_size;
        idx += 4;
    }

    remplis_tampon_principal(m_renderbuffer, "sommets", points, indices);
}

static std::unique_ptr<TamponRendu> cree_tampon_volume(Volume *volume,
                                                       dls::math::vec3f const &view_dir)
{
    auto source = crée_sources_glsl_depuis_fichier("nuanceurs/volume.vert",
                                                   "nuanceurs/volume.frag");
    if (!source.has_value()) {
        std::cerr << __func__ << " erreur : les sources sont invalides !\n";
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(source.value());

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);
    parametres_dessin.taille_ligne(1.0);

    tampon->parametres_dessin(parametres_dessin);

    tampon->ajoute_texture_3d();
    auto texture = tampon->texture_3d();

    auto etendue = limites3f{};
    auto resolution = dls::math::vec3i{};
    auto voxels = dls::tableau<float>{};
    auto ptr_donnees = static_cast<const void *>(nullptr);

    auto grille = volume->grille;

    if (!grille->est_eparse()) {
        etendue = grille->desc().etendue;
        resolution = grille->desc().resolution;

        if (grille->desc().type_donnees == wlk::type_grille::R32) {
            auto grille_scalaire = dynamic_cast<wlk::grille_dense_3d<float> *>(grille);
            ptr_donnees = grille_scalaire->donnees();
        }
    }
    else {
        auto grille_eprs = dynamic_cast<wlk::grille_eparse<float> *>(grille);
        auto min_grille = dls::math::vec3i{100000};
        auto max_grille = dls::math::vec3i{-100000};

        if (grille_eprs->nombre_tuile() == 0) {
            /* la grille est vide */
            min_grille = dls::math::vec3i{0};
            max_grille = dls::math::vec3i{0};
        }
        else {
            wlk::pour_chaque_tuile(*grille_eprs, [&](wlk::tuile_scalaire<float> *tuile) {
                extrait_min_max(tuile->min, min_grille, max_grille);
                extrait_min_max(tuile->max, min_grille, max_grille);
            });
        }

        resolution = max_grille - min_grille;
        etendue.min = grille->index_vers_monde(min_grille);
        etendue.max = grille->index_vers_monde(max_grille);

        auto nombre_voxels = resolution.x * resolution.y * resolution.z;
        voxels.redimensionne(nombre_voxels);

        auto index_voxel = 0l;
#if 0
		for (auto z = 0; z < resolution.z; ++z) {
			for (auto y = 0; y < resolution.y; ++y) {
				for (auto x = 0; x < resolution.x; ++x, ++index_voxel) {
					voxels[index_voxel] = grille->valeur(x, y, z);
				}
			}
		}
#else
        wlk::pour_chaque_tuile_parallele(*grille_eprs, [&](wlk::tuile_scalaire<float> *tuile) {
            auto index_tuile = 0;

            for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
                for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
                    for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
                        /* décale par rapport à min_grille au cas où le minimum
                         * n'est pas zéro */
                        auto pos_tuile = tuile->min - min_grille;
                        pos_tuile.x += i;
                        pos_tuile.y += j;
                        pos_tuile.z += k;

                        index_voxel = pos_tuile.x +
                                      (pos_tuile.y + pos_tuile.z * resolution.y) * resolution.x;

                        voxels[index_voxel] = tuile->donnees[index_tuile];
                    }
                }
            }
        });
#endif
        ptr_donnees = voxels.donnees();
    }

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("volume", texture->code_attache());
    programme->uniforme("offset", etendue.min.x, etendue.min.y, etendue.min.z);
    programme->uniforme("dimension", etendue.taille().x, etendue.taille().y, etendue.taille().z);
    programme->desactive();

    /* crée vertices */
    slice(view_dir, -1ul, tampon.get(), etendue.min, etendue.max);

    /* crée texture 3d */

    texture->attache();
    texture->type(GL_FLOAT, GL_RED, GL_RED);
    texture->filtre_min_mag(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    texture->enveloppe(GL_CLAMP_TO_BORDER);

    int res[3] = {resolution[0], resolution[1], resolution[2]};

    texture->remplie(ptr_donnees, res);
    texture->genere_mip_map(0, 4);
    texture->detache();

    return tampon;
}

/* ************************************************************************** */

RenduCorps::RenduCorps(JJL::Corps &corps) : m_corps(corps)
{
}

void RenduCorps::initialise(ContexteRendu const &contexte,
                            dls::tableau<dls::math::mat4x4f> &matrices)
{
    auto const est_instance = matrices.taille() != 0;

    auto const nombre_de_points = m_corps.nombre_de_points();
    auto const nombre_de_prims = m_corps.nombre_de_primitives();

    if (nombre_de_points == 0l && nombre_de_prims == 0l) {
        return;
    }

    m_stats.nombre_points = nombre_de_points;

    dls::tableau<char> point_utilise(nombre_de_points, 0);

    extrait_données_primitives(nombre_de_prims, est_instance, point_utilise);
    extrait_données_points(nombre_de_prims, est_instance, point_utilise);

    if (!est_instance) {
        return;
    }

    auto parametres_tampon_instance = ParametresTampon{};
    parametres_tampon_instance.attribut = "matrices_instances";
    parametres_tampon_instance.dimension_attribut = 4;
    parametres_tampon_instance.pointeur_donnees_extra = matrices.donnees();
    parametres_tampon_instance.taille_octet_donnees_extra = static_cast<size_t>(
                                                                matrices.taille()) *
                                                            sizeof(dls::math::mat4x4f);
    parametres_tampon_instance.nombre_instances = static_cast<size_t>(matrices.taille());

    if (m_tampon_segments) {
        m_tampon_segments->remplie_tampon_matrices_instance(parametres_tampon_instance);
    }

    if (m_tampon_polygones) {
        m_tampon_polygones->remplie_tampon_matrices_instance(parametres_tampon_instance);
    }

    if (m_tampon_points) {
        m_tampon_points->remplie_tampon_matrices_instance(parametres_tampon_instance);
    }
}

void RenduCorps::dessine(StatistiquesRendu &stats, ContexteRendu const &contexte)
{
    stats.nombre_points += m_stats.nombre_points;
    stats.nombre_polylignes += m_stats.nombre_polylignes;
    stats.nombre_polygones += m_stats.nombre_polygones;
    stats.nombre_volumes += m_stats.nombre_volumes;

    if (m_tampon_points != nullptr) {
        m_tampon_points->dessine(contexte);
    }

    if (m_tampon_polygones != nullptr) {
        auto programme = m_tampon_polygones->programme();
        programme->active();
        programme->uniforme("methode", -1);
        programme->desactive();

        m_tampon_polygones->dessine(contexte);
    }

    if (m_tampon_segments != nullptr) {
        ParametresDessin parametres_dessin;
        parametres_dessin.taille_point(2.0);
        parametres_dessin.type_dessin(GL_POINTS);
        m_tampon_segments->parametres_dessin(parametres_dessin);

        m_tampon_segments->dessine(contexte);

        parametres_dessin.taille_point(1.0);
        parametres_dessin.type_dessin(GL_LINES);
        m_tampon_segments->parametres_dessin(parametres_dessin);

        m_tampon_segments->dessine(contexte);
    }

    if (m_tampon_volume != nullptr) {
        ParametresDessin parametres_dessin;
        parametres_dessin.taille_point(2.0);
        parametres_dessin.type_dessin(GL_TRIANGLES);
        m_tampon_volume->parametres_dessin(parametres_dessin);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_tampon_volume->dessine(contexte);
        glDisable(GL_BLEND);
    }
}

void RenduCorps::extrait_données_primitives(int64_t nombre_de_prims,
                                            bool est_instance,
                                            dls::tableau<char> &points_utilisés)
{
    if (nombre_de_prims == 0l) {
        return;
    }

    auto données = DonnéesTampon();

    if (m_corps.ne_contient_que_des_primitives_de_type(JJL::TypePrimitive::POLYGONE)) {
        auto extractrice = ExtractriceCorpsPolygonesSeuls(m_corps);
        extractrice.extrait_données(données, points_utilisés);
        m_stats.nombre_polygones = extractrice.donne_nombre_de_polygones();
    }
    else if (m_corps.ne_contient_que_des_primitives_de_type(JJL::TypePrimitive::COURBE)) {
        auto extractrice = ExtractriceCorpsCourbesSeules(m_corps);
        extractrice.extrait_données(données, points_utilisés);
        m_stats.nombre_polylignes = extractrice.donne_nombre_de_courbes();
    }
    else if (m_corps.ne_contient_que_des_primitives_de_type(JJL::TypePrimitive::VOLUME)) {
        /* À FAIRE : volumes. */
    }
    else {
        auto extractrice = ExtractriceCorpsMixte(m_corps);
        extractrice.extrait_données(données, points_utilisés);
        m_stats.nombre_polygones = extractrice.donne_nombre_de_polygones();
        m_stats.nombre_polylignes = extractrice.donne_nombre_de_courbes();
    }

    dls::tableau<dls::math::vec3f> &points_polys = données.points_polys;
    dls::tableau<dls::math::vec3f> &points_segment = données.points_segments;
    dls::tableau<dls::math::vec3f> &normaux = données.normaux;
    dls::tableau<dls::phys::couleur32> &couleurs_polys = données.couleurs_polys;
    dls::tableau<dls::phys::couleur32> &couleurs_segment = données.couleurs_segments;

    if (points_polys.taille() != 0) {
        m_tampon_polygones = cree_tampon_surface(false, est_instance);
        remplis_tampon_principal(m_tampon_polygones.get(), "sommets", points_polys);
        remplis_tampon_extra(m_tampon_polygones.get(), "normaux", normaux);
        remplis_tampon_extra(m_tampon_polygones.get(), "couleurs", couleurs_polys);
    }

    if (points_segment.taille() != 0) {
        m_tampon_segments = cree_tampon_segments(est_instance);
        remplis_tampon_principal(m_tampon_segments.get(), "sommets", points_segment);
        remplis_tampon_extra(m_tampon_segments.get(), "couleur_sommet", couleurs_segment);
        auto programme = m_tampon_segments->programme();
        programme->active();
        programme->uniforme("possede_couleur_sommet", 1);
        programme->desactive();
    }
}

void RenduCorps::extrait_données_points(int64_t nombre_de_prims,
                                        bool est_instance,
                                        dls::tableau<char> &points_utilisés)
{
    DonnéesTampon données;

    ExtractriceCorpsPoints extractrice(m_corps);
    extractrice.extrait_données(données, points_utilisés);

    if (données.points.taille() == 0) {
        return;
    }

    m_tampon_points = cree_tampon_segments(est_instance);
    remplis_tampon_principal(m_tampon_points.get(), "sommets", données.points);
    remplis_tampon_extra(m_tampon_points.get(), "couleur_sommet", données.couleurs);

    ParametresDessin parametres_dessin;
    parametres_dessin.taille_point(2.0);
    parametres_dessin.type_dessin(GL_POINTS);
    m_tampon_points->parametres_dessin(parametres_dessin);

    auto programme = m_tampon_points->programme();
    programme->active();
    programme->uniforme("possede_couleur_sommet", 1);
    programme->desactive();
}
