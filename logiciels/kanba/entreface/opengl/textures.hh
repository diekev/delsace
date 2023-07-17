/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/ego/texture.h"

class AtlasTexture;

void génère_texture(dls::ego::Texture2D *texture, const float *données, GLint taille[2]);

void génère_texture_pour_bombage(dls::ego::Texture2D *texture, const float *data, GLint size[2]);

void génère_texture_pour_atlas(AtlasTexture *atlas, const void *donnes, GLint taille[3]);
