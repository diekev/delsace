/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "transformation_type.hh"

#include "parsage/outils_lexemes.hh"

#include "espace_de_travail.hh"
#include "typage.hh"
#include "validation_semantique.hh"

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

/** Si \a type_base_potentiel est un type employé par \a type_dérivé, ou employé par un type
 * employé par \a type_dérivé, retourne le décalage absolu en octet dans la structure de \a
 * type_dérivé du \a type_base_potentiel. Ceci prend en compte le décalage des emplois
 * intermédiaire pour arriver à \a type_base_potentiel.
 * S'il n'y a aucune filliation entre les types, ne retourne rien. */
static std::optional<uint32_t> est_type_de_base(TypeStructure const *type_dérivé,
                                                TypeStructure const *type_base_potentiel)
{
    POUR (type_dérivé->types_employés) {
        auto struct_employée = it->type->comme_type_structure();
        if (struct_employée == type_base_potentiel) {
            return it->decalage;
        }

        auto décalage_depuis_struct_employée = est_type_de_base(struct_employée,
                                                                type_base_potentiel);
        if (décalage_depuis_struct_employée) {
            return it->decalage + décalage_depuis_struct_employée.value();
        }
    }

    return {};
}

static std::optional<uint32_t> est_type_de_base(Type const *type_dérivé,
                                                Type const *type_base_potentiel)
{
    if (type_dérivé->est_type_structure() && type_base_potentiel->est_type_structure()) {
        return est_type_de_base(type_dérivé->comme_type_structure(),
                                type_base_potentiel->comme_type_structure());
    }

    return {};
}

template <typename T, int tag>
struct ValeurOpaqueTaguee {
    T valeur;
};

enum {
    INDEX_MEMBRE = 0,
    AUCUN_TROUVE = 1,
    PLUSIEURS_TROUVES = 2,
};

using IndexMembre = ValeurOpaqueTaguee<int, INDEX_MEMBRE>;
using PlusieursMembres = ValeurOpaqueTaguee<int, PLUSIEURS_TROUVES>;
using AucunMembre = ValeurOpaqueTaguee<int, AUCUN_TROUVE>;

using ResultatRechercheMembre = std::variant<IndexMembre, PlusieursMembres, AucunMembre>;

static bool est_type_pointeur_nul(Type const *type)
{
    return type->est_type_pointeur() && type->comme_type_pointeur()->type_pointe == nullptr;
}

static ResultatRechercheMembre trouve_index_membre_unique_type_compatible(
    TypeCompose const *type, Type const *type_a_tester)
{
    auto const pointeur_nul = est_type_pointeur_nul(type_a_tester);
    int index_membre = -1;
    int index_courant = 0;
    POUR (type->membres) {
        if (it.type == type_a_tester) {
            if (index_membre != -1) {
                return PlusieursMembres{-1};
            }

            index_membre = index_courant;
        }
        else if (type_a_tester->est_type_pointeur() && it.type->est_type_pointeur()) {
            if (pointeur_nul) {
                if (index_membre != -1) {
                    return PlusieursMembres{-1};
                }

                index_membre = index_courant;
            }
            else {
                auto type_pointe_de = type_a_tester->comme_type_pointeur()->type_pointe;
                auto type_pointe_vers = it.type->comme_type_pointeur()->type_pointe;

                if (est_type_de_base(type_pointe_de, type_pointe_vers)) {
                    if (index_membre != -1) {
                        return PlusieursMembres{-1};
                    }

                    index_membre = index_courant;
                }
            }
        }
        else if (est_type_entier(it.type) && type_a_tester->est_type_entier_constant()) {
            if (index_membre != -1) {
                return PlusieursMembres{-1};
            }

            index_membre = index_courant;
        }

        index_courant += 1;
    }

    if (index_membre == -1) {
        return AucunMembre{-1};
    }

    return IndexMembre{index_membre};
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
ResultatTransformation cherche_transformation(Type const *type_de, Type const *type_vers)
{
    if (type_de == type_vers) {
        return TypeTransformation::INUTILE;
    }

    /* nous avons un type de données pour chaque type connu lors de la
     * compilation, donc testons manuellement la compatibilité */
    if (type_de->genre == GenreType::TYPE_DE_DONNEES &&
        type_vers->genre == GenreType::TYPE_DE_DONNEES) {
        return TypeTransformation::INUTILE;
    }

    // À FAIRE(r16)
    if (type_de->genre == GenreType::ENTIER_CONSTANT &&
        (est_type_entier(type_vers) || type_vers->est_type_octet() ||
         type_vers->est_type_reel())) {
        return TransformationType{TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers};
    }

    if (POUR_TRANSTYPAGE) {
        if (est_type_entier(type_vers) && type_de->genre == GenreType::OCTET) {
            if (type_vers->taille_octet > type_de->taille_octet) {
                return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
            }

            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        if (est_type_entier(type_de)) {
            if (type_vers->genre == GenreType::OCTET) {
                if (type_vers->taille_octet < type_de->taille_octet) {
                    return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
                }

                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }

            if (type_vers->genre == GenreType::BOOL) {
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }
        }

        if (type_vers->genre == GenreType::ERREUR &&
            type_de->genre == GenreType::ENTIER_CONSTANT) {
            return TransformationType{TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers};
        }

        if (type_de->genre == GenreType::BOOL && est_type_entier(type_vers)) {
            if (type_vers->taille_octet > type_de->taille_octet) {
                return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
            }

            return TypeTransformation::INUTILE;
        }

        if (type_de->genre == GenreType::ENUM) {
            if (type_vers == static_cast<TypeEnum const *>(type_de)->type_donnees) {
                // on pourrait se passer de la conversion, ou normaliser le type
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }
        }

        if (type_de->genre == GenreType::ERREUR) {
            if (type_vers == type_de->comme_type_erreur()->type_donnees) {
                // on pourrait se passer de la conversion, ou normaliser le type
                return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
            }
        }

        if (type_vers->genre == GenreType::ENUM &&
            static_cast<TypeEnum const *>(type_vers)->type_donnees == type_de) {
            // on pourrait se passer de la conversion, ou normaliser le type
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        if (type_de->est_type_opaque() &&
            type_vers == type_de->comme_type_opaque()->type_opacifie) {
            // on pourrait se passer de la conversion, ou normaliser le type
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    if (type_de->genre == GenreType::ENTIER_CONSTANT && type_vers->genre == GenreType::ENUM) {
        // on pourrait se passer de la conversion, ou normaliser le type
        return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
    }

    if (type_de->genre == GenreType::ENTIER_NATUREL &&
        type_vers->genre == GenreType::ENTIER_NATUREL) {
        if (type_de->taille_octet < type_vers->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        if (POUR_TRANSTYPAGE) {
            if (type_de->taille_octet > type_vers->taille_octet) {
                return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
            }
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_de->genre == GenreType::ENTIER_RELATIF &&
        type_vers->genre == GenreType::ENTIER_RELATIF) {
        if (type_de->taille_octet < type_vers->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        if (POUR_TRANSTYPAGE) {
            if (type_de->taille_octet > type_vers->taille_octet) {
                return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
            }
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (POUR_TRANSTYPAGE) {
        if (est_type_entier(type_de) && type_vers->genre == GenreType::REEL) {
            return TransformationType{TypeTransformation::ENTIER_VERS_REEL, type_vers};
        }

        if (est_type_entier(type_vers) && type_de->genre == GenreType::REEL) {
            return TransformationType{TypeTransformation::REEL_VERS_ENTIER, type_vers};
        }

        // converti relatif <-> naturel
        if (est_type_entier(type_de) && est_type_entier(type_vers)) {
            if (type_de->taille_octet > type_vers->taille_octet) {
                return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
            }

            if (type_de->taille_octet < type_vers->taille_octet) {
                return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
            }

            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    if (type_de->genre == GenreType::REEL && type_vers->genre == GenreType::REEL) {
        /* cas spéciaux pour R16 */
        if (type_de->taille_octet == 2) {
            if (type_vers->taille_octet == 4) {
                return TransformationType{TypeTransformation::R16_VERS_R32, type_vers};
            }

            if (type_vers->taille_octet == 8) {
                return TransformationType{TypeTransformation::R16_VERS_R64, type_vers};
            }

            return TypeTransformation::IMPOSSIBLE;
        }

        /* cas spéciaux pour R16 */
        if (type_vers->taille_octet == 2) {
            if (type_de->taille_octet == 4) {
                return TransformationType{TypeTransformation::R32_VERS_R16, type_vers};
            }

            if (type_de->taille_octet == 8) {
                return TransformationType{TypeTransformation::R64_VERS_R16, type_vers};
            }

            return TypeTransformation::IMPOSSIBLE;
        }

        if (type_de->taille_octet < type_vers->taille_octet) {
            return TransformationType{TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers};
        }

        if (POUR_TRANSTYPAGE) {
            if (type_de->taille_octet > type_vers->taille_octet) {
                return TransformationType{TypeTransformation::REDUIT_TAILLE_TYPE, type_vers};
            }
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_vers->genre == GenreType::UNION) {
        auto type_union = type_vers->comme_type_union();

        if ((type_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_vers);
        }

        auto resultat = trouve_index_membre_unique_type_compatible(type_union, type_de);

        /* Nous pouvons construire une union depuis nul si un seul membre est un pointeur. */
        if (std::holds_alternative<IndexMembre>(resultat)) {
            return TransformationType{TypeTransformation::CONSTRUIT_UNION,
                                      type_vers,
                                      std::get<IndexMembre>(resultat).valeur};
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_vers->genre == GenreType::EINI) {
        /* Il nous faut attendre sur le type pour pouvoir générer l'InfoType. */
        if ((type_de->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_de);
        }
        return TypeTransformation::CONSTRUIT_EINI;
    }

    if (type_de->genre == GenreType::UNION) {
        auto type_union = type_de->comme_type_union();

        if ((type_union->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_union);
        }

        POUR_INDEX (type_union->membres) {
            if (it.type != type_vers || type_union->est_nonsure) {
                continue;
            }

            return TransformationType{TypeTransformation::EXTRAIT_UNION, type_vers, index_it};
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_de->genre == GenreType::EINI) {
        return TransformationType{TypeTransformation::EXTRAIT_EINI, type_vers};
    }

    if (type_vers->genre == GenreType::FONCTION) {
        /* x : fonc()rien = nul; */
        if (type_de->genre == GenreType::POINTEUR &&
            type_de->comme_type_pointeur()->type_pointe == nullptr) {
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        /* Nous savons que les types sont différents, donc si l'un des deux est un
         * pointeur fonction, nous pouvons retourner faux. */
        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_vers->genre == GenreType::REFERENCE) {
        auto type_élément_vers = type_vers->comme_type_reference()->type_pointe;

        if (type_de->est_type_reference()) {
            auto type_élément_de = type_de->comme_type_reference()->type_pointe;

            if ((type_élément_de->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_élément_de);
            }

            if ((type_élément_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_élément_vers);
            }

            auto décalage_type_base = est_type_de_base(type_élément_de, type_élément_vers);
            if (décalage_type_base) {
                return TransformationType::vers_base(type_vers, décalage_type_base.value());
            }
        }

        if (type_élément_vers == type_de) {
            return TypeTransformation::PREND_REFERENCE;
        }

        if ((type_de->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_de);
        }

        if ((type_élément_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_élément_vers);
        }

        auto décalage_type_base = est_type_de_base(type_de, type_élément_vers);
        if (décalage_type_base) {
            return TransformationType::prend_référence_vers_base(type_vers,
                                                                 décalage_type_base.value());
        }
    }

    if (type_de->genre == GenreType::REFERENCE) {
        if (type_de->comme_type_reference()->type_pointe == type_vers) {
            return TypeTransformation::DEREFERENCE;
        }
    }

    if (type_vers->genre == GenreType::TABLEAU_DYNAMIQUE) {
        auto type_pointe = type_vers->comme_type_tableau_dynamique()->type_pointe;

        if (type_pointe->genre == GenreType::OCTET) {
            // a : []octet = nul, voir bug19
            if (type_de->genre == GenreType::POINTEUR) {
                auto type_pointe_de = type_de->comme_type_pointeur()->type_pointe;

                if (type_pointe_de == nullptr) {
                    return TypeTransformation::IMPOSSIBLE;
                }
            }

            return TypeTransformation::CONSTRUIT_TABL_OCTET;
        }

        if (type_de->genre != GenreType::TABLEAU_FIXE) {
            return TypeTransformation::IMPOSSIBLE;
        }

        if (type_pointe == type_de->comme_type_tableau_fixe()->type_pointe) {
            return TypeTransformation::CONVERTI_TABLEAU;
        }

        return TypeTransformation::IMPOSSIBLE;
    }

    if (type_vers->genre == GenreType::POINTEUR && type_de->genre == GenreType::FONCTION) {
        auto type_pointe_vers = type_vers->comme_type_pointeur()->type_pointe;

        /* x : *z8 = y (*rien) */
        if (type_pointe_vers->genre == GenreType::RIEN) {
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    if (type_vers->genre == GenreType::POINTEUR && type_de->genre == GenreType::POINTEUR) {
        auto type_pointe_de = type_de->comme_type_pointeur()->type_pointe;
        auto type_pointe_vers = type_vers->comme_type_pointeur()->type_pointe;

        /* x = nul; */
        if (type_pointe_de == nullptr) {
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        /* x : *z8 = y (*rien) */
        if (type_pointe_de->genre == GenreType::RIEN) {
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        /* x : *nul = y */
        if (type_pointe_vers == nullptr) {
            return TypeTransformation::INUTILE;
        }

        /* x : *rien = y; */
        if (type_pointe_vers->genre == GenreType::RIEN) {
            return TypeTransformation::CONVERTI_VERS_PTR_RIEN;
        }

        /* x : *octet = y; */
        // À FAIRE : pour transtypage uniquement
        if (type_pointe_vers->genre == GenreType::OCTET) {
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }

        if (type_pointe_de->genre == GenreType::STRUCTURE &&
            type_pointe_vers->genre == GenreType::STRUCTURE) {
            auto ts_de = type_pointe_de->comme_type_structure();
            auto ts_vers = type_pointe_vers->comme_type_structure();

            if ((ts_de->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(ts_de);
            }

            if ((ts_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(ts_vers);
            }

            auto décalage_type_base = est_type_de_base(ts_de, ts_vers);
            if (décalage_type_base) {
                return TransformationType::vers_base(type_vers, décalage_type_base.value());
            }

            if (POUR_TRANSTYPAGE) {
                décalage_type_base = est_type_de_base(ts_vers, ts_de);
                if (décalage_type_base) {
                    return TransformationType::vers_dérivé(type_vers, décalage_type_base.value());
                }
            }
        }

        if (POUR_TRANSTYPAGE) {
            // À FAIRE : pour les einis, nous devrions avoir une meilleure sûreté de type
            return TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers};
        }
    }

    if (POUR_TRANSTYPAGE) {
        if ((type_de->est_type_pointeur() || type_de->est_type_fonction()) &&
            est_type_entier(type_vers) && type_vers->taille_octet == 8) {
            return TransformationType{TypeTransformation::POINTEUR_VERS_ENTIER, type_vers};
        }

        if (type_vers->genre == GenreType::POINTEUR &&
            (est_type_entier(type_de) || type_de->genre == GenreType::ENTIER_CONSTANT)) {
            return TransformationType{TypeTransformation::ENTIER_VERS_POINTEUR, type_vers};
        }

        if (type_de->est_type_reference() && type_vers->est_type_reference()) {
            auto type_pointe_de = type_de->comme_type_reference()->type_pointe;
            auto type_pointe_vers = type_vers->comme_type_reference()->type_pointe;

            if (type_pointe_de->est_type_structure() && type_pointe_vers->est_type_structure()) {
                auto ts_de = type_pointe_de->comme_type_structure();
                auto ts_vers = type_pointe_vers->comme_type_structure();

                auto décalage_type_base = est_type_de_base(ts_vers, ts_de);
                if (décalage_type_base) {
                    return TransformationType::vers_dérivé(type_vers, décalage_type_base.value());
                }
            }
        }
    }

    return TypeTransformation::IMPOSSIBLE;
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

ResultatPoidsTransformation verifie_compatibilite(Type const *type_arg, Type const *type_enf)
{
    auto resultat = cherche_transformation<false>(type_enf, type_arg);

    if (std::holds_alternative<Attente>(resultat)) {
        return std::get<Attente>(resultat);
    }

    auto transformation = std::get<TransformationType>(resultat);

    if (transformation.type == TypeTransformation::INUTILE) {
        /* ne convertissons pas implicitement vers *nul quand nous avons une opérande */
        if (type_arg->est_type_pointeur() &&
            type_arg->comme_type_pointeur()->type_pointe == nullptr && type_arg != type_enf) {
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

ResultatPoidsTransformation verifie_compatibilite(Type const *type_arg,
                                                  Type const *type_enf,
                                                  NoeudExpression const *enfant)
{
    auto resultat = cherche_transformation<false>(type_enf, type_arg);

    if (std::holds_alternative<Attente>(resultat)) {
        return std::get<Attente>(resultat);
    }

    auto transformation = std::get<TransformationType>(resultat);

    if (transformation.type == TypeTransformation::INUTILE) {
        return PoidsTransformation{transformation, 1.0};
    }

    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        return PoidsTransformation{transformation, 0.0};
    }

    if (transformation.type == TypeTransformation::PREND_REFERENCE) {
        return PoidsTransformation{transformation,
                                   est_valeur_gauche(enfant->genre_valeur) ? 1.0 : 0.0};
    }

    /* nous savons que nous devons transformer la valeur (par ex. eini), donc
     * donne un mi-poids à l'argument */
    return PoidsTransformation{transformation, 0.5};
}

#if 0
static bool transformation_impossible(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	transformation = TypeTransformation::IMPOSSIBLE;
	return false;
}

static bool transformation_inutile(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	transformation = TypeTransformation::INUTILE;
	return false;
}

static bool tansformation_type_vers_eini(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{

}

template <typename T, int N>
struct matrice {
	T donnees[N][N];

	constexpr void remplis(T d)
	{
		for (auto i = 0; i < N; ++i) {
			for (auto j = 0; j < N; ++j) {
				donnees[i][j] = d;
			}
		}
	}

	constexpr T *operator[](int64_t i)
	{
		return donnees[i];
	}
};

#    define comme_int(x) static_cast<int>(GenreType::x)

static constexpr auto table_transformation_type = [] {
	using type_fonction = bool(*)(EspaceDeTravail &, ContexteValidationCode &, Type *, Type *, TransformationType &);

	matrice<type_fonction, static_cast<int>(GenreType::TOTAL)> table{};
	table.remplis(transformation_impossible);

	/*
	ENUMERE_GENRE_TYPE_EX(ENTIER_NATUREL) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_RELATIF) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_CONSTANT) \
	ENUMERE_GENRE_TYPE_EX(REEL) \
	ENUMERE_GENRE_TYPE_EX(EINI) \
	ENUMERE_GENRE_TYPE_EX(ENUM) \
	ENUMERE_GENRE_TYPE_EX(ERREUR)
	 */

	/* transformations entre types similaires */

	table[comme_int(OCTET)][comme_int(OCTET)] = transformation_inutile;
	table[comme_int(RIEN)][comme_int(RIEN)] = transformation_inutile;
	table[comme_int(BOOL)][comme_int(BOOL)] = transformation_inutile;
	table[comme_int(CHAINE)][comme_int(CHAINE)] = transformation_inutile;
	table[comme_int(EINI)][comme_int(EINI)] = transformation_inutile;
	table[comme_int(TYPE_DE_DONNEES)][comme_int(TYPE_DE_DONNEES)] = transformation_inutile;
	table[comme_int(ENTIER_NATUREL)][comme_int(ENTIER_NATUREL)] = transformation_entre_naturel;
	table[comme_int(ENTIER_RELATIF)][comme_int(ENTIER_RELATIF)] = transformation_entre_relatif;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_CONSTANT)] = transformation_inutile;
	table[comme_int(REEL)][comme_int(REEL)] = transformation_entre_reel;
	table[comme_int(POINTEUR)][comme_int(POINTEUR)] = transformation_entre_pointeur;
	table[comme_int(REFERENCE)][comme_int(REFERENCE)] = transformation_inutile;
	table[comme_int(TABLEAU_FIXE)][comme_int(TABLEAU_FIXE)] = transformation_entre_tableau_fixe;
	table[comme_int(TABLEAU_DYNAMIQUE)][comme_int(TABLEAU_DYNAMIQUE)] = transformation_entre_tableau_dyn;

	/* transformations entre types différents */
	table[comme_int(POINTEUR)][comme_int(FONCTION)] = transformation_pointeur_vers_fonction;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_NATUREL)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_RELATIF)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(REEL)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(POINTEUR)] = transformation_vers_type_cible;
	table[comme_int(ENUM)][comme_int(ENTIER_NATUREL)] = transformation_enum_vers_entier;
	table[comme_int(ENUM)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;
	table[comme_int(ERREUR)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;
	table[comme_int(ERREUR)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;

	return table;
}();
#endif

std::ostream &operator<<(std::ostream &os, TransformationType type)
{
    return os << type.type;
}
