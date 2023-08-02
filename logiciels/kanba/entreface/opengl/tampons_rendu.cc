/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "tampons_rendu.hh"

#include "biblinternes/opengl/atlas_texture.h"

/* ------------------------------------------------------------------------- */
/** \name Tampon Rendu pour une image.
 * \{ */

static const char *source_vertex_image = "#version 330 core\n"
                                         "layout(location = 0) in vec2 vertex;\n"
                                         "smooth out vec2 UV;\n"
                                         "void main()\n"
                                         "{\n"
                                         "	gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);\n"
                                         "	UV = vertex;\n"
                                         "}\n";

static const char *source_fragment_image = "#version 330 core\n"
                                           "layout (location = 0) out vec4 fragment_color;\n"
                                           "smooth in vec2 UV;\n"
                                           "uniform sampler2D image;\n"
                                           " void main()\n"
                                           "{\n"
                                           "	vec2 flipped = vec2(UV.x, 1.0f - UV.y);\n"
                                           "	fragment_color = texture(image, flipped);\n"
                                           "	// fragment_color = vec4(flipped, 1.0, 1.0);\n"
                                           "}\n";

std::unique_ptr<TamponRendu> crée_tampon_image()
{
    auto sources = crée_sources_glsl_depuis_texte(source_vertex_image, source_fragment_image);
    if (!sources.has_value()) {
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    tampon->ajoute_texture();
    auto texture = tampon->texture();

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("image", texture->code_attache());
    programme->desactive();

    float sommets[8] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    unsigned int index[6] = {0, 1, 2, 0, 2, 3};

    auto parametres_tampon = ParametresTampon();
    parametres_tampon.attribut = "vertex";
    parametres_tampon.dimension_attribut = 2;
    parametres_tampon.pointeur_sommets = sommets;
    parametres_tampon.taille_octet_sommets = sizeof(float) * 8;
    parametres_tampon.pointeur_index = index;
    parametres_tampon.taille_octet_index = sizeof(unsigned int) * 6;
    parametres_tampon.elements = 6;

    tampon->remplie_tampon(parametres_tampon);

    return tampon;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Tampon de rendu pour le nuanceur « simple ».
 * \{ */

std::unique_ptr<TamponRendu> crée_tampon_nuanceur_simple(dls::phys::couleur32 couleur)
{
    auto sources = crée_sources_glsl_depuis_fichier("nuanceurs/simple.vert",
                                                    "nuanceurs/simple.frag");
    if (!sources.has_value()) {
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("couleur", couleur.r, couleur.v, couleur.b, couleur.a);
    programme->desactive();

    return tampon;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Tampon de rendu pour une texture diffuse avec atlas.
 * \{ */

std::unique_ptr<TamponRendu> crée_tampon_texture_atlas_diffus()
{
    auto sources = crée_sources_glsl_depuis_fichier("nuanceurs/diffus.vert",
                                                    "nuanceurs/diffus.frag");

    if (!sources.has_value()) {
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    tampon->ajoute_atlas();
    auto texture = tampon->atlas();

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("couleur", 1.0f, 1.0f, 1.0f, 1.0f);
    programme->uniforme("taille_u", 1.0f);
    programme->uniforme("taille_v", 1.0f);
    programme->uniforme("texture_poly", texture->nombre());
    programme->desactive();

    return tampon;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Tampon de rendu pour une texture diffuse avec texture 3D.
 * \{ */

std::unique_ptr<TamponRendu> crée_tampon_texture_bombée_diffus()
{
    auto sources = crée_sources_glsl_depuis_fichier("nuanceurs/texture_bombee.vert",
                                                    "nuanceurs/texture_bombee.frag");

    if (!sources.has_value()) {
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    tampon->ajoute_texture();
    auto texture = tampon->texture();

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("couleur", 1.0f, 1.0f, 1.0f, 1.0f);
    programme->uniforme("taille_u", 1.0f);
    programme->uniforme("taille_v", 1.0f);
    programme->uniforme("texture_poly", texture->code_attache());
    programme->desactive();

    return tampon;
}

/** \} */
