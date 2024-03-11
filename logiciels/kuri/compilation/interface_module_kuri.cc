/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "interface_module_kuri.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"

NoeudDéclarationEntêteFonction *InterfaceKuri::declaration_pour_ident(const IdentifiantCode *ident)
{
#define RETOURNE_SI_APPARIEMENT_IDENT(nom_membre, nom_ident)                                      \
    if (ident == nom_ident) {                                                                     \
        return nom_membre;                                                                        \
    }
    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(RETOURNE_SI_APPARIEMENT_IDENT)
#undef RETOURNE_SI_APPARIEMENT_IDENT
    return nullptr;
}

void InterfaceKuri::mute_membre(NoeudDéclarationEntêteFonction *noeud)
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
