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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "coulisse_mv.hh"

#include "programme.hh"

bool CoulisseMV::cree_fichier_objet(Programme *programme, EspaceDeTravail *espace)
{
    auto repr_inter = representation_intermediaire_programme(*programme, *espace);

#if 0
    auto fonction = static_cast<AtomeFonction *>(metaprogramme->fonction->atome);

    if (!fonction) {
        espace->rapporte_erreur(metaprogramme->fonction,
                                "Impossible de trouver la fonction pour le métaprogramme");
    }
#endif

    if (!repr_inter.globales.est_vide()) {
        // À FAIRE(gestion)
//        auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(
//            espace, globales, fonction);
    }

   // POUR (repr_inter.fonctions) {
        // À FAIRE(gestion) la MV est utilisée pour les globales
        // genere_code_binaire_pour_fonction(it, &mv);
    //}

    return true;
}

bool CoulisseMV::cree_executable(Programme * /*programme*/)
{
    return true;
}
