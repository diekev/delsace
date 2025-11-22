/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "infos_types.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "utilitaires/log.hh"

#include "cas_genre_noeud.hh"
#include "espace_de_travail.hh"
#include "noeud_code.hh"
#include "utilitaires.hh"

/* ------------------------------------------------------------------------- */
/** \name AllocatriceInfosType.
 * \{ */

int64_t AllocatriceInfosType::mémoire_utilisée() const
{
    auto memoire = int64_t(0);
#define ENUMERE_TYPES_INFO_TYPE_EX(type__, nom__) memoire += nom__.mémoire_utilisée();
    ENUMERE_TYPES_INFO_TYPE(ENUMERE_TYPES_INFO_TYPE_EX)
#undef ENUMERE_TYPES_INFO_TYPE_EX

#define ENUME_TYPES_TRANCHES_INFO_TYPE_EX(type__, nom__)                                          \
    memoire += tranches_##nom__.taille_mémoire();                                                 \
    POUR (tranches_##nom__) {                                                                     \
        memoire += it.taille() * taille_de(type__);                                               \
    }

    ENUME_TYPES_TRANCHES_INFO_TYPE(ENUME_TYPES_TRANCHES_INFO_TYPE_EX)

#undef ENUME_TYPES_TRANCHES_INFO_TYPE_EX

    return memoire;
}

template <typename T>
kuri::tranche<T> AllocatriceInfosType::donne_tranche(kuri::tablet<T, 6> const &tableau)
{
    auto pointeur = mémoire::loge_tableau<T>("tranche", tableau.taille());
    auto résultat = kuri::tranche(pointeur, tableau.taille());

    POUR (tableau) {
        *pointeur++ = it;
    }

    stocke_tranche(résultat);
    return résultat;
}

template <typename T>
kuri::tranche<T> AllocatriceInfosType::donne_tranche(kuri::tableau<T> const &tableau)
{
    auto pointeur = mémoire::loge_tableau<T>("tranche", tableau.taille());
    auto résultat = kuri::tranche(pointeur, tableau.taille());

    POUR (tableau) {
        *pointeur++ = it;
    }

    stocke_tranche(résultat);
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Implémentation des fonctions supplémentaires de la ConvertisseuseNoeudCode.
 * \{ */

static void copie_annotations(kuri::tableau<Annotation, int> const &source,
                              kuri::tablet<const Annotation *, 6> &dest)
{
    dest.réserve(source.taille());
    for (auto &annotation : source) {
        dest.ajoute(&annotation);
    }
}

static void remplis_rubrique_info_type(AllocatriceInfosType &allocatrice_infos_types,
                                       EspaceDeTravail *espace,
                                       InfoTypeRubriqueStructure *info_type_rubrique,
                                       RubriqueTypeComposé const &rubrique)
{
    info_type_rubrique->décalage = int32_t(rubrique.décalage);
    info_type_rubrique->nom = rubrique.nom->nom;
    info_type_rubrique->drapeaux = rubrique.drapeaux;

    auto annotations = kuri::tablet<const Annotation *, 6>();

    if (rubrique.decl) {
        if (rubrique.decl->est_base_déclaration_variable()) {
            copie_annotations(rubrique.decl->comme_base_déclaration_variable()->annotations,
                              annotations);
        }
        info_type_rubrique->annotations = allocatrice_infos_types.donne_tranche(annotations);

        const auto lexème = rubrique.decl->lexème;
        const auto fichier = espace->fichier(lexème->fichier);
        info_type_rubrique->chemin_fichier = fichier->chemin();
        info_type_rubrique->nom_fichier = fichier->nom();
        info_type_rubrique->numéro_ligne = lexème->ligne + 1;
        info_type_rubrique->numéro_colonne = lexème->colonne;
    }
}

static void initialise_entête_info_type(InfoType *info_type, Type *type, GenreInfoType genre)
{
    info_type->genre = genre;
    info_type->taille_en_octet = type->taille_octet;
    info_type->alignement = type->alignement;
}

InfoType *ConvertisseuseNoeudCode::crée_info_type_pour(EspaceDeTravail *espace,
                                                       Typeuse &typeuse,
                                                       Type *type)
{
    auto crée_info_type_entier = [this](Type *type_local, bool est_signe) {
        auto info_type = allocatrice_infos_types.infos_types_entiers.ajoute_élément();
        initialise_entête_info_type(info_type, type_local, GenreInfoType::ENTIER);
        info_type->est_signé = est_signe;

        return info_type;
    };

    // À FAIRE : il est possible que les types ne soient pas encore validé quand nous générons des
    // messages pour les entêtes de fonctions
    if (type == nullptr) {
        /* Nous pouvons être ici pour les rubriques nulles des classes polymorphiques. */
        return nullptr;
    }

    if (type->info_type != nullptr &&
        type->possède_drapeau(DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES)) {
        return type->info_type;
    }

    switch (type->genre) {
        case GenreNoeud::TUPLE:
        {
            return nullptr;
        }
        case GenreNoeud::POLYMORPHIQUE:
        {
            auto type_polymorphique = type->comme_type_polymorphique();
            auto info_type = allocatrice_infos_types.infos_types_polymorphiques.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::POLYMORPHIQUE);
            if (type_polymorphique->ident) {
                info_type->ident = type_polymorphique->ident->nom;
            }
            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::OCTET:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::OCTET);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::BOOL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::BOOLÉEN);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::CHAINE:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::CHAINE);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::EINI:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::EINI);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_type_tableau_dynamique();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::TABLEAU);
            info_type->type_élément = crée_info_type_pour(
                espace, typeuse, type_tableau->type_pointé);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_tableau = type->comme_type_tranche();

            auto info_type = allocatrice_infos_types.infos_types_tranches.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::TRANCHE);
            info_type->type_élément = crée_info_type_pour(
                espace, typeuse, type_tableau->type_élément);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = type->comme_type_variadique();

            auto info_type = allocatrice_infos_types.infos_types_variadiques.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::VARIADIQUE);

            // type nul pour les types variadiques des fonctions externes (p.e. printf(const char
            // *, ...))
            if (type_variadique->type_pointé) {
                info_type->type_élément = crée_info_type_pour(
                    espace, typeuse, type_variadique->type_pointé);
            }

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            auto info_type = allocatrice_infos_types.infos_types_tableaux_fixes.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::TABLEAU_FIXE);
            info_type->nombre_éléments = uint32_t(type_tableau->taille);
            info_type->type_élément = crée_info_type_pour(
                espace, typeuse, type_tableau->type_pointé);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            type->info_type = crée_info_type_entier(type, true);
            type->info_type->taille_en_octet = 4;
            type->info_type->alignement = 4;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        {
            type->info_type = crée_info_type_entier(type, false);
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
        {
            type->info_type = crée_info_type_entier(type, true);
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::RÉEL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::RÉEL);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::RIEN:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::RIEN);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::TYPE_DE_DONNÉES);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::RÉFÉRENCE:
        {
            auto info_type = allocatrice_infos_types.infos_types_pointeurs.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::POINTEUR);
            info_type->type_pointé = crée_info_type_pour(
                espace, typeuse, type_déréférencé_pour(type));
            info_type->est_référence = type->est_type_référence();

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            if (!type->info_type) {
                auto info_type = allocatrice_infos_types.infos_types_structures.ajoute_élément();
                type->info_type = info_type;
            }

            auto info_type = static_cast<InfoTypeStructure *>(type->info_type);
            initialise_entête_info_type(info_type, type, GenreInfoType::STRUCTURE);
            info_type->nom = donne_nom_hiérarchique(type_struct);
            info_type->est_polymorphique = type_struct->est_polymorphe;

            if (type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;

                if (type_struct->polymorphe_de_base) {
                    auto polymorphe = const_cast<NoeudDéclarationClasse *>(
                        type_struct->polymorphe_de_base);
                    info_type->polymorphe_de_base = static_cast<InfoTypeStructure *>(
                        crée_info_type_pour(espace, typeuse, polymorphe));
                }

                auto rubriques = kuri::tablet<InfoTypeRubriqueStructure *, 6>();
                rubriques.réserve(type_struct->rubriques.taille());

                POUR (type_struct->rubriques) {
                    if (it.nom == ID::chaine_vide) {
                        continue;
                    }

                    if (it.possède_drapeau(RubriqueTypeComposé::PROVIENT_D_UN_EMPOI)) {
                        continue;
                    }

                    auto info_type_rubrique =
                        allocatrice_infos_types.infos_types_rubriques_structures.ajoute_élément();
                    info_type_rubrique->info = crée_info_type_pour(espace, typeuse, it.type);
                    remplis_rubrique_info_type(
                        allocatrice_infos_types, espace, info_type_rubrique, it);
                    rubriques.ajoute(info_type_rubrique);
                }

                info_type->rubriques = allocatrice_infos_types.donne_tranche(rubriques);

                auto annotations = kuri::tablet<const Annotation *, 6>();
                copie_annotations(type_struct->annotations, annotations);
                info_type->annotations = allocatrice_infos_types.donne_tranche(annotations);

                auto structs_employées = kuri::tablet<InfoTypeStructure *, 6>();
                structs_employées.réserve(type_struct->types_employés.taille());
                POUR (type_struct->types_employés) {
                    auto info_struct_employe = crée_info_type_pour(espace, typeuse, it->type);
                    structs_employées.ajoute(
                        static_cast<InfoTypeStructure *>(info_struct_employe));
                }
                info_type->structs_employées = allocatrice_infos_types.donne_tranche(
                    structs_employées);
            }

            break;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();
            if (!type->info_type) {
                auto info_type = allocatrice_infos_types.infos_types_unions.ajoute_élément();
                type->info_type = info_type;
            }

            auto info_type = static_cast<InfoTypeUnion *>(type->info_type);
            initialise_entête_info_type(info_type, type_union, GenreInfoType::UNION);
            info_type->est_sûre = !type_union->est_nonsure;

            if (type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;

                info_type->type_le_plus_grand = crée_info_type_pour(
                    espace, typeuse, type_union->type_le_plus_grand);
                info_type->décalage_indice = type_union->décalage_indice;
                info_type->nom = donne_nom_hiérarchique(type_union);
                info_type->est_polymorphique = type_union->est_polymorphe;

                if (type_union->polymorphe_de_base) {
                    auto polymorphe = const_cast<NoeudDéclarationClasse *>(
                        type_union->polymorphe_de_base);
                    info_type->polymorphe_de_base = static_cast<InfoTypeUnion *>(
                        crée_info_type_pour(espace, typeuse, polymorphe));
                }

                auto rubriques = kuri::tablet<InfoTypeRubriqueStructure *, 6>();
                rubriques.réserve(type_union->rubriques.taille());

                POUR (type_union->rubriques) {
                    auto info_type_rubrique =
                        allocatrice_infos_types.infos_types_rubriques_structures.ajoute_élément();
                    info_type_rubrique->info = crée_info_type_pour(espace, typeuse, it.type);
                    remplis_rubrique_info_type(
                        allocatrice_infos_types, espace, info_type_rubrique, it);
                    rubriques.ajoute(info_type_rubrique);
                }

                info_type->rubriques = allocatrice_infos_types.donne_tranche(rubriques);

                auto annotations = kuri::tablet<const Annotation *, 6>();
                copie_annotations(type_union->annotations, annotations);
                info_type->annotations = allocatrice_infos_types.donne_tranche(annotations);

                type->info_type = info_type;
            }
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);
            if (!type->info_type) {
                auto info_type = allocatrice_infos_types.infos_types_énums.ajoute_élément();
                type->info_type = info_type;
            }

            auto info_type = static_cast<InfoTypeÉnum *>(type->info_type);
            initialise_entête_info_type(info_type, type_enum, GenreInfoType::ÉNUM);
            info_type->nom = donne_nom_hiérarchique(type_enum);
            info_type->est_drapeau = type_enum->est_type_enum_drapeau();

            if (type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;

                info_type->type_sous_jacent = static_cast<InfoTypeEntier *>(
                    crée_info_type_pour(espace, typeuse, type_enum->type_sous_jacent));

                auto noms = kuri::tablet<kuri::chaine_statique, 6>();
                noms.réserve(type_enum->rubriques.taille());

                POUR (type_enum->rubriques) {
                    if (it.est_implicite()) {
                        continue;
                    }

                    noms.ajoute(it.nom->nom);
                }

                auto valeurs = donne_tableau_valeurs_énum(typeuse, *type_enum);

                info_type->noms = allocatrice_infos_types.donne_tranche(noms);
                info_type->valeurs = allocatrice_infos_types.donne_tranche(valeurs);
            }
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonction = type->comme_type_fonction();

            auto info_type = allocatrice_infos_types.infos_types_fonctions.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::FONCTION);

            auto types_entrée = kuri::tablet<InfoType *, 6>();
            types_entrée.réserve(type_fonction->types_entrées.taille());

            POUR (type_fonction->types_entrées) {
                types_entrée.ajoute(crée_info_type_pour(espace, typeuse, it));
            }

            auto types_sortie = kuri::tablet<InfoType *, 6>();
            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_type_tuple()) {
                auto tuple = type_sortie->comme_type_tuple();
                types_sortie.réserve(tuple->rubriques.taille());

                POUR (tuple->rubriques) {
                    types_sortie.ajoute(crée_info_type_pour(espace, typeuse, it.type));
                }
            }
            else {
                types_sortie.réserve(1);
                types_sortie.ajoute(crée_info_type_pour(espace, typeuse, type_sortie));
            }

            info_type->types_entrée = allocatrice_infos_types.donne_tranche(types_entrée);
            info_type->types_sortie = allocatrice_infos_types.donne_tranche(types_sortie);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();

            auto info_type = allocatrice_infos_types.infos_types_opaques.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::OPAQUE);
            info_type->nom = donne_nom_hiérarchique(type_opaque);
            info_type->type_opacifié = crée_info_type_pour(
                espace, typeuse, type_opaque->type_opacifié);

            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_élément();
            initialise_entête_info_type(info_type, type, GenreInfoType::ADRESSE_FONCTION);
            type->info_type = info_type;
            type->drapeaux_type |= DrapeauxTypes::INFOS_TYPE_SONT_COMPLÈTES;
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    typeuse.définis_info_type_pour_type(type->info_type, type);
    return type->info_type;
}

Type *ConvertisseuseNoeudCode::convertis_info_type(Typeuse &typeuse, InfoType *type)
{
    switch (type->genre) {
        case GenreInfoType::EINI:
        {
            return typeuse.type_eini;
        }
        case GenreInfoType::RÉEL:
        {
            if (type->taille_en_octet == 2) {
                return typeuse.type_r16;
            }

            if (type->taille_en_octet == 4) {
                return typeuse.type_r32;
            }

            if (type->taille_en_octet == 8) {
                return typeuse.type_r64;
            }

            return nullptr;
        }
        case GenreInfoType::ENTIER:
        {
            const auto info_type_entier = static_cast<const InfoTypeEntier *>(type);

            if (info_type_entier->est_signé) {
                if (type->taille_en_octet == 1) {
                    return typeuse.type_z8;
                }

                if (type->taille_en_octet == 2) {
                    return typeuse.type_z16;
                }

                if (type->taille_en_octet == 4) {
                    return typeuse.type_z32;
                }

                if (type->taille_en_octet == 8) {
                    return typeuse.type_z64;
                }

                return nullptr;
            }

            if (type->taille_en_octet == 1) {
                return typeuse.type_n8;
            }

            if (type->taille_en_octet == 2) {
                return typeuse.type_n16;
            }

            if (type->taille_en_octet == 4) {
                return typeuse.type_n32;
            }

            if (type->taille_en_octet == 8) {
                return typeuse.type_n64;
            }

            return nullptr;
        }
        case GenreInfoType::OCTET:
        {
            return typeuse.type_octet;
        }
        case GenreInfoType::BOOLÉEN:
        {
            return typeuse.type_bool;
        }
        case GenreInfoType::CHAINE:
        {
            return typeuse.type_chaine;
        }
        case GenreInfoType::RIEN:
        {
            return typeuse.type_rien;
        }
        case GenreInfoType::ADRESSE_FONCTION:
        {
            return typeuse.type_adresse_fonction;
        }
        case GenreInfoType::POINTEUR:
        {
            const auto info_type_pointeur = static_cast<const InfoTypePointeur *>(type);

            auto type_pointé = convertis_info_type(typeuse, info_type_pointeur->type_pointé);

            if (info_type_pointeur->est_référence) {
                return typeuse.type_référence_pour(type_pointé);
            }

            return typeuse.type_pointeur_pour(type_pointé);
        }
        case GenreInfoType::TABLEAU:
        {
            const auto info_type_tableau = static_cast<const InfoTypeTableau *>(type);
            auto type_élément = convertis_info_type(typeuse, info_type_tableau->type_élément);
            return typeuse.type_tableau_dynamique(type_élément);
        }
        case GenreInfoType::TABLEAU_FIXE:
        {
            const auto info_type_tableau = static_cast<const InfoTypeTableauFixe *>(type);
            auto type_élément = convertis_info_type(typeuse, info_type_tableau->type_élément);
            return typeuse.type_tableau_fixe(type_élément,
                                             int32_t(info_type_tableau->nombre_éléments));
        }
        case GenreInfoType::TRANCHE:
        {
            const auto info_type_tranche = static_cast<const InfoTypeTranche *>(type);
            auto type_élément = convertis_info_type(typeuse, info_type_tranche->type_élément);
            return typeuse.crée_type_tranche(type_élément);
        }
        case GenreInfoType::TYPE_DE_DONNÉES:
        {
            // À FAIRE : préserve l'information de type connu
            return typeuse.type_type_de_données(nullptr);
        }
        case GenreInfoType::FONCTION:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::STRUCTURE:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::ÉNUM:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::UNION:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::OPAQUE:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::POLYMORPHIQUE:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::VARIADIQUE:
        {
            const auto info_type_variadique = static_cast<const InfoTypeVariadique *>(type);
            auto type_élément = convertis_info_type(typeuse, info_type_variadique->type_élément);
            return typeuse.type_variadique(type_élément);
        }
    }

    return nullptr;
}

/** \} */
