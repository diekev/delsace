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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/definitions.h"

#include <iostream>
#include <variant>

struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudExpression;
struct NoeudExpressionLitteraleChaine;
struct NoeudExpressionConstructionTableau;

/* ************************************************************************** */

struct ValeurExpression {
  private:
    using TypeVariant = std::variant<std::monostate,
                                     bool,
                                     double,
                                     long,
                                     NoeudExpressionLitteraleChaine *,
                                     NoeudExpressionConstructionTableau *,
                                     NoeudDeclarationEnteteFonction *>;
    TypeVariant v{};

  public:
    ValeurExpression() = default;

    /* Constructions. */

    ValeurExpression(int e) : v(static_cast<long>(e))
    {
    }

    ValeurExpression(unsigned int e) : v(static_cast<long>(e))
    {
    }

    ValeurExpression(long e) : v(e)
    {
    }

    ValeurExpression(double r) : v(r)
    {
    }

    ValeurExpression(bool c) : v(c)
    {
    }

    ValeurExpression(NoeudExpressionConstructionTableau *t) : v(t)
    {
    }

    ValeurExpression(NoeudDeclarationEnteteFonction *t) : v(t)
    {
    }

    /* Requêtes. */

    inline bool est_valide() const
    {
        return v.index() != std::variant_npos && v.index() != 0;
    }

    inline bool est_entiere() const
    {
        return std::holds_alternative<long>(v);
    }

    inline bool est_booleenne() const
    {
        return std::holds_alternative<bool>(v);
    }

    inline bool est_reelle() const
    {
        return std::holds_alternative<double>(v);
    }

    inline bool est_chaine() const
    {
        return std::holds_alternative<NoeudExpressionLitteraleChaine *>(v);
    }

    inline bool est_tableau_fixe() const
    {
        return std::holds_alternative<NoeudExpressionConstructionTableau *>(v);
    }

    inline bool est_fonction() const
    {
        return std::holds_alternative<NoeudDeclarationEnteteFonction *>(v);
    }

    /* Accès. */

    inline bool booleenne() const
    {
        return std::get<bool>(v);
    }

    inline long entiere() const
    {
        return std::get<long>(v);
    }

    inline double reelle() const
    {
        return std::get<double>(v);
    }

    inline NoeudExpressionLitteraleChaine *chaine() const
    {
        return std::get<NoeudExpressionLitteraleChaine *>(v);
    }

    inline NoeudExpressionConstructionTableau *tableau_fixe() const
    {
        return std::get<NoeudExpressionConstructionTableau *>(v);
    }

    inline NoeudDeclarationEnteteFonction *fonction() const
    {
        return std::get<NoeudDeclarationEnteteFonction *>(v);
    }

    inline bool est_egale_a(ValeurExpression v2) const
    {
        return v == v2.v;
    }
};

inline bool operator==(ValeurExpression v1, ValeurExpression v2)
{
    return v1.est_egale_a(v2);
}

inline bool operator!=(ValeurExpression v1, ValeurExpression v2)
{
    return !(v1 == v2);
}

std::ostream &operator<<(std::ostream &os, ValeurExpression valeur);

struct ResultatExpression {
    ValeurExpression valeur{};
    bool est_errone = true;
    NoeudExpression *noeud_erreur = nullptr;
    const char *message_erreur = nullptr;

    ResultatExpression() = default;

    COPIE_CONSTRUCT(ResultatExpression);
};

ResultatExpression evalue_expression(Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     NoeudExpression *b);
