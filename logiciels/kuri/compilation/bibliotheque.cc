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

#include "bibliotheque.hh"

#include "parsage/identifiant.hh"

#include "statistiques/statistiques.hh"

void GestionnaireBibliotheques::ajoute_bibliotheque(kuri::chaine const &chemin)
{
    auto objet = dls::systeme_fichier::shared_library(dls::chaine(chemin).c_str());
    bibliotheques.ajoute({std::move(objet), chemin});
}

void GestionnaireBibliotheques::ajoute_fonction_pour_symbole(
    IdentifiantCode *symbole, GestionnaireBibliotheques::type_fonction fonction)
{
    symboles_et_fonctions.insere({symbole, fonction});
}

GestionnaireBibliotheques::type_fonction GestionnaireBibliotheques::fonction_pour_symbole(
    IdentifiantCode *symbole)
{
    auto iter = symboles_et_fonctions.trouve(symbole);

    if (iter != symboles_et_fonctions.fin()) {
        return iter->second;
    }

    POUR (bibliotheques) {
        try {
            auto ptr_symbole = it.bib(dls::chaine(symbole->nom.pointeur(), symbole->nom.taille()));
            auto fonction = reinterpret_cast<type_fonction>(ptr_symbole.ptr());
            ajoute_fonction_pour_symbole(symbole, fonction);
            return fonction;
        }
        catch (...) {
            continue;
        }
    }

    // std::cerr << "Impossible de trouver le symbole : " << symbole << '\n';
    return nullptr;
}

long GestionnaireBibliotheques::memoire_utilisee() const
{
    return bibliotheques.taille_memoire();
}

void GestionnaireBibliotheques::rassemble_statistiques(Statistiques &stats) const
{
    stats.memoire_bibliotheques += memoire_utilisee();
}
