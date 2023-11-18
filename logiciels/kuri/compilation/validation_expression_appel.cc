/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "validation_expression_appel.hh"

#include <iostream>
#include <variant>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/assert.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "intrinseques.hh"
#include "monomorpheuse.hh"
#include "monomorphisations.hh"
#include "portee.hh"
#include "validation_semantique.hh"

/* ------------------------------------------------------------------------- */
/** \name Poids pour les arguments polymorphiques et variadiques.
 * Nous réduisons le poids de ces types d'arguments pour favoriser les fonctions ayant été déjà
 * monomorphées, ou celle de même nom n'ayant pas d'arguments variadiques (p.e. pour favoriser
 * fonc(z32)(rien) par rapport à fonc(...z32)(rien)).
 * \{ */

static constexpr double POIDS_POUR_ARGUMENT_POLYMORPHIQUE = 0.95;
static constexpr double POIDS_POUR_ARGUMENT_VARIADIQUE = 0.95;

/** \} */

enum class ChoseÀApparier : int8_t {
    FONCTION_EXTERNE,
    FONCTION_INTERNE,
    POINTEUR_DE_FONCTION,
    STRUCTURE,
    UNION,
};

struct ApparieuseParams {
  private:
    kuri::tablet<IdentifiantCode *, 10> m_noms{};
    kuri::tablet<NoeudExpression *, 10> m_slots{};
    /* Les index sont utilisés pour les membres de structures. L'expression de construction de
     * structure doit avoir le même nombre de paramètres résolus que le nombre de membres de la
     * structure, mais, par exemple, les paramètres constants ne doivent pas être appariés, donc
     * nous stockons les index des membres pour chaque slot afin que l'appariement puisse savoir à
     * quel membre réel le slot appartient. */
    kuri::tablet<int, 10> m_index_pour_slot{};
    kuri::ensemblon<IdentifiantCode *, 10> args_rencontres{};
    bool m_arguments_nommes = false;
    bool m_dernier_argument_est_variadique = false;
    bool m_est_variadique = false;
    bool m_expansion_rencontree = false;
    ChoseÀApparier m_chose_à_apparier{};
    int m_index = 0;
    int m_nombre_arg_variadiques_rencontres = 0;

  public:
    ErreurAppariement erreur{};

    ApparieuseParams(ChoseÀApparier chose_à_apparier) : m_chose_à_apparier(chose_à_apparier)
    {
    }

    void ajoute_param(IdentifiantCode *ident,
                      NoeudExpression *valeur_defaut,
                      bool est_variadique,
                      int index = -1)
    {
        m_noms.ajoute(ident);

        // Ajoute uniquement la valeur défaut si le paramètre n'est pas variadique,
        // car le code d'appariement de type dépend de ce comportement.
        if (!est_variadique) {
            m_slots.ajoute(valeur_defaut);
            m_index_pour_slot.ajoute(index);
        }

        m_est_variadique = est_variadique;
    }

    bool ajoute_expression(IdentifiantCode *ident,
                           NoeudExpression *expr,
                           NoeudExpression *expr_ident)
    {
        if (expr->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
            if (m_chose_à_apparier == ChoseÀApparier::FONCTION_EXTERNE) {
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

            if ((args_rencontres.possède(ident)) && !est_parametre_variadique) {
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
                ajoute_slot(expr);
            }
            else {
                remplis_slot(index_param, expr);
            }
        }
        else {
            if (m_arguments_nommes == true && m_dernier_argument_est_variadique == false) {
                erreur = ErreurAppariement::nom_manquant_apres_variadique(expr);
                return false;
            }

            /* Pour les structures et les unions nous devons sauter les paramètres n'étant pas
             * utiles pour l'appariement. Ce sont notamment les variables constantes. */
            if (m_chose_à_apparier == ChoseÀApparier::STRUCTURE ||
                m_chose_à_apparier == ChoseÀApparier::UNION) {
                while (m_index < m_noms.taille() && m_noms[m_index] == nullptr) {
                    m_index++;
                }
            }

            if (m_dernier_argument_est_variadique || m_index >= m_slots.taille()) {
                if (m_expansion_rencontree && m_nombre_arg_variadiques_rencontres != 0) {
                    erreur = ErreurAppariement::argument_post_expansion_variadique(expr);
                    return false;
                }
                m_nombre_arg_variadiques_rencontres += 1;
                args_rencontres.insere(m_noms[m_noms.taille() - 1]);
                ajoute_slot(expr);
                m_index++;
            }
            else {
                args_rencontres.insere(m_noms[m_index]);
                remplis_slot(m_index++, expr);
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
        assert(m_slots.taille() == m_index_pour_slot.taille());
        return m_slots;
    }

    int index_pour_slot(int64_t index_slot) const
    {
        assert(m_slots.taille() == m_index_pour_slot.taille());
        return m_index_pour_slot[index_slot];
    }

  private:
    void ajoute_slot(NoeudExpression *expression)
    {
        m_slots.ajoute(expression);
        m_index_pour_slot.ajoute(-1);
    }

    void remplis_slot(int64_t index_slot, NoeudExpression *expression)
    {
        m_slots[index_slot] = expression;
    }
};

enum {
    CANDIDATE_EST_DECLARATION,
    CANDIDATE_EST_ACCES,
    CANDIDATE_EST_APPEL_UNIFORME,
    CANDIDATE_EST_INIT_DE,
    CANDIDATE_EST_EXPRESSION_QUELCONQUE,
};

static auto supprime_doublons(kuri::tablet<NoeudDeclaration *, 10> &tablet) -> void
{
    kuri::ensemblon<NoeudDeclaration *, 10> doublons;
    kuri::tablet<NoeudDeclaration *, 10> résultat;

    POUR (tablet) {
        if (doublons.possède(it)) {
            continue;
        }
        doublons.insere(it);
        résultat.ajoute(it);
    }

    if (résultat.taille() != tablet.taille()) {
        tablet = résultat;
    }
}

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

            if (fonction_courante->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
                auto site_monomorphisation = fonction_courante->site_monomorphisation;
                assert_rappel(site_monomorphisation->lexeme,
                              [&]() { std::cerr << erreur::imprime_site(espace, appelee); });
                auto fichier_site = espace.compilatrice().fichier(
                    site_monomorphisation->lexeme->fichier);

                if (fichier_site != fichier) {
                    auto anciennes_declarations = declarations;
                    auto anciennes_modules_visites = modules_visites;
                    trouve_declarations_dans_bloc_ou_module(declarations,
                                                            modules_visites,
                                                            site_monomorphisation->bloc_parent,
                                                            appelee->ident,
                                                            fichier_site);

                    /* L'expansion d'opérateurs pour les boucles « pour » ne réinitialise pas les
                     * blocs parents de toutes les expressions nous faisant potentiellement
                     * revisiter et réajouter les déclarations du bloc du module où l'opérateur fut
                     * défini. À FAIRE : pour l'instant nous supprimons les doublons mais nous
                     * devrons proprement gérer tout ça pour éviter de perdre du temps. */
                    supprime_doublons(declarations);
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
                    !it->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
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

        if (type_accede->est_type_type_de_donnees()) {
            /* Construction d'une structure ou union. */
            type_accede = type_accede->comme_type_type_de_donnees()->type_connu;

            if (!type_accede) {
                contexte.rapporte_erreur("Impossible d'accéder à un « type_de_données »", acces);
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            while (type_accede->est_type_pointeur() || type_accede->est_type_reference()) {
                type_accede = type_dereference_pour(type_accede);
            }
        }

        if (type_accede->est_type_structure()) {
            auto type_struct = type_accede->comme_type_structure();

            if ((type_accede->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_accede);
            }

            auto info_membre = donne_membre_pour_nom(type_struct, membre->ident);

            if (info_membre.has_value()) {
                acces->type = info_membre->membre.type;

                if (acces->type->est_type_type_de_donnees()) {
                    auto type_membre = acces->type->comme_type_type_de_donnees()->type_connu;
                    if (!type_accede) {
                        contexte.rapporte_erreur("Impossible d'utiliser un « type_de_données » "
                                                 "dans une expression d'appel",
                                                 acces);
                        return CodeRetourValidation::Erreur;
                    }

                    if (type_membre->est_type_structure()) {
                        candidates.ajoute({CANDIDATE_EST_DECLARATION,
                                           type_membre->comme_type_structure()->decl});
                        return CodeRetourValidation::OK;
                    }

                    if (type_membre->est_type_union()) {
                        candidates.ajoute(
                            {CANDIDATE_EST_DECLARATION, type_membre->comme_type_union()->decl});
                        return CodeRetourValidation::OK;
                    }
                }

                candidates.ajoute({CANDIDATE_EST_ACCES, acces});
                acces->index_membre = info_membre->index_membre;
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

    if (appelee->type->est_type_fonction()) {
        candidates.ajoute({CANDIDATE_EST_EXPRESSION_QUELCONQUE, appelee});
        return CodeRetourValidation::OK;
    }

    if (appelee->est_construction_structure() || appelee->est_appel()) {
        if (appelee->type->est_type_type_de_donnees()) {
            auto type = appelee->type->comme_type_type_de_donnees()->type_connu;

            if (type->est_type_structure()) {
                candidates.ajoute({CANDIDATE_EST_DECLARATION, type->comme_type_structure()->decl});
                return CodeRetourValidation::OK;
            }

            if (type->est_type_union()) {
                candidates.ajoute({CANDIDATE_EST_DECLARATION, type->comme_type_union()->decl});
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
    if (type_du_parametre->est_type_variadique()) {
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

    return verifie_compatibilite(type_du_parametre, type_de_l_expression, slot);
}

static void crée_tableau_args_variadiques(ContexteValidationCode &contexte,
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
    auto noeud_tableau = contexte.m_tacheronne.assembleuse->crée_args_variadiques(&lexeme_tableau);

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
        contexte.crée_transtypage_implicite_au_besoin(expr->parametres_resolus[i],
                                                      candidate->transformations[i]);
    }

    /* les drapeaux pour les arguments variadics */
    if (!candidate->exprs.est_vide() &&
        candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
        auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(candidate->exprs.back());

        for (auto j = 0; i < nombre_args_variadics; ++i, ++j) {
            contexte.crée_transtypage_implicite_au_besoin(noeud_tableau->expressions[j],
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
    if (!type->est_type_fonction()) {
        return ErreurAppariement::type_non_fonction(b->expression, type);
    }

    POUR (args) {
        if (it.ident == nullptr) {
            continue;
        }

        return ErreurAppariement::nommage_argument_pointeur_fonction(it.expr);
    }

    /* Apparie les arguments au type. */
    auto type_fonction = type->comme_type_fonction();
    auto fonction_variadique = false;

    ApparieuseParams apparieuse(ChoseÀApparier::POINTEUR_DE_FONCTION);
    for (auto i = 0; i < type_fonction->types_entrees.taille(); ++i) {
        auto type_prm = type_fonction->types_entrees[i];
        fonction_variadique |= type_prm->est_type_variadique();
        apparieuse.ajoute_param(nullptr, nullptr, type_prm->est_type_variadique());
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
    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_param = std::min(
            i, static_cast<int64_t>(type_fonction->types_entrees.taille() - 1));
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
            return ErreurAppariement::metypage_argument(slot, type_prm, type_enf);
        }

        transformations[static_cast<int>(i)] = poids_xform.transformation;
    }

    if (fonction_variadique) {
        auto nombre_args = type_fonction->types_entrees.taille();
        auto dernier_type_parametre =
            type_fonction->types_entrees[type_fonction->types_entrees.taille() - 1];
        auto type_donnees_argument_variadique = type_dereference_pour(dernier_type_parametre);
        crée_tableau_args_variadiques(
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

    auto type_fonction = expr->type->comme_type_fonction();
    auto type_pointeur = type_fonction->types_entrees[0];

    if (type_pointeur != args[0].expr->type) {
        return ErreurAppariement::metypage_argument(
            args[0].expr, type_pointeur, args[0].expr->type);
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(args[0].expr);

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
    if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        return ErreurAppariement::metypage_argument(expr, nullptr, nullptr);
    }

    auto monomorpheuse = Monomorpheuse(espace, decl);

    // À FAIRE : vérifie que toutes les constantes ont été renseignées.
    // À FAIRE : gère proprement la validation du type de la constante

    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation;
    auto noms_rencontres = kuri::ensemblon<IdentifiantCode *, 10>();
    POUR (args) {
        if (noms_rencontres.possède(it.ident)) {
            return ErreurAppariement::renommage_argument(it.expr, it.ident);
        }
        noms_rencontres.insere(it.ident);

        auto item = monomorpheuse.item_pour_ident(it.ident);
        if (item == nullptr) {
            return ErreurAppariement::menommage_arguments(it.expr, it.ident);
        }

        auto type = it.expr->type->comme_type_type_de_donnees();
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
    auto const est_variadique = decl->possède_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE);
    auto const est_externe = decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE);

    if (!est_variadique && (args.taille() > nombre_args)) {
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

    auto fonction_variadique_interne = est_variadique && !est_externe;
    auto apparieuse_params = ApparieuseParams(est_externe ? ChoseÀApparier::FONCTION_EXTERNE :
                                                            ChoseÀApparier::FONCTION_INTERNE);
    // slots.redimensionne(nombre_args - decl->est_variadique);

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = parametres_entree[i];
        apparieuse_params.ajoute_param(param->ident,
                                       param->expression,
                                       param->possède_drapeau(DrapeauxNoeud::EST_VARIADIQUE));
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

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        auto résultat_monomorphisation = détermine_monomorphisation(*monomorpheuse, decl, slots);
        if (std::holds_alternative<Attente>(résultat_monomorphisation)) {
            return ErreurAppariement::dependance_non_satisfaite(
                expr, std::get<Attente>(résultat_monomorphisation));
        }
        if (std::holds_alternative<ErreurMonomorphisation>(résultat_monomorphisation)) {
            return ErreurAppariement::monomorphisation(
                expr, std::get<ErreurMonomorphisation>(résultat_monomorphisation));
        }
    }

    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<int64_t>(decl->params.taille() - 1));
        auto param = parametres_entree[index_arg];
        auto arg = param->valeur;
        auto slot = slots[i];

        if (slot == param->expression) {
            continue;
        }

        auto type_de_l_expression = slot->type;
        auto type_du_parametre = arg->type;
        auto poids_polymorphique = POIDS_POUR_ARGUMENT_POLYMORPHIQUE;

        if (arg->type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            auto résultat_type = monomorpheuse->résoud_type_final(param->expression_type);
            if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                return ErreurAppariement::monomorphisation(
                    expr, std::get<ErreurMonomorphisation>(résultat_type));
            }

            auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
            type_du_parametre = type_apparié_pesé.type;
            poids_polymorphique *= type_apparié_pesé.poids_appariement;
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
            poids_pour_enfant *= poids_polymorphique;
        }

        poids_args *= poids_pour_enfant;

        if (poids_args == 0.0) {
            return ErreurAppariement::metypage_argument(
                slot, type_du_parametre, type_de_l_expression);
        }

        transformations[i] = poids_xform.transformation;
    }

    if (fonction_variadique_interne) {
        auto dernier_parametre = decl->parametre_entree(decl->params.taille() - 1);
        auto dernier_type_parametre = dernier_parametre->type;
        auto type_donnees_argument_variadique = type_dereference_pour(dernier_type_parametre);
        auto poids_variadique = POIDS_POUR_ARGUMENT_VARIADIQUE;

        if (type_donnees_argument_variadique->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            auto résultat_type = monomorpheuse->résoud_type_final(
                dernier_parametre->expression_type);
            if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                return ErreurAppariement::monomorphisation(
                    expr, std::get<ErreurMonomorphisation>(résultat_type));
            }

            auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
            type_donnees_argument_variadique = type_apparié_pesé.type;
            poids_variadique *= type_apparié_pesé.poids_appariement;

            /* La résolution de type retourne un type variadique, mais nous voulons le type pointé.
             */
            type_donnees_argument_variadique = type_dereference_pour(
                type_donnees_argument_variadique);
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
        poids_args *= poids_variadique;

        crée_tableau_args_variadiques(
            contexte, slots, nombre_args, type_donnees_argument_variadique);
    }

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.reserve(slots.taille());

    auto transformations_ = kuri::tableau<TransformationType, int>();
    transformations_.reserve(static_cast<int>(transformations.taille()));

    // Il faut supprimer de l'appel les constantes correspondant aux valeur polymorphiques.
    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<int64_t>(decl->params.taille() - 1));
        auto param = parametres_entree[index_arg];

        if (param->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
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
    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        copie_tablet_tableau(monomorpheuse->résultat_pour_monomorphisation(),
                             items_monomorphisation);
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
    if (expr->possède_drapeau(DrapeauxNoeud::POUR_CUISSON)) {
        return apparie_appel_fonction_pour_cuisson(espace, contexte, expr, decl, args);
    }

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        Monomorpheuse monomorpheuse(espace, decl);
        return apparie_appel_fonction(espace, contexte, expr, decl, args, &monomorpheuse);
    }

    return apparie_appel_fonction(espace, contexte, expr, decl, args, nullptr);
}

/* ************************************************************************** */

static bool est_expression_type_ou_valeur_polymorphique(const NoeudExpression *expr)
{
    if (expr->est_reference_declaration()) {
        auto ref_decl = expr->comme_reference_declaration();

        if (ref_decl->declaration_referee->possède_drapeau(
                DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            return true;
        }
    }

    if (expr->type->est_type_type_de_donnees()) {
        auto type_connu = expr->type->comme_type_type_de_donnees()->type_connu;

        if (type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            return true;
        }
    }

    return false;
}

static ResultatAppariement apparie_construction_type_composé_polymorphique(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    kuri::tableau<IdentifiantEtExpression> const &arguments,
    NoeudDeclarationType const *déclaration_type_composé,
    NoeudBloc *params_polymorphiques)
{
    if (expr->parametres.taille() != params_polymorphiques->nombre_de_membres()) {
        return ErreurAppariement::mecomptage_arguments(
            expr, params_polymorphiques->nombre_de_membres(), expr->parametres.taille());
    }

    auto apparieuse_params = ApparieuseParams(ChoseÀApparier::STRUCTURE);

    POUR (*params_polymorphiques->membres.verrou_lecture()) {
        apparieuse_params.ajoute_param(it->ident, nullptr, false);
    }

    POUR (arguments) {
        if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            // À FAIRE : si ceci est au début de la fonction, nous avons des messages d'erreurs
            // assez étranges...
            apparieuse_params.erreur.noeud_decl = déclaration_type_composé;
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
        auto param = params_polymorphiques->membre_pour_index(index_param);
        index_param += 1;

        if (!param->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
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
        if (param->type->est_type_type_de_donnees()) {
            if (!it->type->est_type_type_de_donnees()) {
                return ErreurAppariement::metypage_argument(it, param->type, it->type);
            }

            items_monomorphisation.ajoute({param->ident, it->type, ValeurExpression(), true});
        }
        else {
            if (!(it->type == param->type ||
                  (it->type->est_type_entier_constant() && est_type_entier(param->type)))) {
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
        auto type_poly = espace.compilatrice().typeuse.crée_polymorphique(nullptr);

        type_poly->est_structure_poly = true;
        type_poly->structure = déclaration_type_composé->comme_type_structure();

        return CandidateAppariement::type_polymorphique(
            1.0,
            espace.compilatrice().typeuse.type_type_de_donnees(type_poly),
            {},
            {},
            std::move(items_monomorphisation));
    }

    return CandidateAppariement::initialisation_structure(
        1.0,
        déclaration_type_composé,
        déclaration_type_composé->type->comme_type_compose(),
        {},
        {},
        std::move(items_monomorphisation));
}

static ResultatAppariement apparie_construction_type_composé(
    NoeudExpressionAppel const *expr,
    NoeudDeclarationType const *déclaration_type_composé,
    TypeCompose const *type_compose,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    auto apparieuse_params = ApparieuseParams(ChoseÀApparier::STRUCTURE);

    POUR_INDEX (type_compose->membres) {
        /* Ignore les membres employés pour le moment. */
        if (it.possède_drapeau(MembreTypeComposé::EST_CONSTANT |
                               MembreTypeComposé::EST_UN_EMPLOI)) {
            apparieuse_params.ajoute_param(nullptr, nullptr, false, index_it);
            continue;
        }

        apparieuse_params.ajoute_param(it.nom, it.expression_valeur_defaut, false, index_it);
    }

    POUR (arguments) {
        if (!apparieuse_params.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            return apparieuse_params.erreur;
        }
    }

    auto transformations = kuri::tableau<TransformationType, int>(type_compose->membres.taille());
    auto poids_appariement = 1.0;

    POUR_INDEX (apparieuse_params.slots()) {
        if (it == nullptr) {
            continue;
        }

        auto const index_membre = apparieuse_params.index_pour_slot(index_it);
        auto &membre = type_compose->membres[index_membre];

        auto resultat = verifie_compatibilite(membre.type, it->type, it);

        if (std::holds_alternative<Attente>(resultat)) {
            return ErreurAppariement::dependance_non_satisfaite(expr, std::get<Attente>(resultat));
        }

        auto poids_xform = std::get<PoidsTransformation>(resultat);

        poids_appariement *= poids_xform.poids;

        if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE ||
            poids_appariement == 0.0) {
            return ErreurAppariement::metypage_argument(it, membre.type, it->type);
        }

        transformations[index_it] = poids_xform.transformation;
    }

    return CandidateAppariement::initialisation_structure(poids_appariement,
                                                          déclaration_type_composé,
                                                          déclaration_type_composé->type,
                                                          std::move(apparieuse_params.slots()),
                                                          std::move(transformations));
}

static ResultatAppariement apparie_appel_structure(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    NoeudStruct const *decl_struct,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    assert(!decl_struct->est_union);

    if (decl_struct->est_polymorphe) {
        return apparie_construction_type_composé_polymorphique(
            espace, expr, arguments, decl_struct, decl_struct->bloc_constantes);
    }

    return apparie_construction_type_composé(
        expr, decl_struct, decl_struct->type->comme_type_compose(), arguments);
}

static ResultatAppariement apparie_construction_union(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    NoeudStruct const *decl_struct,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (decl_struct->est_polymorphe) {
        return apparie_construction_type_composé_polymorphique(
            espace, expr, arguments, decl_struct, decl_struct->bloc_constantes);
    }

    if (expr->parametres.taille() > 1) {
        return ErreurAppariement::expression_extra_pour_union(expr);
    }

    if (expr->parametres.taille() == 0) {
        return ErreurAppariement::expression_manquante_union(expr);
    }

    return apparie_construction_type_composé(
        expr, decl_struct, decl_struct->type->comme_type_compose(), arguments);
}

/* ************************************************************************** */

static ResultatAppariement apparie_construction_opaque_polymorphique(
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (arguments.taille() == 0) {
        /* Nous devons avoir au moins 1 argument pour monomorpher. */
        return ErreurAppariement::mecomptage_arguments(expr, 1, 0);
    }

    if (arguments.taille() > 1) {
        /* Nous devons avoir au plus 1 argument pour monomorpher. */
        return ErreurAppariement::mecomptage_arguments(expr, 1, arguments.taille());
    }

    auto arg = arguments[0].expr;
    if (arg->type->est_type_type_de_donnees()) {
        auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);
        return CandidateAppariement::monomorphisation_opaque(
            1.0, type_opaque->decl, type_opaque, std::move(exprs), {});
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);
    return CandidateAppariement::initialisation_opaque(
        1.0, type_opaque->decl, type_opaque, std::move(exprs), {});
}

static ResultatAppariement apparie_construction_opaque_depuis_structure(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    TypeStructure const *type_structure,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    auto résultat_appariement_structure = apparie_appel_structure(
        espace, expr, type_structure->decl, arguments);

    if (std::holds_alternative<ErreurAppariement>(résultat_appariement_structure)) {
        return résultat_appariement_structure;
    }

    /* Change les données de la candidate pour être celles de l'opaque. */
    auto candidate = std::get<CandidateAppariement>(résultat_appariement_structure);
    candidate.note = CANDIDATE_EST_INITIALISATION_OPAQUE_DEPUIS_STRUCTURE;
    candidate.type = type_opaque;
    return candidate;
}

static ResultatAppariement apparie_construction_opaque(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        return apparie_construction_opaque_polymorphique(expr, type_opaque, arguments);
    }

    if (arguments.taille() == 0) {
        /* Nous devons avoir au moins un argument.
         * À FAIRE : la construction par défaut des types opaques requiers d'avoir une construction
         * par défaut des types simples afin de pouvoir les utiliser dans la simplification du
         * code. */
        return ErreurAppariement::mecomptage_arguments(expr, 1, 0);
    }

    auto type_opacifié = donne_type_opacifié_racine(type_opaque);

    if (arguments.taille() > 1) {
        if (!type_opacifié->est_type_structure()) {
            /* Un seul argument pour les opaques de structures. */
            return ErreurAppariement::mecomptage_arguments(expr, 1, arguments.taille());
        }

        auto type_structure = type_opacifié->comme_type_structure();
        return apparie_construction_opaque_depuis_structure(
            espace, expr, type_opaque, type_structure, arguments);
    }

    auto arg = arguments[0].expr;
    auto resultat = verifie_compatibilite(type_opacifié, arg->type);

    if (std::holds_alternative<Attente>(resultat)) {
        return ErreurAppariement::dependance_non_satisfaite(expr, std::get<Attente>(resultat));
    }

    auto poids_xform = std::get<PoidsTransformation>(resultat);

    if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE) {
        /* Essaye de construire une structure depuis l'argument unique si l'opacifié est un type
         * structure. */
        if (type_opacifié->est_type_structure()) {
            auto type_structure = type_opacifié->comme_type_structure();
            return apparie_construction_opaque_depuis_structure(
                espace, expr, type_opaque, type_structure, arguments);
        }

        return ErreurAppariement::metypage_argument(arg, type_opaque->type_opacifie, arg->type);
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);

    auto transformations = kuri::tableau<TransformationType, int>(1);
    transformations[0] = poids_xform.transformation;

    return CandidateAppariement::initialisation_opaque(poids_xform.poids,
                                                       type_opaque->decl,
                                                       type_opaque,
                                                       std::move(exprs),
                                                       std::move(transformations));
}

/* ************************************************************************** */

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

static std::optional<Attente> apparies_candidates(EspaceDeTravail &espace,
                                                  ContexteValidationCode &contexte,
                                                  NoeudExpressionAppel *expr,
                                                  EtatResolutionAppel *état)
{
    /* Réinitialise en cas d'attentes passées. */
    état->résultats.efface();

    POUR (état->liste_candidates) {
        if (it.quoi == CANDIDATE_EST_ACCES) {
            état->résultats.ajoute(
                apparie_appel_pointeur(contexte, expr, it.decl, espace, état->args));
        }
        else if (it.quoi == CANDIDATE_EST_DECLARATION) {
            auto decl = it.decl;

            if (decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
                auto decl_struct = static_cast<NoeudStruct *>(decl);

                if ((decl->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                    return Attente::sur_type(decl->type);
                }

                if (decl_struct->est_union) {
                    état->résultats.ajoute(
                        apparie_construction_union(espace, expr, decl_struct, état->args));
                }
                else {
                    état->résultats.ajoute(
                        apparie_appel_structure(espace, expr, decl_struct, état->args));
                }
            }
            else if (decl->est_type_opaque()) {
                auto decl_opaque = decl->comme_type_opaque();
                if (!decl_opaque->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_declaration(decl_opaque);
                }
                état->résultats.ajoute(apparie_construction_opaque(
                    espace, expr, decl_opaque->type->comme_type_opaque(), état->args));
            }
            else if (decl->est_entete_fonction()) {
                auto decl_fonc = decl->comme_entete_fonction();

                if (!decl_fonc->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_declaration(decl_fonc);
                }

                état->résultats.ajoute(
                    apparie_appel_fonction(espace, contexte, expr, decl_fonc, état->args));
            }
            else if (decl->est_declaration_variable()) {
                auto type = decl->type;

                if (!decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_declaration(decl->comme_declaration_variable());
                }

                /* Nous pouvons avoir une constante polymorphique ou un alias. */
                if (type->est_type_type_de_donnees()) {
                    auto type_de_donnees = decl->type->comme_type_type_de_donnees();
                    auto type_connu = type_de_donnees->type_connu;

                    if (!type_connu) {
                        état->résultats.ajoute(
                            ErreurAppariement::type_non_fonction(expr, type_de_donnees));
                    }
                    else if (type_connu->est_type_structure()) {
                        auto type_struct = type_connu->comme_type_structure();

                        état->résultats.ajoute(
                            apparie_appel_structure(espace, expr, type_struct->decl, état->args));
                    }
                    else if (type_connu->est_type_union()) {
                        auto type_union = type_connu->comme_type_union();

                        état->résultats.ajoute(apparie_construction_union(
                            espace, expr, type_union->decl, état->args));
                    }
                    else if (type_connu->est_type_opaque()) {
                        auto type_opaque = type_connu->comme_type_opaque();

                        état->résultats.ajoute(
                            apparie_construction_opaque(espace, expr, type_opaque, état->args));
                    }
                    else {
                        état->résultats.ajoute(
                            ErreurAppariement::type_non_fonction(expr, type_connu));
                    }
                }
                else if (type->est_type_fonction()) {
                    état->résultats.ajoute(
                        apparie_appel_pointeur(contexte, expr, decl, espace, état->args));
                }
                else if (type->est_type_opaque()) {
                    auto type_opaque = type->comme_type_opaque();

                    état->résultats.ajoute(
                        apparie_construction_opaque(espace, expr, type_opaque, état->args));
                }
                else {
                    état->résultats.ajoute(ErreurAppariement::type_non_fonction(expr, type));
                }
            }
        }
        else if (it.quoi == CANDIDATE_EST_INIT_DE) {
            // ici nous pourrions directement retourner si le type est correcte...
            état->résultats.ajoute(apparie_appel_init_de(it.decl, état->args));
        }
        else if (it.quoi == CANDIDATE_EST_EXPRESSION_QUELCONQUE) {
            état->résultats.ajoute(
                apparie_appel_pointeur(contexte, expr, it.decl, espace, état->args));
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
    if (noeud->est_type_structure()) {
        return noeud->comme_type_structure()->bloc_constantes;
    }
    return nullptr;
}

static std::pair<NoeudExpression *, bool> monomorphise_au_besoin(
    AssembleuseArbre *assembleuse,
    NoeudExpression const *a_copier,
    Monomorphisations *monomorphisations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto monomorphisation = monomorphisations->trouve_monomorphisation(items_monomorphisation);
    if (monomorphisation) {
        return {monomorphisation, false};
    }

    auto copie = copie_noeud(
        assembleuse, a_copier, a_copier->bloc_parent, OptionsCopieNoeud::AUCUNE);
    auto bloc_constantes = bloc_constantes_pour(copie);

    /* Ajourne les constantes dans le bloc. */
    POUR (items_monomorphisation) {
        auto decl_constante =
            trouve_dans_bloc_seul(bloc_constantes, it.ident)->comme_declaration_variable();
        decl_constante->drapeaux |= (DrapeauxNoeud::EST_CONSTANTE |
                                     DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
        decl_constante->drapeaux &= ~(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE |
                                      DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE);
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
    NoeudDeclarationEnteteFonction const *decl,
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

    entete->drapeaux_fonction |= DrapeauxNoeudFonction::EST_MONOMORPHISATION;
    entete->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_POLYMORPHIQUE;
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
    NoeudStruct const *decl_struct,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte.m_tacheronne.assembleuse,
                                                          decl_struct,
                                                          decl_struct->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto structure = copie->comme_type_structure();

    if (!copie_nouvelle) {
        return structure;
    }

    structure->est_polymorphe = false;
    structure->est_monomorphisation = true;
    structure->polymorphe_de_base = const_cast<NoeudStruct *>(decl_struct);

    if (decl_struct->est_union) {
        structure->type = espace.compilatrice().typeuse.reserve_type_union(structure);
    }
    else {
        structure->type = espace.compilatrice().typeuse.reserve_type_structure(structure);
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

/* Cette fonction est utilisée pour valider les appels aux intrinsèques afin de faire en sorte que
 * les intrinsèques requérant des paramètres constants reçurent des valeurs constantes. */
static bool appel_fonction_est_valide(EspaceDeTravail &espace,
                                      NoeudDeclarationEnteteFonction const *fonction,
                                      kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (!fonction->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
        return true;
    }

    if (fonction->ident == ID::intrinsèque_précharge) {
        /* Le deuxième et troisième argument doivent être des valeurs droites. */
        if (args[1].expr->genre_valeur != GenreValeur::DROITE) {
            espace.rapporte_erreur(
                args[1].expr,
                "Le deuxième argument de « intrinsèque_précharge » ne peut pas être une variable, "
                "vous devez utiliser une valeur de RaisonPréchargement directement.");
            return false;
        }

        if (args[2].expr->genre_valeur != GenreValeur::DROITE) {
            espace.rapporte_erreur(
                args[2].expr,
                "Le troisième argument de « intrinsèque_précharge » ne peut pas être une "
                "variable, vous devez utiliser une valeur de TemporalitéPréchargement "
                "directement.");
            return false;
        }

        return true;
    }

    if (fonction->ident == ID::intrinsèque_prédit_avec_probabilité) {
        /* Le troisième argument doit être une constante. */

        if (args[2].expr->genre_valeur != GenreValeur::DROITE) {
            espace.rapporte_erreur(
                args[2].expr,
                "Le troisième argument de « intrinsèque_prédit_avec_probabilité » ne peut pas "
                "être une variable, vous devez utiliser une valeur constante.");
            return false;
        }
    }

    return true;
}

/* ************************************************************************** */

static void rassemble_expressions_paramètres(NoeudExpressionAppel *expr, EtatResolutionAppel *état)
{
    auto &args = état->args;
    args.reserve(expr->parametres.taille());
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

    état->état = EtatResolutionAppel::État::ARGUMENTS_RASSEMBLÉS;
}

static std::optional<Attente> crée_liste_candidates(NoeudExpressionAppel *expr,
                                                    EtatResolutionAppel *état,
                                                    EspaceDeTravail &espace,
                                                    ContexteValidationCode &contexte)
{
    /* Si nous revenons ici suite à une attente nous devons recommencer donc vide la liste pour
     * éviter d'avoir des doublons. */
    état->liste_candidates.efface();

    auto resultat_validation = trouve_candidates_pour_appel(
        espace, contexte, expr, état->args, état->liste_candidates);
    if (est_attente(resultat_validation)) {
        return std::get<Attente>(resultat_validation);
    }

    if (!est_ok(resultat_validation)) {
        // À FAIRE : il est possible qu'une erreur fut rapportée, il faudra sans doute
        //           granulariser ResultatValidation pour différencier d'une erreur lourde ou
        //           rattrappable
        return Attente::sur_symbole(symbole_pour_expression(expr->expression));
    }

    état->état = EtatResolutionAppel::État::LISTE_CANDIDATES_CRÉÉE;
    return {};
}

static ResultatValidation sélectionne_candidate(NoeudExpressionAppel *expr,
                                                EtatResolutionAppel *état,
                                                EspaceDeTravail &espace)
{
    POUR (état->résultats) {
        if (std::holds_alternative<ErreurAppariement>(it)) {
            auto erreur = std::get<ErreurAppariement>(it);

            if (erreur.raison == ERREUR_DEPENDANCE) {
                /* Si nous devons attendre sur quoi que ce soit, nous devrons recommencer
                 * l'appariement. */
                état->résultats.efface();
                état->candidates.efface();
                état->erreurs.efface();
                état->état = EtatResolutionAppel::État::LISTE_CANDIDATES_CRÉÉE;
                return erreur.attente;
            }

            état->erreurs.ajoute(erreur);
        }
        else {
            auto &candidate_test = std::get<CandidateAppariement>(it);
            état->candidates.ajoute(candidate_test);
        }
    }

    if (état->candidates.est_vide()) {
        erreur::lance_erreur_fonction_inconnue(espace, expr, état->erreurs);
        return CodeRetourValidation::Erreur;
    }

    std::sort(état->candidates.debut(), état->candidates.fin(), [](auto &a, auto &b) {
        return a.poids_args > b.poids_args;
    });

    /* À FAIRE(appel) : utilise des poids différents selon la distance entre le site d'appel et la
     * candidate. Si un paramètre possède le même nom qu'une fonction externe, il y aura collision
     * et nous ne pourrons pas choisir quelle fonction appelée.
     * Pour tester, renommer "comp" de Algorithmes.limite_basse et compiler SGBD.
     */
    if (état->candidates.taille() > 1 &&
        (état->candidates[0].poids_args == état->candidates[1].poids_args)) {
        auto e = espace.rapporte_erreur(
            expr,
            "Je ne peux pas déterminer quelle fonction appeler car "
            "plusieurs fonctions correspondent à l'expression d'appel.");

        if (état->candidates[0].noeud_decl && état->candidates[1].noeud_decl) {
            e.ajoute_message("Candidate possible :\n");
            e.ajoute_site(état->candidates[0].noeud_decl);
            e.ajoute_message("Candidate possible :\n");
            e.ajoute_site(état->candidates[1].noeud_decl);
        }
        else {
            e.ajoute_message("Erreur interne ! Aucun site pour les candidates possibles !");
        }

        return CodeRetourValidation::Erreur;
    }

    état->candidate_finale = &état->candidates[0];
    état->état = EtatResolutionAppel::État::CANDIDATE_SÉLECTIONNÉE;
    return CodeRetourValidation::OK;
}

ResultatValidation valide_appel_fonction(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         ContexteValidationCode &contexte,
                                         NoeudExpressionAppel *expr)
{
#ifdef STATISTIQUES_DETAILLEES
    auto possède_erreur = true;
    dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possède_erreur) {
            contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entrée(
                {"tentatives râtées", temps});
        }
        contexte.m_tacheronne.stats_typage.validation_appel.fusionne_entrée(
            {"valide_appel_fonction", temps});
    });
#endif
    if (!expr->état_résolution_appel) {
        expr->état_résolution_appel = compilatrice.crée_ou_donne_état_résolution_appel();
    }

    EtatResolutionAppel &état = *expr->état_résolution_appel;

    if (état.état == EtatResolutionAppel::État::RÉSOLUTION_NON_COMMENCÉE) {
        CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel,
                      VALIDATION_APPEL__PREPARE_ARGUMENTS);
        rassemble_expressions_paramètres(expr, &état);
    }

    if (état.état == EtatResolutionAppel::État::ARGUMENTS_RASSEMBLÉS) {
        CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel,
                      VALIDATION_APPEL__TROUVE_CANDIDATES);

        auto attente_potentielle = crée_liste_candidates(expr, &état, espace, contexte);

        if (attente_potentielle.has_value()) {
            return attente_potentielle.value();
        }
    }

    if (état.état == EtatResolutionAppel::État::LISTE_CANDIDATES_CRÉÉE) {
        CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel,
                      VALIDATION_APPEL__APPARIE_CANDIDATES);
        auto attente_possible = apparies_candidates(espace, contexte, expr, &état);
        if (attente_possible.has_value()) {
            return attente_possible.value();
        }
        état.état = EtatResolutionAppel::État::APPARIEMENT_CANDIDATES_FAIT;
    }

    if (état.état == EtatResolutionAppel::État::APPARIEMENT_CANDIDATES_FAIT) {
        auto résultat = sélectionne_candidate(expr, &état, espace);
        if (!est_ok(résultat)) {
            return résultat;
        }
    }

    assert(état.état == EtatResolutionAppel::État::CANDIDATE_SÉLECTIONNÉE);

    auto candidate = état.candidate_finale;

    // ------------
    // copie les données

    CHRONO_TYPAGE(contexte.m_tacheronne.stats_typage.validation_appel,
                  VALIDATION_APPEL__COPIE_DONNEES);

    expr->parametres_resolus.efface();
    expr->parametres_resolus.reserve(static_cast<int>(candidate->exprs.taille()));

    for (auto enfant : candidate->exprs) {
        expr->parametres_resolus.ajoute(enfant);
    }

    if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
        auto decl_fonction_appelee = candidate->noeud_decl->comme_entete_fonction();

        if (!appel_fonction_est_valide(espace, decl_fonction_appelee, état.args)) {
            return CodeRetourValidation::Erreur;
        }

        if (!candidate->items_monomorphisation.est_vide()) {
            auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
                contexte,
                compilatrice,
                espace,
                decl_fonction_appelee,
                expr,
                std::move(candidate->items_monomorphisation));

            if (doit_monomorpher ||
                !noeud_decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_decl);
            }

            decl_fonction_appelee = noeud_decl;
        }

        // nous devons monomorpher (ou avoir les types monomorphés) avant de pouvoir faire ça
        auto type_fonc = decl_fonction_appelee->type->comme_type_fonction();
        auto type_sortie = type_fonc->type_sortie;

        auto expr_gauche = !expr->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION);
        if (!type_sortie->est_type_rien() && expr_gauche) {
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

        expr->noeud_fonction_appelee = const_cast<NoeudDeclarationEnteteFonction *>(
            decl_fonction_appelee);
        expr->noeud_fonction_appelee->drapeaux |= DrapeauxNoeud::EST_UTILISEE;

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

            if (doit_monomorpher ||
                !noeud_decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_decl);
            }

            decl_fonction_appelee = noeud_decl;
        }

        expr->type = decl_fonction_appelee->type;
        expr->expression = const_cast<NoeudDeclarationEnteteFonction *>(decl_fonction_appelee);
        expr->expression->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
        if (candidate->noeud_decl &&
            candidate->noeud_decl->comme_type_structure()->est_polymorphe) {
            auto decl_struct = candidate->noeud_decl->comme_type_structure();

            auto copie = monomorphise_au_besoin(
                contexte, espace, decl_struct, std::move(candidate->items_monomorphisation));
            expr->type = espace.compilatrice().typeuse.type_type_de_donnees(copie->type);

            /* il est possible d'utiliser un type avant sa validation final, par exemple en
             * paramètre d'une fonction de rappel qui est membre de la structure */
            if ((copie->type->drapeaux & TYPE_FUT_VALIDE) == 0 &&
                copie->type != contexte.union_ou_structure_courante()) {
                // saute l'expression pour ne plus revenir
                contexte.unite->index_courant += 1;
                compilatrice.libère_état_résolution_appel(expr->état_résolution_appel);
                copie->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
                return Attente::sur_type(copie->type);
            }
            expr->noeud_fonction_appelee = copie;
        }
        else {
            expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
            expr->type = const_cast<Type *>(candidate->type);

            for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
                if (expr->parametres_resolus[i] != nullptr) {
                    contexte.crée_transtypage_implicite_au_besoin(expr->parametres_resolus[i],
                                                                  candidate->transformations[i]);
                }
            }
            expr->noeud_fonction_appelee = const_cast<NoeudExpression *>(candidate->noeud_decl);

            if (!expr->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
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
        expr->type = const_cast<Type *>(candidate->type);
        expr->noeud_fonction_appelee = const_cast<NoeudStruct *>(
            expr->type->comme_type_type_de_donnees()
                ->type_connu->comme_type_polymorphique()
                ->structure);
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
        if (expr->type == nullptr) {
            expr->type = candidate->type->comme_type_fonction()->type_sortie;
        }

        applique_transformations(contexte, candidate, expr);

        auto expr_gauche = !expr->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION);
        if (!expr->type->est_type_rien() && expr_gauche) {
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
            ref->declaration_referee->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        }
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
        // le type du retour
        expr->type = TypeBase::RIEN;
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE) {
        if (!expr->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
            espace.rapporte_erreur(
                expr,
                "La valeur de l'expression de construction d'opaque n'est pas "
                "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                "ou l'utiliser comme type ?");
            return CodeRetourValidation::Erreur;
        }

        auto type_opaque = candidate->type->comme_type_opaque();
        if (type_opaque->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type_opaque = contexte.espace->compilatrice().typeuse.monomorphe_opaque(
                type_opaque->decl, candidate->exprs[0]->type);
        }
        else {
            for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
                contexte.crée_transtypage_implicite_au_besoin(expr->parametres_resolus[i],
                                                              candidate->transformations[i]);
            }
        }

        expr->type = const_cast<TypeOpaque *>(type_opaque);
        expr->aide_generation_code = CONSTRUIT_OPAQUE;
        expr->noeud_fonction_appelee = type_opaque->decl;
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE_DEPUIS_STRUCTURE) {
        if (!expr->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
            espace.rapporte_erreur(
                expr,
                "La valeur de l'expression de construction d'opaque n'est pas "
                "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                "ou l'utiliser comme type ?");
            return CodeRetourValidation::Erreur;
        }

        auto type_opaque = candidate->type->comme_type_opaque();
        for (auto i = 0; i < expr->parametres_resolus.taille(); ++i) {
            contexte.crée_transtypage_implicite_au_besoin(expr->parametres_resolus[i],
                                                          candidate->transformations[i]);
        }

        expr->type = const_cast<TypeOpaque *>(type_opaque);
        expr->aide_generation_code = CONSTRUIT_OPAQUE_DEPUIS_STRUCTURE;
        expr->noeud_fonction_appelee = type_opaque->decl;
    }
    else if (candidate->note == CANDIDATE_EST_MONOMORPHISATION_OPAQUE) {
        auto type_opaque = candidate->type->comme_type_opaque();
        auto type_opacifie = candidate->exprs[0]->type->comme_type_type_de_donnees();

        /* différencie entre Type($T) et Type(T) où T dans le deuxième cas est connu */
        if ((type_opacifie->type_connu->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
            type_opaque = contexte.espace->compilatrice().typeuse.monomorphe_opaque(
                type_opaque->decl, type_opacifie->type_connu);
        }

        expr->noeud_fonction_appelee = type_opaque->decl;
        expr->type = contexte.espace->compilatrice().typeuse.type_type_de_donnees(
            const_cast<TypeOpaque *>(type_opaque));
        expr->aide_generation_code = MONOMORPHE_TYPE_OPAQUE;
    }

#ifdef STATISTIQUES_DETAILLEES
    possède_erreur = false;
#endif

    compilatrice.libère_état_résolution_appel(expr->état_résolution_appel);

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
