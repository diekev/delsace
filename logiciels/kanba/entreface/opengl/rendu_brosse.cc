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

#include "rendu_brosse.h"

#include "biblinternes/ego/outils.h"
#include "biblinternes/math/vecteur.hh"

#include "biblinternes/outils/constantes.h"

/* ************************************************************************** */

static const char *source_vertex =
    "#version 330 core\n"
    "layout(location = 0) in vec3 sommets;\n"
    "uniform float taille_x;\n"
    "uniform float taille_y;\n"
    "uniform float pos_x;\n"
    "uniform float pos_y;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = vec4(sommets.x * taille_x + pos_x, sommets.y * taille_y + pos_y, -1, 1.0);\n"
    "}\n";

static const char *source_fragment = "#version 330 core\n"
                                     "layout (location = 0) out vec4 couleur_fragment;\n"
                                     "uniform vec4 couleur;\n"
                                     " void main()\n"
                                     "{\n"
                                     "	couleur_fragment = couleur;\n"
                                     "}\n";

static std::unique_ptr<TamponRendu> creer_tampon()
{
    auto sources = crée_sources_glsl_depuis_texte(source_vertex, source_fragment);
    if (!sources.has_value()) {
        std::cerr << "Erreur : les sources GLSL sont invalides\n";
        return nullptr;
    }

    auto tampon = TamponRendu::crée_unique(sources.value());

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);
    parametres_dessin.taille_ligne(1.0);

    tampon->parametres_dessin(parametres_dessin);

    auto programme = tampon->programme();
    programme->active();
    programme->uniforme("couleur", 0.8f, 0.1f, 0.1f, 1.0f);
    programme->desactive();

    return tampon;
}

/* ************************************************************************** */

void RenduBrosse::initialise()
{
    if (m_tampon_contour != nullptr) {
        return;
    }

    m_tampon_contour = creer_tampon();

    auto const &points = 64l;

    dls::tableau<dls::math::vec3f> sommets(points + 1);

    for (auto i = 0l; i <= points; i++) {
        auto const angle = constantes<float>::TAU * static_cast<float>(i) /
                           static_cast<float>(points);
        auto const x = std::cos(angle);
        auto const y = std::sin(angle);
        sommets[i] = dls::math::vec3f(x, y, 0.0f);
    }

    dls::tableau<unsigned int> index;
    index.reserve(points * 2);

    for (unsigned i = 0; i < 64; ++i) {
        index.ajoute(i);
        index.ajoute(i + 1);
    }

    remplis_tampon_principal(m_tampon_contour.get(), "sommets", sommets, index);
}

void RenduBrosse::dessine(ContexteRendu const &contexte,
                          const float taille_x,
                          const float taille_y,
                          const float pos_x,
                          const float pos_y)
{
    auto programme = m_tampon_contour->programme();
    programme->active();
    programme->uniforme("taille_x", taille_x);
    programme->uniforme("taille_y", taille_y);
    programme->uniforme("pos_x", pos_x);
    programme->uniforme("pos_y", pos_y);
    programme->desactive();

    m_tampon_contour->dessine(contexte);
}
