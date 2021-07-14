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

#include <iostream>

#include "representation_intermediaire/code_binaire.hh"
#include "representation_intermediaire/constructrice_ri.hh"

#include "espace_de_travail.hh"
#include "programme.hh"

bool CoulisseMV::cree_fichier_objet(Programme *programme, EspaceDeTravail *espace, ConstructriceRI &constructrice_ri)
{
    auto repr_inter = representation_intermediaire_programme(*programme, *espace);
    auto metaprogramme = programme->pour_metaprogramme();
    assert(metaprogramme);

    auto fonction = static_cast<AtomeFonction *>(metaprogramme->fonction->atome);

    if (!fonction) {
        espace->rapporte_erreur(metaprogramme->fonction,
                                "Impossible de trouver la fonction pour le métaprogramme");
        return false;
    }

    if (!repr_inter.globales.est_vide()) {
        auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(
            espace, repr_inter.globales, fonction);

        repr_inter.fonctions.ajoute(fonc_init);
    }

    POUR (repr_inter.fonctions) {
        genere_code_binaire_pour_fonction(espace, it, metaprogramme);
    }

    return true;
}

bool CoulisseMV::cree_executable(Programme *programme)
{
    auto metaprogramme = programme->pour_metaprogramme();
    assert(metaprogramme);

    /* Liaison du code binaire du métaprogramme (application des patchs). */

    /* nous devons utiliser nos propres données pour les globales */
    auto ptr_donnees_globales = metaprogramme->donnees_globales.donnees();
    auto ptr_donnees_constantes = metaprogramme->donnees_constantes.donnees();

    // initialise les globales pour le métaprogramme
    POUR (metaprogramme->patchs_donnees_constantes) {
        void *adresse_ou = nullptr;
        void *adresse_quoi = nullptr;

        if (it.quoi == ADRESSE_CONSTANTE) {
            adresse_quoi = ptr_donnees_constantes + it.decalage_quoi;
        }
        else {
            adresse_quoi = ptr_donnees_globales + it.decalage_quoi;
        }

        if (it.ou == DONNEES_CONSTANTES) {
            adresse_ou = ptr_donnees_constantes + it.decalage_ou;
        }
        else {
            adresse_ou = ptr_donnees_globales + it.decalage_ou;
        }

        *reinterpret_cast<void **>(adresse_ou) = adresse_quoi;
        // std::cerr << "Écris adresse : " << adresse_quoi << ", à " << adresse_ou << '\n';
    }

    return true;
}
