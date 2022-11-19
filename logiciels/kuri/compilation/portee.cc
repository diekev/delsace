/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "portee.hh"

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/noeud_expression.hh"
#include "espace_de_travail.hh"
#include "parsage/modules.hh"

NoeudDeclaration *trouve_dans_bloc_seul(NoeudBloc *bloc, IdentifiantCode const *ident)
{
    return bloc->declaration_pour_ident(ident);
}

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc, IdentifiantCode const *ident)
{
    auto bloc_courant = bloc;

    while (bloc_courant != nullptr) {
        auto it = trouve_dans_bloc_seul(bloc_courant, ident);
        if (it) {
            return it;
        }
        bloc_courant = bloc_courant->bloc_parent;
    }

    return nullptr;
}

NoeudDeclaration *trouve_dans_bloc(NoeudBloc *bloc,
                                   NoeudDeclaration const *decl,
                                   NoeudBloc *bloc_final)
{
    auto bloc_courant = bloc;

    while (bloc_courant != bloc_final) {
        auto autre_decl = bloc->declaration_avec_meme_ident_que(decl);
        if (autre_decl) {
            return autre_decl;
        }

        bloc_courant = bloc_courant->bloc_parent;
    }

    return nullptr;
}

NoeudDeclaration *trouve_dans_bloc_ou_module(NoeudBloc *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier)
{
    auto decl = trouve_dans_bloc(bloc, ident);

    if (decl != nullptr) {
        return decl;
    }

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importes, [&](auto &module) {
        decl = trouve_dans_bloc(module->bloc, ident);

        if (decl != nullptr) {
            return kuri::DecisionIteration::Arrete;
        }

        return kuri::DecisionIteration::Continue;
    });

    return decl;
}

void trouve_declarations_dans_bloc(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                   NoeudBloc *bloc,
                                   IdentifiantCode const *ident)
{
    auto bloc_courant = bloc;

    while (bloc_courant != nullptr) {
        bloc_courant->declarations_pour_ident(declarations, ident);
        bloc_courant = bloc_courant->bloc_parent;
    }
}

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             NoeudBloc *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier)
{
    trouve_declarations_dans_bloc(declarations, bloc, ident);

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importes, [&](auto &module) {
        trouve_declarations_dans_bloc(declarations, module->bloc, ident);
        return kuri::DecisionIteration::Continue;
    });
}

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             kuri::ensemblon<Module const *, 10> &modules_visites,
                                             NoeudBloc *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier)
{
    if (!modules_visites.possede(fichier->module)) {
        trouve_declarations_dans_bloc(declarations, bloc, ident);
    }

    modules_visites.insere(fichier->module);

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importes, [&](auto &module) {
        if (modules_visites.possede(module)) {
            return kuri::DecisionIteration::Continue;
        }
        modules_visites.insere(module);
        trouve_declarations_dans_bloc(declarations, module->bloc, ident);
        return kuri::DecisionIteration::Continue;
    });
}

NoeudExpression *bloc_est_dans_boucle(NoeudBloc const *bloc, IdentifiantCode const *ident_boucle)
{
    while (bloc->bloc_parent) {
        if (bloc->appartiens_a_boucle) {
            auto boucle = bloc->appartiens_a_boucle;

            if (ident_boucle == nullptr) {
                return boucle;
            }

            if (boucle->genre == GenreNoeud::INSTRUCTION_POUR && boucle->ident == ident_boucle) {
                return boucle;
            }
        }

        bloc = bloc->bloc_parent;
    }

    return nullptr;
}

bool bloc_est_dans_differe(NoeudBloc const *bloc)
{
    while (bloc->bloc_parent) {
        if (bloc->appartiens_a_differe) {
            return true;
        }

        bloc = bloc->bloc_parent;
    }

    return false;
}

NoeudExpression *derniere_instruction(NoeudBloc const *b)
{
    auto expressions = b->expressions.verrou_lecture();
    auto taille = expressions->taille();

    if (taille == 0) {
        return NoeudExpression::nul();
    }

    auto di = expressions->a(taille - 1);

    if (di->est_retourne() || di->est_arrete() || di->est_continue() || di->est_reprends()) {
        return di;
    }

    if (di->genre == GenreNoeud::INSTRUCTION_SI) {
        auto inst = static_cast<NoeudSi *>(di);

        if (inst->bloc_si_faux == nullptr) {
            return NoeudExpression::nul();
        }

        if (!inst->bloc_si_faux->est_bloc()) {
            return NoeudExpression::nul();
        }

        return derniere_instruction(inst->bloc_si_faux->comme_bloc());
    }

    if (di->genre == GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE) {
        auto inst = static_cast<NoeudPousseContexte *>(di);
        return derniere_instruction(inst->bloc);
    }

    if (di->est_discr()) {
        auto discr = di->comme_discr();

        if (discr->bloc_sinon) {
            return derniere_instruction(discr->bloc_sinon);
        }

        /* vérifie que toutes les branches retournes */
        POUR (discr->paires_discr) {
            di = derniere_instruction(it->bloc);

            if (di == nullptr || !di->est_retourne()) {
                break;
            }
        }

        return di;
    }

    return NoeudExpression::nul();
}
