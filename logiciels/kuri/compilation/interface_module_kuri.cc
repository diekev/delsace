/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "interface_module_kuri.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"

NoeudDéclarationEntêteFonction *InterfaceKuri::déclaration_pour_ident(const IdentifiantCode *ident)
{
#define RETOURNE_SI_APPARIEMENT_IDENT(nom_rubrique, nom_ident)                                    \
    if (ident == nom_ident) {                                                                     \
        return nom_rubrique;                                                                      \
    }
    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(RETOURNE_SI_APPARIEMENT_IDENT)
#undef RETOURNE_SI_APPARIEMENT_IDENT
    return nullptr;
}

void InterfaceKuri::mute_rubrique(NoeudDéclarationEntêteFonction *noeud)
{
#define INIT_RUBRIQUE(rubrique, nom)                                                              \
    if (noeud->ident == nom) {                                                                    \
        rubrique = noeud;                                                                         \
        return;                                                                                   \
    }

    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(INIT_RUBRIQUE)

#undef INIT_RUBRIQUE
}

bool ident_est_pour_fonction_interface(const IdentifiantCode *ident)
{
#define COMPARE_NOM(rubrique, nom)                                                                \
    if (ident == nom) {                                                                           \
        return true;                                                                              \
    }

    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(COMPARE_NOM)

#undef COMPARE_NOM
    return false;
}

void renseigne_type_interface(Typeuse &typeuse, const IdentifiantCode *ident, Type *type)
{
#define INIT_TYPE(rubrique, nom)                                                                  \
    if (ident == nom) {                                                                           \
        typeuse.rubrique = type;                                                                  \
        return;                                                                                   \
    }

    ENUMERE_TYPE_INTERFACE_HORS_INFOS_TYPES(INIT_TYPE)

#define INIT_INFO_TYPE(rubrique, nom)                                                             \
    if (ident == nom) {                                                                           \
        typeuse.rubrique = type;                                                                  \
        type->drapeaux_type |= DrapeauxTypes::EST_TYPE_INFO_TYPE;                                 \
        return;                                                                                   \
    }

    ENUMERE_TYPE_INFO_TYPES(INIT_INFO_TYPE)

#undef INIT_TYPE
}

bool ident_est_pour_type_interface(const IdentifiantCode *ident)
{
#define COMPARE_TYPE(rubrique, nom)                                                               \
    if (ident == nom) {                                                                           \
        return true;                                                                              \
    }

    ENUMERE_TYPE_INTERFACE_MODULE_KURI(COMPARE_TYPE)

#undef COMPARE_TYPE
    return false;
}

bool est_type_interface_disponible(Typeuse &typeuse, const IdentifiantCode *ident)
{
#define COMPARE_TYPE(rubrique, nom)                                                               \
    if (ident == nom) {                                                                           \
        return typeuse.rubrique != nullptr;                                                       \
    }

    ENUMERE_TYPE_INTERFACE_MODULE_KURI(COMPARE_TYPE)

#undef COMPARE_TYPE
    return false;
}
