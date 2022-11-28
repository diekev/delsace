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

static const TypeCompose::Membre *trouve_membre(TypeCompose const *type,
                                                IdentifiantCode const *ident)
{
    POUR (type->membres) {
        if (it.nom == ident) {
            return &it;
        }
    }

    return nullptr;
}

static const TypeCompose::Membre *trouve_membre(TypeCompose const *type, Type const *type_membre)
{
    POUR (type->membres) {
        if (it.type == type_membre) {
            return &it;
        }
    }

    return nullptr;
}

static inline long trouve_index_membre(TypeCompose const *type, TypeCompose::Membre const *membre)
{
    return (membre - &type->membres[0]);
}

static inline bool est_implicite(TypeCompose::Membre const &membre)
{
    return membre.drapeaux == TypeCompose::Membre::EST_IMPLICITE;
}

static inline bool est_constant(TypeCompose::Membre const &membre)
{
    return membre.drapeaux == TypeCompose::Membre::EST_CONSTANT;
}

static inline bool est_utilisable_pour_discrimination(TypeCompose::Membre const &membre)
{
    return !est_implicite(membre) && !est_constant(membre);
}

// --------------------------------------------

static ResultatValidation valide_presence_membre(
    EspaceDeTravail *espace,
    NoeudExpression *expression,
    TypeCompose *type,
    kuri::ensemblon<TypeCompose::Membre const *, 16> const &membres_rencontres)
{
    auto valeurs_manquantes = kuri::ensemble<kuri::chaine_statique>();

    POUR (type->membres) {
        if (!est_utilisable_pour_discrimination(it)) {
            continue;
        }

        if (membres_rencontres.possede(&it)) {
            continue;
        }

        valeurs_manquantes.insere(it.nom->nom);
    }

    if (valeurs_manquantes.taille() != 0) {
        erreur::valeur_manquante_discr(*espace, expression, valeurs_manquantes);
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

// --------------------------------------------

ResultatValidation ContexteValidationCode::valide_discr_enum(NoeudDiscr *inst, Type *type)
{
    auto expression = inst->expression_discriminee;
    auto type_enum = static_cast<TypeEnum *>(type);
    inst->op = type_enum->operateur_egt;

    auto membres_rencontres = kuri::ensemblon<TypeCompose::Membre const *, 16>();
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

            auto membre = trouve_membre(type_enum, f->ident);

            if (!membre) {
                rapporte_erreur_membre_inconnu(inst, f, type_enum);
                return CodeRetourValidation::Erreur;
            }

            if (est_implicite(*membre)) {
                espace->rapporte_erreur(f,
                                        "Les membres implicites des énumérations ne peuvent être "
                                        "utilisés comme expression de discrimination");
                return CodeRetourValidation::Erreur;
            }

            if (est_constant(*membre)) {
                espace->rapporte_erreur(f,
                                        "Les membres constants des énumérations ne peuvent être "
                                        "utilisés comme expression de discrimination");
                return CodeRetourValidation::Erreur;
            }

            if (membres_rencontres.possede(membre)) {
                rapporte_erreur("Redéfinition de l'expression", f);
                return CodeRetourValidation::Erreur;
            }

            membres_rencontres.insere(membre);
        }
    }

    if (inst->bloc_sinon == nullptr) {
        return valide_presence_membre(espace, expression, type_enum, membres_rencontres);
    }

    return CodeRetourValidation::OK;
}

struct ExpressionTestDiscrimination {
    IdentifiantCode *ident = nullptr;
    NoeudExpression *ref = nullptr;
    NoeudExpressionAppel *est_expression_appel = nullptr;
};

static std::optional<ExpressionTestDiscrimination> expression_valide_discrimination(
    NoeudExpression *expression)
{
    if (expression->est_reference_declaration()) {
        auto resultat = ExpressionTestDiscrimination{};
        resultat.ident = expression->ident;
        resultat.ref = expression->comme_reference_declaration();
        return resultat;
    }

    if (expression->est_reference_type()) {
        auto resultat = ExpressionTestDiscrimination{};
        resultat.ident = expression->ident;
        resultat.ref = expression->comme_reference_type();
        return resultat;
    }

    if (expression->est_appel()) {
        auto appel = expression->comme_appel();

        auto expression_ref = appel->expression;
        if (!expression_ref->est_reference_declaration() &&
            !expression_ref->est_reference_type()) {
            return {};
        }

        auto resultat = ExpressionTestDiscrimination{};
        resultat.ident = expression_ref->ident;
        resultat.ref = expression_ref;
        resultat.est_expression_appel = appel;
        return resultat;
    }

    return {};
}

static bool cree_variable_pour_expression_test(EspaceDeTravail *espace,
                                               AssembleuseArbre *assembleuse,
                                               NoeudExpression *expression,
                                               TypeUnion *type_union,
                                               NoeudBloc *bloc_parent,
                                               NoeudPaireDiscr *paire_discr,
                                               TypeCompose::Membre const *membre,
                                               NoeudExpressionAppel *appel)
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

    auto decl_prec = trouve_dans_bloc(bloc_parent, param->ident);

    if (decl_prec != nullptr) {
        espace->rapporte_erreur(param,
                                "Ne peut pas utiliser implicitement le membre car une "
                                "variable de ce nom existe déjà");
        return false;
    }

    /* À FAIRE(discr) : ceci n'est que pour la visite des noeuds dans le GestionnaireCode.
     */
    param->type = membre->type;
    appel->type = membre->type;

    auto bloc_insertion = paire_discr->bloc;

    /* L'initialisation est une extraction de la valeur de l'union.
     * À FAIRE(discr) : ignore la vérification sur l'activité du membre. */
    auto init_decl = assembleuse->cree_comme(param->lexeme);
    init_decl->expression = expression;
    init_decl->type = membre->type;
    init_decl->transformation = {
        TypeTransformation::EXTRAIT_UNION, membre->type, trouve_index_membre(type_union, membre)};

    auto decl_expr = assembleuse->cree_declaration_variable(param->comme_reference_declaration(),
                                                            init_decl);
    decl_expr->bloc_parent = bloc_insertion;
    decl_expr->type = membre->type;
    decl_expr->drapeaux |= DECLARATION_FUT_VALIDEE;

    bloc_insertion->expressions->pousse_front(decl_expr);
    bloc_insertion->ajoute_membre(decl_expr);

    paire_discr->variable_capturee = decl_expr;

    return true;
}

ResultatValidation ContexteValidationCode::valide_discr_union(NoeudDiscr *inst, Type *type)
{
    auto expression = inst->expression_discriminee;
    auto type_union = type->comme_union();
    auto decl = type_union->decl;
    inst->op = m_compilatrice.typeuse[TypeBase::Z32]->operateur_egt;

    if (decl->est_nonsure) {
        rapporte_erreur("« discr » ne peut prendre une union nonsûre", expression);
        return CodeRetourValidation::Erreur;
    }

    auto membres_rencontres = kuri::ensemblon<TypeCompose::Membre const *, 16>();

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

        auto expression_valide = expression_valide_discrimination(feuille);
        if (!expression_valide.has_value()) {
            espace->rapporte_erreur(feuille, "Attendu une référence à un membre de l'union")
                .ajoute_message("L'expression est de genre : ", feuille->genre, "\n");
            return CodeRetourValidation::Erreur;
        }

        auto membre = trouve_membre(type_union, expression_valide->ident);

        if (!membre) {
            rapporte_erreur_membre_inconnu(inst, feuille, type_union);
            return CodeRetourValidation::Erreur;
        }

        if (est_implicite(*membre)) {
            espace->rapporte_erreur(feuille,
                                    "Les membres implicites des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (est_constant(*membre)) {
            espace->rapporte_erreur(feuille,
                                    "Les membres constants des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membres_rencontres.possede(membre)) {
            rapporte_erreur("Redéfinition de l'expression", feuille);
            return CodeRetourValidation::Erreur;
        }

        /* À FAIRE(discr) : ceci n'est que pour la simplification du code. */
        feuille->ident = membre->nom;

        membres_rencontres.insere(membre);

        /* Ajoute la variable dans le bloc suivant. */
        if (expression_valide->est_expression_appel) {
            if (membre->type->est_rien()) {
                espace->rapporte_erreur(expression_valide->est_expression_appel,
                                        "Impossible de capturer une variable depuis un membre "
                                        "d'union de type « rien »");
                return CodeRetourValidation::Erreur;
            }
            cree_variable_pour_expression_test(espace,
                                               m_tacheronne.assembleuse,
                                               expression,
                                               type_union,
                                               inst->bloc_parent,
                                               inst->paires_discr[i],
                                               membre,
                                               expression_valide->est_expression_appel);
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

ResultatValidation ContexteValidationCode::valide_discr_union_anonyme(NoeudDiscr *inst, Type *type)
{
    auto type_union = type->comme_union();
    inst->op = m_compilatrice.typeuse[TypeBase::Z32]->operateur_egt;
    inst->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

    auto membres_rencontres = kuri::ensemblon<TypeCompose::Membre const *, 16>();

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;
        auto feuilles = expr_paire->comme_virgule();

        if (feuilles->expressions.taille() != 1) {
            rapporte_erreur("Trop d'expression pour le bloc de discrimination de l'union.",
                            expr_paire);
            return CodeRetourValidation::Erreur;
        }

        auto feuille = feuilles->expressions[0];

        auto expression_valide = expression_valide_discrimination(feuille);
        if (!expression_valide.has_value()) {
            espace->rapporte_erreur(feuille, "Attendu une référence à un membre de l'union")
                .ajoute_message("L'expression est de genre : ", feuille->genre, "\n");
            return CodeRetourValidation::Erreur;
        }

        auto ref_type = expression_valide->ref;

        valide_semantique_noeud(ref_type);

        Type *type_expr;
        if (resoud_type_final(ref_type, type_expr) == CodeRetourValidation::Erreur) {
            rapporte_erreur("Ne peut résoudre le type", ref_type);
            return CodeRetourValidation::Erreur;
        }

        expr_paire->type = type_expr;

        auto membre = trouve_membre(type_union, type_expr);
        if (!membre) {
            rapporte_erreur("Le type n'est pas membre de l'union", feuille);
            return CodeRetourValidation::Erreur;
        }

        if (est_implicite(*membre)) {
            espace->rapporte_erreur(feuille,
                                    "Les membres implicites des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (est_constant(*membre)) {
            espace->rapporte_erreur(feuille,
                                    "Les membres constants des unions ne peuvent être "
                                    "utilisés comme expression de discrimination");
            return CodeRetourValidation::Erreur;
        }

        if (membres_rencontres.possede(membre)) {
            rapporte_erreur("Redéfinition de l'expression", feuille);
            return CodeRetourValidation::Erreur;
        }

        /* À FAIRE(discr) : ceci n'est que pour la simplification du code. */
        feuille->ident = membre->nom;

        membres_rencontres.insere(membre);

        /* Ajoute la variable dans le bloc suivant. */
        if (expression_valide->est_expression_appel) {
            if (ref_type->type->est_rien()) {
                espace->rapporte_erreur(expression_valide->est_expression_appel,
                                        "Impossible de capturer une variable depuis un membre "
                                        "d'union de type « rien »");
                return CodeRetourValidation::Erreur;
            }
            cree_variable_pour_expression_test(espace,
                                               m_tacheronne.assembleuse,
                                               inst->expression_discriminee,
                                               type_union,
                                               inst->bloc_parent,
                                               inst->paires_discr[i],
                                               membre,
                                               expression_valide->est_expression_appel);
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
ResultatValidation ContexteValidationCode::valide_discr_scalaire(NoeudDiscr *inst, Type *type)
{
    auto type_pour_la_recherche = type;
    if (type->genre == GenreType::TYPE_DE_DONNEES) {
        type_pour_la_recherche = m_compilatrice.typeuse.type_type_de_donnees_;
    }

    auto candidats = kuri::tablet<OperateurCandidat, 10>();
    auto resultat = cherche_candidats_operateurs(
        *espace, type_pour_la_recherche, type_pour_la_recherche, GenreLexeme::EGALITE, candidats);
    if (resultat.has_value()) {
        return resultat.value();
    }

    auto meilleur_candidat = OperateurCandidat::nul_const();
    auto poids = 0.0;

    for (auto const &candidat : candidats) {
        if (candidat.poids > poids) {
            poids = candidat.poids;
            meilleur_candidat = &candidat;
        }
    }

    if (meilleur_candidat == nullptr) {
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

    inst->op = meilleur_candidat->op;

    for (int i = 0; i < inst->paires_discr.taille(); ++i) {
        auto expr_paire = inst->paires_discr[i]->expression;

        auto feuilles = expr_paire->comme_virgule();

        for (auto j = 0; j < feuilles->expressions.taille(); ++j) {
            auto resultat_validation = valide_semantique_noeud(feuilles->expressions[j]);
            if (!est_ok(resultat_validation)) {
                return resultat_validation;
            }

            auto const resultat_transtype = transtype_si_necessaire(feuilles->expressions[j],
                                                                    type);
            if (!est_ok(resultat_transtype)) {
                return resultat_transtype;
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

ResultatValidation ContexteValidationCode::valide_discrimination(NoeudDiscr *inst)
{
    auto expression = inst->expression_discriminee;
    auto type = expression->type;

    if (type->genre == GenreType::REFERENCE) {
        transtype_si_necessaire(inst->expression_discriminee, TypeTransformation::DEREFERENCE);
        type = type->comme_reference()->type_pointe;
    }

    if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
        return Attente::sur_type(type);
    }

    if (type->genre == GenreType::UNION && type->comme_union()->est_anonyme) {
        return valide_discr_union_anonyme(inst, type);
    }

    if (type->genre == GenreType::UNION) {
        return valide_discr_union(inst, type);
    }

    if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
        return valide_discr_enum(inst, type);
    }

    return valide_discr_scalaire(inst, type);
}
