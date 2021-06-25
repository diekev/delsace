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

#include "programme.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"

void Programme::ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    if (possede(fonction)) {
        return;
    }
    fonctions.ajoute(fonction);
    fonctions_utilisees.insere(fonction);
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    globales.ajoute(globale);
    globales_utilisees.insere(globale);
}

void Programme::ajoute_type(Type *type)
{
    if (possede(type)) {
        return;
    }
    types.ajoute(type);
    types_utilises.insere(type);
}

bool Programme::typages_termines() const
{
    POUR (fonctions) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            std::cerr << "-- typage non terminé pour " << it->lexeme->chaine << '\n';
            return false;
        }

        if (!it->est_externe && !it->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            std::cerr << "-- typage non terminé pour corps " << it->lexeme->chaine << '\n';
            return false;
        }
    }

    POUR (globales) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            std::cerr << "-- typage non terminé pour " << it->lexeme->chaine << '\n';
            return false;
        }
    }

    POUR (types) {
        if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
            std::cerr << "-- typage non terminé pour " << chaine_type(it) << '\n';
            return false;
        }
    }

    return true;
}

bool Programme::ri_generees() const
{
    std::cerr << __func__ << '\n';
    if (!typages_termines()) {
        std::cerr << "-- typages non terminés !\n";
        return false;
    }

    POUR (fonctions) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            assert(it->unite);
            std::cerr << "-- ri non générée pour " << it->lexeme->chaine << '\n';
            return false;
        }
    }
    std::cerr << "-- ri fonctions générées !\n";

    POUR (globales) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            return false;
        }
    }
    std::cerr << "-- ri globales générées !\n";

    POUR (types) {
        if ((it->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
            return false;
        }
    }
    std::cerr << "-- ri types générées !\n";

    return true;
}
