/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include <cstdint>
#include <iosfwd>
#include <variant>

#include "utilitaires/macros.hh"

struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudExpression;
struct NoeudExpressionLitteraleChaine;
struct NoeudExpressionConstructionTableau;
struct Type;

/* ************************************************************************** */

struct ValeurExpression {
  private:
    using TypeVariant = std::variant<std::monostate,
                                     bool,
                                     double,
                                     int64_t,
                                     NoeudExpressionLitteraleChaine *,
                                     NoeudExpressionConstructionTableau *,
                                     NoeudDeclarationEnteteFonction *,
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

    ValeurExpression(NoeudDeclarationEnteteFonction *t) : v(t)
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

    inline bool est_entiere() const
    {
        return std::holds_alternative<int64_t>(v);
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

    inline bool est_type() const
    {
        return std::holds_alternative<Type *>(v);
    }

    /* Accès. */

    inline bool booleenne() const
    {
        return std::get<bool>(v);
    }

    inline int64_t entiere() const
    {
        return std::get<int64_t>(v);
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

    inline Type *type() const
    {
        return std::get<Type *>(v);
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
    const NoeudExpression *noeud_erreur = nullptr;
    const char *message_erreur = nullptr;

    ResultatExpression() = default;

    ResultatExpression(ValeurExpression valeur_valide) : valeur(valeur_valide), est_errone(false)
    {
    }

    COPIE_CONSTRUCT(ResultatExpression);
};

ResultatExpression evalue_expression(const Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     const NoeudExpression *b);
