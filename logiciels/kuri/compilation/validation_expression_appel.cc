/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "validation_expression_appel.hh"

#include <iostream>
#include <variant>

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "intrinseques.hh"
#include "monomorpheuse.hh"
#include "monomorphisations.hh"
#include "portee.hh"
#include "utilitaires/log.hh"
#include "validation_semantique.hh"

#include "utilitaires/chrono.hh"
#include "utilitaires/divers.hh"
#include "utilitaires/macros.hh"

/* ------------------------------------------------------------------------- */
/** \name Poids pour les arguments polymorphiques et variadiques.
 * Nous réduisons le poids de ces types d'arguments pour favoriser les fonctions ayant été déjà
 * monomorphées, ou celle de même nom n'ayant pas d'arguments variadiques (p.e. pour favoriser
 * fonc(z32)(rien) par rapport à fonc(...z32)(rien)).
 * \{ */

static constexpr double POIDS_POUR_ARGUMENT_POLYMORPHIQUE = 0.95;
static constexpr double POIDS_POUR_ARGUMENT_VARIADIQUE = 0.95;

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ErreurAppariement
 * \{ */

ErreurAppariement ErreurAppariement::mécomptage_arguments(const NoeudExpression *site,
                                                          int64_t nombre_requis,
                                                          int64_t nombre_obtenu)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::MÉCOMPTAGE_ARGS, site);
    erreur.nombre_arguments.nombre_obtenu = nombre_obtenu;
    erreur.nombre_arguments.nombre_requis = nombre_requis;
    return erreur;
}

ErreurAppariement ErreurAppariement::métypage_argument(const NoeudExpression *site,
                                                       Type *type_attendu,
                                                       Type *type_obtenu)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::MÉTYPAGE_ARG, site);
    erreur.type_arguments.type_attendu = type_attendu;
    erreur.type_arguments.type_obtenu = type_obtenu;
    return erreur;
}

ErreurAppariement ErreurAppariement::monomorphisation(
    const NoeudExpression *site, ErreurMonomorphisation erreur_monomorphisation)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::MONOMORPHISATION, site);
    erreur.erreur_monomorphisation = erreur_monomorphisation;
    return erreur;
}

ErreurAppariement ErreurAppariement::type_non_fonction(const NoeudExpression *site, Type *type)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::TYPE_N_EST_PAS_FONCTION, site);
    erreur.type = type;
    return erreur;
}

ErreurAppariement ErreurAppariement::ménommage_arguments(const NoeudExpression *site,
                                                         IdentifiantCode *ident)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::MÉNOMMAGE_ARG, site);
    erreur.nom_arg = ident;
    return erreur;
}

ErreurAppariement ErreurAppariement::nommage_manquant_pour_cuisson(const NoeudExpression *site)
{
    return crée_erreur(RaisonErreurAppariement::NOMMAGE_MANQUANT_POUR_CUISSON, site);
}

ErreurAppariement ErreurAppariement::renommage_argument(const NoeudExpression *site,
                                                        IdentifiantCode *ident)
{
    auto erreur = crée_erreur(RaisonErreurAppariement::RENOMMAGE_ARG, site);
    erreur.nom_arg = ident;
    return erreur;
}

ErreurAppariement ErreurAppariement::crée_erreur(RaisonErreurAppariement raison,
                                                 const NoeudExpression *site)
{
    ErreurAppariement erreur;
    erreur.raison = raison;
    erreur.site_erreur = site;
    return erreur;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name CandidateAppariement
 * \{ */

CandidateAppariement CandidateAppariement::appel_fonction(
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_APPEL_FONCTION,
                          poids,
                          noeud_decl,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::appel_fonction(
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto candidate = crée_candidate(CANDIDATE_EST_APPEL_FONCTION,
                                    poids,
                                    noeud_decl,
                                    type,
                                    std::move(exprs),
                                    std::move(transformations));
    candidate.items_monomorphisation = std::move(items_monomorphisation);
    return candidate;
}

CandidateAppariement CandidateAppariement::cuisson_fonction(
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto candidate = crée_candidate(CANDIDATE_EST_CUISSON_FONCTION,
                                    poids,
                                    noeud_decl,
                                    type,
                                    std::move(exprs),
                                    std::move(transformations));
    candidate.items_monomorphisation = std::move(items_monomorphisation);
    return candidate;
}

CandidateAppariement CandidateAppariement::appel_pointeur(
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_APPEL_POINTEUR,
                          poids,
                          noeud_decl,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::construction_chaine(
    double poids,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_CONSTRUCTION_CHAINE,
                          poids,
                          nullptr,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::initialisation_structure(
    double poids,
    const Type *noeud_decl,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_INITIALISATION_STRUCTURE,
                          poids,
                          noeud_decl,
                          nullptr,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::monomorphisation_structure(
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto candidate = crée_candidate(CANDIDATE_EST_MONOMORPHISATION_STRUCTURE,
                                    poids,
                                    noeud_decl,
                                    type,
                                    std::move(exprs),
                                    std::move(transformations));
    candidate.items_monomorphisation = std::move(items_monomorphisation);
    return candidate;
}

CandidateAppariement CandidateAppariement::type_polymorphique(
    double poids,
    Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_TYPE_POLYMORPHIQUE,
                          poids,
                          nullptr,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::type_polymorphique(
    double poids,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto candidate = crée_candidate(CANDIDATE_EST_TYPE_POLYMORPHIQUE,
                                    poids,
                                    nullptr,
                                    type,
                                    std::move(exprs),
                                    std::move(transformations));
    candidate.items_monomorphisation = std::move(items_monomorphisation);
    return candidate;
}

CandidateAppariement CandidateAppariement::appel_init_de(
    double poids,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_APPEL_INIT_DE,
                          poids,
                          nullptr,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::initialisation_opaque(
    double poids,
    Type const *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_INITIALISATION_OPAQUE,
                          poids,
                          type,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::monomorphisation_opaque(
    double poids,
    Type const *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    return crée_candidate(CANDIDATE_EST_MONOMORPHISATION_OPAQUE,
                          poids,
                          type,
                          type,
                          std::move(exprs),
                          std::move(transformations));
}

CandidateAppariement CandidateAppariement::crée_candidate(
    int note,
    double poids,
    const NoeudExpression *noeud_decl,
    const Type *type,
    kuri::tablet<NoeudExpression *, 10> &&exprs,
    kuri::tableau<TransformationType, int> &&transformations)
{
    CandidateAppariement candidate;
    candidate.note = note;
    candidate.poids_args = poids;
    candidate.type = type;
    candidate.exprs = std::move(exprs);
    candidate.transformations = std::move(transformations);
    candidate.noeud_decl = noeud_decl;
    return candidate;
}

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
    kuri::ensemblon<IdentifiantCode *, 10> m_args_rencontrés{};
    bool m_arguments_nommés = false;
    bool m_dernier_argument_est_variadique = false;
    bool m_est_variadique = false;
    bool m_expansion_rencontrée = false;
    ChoseÀApparier m_chose_à_apparier{};
    int m_index = 0;
    int m_nombre_arg_variadiques_rencontrés = 0;

  public:
    ErreurAppariement erreur{};

    ApparieuseParams(ChoseÀApparier chose_à_apparier) : m_chose_à_apparier(chose_à_apparier)
    {
    }

    void ajoute_param(IdentifiantCode *ident,
                      NoeudExpression *valeur_défaut,
                      bool est_variadique,
                      int index = -1)
    {
        m_noms.ajoute(ident);

        // Ajoute uniquement la valeur défaut si le paramètre n'est pas variadique,
        // car le code d'appariement de type dépend de ce comportement.
        if (!est_variadique) {
            m_slots.ajoute(valeur_défaut);
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

            if (m_expansion_rencontrée) {
                if (m_nombre_arg_variadiques_rencontrés != 0) {
                    erreur = ErreurAppariement::expansion_variadique_post_argument(expr);
                }
                else {
                    erreur = ErreurAppariement::multiple_expansions_variadiques(expr);
                }
                return false;
            }

            m_expansion_rencontrée = true;
        }

        if (ident) {
            m_arguments_nommés = true;

            auto index_param = 0l;

            POUR (m_noms) {
                if (ident == it) {
                    break;
                }

                index_param += 1;
            }

            if (index_param >= m_noms.taille()) {
                erreur = ErreurAppariement::ménommage_arguments(expr_ident, ident);
                return false;
            }

            auto est_paramètre_variadique = index_param == m_noms.taille() - 1 && m_est_variadique;

            if ((m_args_rencontrés.possède(ident)) && !est_paramètre_variadique) {
                erreur = ErreurAppariement::renommage_argument(expr_ident, ident);
                return false;
            }

            m_dernier_argument_est_variadique = est_paramètre_variadique;

            m_args_rencontrés.insère(ident);

            if (m_dernier_argument_est_variadique || index_param >= m_slots.taille()) {
                if (m_expansion_rencontrée && m_nombre_arg_variadiques_rencontrés != 0) {
                    erreur = ErreurAppariement::argument_post_expansion_variadique(expr);
                    return false;
                }
                m_nombre_arg_variadiques_rencontrés += 1;
                ajoute_slot(expr);
            }
            else {
                remplis_slot(index_param, expr);
            }
        }
        else {
            if (m_arguments_nommés == true && m_dernier_argument_est_variadique == false) {
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
                if (m_expansion_rencontrée && m_nombre_arg_variadiques_rencontrés != 0) {
                    erreur = ErreurAppariement::argument_post_expansion_variadique(expr);
                    return false;
                }
                m_nombre_arg_variadiques_rencontrés += 1;
                m_args_rencontrés.insère(m_noms[m_noms.taille() - 1]);
                ajoute_slot(expr);
                m_index++;
            }
            else {
                m_args_rencontrés.insère(m_noms[m_index]);
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
            erreur.raison = RaisonErreurAppariement::ARGUMENTS_MANQUANTS;
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
    CANDIDATE_EST_DÉCLARATION,
    CANDIDATE_EST_ACCÈS_MEMBRE,
    CANDIDATE_EST_INIT_DE,
    CANDIDATE_EST_EXPRESSION_QUELCONQUE,
    CANDIDATE_EST_TYPE_CHAINE,
};

static auto supprime_doublons(kuri::tablet<NoeudDéclaration *, 10> &tablet) -> void
{
    kuri::ensemblon<NoeudDéclaration *, 10> doublons;
    kuri::tablet<NoeudDéclaration *, 10> résultat;

    POUR (tablet) {
        if (doublons.possède(it)) {
            continue;
        }
        doublons.insère(it);
        résultat.ajoute(it);
    }

    if (résultat.taille() != tablet.taille()) {
        tablet = résultat;
    }
}

/* Voir les autres commentaires à propos des doublons. */
static auto supprime_doublons(ListeCandidatesExpressionAppel &candidates)
{
    if (candidates.taille() <= 1) {
        return;
    }

    kuri::ensemblon<NoeudDéclaration *, TAILLE_CANDIDATES_DEFAUT> doublons;
    ListeCandidatesExpressionAppel résultat;

    POUR (candidates) {
        if (!it.decl->est_déclaration()) {
            résultat.ajoute(it);
            continue;
        }

        auto decl = it.decl->comme_déclaration();
        if (doublons.possède(decl)) {
            continue;
        }
        doublons.insère(decl);
        résultat.ajoute(it);
    }

    if (résultat.taille() != candidates.taille()) {
        candidates = résultat;
    }
}

static void ajoute_candidate_pour_déclaration(ListeCandidatesExpressionAppel &candidates,
                                              NoeudDéclaration *decl)
{
    if (decl->est_déclaration_constante()) {
        auto constante = decl->comme_déclaration_constante();
        if (constante->valeur_expression.est_fonction()) {
            decl = constante->valeur_expression.fonction();
        }
        else if (constante->valeur_expression.est_type()) {
            decl = constante->valeur_expression.type();
        }
    }
    candidates.ajoute({CANDIDATE_EST_DÉCLARATION, decl});
}

static void trouve_candidates_pour_expression(
    Sémanticienne &contexte,
    EspaceDeTravail &espace,
    NoeudExpression *appelée,
    Fichier const *fichier,
    kuri::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT> &candidates)
{
    auto modules_visités = kuri::ensemblon<Module const *, 10>();
    auto déclarations = kuri::tablet<NoeudDéclaration *, 10>();
    trouve_declarations_dans_bloc_ou_module(déclarations,
                                            modules_visités,
                                            fichier->module,
                                            appelée->bloc_parent,
                                            appelée->ident,
                                            fichier);

    if (contexte.fonction_courante()) {
        auto fonction_courante = contexte.fonction_courante();

        if (fonction_courante->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
            auto site_monomorphisation = fonction_courante->site_monomorphisation;
            assert_rappel(site_monomorphisation->lexème,
                          [&]() { dbg() << erreur::imprime_site(espace, appelée); });
            auto fichier_site = espace.compilatrice().fichier(
                site_monomorphisation->lexème->fichier);

            if (fichier_site != fichier) {
                auto anciennes_déclarations = déclarations;
                auto anciens_modules_visités = modules_visités;
                trouve_declarations_dans_bloc_ou_module(déclarations,
                                                        modules_visités,
                                                        fichier_site->module,
                                                        site_monomorphisation->bloc_parent,
                                                        appelée->ident,
                                                        fichier_site);

                /* L'expansion d'opérateurs pour les boucles « pour » ne réinitialise pas les
                 * blocs parents de toutes les expressions nous faisant potentiellement
                 * revisiter et réajouter les déclarations du bloc du module où l'opérateur fut
                 * défini. À FAIRE : pour l'instant nous supprimons les doublons mais nous
                 * devrons proprement gérer tout ça pour éviter de perdre du temps. */
                supprime_doublons(déclarations);
            }
        }
    }

    POUR (déclarations) {
        // on peut avoir des expressions du genre inverse := inverse(matrice),
        // À FAIRE : si nous enlevons la vérification du drapeau EST_GLOBALE, la compilation
        // est bloquée dans une boucle infinie, il nous faudra un état pour dire qu'aucune
        // candidate n'a été trouvée
        if (it->genre == GenreNoeud::DÉCLARATION_VARIABLE) {
            if (it->lexème->fichier == appelée->lexème->fichier &&
                it->lexème->ligne >= appelée->lexème->ligne &&
                !it->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                continue;
            }
        }

        ajoute_candidate_pour_déclaration(candidates, it);
    }
}

static ResultatPoidsTransformation apparie_type_paramètre_appel_fonction(
    NoeudExpression const *slot, Type const *type_du_paramètre, Type const *type_de_l_expression)
{
    if (type_du_paramètre->est_type_variadique()) {
        /* Si le paramètre est variadique, utilise le type pointé pour vérifier la compatibilité,
         * sinon nous apparierons, par exemple, un « z32 » avec « ...z32 ».
         */
        type_du_paramètre = type_déréférencé_pour(type_du_paramètre);

        if (type_du_paramètre == nullptr) {
            /* Pour les fonctions variadiques externes, nous acceptons tous les types. */
            if (type_de_l_expression->est_type_entier_constant()) {
                return PoidsTransformation{
                    TransformationType(TypeTransformation::CONVERTI_ENTIER_CONSTANT,
                                       TypeBase::Z32),
                    1.0};
            }
            return PoidsTransformation{TransformationType(), 1.0};
        }

        if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
            /* Pour les expansions variadiques, nous devons également utiliser le type pointé. */
            type_de_l_expression = type_déréférencé_pour(type_de_l_expression);
        }
    }

    return vérifie_compatibilité(type_du_paramètre, type_de_l_expression, slot);
}

static void crée_tableau_args_variadiques(Sémanticienne &contexte,
                                          Lexème const *lexème,
                                          kuri::tablet<NoeudExpression *, 10> &slots,
                                          int nombre_args,
                                          Type *type_données_argument_variadique)
{
    auto index_premier_var_arg = nombre_args - 1;
    if (slots.taille() == nombre_args &&
        slots[index_premier_var_arg]->est_expansion_variadique()) {
        return;
    }

    /* Pour les fonctions variadiques interne, nous créons un tableau
     * correspondant au types des arguments. */
    auto noeud_tableau = contexte.donne_assembleuse()->crée_args_variadiques(lexème);

    noeud_tableau->type = type_données_argument_variadique;
    // @embouteillage, ceci gaspille également de la mémoire si la candidate n'est pas
    // sélectionné
    noeud_tableau->expressions.réserve(static_cast<int>(slots.taille()) - index_premier_var_arg);

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

static void applique_transformations(Sémanticienne &contexte,
                                     CandidateAppariement const *candidate,
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
        contexte.crée_transtypage_implicite_au_besoin(expr->paramètres_résolus[i],
                                                      candidate->transformations[i]);
    }

    /* les drapeaux pour les arguments variadics */
    if (!candidate->exprs.est_vide() &&
        candidate->exprs.back()->genre == GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES) {
        auto noeud_tableau = candidate->exprs.back()->comme_args_variadiques();

        for (auto j = 0; i < nombre_args_variadics; ++i, ++j) {
            contexte.crée_transtypage_implicite_au_besoin(noeud_tableau->expressions[j],
                                                          candidate->transformations[i]);
        }
    }
}

static RésultatAppariement apparie_construction_chaine(
    Sémanticienne &contexte,
    NoeudExpressionAppel const *b,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (args.taille() != 2) {
        return ErreurAppariement::mécomptage_arguments(b, 2, args.taille());
    }

    if (args[0].expr->type != TypeBase::PTR_Z8) {
        return ErreurAppariement::métypage_argument(
            args[0].expr, TypeBase::PTR_Z8, args[0].expr->type);
    }

    auto résultat = cherche_transformation_pour_transtypage(args[1].expr->type, TypeBase::Z64);
    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);
    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        return ErreurAppariement::métypage_argument(
            args[1].expr, TypeBase::Z64, args[1].expr->type);
    }

    auto transformations = kuri::tableau<TransformationType, int>(2);
    transformations[0] = TransformationType(TypeTransformation::INUTILE);
    transformations[1] = transformation;

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.ajoute(args[0].expr);
    exprs.ajoute(args[1].expr);

    return CandidateAppariement::construction_chaine(
        1.0, TypeBase::CHAINE, std::move(exprs), std::move(transformations));
}

static RésultatAppariement apparie_appel_pointeur(
    Sémanticienne &contexte,
    NoeudExpressionAppel const *b,
    NoeudExpression const *decl_pointeur_fonction,
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
    for (auto i = 0; i < type_fonction->types_entrées.taille(); ++i) {
        auto type_prm = type_fonction->types_entrées[i];
        fonction_variadique |= type_prm->est_type_variadique();
        apparieuse.ajoute_param(nullptr, nullptr, type_prm->est_type_variadique());
    }

    if (args.taille() > type_fonction->types_entrées.taille() && !fonction_variadique) {
        return ErreurAppariement::mécomptage_arguments(
            b, type_fonction->types_entrées.taille(), args.taille());
    }

    POUR (args) {
        if (!apparieuse.ajoute_expression(it.ident, it.expr, it.expr_ident)) {
            return apparieuse.erreur;
        }
    }

    if (!apparieuse.tous_les_slots_sont_remplis()) {
        return ErreurAppariement::mécomptage_arguments(
            b, type_fonction->types_entrées.taille(), args.taille());
    }

    auto &slots = apparieuse.slots();
    auto transformations = kuri::tableau<TransformationType, int>(
        static_cast<int>(slots.taille()));
    auto poids_args = 1.0;

    /* Validation des types passés en paramètre. */
    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_param = std::min(
            i, static_cast<int64_t>(type_fonction->types_entrées.taille() - 1));
        auto slot = slots[i];
        auto type_prm = type_fonction->types_entrées[static_cast<int>(index_param)];
        auto type_enf = slot->type;

        auto résultat = apparie_type_paramètre_appel_fonction(slot, type_prm, type_enf);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto poids_xform = std::get<PoidsTransformation>(résultat);
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
            return ErreurAppariement::métypage_argument(slot, type_prm, type_enf);
        }

        transformations[static_cast<int>(i)] = poids_xform.transformation;
    }

    if (fonction_variadique) {
        auto nombre_args = type_fonction->types_entrées.taille();
        auto dernier_type_paramètre =
            type_fonction->types_entrées[type_fonction->types_entrées.taille() - 1];
        auto type_données_argument_variadique = type_déréférencé_pour(dernier_type_paramètre);
        crée_tableau_args_variadiques(
            contexte, b->lexème, slots, nombre_args, type_données_argument_variadique);
    }

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.réserve(type_fonction->types_entrées.taille());

    POUR (slots) {
        exprs.ajoute(it);
    }

    return CandidateAppariement::appel_pointeur(
        poids_args, decl_pointeur_fonction, type, std::move(exprs), std::move(transformations));
}

static bool type_est_compatible_pour_init_de(Type const *type_initialisé,
                                             Type const *type_expression)
{
    if (type_initialisé == type_expression) {
        return true;
    }

    if (type_initialisé == TypeBase::PTR_RIEN &&
        (type_expression->est_type_pointeur() || type_expression->est_type_fonction())) {
        return true;
    }

    return false;
}

static RésultatAppariement apparie_appel_init_de(
    NoeudExpression const *expr, kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (args.taille() > 1) {
        return ErreurAppariement::mécomptage_arguments(expr, 1, args.taille());
    }

    auto type_fonction = expr->type->comme_type_fonction();
    auto type_pointeur = type_fonction->types_entrées[0];

    auto type_initialisé = type_pointeur->comme_type_pointeur()->type_pointé;
    auto type_expression = args[0].expr->type;

    if (!type_expression->est_type_pointeur()) {
        /* À FAIRE : nous pourrions avoir une erreur disant 'un pointeur est requis pour init_de()'
         */
        return ErreurAppariement::métypage_argument(
            args[0].expr, type_pointeur, args[0].expr->type);
    }

    type_expression = type_expression->comme_type_pointeur()->type_pointé;

    if (!type_est_compatible_pour_init_de(type_initialisé, type_expression)) {
        return ErreurAppariement::métypage_argument(
            args[0].expr, type_pointeur, args[0].expr->type);
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(args[0].expr);

    auto transformations = kuri::tableau<TransformationType, int>(1);

    if (type_initialisé != type_expression) {
        transformations[0] = TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                type_pointeur};
    }
    else {
        transformations[0] = TransformationType{TypeTransformation::INUTILE};
    }

    return CandidateAppariement::appel_init_de(
        1.0, expr->type, std::move(exprs), std::move(transformations));
}

/* ************************************************************************** */

static RésultatAppariement apparie_appel_fonction_pour_cuisson(
    EspaceDeTravail &espace,
    Sémanticienne &contexte,
    NoeudExpressionAppel const *expr,
    NoeudDéclarationEntêteFonction const *decl,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        return ErreurAppariement::métypage_argument(expr, nullptr, nullptr);
    }

    auto monomorpheuse = Monomorpheuse(espace, decl);

    // À FAIRE : vérifie que toutes les constantes ont été renseignées.
    // À FAIRE : gère proprement la validation du type de la constante

    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation;
    auto noms_rencontrés = kuri::ensemblon<IdentifiantCode *, 10>();
    POUR (args) {
        if (!it.ident) {
            return ErreurAppariement::nommage_manquant_pour_cuisson(it.expr);
        }

        if (noms_rencontrés.possède(it.ident)) {
            return ErreurAppariement::renommage_argument(it.expr, it.ident);
        }
        noms_rencontrés.insère(it.ident);

        auto item = monomorpheuse.item_pour_ident(it.ident);
        if (item == nullptr) {
            return ErreurAppariement::ménommage_arguments(it.expr, it.ident);
        }

        auto type = it.expr->type->comme_type_type_de_données();
        items_monomorphisation.ajoute(
            {it.ident, type, ValeurExpression(), GenreItem::TYPE_DE_DONNÉES});
    }

    return CandidateAppariement::cuisson_fonction(
        1.0, decl, nullptr, {}, {}, std::move(items_monomorphisation));
}

static RésultatAppariement apparie_appel_fonction(
    Sémanticienne &contexte,
    NoeudExpressionAppel const *expr,
    NoeudDéclarationEntêteFonction const *decl,
    kuri::tableau<IdentifiantEtExpression> const &args,
    Monomorpheuse *monomorpheuse)
{
    auto const nombre_args = decl->params.taille();
    auto const est_variadique = decl->possède_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE);
    auto const est_externe = decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE);

    if (!est_variadique && (args.taille() > nombre_args)) {
        return ErreurAppariement::mécomptage_arguments(expr, nombre_args, args.taille());
    }

    if (nombre_args == 0 && args.taille() == 0) {
        return CandidateAppariement::appel_fonction(1.0, decl, decl->type, {}, {});
    }

    /* mise en cache des paramètres d'entrées, accéder à cette fonction se voit dans les profiles
     */
    kuri::tablet<BaseDéclarationVariable *, 10> paramètres_entree;
    for (auto i = 0; i < decl->params.taille(); ++i) {
        paramètres_entree.ajoute(decl->parametre_entree(i));
    }

    auto fonction_variadique_interne = est_variadique && !est_externe;
    auto apparieuse_params = ApparieuseParams(est_externe ? ChoseÀApparier::FONCTION_EXTERNE :
                                                            ChoseÀApparier::FONCTION_INTERNE);
    // slots.redimensionne(nombre_args - decl->est_variadique);

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = paramètres_entree[i];
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
            return std::get<Attente>(résultat_monomorphisation);
        }
        if (std::holds_alternative<ErreurMonomorphisation>(résultat_monomorphisation)) {
            return ErreurAppariement::monomorphisation(
                expr, std::get<ErreurMonomorphisation>(résultat_monomorphisation));
        }
    }

    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<int64_t>(decl->params.taille() - 1));
        auto param = paramètres_entree[index_arg];
        auto arg = param;
        auto slot = slots[i];

        if (slot == param->expression) {
            continue;
        }

        auto type_de_l_expression = slot->type;
        auto type_du_paramètre = arg->type;
        auto poids_polymorphique = POIDS_POUR_ARGUMENT_POLYMORPHIQUE;

        if (arg->type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            auto résultat_type = monomorpheuse->résoud_type_final(param->expression_type);
            if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                return ErreurAppariement::monomorphisation(
                    expr, std::get<ErreurMonomorphisation>(résultat_type));
            }

            auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
            type_du_paramètre = type_apparié_pesé.type;
            poids_polymorphique *= type_apparié_pesé.poids_appariement;
        }

        auto résultat = apparie_type_paramètre_appel_fonction(
            slot, type_du_paramètre, type_de_l_expression);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto poids_xform = std::get<PoidsTransformation>(résultat);
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
        if (arg->type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            poids_pour_enfant *= poids_polymorphique;
        }

        poids_args *= poids_pour_enfant;

        if (poids_args == 0.0) {
            return ErreurAppariement::métypage_argument(
                slot, type_du_paramètre, type_de_l_expression);
        }

        transformations[i] = poids_xform.transformation;
    }

    if (fonction_variadique_interne) {
        auto dernier_paramètre = decl->parametre_entree(decl->params.taille() - 1);
        auto dernier_type_paramètre = dernier_paramètre->type;
        auto type_données_argument_variadique = type_déréférencé_pour(dernier_type_paramètre);
        auto poids_variadique = POIDS_POUR_ARGUMENT_VARIADIQUE;

        if (type_données_argument_variadique->possède_drapeau(
                DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            auto résultat_type = monomorpheuse->résoud_type_final(
                dernier_paramètre->expression_type);
            if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                return ErreurAppariement::monomorphisation(
                    expr, std::get<ErreurMonomorphisation>(résultat_type));
            }

            auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
            type_données_argument_variadique = type_apparié_pesé.type;
            poids_variadique *= type_apparié_pesé.poids_appariement;

            /* La résolution de type retourne un type variadique, mais nous voulons le type pointé.
             */
            type_données_argument_variadique = type_déréférencé_pour(
                type_données_argument_variadique);
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
            contexte, expr->lexème, slots, nombre_args, type_données_argument_variadique);
    }

    auto exprs = kuri::tablet<NoeudExpression *, 10>();
    exprs.réserve(slots.taille());

    auto transformations_ = kuri::tableau<TransformationType, int>();
    transformations_.réserve(static_cast<int>(transformations.taille()));

    // Il faut supprimer de l'appel les constantes correspondant aux valeur polymorphiques.
    for (auto i = int64_t(0); i < slots.taille(); ++i) {
        auto index_arg = std::min(i, static_cast<int64_t>(decl->params.taille() - 1));
        auto param = paramètres_entree[index_arg];

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

static RésultatAppariement apparie_appel_fonction(
    EspaceDeTravail &espace,
    Sémanticienne &contexte,
    NoeudExpressionAppel const *expr,
    NoeudDéclarationEntêteFonction const *decl,
    kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (expr->possède_drapeau(DrapeauxNoeud::POUR_CUISSON)) {
        return apparie_appel_fonction_pour_cuisson(espace, contexte, expr, decl, args);
    }

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        Monomorpheuse monomorpheuse(espace, decl);
        return apparie_appel_fonction(contexte, expr, decl, args, &monomorpheuse);
    }

    return apparie_appel_fonction(contexte, expr, decl, args, nullptr);
}

/* ************************************************************************** */

static bool est_expression_type_ou_valeur_polymorphique(const NoeudExpression *expr)
{
    if (expr->est_référence_déclaration()) {
        auto ref_decl = expr->comme_référence_déclaration();

        if (ref_decl->déclaration_référée->possède_drapeau(
                DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            return true;
        }
    }

    if (expr->type->est_type_type_de_données()) {
        auto type_connu = expr->type->comme_type_type_de_données()->type_connu;

        if (type_connu->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            return true;
        }
    }

    return false;
}

static RésultatAppariement apparie_construction_type_composé_polymorphique(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    kuri::tableau<IdentifiantEtExpression> const &arguments,
    NoeudDéclarationType const *déclaration_type_composé,
    NoeudBloc const *params_polymorphiques)
{
    if (expr->paramètres.taille() != params_polymorphiques->nombre_de_membres()) {
        return ErreurAppariement::mécomptage_arguments(
            expr, params_polymorphiques->nombre_de_membres(), expr->paramètres.taille());
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
        apparieuse_params.erreur.noeud_decl = déclaration_type_composé;
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
                dbg() << "Les types polymorphiques ne sont pas supportés sur les "
                         "structures pour le moment.";
            });
            continue;
        }

        if (est_expression_type_ou_valeur_polymorphique(it)) {
            est_type_argument_polymorphique = true;
            continue;
        }

        // vérifie la contrainte
        if (param->type->est_type_type_de_données()) {
            if (!it->type->est_type_type_de_données()) {
                return ErreurAppariement::métypage_argument(it, param->type, it->type);
            }

            items_monomorphisation.ajoute(
                {param->ident, it->type, ValeurExpression(), GenreItem::TYPE_DE_DONNÉES});
        }
        else {
            if (!(it->type == param->type ||
                  (it->type->est_type_entier_constant() && est_type_entier(param->type)))) {
                return ErreurAppariement::métypage_argument(it, param->type, it->type);
            }

            auto valeur = évalue_expression(espace.compilatrice(), it->bloc_parent, it);

            if (valeur.est_erroné) {
                espace.rapporte_erreur(it, "La valeur n'est pas constante");
            }

            items_monomorphisation.ajoute(
                {param->ident, param->type, valeur.valeur, GenreItem::VALEUR});
        }
    }

    if (est_type_argument_polymorphique) {
        auto type_poly = espace.compilatrice().typeuse.type_type_de_donnees(
            const_cast<NoeudDéclarationType *>(déclaration_type_composé));

        return CandidateAppariement::type_polymorphique(
            1.0, type_poly, {}, {}, std::move(items_monomorphisation));
    }

    return CandidateAppariement::monomorphisation_structure(
        1.0,
        déclaration_type_composé,
        déclaration_type_composé->comme_type_composé(),
        {},
        {},
        std::move(items_monomorphisation));
}

static RésultatAppariement apparie_construction_type_composé(
    NoeudExpressionAppel const *expr,
    NoeudDéclarationType const *déclaration_type_composé,
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
            apparieuse_params.erreur.noeud_decl = déclaration_type_composé;
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
        if (index_membre == -1) {
            /* À FAIRE : meilleure erreur, ceci peut arriver pour les constructions invalides. */
            return ErreurAppariement::mécomptage_arguments(
                it, type_compose->membres.taille(), apparieuse_params.slots().taille());
        }
        auto &membre = type_compose->membres[index_membre];

        auto résultat = vérifie_compatibilité(membre.type, it->type, it);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto poids_xform = std::get<PoidsTransformation>(résultat);

        poids_appariement *= poids_xform.poids;

        if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE ||
            poids_appariement == 0.0) {
            return ErreurAppariement::métypage_argument(it, membre.type, it->type);
        }

        transformations[index_it] = poids_xform.transformation;
    }

    return CandidateAppariement::initialisation_structure(poids_appariement,
                                                          déclaration_type_composé,
                                                          std::move(apparieuse_params.slots()),
                                                          std::move(transformations));
}

static RésultatAppariement apparie_appel_structure(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    NoeudStruct const *decl_struct,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (decl_struct->est_polymorphe) {
        return apparie_construction_type_composé_polymorphique(
            espace, expr, arguments, decl_struct, decl_struct->bloc_constantes);
    }

    return apparie_construction_type_composé(
        expr, decl_struct, decl_struct->comme_type_composé(), arguments);
}

static RésultatAppariement apparie_construction_union(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    NoeudUnion const *decl_struct,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (decl_struct->est_polymorphe) {
        return apparie_construction_type_composé_polymorphique(
            espace, expr, arguments, decl_struct, decl_struct->bloc_constantes);
    }

    if (expr->paramètres.taille() > 1) {
        return ErreurAppariement::expression_extra_pour_union(expr);
    }

    if (expr->paramètres.taille() == 0) {
        return ErreurAppariement::expression_manquante_union(expr);
    }

    return apparie_construction_type_composé(
        expr, decl_struct, decl_struct->comme_type_composé(), arguments);
}

/* ************************************************************************** */

static RésultatAppariement apparie_construction_opaque_polymorphique(
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (arguments.taille() == 0) {
        /* Nous devons avoir au moins 1 argument pour monomorpher. */
        return ErreurAppariement::mécomptage_arguments(expr, 1, 0);
    }

    if (arguments.taille() > 1) {
        /* Nous devons avoir au plus 1 argument pour monomorpher. */
        return ErreurAppariement::mécomptage_arguments(expr, 1, arguments.taille());
    }

    auto arg = arguments[0].expr;
    if (arg->type->est_type_type_de_données()) {
        auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);
        return CandidateAppariement::monomorphisation_opaque(
            1.0, type_opaque, std::move(exprs), {});
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);
    return CandidateAppariement::initialisation_opaque(1.0, type_opaque, std::move(exprs), {});
}

static RésultatAppariement apparie_construction_opaque_depuis_structure(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    TypeStructure const *type_structure,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    auto résultat_appariement_structure = apparie_appel_structure(
        espace, expr, type_structure, arguments);

    if (std::holds_alternative<ErreurAppariement>(résultat_appariement_structure)) {
        return résultat_appariement_structure;
    }

    /* Change les données de la candidate pour être celles de l'opaque. */
    auto candidate = std::get<CandidateAppariement>(résultat_appariement_structure);
    candidate.note = CANDIDATE_EST_INITIALISATION_OPAQUE_DEPUIS_STRUCTURE;
    candidate.type = type_opaque;
    return candidate;
}

static RésultatAppariement apparie_construction_opaque(
    EspaceDeTravail &espace,
    NoeudExpressionAppel const *expr,
    TypeOpaque const *type_opaque,
    kuri::tableau<IdentifiantEtExpression> const &arguments)
{
    if (type_opaque->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        return apparie_construction_opaque_polymorphique(expr, type_opaque, arguments);
    }

    if (arguments.taille() == 0) {
        /* Nous devons avoir au moins un argument.
         * À FAIRE : la construction par défaut des types opaques requiers d'avoir une construction
         * par défaut des types simples afin de pouvoir les utiliser dans la simplification du
         * code. */
        return ErreurAppariement::mécomptage_arguments(expr, 1, 0);
    }

    auto type_opacifié = donne_type_opacifié_racine(type_opaque);

    if (arguments.taille() > 1) {
        if (!type_opacifié->est_type_structure()) {
            /* Un seul argument pour les opaques de structures. */
            return ErreurAppariement::mécomptage_arguments(expr, 1, arguments.taille());
        }

        auto type_structure = type_opacifié->comme_type_structure();
        return apparie_construction_opaque_depuis_structure(
            espace, expr, type_opaque, type_structure, arguments);
    }

    auto arg = arguments[0].expr;
    auto résultat = vérifie_compatibilité(type_opacifié, arg->type);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto poids_xform = std::get<PoidsTransformation>(résultat);

    if (poids_xform.transformation.type == TypeTransformation::IMPOSSIBLE) {
        /* Essaye de construire une structure depuis l'argument unique si l'opacifié est un type
         * structure. */
        if (type_opacifié->est_type_structure()) {
            auto type_structure = type_opacifié->comme_type_structure();
            return apparie_construction_opaque_depuis_structure(
                espace, expr, type_opaque, type_structure, arguments);
        }

        return ErreurAppariement::métypage_argument(arg, type_opaque->type_opacifié, arg->type);
    }

    auto exprs = kuri::crée_tablet<NoeudExpression *, 10>(arg);

    auto transformations = kuri::tableau<TransformationType, int>(1);
    transformations[0] = poids_xform.transformation;

    return CandidateAppariement::initialisation_opaque(
        poids_xform.poids, type_opaque, std::move(exprs), std::move(transformations));
}

/* ************************************************************************** */

static CodeRetourValidation trouve_candidates_pour_appel(
    EspaceDeTravail &espace,
    Sémanticienne &contexte,
    NoeudExpressionAppel const *expr,
    kuri::tableau<IdentifiantEtExpression> &args,
    ListeCandidatesExpressionAppel &candidates)
{
    auto appelée = expr->expression;
    auto fichier = espace.compilatrice().fichier(appelée->lexème->fichier);

    if (appelée->est_référence_déclaration()) {
        trouve_candidates_pour_expression(contexte, espace, appelée, fichier, candidates);
        return CodeRetourValidation::OK;
    }

    if (appelée->type && appelée->type->est_type_type_de_données()) {
        auto type_connu = appelée->type->comme_type_type_de_données()->type_connu;
        if (!type_connu) {
            contexte.rapporte_erreur("Impossible d'utiliser un « type_de_données » "
                                     "dans une expression d'appel",
                                     appelée);
            return CodeRetourValidation::Erreur;
        }

        if (type_connu->est_type_structure()) {
            ajoute_candidate_pour_déclaration(candidates, type_connu->comme_type_structure());
            return CodeRetourValidation::OK;
        }

        if (type_connu->est_type_union()) {
            ajoute_candidate_pour_déclaration(candidates, type_connu->comme_type_union());
            return CodeRetourValidation::OK;
        }

        if (type_connu->est_type_chaine()) {
            candidates.ajoute({CANDIDATE_EST_TYPE_CHAINE, type_connu->comme_type_chaine()});
            return CodeRetourValidation::OK;
        }

        contexte.rapporte_erreur("Impossible d'utiliser un « type_de_données » "
                                 "dans une expression d'appel",
                                 appelée);
        return CodeRetourValidation::Erreur;
    }

    if (appelée->genre == GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE) {
        auto accès = appelée->comme_référence_membre();

        if (accès->aide_génération_code == PEUT_ÊTRE_APPEL_UNIFORME) {
            auto référence = NoeudExpression();
            référence.lexème = accès->lexème;
            référence.bloc_parent = accès->bloc_parent;
            référence.ident = accès->ident;
            trouve_candidates_pour_expression(contexte, espace, &référence, fichier, candidates);
            accès->accédée->genre_valeur = GenreValeur::TRANSCENDANTALE;
            args.ajoute_au_début({nullptr, nullptr, accès->accédée});
            return CodeRetourValidation::OK;
        }

        if (accès->déclaration_référée) {
            auto accédée = accès->accédée->comme_référence_déclaration();
            auto déclaration_module = accédée->déclaration_référée->comme_déclaration_module();
            auto module = déclaration_module->module;
            auto declarations = kuri::tablet<NoeudDéclaration *, 10>();
            trouve_déclarations_dans_module(declarations, module, accès->ident, fichier);

            POUR (declarations) {
                ajoute_candidate_pour_déclaration(candidates, it);
            }
            return CodeRetourValidation::OK;
        }

        candidates.ajoute({CANDIDATE_EST_ACCÈS_MEMBRE, accès});
        return CodeRetourValidation::OK;
    }

    if (appelée->genre == GenreNoeud::EXPRESSION_INIT_DE) {
        candidates.ajoute({CANDIDATE_EST_INIT_DE, appelée});
        return CodeRetourValidation::OK;
    }

    if (appelée->type->est_type_fonction()) {
        candidates.ajoute({CANDIDATE_EST_EXPRESSION_QUELCONQUE, appelée});
        return CodeRetourValidation::OK;
    }

    contexte.rapporte_erreur("L'expression n'est pas de type fonction", appelée);
    return CodeRetourValidation::Erreur;
}

static std::optional<Attente> apparies_candidates(EspaceDeTravail &espace,
                                                  Sémanticienne &contexte,
                                                  NoeudExpressionAppel const *expr,
                                                  ÉtatRésolutionAppel *état)
{
    /* Réinitialise en cas d'attentes passées. */
    état->résultats.efface();

    POUR (état->liste_candidates) {
        if (it.quoi == CANDIDATE_EST_ACCÈS_MEMBRE ||
            it.quoi == CANDIDATE_EST_EXPRESSION_QUELCONQUE) {
            état->résultats.ajoute(apparie_appel_pointeur(contexte, expr, it.decl, état->args));
        }
        else if (it.quoi == CANDIDATE_EST_TYPE_CHAINE) {
            état->résultats.ajoute(apparie_construction_chaine(contexte, expr, état->args));
        }
        else if (it.quoi == CANDIDATE_EST_DÉCLARATION) {
            auto decl = it.decl;

            if (decl->est_type_structure()) {
                auto decl_struct = decl->comme_type_structure();

                if (!decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_type(decl_struct);
                }

                état->résultats.ajoute(
                    apparie_appel_structure(espace, expr, decl_struct, état->args));
            }
            else if (decl->est_type_union()) {
                auto decl_union = decl->comme_type_union();

                if (!decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_type(decl_union);
                }

                état->résultats.ajoute(
                    apparie_construction_union(espace, expr, decl_union, état->args));
            }
            else if (decl->est_type_opaque()) {
                auto decl_opaque = decl->comme_type_opaque();
                if (!decl_opaque->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_déclaration(decl_opaque);
                }
                état->résultats.ajoute(
                    apparie_construction_opaque(espace, expr, decl_opaque, état->args));
            }
            else if (decl->est_entête_fonction()) {
                auto decl_fonc = decl->comme_entête_fonction();

                if (!decl_fonc->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_déclaration(decl_fonc);
                }

                état->résultats.ajoute(
                    apparie_appel_fonction(espace, contexte, expr, decl_fonc, état->args));
            }
            else if (decl->est_base_déclaration_variable()) {
                auto type = decl->type;

                if (!decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                    return Attente::sur_déclaration(decl->comme_base_déclaration_variable());
                }

                /* Nous pouvons avoir une constante polymorphique ou un alias. */
                if (type->est_type_type_de_données()) {
                    auto type_de_données = decl->type->comme_type_type_de_données();
                    auto type_connu = type_de_données->type_connu;

                    if (!type_connu) {
                        état->résultats.ajoute(
                            ErreurAppariement::type_non_fonction(expr, type_de_données));
                    }
                    else if (type_connu->est_type_structure()) {
                        auto type_struct = type_connu->comme_type_structure();

                        état->résultats.ajoute(
                            apparie_appel_structure(espace, expr, type_struct, état->args));
                    }
                    else if (type_connu->est_type_union()) {
                        auto type_union = type_connu->comme_type_union();
                        état->résultats.ajoute(
                            apparie_construction_union(espace, expr, type_union, état->args));
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
                        apparie_appel_pointeur(contexte, expr, decl, état->args));
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
    }

    return {};
}

/* ************************************************************************** */

static NoeudBloc *bloc_constantes_pour(NoeudExpression const *noeud)
{
    if (noeud->est_entête_fonction()) {
        return noeud->comme_entête_fonction()->bloc_constantes;
    }
    if (noeud->est_déclaration_classe()) {
        return noeud->comme_déclaration_classe()->bloc_constantes;
    }
    assert_rappel(false, [&]() {
        dbg() << "[bloc_constantes_pour] Obtenu un noeud de genre " << noeud->genre;
    });
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
            trouve_dans_bloc_seul(bloc_constantes, it.ident)->comme_déclaration_constante();
        decl_constante->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
        decl_constante->drapeaux &= ~(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE |
                                      DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE);
        decl_constante->type = const_cast<Type *>(it.type);

        if (it.genre == GenreItem::VALEUR) {
            decl_constante->valeur_expression = it.valeur;
        }
    }

    monomorphisations->ajoute(items_monomorphisation, copie);
    return {copie, true};
}

static std::pair<NoeudDéclarationEntêteFonction *, bool> monomorphise_au_besoin(
    Sémanticienne &contexte,
    Compilatrice &compilatrice,
    EspaceDeTravail &espace,
    NoeudDéclarationEntêteFonction const *decl,
    NoeudExpression *site,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte.donne_assembleuse(),
                                                          decl,
                                                          decl->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto entête = copie->comme_entête_fonction();

    if (!copie_nouvelle) {
        return {entête, false};
    }

    entête->drapeaux_fonction |= DrapeauxNoeudFonction::EST_MONOMORPHISATION;
    entête->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_POLYMORPHIQUE;
    entête->site_monomorphisation = site;

    /* Supprime les valeurs polymorphiques.
     * À FAIRE : optimise en utilisant un drapeau sur l'entête pour dire que les paramètres
     * contiennent une déclaration de valeur ou de type polymorphique. */
    auto nouveau_params = kuri::tablet<NoeudExpression *, 6>();
    POUR (entête->params) {
        auto decl_constante = trouve_dans_bloc_seul(entête->bloc_constantes, it->ident);
        if (decl_constante) {
            continue;
        }

        nouveau_params.ajoute(it);
    }

    if (nouveau_params.taille() != entête->params.taille()) {
        POUR_INDEX (nouveau_params) {
            static_cast<void>(it);
            entête->params[index_it] = nouveau_params[index_it];
        }
        entête->params.redimensionne(int(nouveau_params.taille()));
    }

    compilatrice.gestionnaire_code->requiers_typage(&espace, entête);
    return {entête, true};
}

static NoeudDéclarationClasse *monomorphise_au_besoin(
    Sémanticienne &contexte,
    EspaceDeTravail &espace,
    NoeudDéclarationClasse const *decl_struct,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte.donne_assembleuse(),
                                                          decl_struct,
                                                          decl_struct->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto structure = copie->comme_déclaration_classe();

    if (!copie_nouvelle) {
        return structure;
    }

    structure->est_polymorphe = false;
    structure->est_monomorphisation = true;
    structure->polymorphe_de_base = decl_struct;

    espace.compilatrice().gestionnaire_code->requiers_typage(&espace, structure);

    return structure;
}

/* ************************************************************************** */

static bool requiers_valeur_droite(EspaceDeTravail &espace,
                                   IdentifiantCode const *nom,
                                   kuri::tableau_statique<IdentifiantEtExpression> args,
                                   int argument,
                                   kuri::chaine_statique nom_énum = "")
{
    auto expr = args[argument].expr;
    if (expr->genre_valeur == GenreValeur::DROITE) {
        return true;
    }

    static const kuri::chaine_statique compteurs[] = {
        "premier", "deuxième", "troisième", "quatrième", "cinquième", "sizième"};

    Enchaineuse enchaineuse;
    enchaineuse << "Le " << compteurs[argument] << " argument de « " << nom->nom
                << " » ne peut pas être une variable, vous devez utiliser une valeur ";

    if (nom_énum != "") {
        enchaineuse << "de " << nom_énum << " directement.";
    }
    else {
        enchaineuse << "constante.";
    }

    espace.rapporte_erreur(expr, enchaineuse.chaine());
    return false;
}

/* Cette fonction est utilisée pour valider les appels aux intrinsèques afin de faire en sorte
 * que les intrinsèques requérant des paramètres constants reçurent des valeurs constantes. */
static bool appel_fonction_est_valide(EspaceDeTravail &espace,
                                      NoeudDéclarationEntêteFonction const *fonction,
                                      kuri::tableau<IdentifiantEtExpression> const &args)
{
    if (!fonction->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
        return true;
    }

    if (fonction->ident == ID::intrinsèque_précharge) {
        if (!requiers_valeur_droite(espace, fonction->ident, args, 1, "RaisonPréchargement")) {
            return false;
        }

        if (!requiers_valeur_droite(
                espace, fonction->ident, args, 2, "TemporalitéPréchargement")) {
            return false;
        }

        return true;
    }

    if (fonction->ident == ID::intrinsèque_prédit_avec_probabilité) {
        return requiers_valeur_droite(espace, fonction->ident, args, 2);
    }

    if (fonction->ident == ID::atomique_charge || fonction->ident == ID::atomique_stocke) {
        if (!requiers_valeur_droite(espace, fonction->ident, args, 1, "OrdreMémoire")) {
            return false;
        }

        auto expr_ordre_mémoire = args[1].expr;
        auto résultat = évalue_expression(espace.compilatrice(), expr_ordre_mémoire);
        if (résultat.est_erroné) {
            espace.rapporte_erreur(expr_ordre_mémoire, résultat.message_erreur);
            return false;
        }

        auto valeur = résultat.valeur.entière();

        switch (OrdreMémoire(valeur)) {
            default:
            {
                auto message = enchaine("Le troisième argument de ",
                                        fonction->ident->nom,
                                        " doit être l'une des valeurs suivante "
                                        ":\n\tOrdreMémoire.RELAXÉ\n\tOrdreMémoire.CONSOMME\n\t"
                                        "OrdreMémoire.ACQUIÈRE\n\tOrdreMémoire.SEQ_CST");
                espace.rapporte_erreur(expr_ordre_mémoire, message);
                return false;
            }
            case OrdreMémoire::RELAXÉ:
            case OrdreMémoire::CONSOMME:
            case OrdreMémoire::ACQUIÈRE:
            case OrdreMémoire::SEQ_CST:
            {
                break;
            }
        }

        return true;
    }

    if (est_élément(fonction->ident,
                    ID::atomique_donne_puis_sst,
                    ID::atomique_donne_puis_ajt,
                    ID::atomique_donne_puis_et,
                    ID::atomique_donne_puis_ou,
                    ID::atomique_donne_puis_oux,
                    ID::atomique_donne_puis_net,
                    ID::atomique_échange)) {
        return requiers_valeur_droite(espace, fonction->ident, args, 2, "OrdreMémoire");
    }

    if (fonction->ident == ID::atomique_compare_échange) {
        if (!requiers_valeur_droite(espace, fonction->ident, args, 3, "")) {
            return false;
        }

        if (!requiers_valeur_droite(espace, fonction->ident, args, 4, "OrdreMémoire")) {
            return false;
        }

        if (!requiers_valeur_droite(espace, fonction->ident, args, 5, "OrdreMémoire")) {
            return false;
        }

        auto résultat_arg4 = évalue_expression(espace.compilatrice(), args[4].expr);
        if (résultat_arg4.est_erroné) {
            espace.rapporte_erreur(args[4].expr, résultat_arg4.message_erreur);
            return false;
        }

        auto résultat_arg5 = évalue_expression(espace.compilatrice(), args[5].expr);
        if (résultat_arg5.est_erroné) {
            espace.rapporte_erreur(args[5].expr, résultat_arg5.message_erreur);
            return false;
        }

        auto valeur_arg4 = résultat_arg4.valeur.entière();
        auto valeur_arg5 = résultat_arg5.valeur.entière();

        switch (OrdreMémoire(valeur_arg5)) {
            default:
            {
                break;
            }
            case OrdreMémoire::RELÂCHE:
            case OrdreMémoire::ACQUIÈRE_RELÂCHE:
            {
                auto message = enchaine(
                    "Le dernier argument de ",
                    fonction->ident->nom,
                    " ne peut pas être OrdreMémoire.RELÂCHE ou OrdreMémoire.ACQUIÈRE_RELÂCHE.");
                espace.rapporte_erreur(args[5].expr, message);
                return false;
            }
        }

        static bool est_ordre_plus_fort_que[6][6] = {
            {false, false, false, false, false, false},
            {true, false, false, false, false, false},
            {true, true, false, false, false, false},
            {true, true, true, false, false, false},
            {true, true, true, true, false, false},
            {true, true, true, true, true, false},
        };

        if (est_ordre_plus_fort_que[valeur_arg5][valeur_arg4]) {
            auto message = enchaine("L'ordre mémoire pour l'échec de ",
                                    fonction->ident->nom,
                                    " ne peut pas être plus fort que celui pour le succès.");
            espace.rapporte_erreur(args[5].expr, message);
            return false;
        }

        return true;
    }

    if (fonction->ident == ID::atomique_barrière_fil ||
        fonction->ident == ID::atomique_barrière_signal) {
        return requiers_valeur_droite(espace, fonction->ident, args, 0, "OrdreMémoire");
    }

    if (fonction->ident == ID::atomique_toujours_sans_verrou) {
        return requiers_valeur_droite(espace, fonction->ident, args, 0, "");
    }

    return true;
}

/* ************************************************************************** */

static void rassemble_expressions_paramètres(NoeudExpressionAppel const *expr,
                                             ÉtatRésolutionAppel *état)
{
    auto &args = état->args;
    args.réserve(expr->paramètres.taille());
    POUR (expr->paramètres) {
        // l'argument est nommé
        if (it->est_assignation_variable()) {
            auto assign = it->comme_assignation_variable();
            auto nom_arg = assign->assignée;
            auto arg = assign->expression;

            args.ajoute({nom_arg->ident, nom_arg, arg});
        }
        else {
            args.ajoute({nullptr, nullptr, it});
        }
    }

    état->état = ÉtatRésolutionAppel::État::ARGUMENTS_RASSEMBLÉS;
}

static RésultatValidation crée_liste_candidates(NoeudExpressionAppel const *expr,
                                                ÉtatRésolutionAppel *état,
                                                EspaceDeTravail &espace,
                                                Sémanticienne &contexte)
{
    /* Si nous revenons ici suite à une attente nous devons recommencer donc vide la liste pour
     * éviter d'avoir des doublons. */
    état->liste_candidates.efface();

    auto code_retour = trouve_candidates_pour_appel(
        espace, contexte, expr, état->args, état->liste_candidates);

    if (code_retour == CodeRetourValidation::Erreur) {
        return code_retour;
    }

    if (état->liste_candidates.taille() == 0) {
        return Attente::sur_symbole(expr->expression);
    }

    supprime_doublons(état->liste_candidates);

    état->état = ÉtatRésolutionAppel::État::LISTE_CANDIDATES_CRÉÉE;
    return CodeRetourValidation::OK;
}

static RésultatValidation sélectionne_candidate(NoeudExpressionAppel const *expr,
                                                ÉtatRésolutionAppel *état,
                                                EspaceDeTravail &espace)
{
    POUR (état->résultats) {
        if (std::holds_alternative<Attente>(it)) {
            /* Si nous devons attendre sur quoi que ce soit, nous devrons recommencer
             * l'appariement. */
            état->résultats.efface();
            état->candidates.efface();
            état->erreurs.efface();
            état->état = ÉtatRésolutionAppel::État::LISTE_CANDIDATES_CRÉÉE;
            return std::get<Attente>(it);
        }

        if (std::holds_alternative<ErreurAppariement>(it)) {
            auto erreur = std::get<ErreurAppariement>(it);
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

        auto decl_candidate1 = état->candidates[0].noeud_decl;
        auto decl_candidate2 = état->candidates[1].noeud_decl;

        /* À FAIRE : il est possible d'avoir des doublons quand un module A importe deux modules B
         * et C, et que B crée une constante depuis un symbole de C, par exemple
         *
         * module A :
         *     importe B
         *     importe C
         *
         *     fonction_de_C()
         *
         * module B :
         *      C :: importe C
         *      fonction_de_C :: C.fonction_de_C
         *
         * module C :
         *      fonction_de_C :: fonc () {}
         */
        if (decl_candidate1 != decl_candidate2) {
            auto e = espace.rapporte_erreur(
                expr,
                "Je ne peux pas déterminer quelle fonction appeler car "
                "plusieurs fonctions correspondent à l'expression d'appel.");

            if (decl_candidate1 && decl_candidate2) {
                e.ajoute_message("Candidate possible :\n");
                e.ajoute_site(decl_candidate1);
                e.ajoute_message("Candidate possible :\n");
                e.ajoute_site(decl_candidate2);
            }
            else {
                e.ajoute_message("Erreur interne ! Aucun site pour les candidates possibles !");
            }

            return CodeRetourValidation::Erreur;
        }
    }

    état->candidate_finale = &état->candidates[0];
    état->état = ÉtatRésolutionAppel::État::CANDIDATE_SÉLECTIONNÉE;
    return CodeRetourValidation::OK;
}

static void copie_paramètres_résolus(NoeudExpressionAppel *appel,
                                     CandidateAppariement const *candidate,
                                     AssembleuseArbre *assembleuse)
{
    appel->paramètres_résolus.efface();
    appel->paramètres_résolus.réserve(static_cast<int>(candidate->exprs.taille()));
    POUR (candidate->exprs) {
        /* Copie les expressions par défaut des paramètres afin d'éviter d'avoir des conflits lors
         * de la canonicalisation du code où la même expression pourrait avoir différentes
         * substitution (p.e. pour les PositionCodeSource()).
         *
         * Une expression peut être nulle pour les expressions de construction de type. */
        if (it && it->possède_drapeau(DrapeauxNoeud::EST_EXPRESSION_DÉFAUT)) {
            /* À FAIRE : ajout d'un drapeau pour copier les données de validation sémantique,
             * et change noeud.adn pour utiliser ce drapeau afin que les copies ailleurs
             * ne copient que ce dont elles ont besoin. */
            auto nouveau_it = copie_noeud(
                assembleuse, it, it->bloc_parent, OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION);
            nouveau_it->drapeaux &= ~DrapeauxNoeud::EST_EXPRESSION_DÉFAUT;
            it = nouveau_it;
        }
        appel->paramètres_résolus.ajoute(it);
    }
}

RésultatValidation valide_appel_fonction(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         Sémanticienne &contexte,
                                         NoeudExpressionAppel *expr)
{
#ifdef STATISTIQUES_DETAILLEES
    auto possède_erreur = true;
    kuri::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possède_erreur) {
            contexte.donne_stats_typage().validation_appel.fusionne_entrée(
                {"tentatives râtées", temps});
        }
        contexte.donne_stats_typage().validation_appel.fusionne_entrée(
            {"valide_appel_fonction", temps});
    });
#endif
    if (!expr->état_résolution_appel) {
        expr->état_résolution_appel = compilatrice.crée_ou_donne_état_résolution_appel();
    }

    ÉtatRésolutionAppel &état = *expr->état_résolution_appel;

    if (état.état == ÉtatRésolutionAppel::État::RÉSOLUTION_NON_COMMENCÉE) {
        CHRONO_TYPAGE(contexte.donne_stats_typage().validation_appel,
                      VALIDATION_APPEL__PREPARE_ARGUMENTS);
        rassemble_expressions_paramètres(expr, &état);
    }

    if (état.état == ÉtatRésolutionAppel::État::ARGUMENTS_RASSEMBLÉS) {
        CHRONO_TYPAGE(contexte.donne_stats_typage().validation_appel,
                      VALIDATION_APPEL__TROUVE_CANDIDATES);

        auto résultat_liste = crée_liste_candidates(expr, &état, espace, contexte);
        if (!est_ok(résultat_liste)) {
            return résultat_liste;
        }
    }

    if (état.état == ÉtatRésolutionAppel::État::LISTE_CANDIDATES_CRÉÉE) {
        CHRONO_TYPAGE(contexte.donne_stats_typage().validation_appel,
                      VALIDATION_APPEL__APPARIE_CANDIDATES);
        auto attente_possible = apparies_candidates(espace, contexte, expr, &état);
        if (attente_possible.has_value()) {
            return attente_possible.value();
        }
        état.état = ÉtatRésolutionAppel::État::APPARIEMENT_CANDIDATES_FAIT;
    }

    if (état.état == ÉtatRésolutionAppel::État::APPARIEMENT_CANDIDATES_FAIT) {
        auto résultat = sélectionne_candidate(expr, &état, espace);
        if (!est_ok(résultat)) {
            return résultat;
        }
    }

    assert(état.état == ÉtatRésolutionAppel::État::CANDIDATE_SÉLECTIONNÉE);

    auto candidate = état.candidate_finale;

    // ------------
    // copie les données

    CHRONO_TYPAGE(contexte.donne_stats_typage().validation_appel, VALIDATION_APPEL__COPIE_DONNEES);

    copie_paramètres_résolus(expr, candidate, contexte.donne_assembleuse());

    if (candidate->note == CANDIDATE_EST_APPEL_FONCTION) {
        auto decl_fonction_appelée = candidate->noeud_decl->comme_entête_fonction();

        if (!appel_fonction_est_valide(espace, decl_fonction_appelée, état.args)) {
            return CodeRetourValidation::Erreur;
        }

        if (!candidate->items_monomorphisation.est_vide()) {
            auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
                contexte,
                compilatrice,
                espace,
                decl_fonction_appelée,
                expr,
                std::move(candidate->items_monomorphisation));

            if (doit_monomorpher ||
                !noeud_decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_déclaration(noeud_decl);
            }

            decl_fonction_appelée = noeud_decl;
        }
        else if (decl_fonction_appelée->possède_drapeau(DrapeauxNoeudFonction::EST_MACRO)) {
            auto copie_macro = copie_noeud(contexte.donne_assembleuse(),
                                           decl_fonction_appelée,
                                           decl_fonction_appelée->bloc_parent,
                                           OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION |
                                               OptionsCopieNoeud::COPIE_PARAMÈTRES_DANS_MEMBRES);

            auto entête_copie_macro = copie_macro->comme_entête_fonction();
            decl_fonction_appelée = entête_copie_macro;
        }

        // nous devons monomorpher (ou avoir les types monomorphés) avant de pouvoir faire ça
        auto type_fonc = decl_fonction_appelée->type->comme_type_fonction();
        auto type_sortie = type_fonc->type_sortie;

        auto expr_gauche = !expr->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION);
        if (!type_sortie->est_type_rien() && expr_gauche) {
            espace
                .rapporte_erreur(
                    expr,
                    "La valeur de retour de la fonction n'est pas utilisée. Il est important de "
                    "toujours utiliser les valeurs retournées par les fonctions, par exemple pour "
                    "ne "
                    "pas oublier de vérifier si une erreur existe.")
                .ajoute_message("La fonction a été déclarée comme retournant une valeur :\n")
                .ajoute_site(decl_fonction_appelée)
                .ajoute_conseil(
                    "si vous ne voulez pas utiliser la valeur de retour, vous pouvez utiliser « _ "
                    "» comme identifiant pour la capturer et l'ignorer :\n")
                .ajoute_message("\t_ := appel_mais_ignore_le_retourne()\n");
            return CodeRetourValidation::Erreur;
        }

        applique_transformations(contexte, candidate, expr);

        expr->noeud_fonction_appelée = const_cast<NoeudDéclarationEntêteFonction *>(
            decl_fonction_appelée);
        expr->noeud_fonction_appelée->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        /* Il est possible que le type ne fut pas initialisé, ou alors que le type est celui d'une
         * autre fonction assignée lors de la validation sémantique de l'expression (une référence
         * utilise le type de la première fonction trouvée, c'est ici que nous résolvons la bonne
         * fonction). Changeons alors le type pour éviter toute confusion dans les assertions ou
         * les étapes suivantes de compilation. */
        expr->expression->type = expr->noeud_fonction_appelée->type;

        if (expr->type == nullptr) {
            expr->type = type_sortie;
        }
    }
    else if (candidate->note == CANDIDATE_EST_CUISSON_FONCTION) {
        auto decl_fonction_appelée = candidate->noeud_decl->comme_entête_fonction();

        if (!candidate->items_monomorphisation.est_vide()) {
            auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
                contexte,
                compilatrice,
                espace,
                decl_fonction_appelée,
                expr,
                std::move(candidate->items_monomorphisation));

            if (doit_monomorpher ||
                !noeud_decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_déclaration(noeud_decl);
            }

            decl_fonction_appelée = noeud_decl;
        }

        expr->type = decl_fonction_appelée->type;
        expr->expression = const_cast<NoeudDéclarationEntêteFonction *>(decl_fonction_appelée);
        expr->expression->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    }
    else if (candidate->note == CANDIDATE_EST_MONOMORPHISATION_STRUCTURE) {
        auto decl_struct = candidate->noeud_decl->comme_déclaration_classe();

        auto copie = monomorphise_au_besoin(
            contexte, espace, decl_struct, std::move(candidate->items_monomorphisation));
        expr->type = espace.compilatrice().typeuse.type_type_de_donnees(copie);

        /* il est possible d'utiliser un type avant sa validation final, par exemple en
         * paramètre d'une fonction de rappel qui est membre de la structure */
        if (!copie->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE) &&
            copie != contexte.union_ou_structure_courante()) {
            // saute l'expression pour ne plus revenir
            contexte.donne_arbre()->index_courant += 1;
            compilatrice.libère_état_résolution_appel(expr->état_résolution_appel);
            copie->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
            return Attente::sur_type(copie);
        }
        expr->noeud_fonction_appelée = copie;
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_STRUCTURE) {
        expr->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
        expr->type = const_cast<Type *>(candidate->noeud_decl->comme_déclaration_type());

        for (auto i = 0; i < expr->paramètres_résolus.taille(); ++i) {
            if (expr->paramètres_résolus[i] != nullptr) {
                contexte.crée_transtypage_implicite_au_besoin(expr->paramètres_résolus[i],
                                                              candidate->transformations[i]);
            }
        }
        expr->noeud_fonction_appelée = const_cast<NoeudExpression *>(candidate->noeud_decl);

        if (!expr->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
            espace.rapporte_erreur(
                expr,
                "La valeur de l'expression de construction de structure n'est pas "
                "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                "ou l'utiliser comme type ?");
            return CodeRetourValidation::Erreur;
        }
    }
    else if (candidate->note == CANDIDATE_EST_TYPE_POLYMORPHIQUE) {
        expr->type = const_cast<Type *>(candidate->type);
        expr->noeud_fonction_appelée =
            (expr->type->comme_type_type_de_données()->type_connu->comme_déclaration_type());
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_POINTEUR) {
        if (expr->type == nullptr) {
            expr->type = candidate->type->comme_type_fonction()->type_sortie;
        }

        applique_transformations(contexte, candidate, expr);

        auto expr_gauche = !expr->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION);
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

        if (expr->expression->est_référence_déclaration()) {
            auto ref = expr->expression->comme_référence_déclaration();
            ref->déclaration_référée->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        }
    }
    else if (candidate->note == CANDIDATE_EST_CONSTRUCTION_CHAINE) {
        expr->type = TypeBase::CHAINE;
        expr->aide_génération_code = CONSTRUIT_CHAINE;
        applique_transformations(contexte, candidate, expr);
    }
    else if (candidate->note == CANDIDATE_EST_APPEL_INIT_DE) {
        // le type du retour
        expr->type = TypeBase::RIEN;
        applique_transformations(contexte, candidate, expr);
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE) {
        if (!expr->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
            espace.rapporte_erreur(
                expr,
                "La valeur de l'expression de construction d'opaque n'est pas "
                "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                "ou l'utiliser comme type ?");
            return CodeRetourValidation::Erreur;
        }

        auto type_opaque = candidate->type->comme_type_opaque();
        if (type_opaque->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            type_opaque = espace.compilatrice().typeuse.monomorphe_opaque(
                type_opaque, candidate->exprs[0]->type);
        }
        else {
            for (auto i = 0; i < expr->paramètres_résolus.taille(); ++i) {
                contexte.crée_transtypage_implicite_au_besoin(expr->paramètres_résolus[i],
                                                              candidate->transformations[i]);
            }
        }

        expr->type = const_cast<TypeOpaque *>(type_opaque);
        expr->aide_génération_code = CONSTRUIT_OPAQUE;
        expr->noeud_fonction_appelée = const_cast<TypeOpaque *>(type_opaque);
    }
    else if (candidate->note == CANDIDATE_EST_INITIALISATION_OPAQUE_DEPUIS_STRUCTURE) {
        if (!expr->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
            espace.rapporte_erreur(
                expr,
                "La valeur de l'expression de construction d'opaque n'est pas "
                "utilisée. Peut-être vouliez-vous l'assigner à quelque variable "
                "ou l'utiliser comme type ?");
            return CodeRetourValidation::Erreur;
        }

        auto type_opaque = candidate->type->comme_type_opaque();
        for (auto i = 0; i < expr->paramètres_résolus.taille(); ++i) {
            contexte.crée_transtypage_implicite_au_besoin(expr->paramètres_résolus[i],
                                                          candidate->transformations[i]);
        }

        expr->type = const_cast<TypeOpaque *>(type_opaque);
        expr->aide_génération_code = CONSTRUIT_OPAQUE_DEPUIS_STRUCTURE;
        expr->noeud_fonction_appelée = const_cast<TypeOpaque *>(type_opaque);
    }
    else if (candidate->note == CANDIDATE_EST_MONOMORPHISATION_OPAQUE) {
        auto type_opaque = candidate->type->comme_type_opaque();
        auto type_opacifie = candidate->exprs[0]->type->comme_type_type_de_données();

        /* différencie entre Type($T) et Type(T) où T dans le deuxième cas est connu */
        if (!type_opacifie->type_connu->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            type_opaque = espace.compilatrice().typeuse.monomorphe_opaque(
                type_opaque, type_opacifie->type_connu);
        }

        expr->noeud_fonction_appelée = const_cast<TypeOpaque *>(type_opaque);
        expr->type = espace.compilatrice().typeuse.type_type_de_donnees(
            const_cast<TypeOpaque *>(type_opaque));
        expr->aide_génération_code = MONOMORPHE_TYPE_OPAQUE;
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
    double résultat = poids_pour_origine(declaration_candidate.origine);

    /* Pondère avec le poids des expressions passées en paramètres. */
    POUR (declaration_candidate.expressions) {
        résultat *= it.poids
    }

    return résultat;
}

struct DeclarationCandidatePourAppel {
    NoeudExpression *site;
    OrigineDeclaration declaration;
};
#endif
