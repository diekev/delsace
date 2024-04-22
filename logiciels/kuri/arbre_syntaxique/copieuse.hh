/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "noeud_expression.hh"
#include "prodeclaration.hh"

#include "structures/table_hachage.hh"

struct AssembleuseArbre;

enum class OptionsCopieNoeud : uint32_t {
    AUCUNE = 0u,
    PRÉSERVE_DRAPEAUX_VALIDATION = (1u << 0),
    /* Par défaut, les paramètres ne sont pas copiés dans les membres du blocs de paramètres (car
     * ceci est fait lors de la validation sémanantique), mais nous les voulons pour les copies des
     * macros. */
    COPIE_PARAMÈTRES_DANS_MEMBRES = (1u << 1),
};
DEFINIS_OPERATEURS_DRAPEAU(OptionsCopieNoeud)

struct Copieuse {
  private:
    AssembleuseArbre *assem;
    kuri::table_hachage<NoeudExpression const *, NoeudExpression *> noeuds_copies{"noeud_copiés"};
    OptionsCopieNoeud m_options{};

  public:
    Copieuse(AssembleuseArbre *assembleuse, OptionsCopieNoeud options);

    EMPECHE_COPIE(Copieuse);

    /* L'implémentation de cette fonction est générée par l'ADN. */
    NoeudExpression *copie_noeud(const NoeudExpression *racine);

    void insere_copie(const NoeudExpression *racine, NoeudExpression *copie);

  private:
    NoeudExpression *trouve_copie(const NoeudExpression *racine);

    void copie_membres_de_bases_et_insère(const NoeudExpression *racine, NoeudExpression *nracine);
};

NoeudExpression *copie_noeud(AssembleuseArbre *assem,
                             const NoeudExpression *racine,
                             NoeudBloc *bloc_parent,
                             OptionsCopieNoeud options);
