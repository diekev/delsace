/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "textures.hh"

#include "biblinternes/ego/outils.h"
#include "biblinternes/opengl/atlas_texture.h"

/* ------------------------------------------------------------------------- */
/** \name Génération Texture 2D.
 * \{ */

void génère_texture(dls::ego::Texture2D *texture, const float *données, GLint taille[2])
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
    texture->remplie(données, taille);
    dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
    texture->detache();
    dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Génération Texture 2D pour bombage.
 * \{ */

void génère_texture_pour_bombage(dls::ego::Texture2D *texture,
                                 const float *données,
                                 GLint taille[2])
{
    texture->deloge(true);
    dls::ego::util::GPU_check_errors("Erreur lors de la suppression de la texture");
    texture->attache();
    dls::ego::util::GPU_check_errors("Erreur lors de l'attache de la texture");
    texture->type(GL_FLOAT, GL_RGB, GL_RGB);
    dls::ego::util::GPU_check_errors("Erreur lors du typage de la texture");
    texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
    dls::ego::util::GPU_check_errors("Erreur lors du filtrage de la texture");
    texture->enveloppe(GL_REPEAT);
    dls::ego::util::GPU_check_errors("Erreur lors du wrapping de la texture");
    texture->remplie(données, taille);
    dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
    texture->detache();
    dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Génération atlas texture 3D.
 * \{ */

void génère_texture_pour_atlas(AtlasTexture *atlas, const void *donnes, GLint taille[3])
{
    atlas->detruit(true);
    dls::ego::util::GPU_check_errors("Erreur lors de la suppression de la texture");
    atlas->attache();
    dls::ego::util::GPU_check_errors("Erreur lors de l'attache de la texture");
    atlas->typage(GL_FLOAT, GL_RGBA, GL_RGBA);
    dls::ego::util::GPU_check_errors("Erreur lors du typage de la texture");
    atlas->filtre_min_mag(GL_NEAREST, GL_NEAREST);
    dls::ego::util::GPU_check_errors("Erreur lors du filtrage de la texture");
    atlas->enveloppage(GL_CLAMP);
    dls::ego::util::GPU_check_errors("Erreur lors du wrapping de la texture");
    atlas->rempli(donnes, taille);
    dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
    atlas->detache();
    dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}

/** \} */
