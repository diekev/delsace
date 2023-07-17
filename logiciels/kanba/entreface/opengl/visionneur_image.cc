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

#include "visionneur_image.h"

#include "biblinternes/ego/outils.h"
#include "biblinternes/image/pixel.h"
#include "biblinternes/opengl/contexte_rendu.h"

#include "../editeur_canevas.h"
#include "coeur/kanba.h"
#include "coeur/maillage.h"

/* ************************************************************************** */

static const char *source_vertex = "#version 330 core\n"
                                   "layout(location = 0) in vec2 vertex;\n"
                                   "smooth out vec2 UV;\n"
                                   "void main()\n"
                                   "{\n"
                                   "	gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);\n"
                                   "	UV = vertex;\n"
                                   "}\n";

static const char *source_fragment = "#version 330 core\n"
                                     "layout (location = 0) out vec4 fragment_color;\n"
                                     "smooth in vec2 UV;\n"
                                     "uniform sampler2D image;\n"
                                     " void main()\n"
                                     "{\n"
                                     "	vec2 flipped = vec2(UV.x, 1.0f - UV.y);\n"
                                     "	fragment_color = texture(image, flipped);\n"
                                     "	// fragment_color = vec4(flipped, 1.0, 1.0);\n"
                                     "}\n";

static std::unique_ptr<TamponRendu> cree_tampon_image()
{
    auto sources = crée_sources_glsl_depuis_texte(source_vertex, source_fragment);
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

static void generate_texture(dls::ego::Texture2D *texture, const float *data, GLint size[2])
{
    texture->deloge(true);
    dls::ego::util::GPU_check_errors("Erreur lors de la suppression de la texture");
    texture->attache();
    dls::ego::util::GPU_check_errors("Erreur lors de l'attache de la texture");
    texture->type(GL_FLOAT, GL_RGBA, GL_RGBA);
    dls::ego::util::GPU_check_errors("Erreur lors du typage de la texture");
    texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
    dls::ego::util::GPU_check_errors("Erreur lors du filtrage de la texture");
    texture->enveloppe(GL_CLAMP);
    dls::ego::util::GPU_check_errors("Erreur lors du wrapping de la texture");
    texture->remplie(data, size);
    dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
    texture->detache();
    dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}

/* ************************************************************************** */

VisionneurImage::VisionneurImage(VueCanevas2D *parent, KNB::Kanba *kanba)
    : m_parent(parent), m_kanba(kanba)
{
}

void VisionneurImage::initialise()
{
    m_tampon = cree_tampon_image();
    charge_image();
}

void VisionneurImage::peint_opengl()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_BLEND);
    if (m_tampon) {
        m_tampon->dessine({});
    }
    glDisable(GL_BLEND);
    dls::ego::util::GPU_check_errors("Erreur lors du dessin de la texture");
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
    glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image()
{
    if (!m_tampon) {
        return;
    }

    auto maillage = m_kanba->maillage;
    if (maillage == nullptr) {
        return;
    }

    auto canal_fusionné = maillage->donne_canal_fusionné();

    GLint size[] = {GLint(canal_fusionné.largeur), GLint(canal_fusionné.hauteur)};

    if (m_largeur != size[0] || m_hauteur != size[1]) {
        m_hauteur = size[0];
        m_largeur = size[1];
    }

    generate_texture(
        m_tampon->texture(), reinterpret_cast<float *>(canal_fusionné.tampon_diffusion), size);

    dls::ego::util::GPU_check_errors("Unable to create image texture");
}
