/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "infos_types.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "utilitaires/log.hh"

#include "cas_genre_noeud.hh"
#include "noeud_code.hh"
#include "utilitaires.hh"

/* ------------------------------------------------------------------------- */
/** \name AllocatriceInfosType.
 * \{ */

int64_t AllocatriceInfosType::memoire_utilisee() const
{
    auto memoire = int64_t(0);
#define ENUMERE_TYPES_INFO_TYPE_EX(type__, nom__) memoire += nom__.memoire_utilisee();
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
    auto pointeur = memoire::loge_tableau<T>("tranche", tableau.taille());
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

static void remplis_membre_info_type(AllocatriceInfosType &allocatrice_infos_types,
                                     InfoTypeMembreStructure *info_type_membre,
                                     MembreTypeComposé const &membre)
{
    info_type_membre->décalage = membre.decalage;
    info_type_membre->nom = membre.nom->nom;
    info_type_membre->drapeaux = membre.drapeaux;

    auto annotations = kuri::tablet<const Annotation *, 6>();

    if (membre.decl) {
        if (membre.decl->est_base_declaration_variable()) {
            copie_annotations(membre.decl->comme_base_declaration_variable()->annotations,
                              annotations);
        }
        info_type_membre->annotations = allocatrice_infos_types.donne_tranche(annotations);
    }
}

InfoType *ConvertisseuseNoeudCode::crée_info_type_pour(Typeuse &typeuse, Type *type)
{
    auto crée_info_type_entier = [this](uint32_t taille_en_octet, bool est_signe) {
        auto info_type = allocatrice_infos_types.infos_types_entiers.ajoute_element();
        info_type->genre = GenreInfoType::ENTIER;
        info_type->taille_en_octet = taille_en_octet;
        info_type->est_signé = est_signe;

        return info_type;
    };

    // À FAIRE : il est possible que les types ne soient pas encore validé quand nous générons des
    // messages pour les entêtes de fonctions
    if (type == nullptr) {
        return nullptr;
    }

    if (type->info_type != nullptr) {
        return type->info_type;
    }

    switch (type->genre) {
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        {
            return nullptr;
        }
        case GenreNoeud::OCTET:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::OCTET;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::BOOL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::BOOLÉEN;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::CHAINE:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::CHAINE;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::EINI:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::EINI;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_type_tableau_dynamique();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->type_élément = crée_info_type_pour(typeuse, type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_tableau = type->comme_type_tranche();

            auto info_type = allocatrice_infos_types.infos_types_tranches.ajoute_element();
            info_type->genre = GenreInfoType::TRANCHE;
            info_type->taille_en_octet = type->taille_octet;
            info_type->type_élément = crée_info_type_pour(typeuse, type_tableau->type_élément);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = type->comme_type_variadique();

            auto info_type = allocatrice_infos_types.infos_types_variadiques.ajoute_element();
            info_type->genre = GenreInfoType::VARIADIQUE;
            info_type->taille_en_octet = type->taille_octet;

            // type nul pour les types variadiques des fonctions externes (p.e. printf(const char
            // *, ...))
            if (type_variadique->type_pointe) {
                info_type->type_élément = crée_info_type_pour(typeuse,
                                                              type_variadique->type_pointe);
            }

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            auto info_type = allocatrice_infos_types.infos_types_tableaux_fixes.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU_FIXE;
            info_type->taille_en_octet = type->taille_octet;
            info_type->nombre_éléments = uint32_t(type_tableau->taille);
            info_type->type_élément = crée_info_type_pour(typeuse, type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            type->info_type = crée_info_type_entier(4, true);
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        {
            type->info_type = crée_info_type_entier(type->taille_octet, false);
            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
        {
            type->info_type = crée_info_type_entier(type->taille_octet, true);
            break;
        }
        case GenreNoeud::REEL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::RÉEL;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::RIEN:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::RIEN;
            info_type->taille_en_octet = 0;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TYPE_DE_DONNEES:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::TYPE_DE_DONNÉES;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::REFERENCE:
        {
            auto info_type = allocatrice_infos_types.infos_types_pointeurs.ajoute_element();
            info_type->genre = GenreInfoType::POINTEUR;
            info_type->type_pointé = crée_info_type_pour(typeuse, type_déréférencé_pour(type));
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_référence = type->est_type_reference();

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            auto info_type = allocatrice_infos_types.infos_types_structures.ajoute_element();
            type->info_type = info_type;

            info_type->genre = GenreInfoType::STRUCTURE;
            info_type->taille_en_octet = type->taille_octet;
            info_type->nom = donne_nom_hiérarchique(type_struct);

            auto membres = kuri::tablet<InfoTypeMembreStructure *, 6>();
            membres.réserve(type_struct->membres.taille());

            POUR (type_struct->membres) {
                if (it.nom == ID::chaine_vide) {
                    continue;
                }

                if (it.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                    continue;
                }

                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = crée_info_type_pour(typeuse, it.type);
                remplis_membre_info_type(allocatrice_infos_types, info_type_membre, it);
                membres.ajoute(info_type_membre);
            }

            info_type->membres = allocatrice_infos_types.donne_tranche(membres);

            auto annotations = kuri::tablet<const Annotation *, 6>();
            copie_annotations(type_struct->annotations, annotations);
            info_type->annotations = allocatrice_infos_types.donne_tranche(annotations);

            auto structs_employées = kuri::tablet<InfoTypeStructure *, 6>();
            structs_employées.réserve(type_struct->types_employés.taille());
            POUR (type_struct->types_employés) {
                auto info_struct_employe = crée_info_type_pour(typeuse, it->type);
                structs_employées.ajoute(static_cast<InfoTypeStructure *>(info_struct_employe));
            }
            info_type->structs_employées = allocatrice_infos_types.donne_tranche(
                structs_employées);

            break;
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            auto info_type = allocatrice_infos_types.infos_types_unions.ajoute_element();
            info_type->genre = GenreInfoType::UNION;
            info_type->est_sûre = !type_union->est_nonsure;
            info_type->type_le_plus_grand = crée_info_type_pour(typeuse,
                                                                type_union->type_le_plus_grand);
            info_type->décalage_index = type_union->decalage_index;
            info_type->taille_en_octet = type_union->taille_octet;
            info_type->nom = donne_nom_hiérarchique(type_union);

            auto membres = kuri::tablet<InfoTypeMembreStructure *, 6>();
            membres.réserve(type_union->membres.taille());

            POUR (type_union->membres) {
                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = crée_info_type_pour(typeuse, it.type);
                remplis_membre_info_type(allocatrice_infos_types, info_type_membre, it);
                membres.ajoute(info_type_membre);
            }

            info_type->membres = allocatrice_infos_types.donne_tranche(membres);

            auto annotations = kuri::tablet<const Annotation *, 6>();
            copie_annotations(type_union->annotations, annotations);
            info_type->annotations = allocatrice_infos_types.donne_tranche(annotations);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);

            auto info_type = allocatrice_infos_types.infos_types_énums.ajoute_element();
            info_type->genre = GenreInfoType::ÉNUM;
            info_type->nom = donne_nom_hiérarchique(type_enum);
            info_type->est_drapeau = type_enum->est_type_enum_drapeau();
            info_type->taille_en_octet = type_enum->taille_octet;
            info_type->type_sous_jacent = static_cast<InfoTypeEntier *>(
                crée_info_type_pour(typeuse, type_enum->type_sous_jacent));

            auto noms = kuri::tablet<kuri::chaine_statique, 6>();
            noms.réserve(type_enum->membres.taille());
            auto valeurs = kuri::tablet<int, 6>();
            valeurs.réserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
                    continue;
                }

                noms.ajoute(it.nom->nom);
                valeurs.ajoute(it.valeur);
            }

            info_type->noms = allocatrice_infos_types.donne_tranche(noms);
            info_type->valeurs = allocatrice_infos_types.donne_tranche(valeurs);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonction = type->comme_type_fonction();

            auto info_type = allocatrice_infos_types.infos_types_fonctions.ajoute_element();
            info_type->genre = GenreInfoType::FONCTION;
            info_type->est_coroutine = false;
            info_type->taille_en_octet = type->taille_octet;

            auto types_entrée = kuri::tablet<InfoType *, 6>();
            types_entrée.réserve(type_fonction->types_entrees.taille());

            POUR (type_fonction->types_entrees) {
                types_entrée.ajoute(crée_info_type_pour(typeuse, it));
            }

            auto types_sortie = kuri::tablet<InfoType *, 6>();
            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_type_tuple()) {
                auto tuple = type_sortie->comme_type_tuple();
                types_sortie.réserve(tuple->membres.taille());

                POUR (tuple->membres) {
                    types_sortie.ajoute(crée_info_type_pour(typeuse, it.type));
                }
            }
            else {
                types_sortie.réserve(1);
                types_sortie.ajoute(crée_info_type_pour(typeuse, type_sortie));
            }

            info_type->types_entrée = allocatrice_infos_types.donne_tranche(types_entrée);
            info_type->types_sortie = allocatrice_infos_types.donne_tranche(types_sortie);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();

            auto info_type = allocatrice_infos_types.infos_types_opaques.ajoute_element();
            info_type->genre = GenreInfoType::OPAQUE;
            info_type->nom = donne_nom_hiérarchique(type_opaque);
            info_type->type_opacifié = crée_info_type_pour(typeuse, type_opaque->type_opacifie);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::ADRESSE_FONCTION;
            info_type->taille_en_octet = type->taille_octet;
            return info_type;
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
            return TypeBase::EINI;
        }
        case GenreInfoType::RÉEL:
        {
            if (type->taille_en_octet == 2) {
                return TypeBase::R16;
            }

            if (type->taille_en_octet == 4) {
                return TypeBase::R32;
            }

            if (type->taille_en_octet == 8) {
                return TypeBase::R64;
            }

            return nullptr;
        }
        case GenreInfoType::ENTIER:
        {
            const auto info_type_entier = static_cast<const InfoTypeEntier *>(type);

            if (info_type_entier->est_signé) {
                if (type->taille_en_octet == 1) {
                    return TypeBase::Z8;
                }

                if (type->taille_en_octet == 2) {
                    return TypeBase::Z16;
                }

                if (type->taille_en_octet == 4) {
                    return TypeBase::Z32;
                }

                if (type->taille_en_octet == 8) {
                    return TypeBase::Z64;
                }

                return nullptr;
            }

            if (type->taille_en_octet == 1) {
                return TypeBase::N8;
            }

            if (type->taille_en_octet == 2) {
                return TypeBase::N16;
            }

            if (type->taille_en_octet == 4) {
                return TypeBase::N32;
            }

            if (type->taille_en_octet == 8) {
                return TypeBase::N64;
            }

            return nullptr;
        }
        case GenreInfoType::OCTET:
        {
            return TypeBase::OCTET;
        }
        case GenreInfoType::BOOLÉEN:
        {
            return TypeBase::BOOL;
        }
        case GenreInfoType::CHAINE:
        {
            return TypeBase::CHAINE;
        }
        case GenreInfoType::RIEN:
        {
            return TypeBase::RIEN;
        }
        case GenreInfoType::ADRESSE_FONCTION:
        {
            return TypeBase::ADRESSE_FONCTION;
        }
        case GenreInfoType::POINTEUR:
        {
            const auto info_type_pointeur = static_cast<const InfoTypePointeur *>(type);

            auto type_pointé = convertis_info_type(typeuse, info_type_pointeur->type_pointé);

            if (info_type_pointeur->est_référence) {
                return typeuse.type_reference_pour(type_pointé);
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
            return typeuse.type_type_de_donnees(nullptr);
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
