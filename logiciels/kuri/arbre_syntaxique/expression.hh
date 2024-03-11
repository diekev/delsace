/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include <cstdint>
#include <iosfwd>
#include <variant>

#include "utilitaires/macros.hh"

struct Compilatrice;
struct NoeudBloc;
struct NoeudDéclarationEntêteFonction;
struct NoeudExpression;
struct NoeudExpressionLittéraleChaine;
struct NoeudExpressionConstructionTableau;
struct NoeudDéclarationType;

using Type = NoeudDéclarationType;

/* ************************************************************************** */

struct ValeurExpression {
  private:
    using TypeVariant = std::variant<std::monostate,
                                     bool,
                                     double,
                                     int64_t,
                                     NoeudExpressionLittéraleChaine *,
                                     NoeudExpressionConstructionTableau *,
                                     NoeudDéclarationEntêteFonction *,
                                     Type *>;
    TypeVariant v{};

  public:
    ValeurExpression() = default;

    /* Constructions. */

    ValeurExpression(int e) : v(static_cast<int64_t>(e))
    {
    }

    ValeurExpression(uint32_t e) : v(static_cast<int64_t>(e))
    {
    }

    ValeurExpression(int64_t e) : v(e)
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

    ValeurExpression(NoeudDéclarationEntêteFonction *t) : v(t)
    {
    }

    ValeurExpression(Type *t) : v(t)
    {
    }

    /* Requêtes. */

    inline bool est_valide() const
    {
        return v.index() != std::variant_npos && v.index() != 0;
    }

    inline bool est_entière() const
    {
        return std::holds_alternative<int64_t>(v);
    }

    inline bool est_booléenne() const
    {
        return std::holds_alternative<bool>(v);
    }

    inline bool est_réelle() const
    {
        return std::holds_alternative<double>(v);
    }

    inline bool est_chaine() const
    {
        return std::holds_alternative<NoeudExpressionLittéraleChaine *>(v);
    }

    inline bool est_tableau_fixe() const
    {
        return std::holds_alternative<NoeudExpressionConstructionTableau *>(v);
    }

    inline bool est_fonction() const
    {
        return std::holds_alternative<NoeudDéclarationEntêteFonction *>(v);
    }

    inline bool est_type() const
    {
        return std::holds_alternative<Type *>(v);
    }

    /* Accès. */

    inline bool booléenne() const
    {
        return std::get<bool>(v);
    }

    inline int64_t entière() const
    {
        return std::get<int64_t>(v);
    }

    inline double réelle() const
    {
        return std::get<double>(v);
    }

    inline NoeudExpressionLittéraleChaine *chaine() const
    {
        return std::get<NoeudExpressionLittéraleChaine *>(v);
    }

    inline NoeudExpressionConstructionTableau *tableau_fixe() const
    {
        return std::get<NoeudExpressionConstructionTableau *>(v);
    }

    inline NoeudDéclarationEntêteFonction *fonction() const
    {
        return std::get<NoeudDéclarationEntêteFonction *>(v);
    }

    inline Type *type() const
    {
        return std::get<Type *>(v);
    }

    inline bool est_égale_à(ValeurExpression v2) const
    {
        return v == v2.v;
    }
};

inline bool operator==(ValeurExpression v1, ValeurExpression v2)
{
    return v1.est_égale_à(v2);
}

inline bool operator!=(ValeurExpression v1, ValeurExpression v2)
{
    return !(v1 == v2);
}

std::ostream &operator<<(std::ostream &os, ValeurExpression valeur);

struct RésultatExpression {
    ValeurExpression valeur{};
    bool est_erroné = true;
    const NoeudExpression *noeud_erreur = nullptr;
    const char *message_erreur = nullptr;

    RésultatExpression() = default;

    RésultatExpression(ValeurExpression valeur_valide) : valeur(valeur_valide), est_erroné(false)
    {
    }

    COPIE_CONSTRUCT(RésultatExpression);
};

RésultatExpression évalue_expression(const Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     const NoeudExpression *b);
