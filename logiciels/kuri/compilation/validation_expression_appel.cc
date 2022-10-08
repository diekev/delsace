/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "validation_expression_appel.hh"

#include <variant>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/assert.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "monomorphisations.hh"
#include "portee.hh"
#include "validation_semantique.hh"

using ResultatAppariement = std::variant<ErreurAppariement, CandidateAppariement>;

struct Monomorpheuse {
    kuri::tablet<ItemMonomorphisation, 6> items{};

    using paire_type = dls::paire<Type *, Type *>;
    kuri::tablet<paire_type, 6> paires_types{};

    // mise en cache des types structures polymorphiques et de leurs monomorphisations passées
    kuri::tablet<paire_type, 6> table_structures{};

    ErreurAppariement erreur{};

    EspaceDeTravail &m_espace;

    Monomorpheuse(EspaceDeTravail &espace, NoeudBloc *bloc_constantes) : m_espace(espace)
    {
        bloc_constantes->membres.avec_verrou_lecture(
            [&](const kuri::tableau<NoeudDeclaration *, int> &membres) {
                POUR (membres) {
                    ajoute_item(it->ident);
                }
            });
    }

    void ajoute_item(IdentifiantCode *ident)
    {
        auto item = item_pour_ident(ident);
        if (item) {
            return;
        }
        /* Par défaut, nous considérons que l'item est pour un type, et non une valeur, car le code
         * fut d'abord écris pour les types. Il faudra sans doute revoir le système. */
        items.ajoute({ident, nullptr, {}, true});
    }

    bool ajoute_contrainte(IdentifiantCode *ident,
                           Type *type_contrainte,
                           Type *type_donne,
                           NoeudExpression *expr)
    {
        auto item = item_pour_ident(ident);
        if (!item) {
            return false;
        }

        /* Cas pour un type de données ($T: type_de_données). */
        if (type_contrainte->est_type_de_donnees()) {
            if (!type_donne->est_type_de_donnees()) {
                return false;
            }

            auto type = type_donne->comme_type_de_donnees()->type_connu;
            if (!type) {
                return false;
            }

            item->type = type_donne;
            item->est_type = true;
            return true;
        }

        /* Cas pour une constante typée ($N: z32). */
        item->est_type = false;

        if (type_contrainte->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            if (!ajoute_paire_types(type_contrainte, type_donne)) {
                return false;
            }

            /* À FAIRE: apparie_type doit pouvoir gérer les fonctions. */
            item->type = type_donne;
        }
        else {
            if (!(type_contrainte == type_donne ||
                  (type_donne->est_entier_constant() && est_type_entier(type_contrainte)))) {
                return false;
            }

            item->type = type_contrainte;
        }

        auto valeur = evalue_expression(m_espace.compilatrice(), expr->bloc_parent, expr);
        if (valeur.est_errone) {
            m_espace.rapporte_erreur(expr, "La valeur n'est pas constante");
            return false;
        }

        item->valeur = valeur.valeur;
        return true;
    }

    ItemMonomorphisation *item_pour_ident(IdentifiantCode const *ident)
    {
        POUR (items) {
            if (it.ident != ident) {
                continue;
            }

            return &it;
        }

        return nullptr;
    }

    bool ajoute_paire_types(Type *type_poly, Type *type_cible)
    {
        // enlève les références
        if (type_poly->est_reference()) {
            type_poly = type_poly->comme_reference()->type_pointe;
        }

        if (type_cible->est_reference()) {
            type_cible = type_cible->comme_reference()->type_pointe;
        }

        // si nous avons des fonctions, ajoute ici les paires pour chaque type polymorphique
        if (type_poly->est_fonction() && type_cible->est_fonction()) {
            auto type_poly_fonction = type_poly->comme_fonction();
            auto type_cible_fonction = type_cible->comme_fonction();

            if (type_poly_fonction->types_entrees.taille() !=
                type_cible_fonction->types_entrees.taille()) {
                return false;
            }

            for (auto i = 0; i < type_poly_fonction->types_entrees.taille(); ++i) {
                if (type_poly_fonction->types_entrees[i]->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                    if (!ajoute_paire_types(type_poly_fonction->types_entrees[i],
                                            type_cible_fonction->types_entrees[i])) {
                        return false;
                    }
                }
            }

            if (type_poly_fonction->type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                if (!ajoute_paire_types(type_poly_fonction->type_sortie,
                                        type_cible_fonction->type_sortie)) {
                    return false;
                }
            }

            return true;
        }

        if (type_poly->est_opaque() && type_cible->est_opaque()) {
            paires_types.ajoute({type_poly->comme_opaque()->type_opacifie,
                                 type_cible->comme_opaque()->type_opacifie});
            return true;
        }

        // À FAIRE(poly) : comment détecter ces cas ? x: fonc()() = nul
        if (type_poly->est_fonction() && type_cible->est_pointeur() &&
            type_cible->comme_pointeur()->type_pointe == nullptr) {
            return true;
        }

        /* type_cible peut-être variadique si nous avons une expansion variadique. */
        if (type_poly->est_variadique() && !type_cible->est_variadique()) {
            type_poly = type_poly->comme_variadique()->type_pointe;
        }

        // vérifie si nous avons des structures polymorphiques, sinon ajoute les types à la liste
        return apparie_structure(type_poly, type_cible);
    }

    bool apparie_structure(Type *type_polymorphique, Type *type_cible)
    {
        auto type_courant = type_cible;
        auto type_courant_poly = type_polymorphique;

        while (true) {
            if (type_courant_poly->genre == GenreType::POLYMORPHIQUE) {
                auto type = type_courant_poly->comme_polymorphique();

                // le type peut-être celui d'une structure (Polymorphe(T = $T)
                if (type->est_structure_poly) {
                    if (!type_courant->est_structure() && !type_courant->est_union()) {
                        return false;
                    }

                    auto decl_struct = decl_pour_type(type_courant)->comme_structure();

                    // À FAIRE : que faire ici?
                    if (!decl_struct->est_monomorphisation) {
                        return false;
                    }

                    if (decl_struct->polymorphe_de_base != type->structure) {
                        return false;
                    }

                    auto type_compose = type_courant->comme_compose();

                    for (auto i = 0; i < type->types_constants_structure.taille(); ++i) {
                        auto type1 = type->types_constants_structure[i]->comme_polymorphique();
                        auto type2 = Type::nul();

                        POUR (type_compose->membres) {
                            if (it.nom == type1->ident) {
                                type2 = it.type->comme_type_de_donnees();
                                break;
                            }
                        }

                        if (type2) {
                            paires_types.ajoute({type1, type2});
                        }
                    }

                    table_structures.ajoute(
                        {type_polymorphique,
                         m_espace.compilatrice().typeuse.type_type_de_donnees(type_cible)});
                    return true;
                }

                break;
            }

            if (type_courant->genre != type_courant_poly->genre) {
                return false;
            }

            // À FAIRE(tableau fixe)
            type_courant = type_dereference_pour(type_courant);
            type_courant_poly = type_dereference_pour(type_courant_poly);
        }

        paires_types.ajoute({type_polymorphique,
                             m_espace.compilatrice().typeuse.type_type_de_donnees(type_cible)});
        return true;
    }

    bool resoud_polymorphes(Typeuse &typeuse)
    {
        POUR (paires_types) {
            IdentifiantCode *ident = nullptr;
            auto type_polymorphique = it.premier;
            auto type_cible = it.second;
            if (type_cible->est_type_de_donnees()) {
                type_cible = type_cible->comme_type_de_donnees()->type_connu;
            }
            auto type = apparie_type(typeuse, type_polymorphique, type_cible, ident);

            if (!type) {
                erreur = ErreurAppariement::definition_type_polymorphique_impossible(nullptr,
                                                                                     ident);
                return false;
            }

            auto item = item_pour_ident(ident);
            if (!item) {
                return false;
            }
            /* Nous voulons un type ici, et nous avons l'identifiant d'une valeur : il y a conflit.
             */
            if (!item->est_type) {
                return false;
            }
            if (item->type == nullptr) {
                item->type = m_espace.compilatrice().typeuse.type_type_de_donnees(type);
            }
        }

        POUR (items) {
            if (it.type == nullptr) {
                erreur = ErreurAppariement::definition_type_polymorphique_impossible(nullptr,
                                                                                     it.ident);
                return false;
            }
        }

        return true;
    }

    Type *apparie_type(Typeuse &typeuse,
                       Type *type_polymorphique,
                       Type *type_cible,
                       IdentifiantCode *&ident)
    {
        auto type_courant = type_cible;
        auto type_courant_poly = type_polymorphique;

        while (true) {
            if (type_courant_poly->genre == GenreType::POLYMORPHIQUE) {
                ident = type_courant_poly->comme_polymorphique()->ident;
                break;
            }

            if (type_courant->genre != type_courant_poly->genre) {
                return nullptr;
            }

            // À FAIRE(tableau fixe)
            type_courant = type_dereference_pour(type_courant);
            type_courant_poly = type_dereference_pour(type_courant_poly);
        }

        if (type_courant && type_courant->est_entier_constant()) {
            return typeuse[TypeBase::Z32];
        }

        return type_courant;
    }

    Type *resoud_type_final(Typeuse &typeuse, Type *type_polymorphique)
    {
        auto resultat = Type::nul();

        POUR (table_structures) {
            if (it.premier == type_polymorphique) {
                return it.second->comme_type_de_donnees()->type_connu;
            }
        }

        if (type_polymorphique->genre == GenreType::POINTEUR) {
            auto type_pointe = type_polymorphique->comme_pointeur()->type_pointe;
            auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
            resultat = typeuse.type_pointeur_pour(type_pointe_pour_type);
        }
        else if (type_polymorphique->genre == GenreType::REFERENCE) {
            auto type_pointe = type_polymorphique->comme_reference()->type_pointe;
            auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
            resultat = typeuse.type_reference_pour(type_pointe_pour_type);
        }
        else if (type_polymorphique->genre == GenreType::TABLEAU_DYNAMIQUE) {
            auto type_pointe = type_polymorphique->comme_tableau_dynamique()->type_pointe;
            auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
            resultat = typeuse.type_tableau_dynamique(type_pointe_pour_type);
        }
        else if (type_polymorphique->genre == GenreType::TABLEAU_FIXE) {
            auto type_tableau_fixe = type_polymorphique->comme_tableau_fixe();
            auto type_pointe = type_tableau_fixe->type_pointe;
            auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
            resultat = typeuse.type_tableau_fixe(type_pointe_pour_type, type_tableau_fixe->taille);
        }
        else if (type_polymorphique->genre == GenreType::VARIADIQUE) {
            auto type_pointe = type_polymorphique->comme_variadique()->type_pointe;
            auto type_pointe_pour_type = resoud_type_final(typeuse, type_pointe);
            resultat = typeuse.type_variadique(type_pointe_pour_type);
        }
        else if (type_polymorphique->genre == GenreType::POLYMORPHIQUE) {
            auto type = type_polymorphique->comme_polymorphique();

            POUR (items) {
                if (it.ident == type->ident) {
                    resultat = const_cast<Type *>(it.type);
                    if (resultat->est_type_de_donnees()) {
                        resultat = resultat->comme_type_de_donnees()->type_connu;
                    }
                    break;
                }
            }
        }
        else if (type_polymorphique->genre == GenreType::FONCTION) {
            auto type_fonction = type_polymorphique->comme_fonction();

            auto types_entrees = kuri::tablet<Type *, 6>();
            types_entrees.reserve(type_fonction->types_entrees.taille());

            POUR (type_fonction->types_entrees) {
                if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                    auto type_param = resoud_type_final(typeuse, it);
                    types_entrees.ajoute(type_param);
                }
                else {
                    types_entrees.ajoute(it);
                }
            }

            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_tuple()) {
                auto membres = kuri::tablet<TypeCompose::Membre, 6>();

                auto tuple = type_sortie->comme_tuple();

                POUR (tuple->membres) {
                    if (it.type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                        membres.ajoute({nullptr, resoud_type_final(typeuse, it.type)});
                    }
                    else {
                        membres.ajoute({nullptr, it.type});
                    }
                }
            }
            else if (type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                type_sortie = resoud_type_final(typeuse, type_sortie);
            }

            resultat = typeuse.type_fonction(types_entrees, type_sortie);
        }
        else if (type_polymorphique->est_opaque()) {
            auto type_opaque = type_polymorphique->comme_opaque();
            auto type_opacifie = resoud_type_final(typeuse, type_opaque->type_opacifie);
            resultat = typeuse.monomorphe_opaque(type_opaque->decl, type_opacifie);
        }
        else {
            assert_rappel(false, [&]() {
                std::cerr << "Type inattendu dans la résolution de type polymorphique : "
                          << chaine_type(type_polymorphique) << "\n";
            });
        }

        return resultat;
    }
};

struct ApparieuseParams {
  private:
    kuri::tablet<IdentifiantCode *, 10> m_noms{};
    kuri::tablet<NoeudExpression *, 10> m_slots{};
    kuri::ensemblon<IdentifiantCode *, 10> args_rencontres{};
    bool m_arguments_nommes = false;
    bool m_dernier_argument_est_variadique = false;
    bool m_est_variadique = false;
    bool m_expansion_rencontree = false;
    bool m_fonction_externe = false;
    int m_index = 0;
    int m_nombre_arg_variadiques_rencontres = 0;

  public:
    ErreurAppariement erreur{};

    ApparieuseParams(bool fonction_externe) : m_fonction_externe(fonction_externe)
    {
    }

    void ajoute_param(IdentifiantCode *ident, NoeudExpression *valeur_defaut, bool est_variadique)
    {
        m_noms.ajoute(ident);

        // Ajoute uniquement la valeur défaut si le paramètre n'est pas variadique,
        // car le code d'appariement de type dépend de ce comportement.
        if (!est_variadique) {
            m_slots.ajoute(valeur_defaut);
        }

        m_est_variadique = est_variadique;
    }

    bool ajoute_expression(IdentifiantCode *ident,
                           NoeudExpression *expr,
                           NoeudExpression *expr_ident)
    {
        if (expr->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
            if (m_fonction_externe) {
                erreur = ErreurAppariement::expansion_variadique_externe(expr);
                return false;
            }

            if (m_expansion_rencontree) {
                if (m_nombre_arg_variadiques_rencontres != 0) {
                    erreur = ErreurAppariement::expansion_variadique_post_argument(expr);
                }
                else {
                    erreur = ErreurAppariement::multiple_expansions_variadiques(expr);
                }
                return false;
            }

            m_expansion_rencontree = true;
        }

        if (ident) {
            m_arguments_nommes = true;

            auto index_param = 0l;

            POUR (m_noms) {
                if (ident == it) {
                    break;
                }

                index_param += 1;
            }

            if (index_param >= m_noms.taille()) {
                erreur = ErreurAppariement::menommage_arguments(expr_ident, ident);
                return false;
            }

            auto est_parametre_variadique = index_param == m_noms.taille() - 1 && m_est_variadique;

            if ((args_rencontres.possede(ident)) && !est_parametre_variadique) {
                erreur = ErreurAppariement::renommage_argument(expr_ident, ident);
                return false;
            }

            m_dernier_argument_est_variadique = est_parametre_variadique;

            args_rencontres.insere(ident);

            if (m_dernier_argument_est_variadique || index_param >= m_slots.taille()) {
                if (m_expansion_rencontree && m_nombre_arg_variadiques_rencontres != 0) {
                    erreur = ErreurAppariement::argument_post_expansion_variadique(expr);
                    return false;
                }
                m_nombre_arg_variadiques_rencontres += 1;
                m_slots.ajoute(expr);
            }
            else {
                m_slots[index_param] = expr;
            }
        }
        else {
            if (m_arguments_nommes == true && m_dernier_argument_est_variadique == false) {
                erreur = ErreurAppariement::nom_manquant_apres_variadique(expr);
                return false;
            }

            if (m_dernier_argument_est_variadique || m_index >= m_slots.taille()) {
                if (m_expansion_rencontree && m_nombre_arg_variadiques_rencontres != 0) {
                    erreur = ErreurAppariement::argument_post_expansion_variadique(expr);
                    return false;
                }
                m_nombre_arg_variadiques_rencontres += 1;
                args_rencontres.insere(m_noms[m_noms.taille() - 1]);
                m_slots.ajoute(expr);
                m_index++;
            }
            else {
                args_rencontres.insere(m_noms[m_index]);
                m_slots[m_index++] = expr;
            }
        }

        return true;
    }

    bool tous_les_slots_sont_remplis()
    {
        for (auto i = 0; i < m_noms.taille() - m_est_variadique; ++i) {
            if (m_slots[i] == nullptr) {
                erreur.arguments_manquants_.ajoute(m_noms[i]);
            }
        }

        if (!erreur.arguments_manquants_.est_vide()) {
            erreur.raison = ARGUMENTS_MANQUANTS;
            return false;
        }

        return true;
    }

    kuri::tablet<NoeudExpression *, 10> &slots()
    {
        return m_slots;
    }
};

enum {
    CANDIDATE_EST_DECLARATION,
    CANDIDATE_EST_ACCES,
    CANDIDATE_EST_APPEL_UNIFORME,
    CANDIDATE_EST_INIT_DE,
    CANDIDATE_EST_EXPRESSION_QUELCONQUE,
};

static constexpr auto TAILLE_CANDIDATES_DEFAUT = 10;

struct CandidateExpressionAppel {
    int quoi = 0;
    NoeudExpression *decl = nullptr;
};

static ResultatValidation trouve_candidates_pour_fonction_appelee(
    ContexteValidationCode &contexte,
    EspaceDeTravail &espace,
    NoeudExpression *appelee,
    kuri::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT> &candidates)
{
    auto fichier = espace.compilatrice().fichier(appelee->lexeme->fichier);

    if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
        auto modules_visites = kuri::ensemblon<Module const *, 10>();
        auto declarations = kuri::tablet<NoeudDeclaration *, 10>();
        trouve_declarations_dans_bloc_ou_module(
            declarations, modules_visites, appelee->bloc_parent, appelee->ident, fichier);

        if (contexte.fonction_courante()) {
            auto fonction_courante = contexte.fonction_courante();

            if (fonction_courante->est_monomorphisation) {
                auto site_monomorphisation = fonction_courante->site_monomorphisation;
                auto fichier_site = espace.compilatrice().fichier(
                    site_monomorphisation->lexeme->fichier);

                if (fichier_site != fichier) {
                    trouve_declarations_dans_bloc_ou_module(declarations,
                                                            modules_visites,
                                                            site_monomorphisation->bloc_parent,
                                                            appelee->ident,
                                                            fichier_site);
                }
            }
        }

        POUR (declarations) {
            // on peut avoir des expressions du genre inverse := inverse(matrice),
            // À FAIRE : si nous enlevons la vérification du drapeau EST_GLOBALE, la compilation
            // est bloquée dans une boucle infinie, il nous faudra un état pour dire qu'aucune
            // candidate n'a été trouvée
            if (it->genre == GenreNoeud::DECLARATION_VARIABLE) {
                if (it->lexeme->fichier == appelee->lexeme->fichier &&
                    it->lexeme->ligne >= appelee->lexeme->ligne &&
                    !it->possede_drapeau(EST_GLOBALE)) {
                    continue;
                }
            }

            candidates.ajoute({CANDIDATE_EST_DECLARATION, it});
        }

        return CodeRetourValidation::OK;
    }

    if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
        auto acces = static_cast<NoeudExpressionMembre *>(appelee);

        auto accede = acces->accedee;
        auto membre = acces->membre;

        if (accede->genre == GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
            auto declaration_referee = accede->comme_reference_declaration()->declaration_referee;

            if (declaration_referee->est_declaration_module()) {
                if (!fichier->importe_module(declaration_referee->ident)) {
                    /* Nous savons que c'est un module car un autre fichier du module l'importe :
                     * la validation sémantique utilise #trouve_dans_bloc_ou_module. */
                    espace.rapporte_erreur(
                        accede,
                        "Référence d'un module alors qu'il n'a pas été importé dans le fichier.");
                    return CodeRetourValidation::Erreur;
                }

                auto module = espace.compilatrice().module(accede->ident);
                auto declarations = kuri::tablet<NoeudDeclaration *, 10>();
                trouve_declarations_dans_bloc(declarations, module->bloc, membre->ident);

                POUR (declarations) {
                    candidates.ajoute({CANDIDATE_EST_DECLARATION, it});
                }

                return CodeRetourValidation::OK;
            }
        }

        auto type_accede = accede->type;

        if (type_accede->est_type_de_donnees()) {
            /* Construction d'une structure ou union. */
            type_accede = type_accede->comme_type_de_donnees()->type_connu;

            if (!type_accede) {
                contexte.rapporte_erreur("Impossible d'accéder à un « type_de_données »", acces);
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            while (type_accede->genre == GenreType::POINTEUR ||
                   type_accede->genre == GenreType::REFERENCE) {
                type_accede = type_dereference_pour(type_accede);
            }
        }

        if (type_accede->genre == GenreType::STRUCTURE) {
            auto type_struct = type_accede->comme_structure();

            if ((type_accede->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_accede);
            }

            auto membre_trouve = false;
            auto index_membre = 0;

            POUR (type_struct->membres) {
                if (it.nom == membre->ident) {
                    acces->type = it.type;
                    membre_trouve = true;
                    break;
                }

                index_membre += 1;
            }

            if (membre_trouve != false) {
                if (acces->type->est_type_de_donnees()) {
                    auto type_membre = acces->type->comme_type_de_donnees()->type_connu;
                    if (!type_accede) {
                        contexte.rapporte_erreur("Impossible d'utiliser un « type_de_données » "
                                                 "dans une expression d'appel",
                                                 acces);
                        return CodeRetourValidation::Erreur;
                    }

                    if (type_membre->est_structure()) {
                        candidates.ajoute(
                            {CANDIDATE_EST_DECLARATION, type_membre->comme_structure()->decl});
                        return CodeRetourValidation::OK;
                    }

                    if (type_membre->est_union()) {
                        candidates.ajoute(
                            {CANDIDATE_EST_DECLARATION, type_membre->comme_union()->decl});
                        return CodeRetourValidation::OK;
                    }
                }

                candidates.ajoute({CANDIDATE_EST_ACCES, acces});
                acces->index_membre = index_membre;
                return CodeRetourValidation::OK;
            }
        }

        candidates.ajoute({CANDIDATE_EST_APPEL_UNIFORME, acces});
        return CodeRetourValidation::OK;
    }

    if (appelee->genre == GenreNoeud::EXPRESSION_INIT_DE) {
        candidates.ajoute({CANDIDATE_EST_INIT_DE, appelee});
        return CodeRetourValidation::OK;
    }

    if (appelee->type->genre == GenreType::FONCTION) {
        candidates.ajoute({CANDIDATE_EST_EXPRESSION_QUELCONQUE, appelee});
        return CodeRetourValidation::OK;
    }

    if (appelee->est_construction_structure() || appelee->est_appel()) {
        if (appelee->type->est_type_de_donnees()) {
            auto type = appelee->type->comme_type_de_donnees()->type_connu;

            if (type->est_structure()) {
                candidates.ajoute({CANDIDATE_EST_DECLARATION, type->comme_structure()->decl});
                return CodeRetourValidation::OK;
            }

            if (type->est_union()) {
                candidates.ajoute({CANDIDATE_EST_DECLARATION, type->comme_union()->decl});
                return CodeRetourValidation::OK;
            }
        }
    }

    contexte.rapporte_erreur("L'expression n'est pas de type fonction", appelee);
    return CodeRetourValidation::Erreur;
}

static ResultatPoidsTransformation apparie_type_parametre_appel_fonction(
    EspaceDeTravail &espace,
    NoeudExpression *slot,
    Type *type_du_parametre,
    Type *type_de_l_expression)
{
    if (type_du_parametre->est_variadique()) {
        /* Si le paramètre est variadique, utilise le type pointé pour vérifier la compatibilité,
         * sinon nous apparierons, par exemple, un « z32 » avec « ...z32 ».
         */
        type_du_parametre = type_dereference_pour(type_du_parametre);

        if (type_du_parametre == nullptr) {
            /* Pour les fonctions variadiques externes, nous acceptons tous les types. */
            return PoidsTransformation{TransformationType(), 1.0};
        }

        if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
            /* Pour les expansions variadiques, nous devons également utiliser le type pointé. */
            type_de_l_expression = type_dereference_pour(type_de_l_expression);
        }
    }

    return verifie_compatibilite(
        espace.compilatrice(), type_du_parametre, type_de_l_expression, slot);
}

static void cree_tableau_args_variadiques(ContexteValidationCode &contexte,
                                          kuri::tablet<NoeudExpression *, 10> &slots,
                                          int nombre_args,
                                          Type *type_donnees_argument_variadique)
{
    auto index_premier_var_arg = nombre_args - 1;
    if (slots.taille() == nombre_args &&
        slots[index_premier_var_arg]->est_expansion_variadique()) {
        return;
    }

    /* Pour les fonctions variadiques interne, nous créons un tableau
     * correspondant au types des arguments. */
    static Lexeme lexeme_tableau = {"", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0};
    auto noeud_tableau = contexte.m_tacheronne.assembleuse->cree_args_variadiques(&lexeme_tableau);

    noeud_tableau->type = type_donnees_argument_variadique;
    // @embouteillage, ceci gaspille également de la mémoire si la candidate n'est pas
    // sélectionné
    noeud_tableau->expressions.reserve(static_cast<int>(slots.taille()) - index_premier_var_arg);

    for (auto i = index_premier_var_arg; i < slots.taille(); ++i) {
        noeud_tableau->expressions.ajoute(slots[i]);
    }

    if (index_premier_var_arg >= slots.taille()) {
        slots.ajoute(noeud_tableau);
    }
    else {
        slots[index_premier_var_arg] = noeud_tableau;
    }

    slots.redimensionne(nombre_args);
}

static void applique_transformations(ContexteValidationCode &contexte,
                                     CandidateAppariement *candidate,
                                     NoeudExpressionAppel *expr)
{
    auto nombre_args_simples = static_cast<int>(candidate->exprs.taille());
    auto nombre_args_variadics = nombre_args_simples;

    if (!candidate->exprs.est_vide() &&
        candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
        /* ne compte pas le tableau */
        nombre_args_simples -= 1;
        nombre_args_variadics = candidate->transformations.taille();
    }

    auto i = 0;
    /* les drapeaux pour les arguments simples */
    for (; i < nombre_args_simples; ++i) {
        contexte.transtype_si_necessaire(expr->parametres_resolus[i],
                                         candidate->transformations[i]);
    }

    /* les drapeaux pour les arguments variadics */
    if (!candidate->exprs.est_vide() &&
        candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
        auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());

        for (auto j = 0; i < nombre_args_variadics; ++i, ++j) {
            contexte.transtype_si_necessaire(noeud_tableau->expressions[j],
                                             candidate->transformations[i]);
        }
    }
}

static ResultatAppariement apparie_appel_pointeur(
    ContexteValidationCode &contexte,
    NoeudExpressionAppel const *b,
    NoeudExpression *decl_pointeur_fonction,
    EspaceDeTravail &espace,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    auto type = decl_pointeur_fonction->type;
    // À FAIRE : ceci fut découvert alors que nous avions un type_de_données comme membre
    //    membre :: fonc()(rien)
    // au lieu de
    //    membre : fonc()(rien)
    // il faudra un système plus robuste
    if (!type->est_fonction()) {
        return ErreurAppariement::type_non_fonction(b->expression, type);
    }

    POUR (args) {
        if (it.ident == nullptr) {
            continue;
        }

        return ErreurAppariement::nommage_argument_pointeur_fonction(it.expr);
    }

    /* Apparie les arguments au type. */
    auto type_fonction = type->comme_fonction();
    auto fonction_variadique = false;

    ApparieuseParams apparieuse(false);
    for (auto i = 0; i < type_fonction->types_entrees.taille(); ++i) {
        auto type_prm = type_fonction->types_entrees[i];
        fonction_variadique |= type_prm->genre == GenreType::VARIADIQUE;
        apparieuse.ajoute_param(nullptr, nullptr, type_prm->genre == GenreType::VARIADIQUE);
    }

    POUR (args) {
        if (!apparieuse.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            return apparieuse.erreur;
        }
    }

    if (!apparieuse.tous_les_slots_sont_remplis()) {
        return ErreurAppariement::mecomptage_arguments(
            b, type_fonction->types_entrees.taille(), args.taille());
    }

    auto &slots = apparieuse.slots();
    auto transformations = kuri::tableau<TransformationType, int>(
        static_cast<int>(slots.taille()));
    auto poids_args = 1.0;

    /* Validation des types passés en paramètre. */
    for (auto i = 0l; i < slots.taille(); ++i) {
        auto index_param = std::min(i,
                                    static_cast<long>(type_fonction->types_entrees.taille() - 1));
        auto slot = slots[i];
        auto type_prm = type_fonction->types_entrees[static_cast<int>(index_param)];
        auto type_enf = slot->type;

        auto resultat = apparie_type_parametre_appel_fonction(espace, slot, type_prm, type_enf);

        if (std::holds_alternative<Attente>(resultat)) {
            return ErreurAppariement::dependance_non_satisfaite(slot, std::get<Attente>(resultat));
        }

        auto poids_xform = std::get<PoidsTransformation>(resultat);
        auto poids_pour_enfant = poids_xform.poids;

        if (slot->est_expansion_variadique()) {
            // aucune transformation acceptée sauf si nous avons un tableau fixe qu'il
            // faudra convertir en un tableau dynamique
            if (poids_pour_enfant != 1.0) {
                poids_pour_enfant = 0.0;
            }
        }

        poids_args *= poids_pour_enfant;

        if (poids_args == 0.0) {
            return ErreurAppariement::metypage_argument(slot, type_enf, type_prm);
        }

        transformations[static_cast<int>(i)] = poids_xform.transformation;
    }

    if (fonction_variadique) {
        auto nombre_args = type_fonction->types_entrees.taille();
        auto dernier_type_parametre =
            type_fonction->types_entrees[type_fonction->types_entrees.taille() - 1];
        auto type_donnees_argument_variadique = type_dereference_pour(dernier_type_parametre);
        cree_tableau_args_variadiques(
            contexte, slots, nombre_args, type_donnees_argument_variadique);
    }

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.reserve(type_fonction->types_entrees.taille());

    POUR (slots) {
        exprs.ajoute(it);
    }

    return CandidateAppariement::appel_pointeur(
        poids_args, decl_pointeur_fonction, type, std::move(exprs), std::move(transformations));
}

static ResultatAppariement apparie_appel_init_de(
    NoeudExpression *expr, kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (args.taille() > 1) {
        return ErreurAppariement::mecomptage_arguments(expr, 1, args.taille());
    }

    auto type_fonction = expr->type->comme_fonction();
    auto type_pointeur = type_fonction->types_entrees[0];

    if (type_pointeur != args[0].expr->type) {
        return ErreurAppariement::metypage_argument(
            args[0].expr, type_pointeur, args[0].expr->type);
    }

    auto exprs = kuri::cree_tablet<NoeudExpression *, 10>(args[0].expr);

    auto transformations = kuri::tableau<TransformationType, int>(1);
    transformations[0] = {TypeTransformation::INUTILE};

    return CandidateAppariement::appel_init_de(
        1.0, expr->type, std::move(exprs), std::move(transformations));
}

/* ************************************************************************** */

static ResultatAppariement apparie_appel_fonction_pour_cuisson(
    EspaceDeTravail &espace,
    ContexteValidationCode &contexte,
    NoeudExpressionAppel *expr,
    NoeudDeclarationEnteteFonction *decl,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (!decl->est_polymorphe) {
        return ErreurAppariement::metypage_argument(expr, nullptr, nullptr);
    }

    auto monomorpheuse = Monomorpheuse(espace, decl->bloc_constantes);

    // À FAIRE : vérifie que toutes les constantes ont été renseignées.
    // À FAIRE : gère proprement la validation du type de la constante

    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation;
    auto noms_rencontres = kuri::ensemblon<IdentifiantCode *, 10>();
    POUR (args) {
        if (noms_rencontres.possede(it.ident)) {
            return ErreurAppariement::renommage_argument(it.expr, it.ident);
        }
        noms_rencontres.insere(it.ident);

        auto item = monomorpheuse.item_pour_ident(it.ident);
        if (item == nullptr) {
            return ErreurAppariement::menommage_arguments(it.expr, it.ident);
        }

        auto type = it.expr->type->comme_type_de_donnees();
        items_monomorphisation.ajoute({it.ident, type, ValeurExpression(), true});
    }

    return CandidateAppariement::cuisson_fonction(
        1.0, decl, nullptr, {}, {}, std::move(items_monomorphisation));
}

static ResultatAppariement apparie_appel_fonction(
    EspaceDeTravail &espace,
    ContexteValidationCode &contexte,
    NoeudExpressionAppel *expr,
    NoeudDeclarationEnteteFonction *decl,
    kuri::tableau<IdentifiantEtExpression> const &args,
    Monomorpheuse *monomorpheuse)
{
    auto const nombre_args = decl->params.taille();

    if (!decl->est_variadique && (args.taille() > nombre_args)) {
        return ErreurAppariement::mecomptage_arguments(expr, nombre_args, args.taille());
    }

    if (nombre_args == 0 && args.taille() == 0) {
        return CandidateAppariement::appel_fonction(1.0, decl, decl->type, {}, {});
    }

    /* mise en cache des paramètres d'entrées, accéder à cette fonction se voit dans les profiles
     */
    kuri::tablet<NoeudDeclarationVariable *, 10> parametres_entree;
    for (auto i = 0; i < decl->params.taille(); ++i) {
        parametres_entree.ajoute(decl->parametre_entree(i));
    }

    auto fonction_variadique_interne = decl->est_variadique && !decl->est_externe;
    auto apparieuse_params = ApparieuseParams(decl->est_externe);
    // slots.redimensionne(nombre_args - decl->est_variadique);

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = parametres_entree[i];
        apparieuse_params.ajoute_param(
            param->ident, param->expression, param->possede_drapeau(EST_VARIADIQUE));
    }

    POUR (args) {
        if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            return apparieuse_params.erreur;
        }
    }

    if (!apparieuse_params.tous_les_slots_sont_remplis()) {
        return apparieuse_params.erreur;
    }

    auto poids_args = 1.0;

    auto &slots = apparieuse_params.slots();
    auto transformations = kuri::tablet<TransformationType, 10>(slots.taille());

    if (decl->est_polymorphe) {
        for (auto i = 0l; i < slots.taille(); ++i) {
            auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
            auto param = parametres_entree[index_arg];
            auto arg = param->valeur;
            auto slot = slots[i];

            if (param->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
                if (!monomorpheuse->ajoute_contrainte(param->ident, arg->type, slot->type, slot)) {
                    return ErreurAppariement::metypage_argument(slot, arg->type, slot->type);
                }
            }

            if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                if (!monomorpheuse->ajoute_paire_types(arg->type, slot->type)) {
                    return ErreurAppariement::metypage_argument(slot, arg->type, slot->type);
                }
            }
        }

        if (!monomorpheuse->resoud_polymorphes(espace.compilatrice().typeuse)) {
            return monomorpheuse->erreur;
        }
    }

    for (auto i = 0l; i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
        auto param = parametres_entree[index_arg];
        auto arg = param->valeur;
        auto slot = slots[i];

        if (slot == param->expression) {
            continue;
        }

        auto type_de_l_expression = slot->type;
        auto type_du_parametre = arg->type;

        if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type_du_parametre = monomorpheuse->resoud_type_final(espace.compilatrice().typeuse,
                                                                 type_du_parametre);
        }

        auto resultat = apparie_type_parametre_appel_fonction(
            espace, slot, type_du_parametre, type_de_l_expression);

        if (std::holds_alternative<Attente>(resultat)) {
            return ErreurAppariement::dependance_non_satisfaite(arg, std::get<Attente>(resultat));
        }

        auto poids_xform = std::get<PoidsTransformation>(resultat);
        auto poids_pour_enfant = poids_xform.poids;

        if (slot->est_expansion_variadique()) {
            // aucune transformation acceptée sauf si nous avons un tableau fixe qu'il
            // faudra convertir en un tableau dynamique
            if (poids_pour_enfant != 1.0) {
                poids_pour_enfant = 0.0;
            }
        }

        // allège les polymorphes pour que les versions déjà monomorphées soient préférées pour
        // la selection de la meilleure candidate
        if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            poids_pour_enfant *= 0.95;
        }

        poids_args *= poids_pour_enfant;

        if (poids_args == 0.0) {
            return ErreurAppariement::metypage_argument(
                slot, type_du_parametre, type_de_l_expression);
        }

        transformations[i] = poids_xform.transformation;
    }

    if (fonction_variadique_interne) {
        auto dernier_type_parametre = decl->params[decl->params.taille() - 1]->type;
        auto type_donnees_argument_variadique = type_dereference_pour(dernier_type_parametre);

        if (type_donnees_argument_variadique->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type_donnees_argument_variadique = monomorpheuse->resoud_type_final(
                espace.compilatrice().typeuse, type_donnees_argument_variadique);
        }

        /* Il y a des collisions entre les fonctions variadiques et les fonctions non-variadiques
         * quand le nombre d'arguments correspond pour tous les cas.
         *
         * Par exemple :
         *
         * passe_une_chaine       :: fonc (a : chaine)
         * passe_plusieurs_chaine :: fonc (args : ...chaine)
         * sont ambigües si la variadique n'est appelée qu'avec un seul élément
         *
         * et
         *
         * ne_passe_rien           :: fonc ()
         * ne_passe_peut_être_rien :: fonc (args: ...z32)
         * sont ambigües si la variadique est appelée sans arguments
         *
         * Donc diminue le poids pour les fonctions variadiques.
         */
        poids_args *= 0.95;

        cree_tableau_args_variadiques(
            contexte, slots, nombre_args, type_donnees_argument_variadique);
    }

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.reserve(slots.taille());

    auto transformations_ = kuri::tableau<TransformationType, int>();
    transformations_.reserve(static_cast<int>(transformations.taille()));

    // Il faut supprimer de l'appel les constantes correspondant aux valeur polymorphiques.
    for (auto i = 0l; i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<long>(decl->params.taille() - 1));
        auto param = parametres_entree[index_arg];

        if (param->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
            continue;
        }

        exprs.ajoute(slots[i]);

        if (i < transformations.taille()) {
            transformations_.ajoute(transformations[i]);
        }
    }

    for (auto i = slots.taille(); i < transformations.taille(); ++i) {
        transformations_.ajoute(transformations[i]);
    }

    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation;
    if (decl->est_polymorphe) {
        copie_tablet_tableau(monomorpheuse->items, items_monomorphisation);
    }

    return CandidateAppariement::appel_fonction(poids_args,
                                                decl,
                                                decl->type,
                                                std::move(exprs),
                                                std::move(transformations_),
                                                std::move(items_monomorphisation));
}

static ResultatAppariement apparie_appel_fonction(
    EspaceDeTravail &espace,
    ContexteValidationCode &contexte,
    NoeudExpressionAppel *expr,
    NoeudDeclarationEnteteFonction *decl,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (expr->drapeaux & POUR_CUISSON) {
        return apparie_appel_fonction_pour_cuisson(espace, contexte, expr, decl, args);
    }

    if (decl->est_polymorphe) {
        Monomorpheuse monomorpheuse(espace, decl->bloc_constantes);
        return apparie_appel_fonction(espace, contexte, expr, decl, args, &monomorpheuse);
    }

    return apparie_appel_fonction(espace, contexte, expr, decl, args, nullptr);
}

/* ************************************************************************** */

static bool est_expression_type_ou_valeur_polymorphique(const NoeudExpression *expr)
{
    if (expr->est_reference_declaration()) {
        auto ref_decl = expr->comme_reference_declaration();

        if (ref_decl->declaration_referee->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
            return true;
        }
    }

    if (expr->type->est_type_de_donnees()) {
        auto type_connu = expr->type->comme_type_de_donnees()->type_connu;

        if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            return true;
        }
    }

    return false;
}

static ResultatAppariement apparie_appel_structure(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    NoeudStruct *decl_struct,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    auto type_compose = decl_struct->type->comme_compose();

    if (decl_struct->est_polymorphe) {
        auto params_polymorphiques = decl_struct->bloc_constantes;
        if (expr->parametres.taille() != params_polymorphiques->membres->taille()) {
            return ErreurAppariement::mecomptage_arguments(
                expr, params_polymorphiques->membres->taille(), expr->parametres.taille());
        }

        auto apparieuse_params = ApparieuseParams(false);

        POUR (*params_polymorphiques->membres.verrou_lecture()) {
            apparieuse_params.ajoute_param(it->ident, nullptr, false);
        }

        POUR (arguments) {
            if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
                // À FAIRE : si ceci est au début de la fonction, nous avons des messages d'erreurs
                // assez étranges...
                apparieuse_params.erreur.noeud_decl = decl_struct;
                return apparieuse_params.erreur;
            }
        }

        if (!apparieuse_params.tous_les_slots_sont_remplis()) {
            return apparieuse_params.erreur;
        }

        kuri::tableau<ItemMonomorphisation, int> items_monomorphisation;

        auto index_param = 0;
        // détecte les arguments polymorphiques dans les fonctions polymorphiques
        auto est_type_argument_polymorphique = false;
        POUR (apparieuse_params.slots()) {
            auto param = params_polymorphiques->membres->a(index_param);
            index_param += 1;

            if (!param->possede_drapeau(EST_VALEUR_POLYMORPHIQUE)) {
                assert_rappel(false, []() {
                    std::cerr << "Les types polymorphiques ne sont pas supportés sur les "
                                 "structures pour le moment\n";
                });
                continue;
            }

            if (est_expression_type_ou_valeur_polymorphique(it)) {
                est_type_argument_polymorphique = true;
                continue;
            }

            // vérifie la contrainte
            if (param->type->est_type_de_donnees()) {
                if (!it->type->est_type_de_donnees()) {
                    return ErreurAppariement::metypage_argument(it, param->type, it->type);
                }

                items_monomorphisation.ajoute({param->ident, it->type, ValeurExpression(), true});
            }
            else {
                if (!(it->type == param->type ||
                      (it->type->est_entier_constant() && est_type_entier(param->type)))) {
                    return ErreurAppariement::metypage_argument(it, param->type, it->type);
                }

                auto valeur = evalue_expression(espace.compilatrice(), it->bloc_parent, it);

                if (valeur.est_errone) {
                    espace.rapporte_erreur(it, "La valeur n'est pas constante");
                }

                items_monomorphisation.ajoute({param->ident, param->type, valeur.valeur, false});
            }
        }

        if (est_type_argument_polymorphique) {
            auto type_poly = espace.compilatrice().typeuse.cree_polymorphique(nullptr);

            type_poly->est_structure_poly = true;
            type_poly->structure = decl_struct;

            POUR (arguments) {
                if (it.expr->type->est_type_de_donnees()) {
                    auto type_connu = it.expr->type->comme_type_de_donnees()->type_connu;

                    if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                        type_poly->types_constants_structure.ajoute(type_connu);
                    }
                }
            }

            return CandidateAppariement::type_polymorphique(
                1.0,
                espace.compilatrice().typeuse.type_type_de_donnees(type_poly),
                {},
                {},
                std::move(items_monomorphisation));
        }

        return CandidateAppariement::initialisation_structure(
            1.0, decl_struct, decl_struct->type, {}, {}, std::move(items_monomorphisation));
    }

    if (decl_struct->est_union) {
        if (expr->parametres.taille() > 1) {
            return ErreurAppariement::expression_extra_pour_union(expr);
        }

        if (expr->parametres.taille() == 0) {
            return ErreurAppariement::expression_manquante_union(expr);
        }
    }

    // À FAIRE : détecte quand nous avons des constantes
    auto apparieuse_params = ApparieuseParams(false);

    POUR (type_compose->membres) {
        if (it.drapeaux & TypeCompose::Membre::EST_CONSTANT) {
            apparieuse_params.ajoute_param(nullptr, nullptr, false);
            continue;
        }

        apparieuse_params.ajoute_param(it.nom, it.expression_valeur_defaut, false);
    }

    POUR (arguments) {
        if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            return apparieuse_params.erreur;
        }
    }

    auto transformations = kuri::tableau<TransformationType, int>(type_compose->membres.taille());
    auto poids_appariement = 1.0;

    auto index_membre = 0;
    POUR (apparieuse_params.slots()) {
        if (it == nullptr) {
            index_membre += 1;
            continue;
        }

        auto &membre = type_compose->membres[index_membre];

        auto resultat = verifie_compatibilite(espace.compilatrice(), membre.type, it->type, it);

        if (std::holds_alternative<Attente>(resultat)) {
            return ErreurAppariement::dependance_non_satisfaite(expr, std::get<Attente>(resultat));
        }

        auto poids_xform = std::get<PoidsTransformation>(resultat);

        poids_appariement *= poids_xform.poids;

        if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE ||
            poids_appariement == 0.0) {
            return ErreurAppariement::metypage_argument(it, membre.type, it->type);
        }

        transformations[index_membre] = poids_xform.transformation;
        index_membre += 1;
    }

    return CandidateAppariement::initialisation_structure(poids_appariement,
                                                          decl_struct,
                                                          decl_struct->type,
                                                          std::move(apparieuse_params.slots()),
                                                          std::move(transformations));
}

/* ************************************************************************** */

static ResultatAppariement apparie_construction_opaque(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    TypeOpaque *type_opaque,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (arguments.taille() == 0) {
        // À FAIRE : la construction par défaut des types opaques requiers d'avoir une construction
        // par défaut des types simples afin de pouvoir les utiliser dans la simplification du code
        return ErreurAppariement::mecomptage_arguments(expr, 1, 0);
    }

    if (arguments.taille() > 1) {
        return ErreurAppariement::mecomptage_arguments(expr, 1, 0);
    }

    auto arg = arguments[0].expr;

    if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        if (arg->type->est_type_de_donnees()) {
            auto exprs = kuri::cree_tablet<NoeudExpression *, 10>(arg);
            return CandidateAppariement::monomorphisation_opaque(
                1.0, type_opaque->decl, type_opaque, std::move(exprs), {});
        }

        auto exprs = kuri::cree_tablet<NoeudExpression *, 10>(arg);
        return CandidateAppariement::initialisation_opaque(
            1.0, type_opaque->decl, type_opaque, std::move(exprs), {});
    }

    auto resultat = verifie_compatibilite(
        espace.compilatrice(), type_opaque->type_opacifie, arg->type);

    if (std::holds_alternative<Attente>(resultat)) {
        return ErreurAppariement::dependance_non_satisfaite(expr, std::get<Attente>(resultat));
    }

    auto poids_xform = std::get<PoidsTransformation>(resultat);

    if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE) {
        return ErreurAppariement::metypage_argument(arg, type_opaque->type_opacifie, arg->type);
    }

    auto exprs = kuri::cree_tablet<NoeudExpression *, 10>(arg);

    auto transformations = kuri::tableau<TransformationType, int>(1);
    transformations[0] = poids_xform.transformation;

    return CandidateAppariement::initialisation_opaque(poids_xform.poids,
                                                       type_opaque->decl,
                                                       type_opaque,
                                                       std::move(exprs),
                                                       std::move(transformations));
}

/* ************************************************************************** */

struct ContexteValidationAppel {
    kuri::tableau<IdentifiantEtExpression> args{};
    kuri::tablet<ResultatAppariement, 10> resultats{};

    void efface()
    {
        args.efface();
        resultats.efface();
    }
};

using ListeCandidatesExpressionAppel =
    kuri::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT>;

static ResultatValidation trouve_candidates_pour_appel(
    EspaceDeTravail &espace,
    ContexteValidationCode &contexte,
    NoeudExpressionAppel *expr,
    kuri::tableau<IdentifiantEtExpression> &args,
    ListeCandidatesExpressionAppel &resultat)
{
    auto candidates_appel = ListeCandidatesExpressionAppel();
    auto resultat_validation = trouve_candidates_pour_fonction_appelee(
        contexte, espace, expr->expression, candidates_appel);
    if (!est_ok(resultat_validation)) {
        return resultat_validation;
    }

    if (candidates_appel.taille() == 0) {
        return CodeRetourValidation::Erreur;
    }

    POUR (candidates_appel) {
        if (it.quoi == CANDIDATE_EST_APPEL_UNIFORME) {
            auto acces = static_cast<NoeudExpressionBinaire *>(it.decl);
            auto candidates = ListeCandidatesExpressionAppel();
            resultat_validation = trouve_candidates_pour_fonction_appelee(
                contexte, espace, acces->operande_droite, candidates);
            if (!est_ok(resultat_validation)) {
                return resultat_validation;
            }

            if (candidates.taille() == 0) {
                return Attente::sur_symbole(acces->operande_droite->comme_reference_declaration());
            }

            args.pousse_front({nullptr, nullptr, acces->operande_gauche});

            for (auto c : candidates) {
                resultat.ajoute(c);
            }
        }
        else {
            resultat.ajoute(it);
        }
    }

    return CodeRetourValidation::OK;
}

static std::optional<Attente> apparies_candidates(
    EspaceDeTravail &espace,
    ContexteValidationCode &contexte,
    NoeudExpressionAppel *expr,
    kuri::tableau<IdentifiantEtExpression> &args,
    ListeCandidatesExpressionAppel const &candidates_appel,
    ContexteValidationAppel &resultat)
{
    POUR (candidates_appel) {
        if (it.quoi == CANDIDATE_EST_ACCES) {
            resultat.resultats.ajoute(
                apparie_appel_pointeur(contexte, expr, it.decl, espace, args));
        }
        else if (it.quoi == CANDIDATE_EST_DECLARATION) {
            auto decl = it.decl;

            if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
                auto decl_struct = static_cast<NoeudStruct *>(decl);

                if ((decl->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                    return Attente::sur_type(decl->type);
                }

                resultat.resultats.ajoute(
                    apparie_appel_structure(espace, expr, decl_struct, args));
            }
            else if (decl->est_type_opaque()) {
                auto decl_opaque = decl->comme_type_opaque();
                if (!decl_opaque->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_declaration(decl_opaque);
                }
                resultat.resultats.ajoute(apparie_construction_opaque(
                    espace, expr, decl_opaque->type->comme_opaque(), args));
            }
            else if (decl->est_entete_fonction()) {
                auto decl_fonc = decl->comme_entete_fonction();

                if (!decl_fonc->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_declaration(decl_fonc);
                }

                resultat.resultats.ajoute(
                    apparie_appel_fonction(espace, contexte, expr, decl_fonc, args));
            }
            else if (decl->est_declaration_variable()) {
                auto type = decl->type;

                if ((decl->drapeaux & DECLARATION_FUT_VALIDEE) == 0) {
                    return Attente::sur_declaration(decl->comme_declaration_variable());
                }

                /* Nous pouvons avoir une constante polymorphique ou un alias. */
                if (type->est_type_de_donnees()) {
                    auto type_de_donnees = decl->type->comme_type_de_donnees();
                    auto type_connu = type_de_donnees->type_connu;

                    if (!type_connu) {
                        resultat.resultats.ajoute(
                            ErreurAppariement::type_non_fonction(expr, type_de_donnees));
                    }

                    if (type_connu->est_structure()) {
                        auto type_struct = type_connu->comme_structure();

                        resultat.resultats.ajoute(
                            apparie_appel_structure(espace, expr, type_struct->decl, args));
                    }
                    else if (type_connu->est_union()) {
                        auto type_union = type_connu->comme_union();

                        resultat.resultats.ajoute(
                            apparie_appel_structure(espace, expr, type_union->decl, args));
                    }
                    else if (type_connu->est_opaque()) {
                        auto type_opaque = type_connu->comme_opaque();

                        resultat.resultats.ajoute(
                            apparie_construction_opaque(espace, expr, type_opaque, args));
                    }
                    else {
                        resultat.resultats.ajoute(
                            ErreurAppariement::type_non_fonction(expr, type_connu));
                    }
                }
                else if (type->est_fonction()) {
                    resultat.resultats.ajoute(
                        apparie_appel_pointeur(contexte, expr, decl, espace, args));
                }
                else if (type->est_opaque()) {
                    auto type_opaque = type->comme_opaque();

                    resultat.resultats.ajoute(
                        apparie_construction_opaque(espace, expr, type_opaque, args));
                }
                else {
                    resultat.resultats.ajoute(ErreurAppariement::type_non_fonction(expr, type));
                }
            }
        }
        else if (it.quoi == CANDIDATE_EST_INIT_DE) {
            // ici nous pourrions directement retourner si le type est correcte...
            resultat.resultats.ajoute(apparie_appel_init_de(it.decl, args));
        }
        else if (it.quoi == CANDIDATE_EST_EXPRESSION_QUELCONQUE) {
            resultat.resultats.ajoute(
                apparie_appel_pointeur(contexte, expr, it.decl, espace, args));
        }
    }

    return {};
}

/* ************************************************************************** */

static NoeudBloc *bloc_constantes_pour(NoeudExpression *noeud)
{
    if (noeud->est_entete_fonction()) {
        return noeud->comme_entete_fonction()->bloc_constantes;
    }
    if (noeud->est_structure()) {
        return noeud->comme_structure()->bloc_constantes;
    }
    return nullptr;
}

static std::pair<NoeudExpression *, bool> monomorphise_au_besoin(
    AssembleuseArbre *assembleuse,
    NoeudExpression *a_copier,
    Monomorphisations *monomorphisations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto monomorphisation = monomorphisations->trouve_monomorphisation(items_monomorphisation);
    if (monomorphisation) {
        return {monomorphisation, false};
    }

    auto copie = copie_noeud(assembleuse, a_copier, a_copier->bloc_parent);
    auto bloc_constantes = bloc_constantes_pour(copie);

    /* Ajourne les constantes dans le bloc. */
    POUR (items_monomorphisation) {
        auto decl_constante =
            trouve_dans_bloc_seul(bloc_constantes, it.ident)->comme_declaration_variable();
        decl_constante->drapeaux |= (EST_CONSTANTE | DECLARATION_FUT_VALIDEE);
        decl_constante->drapeaux &= ~(EST_VALEUR_POLYMORPHIQUE | DECLARATION_TYPE_POLYMORPHIQUE);
        decl_constante->type = const_cast<Type *>(it.type);

        if (!it.est_type) {
            decl_constante->valeur_expression = it.valeur;
        }
    }

    monomorphisations->ajoute(items_monomorphisation, copie);
    return {copie, true};
}

static std::pair<NoeudDeclarationEnteteFonction *, bool> monomorphise_au_besoin(
    ContexteValidationCode &contexte,
    Compilatrice &compilatrice,
    EspaceDeTravail &espace,
    NoeudDeclarationEnteteFonction *decl,
    NoeudExpression *site,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte.m_tacheronne.assembleuse,
                                                          decl,
                                                          decl->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto entete = copie->comme_entete_fonction();

    if (!copie_nouvelle) {
        return {entete, false};
    }

    entete->est_monomorphisation = true;
    entete->est_polymorphe = false;
    entete->site_monomorphisation = site;

    // Supprime les valeurs polymorphiques
    // À FAIRE : optimise
    auto nouveau_params = kuri::tableau<NoeudExpression *, int>();
    POUR (entete->params) {
        auto decl_constante = trouve_dans_bloc_seul(entete->bloc_constantes, it->ident);
        if (decl_constante) {
            continue;
        }

        nouveau_params.ajoute(it);
    }

    if (nouveau_params.taille() != entete->params.taille()) {
        entete->params = std::move(nouveau_params);
    }

    compilatrice.gestionnaire_code->requiers_typage(&espace, entete);
    return {entete, true};
}

static NoeudStruct *monomorphise_au_besoin(
    ContexteValidationCode &contexte,
    EspaceDeTravail &espace,
    NoeudStruct *decl_struct,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte.m_tacheronne.assembleuse,
                                                          decl_struct,
                                                          decl_struct->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto structure = copie->comme_structure();

    if (!copie_nouvelle) {
        return structure;
    }

    structure->est_polymorphe = false;
    structure->est_monomorphisation = true;
    structure->polymorphe_de_base = decl_struct;

    if (decl_struct->est_union) {
        structure->type = espace.compilatrice().typeuse.reserve_type_union(structure);
    }
    else {
        structure->type = espace.compilatrice().typeuse.reserve_type_structure(structure);
    }

    /* Ajout dans le bloc pour que la monomorphisation puisse la trouver.
     * En effet, la monomorphisation se base sur les membres du type. */
    POUR (items_monomorphisation) {
        auto decl_constante = trouve_dans_bloc_seul(structure->bloc_constantes, it.ident)
                                  ->comme_declaration_variable();
        structure->bloc->membres->ajoute(decl_constante);
    }

    contexte.m_compilatrice.gestionnaire_code->requiers_typage(&espace, structure);

    return structure;
}

/* ************************************************************************** */

static NoeudExpressionReference *symbole_pour_expression(NoeudExpression *expression)
{
    if (expression->est_reference_membre()) {
        return expression->comme_reference_membre()->membre->comme_reference_declaration();
    }

    return expression->comme_reference_declaration();
}

/* ************************************************************************** */

// À FAIRE : ajout d'un état de résolution des appels afin de savoir à quelle étape nous nous
// arrêté en cas d'erreur recouvrable (typage fait, tri des arguments fait, etc.)
ResultatValidation valide_appel_fonction(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         ContexteValidationCode &contexte,
                                         NoeudExpressionAppel *expr)
{
#ifdef STATISTIQUES_DETAILLEES
    auto possede_erreur = true;
    dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possede_erreur) {
            contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entree(
                {"tentatives râtées", temps});
        }
        contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entree(
            {"valide_appel_fonction", temps});
    });
#endif

    // ------------
    // valide d'abord les expressions, leurs types sont nécessaire pour trouver les candidates

    static ContexteValidationAppel ctx;
    ctx.efface();

    auto &args = ctx.args;
    args.reserve(expr->parametres.taille());

    {
        CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "prépare arguments");

        POUR (expr->parametres) {
            // l'argument est nommé
            if (it->est_assignation_variable()) {
                auto assign = it->comme_assignation_variable();
                auto nom_arg = assign->variable;
                auto arg = assign->expression;

                args.ajoute({nom_arg->ident, nom_arg, arg});
            }
            else {
                args.ajoute({nullptr, nullptr, it});
            }
        }
    }

    // ------------
    // trouve la fonction, pour savoir ce que l'on a

    {
        CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "trouve candidate");

        ListeCandidatesExpressionAppel candidates;

        auto resultat_validation = trouve_candidates_pour_appel(
            espace, contexte, expr, args, candidates);
        if (est_attente(resultat_validation)) {
            return std::get<Attente>(resultat_validation);
        }
        if (!est_ok(resultat_validation)) {
            // À FAIRE : il est possible qu'une erreur fut rapportée, il faudra sans doute
            //           granulariser ResultatValidation pour différencier d'une erreur lourde ou
            //           rattrappable
            return Attente::sur_symbole(symbole_pour_expression(expr->expression));
        }

        auto attente_possible = apparies_candidates(espace, contexte, expr, args, candidates, ctx);
        if (attente_possible.has_value()) {
            return attente_possible.value();
        }
    }

    kuri::tablet<CandidateAppariement, 10> candidates;
    kuri::tablet<ErreurAppariement, 10> erreurs;

    POUR (ctx.resultats) {
        if (std::holds_alternative<ErreurAppariement>(it)) {
            auto &erreur = std::get<ErreurAppariement>(it);

            if (erreur.raison == ERREUR_DEPENDANCE) {
                return erreur.attente;
            }

            erreurs.ajoute(erreur);
        }
        else {
            auto &candidate_test = std::get<CandidateAppariement>(it);
            candidates.ajoute(candidate_test);
        }
    }

    if (candidates.est_vide()) {
        erreur::lance_erreur_fonction_inconnue(espace, expr, erreurs);
        return CodeRetourValidation::Erreur;
    }

    std::sort(candidates.debut(), candidates.fin(), [](auto &a, auto &b) {
        return a.poids_args > b.poids_args;
    });

    /* À FAIRE(appel) : utilise des poids différents selon la distance entre le site d'appel et la
     * candidate. Si un paramètre possède le même nom qu'une fonction externe, il y aura collision
     * et nous ne pourrons pas choisir quelle fonction appelée.
     * Pour tester, renommer "comp" de Algorithmes.limite_basse et compiler SGBD.
     */
    if (candidates.taille() > 1 && (candidates[0].poids_args == candidates[1].poids_args)) {
        auto e = espace.rapporte_erreur(
            expr,
            "Je ne peux pas déterminer quelle fonction appeler car "
            "plusieurs fonctions correspondent à l'expression d'appel.");

        if (candidates[0].noeud_decl && candidates[1].noeud_decl) {
            e.ajoute_message("Candidate possible :\n");
            e.ajoute_site(candidates[0].noeud_decl);
            e.ajoute_message("Candidate possible :\n");
            e.ajoute_site(candidates[1].noeud_decl);
        }
        else {
            e.ajoute_message("Erreur interne ! Aucun site pour les candidates possibles !");
        }

        return CodeRetourValidation::Erreur;
    }

    auto candidate = &candidates[0];

    // ------------
    // copie les données

    CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel, "copie données");

#if 0
	struct StatisExprs {
		int nombre_exprs = 0;
		kuri::tableau<int> tailles{};

		~StatisExprs()
		{
			std::cerr << "Stats pour les tailles des expressions d'appels\n";
			std::cerr << "-- nombre d'expressions : " << nombre_exprs << '\n';
			for (int i = 0; i < tailles.taille(); ++i) {
				std::cerr << "-- taille " << i << " : " << tailles[i]
							 << " (" << static_cast<float>(tailles[i]) / static_cast<float>(nombre_exprs) << "%)" << '\n';
			}
		}

		void ajoute_taille(int t)
		{
			tailles.redimensionne(t + 1, 0);
			tailles[t] += 1;
			nombre_exprs += 1;
		}
	};

	static StatisExprs statis_exprs;
	statis_exprs.ajoute_taille(static_cast<int>(candidate->exprs.taille()));
#endif

    expr->parametres_resolus.reserve(static_cast<int>(candidate->exprs.taille()));

    for (auto enfant : candidate->exprs) {
        expr->parametres_resolus.ajoute(enfant);
    }

    if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
        auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

        if (!candidate->items_monomorphisation.est_vide()) {
            auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
                contexte,
                compilatrice,
                espace,
                decl_fonction_appelee,
                expr,
                std::move(candidate->items_monomorphisation));

            if (doit_monomorpher || !noeud_decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_decl);
            }

            decl_fonction_appelee = noeud_decl;
        }

        // nous devons monomorpher (ou avoir les types monomorphés) avant de pouvoir faire ça
        auto type_fonc = decl_fonction_appelee->type->comme_fonction();
        auto type_sortie = type_fonc->type_sortie;

        auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
        if (type_sortie->genre != GenreType::RIEN && expr_gauche) {
            espace
                .rapporte_erreur(
                    expr,
                    "La valeur de retour de la fonction n'est pas utilisée. Il est important de "
                    "toujours utiliser les valeurs retournées par les fonctions, par exemple pour "
                    "ne "
                    "pas oublier de vérifier si une erreur existe.")
                .ajoute_message("La fonction a été déclarée comme retournant une valeur :\n")
                .ajoute_site(decl_fonction_appelee)
                .ajoute_conseil(
                    "si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ "
                    "» comme identifiant pour la capturer et l'ignorer :\n")
                .ajoute_message("\t_ := appel_mais_ignore_le_retourne()\n");
            return CodeRetourValidation::Erreur;
        }

        applique_transformations(contexte, candidate, expr);

        expr->noeud_fonction_appelee = decl_fonction_appelee;
        decl_fonction_appelee->drapeaux |= EST_UTILISEE;

        if (expr->type == nullptr) {
            expr->type = type_sortie;
        }
    }
    else if (candidate->note == CANDIDATE_EST_CUISSON_FONCTION) {
        auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

        if (!candidate->items_monomorphisation.est_vide()) {
            auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
                contexte,
                compilatrice,
                espace,
                decl_fonction_appelee,
                expr,
                std::move(candidate->items_monomorphisation));

            if (doit_monomorpher || !noeud_decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_decl);
            }

            decl_fonction_appelee = noeud_decl;
        }

        expr->type = decl_fonction_appelee->type;
        expr->expression = decl_fonction_appelee;
        decl_fonction_appelee->drapeaux |= EST_UTILISEE;
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
        if (candidate->noeud_decl && candidate->noeud_decl->comme_structure()->est_polymorphe) {
            auto decl_struct = candidate->noeud_decl->comme_structure();

            auto copie = monomorphise_au_besoin(
                contexte, espace, decl_struct, std::move(candidate->items_monomorphisation));
            expr->type = espace.compilatrice().typeuse.type_type_de_donnees(copie->type);

            /* il est possible d'utiliser un type avant sa validation final, par exemple en
             * paramètre d'une fonction de rappel qui est membre de la structure */
            if ((copie->type->drapeaux & TYPE_FUT_VALIDE) == 0 &&
                copie->type != contexte.union_ou_structure_courante()) {
                // saute l'expression pour ne plus revenir
                contexte.unite->index_courant += 1;
                copie->drapeaux |= EST_UTILISEE;
                return Attente::sur_type(copie->type);
            }
        }
        else {
            expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
            expr->type = candidate->type;

            for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
                if (expr->parametres_resolus[i] != nullptr) {
                    contexte.transtype_si_necessaire(expr->parametres_resolus[i],
                                                     candidate->transformations[i]);
                }
            }

            if (!expr->possede_drapeau(DROITE_ASSIGNATION)) {
                espace.rapporte_erreur(
                    expr,
                    "La valeur de l'expression de construction de structure n'est pas "
                    "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                    "ou l'utiliser comme type ?");
                return CodeRetourValidation::Erreur;
            }
        }
    }
    else if (candidate->note == CANDIDATE_EST_TYPE_POLYMORPHIQUE) {
        expr->type = candidate->type;
        expr->noeud_fonction_appelee =
            (expr->type->comme_type_de_donnees()->type_connu->comme_polymorphique()->structure);
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
        if (expr->type == nullptr) {
            expr->type = candidate->type->comme_fonction()->type_sortie;
        }

        applique_transformations(contexte, candidate, expr);

        auto expr_gauche = !expr->possede_drapeau(DROITE_ASSIGNATION);
        if (expr->type->genre != GenreType::RIEN && expr_gauche) {
            espace
                .rapporte_erreur(expr,
                                 "La valeur de retour du pointeur de fonction n'est pas utilisée. "
                                 "Il est important "
                                 "de toujours utiliser les valeurs retournées par les fonctions, "
                                 "par exemple pour "
                                 "ne pas oublier de vérifier si une erreur existe.")
                .ajoute_message("Le type de retour du pointeur de fonctions est : ")
                .ajoute_message(chaine_type(expr->type))
                .ajoute_message("\n")
                .ajoute_conseil(
                    "si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ "
                    "» comme identifiant pour la capturer et l'ignorer :\n")
                .ajoute_message("\t_ := appel_mais_ignore_le_retourne()\n");
            return CodeRetourValidation::Erreur;
        }

        if (expr->expression->est_reference_declaration()) {
            auto ref = expr->expression->comme_reference_declaration();
            ref->declaration_referee->drapeaux |= EST_UTILISEE;
        }
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
        // le type du retour
        expr->type = espace.compilatrice().typeuse[TypeBase::RIEN];
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE) {
        auto type_opaque = candidate->type->comme_opaque();

        if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type_opaque = contexte.espace->compilatrice().typeuse.monomorphe_opaque(
                type_opaque->decl, candidate->exprs[0]->type);
            expr->type = type_opaque;
            expr->aide_generation_code = CONSTRUIT_OPAQUE;
            expr->noeud_fonction_appelee = type_opaque->decl;
        }
        else {
            for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
                contexte.transtype_si_necessaire(expr->parametres_resolus[i],
                                                 candidate->transformations[i]);
            }

            expr->type = type_opaque;
            expr->aide_generation_code = CONSTRUIT_OPAQUE;
            expr->noeud_fonction_appelee = type_opaque->decl;
        }
    }
    else if (candidate->note == CANDIDATE_EST_MONOMORPHISATION_OPAQUE) {
        auto type_opaque = candidate->type->comme_opaque();
        auto type_opacifie = candidate->exprs[0]->type->comme_type_de_donnees();

        /* différencie entre Type($T) et Type(T) où T dans le deuxième cas est connu */
        if ((type_opacifie->type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
            type_opaque = contexte.espace->compilatrice().typeuse.monomorphe_opaque(
                type_opaque->decl, type_opacifie->type_connu);
        }

        expr->noeud_fonction_appelee = type_opaque->decl;
        expr->type = contexte.espace->compilatrice().typeuse.type_type_de_donnees(type_opaque);
        expr->aide_generation_code = MONOMORPHE_TYPE_OPAQUE;
    }

#ifdef STATISTIQUES_DETAILLEES
    possede_erreur = false;
#endif

    assert(expr->type);
    return CodeRetourValidation::OK;
}

#if 0
enum class OrigineDeclaration {
    VARIABLE_LOCALE_OU_PARAMÈTRE
    DECLARATION_NICHÉE_DANS_FONCTION_COURANTE
    DECLARATION_DU_MEME_FICHIER
    DECLARTION_DU_MEME_MODULE
    DECLARTION_IMPORTEE_PAR_MODULE

    NOMBRE_D_ORIGINES
};

static constexpr double poids_pour_origine[NOMBRE_D_ORIGINES] = {
    1.0, 0.9, 0.8, 0.7, 0.6
};

static double poids_pour_candidate(const DeclarationCandidatePourAppel &declaration_candidate)
{
    double resultat = poids_pour_origine(declaration_candidate.origine);

    /* Pondère avec le poids des expressions passées en paramètres. */
    POUR (declaration_candidate.expressions) {
        resultat *= it.poids
    }

    return resultat;
}

struct DeclarationCandidatePourAppel {
    NoeudExpression *site;
    OrigineDeclaration declaration;
};
#endif
