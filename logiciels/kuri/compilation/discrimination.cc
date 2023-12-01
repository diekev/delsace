/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "discrimination.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "structures/ensemblon.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "portee.hh"
#include "typage.hh"
#include "validation_semantique.hh"

// --------------------------------------------

static ResultatValidation valide_présence_membre(
    EspaceDeTravail *espace,
    NoeudExpression *expression,
    TypeCompose *type,
    kuri::ensemblon<IdentifiantCode const *, 16> const &membres_rencontrés)
{
    auto valeurs_manquantes = kuri::ensemble<kuri::chaine_statique>();

    POUR (type->membres) {
        if (!it.est_utilisable_pour_discrimination()) {
            continue;
        }

        if (membres_rencontrés.possède(it.nom)) {
            continue;
        }

        valeurs_manquantes.insère(it.nom->nom);
    }

    if (valeurs_manquantes.taille() != 0) {
        erreur::valeur_manquante_discr(*espace, expression, valeurs_manquantes);
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

// --------------------------------------------

ResultatValidation Sémanticienne::valide_discr_énum(NoeudDiscr *inst, Type *type)
{
    auto expression = inst->expression_discriminee;
    auto type_énum = static_cast<TypeEnum *>(type);
    inst->op = type_énum->table_opérateurs->opérateur_egt;

    auto membres_rencontrés = kuri::ensemblon<IdentifiantCode const *, 16>();
    inst->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;

        auto feuilles = expr_paire->comme_virgule();

        for (auto f : feuilles->expressions) {
            if (f->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                rapporte_erreur("expression inattendue dans la discrimination, seules les "
                                "références de déclarations sont supportées pour le moment",
                                f);
                return CodeRetourValidation::Erreur;
            }

            auto info_membre = donne_membre_pour_nom(type_énum, f->ident);

            if (!info_membre) {
                rapporte_erreur_membre_inconnu(inst, f, type_énum);
                return CodeRetourValidation::Erreur;
            }

            auto membre = info_membre->membre;

            if (membre.est_implicite()) {
                espace->rapporte_erreur(f,
                                        "Les membres implicites des énumérations ne peuvent être "
                                        "utilisés comme expression de discrimination");
                return CodeRetourValidation::Erreur;
            }

            if (membre.est_constant()) {
                espace->rapporte_erreur(f,
                                        "Les membres constants des énumérations ne peuvent être "
                                        "utilisés comme expression de discrimination");
                return CodeRetourValidation::Erreur;
            }

            if (membres_rencontrés.possède(membre.nom)) {
                rapporte_erreur("Redéfinition de l'expression", f);
                return CodeRetourValidation::Erreur;
            }

            membres_rencontrés.insere(membre.nom);
        }
    }

    if (inst->bloc_sinon == nullptr) {
        return valide_présence_membre(espace, expression, type_énum, membres_rencontrés);
    }

    return CodeRetourValidation::OK;
}

struct ExpressionTestDiscrimination {
    IdentifiantCode *ident = nullptr;
    NoeudExpression *référence = nullptr;
    NoeudExpressionAppel *est_expression_appel = nullptr;
};

static std::optional<ExpressionTestDiscrimination> expression_valide_discrimination(
    NoeudExpression *expression, bool opérande_appel_doit_être_référence)
{
    if (expression->est_reference_declaration()) {
        auto résultat = ExpressionTestDiscrimination{};
        résultat.ident = expression->ident;
        résultat.référence = expression->comme_reference_declaration();
        return résultat;
    }

    if (expression->est_reference_type()) {
        auto résultat = ExpressionTestDiscrimination{};
        résultat.ident = expression->ident;
        résultat.référence = expression->comme_reference_type();
        return résultat;
    }

    if (expression->est_appel()) {
        auto appel = expression->comme_appel();

        auto expression_ref = appel->expression;
        if (opérande_appel_doit_être_référence && (!expression_ref->est_reference_declaration() &&
                                                   !expression_ref->est_reference_type())) {
            return {};
        }

        auto résultat = ExpressionTestDiscrimination{};
        résultat.ident = expression_ref->ident;
        résultat.référence = expression_ref;
        résultat.est_expression_appel = appel;
        return résultat;
    }

    if (!opérande_appel_doit_être_référence) {
        /* Pour les unions anonymes. */
        auto résultat = ExpressionTestDiscrimination{};
        résultat.ident = expression->ident;
        résultat.référence = expression;
        return résultat;
    }

    return {};
}

static bool crée_variable_pour_expression_test(EspaceDeTravail *espace,
                                               AssembleuseArbre *assembleuse,
                                               NoeudExpression *expression,
                                               TypeUnion *type_union,
                                               NoeudBloc *bloc_parent,
                                               NoeudPaireDiscr *paire_discr,
                                               InformationMembreTypeCompose const &info_membre,
                                               NoeudExpressionAppel *appel,
                                               NoeudBloc *bloc_final_recherche_variable)
{
    if (appel->parametres.taille() != 1) {
        espace->rapporte_erreur(
            appel, "Trop d'expressions pour la destructuration de la valeur de l'union");
        return false;
    }

    auto param = appel->parametres[0];

    if (!param->est_reference_declaration()) {
        espace->rapporte_erreur(param,
                                "Le paramètre d'une destructuration de la valeur de l'union doit "
                                "être une référence de déclaration");
        return false;
    }

    auto déclaration_existante = trouve_dans_bloc(
        bloc_parent, param->ident, bloc_final_recherche_variable);

    if (déclaration_existante != nullptr) {
        espace->rapporte_erreur(param,
                                "Ne peut pas utiliser implicitement le membre car une "
                                "variable de ce nom existe déjà");
        return false;
    }

    /* À FAIRE(discr) : ceci n'est que pour la visite des noeuds dans le GestionnaireCode.
     */
    auto type_membre = info_membre.membre.type;
    param->type = type_membre;
    appel->type = type_membre;

    auto bloc_insertion = paire_discr->bloc;

    /* L'initialisation est une extraction de la valeur de l'union.
     * À FAIRE(discr) : ignore la vérification sur l'activité du membre. */
    auto initialisation_déclaration = assembleuse->crée_comme(param->lexeme);
    initialisation_déclaration->expression = expression;
    initialisation_déclaration->type = type_membre;
    initialisation_déclaration->transformation = {
        TypeTransformation::EXTRAIT_UNION, type_membre, info_membre.index_membre};

    auto déclaration_pour_expression = assembleuse->crée_declaration_variable(
        param->comme_reference_declaration(), initialisation_déclaration);
    déclaration_pour_expression->bloc_parent = bloc_insertion;
    déclaration_pour_expression->type = type_membre;
    déclaration_pour_expression->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    déclaration_pour_expression->genre_valeur = GenreValeur::TRANSCENDANTALE;

    bloc_insertion->expressions->pousse_front(déclaration_pour_expression);
    bloc_insertion->ajoute_membre(déclaration_pour_expression);

    paire_discr->variable_capturee = déclaration_pour_expression;

    return true;
}

ResultatValidation Sémanticienne::valide_discr_union(NoeudDiscr *inst, Type *type)
{
    auto expression = inst->expression_discriminee;
    auto type_union = type->comme_type_union();
    auto déclaration_union = type_union->decl;
    inst->op = TypeBase::Z32->table_opérateurs->opérateur_egt;

    if (déclaration_union->est_nonsure) {
        rapporte_erreur("« discr » ne peut prendre une union nonsûre", expression);
        return CodeRetourValidation::Erreur;
    }

    auto membres_rencontrés = kuri::ensemblon<IdentifiantCode const *, 16>();

    inst->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;
        auto feuilles = expr_paire->comme_virgule();

        if (feuilles->expressions.taille() != 1) {
            rapporte_erreur("Trop d'expression pour le bloc de discrimination de l'union.",
                            expr_paire);
            return CodeRetourValidation::Erreur;
        }

        auto feuille = feuilles->expressions[0];

        auto expression_valide = expression_valide_discrimination(feuille, true);
        if (!expression_valide.has_value()) {
            espace
                ->rapporte_erreur(feuille, "Expression invalide pour discriminer l'union anonyme")
                .ajoute_message("L'expression est de genre : ", feuille->genre, "\n");
            return CodeRetourValidation::Erreur;
        }

        auto info_membre = donne_membre_pour_nom(type_union, expression_valide->ident);

        if (!info_membre) {
            rapporte_erreur_membre_inconnu(inst, feuille, type_union);
            return CodeRetourValidation::Erreur;
        }

        auto membre = info_membre->membre;

        if (membre.est_implicite()) {
            espace->rapporte_erreur(feuille,
                                    "Les membres implicites des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membre.est_constant()) {
            espace->rapporte_erreur(feuille,
                                    "Les membres constants des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membres_rencontrés.possède(membre.nom)) {
            rapporte_erreur("Redéfinition de l'expression", feuille);
            return CodeRetourValidation::Erreur;
        }

        /* À FAIRE(discr) : ceci n'est que pour la simplification du code. */
        feuille->ident = membre.nom;

        membres_rencontrés.insere(membre.nom);

        /* Ajoute la variable dans le bloc suivant. */
        if (expression_valide->est_expression_appel) {
            if (membre.type->est_type_rien()) {
                espace->rapporte_erreur(expression_valide->est_expression_appel,
                                        "Impossible de capturer une variable depuis un membre "
                                        "d'union de type « rien »");
                return CodeRetourValidation::Erreur;
            }
            crée_variable_pour_expression_test(espace,
                                               m_tacheronne->assembleuse,
                                               expression,
                                               type_union,
                                               inst->bloc_parent,
                                               inst->paires_discr[i],
                                               info_membre.value(),
                                               expression_valide->est_expression_appel,
                                               fonction_courante()->bloc_constantes);
        }
    }

    if (inst->bloc_sinon == nullptr) {
        espace->rapporte_erreur(
            inst,
            "Les discriminations d'unions anonymes doivent avoir un bloc « sinon » afin de "
            "s'assurer que les valeurs non-initialisées soient prises en compte");
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

ResultatValidation Sémanticienne::valide_discr_union_anonyme(NoeudDiscr *inst, Type *type)
{
    auto type_union = type->comme_type_union();
    inst->op = TypeBase::Z32->table_opérateurs->opérateur_egt;
    inst->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

    auto membres_rencontrés = kuri::ensemblon<IdentifiantCode const *, 16>();

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;
        auto feuilles = expr_paire->comme_virgule();

        if (feuilles->expressions.taille() != 1) {
            rapporte_erreur("Trop d'expression pour le bloc de discrimination de l'union.",
                            expr_paire);
            return CodeRetourValidation::Erreur;
        }

        auto feuille = feuilles->expressions[0];

        auto expression_valide = expression_valide_discrimination(feuille, false);
        if (!expression_valide.has_value()) {
            espace->rapporte_erreur(feuille, "Attendu une référence à un membre de l'union")
                .ajoute_message("L'expression est de genre : ", feuille->genre, "\n");
            return CodeRetourValidation::Erreur;
        }

        auto référence_type = expression_valide->référence;

        Type *type_expr;
        if (resoud_type_final(référence_type, type_expr) == CodeRetourValidation::Erreur) {
            rapporte_erreur("Ne peut résoudre le type", référence_type);
            return CodeRetourValidation::Erreur;
        }

        expr_paire->type = type_expr;

        auto info_membre = donne_membre_pour_type(type_union, type_expr);
        if (!info_membre) {
            rapporte_erreur("Le type n'est pas membre de l'union", feuille);
            return CodeRetourValidation::Erreur;
        }

        auto membre = info_membre->membre;

        if (membre.est_implicite()) {
            espace->rapporte_erreur(feuille,
                                    "Les membres implicites des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membre.est_constant()) {
            espace->rapporte_erreur(feuille,
                                    "Les membres constants des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membres_rencontrés.possède(membre.nom)) {
            rapporte_erreur("Redéfinition de l'expression", feuille);
            return CodeRetourValidation::Erreur;
        }

        /* À FAIRE(discr) : meilleure structure pour stocker les informations de chaque expression,
         * ceci n'est que pour la simplification du code. */
        feuille->ident = membre.nom;

        membres_rencontrés.insere(membre.nom);

        /* Ajoute la variable dans le bloc suivant. */
        if (expression_valide->est_expression_appel) {
            if (référence_type->type->est_type_rien()) {
                espace->rapporte_erreur(expression_valide->est_expression_appel,
                                        "Impossible de capturer une variable depuis un membre "
                                        "d'union de type « rien »");
                return CodeRetourValidation::Erreur;
            }
            crée_variable_pour_expression_test(espace,
                                               m_tacheronne->assembleuse,
                                               inst->expression_discriminee,
                                               type_union,
                                               inst->bloc_parent,
                                               inst->paires_discr[i],
                                               info_membre.value(),
                                               expression_valide->est_expression_appel,
                                               fonction_courante()->bloc_constantes);
        }
    }

    if (inst->bloc_sinon == nullptr) {
        espace->rapporte_erreur(
            inst,
            "Les discriminations d'unions anonymes doivent avoir un bloc « sinon » afin de "
            "s'assurer que les valeurs non-initialisées soient prises en compte");
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

/* Pour les discriminations de valeurs scalaires, nous devons vérifier s'il y a un opérateur nous
 * permettant de comparer les valeurs avec l'expression.
 * Un bloc « sinon » est requis afin de s'assurer que tous les cas sont gérés.
 *
 * À FAIRE(discr) : comment vérifier la duplication de valeur testée ?
 * À FAIRE(discr) : stocke l'opérateur dans les paires de discriminations, afin de pouvoir utiliser
 * des opérateurs personnalisés (peut également être résolu via un système de définition de
 * conversion dans le langage).
 */
ResultatValidation Sémanticienne::valide_discr_scalaire(NoeudDiscr *inst, Type *type)
{
    auto type_pour_la_recherche = type;
    if (type->est_type_type_de_donnees()) {
        type_pour_la_recherche = m_compilatrice.typeuse.type_type_de_donnees_;
    }

    auto résultat = trouve_opérateur_pour_expression(
        *espace, nullptr, type_pour_la_recherche, type_pour_la_recherche, GenreLexeme::EGALITE);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    if (std::holds_alternative<bool>(résultat)) {
        espace
            ->rapporte_erreur(inst,
                              "Je ne peux pas valider l'expression de discrimination car "
                              "je n'arrive à trouver un opérateur de comparaison pour le "
                              "type de l'expression. ")
            .ajoute_message("Le type de l'expression est : ")
            .ajoute_message(chaine_type(type))
            .ajoute_message(".\n\n")
            .ajoute_conseil("Les discriminations ont besoin d'un opérateur « == » défini pour le "
                            "type afin de pouvoir comparer les valeurs,"
                            " donc si vous voulez utiliser une discrimination sur un type "
                            "personnalisé, vous pouvez définir l'opérateur comme ceci :\n\n"
                            "\topérateur == :: fonc (a: MonType, b: MonType) -> bool\n\t{\n\t\t "
                            "/* logique de comparaison */\n\t}\n");
        return CodeRetourValidation::Erreur;
    }

    auto candidat = std::get<OpérateurCandidat>(résultat);
    inst->op = candidat.op;

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;

        auto feuilles = expr_paire->comme_virgule();

        for (auto j = 0; j < feuilles->expressions.taille(); ++j) {
            auto feuille = feuilles->expressions[j];

            auto expression_valide = expression_valide_discrimination(feuille, false);
            if (!expression_valide.has_value()) {
                espace
                    ->rapporte_erreur(
                        feuille,
                        "Expression invalide pour la discrimination de la valeur scalaire")
                    .ajoute_message("L'expression est de genre : ", feuille->genre, "\n");
                return CodeRetourValidation::Erreur;
            }

            auto expression = expression_valide->référence;

            auto const résultat_transtype = crée_transtypage_implicite_si_possible(
                expression, type, RaisonTranstypageImplicite::POUR_TEST_DISCRIMINATION);
            if (!est_ok(résultat_transtype)) {
                return résultat_transtype;
            }
        }
    }

    if (inst->bloc_sinon == nullptr) {
        espace->rapporte_erreur(inst,
                                "Les discriminations de valeurs scalaires doivent "
                                "avoir un bloc « sinon »");
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

ResultatValidation Sémanticienne::valide_discrimination(NoeudDiscr *inst)
{
    auto expression = inst->expression_discriminee;
    auto type = expression->type;

    if (type->est_type_reference()) {
        crée_transtypage_implicite_au_besoin(inst->expression_discriminee,
                                             TypeTransformation::DEREFERENCE);
        type = type->comme_type_reference()->type_pointe;
    }

    if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
        return Attente::sur_type(type);
    }

    if (type->est_type_union() && type->comme_type_union()->est_anonyme) {
        return valide_discr_union_anonyme(inst, type);
    }

    if (type->est_type_union()) {
        return valide_discr_union(inst, type);
    }

    if (type->est_type_enum() || type->est_type_erreur()) {
        return valide_discr_énum(inst, type);
    }

    return valide_discr_scalaire(inst, type);
}
