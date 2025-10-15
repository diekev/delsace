/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "monomorpheuse.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "portee.hh"
#include "transformation_type.hh"
#include "typage.hh"

#include "utilitaires/log.hh"

static kuri::chaine_statique commentaire_pour_genre(ItemMonomorphisation const &item)
{
    if (item.genre == GenreItem::VALEUR) {
        return "une valeur";
    }

    if (item.genre == GenreItem::TYPE_DE_DONNÉES) {
        return "un type";
    }

    return "indéfini";
}

kuri::chaine ErreurMonomorphisation::message() const
{
#define SI_ERREUR_EST(Type)                                                                       \
    if (std::holds_alternative<Type>(this->données)) {                                            \
        auto données_erreur = std::get<Type>(this->données);

#define FIN_ERREUR(Type) }

    Enchaineuse enchaineuse;
    enchaineuse << "\tDans la monomorphisation de « " << polymorphe->ident->nom << " » :\n";

    SI_ERREUR_EST(DonnéesErreurContrainte)
    {
        enchaineuse << "\t\tContrainte incompatible pour « "
                    << données_erreur.item_contrainte.ident->nom << " ».\n";

        if (données_erreur.résultat == ÉtatRésolutionContrainte::PasLeMêmeType) {
            enchaineuse << "\t\tLe type désiré est "
                        << chaine_type(données_erreur.item_contrainte.type) << ".\n";
            enchaineuse << "\t\tMais le type reçu est "
                        << chaine_type(données_erreur.item_reçu.type) << ".\n";
            return enchaineuse.chaine();
        }

        if (données_erreur.résultat == ÉtatRésolutionContrainte::ContrainteNonRespectée) {
            auto const type_reçu =
                données_erreur.item_reçu.type->comme_type_type_de_données()->type_connu;
            enchaineuse << "\t\tLa contrainte requiers un type compatible avec « "
                        << chaine_type(données_erreur.item_contrainte.contrainte_type)
                        << " », mais le type reçu « " << chaine_type(type_reçu)
                        << " » n'est pas compatible.\n";
            return enchaineuse.chaine();
        }

        enchaineuse << "\t\tNous voulions "
                    << commentaire_pour_genre(données_erreur.item_contrainte) << ".\n";
        enchaineuse << "\t\tMais nous avons reçu "
                    << commentaire_pour_genre(données_erreur.item_reçu) << ".\n";
        return enchaineuse.chaine();
    }
    FIN_ERREUR(DonnéesErreurContrainte)

    SI_ERREUR_EST(DonnéesErreurCompatibilité)
    {
        enchaineuse << "\t\tValeurs incompatibles pour « $"
                    << données_erreur.item_contrainte.ident->nom << " ».\n\n";

        if (données_erreur.résultat == ÉtatRésolutionCompatibilité::PasLaMêmeValeur) {
            enchaineuse << "\t\tNous voulions la valeur " << données_erreur.item_contrainte.valeur
                        << ".\n";
            enchaineuse << "\t\tMais nous avons obtenu la valeur "
                        << données_erreur.item_reçu.valeur << ".\n";
        }
        else {
            auto const type_voulu =
                données_erreur.item_contrainte.type->comme_type_type_de_données()->type_connu;
            auto const type_obtenu =
                données_erreur.item_reçu.type->comme_type_type_de_données()->type_connu;
            enchaineuse << "\t\tNous voulions le type " << chaine_type(type_voulu) << ".\n";
            enchaineuse << "\t\tMais nous avons obtenu le type " << chaine_type(type_obtenu)
                        << ".\n";
        }

        return enchaineuse.chaine();
    }
    FIN_ERREUR(DonnéesErreurCompatibilité)

    SI_ERREUR_EST(DonnéesErreurItemManquante)
    {
        return enchaine(
            "impossible de trouver l'item pour « ", données_erreur.item_reçu.ident->nom, " »");
    }
    FIN_ERREUR(DonnéesErreurItemManquante)

    SI_ERREUR_EST(DonnéesErreurInterne)
    {
        return enchaine(données_erreur.message);
    }
    FIN_ERREUR(DonnéesErreurInterne)

    SI_ERREUR_EST(DonnéesErreurGenreType)
    {
        return enchaine(chaine_type(données_erreur.type_reçu), " ", données_erreur.message);
    }
    FIN_ERREUR(DonnéesErreurGenreType)

    SI_ERREUR_EST(DonnéesErreurOpérateurNonGéré)
    {
        return enchaine("genre opérateur non géré : ", données_erreur.lexeme);
    }
    FIN_ERREUR(DonnéesErreurOpérateurNonGéré)

    SI_ERREUR_EST(DonnéesErreurRéférenceInconnue)
    {
        return enchaine("aucun type ou aucune valeur polymorphique correspondant à : ",
                        données_erreur.ident->nom);
    }
    FIN_ERREUR(DonnéesErreurRéférenceInconnue)

    SI_ERREUR_EST(DonnéesErreurMonomorphisationManquante)
    {
        enchaineuse << "\t\tAucune monomorphisation connue pour les items.\n\n";

        enchaineuse << "\t\tLes items sont :\n";
        POUR (données_erreur.items) {
            enchaineuse << "\t\t\t" << it << '\n';
        }

        enchaineuse << '\n';
        données_erreur.monomorphisations->imprime(enchaineuse, 2);

        return enchaineuse.chaine();
    }
    FIN_ERREUR(DonnéesErreurMonomorphisationManquante)

    SI_ERREUR_EST(DonnéesErreurSémantique)
    {
        return enchaine(données_erreur.message);
    }
    FIN_ERREUR(DonnéesErreurSémantique)

#undef SI_ERREUR_EST
#undef FIN_ERREUR
    return "erreur inconnue";
}

Monomorpheuse::Monomorpheuse(EspaceDeTravail &ref_espace,
                             const NoeudDéclarationEntêteFonction *entete)
    : espace(ref_espace), typeuse(espace.compilatrice().typeuse), polymorphe(entete)
{
    POUR (*entete->bloc_constantes->rubriques.verrou_lecture()) {
        ajoute_item_pour_constante(it->comme_déclaration_constante());
    }

    POUR (items) {
        items_résultat.ajoute({it.ident, nullptr, {}, GenreItem::INDÉFINI});
    }
}

static ItemMonomorphisation *trouve_item_pour_ident(
    kuri::tableau_statique<ItemMonomorphisation> const &tableau_items,
    IdentifiantCode const *ident)
{
    POUR (tableau_items) {
        if (it.ident != ident) {
            continue;
        }

        return const_cast<ItemMonomorphisation *>(&it);
    }

    return nullptr;
}

ItemMonomorphisation *Monomorpheuse::item_pour_ident(IdentifiantCode const *ident)
{
    return trouve_item_pour_ident(items, ident);
}

ItemMonomorphisation *Monomorpheuse::item_résultat_pour_ident(IdentifiantCode const *ident)
{
    return trouve_item_pour_ident(items_résultat, ident);
}

ValeurExpression Monomorpheuse::évalue_valeur(const NoeudExpression *expr)
{
    auto résultat = évalue_expression(espace.compilatrice(), expr->bloc_parent, expr);

    if (résultat.est_erroné) {
        erreur_sémantique(expr, "l'expression n'est pas constante");
        return {};
    }

    return résultat.valeur;
}

/** ***************************************************************
 * Rapport d'erreurs.
 * \{
 */

void Monomorpheuse::ajoute_erreur(const NoeudExpression *site, DonnéesErreur donnees)
{
    if (erreur_courante.has_value()) {
        return;
    }

    auto erreur = ErreurMonomorphisation();
    erreur.site = site;
    erreur.polymorphe = polymorphe;
    erreur.données = donnees;
    erreur_courante = erreur;
}

void Monomorpheuse::erreur_interne(const NoeudExpression *site, kuri::chaine message)
{
    ajoute_erreur(site, DonnéesErreurInterne{message});
}

void Monomorpheuse::erreur_contrainte(const NoeudExpression *site,
                                      ÉtatRésolutionContrainte résultat,
                                      ItemMonomorphisation item_contrainte,
                                      ItemMonomorphisation item_reçu)
{
    ajoute_erreur(site, DonnéesErreurContrainte{résultat, item_contrainte, item_reçu});
}

void Monomorpheuse::erreur_compatibilité(const NoeudExpression *site,
                                         ÉtatRésolutionCompatibilité résultat,
                                         ItemMonomorphisation item_contrainte,
                                         ItemMonomorphisation item_reçu)
{
    ajoute_erreur(site, DonnéesErreurCompatibilité{résultat, item_contrainte, item_reçu});
}

void Monomorpheuse::erreur_item_manquant(const NoeudExpression *site,
                                         ItemMonomorphisation item_reçu)
{
    ajoute_erreur(site, DonnéesErreurItemManquante{item_reçu});
}

void Monomorpheuse::erreur_genre_type(const NoeudExpression *site,
                                      const Type *type_reçu,
                                      kuri::chaine_statique message)
{
    ajoute_erreur(site, DonnéesErreurGenreType{type_reçu, message});
}

void Monomorpheuse::erreur_opérateur_non_géré(const NoeudExpression *site, GenreLexème lexeme)
{
    ajoute_erreur(site, DonnéesErreurOpérateurNonGéré{lexeme});
}

void Monomorpheuse::erreur_référence_inconnue(const NoeudExpression *site)
{
    ajoute_erreur(site, DonnéesErreurRéférenceInconnue{site->ident});
}

void Monomorpheuse::erreur_monomorphisation_inconnue(
    const NoeudExpression *site,
    const kuri::tablet<ItemMonomorphisation, 6> &items_,
    const Monomorphisations *monomorphisations)
{
    ajoute_erreur(site, DonnéesErreurMonomorphisationManquante{items_, monomorphisations});
}

void Monomorpheuse::erreur_sémantique(const NoeudExpression *site, kuri::chaine_statique message)
{
    ajoute_erreur(site, DonnéesErreurSémantique{message});
}

/** \} */

void Monomorpheuse::ajoute_candidat(const IdentifiantCode *ident, const Type *type_reçu)
{
    /* Nous devons toujours avoir un type_de_données. Recevoir quelque chose qui n'en pas un veut
     * sans doute dire que le candidat a été ajouté directement via une expression résolue de
     * l'appel.
     * Vérifie que le type n'est pas nul car il pourrait s'agir de l'élément du type nul.
     * À FAIRE : considère avoir un type élémént non nul pour type_de(nul). */
    if (!type_reçu || !type_reçu->est_type_type_de_données()) {
        /* Garantie que nous n'avons pas d'entiers constants dans les résultats. */
        if (type_reçu && type_reçu->est_type_entier_constant()) {
            type_reçu = typeuse.type_z32;
        }

        type_reçu = typeuse.type_type_de_donnees(const_cast<Type *>(type_reçu));
    }

    candidats.ajoute({ident, type_reçu, {}, GenreItem::TYPE_DE_DONNÉES});
}

void Monomorpheuse::ajoute_candidat_valeur(const IdentifiantCode *ident,
                                           const Type *type,
                                           const ValeurExpression valeur)
{
    candidats.ajoute({ident, type, valeur, GenreItem::VALEUR});
}

void Monomorpheuse::ajoute_candidat_depuis_référence_déclaration(
    const NoeudExpressionRéférence *reference, const Type *type_reçu)
{
    auto const decl = reference->déclaration_référée;

    if (decl->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
        ajoute_candidat(decl->ident, type_reçu);
    }
    else if (decl->est_type_structure()) {
        auto const structure = decl->comme_type_structure();
        ajoute_candidats_depuis_déclaration_structure(structure, type_reçu);
    }
}

void Monomorpheuse::ajoute_candidats_depuis_type_fonction(
    const NoeudExpressionTypeFonction *decl_type_fonction,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    if (!type_reçu->est_type_fonction()) {
        erreur_genre_type(nullptr, type_reçu, "n'est pas un type fonction");
        return;
    }

    auto const type_fonction_reçu = type_reçu->comme_type_fonction();
    auto const type_sortie_fonction_reçu = type_fonction_reçu->type_sortie;

    if (decl_type_fonction->types_entrée.taille() != type_fonction_reçu->types_entrées.taille()) {
        erreur_genre_type(site, type_reçu, "n'a pas le bon nombre de paramètres en entrée");
        return;
    }

    if (decl_type_fonction->types_sortie.taille() == 1 &&
        type_sortie_fonction_reçu->est_type_tuple()) {
        erreur_genre_type(site, type_reçu, "n'a pas le bon nombre de paramètres en sortie");
        return;
    }

    if (decl_type_fonction->types_sortie.taille() > 1 &&
        !type_sortie_fonction_reçu->est_type_tuple()) {
        erreur_genre_type(site, type_reçu, "n'a pas le bon nombre de paramètres en sortie");
        return;
    }

    for (auto i = 0; i < decl_type_fonction->types_entrée.taille(); i++) {
        auto const param = decl_type_fonction->types_entrée[i];
        parse_candidats(param, site, type_fonction_reçu->types_entrées[i]);
    }

    if (decl_type_fonction->types_sortie.taille() == 1) {
        parse_candidats(decl_type_fonction->types_sortie[0], site, type_sortie_fonction_reçu);
    }
    else {
        auto const tuple = type_sortie_fonction_reçu->comme_type_tuple();
        for (auto i = 0; i < decl_type_fonction->types_sortie.taille(); i++) {
            auto const param = decl_type_fonction->types_sortie[i];
            parse_candidats(param, site, tuple->rubriques[i].type);
        }
    }
}

void Monomorpheuse::ajoute_candidats_depuis_déclaration_structure(const NoeudStruct *structure,
                                                                  const Type *type_reçu)
{
    if (!structure->est_polymorphe) {
        return;
    }

    if (!type_reçu->est_type_structure() && !type_reçu->est_type_union()) {
        erreur_genre_type(nullptr, type_reçu, "n'est ni une structure ni une union");
        return;
    }

    auto decl_struct = type_reçu->comme_déclaration_classe();
    if (decl_struct->polymorphe_de_base != structure) {
        erreur_genre_type(nullptr, type_reçu, "n'est pas une forme du polymorphe");
        return;
    }

    POUR (*decl_struct->bloc_constantes->rubriques.verrou_lecture()) {
        auto param_poly = trouve_dans_bloc(structure->bloc_constantes, it->ident);

        if (param_poly->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            if (it->type->est_type_type_de_données()) {
                ajoute_candidat(it->ident, it->type);
            }
            else {
                ajoute_candidat_valeur(
                    it->ident, it->type, it->comme_déclaration_constante()->valeur_expression);
            }
        }
        else if (param_poly->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
            ajoute_candidat(it->ident, it->type);
        }
    }
}

static NoeudDéclaration *rubrique_pour_ident_ou_index(NoeudBloc *bloc,
                                                      const IdentifiantCode *ident,
                                                      int index)
{
    if (ident) {
        return trouve_dans_bloc(bloc, ident);
    }
    return bloc->rubrique_pour_index(index);
}

void Monomorpheuse::ajoute_candidats_depuis_construction_structure(
    const NoeudExpressionConstructionStructure *construction,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    auto declaration_appelee = construction->noeud_fonction_appelée;

    if (declaration_appelee->est_type_opaque()) {
        ajoute_candidats_depuis_construction_opaque(construction, site, type_reçu);
        return;
    }

    auto structure_construite = declaration_appelee->comme_déclaration_classe();
    if (!structure_construite->est_polymorphe) {
        return;
    }

    if (!type_reçu->est_type_structure() && !type_reçu->est_type_union()) {
        erreur_genre_type(site, type_reçu, "n'est ni une structure ni une union");
        return;
    }

    auto decl_struct_type = type_reçu->comme_déclaration_classe();
    if (decl_struct_type->polymorphe_de_base != structure_construite) {
        erreur_genre_type(site, type_reçu, "n'est pas une forme du polymorphe");
        return;
    }

    for (int i = 0; i < construction->paramètres.taille(); i++) {
        auto it = construction->paramètres[i];
        IdentifiantCode const *ident_param = nullptr;
        /* L'argument est nommé. */
        if (it->est_assignation_variable()) {
            auto assign = it->comme_assignation_variable();
            it = assign->expression;
            ident_param = assign->assignée->ident;
        }

        if (!it->est_référence_déclaration()) {
            continue;
        }

        auto param_poly = rubrique_pour_ident_ou_index(
            decl_struct_type->bloc_constantes, ident_param, i);

        if (param_poly->type->est_type_type_de_données()) {
            ajoute_candidat(it->ident, param_poly->type);
        }
        else {
            ajoute_candidat_valeur(it->ident,
                                   param_poly->type,
                                   param_poly->comme_déclaration_constante()->valeur_expression);
        }
    }
}

void Monomorpheuse::ajoute_candidats_depuis_construction_opaque(
    const NoeudExpressionConstructionStructure *construction,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    if (!type_reçu->est_type_opaque()) {
        erreur_genre_type(site, type_reçu, "n'est pas un type opaque");
        return;
    }

    auto type_opaque = type_reçu->comme_type_opaque();

    for (int i = 0; i < construction->paramètres.taille(); i++) {
        auto it = construction->paramètres[i];
        if (!it->est_référence_déclaration()) {
            continue;
        }

        auto decl_referee = it->comme_référence_déclaration()->déclaration_référée;

        if (decl_referee->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            if (decl_referee->type->est_type_type_de_données()) {
                ajoute_candidat(it->ident, decl_referee->type);
            }
            else {
                erreur_sémantique(site,
                                  "les opaques ne peuvent recevoir de valeurs polymorphiques");
            }
        }
        else if (decl_referee->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
            ajoute_candidat(it->ident, type_opaque->type_opacifié);
        }
    }
}

void Monomorpheuse::ajoute_candidats_depuis_déclaration_tranche(
    const NoeudExpressionTypeTranche *expr_type_tranche,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    auto type_élément = Type::nul_const();
    if (type_reçu->est_type_tranche()) {
        auto type_tranche = type_reçu->comme_type_tranche();
        type_élément = type_tranche->type_élément;
    }
    else if (type_reçu->est_type_tableau_fixe()) {
        auto type_tableau_fixe = type_reçu->comme_type_tableau_fixe();
        type_élément = type_tableau_fixe->type_pointé;
    }
    else if (type_reçu->est_type_tableau_dynamique()) {
        auto type_tableau_dynamique = type_reçu->comme_type_tableau_dynamique();
        type_élément = type_tableau_dynamique->type_pointé;
    }
    else {
        erreur_genre_type(
            site, type_reçu, "n'est pas une tranche, ni un tableau fixe ou dynamique");
        return;
    }

    auto const expression_type = expr_type_tranche->expression_type;
    parse_candidats(expression_type, site, type_élément);
}

void Monomorpheuse::ajoute_candidats_depuis_déclaration_tableau(
    const NoeudExpressionTypeTableauDynamique *expr_type_tableau,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    auto const expression_type = expr_type_tableau->expression_type;

    if (!type_reçu->est_type_tableau_dynamique()) {
        erreur_genre_type(site, type_reçu, "n'est pas un tableau dynamique");
        return;
    }

    auto type_tableau = type_reçu->comme_type_tableau_dynamique();
    parse_candidats(expression_type, site, type_tableau->type_pointé);
}

void Monomorpheuse::ajoute_candidats_depuis_déclaration_tableau(
    const NoeudExpressionTypeTableauFixe *expr_type_tableau,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    if (!type_reçu->est_type_tableau_fixe()) {
        erreur_genre_type(nullptr, type_reçu, "n'est pas un tableau fixe");
        return;
    }

    auto const expression_taille = expr_type_tableau->expression_taille;
    auto const type_tableau = type_reçu->comme_type_tableau_fixe();
    if (expression_taille->est_référence_déclaration()) {
        auto decl_referee = expression_taille->comme_référence_déclaration()->déclaration_référée;
        if (decl_referee->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE |
                                          DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
            ValeurExpression valeur = type_tableau->taille;
            ajoute_candidat_valeur(decl_referee->ident, typeuse.type_z64, valeur);
        }
    }

    auto const expression_type = expr_type_tableau->expression_type;
    parse_candidats(expression_type, site, type_tableau->type_pointé);
}

void Monomorpheuse::parse_candidats(const NoeudExpression *expression_polymorphique,
                                    const NoeudExpression *site,
                                    const Type *type_reçu)
{
    if (expression_polymorphique->est_déclaration_variable()) {
        if (expression_polymorphique->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            ajoute_candidat(expression_polymorphique->ident, type_reçu);
        }
    }
    else if (expression_polymorphique->est_prise_adresse()) {
        auto const prise_adresse = expression_polymorphique->comme_prise_adresse();
        if (type_reçu->est_type_pointeur()) {
            auto type_élément = type_reçu->comme_type_pointeur()->type_pointé;
            /* "nul" peut-être passé en paramètre... */
            if (type_élément != nullptr) {
                parse_candidats(
                    prise_adresse->opérande, site, type_reçu->comme_type_pointeur()->type_pointé);
            }
        }
        else {
            erreur_genre_type(site, type_reçu, "n'est pas un pointeur");
        }
    }
    else if (expression_polymorphique->est_prise_référence()) {
        auto const prise_référence = expression_polymorphique->comme_prise_référence();
        if (type_reçu->est_type_référence()) {
            parse_candidats(
                prise_référence->opérande, site, type_reçu->comme_type_référence()->type_pointé);
        }
        else {
            /* Il est possible que la référence soit implicite, donc tente d'apparier avec le
             * type directement. */
            parse_candidats(prise_référence->opérande, site, type_reçu);
        }
    }
    else if (expression_polymorphique->est_expression_type_tranche()) {
        auto type_tranche = expression_polymorphique->comme_expression_type_tranche();
        ajoute_candidats_depuis_déclaration_tranche(type_tranche, site, type_reçu);
    }
    else if (expression_polymorphique->est_expression_type_tableau_dynamique()) {
        auto type_tableau_dynamique =
            expression_polymorphique->comme_expression_type_tableau_dynamique();
        ajoute_candidats_depuis_déclaration_tableau(type_tableau_dynamique, site, type_reçu);
    }
    else if (expression_polymorphique->est_expression_type_tableau_fixe()) {
        auto type_tableau_fixe = expression_polymorphique->comme_expression_type_tableau_fixe();
        ajoute_candidats_depuis_déclaration_tableau(type_tableau_fixe, site, type_reçu);
    }
    else if (expression_polymorphique->est_référence_déclaration()) {
        auto const ref_decl = expression_polymorphique->comme_référence_déclaration();
        ajoute_candidat_depuis_référence_déclaration(ref_decl, type_reçu);
    }
    else if (expression_polymorphique->est_construction_structure()) {
        auto const construction = expression_polymorphique->comme_construction_structure();
        ajoute_candidats_depuis_construction_structure(construction, site, type_reçu);
    }
    else if (expression_polymorphique->est_appel()) {
        auto const construction = expression_polymorphique->comme_appel();
        ajoute_candidats_depuis_construction_structure(
            static_cast<NoeudExpressionConstructionStructure const *>(construction),
            site,
            type_reçu);
    }
    else if (expression_polymorphique->est_expression_type_fonction()) {
        auto type_fonction = expression_polymorphique->comme_expression_type_fonction();
        ajoute_candidats_depuis_type_fonction(type_fonction, site, type_reçu);
    }
    else if (expression_polymorphique->est_expansion_variadique()) {
        auto expansion = expression_polymorphique->comme_expansion_variadique();
        ajoute_candidats_depuis_expansion_variadique(expansion, site, type_reçu);
    }
    else if (expression_polymorphique->est_référence_rubrique()) {
        erreur_interne(site, "les références de rubrique ne sont pas encore implémentées");
    }
    else if (expression_polymorphique->est_référence_type()) {
        /* Rien à faire. */
    }
    else if (expression_polymorphique->est_expression_binaire()) {
        auto expression = expression_polymorphique->comme_expression_binaire();
        parse_candidats(expression->opérande_gauche, site, type_reçu);
    }
    else {
        erreur_interne(site, enchaine("type de noeud non géré ", expression_polymorphique->genre));
    }
}

void Monomorpheuse::ajoute_candidats_depuis_expansion_variadique(
    const NoeudExpressionExpansionVariadique *expansion,
    const NoeudExpression *site,
    const Type *type_reçu)
{
    /* Pour les types variadiques nous avons 2 cas :
     * - soit nous recevons une expansion (...args)
     * - soit nous recevons une liste (0, 1, 2, 3)
     * Nous devons donc apparier avec le type pointé si nous sommes dans le premier cas.
     */
    auto type = type_reçu;
    if (type_reçu->est_type_variadique()) {
        type = type_reçu->comme_type_variadique()->type_pointé;
    }
    else if (type_reçu->est_type_tableau_dynamique()) {
        type = type_reçu->comme_type_tableau_dynamique()->type_pointé;
    }
    else if (type_reçu->est_type_tranche()) {
        type = type_reçu->comme_type_tranche()->type_élément;
    }
    else if (type_reçu->est_type_tableau_fixe()) {
        type = type_reçu->comme_type_tableau_fixe()->type_pointé;
    }
    parse_candidats(expansion->expression, site, type);
}

Type *Monomorpheuse::résoud_type_final_impl(const NoeudExpression *expression_polymorphique)
{
    profondeur_appariement_type += 1;
    if (expression_polymorphique->est_prise_adresse()) {
        auto const prise_adresse = expression_polymorphique->comme_prise_adresse();
        auto type_pointe = résoud_type_final_impl(prise_adresse->opérande);
        if (!type_pointe) {
            return nullptr;
        }
        return typeuse.type_pointeur_pour(type_pointe);
    }
    else if (expression_polymorphique->est_prise_référence()) {
        auto const prise_référence = expression_polymorphique->comme_prise_référence();
        auto type_pointe = résoud_type_final_impl(prise_référence->opérande);
        if (!type_pointe) {
            return nullptr;
        }
        return typeuse.type_reference_pour(type_pointe);
    }
    else if (expression_polymorphique->est_expression_type_tableau_fixe()) {
        auto const type_tableau_fixe =
            expression_polymorphique->comme_expression_type_tableau_fixe();
        return résoud_type_final_pour_déclaration_tableau_fixe(type_tableau_fixe);
    }
    else if (expression_polymorphique->est_expression_type_tranche()) {
        auto const type_tranche = expression_polymorphique->comme_expression_type_tranche();
        return résoud_type_final_pour_déclaration_tranche(type_tranche);
    }
    else if (expression_polymorphique->est_expression_type_tableau_dynamique()) {
        auto const type_tableau_dynamique =
            expression_polymorphique->comme_expression_type_tableau_dynamique();
        return résoud_type_final_pour_déclaration_tableau_dynamique(type_tableau_dynamique);
    }
    else if (expression_polymorphique->est_référence_déclaration()) {
        auto const ref_decl = expression_polymorphique->comme_référence_déclaration();
        return résoud_type_final_pour_référence_déclaration(ref_decl);
    }
    else if (expression_polymorphique->est_construction_structure()) {
        auto const construction = expression_polymorphique->comme_construction_structure();
        return résoud_type_final_pour_construction_structure(construction);
    }
    else if (expression_polymorphique->est_appel()) {
        auto const construction = expression_polymorphique->comme_appel();
        return résoud_type_final_pour_construction_structure(
            static_cast<NoeudExpressionConstructionStructure const *>(construction));
    }
    else if (expression_polymorphique->est_expression_type_fonction()) {
        auto type_fonction = expression_polymorphique->comme_expression_type_fonction();
        return résoud_type_final_pour_type_fonction(type_fonction);
    }
    else if (expression_polymorphique->est_expansion_variadique()) {
        auto expansion = expression_polymorphique->comme_expansion_variadique();
        return résoud_type_final_pour_expansion_variadique(expansion);
    }
    else if (expression_polymorphique->est_référence_rubrique()) {
        erreur_interne(expression_polymorphique,
                       "les références de rubrique ne sont pas encore implémentées");
    }
    else if (expression_polymorphique->est_référence_type()) {
        return expression_polymorphique->type->comme_type_type_de_données()->type_connu;
    }
    else if (expression_polymorphique->est_expression_binaire()) {
        auto expression = expression_polymorphique->comme_expression_binaire();
        return résoud_type_final_impl(expression->opérande_gauche);
    }
    else if (expression_polymorphique->est_déclaration_variable()) {
        auto variable = expression_polymorphique->comme_déclaration_variable();
        return résoud_type_final_impl(variable->expression_type);
    }
    else {
        erreur_interne(expression_polymorphique,
                       enchaine("type de noeud non géré ", expression_polymorphique->genre));
    }
    return nullptr;
}

RésultatRésolutionType Monomorpheuse::résoud_type_final(
    const NoeudExpression *expression_polymorphique)
{
    profondeur_appariement_type = 0;
    auto type = résoud_type_final_impl(expression_polymorphique);

    if (a_une_erreur()) {
        return erreur().value();
    }

    auto const profondeur_type = donne_profondeur_type(type);
    auto const poids_appariement_type = double(profondeur_appariement_type) /
                                        double(profondeur_type);

    return TypeAppariéPesé{type, poids_appariement_type};
}

void Monomorpheuse::logue() const
{
    Enchaineuse sortie;
    sortie << "------------------------------------------------\n";
    sortie << "Monomorphisation de " << polymorphe->ident->nom << '\n';
    sortie << erreur::imprime_site(espace, polymorphe);

    POUR (candidats) {
        sortie << "Candidat " << it << '\n';
    }

    if (a_une_erreur()) {
        sortie << "Erreur de monomorphisation : " << erreur_courante->message() << '\n';
    }

    sortie << "\n";
    sortie << "Résultat :\n";
    POUR (résultat_pour_monomorphisation()) {
        sortie << "-- " << it << '\n';
    }

    dbg() << sortie.chaine();
}

void Monomorpheuse::ajoute_item_pour_constante(NoeudDéclarationConstante *constante)
{
    ItemMonomorphisation item;
    item.ident = constante->ident;

    if (constante->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
        if (!constante->type) {
            /* Valeur taille polymorphique. */
            item.genre = GenreItem::VALEUR;
        }
        else if (constante->type->est_type_type_de_données()) {
            /* $T: type_de_données */
            item.genre = GenreItem::TYPE_DE_DONNÉES;
        }
        else {
            /* Par exemple : $N: z32, ou $F: fonc()(rien). */
            item.genre = GenreItem::VALEUR;
            item.type = constante->type;
            item.expression_type = constante->expression_type;
        }
    }
    else if (constante->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
        /* $T ou $T/Contrainte */
        if (constante->possède_drapeau(DrapeauxNoeud::EXPRESSION_TYPE_EST_CONTRAINTE_POLY)) {
            item.genre = GenreItem::TYPE_DE_DONNÉES;
            item.contrainte_type =
                constante->expression_type->type->comme_type_type_de_données()->type_connu;
        }
        else {
            item.genre = GenreItem::INDÉFINI;
        }
    }

    items.ajoute(item);
}

Type *Monomorpheuse::résoud_type_final_pour_référence_déclaration(
    const NoeudExpressionRéférence *reference)
{
    auto decl_referee = reference->déclaration_référée;

    if (!decl_referee->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
        if (decl_referee->est_déclaration_type()) {
            return decl_referee->comme_déclaration_type();
        }
        return decl_referee->type;
    }

    auto item = trouve_item_pour_ident(items_résultat, decl_referee->ident);
    if (!item) {
        erreur_référence_inconnue(reference);
        return nullptr;
    }

    if (item->type->est_type_type_de_données()) {
        return item->type->comme_type_type_de_données()->type_connu;
    }

    return const_cast<Type *>(item->type);
}

Type *Monomorpheuse::résoud_type_final_pour_type_fonction(
    const NoeudExpressionTypeFonction *decl_type_fonction)
{
    kuri::tablet<Type *, 6> types_entrees;
    Type *type_sortie;

    for (auto i = 0; i < decl_type_fonction->types_entrée.taille(); i++) {
        auto type = résoud_type_final_impl(decl_type_fonction->types_entrée[i]);
        if (!type) {
            return nullptr;
        }
        types_entrees.ajoute(type);
    }

    if (decl_type_fonction->types_sortie.taille() > 1) {
        kuri::tablet<RubriqueTypeComposé, 6> types_sorties;

        for (auto i = 0; i < decl_type_fonction->types_sortie.taille(); i++) {
            auto type = résoud_type_final_impl(decl_type_fonction->types_sortie[i]);
            if (!type) {
                return nullptr;
            }
            RubriqueTypeComposé rubrique;
            rubrique.type = type;
            types_sorties.ajoute(rubrique);
        }

        type_sortie = typeuse.crée_tuple(types_sorties);
    }
    else {
        type_sortie = résoud_type_final_impl(decl_type_fonction->types_sortie[0]);
    }

    if (!type_sortie) {
        return nullptr;
    }

    return typeuse.type_fonction(types_entrees, type_sortie);
}

Type *Monomorpheuse::résoud_type_final_pour_construction_structure(
    const NoeudExpressionConstructionStructure *construction)
{
    auto declaration_appelee = construction->noeud_fonction_appelée;
    if (declaration_appelee->est_type_opaque()) {
        return résoud_type_final_pour_construction_opaque(construction);
    }

    auto structure_construite = declaration_appelee->comme_déclaration_classe();
    if (!structure_construite->est_polymorphe) {
        return structure_construite;
    }

    kuri::tablet<ItemMonomorphisation, 6> items_structure;
    POUR (*structure_construite->bloc_constantes->rubriques.verrou_lecture()) {
        items_structure.ajoute({it->ident, nullptr, {}, GenreItem::TYPE_DE_DONNÉES});
    }

    /* Extrait les items du résultat qui sont applicables à la structure. */
    for (int i = 0; i < construction->paramètres.taille(); i++) {
        auto it = construction->paramètres[i];
        IdentifiantCode const *ident_param = nullptr;

        if (it->est_assignation_variable()) {
            /* L'argument est nommé. */
            auto assign = it->comme_assignation_variable();
            it = assign->expression;
            ident_param = assign->assignée->ident;
        }

        if (!it->est_référence_déclaration()) {
            // À FAIRE(monomorphisation) : erreur ?
            continue;
        }

        auto decl_referee = it->comme_référence_déclaration()->déclaration_référée;

        auto item_résultat = trouve_item_pour_ident(items_résultat, decl_referee->ident);
        assert(item_résultat);

        auto item_structure = ident_param ? trouve_item_pour_ident(items_structure, ident_param) :
                                            &items_structure[i];
        assert(item_structure);

        /* Copie tout sauf l'identifiant qui peut être différent. */
        item_structure->genre = item_résultat->genre;
        item_structure->type = item_résultat->type;
        item_structure->valeur = item_résultat->valeur;
    }

    auto monomorphisations = structure_construite->monomorphisations;
    auto monomorphisation = monomorphisations->trouve_monomorphisation(items_structure);

    if (!monomorphisation) {
        erreur_monomorphisation_inconnue(construction, items_résultat, monomorphisations);
        return nullptr;
    }

    return monomorphisation->comme_déclaration_type();
}

Type *Monomorpheuse::résoud_type_final_pour_construction_opaque(
    const NoeudExpressionConstructionStructure *construction)
{
    auto declaration_appelee = construction->noeud_fonction_appelée;
    auto opaque_construite = declaration_appelee->comme_type_opaque();

    auto expression_opacifie = opaque_construite->expression_type;
    if (!expression_opacifie->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
        return opaque_construite->type;
    }

    /* Nous n'avons qu'un seul paramètre pour l'instant. */
    auto param = construction->paramètres[0];

    if (param->est_assignation_variable()) {
        /* L'argument est nommé. */
        auto assign = param->comme_assignation_variable();
        param = assign->expression;
    }

    if (!param->est_référence_déclaration()) {
        // À FAIRE(monomorphisation) : erreur ?
        return nullptr;
    }

    auto decl_referee = param->comme_référence_déclaration()->déclaration_référée;

    auto item_résultat = trouve_item_pour_ident(items_résultat, decl_referee->ident);
    assert(item_résultat);

    /* À FAIRE(opaque) : il faudrait pouvoir vérifier que la monomorphisation existe. */
    return typeuse.monomorphe_opaque(
        opaque_construite,
        const_cast<Type *>(item_résultat->type->comme_type_type_de_données()->type_connu));
}

Type *Monomorpheuse::résoud_type_final_pour_déclaration_tranche(
    const NoeudExpressionTypeTranche *expr_tranche)
{
    auto type_pointe = résoud_type_final_impl(expr_tranche->expression_type);
    if (!type_pointe) {
        return nullptr;
    }
    return typeuse.crée_type_tranche(type_pointe);
}

Type *Monomorpheuse::résoud_type_final_pour_déclaration_tableau_dynamique(
    const NoeudExpressionTypeTableauDynamique *expr_tableau_dynamique)
{
    auto type_pointe = résoud_type_final_impl(expr_tableau_dynamique->expression_type);
    if (!type_pointe) {
        return nullptr;
    }
    return typeuse.type_tableau_dynamique(type_pointe);
}

Type *Monomorpheuse::résoud_type_final_pour_déclaration_tableau_fixe(
    const NoeudExpressionTypeTableauFixe *expr_tableau_fixe)
{
    auto type_pointe = résoud_type_final_impl(expr_tableau_fixe->expression_type);
    if (!type_pointe) {
        return nullptr;
    }

    auto expression_taille = expr_tableau_fixe->expression_taille;
    if (expression_taille->est_référence_déclaration()) {
        auto item = trouve_item_pour_ident(items_résultat, expression_taille->ident);
        assert(item);
        return typeuse.type_tableau_fixe(type_pointe, int32_t(item->valeur.entière()));
    }
    auto valeur_taille = évalue_valeur(expression_taille);
    if (!valeur_taille.est_entière()) {
        erreur_sémantique(expression_taille, "La taille du tableau n'est pas une valeur entière");
        return nullptr;
    }
    return typeuse.type_tableau_fixe(type_pointe, static_cast<int>(valeur_taille.entière()));
}

Type *Monomorpheuse::résoud_type_final_pour_expansion_variadique(
    const NoeudExpressionExpansionVariadique *expansion)
{
    auto type_pointe = résoud_type_final_impl(expansion->expression);
    if (!type_pointe) {
        return nullptr;
    }
    return typeuse.type_variadique(type_pointe);
}

RésultatContrainte Monomorpheuse::applique_contrainte(ItemMonomorphisation const &item,
                                                      ItemMonomorphisation const &candidat)
{
    if (item.genre != candidat.genre) {
        return ÉtatRésolutionContrainte::PasLaMêmeChose;
    }

    if (item.genre == GenreItem::TYPE_DE_DONNÉES) {
        /* Nous avons un type sans contrainte, donc tout est bon. */
        if (!item.contrainte_type) {
            return ÉtatRésolutionContrainte::Ok;
        }

        /* Vérifie que la containte est respectée. */

        auto type_de_données = candidat.type->comme_type_type_de_données();

        /* Vérifie si la contrainte est celle d'un type polymorphique. */
        if (type_de_données->type_connu->est_type_structure()) {
            auto type_structure = type_de_données->type_connu->comme_type_structure();
            if (item.contrainte_type == type_structure->polymorphe_de_base) {
                return ÉtatRésolutionContrainte::Ok;
            }
        }
        else if (type_de_données->type_connu->est_type_union()) {
            auto type_union = type_de_données->type_connu->comme_type_union();
            if (item.contrainte_type == type_union->polymorphe_de_base) {
                return ÉtatRésolutionContrainte::Ok;
            }
        }

        /* Vérification pour un type normal. */

        auto résultat = cherche_transformation_pour_transtypage(type_de_données->type_connu,
                                                                item.contrainte_type);
        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            return ÉtatRésolutionContrainte::ContrainteNonRespectée;
        }

        return ÉtatRésolutionContrainte::Ok;
    }

    /* Nous avons une valeur, il faut vérifier le type. */
    auto type_item = item.type;

    if (type_item == nullptr) {
        /* Type de valeur indéfini (par exemple pour la taille polymorphique d'un tableau). */
        return ÉtatRésolutionContrainte::Ok;
    }

    if (type_item->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        /* Le type peut être polymorphique, par exemple fonc(T)(rien), dans lequel cas il nous
         * faut d'abord résoudre le type final. */
        type_item = résoud_type_final_impl(item.expression_type);
    }

    auto résultat = cherche_transformation(candidat.type, type_item);
    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);
    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        return ÉtatRésolutionContrainte::PasLeMêmeType;
    }

    return ÉtatRésolutionContrainte::Ok;
}

RésultatCompatibilité Monomorpheuse::sont_compatibles(ItemMonomorphisation const &item,
                                                      ItemMonomorphisation const &candidat)
{
    if (item.genre == GenreItem::TYPE_DE_DONNÉES) {
        auto résultat = cherche_transformation(candidat.type, item.type);
        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            return ÉtatRésolutionCompatibilité::PasLeMêmeType;
        }

        return ÉtatRésolutionCompatibilité::Ok;
    }

    /* Nous avons une valeur. */
    if (item.valeur != candidat.valeur) {
        return ÉtatRésolutionCompatibilité::PasLaMêmeValeur;
    }

    return ÉtatRésolutionCompatibilité::Ok;
}

RésultatUnification Monomorpheuse::unifie()
{
    POUR (candidats) {
        auto contrainte = item_pour_ident(it.ident);
        if (!contrainte) {
            erreur_item_manquant(nullptr, it);
            return erreur().value();
        }

        if (contrainte->genre == GenreItem::INDÉFINI) {
            contrainte->genre = it.genre;
            contrainte->type = it.type;
            contrainte->valeur = it.valeur;
            contrainte->expression_type = it.expression_type;
        }
        else {
            auto résultat_contrainte = applique_contrainte(*contrainte, it);
            if (std::holds_alternative<Attente>(résultat_contrainte)) {
                return std::get<Attente>(résultat_contrainte);
            }

            auto erreur_cont = std::get<ÉtatRésolutionContrainte>(résultat_contrainte);
            if (erreur_cont != ÉtatRésolutionContrainte::Ok) {
                erreur_contrainte(nullptr, erreur_cont, *contrainte, it);
                return erreur().value();
            }
        }

        auto résultat = item_résultat_pour_ident(it.ident);

        /* Initialise le résultat si c'est le premier élément que nous avons pour lui. */
        if (!résultat->type) {
            *résultat = it;
        }
        else {
            auto résultat_compatibilité = sont_compatibles(*résultat, it);
            if (std::holds_alternative<Attente>(résultat_compatibilité)) {
                return std::get<Attente>(résultat_compatibilité);
            }

            auto erreur_compat = std::get<ÉtatRésolutionCompatibilité>(résultat_compatibilité);
            if (erreur_compat != ÉtatRésolutionCompatibilité::Ok) {
                erreur_compatibilité(nullptr, erreur_compat, *résultat, it);
                return erreur().value();
            }
        }

        /* Évite les entiers constants dans le type final. */
        if (résultat->type->est_type_entier_constant()) {
            résultat->type = contrainte->type;
        }
    }

    return true;
}

RésultatMonomorphisation détermine_monomorphisation(
    Monomorpheuse &monomorpheuse,
    const NoeudDéclarationEntêteFonction *entête,
    kuri::tableau_statique<NoeudExpression *> arguments_reçus)
{
#if 0
    SUR_SORTIE_PORTEE {
        if (entête->possède_drapeau(DrapeauxNoeud::DEBOGUE)) {
            monomorpheuse.logue();
        }
    };
#endif

    for (auto i = int64_t(0); i < arguments_reçus.taille(); ++i) {
        auto indice_arg = std::min(i, static_cast<int64_t>(entête->params.taille() - 1));
        auto param = entête->parametre_entree(indice_arg);
        auto slot = arguments_reçus[i];

        if (param->type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            monomorpheuse.parse_candidats(param->expression_type, slot, slot->type);
        }

        if (param->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            if (param->type->est_type_type_de_données()) {
                monomorpheuse.ajoute_candidat(param->ident, slot->type);
            }
            else {
                auto valeur = monomorpheuse.évalue_valeur(slot);
                monomorpheuse.ajoute_candidat_valeur(param->ident, slot->type, valeur);
            }
        }

        if (monomorpheuse.a_une_erreur()) {
            return monomorpheuse.erreur().value();
        }
    }

    auto résultat = monomorpheuse.unifie();
    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    if (std::holds_alternative<ErreurMonomorphisation>(résultat)) {
        return std::get<ErreurMonomorphisation>(résultat);
    }

    return true;
}
