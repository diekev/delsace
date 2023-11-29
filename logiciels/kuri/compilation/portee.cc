/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "portee.hh"

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/noeud_expression.hh"
#include "espace_de_travail.hh"
#include "parsage/modules.hh"

NoeudDeclaration *trouve_dans_bloc_seul(NoeudBloc const *bloc, IdentifiantCode const *ident)
{
    return bloc->declaration_pour_ident(ident);
}

static bool la_fonction_courante_a_changé(NoeudDeclarationEnteteFonction const *fonction_courante,
                                          NoeudDeclarationEnteteFonction const *nouvelle_fonction)
{
    if (fonction_courante == nouvelle_fonction) {
        return false;
    }

    /* Détecte les cas où nous commençons dans le bloc d'une déclaration de
     * type fonction. */
    if (nouvelle_fonction && nouvelle_fonction->est_declaration_type) {
        return false;
    }

    return true;
}

static bool est_locale_ou_paramètre_non_polymorphique(NoeudExpression const *noeud)
{
    /* Détecte les paramètres polymorphiques (p.e. $Type: type_de_données), qui ne doivent
     * pas être ignorés. */
    if (noeud->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
        return false;
    }

    if (noeud->est_declaration_constante()) {
        return false;
    }

    if (noeud->possède_drapeau(DrapeauxNoeud::EST_LOCALE) ||
        noeud->possède_drapeau(DrapeauxNoeud::EST_PARAMETRE)) {
        return true;
    }

    return false;
}

NoeudDeclaration *trouve_dans_bloc(NoeudBloc const *bloc,
                                   IdentifiantCode const *ident,
                                   NoeudBloc const *bloc_final,
                                   NoeudDeclarationEnteteFonction const *fonction_courante)
{
    auto bloc_courant = bloc;
    auto ignore_paramètres_et_locales = false;

    while (bloc_courant != bloc_final) {
        auto it = trouve_dans_bloc_seul(bloc_courant, ident);
        if (it) {
            if (!(ignore_paramètres_et_locales && est_locale_ou_paramètre_non_polymorphique(it))) {
                return it;
            }
        }
        bloc_courant = bloc_courant->bloc_parent;

        if (bloc_courant && la_fonction_courante_a_changé(fonction_courante,
                                                          bloc_courant->appartiens_à_fonction)) {
            /* Nous avons changé de fonction, ignore les paramètres et les locales. */
            ignore_paramètres_et_locales = true;
        }
    }

    return nullptr;
}

NoeudDeclaration *trouve_dans_bloc(NoeudBloc const *bloc,
                                   NoeudDeclaration const *decl,
                                   NoeudBloc const *bloc_final,
                                   NoeudDeclarationEnteteFonction const *fonction_courante)
{
    auto bloc_courant = bloc;
    auto ignore_paramètres_et_locales = false;

    while (bloc_courant != bloc_final) {
        auto autre_decl = bloc_courant->declaration_avec_meme_ident_que(decl);
        if (autre_decl) {
            if (!(ignore_paramètres_et_locales &&
                  est_locale_ou_paramètre_non_polymorphique(autre_decl))) {
                return autre_decl;
            }
        }

        bloc_courant = bloc_courant->bloc_parent;

        if (bloc_courant && la_fonction_courante_a_changé(fonction_courante,
                                                          bloc_courant->appartiens_à_fonction)) {
            /* Nous avons changé de fonction, ignore les paramètres et les locales. */
            ignore_paramètres_et_locales = true;
        }
    }

    return nullptr;
}

NoeudDeclaration *trouve_dans_bloc_ou_module(
    NoeudBloc const *bloc,
    IdentifiantCode const *ident,
    Fichier const *fichier,
    NoeudDeclarationEnteteFonction const *fonction_courante)
{
    auto decl = trouve_dans_bloc(bloc, ident, nullptr, fonction_courante);

    if (decl != nullptr) {
        return decl;
    }

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importés, [&](auto &module) {
        decl = trouve_dans_bloc(module->bloc, ident, nullptr, fonction_courante);

        if (decl != nullptr) {
            return kuri::DécisionItération::Arrête;
        }

        return kuri::DécisionItération::Continue;
    });

    return decl;
}

NoeudDeclaration *trouve_dans_bloc_ou_module(ContexteRechecheSymbole const contexte,
                                             IdentifiantCode const *ident)
{
    return trouve_dans_bloc_ou_module(
        contexte.bloc_racine, ident, contexte.fichier, contexte.fonction_courante);
}

void trouve_declarations_dans_bloc(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                   NoeudBloc const *bloc,
                                   IdentifiantCode const *ident)
{
    auto bloc_courant = bloc;

    while (bloc_courant != nullptr) {
        auto decl = bloc_courant->declaration_pour_ident(ident);
        if (decl && decl->est_declaration_symbole()) {
            declarations.ajoute(decl);
            if (decl->est_entete_fonction()) {
                auto entête = decl->comme_entete_fonction();
                POUR (*entête->ensemble_de_surchages.verrou_lecture()) {
                    declarations.ajoute(it);
                }
            }
            else if (decl->est_declaration_type()) {
                auto type = decl->comme_declaration_type();
                POUR (*type->ensemble_de_surchages.verrou_lecture()) {
                    declarations.ajoute(it);
                }
            }
        }
        bloc_courant = bloc_courant->bloc_parent;
    }
}

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier)
{
    trouve_declarations_dans_bloc(declarations, bloc, ident);

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importés, [&](auto &module) {
        trouve_declarations_dans_bloc(declarations, module->bloc, ident);
        return kuri::DécisionItération::Continue;
    });
}

void trouve_declarations_dans_bloc_ou_module(kuri::tablet<NoeudDeclaration *, 10> &declarations,
                                             kuri::ensemblon<Module const *, 10> &modules_visites,
                                             NoeudBloc const *bloc,
                                             IdentifiantCode const *ident,
                                             Fichier const *fichier)
{
    if (!modules_visites.possède(fichier->module)) {
        trouve_declarations_dans_bloc(declarations, bloc, ident);
    }

    modules_visites.insere(fichier->module);

    /* cherche dans les modules importés */
    pour_chaque_element(fichier->modules_importés, [&](auto &module) {
        if (modules_visites.possède(module)) {
            return kuri::DécisionItération::Continue;
        }
        modules_visites.insere(module);
        trouve_declarations_dans_bloc(declarations, module->bloc, ident);
        return kuri::DécisionItération::Continue;
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

    if (di->est_si()) {
        auto inst = di->comme_si();

        if (inst->bloc_si_faux == nullptr) {
            return NoeudExpression::nul();
        }

        if (!inst->bloc_si_faux->est_bloc()) {
            return NoeudExpression::nul();
        }

        return derniere_instruction(inst->bloc_si_faux->comme_bloc());
    }

    if (di->est_pousse_contexte()) {
        auto inst = di->comme_pousse_contexte();
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
