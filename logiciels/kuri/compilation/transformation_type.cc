/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "transformation_type.hh"

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/outils_lexemes.hh"

#include "espace_de_travail.hh"
#include "typage.hh"
#include "validation_semantique.hh"

#include "utilitaires/log.hh"

#define REQUIERS_TYPE_VALIDE(variable)                                                            \
    do {                                                                                          \
        if (!variable->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {                 \
            return Attente::sur_type(variable);                                                   \
        }                                                                                         \
    } while (0)

const char *chaine_transformation(TypeTransformation type)
{
    switch (type) {
#define ENUMERE_TYPE_TRANSFORMATION_EX(type)                                                      \
    case TypeTransformation::type:                                                                \
        return #type;
        ENUMERE_TYPES_TRANSFORMATION
#undef ENUMERE_TYPE_TRANSFORMATION_EX
    }
    return "ERREUR";
}

std::ostream &operator<<(std::ostream &os, TypeTransformation type)
{
    os << chaine_transformation(type);
    return os;
}

/* Trouve la transformation nécessaire pour aller d'un type à un autre de
 * manière conditionnelle.
 *
 * L'utilisation du graphe de dépendance pour trouver les conversions entre
 * types fut considéré mais la vitesse de compilation souffra énormément.
 * Ce fut 50x plus lent que la méthode ci-bas où toutes les relations possibles
 * sont codées en dur. Un profilage montra que la plupart du temps était passé à
 * enfiler et défiler les noeuds à visiter, mais aussi à vérifier si un noeud a
 * déjà été visité.
 *
 * Un point fort du graphe serait d'avoir des conversion complexes entre par
 * exemple des unités de mesure (p.e. de centimètre (cm) vers pieds (ft)) sans
 * devoir explicitement avoir cette conversion dans le code tant que types se
 * trouvent entre les deux (p.e. cm -> dm -> m -> ft). Ou encore automatiquement
 * construire des unions depuis une valeur d'un type membre.
 *
 * Cette méthode est limitée, mais largement plus rapide que celle utilisant un
 * graphe, qui sera sans doute révisée plus tard.
 */
template <bool POUR_TRANSTYPAGE>
ResultatTransformation peut_transformer_type_vers_entier(Type const *type_de,
                                                         Type const *type_vers)
{
    if (type_de->genre == type_vers->genre) {
        if (type_de->taille_octet < type_vers->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        if (POUR_TRANSTYPAGE) {
            if (type_de->taille_octet > type_vers->taille_octet) {
                return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
            }
        }

        return TransformationType(TypeTransformation::IMPOSSIBLE);
    }

    if (type_de->est_type_entier_constant()) {
        return TransformationType{TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers};
    }

    if (!POUR_TRANSTYPAGE) {
        return TransformationType{TypeTransformation::IMPOSSIBLE};
    }

    if (type_de->est_type_réel()) {
        return TransformationType{TypeTransformation::REEL_VERS_ENTIER, type_vers};
    }

    if ((type_de->est_type_pointeur() || type_de->est_type_fonction() ||
         type_de->est_type_adresse_fonction()) &&
        est_type_entier(type_vers) && type_vers->taille_octet == 8) {
        return TransformationType{TypeTransformation::POINTEUR_VERS_ENTIER, type_vers};
    }

    if (type_de->est_type_octet()) {
        if (type_vers->taille_octet > type_de->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
    }

    if (type_de->est_type_bool()) {
        if (type_vers->taille_octet > type_de->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        return TransformationType(TypeTransformation::INUTILE);
    }

    if (type_de->est_type_énum()) {
        if (type_vers == static_cast<TypeEnum const *>(type_de)->type_sous_jacent) {
            // on pourrait se passer de la conversion, ou normaliser le type
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    // converti relatif <-> naturel
    if (est_type_entier(type_de)) {
        if (type_de->taille_octet > type_vers->taille_octet) {
            return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
        }

        if (type_de->taille_octet < type_vers->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
    }

    return TransformationType{TypeTransformation::IMPOSSIBLE};
}

template <bool POUR_TRANSTYPAGE>
ResultatTransformation cherche_transformation(Type const *type_de, Type const *type_vers)
{
    if (type_de == type_vers) {
        return TransformationType(TypeTransformation::INUTILE);
    }

    /* nous avons un type de données pour chaque type connu lors de la
     * compilation, donc testons manuellement la compatibilité */
    if (type_de->est_type_type_de_données() && type_vers->est_type_type_de_données()) {
        return TransformationType(TypeTransformation::INUTILE);
    }

    switch (type_vers->genre) {
        case GenreNoeud::CHAINE:
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            break;
        }
        case GenreNoeud::EINI:
        {
            /* Il nous faut attendre sur le type pour pouvoir générer l'InfoType. */
            REQUIERS_TYPE_VALIDE(type_de);
            return TransformationType(TypeTransformation::CONSTRUIT_EINI);
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type_vers->comme_type_union();

            REQUIERS_TYPE_VALIDE(type_vers);

            auto résultat = trouve_index_membre_unique_type_compatible(type_union, type_de);

            /* Nous pouvons construire une union depuis nul si un seul membre est un pointeur. */
            if (std::holds_alternative<IndexMembre>(résultat)) {
                return TransformationType{TypeTransformation::CONSTRUIT_UNION,
                                          type_vers,
                                          std::get<IndexMembre>(résultat).valeur};
            }
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_pointe = type_vers->comme_type_tableau_dynamique()->type_pointé;

            if (type_pointe->est_type_octet()) {
                // a : [..]octet = nul, voir bug19
                if (type_de->est_type_pointeur()) {
                    auto type_pointe_de = type_de->comme_type_pointeur()->type_pointé;

                    if (type_pointe_de == nullptr) {
                        return TransformationType(TypeTransformation::IMPOSSIBLE);
                    }
                }

                return TransformationType(TypeTransformation::CONSTRUIT_TABL_OCTET);
            }

            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_élément = type_vers->comme_type_tranche()->type_élément;
            if (type_de->est_type_tableau_fixe()) {
                if (type_élément == type_de->comme_type_tableau_fixe()->type_pointé) {
                    return TransformationType(
                        TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE, type_vers);
                }
                return TransformationType(TypeTransformation::IMPOSSIBLE);
            }

            if (type_de->est_type_tableau_dynamique()) {
                if (type_élément == type_de->comme_type_tableau_dynamique()->type_pointé) {
                    return TransformationType(
                        TypeTransformation::CONVERTI_TABLEAU_DYNAMIQUE_VERS_TRANCHE, type_vers);
                }
                return TransformationType(TypeTransformation::IMPOSSIBLE);
            }

            break;
        }
        case GenreNoeud::BOOL:
        {
            if (POUR_TRANSTYPAGE) {
                if (est_type_entier(type_de)) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }
            }
            break;
        }
        case GenreNoeud::OCTET:
        {
            if (type_de->est_type_entier_constant()) {
                return TransformationType{TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers};
            }
            if (POUR_TRANSTYPAGE) {
                if (est_type_entier(type_de)) {
                    if (type_vers->taille_octet < type_de->taille_octet) {
                        return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE,
                                                  type_vers};
                    }

                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }
            }
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        {
            auto résultat = peut_transformer_type_vers_entier<POUR_TRANSTYPAGE>(type_de,
                                                                                type_vers);
            if (!std::holds_alternative<TransformationType>(résultat)) {
                return résultat;
            }
            auto transformation = std::get<TransformationType>(résultat);
            if (transformation.type != TypeTransformation::IMPOSSIBLE) {
                return transformation;
            }
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            if (POUR_TRANSTYPAGE) {
                if (type_de->est_type_entier_constant()) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }

                REQUIERS_TYPE_VALIDE(type_vers);

                if (static_cast<TypeEnum const *>(type_vers)->type_sous_jacent == type_de) {
                    // on pourrait se passer de la conversion, ou normaliser le type
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }
            }
            break;
        }
        case GenreNoeud::RÉEL:
        {
            if (type_de->est_type_réel()) {
                /* cas spéciaux pour R16 */
                if (type_de->taille_octet == 2) {
                    if (type_vers->taille_octet == 4) {
                        return TransformationType{TypeTransformation::R16_VERS_R32, type_vers};
                    }

                    if (type_vers->taille_octet == 8) {
                        return TransformationType{TypeTransformation::R16_VERS_R64, type_vers};
                    }

                    return TransformationType(TypeTransformation::IMPOSSIBLE);
                }

                if (type_de->taille_octet < type_vers->taille_octet) {
                    return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
                }

                if (POUR_TRANSTYPAGE) {
                    /* cas spéciaux pour R16 */
                    if (type_vers->taille_octet == 2) {
                        if (type_de->taille_octet == 4) {
                            return TransformationType{TypeTransformation::R32_VERS_R16, type_vers};
                        }

                        if (type_de->taille_octet == 8) {
                            return TransformationType{TypeTransformation::R64_VERS_R16, type_vers};
                        }

                        return TransformationType(TypeTransformation::IMPOSSIBLE);
                    }

                    if (type_de->taille_octet > type_vers->taille_octet) {
                        return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE,
                                                  type_vers};
                    }
                }

                return TransformationType(TypeTransformation::IMPOSSIBLE);
            }

            if (type_de->est_type_entier_constant()) {
                return TransformationType{TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers};
            }

            if (POUR_TRANSTYPAGE) {
                // À FAIRE(r16)
                if (est_type_entier(type_de)) {
                    return TransformationType{TypeTransformation::ENTIER_VERS_REEL, type_vers};
                }
            }

            break;
        }
        case GenreNoeud::RÉFÉRENCE:
        {
            auto type_élément_vers = type_vers->comme_type_référence()->type_pointé;

            if (type_de->est_type_référence()) {
                auto type_élément_de = type_de->comme_type_référence()->type_pointé;

                REQUIERS_TYPE_VALIDE(type_élément_de);
                REQUIERS_TYPE_VALIDE(type_élément_vers);

                auto décalage_type_base = est_type_de_base(type_élément_de, type_élément_vers);
                if (décalage_type_base) {
                    return TransformationType::vers_base(type_vers, décalage_type_base.value());
                }
            }

            if (type_élément_vers == type_de) {
                return TransformationType(TypeTransformation::PREND_REFERENCE);
            }

            REQUIERS_TYPE_VALIDE(type_de);
            REQUIERS_TYPE_VALIDE(type_élément_vers);

            auto décalage_type_base = est_type_de_base(type_de, type_élément_vers);
            if (décalage_type_base) {
                return TransformationType::prend_référence_vers_base(type_vers,
                                                                     décalage_type_base.value());
            }

            if (POUR_TRANSTYPAGE) {
                if (type_de->est_type_référence()) {
                    auto type_pointe_de = type_de->comme_type_référence()->type_pointé;
                    auto type_pointe_vers = type_vers->comme_type_référence()->type_pointé;

                    if (type_pointe_de->est_type_structure() &&
                        type_pointe_vers->est_type_structure()) {
                        auto ts_de = type_pointe_de->comme_type_structure();
                        auto ts_vers = type_pointe_vers->comme_type_structure();

                        décalage_type_base = est_type_de_base(ts_vers, ts_de);
                        if (décalage_type_base) {
                            return TransformationType::vers_dérivé(type_vers,
                                                                   décalage_type_base.value());
                        }
                    }
                }
            }
            break;
        }
        case GenreNoeud::POINTEUR:
        {
            if (type_de->est_type_pointeur()) {
                auto type_pointe_de = type_de->comme_type_pointeur()->type_pointé;
                auto type_pointe_vers = type_vers->comme_type_pointeur()->type_pointé;

                /* x = nul; */
                if (type_pointe_de == nullptr) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }

                /* x : *z8 = y (*rien) */
                if (type_pointe_de->est_type_rien()) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }

                /* x : *nul = y */
                if (type_pointe_vers == nullptr) {
                    return TransformationType(TypeTransformation::INUTILE);
                }

                /* x : *rien = y; */
                if (type_pointe_vers->est_type_rien()) {
                    return TransformationType(TypeTransformation::CONVERTI_VERS_PTR_RIEN);
                }

                /* x : *octet = y; */
                // À FAIRE : pour transtypage uniquement
                if (type_pointe_vers->est_type_octet()) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }

                if (type_pointe_de->est_type_structure() &&
                    type_pointe_vers->est_type_structure()) {
                    auto ts_de = type_pointe_de->comme_type_structure();
                    auto ts_vers = type_pointe_vers->comme_type_structure();

                    REQUIERS_TYPE_VALIDE(ts_de);
                    REQUIERS_TYPE_VALIDE(ts_vers);

                    auto décalage_type_base = est_type_de_base(ts_de, ts_vers);
                    if (décalage_type_base) {
                        return TransformationType::vers_base(type_vers,
                                                             décalage_type_base.value());
                    }

                    if (POUR_TRANSTYPAGE) {
                        décalage_type_base = est_type_de_base(ts_vers, ts_de);
                        if (décalage_type_base) {
                            return TransformationType::vers_dérivé(type_vers,
                                                                   décalage_type_base.value());
                        }
                    }
                }

                if (POUR_TRANSTYPAGE) {
                    // À FAIRE : pour les einis, nous devrions avoir une meilleure sûreté de type
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }
            }

            if (POUR_TRANSTYPAGE) {
                if ((est_type_entier(type_de) || type_de->est_type_entier_constant())) {
                    return TransformationType{TypeTransformation::ENTIER_VERS_POINTEUR, type_vers};
                }
            }
            break;
        }
        case GenreNoeud::FONCTION:
        {
            /* x : fonc()rien = nul; */
            if (type_de->est_type_pointeur() &&
                type_de->comme_type_pointeur()->type_pointé == nullptr) {
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }
            break;
        }
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            /* x : adresse_fonction = nul; */
            if (type_de->est_type_pointeur() &&
                type_de->comme_type_pointeur()->type_pointé == nullptr) {
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }

            if (type_de->est_type_fonction()) {
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }

            if (POUR_TRANSTYPAGE) {
                if (type_de == TypeBase::PTR_RIEN) {
                    return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                              type_vers};
                }
            }

            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            break;
        }
        case GenreNoeud::TUPLE:
        {
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false,
                          [&]() { dbg() << "Noeud non-géré pour type : " << type_vers->genre; });
            break;
        }
    }

    if (POUR_TRANSTYPAGE) {
        if (type_de->est_type_opaque() &&
            type_vers == type_de->comme_type_opaque()->type_opacifié) {
            // on pourrait se passer de la conversion, ou normaliser le type
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    if (type_de->est_type_union()) {
        auto type_union = type_de->comme_type_union();

        REQUIERS_TYPE_VALIDE(type_union);

        POUR_INDEX (type_union->donne_membres_pour_code_machine()) {
            if (it.type != type_vers || type_union->est_nonsure) {
                continue;
            }

            return TransformationType{TypeTransformation::EXTRAIT_UNION, type_vers, index_it};
        }
    }

    if (type_de->est_type_eini()) {
        return TransformationType{TypeTransformation::EXTRAIT_EINI, type_vers};
    }

    if (type_de->est_type_référence()) {
        if (type_de->comme_type_référence()->type_pointé == type_vers) {
            return TransformationType(TypeTransformation::DEREFERENCE);
        }
    }

    if (type_de->est_type_opaque()) {
        auto type_opacifié = type_de->comme_type_opaque()->type_opacifié;
        auto résultat = cherche_transformation(type_opacifié, type_vers);
        if (!std::holds_alternative<TransformationType>(résultat)) {
            return résultat;
        }

        auto transformation = std::get<TransformationType>(résultat);
        /* Préserve la transformation d'opaque de pointeur vers *octet. */
        if (transformation.type == TypeTransformation::CONVERTI_VERS_TYPE_CIBLE) {
            if (type_opacifié->est_type_pointeur() && type_vers == TypeBase::PTR_OCTET) {
                return résultat;
            }
        }

        /* Tout autre conversion est impossible. */
        if (transformation.type != TypeTransformation::INUTILE) {
            return TransformationType(TypeTransformation::IMPOSSIBLE);
        }

        return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
    }

    return TransformationType(TypeTransformation::IMPOSSIBLE);
}

ResultatTransformation cherche_transformation(Type const *type_de, Type const *type_vers)
{
    return cherche_transformation<false>(type_de, type_vers);
}

ResultatTransformation cherche_transformation_pour_transtypage(Type const *type_de,
                                                               Type const *type_vers)
{
    return cherche_transformation<true>(type_de, type_vers);
}

ResultatPoidsTransformation vérifie_compatibilité(Type const *type_vers, Type const *type_de)
{
    auto résultat = cherche_transformation<false>(type_de, type_vers);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);

    if (transformation.type == TypeTransformation::INUTILE) {
        /* ne convertissons pas implicitement vers *nul quand nous avons une opérande */
        if (type_vers->est_type_pointeur() &&
            type_vers->comme_type_pointeur()->type_pointé == nullptr && type_vers != type_de) {
            return PoidsTransformation{transformation, 0.0};
        }

        return PoidsTransformation{transformation, 1.0};
    }

    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        return PoidsTransformation{transformation, 0.0};
    }

    /* nous savons que nous devons transformer la valeur (par ex. eini), donc
     * donne un mi-poids à l'argument */
    return PoidsTransformation{transformation, 0.5};
}

ResultatPoidsTransformation vérifie_compatibilité(Type const *type_vers,
                                                  Type const *type_de,
                                                  NoeudExpression const *noeud)
{
    auto résultat = cherche_transformation<false>(type_de, type_vers);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);

    if (transformation.type == TypeTransformation::INUTILE) {
        return PoidsTransformation{transformation, 1.0};
    }

    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        return PoidsTransformation{transformation, 0.0};
    }

    if (transformation.type == TypeTransformation::PREND_REFERENCE) {
        return PoidsTransformation{transformation,
                                   est_valeur_gauche(noeud->genre_valeur) ? 1.0 : 0.0};
    }

    /* nous savons que nous devons transformer la valeur (par ex. eini), donc
     * donne un mi-poids à l'argument */
    return PoidsTransformation{transformation, 0.5};
}

std::ostream &operator<<(std::ostream &os, TransformationType type)
{
    return os << type.type;
}
