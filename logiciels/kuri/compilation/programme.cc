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
    m_fonctions.ajoute(fonction);
    m_fonctions_utilisees.insere(fonction);
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insere(globale);
}

void Programme::ajoute_type(Type *type)
{
    if (possede(type)) {
        return;
    }
    m_types.ajoute(type);
    m_types_utilises.insere(type);
}

#undef DEBOGUE_VERIFICATIONS

bool Programme::typages_termines() const
{
    POUR (m_fonctions) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour " << it->lexeme->chaine << '\n';
#endif
            return false;
        }

        if (!it->est_externe && !it->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour corps " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }

    POUR (m_globales) {
        if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }

    POUR (m_types) {
        if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- typage non terminé pour " << chaine_type(it) << '\n';
#endif
            return false;
        }
    }

    return true;
}

bool Programme::ri_generees() const
{
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << __func__ << '\n';
#endif
    if (!typages_termines()) {
#ifdef DEBOGUE_VERIFICATIONS
        std::cerr << "-- typages non terminés !\n";
#endif
        return false;
    }

    POUR (m_fonctions) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            assert(it->unite);
#ifdef DEBOGUE_VERIFICATIONS
            std::cerr << "-- ri non générée pour " << it->lexeme->chaine << '\n';
#endif
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri fonctions générées !\n";
#endif

    POUR (m_globales) {
        if (!it->possede_drapeau(RI_FUT_GENEREE)) {
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri globales générées !\n";
#endif

    POUR (m_types) {
        if ((it->drapeaux & RI_TYPE_FUT_GENEREE) == 0) {
            return false;
        }
    }
#ifdef DEBOGUE_VERIFICATIONS
    std::cerr << "-- ri types générées !\n";
#endif

    return true;
}
