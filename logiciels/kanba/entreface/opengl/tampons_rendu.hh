/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/opengl/tampon_rendu.h"

std::unique_ptr<TamponRendu> crée_tampon_image();

std::unique_ptr<TamponRendu> crée_tampon_nuanceur_simple(dls::phys::couleur32 couleur);

std::unique_ptr<TamponRendu> crée_tampon_texture_atlas_diffus();

std::unique_ptr<TamponRendu> crée_tampon_texture_bombée_diffus();
