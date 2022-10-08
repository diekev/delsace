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

#include "interface_module_kuri.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"

NoeudDeclarationEnteteFonction *InterfaceKuri::declaration_pour_ident(const IdentifiantCode *ident)
{
#define RETOURNE_SI_APPARIEMENT_IDENT(nom_membre, nom_ident)                                      \
    if (ident == nom_ident) {                                                                     \
        return nom_membre;                                                                        \
    }
    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(RETOURNE_SI_APPARIEMENT_IDENT)
#undef RETOURNE_SI_APPARIEMENT_IDENT
    return nullptr;
}

void InterfaceKuri::mute_membre(NoeudDeclarationEnteteFonction *noeud)
{
#define INIT_MEMBRE(membre, nom)                                                                  \
    if (noeud->ident == nom) {                                                                    \
        membre = noeud;                                                                           \
        return;                                                                                   \
    }

    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(INIT_MEMBRE)

#undef INIT_MEMBRE
}

bool ident_est_pour_fonction_interface(const IdentifiantCode *ident)
{
#define COMPARE_NOM(membre, nom)                                                                  \
    if (ident == nom) {                                                                           \
        return true;                                                                              \
    }

    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(COMPARE_NOM)

#undef COMPARE_NOM
    return false;
}

void renseigne_type_interface(Typeuse &typeuse, const IdentifiantCode *ident, Type *type)
{
#define INIT_TYPE(membre, nom)                                                                    \
    if (ident == nom) {                                                                           \
        typeuse.membre = type;                                                                    \
        return;                                                                                   \
    }

    ENUMERE_TYPE_INTERFACE_MODULE_KURI(INIT_TYPE)

#undef INIT_TYPE
}

bool ident_est_pour_type_interface(const IdentifiantCode *ident)
{
#define COMPARE_TYPE(membre, nom)                                                                 \
    if (ident == nom) {                                                                           \
        return true;                                                                              \
    }

    ENUMERE_TYPE_INTERFACE_MODULE_KURI(COMPARE_TYPE)

#undef COMPARE_TYPE
    return false;
}

bool est_type_interface_disponible(Typeuse &typeuse, const IdentifiantCode *ident)
{
#define COMPARE_TYPE(membre, nom)                                                                 \
    if (ident == nom) {                                                                           \
        return typeuse.membre != nullptr;                                                         \
    }

    ENUMERE_TYPE_INTERFACE_MODULE_KURI(COMPARE_TYPE)

#undef COMPARE_TYPE
    return false;
}
