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

#pragma once

#include "biblinternes/bruit/parametres.hh"
#include "biblinternes/moultfilage/synchronise.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/arbre_kd.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "danjo/types/courbe_bezier.h"
#include "danjo/types/rampe_couleur.h"

#include "corps/polyedre.hh"

struct Corps;
struct Image;

namespace vision {
class Camera3D;
}

namespace lcc {

enum class ctx_script : unsigned short {
    invalide = 0,
    pixel = (1 << 0),
    voxel = (1 << 1),
    fichier = (1 << 2),
    nuanceur = (1 << 3),
    primitive = (1 << 4),
    point = (1 << 5),

    detail = (8 << 0),
    topologie = (8 << 1),

    /* outils */
    tous = (pixel | voxel | fichier | nuanceur | primitive | detail | topologie),
};

DEFINIE_OPERATEURS_DRAPEAU(ctx_script, unsigned short)

/* ************************************************************************** */

struct magasin_tableau {
    dls::tableau<dls::tableau<int>> tableaux;

    std::pair<dls::tableau<int> &, long> cree_tableau()
    {
        tableaux.ajoute({});
        return {tableaux.back(), tableaux.taille() - 1};
    }

    dls::tableau<int> &tableau(long idx)
    {
        return tableaux[idx];
    }

    void reinitialise()
    {
        tableaux.efface();
    }
};

/* ************************************************************************** */

/* Le contexte local est pour les données locales à chaque thread et chaque
 * itération. */
struct ctx_local {
    magasin_tableau tableaux;
    dls::tableau<bruit::parametres> params_bruits{};
    dls::tableau<dls::chaine> chaines{};

    void reinitialise()
    {
        tableaux.reinitialise();
        params_bruits.efface();
        chaines.efface();
    }
};

/* ************************************************************************** */

struct ctx_exec {
    /* Le corps dans notre contexte. */
    dls::synchronise<Corps *> ptr_corps{};

    /* Le corps d'entrée */
    Corps const *corps = nullptr;

    /* Le polyedre de notre corps */
    Polyedre polyedre{};

    /* Le polyedre de notre corps */
    arbre_3df arbre_kd{};

    /* Toutes les images. */
    dls::tableau<Image const *> images{};

    /* Toutes les caméras. */
    dls::tableau<vision::Camera3D const *> cameras{};

    /* Toutes les courbes couleur. */
    dls::tableau<CourbeCouleur const *> courbes_couleur{};

    /* Toutes les courbes valeur. */
    dls::tableau<CourbeBezier const *> courbes_valeur{};

    /* Toutes les rampes couleur. */
    dls::tableau<RampeCouleur const *> rampes_couleur{};

    /* Toutes les chaines. */
    dls::tableau<dls::chaine> chaines{};

    /* Si contexte topologie primitive. */
    // dls::tableau<Corps const *> corps_entrees;

    ctx_exec() = default;
    COPIE_CONSTRUCT(ctx_exec);

    void reinitialise()
    {
        ptr_corps = nullptr;
        images.efface();
        cameras.efface();
        courbes_couleur.efface();
        courbes_valeur.efface();
        rampes_couleur.efface();
    }
};

} /* namespace lcc */
