/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#define DEFINIS_ACCESSEUR_MEMBRE(type, nom)                                                       \
    type donne_##nom() const                                                                      \
    {                                                                                             \
        return m_##nom;                                                                           \
    }

#define DEFINIS_ACCESSEUR_MEMBRE_TABLEAU(type, nom)                                               \
    const type &donne_##nom() const                                                               \
    {                                                                                             \
        return m_##nom;                                                                           \
    }

#define DEFINIS_MUTATEUR_MEMBRE(type, nom)                                                        \
    void définis_##nom(type valeur)                                                               \
    {                                                                                             \
        m_##nom = valeur;                                                                         \
    }

#define DEFINIS_ACCESSEUR_MUTATEUR_MEMBRE(type, nom)                                              \
    DEFINIS_ACCESSEUR_MEMBRE(type, nom)                                                           \
    DEFINIS_MUTATEUR_MEMBRE(type, nom)

namespace KNB {

}
