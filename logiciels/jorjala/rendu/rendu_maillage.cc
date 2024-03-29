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

#include "rendu_maillage.h"

#include <numeric>

#include "biblinternes/math/transformation.hh"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/lumiere.h"
#include "coeur/maillage.h"
#include "coeur/nuanceur.h"
#include "coeur/scene.h"

/* ************************************************************************** */

RenduMaillage::RenduMaillage(kdo::Maillage *maillage) : m_maillage(maillage)
{
}

RenduMaillage::~RenduMaillage()
{
    delete m_tampon_surface;
    delete m_tampon_normal;
}

void RenduMaillage::genere_tampon_surface()
{
    if (m_tampon_surface != nullptr) {
        return;
    }

    m_tampon_surface = new TamponRendu;

    m_tampon_surface->charge_source_programme(dls::ego::Nuanceur::VERTEX,
                                              dls::contenu_fichier("nuanceurs/diffus.vert"));

    m_tampon_surface->charge_source_programme(dls::ego::Nuanceur::FRAGMENT,
                                              dls::contenu_fichier("nuanceurs/diffus.frag"));

    m_tampon_surface->finalise_programme();

    ParametresProgramme parametre_programme;
    parametre_programme.ajoute_attribut("sommets");
    parametre_programme.ajoute_attribut("normal");
    parametre_programme.ajoute_uniforme("N");
    parametre_programme.ajoute_uniforme("matrice");
    parametre_programme.ajoute_uniforme("MVP");
    parametre_programme.ajoute_uniforme("couleur");

    for (size_t i = 0; i < 8; ++i) {
        parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].couleur");
        parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].position");
        parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].type");
    }

    m_tampon_surface->parametres_programme(parametre_programme);

    auto programme = m_tampon_surface->programme();
    programme->active();
    programme->uniforme("couleur", 1.0f, 1.0f, 1.0f, 1.0f);
    programme->desactive();

    auto nombre_sommets = std::distance(m_maillage->begin(), m_maillage->end());

    dls::tableau<dls::math::vec3f> sommets;
    sommets.reserve(nombre_sommets * 3);

    dls::tableau<dls::math::vec3f> normaux;
    normaux.reserve(nombre_sommets * 3);

    /* OpenGL ne travaille qu'avec des floats. */
    for (const kdo::Triangle *triangle : *m_maillage) {
        sommets.pousse(dls::math::vec3f(static_cast<float>(triangle->v0.x),
                                        static_cast<float>(triangle->v0.y),
                                        static_cast<float>(triangle->v0.z)));
        sommets.pousse(dls::math::vec3f(static_cast<float>(triangle->v1.x),
                                        static_cast<float>(triangle->v1.y),
                                        static_cast<float>(triangle->v1.z)));
        sommets.pousse(dls::math::vec3f(static_cast<float>(triangle->v2.x),
                                        static_cast<float>(triangle->v2.y),
                                        static_cast<float>(triangle->v2.z)));

        auto normal = dls::math::vec3f(static_cast<float>(triangle->normal.x),
                                       static_cast<float>(triangle->normal.y),
                                       static_cast<float>(triangle->normal.z));

        normaux.pousse(normal);
        normaux.pousse(normal);
        normaux.pousse(normal);
    }

    dls::tableau<unsigned int> indices(sommets.taille());
    std::iota(indices.debut(), indices.fin(), 0);

    ParametresTampon parametres_tampon;
    parametres_tampon.attribut = "sommets";
    parametres_tampon.dimension_attribut = 3;
    parametres_tampon.pointeur_sommets = sommets.donnees();
    parametres_tampon.taille_octet_sommets = static_cast<size_t>(sommets.taille()) *
                                             sizeof(dls::math::vec3f);
    parametres_tampon.pointeur_index = indices.donnees();
    parametres_tampon.taille_octet_index = static_cast<size_t>(indices.taille()) *
                                           sizeof(unsigned int);
    parametres_tampon.elements = static_cast<size_t>(indices.taille());

    m_tampon_surface->remplie_tampon(parametres_tampon);

    parametres_tampon.attribut = "normal";
    parametres_tampon.pointeur_donnees_extra = normaux.donnees();
    parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(normaux.taille()) *
                                                   sizeof(dls::math::vec3f);

    m_tampon_surface->remplie_tampon_extra(parametres_tampon);
}

void RenduMaillage::genere_tampon_normal()
{
    if (m_tampon_normal != nullptr) {
        return;
    }

    m_tampon_normal = new TamponRendu;

    m_tampon_normal->charge_source_programme(dls::ego::Nuanceur::VERTEX,
                                             dls::contenu_fichier("nuanceurs/simple.vert"));

    m_tampon_normal->charge_source_programme(dls::ego::Nuanceur::FRAGMENT,
                                             dls::contenu_fichier("nuanceurs/simple.frag"));

    m_tampon_normal->finalise_programme();

    ParametresProgramme parametre_programme;
    parametre_programme.ajoute_attribut("sommets");
    parametre_programme.ajoute_attribut("normal");
    parametre_programme.ajoute_uniforme("N");
    parametre_programme.ajoute_uniforme("matrice");
    parametre_programme.ajoute_uniforme("MVP");
    parametre_programme.ajoute_uniforme("couleur");

    m_tampon_normal->parametres_programme(parametre_programme);

    auto programme = m_tampon_normal->programme();
    programme->active();
    programme->uniforme("couleur", 0.5f, 1.0f, 0.5f, 1.0f);
    programme->desactive();

    auto nombre_triangle = std::distance(m_maillage->begin(), m_maillage->end());

    dls::tableau<dls::math::vec3f> sommets;
    sommets.reserve(nombre_triangle * 2);

    for (const kdo::Triangle *triangle : *m_maillage) {
        auto const &N = normalise(triangle->normal);
        auto const &V = moyenne(triangle->v0, triangle->v1, triangle->v2);

        auto const &NV = V + N;

        sommets.pousse(dls::math::vec3f(
            static_cast<float>(V.x), static_cast<float>(V.y), static_cast<float>(V.z)));
        sommets.pousse(dls::math::vec3f(
            static_cast<float>(NV.x), static_cast<float>(NV.y), static_cast<float>(NV.z)));
    }

    dls::tableau<unsigned int> indices(sommets.taille());
    std::iota(indices.debut(), indices.fin(), 0);

    ParametresTampon parametres_tampon;
    parametres_tampon.attribut = "sommets";
    parametres_tampon.dimension_attribut = 3;
    parametres_tampon.pointeur_sommets = sommets.donnees();
    parametres_tampon.taille_octet_sommets = static_cast<size_t>(sommets.taille()) *
                                             sizeof(dls::math::vec3f);
    parametres_tampon.pointeur_index = indices.donnees();
    parametres_tampon.taille_octet_index = static_cast<size_t>(indices.taille()) *
                                           sizeof(unsigned int);
    parametres_tampon.elements = static_cast<size_t>(indices.taille());

    m_tampon_normal->remplie_tampon(parametres_tampon);

    ParametresDessin parametres_dessin;
    parametres_dessin.type_dessin(GL_LINES);

    m_tampon_normal->parametres_dessin(parametres_dessin);
}

void RenduMaillage::initialise()
{
    genere_tampon_surface();
    genere_tampon_normal();
}

void RenduMaillage::dessine(ContexteRendu const &contexte, kdo::Scene const &scene)
{
    Spectre spectre(1.0f);
    auto nuanceur = m_maillage->nuanceur();

    switch (nuanceur->type) {
        case kdo::TypeNuanceur::DIFFUS:
            spectre = dynamic_cast<kdo::NuanceurDiffus *>(nuanceur)->spectre;
            break;
        case kdo::TypeNuanceur::ANGLE_VUE:
            spectre = dynamic_cast<kdo::NuanceurAngleVue *>(nuanceur)->spectre;
            break;
        default:
            break;
    }

    auto programme = m_tampon_surface->programme();
    programme->active();
    programme->uniforme("couleur", spectre[0], spectre[1], spectre[2], 1.0f);

    /* À FAIRE : trouve les lumières les plus proches. */
    auto const nombre_lumiere = std::min(8l, scene.lumieres.taille());

    for (long i = 0; i < nombre_lumiere; ++i) {
        auto lumiere = scene.lumieres[i];
        spectre = lumiere->spectre;
        auto transform = lumiere->transformation.matrice();
        programme->uniforme("lumieres[" + std::to_string(i) + "].couleur",
                            spectre[0],
                            spectre[1],
                            spectre[2],
                            1.0f);

        if (lumiere->type == kdo::type_lumiere::DISTANTE) {
            programme->uniforme("lumieres[" + std::to_string(i) + "].position", 0.0f, 0.0f, 1.0f);
        }
        else {
            programme->uniforme("lumieres[" + std::to_string(i) + "].position",
                                float(transform[0][3]),
                                float(transform[1][3]),
                                float(transform[2][3]));
        }

        programme->uniforme("lumieres[" + std::to_string(i) + "].type",
                            static_cast<int>(lumiere->type));
    }

    programme->desactive();

    m_tampon_surface->dessine(contexte);

    if (m_maillage->dessine_normaux()) {
        m_tampon_normal->dessine(contexte);
    }
}

dls::math::mat4x4d RenduMaillage::matrice() const
{
    return m_maillage->transformation().matrice();
}
