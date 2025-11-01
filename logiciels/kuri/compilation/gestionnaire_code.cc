/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "gestionnaire_code.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "programme.hh"

#include "utilitaires/divers.hh"
#include "utilitaires/log.hh"
#include "utilitaires/macros.hh"

#include "plateforme/windows.h"

/*
  À FAIRE(gestion) : pour chaque type :
- avoir un lexème sentinel pour l'impression des erreurs si le noeud est crée lors de la
compilation
 */

#undef TEMPORISE_UNITES_POUR_SIMULER_MOULTFILAGE

#undef STATS_DÉTAILLÉES_GESTION

#ifdef STATISTIQUES_DETAILLEES
#    define STATS_DÉTAILLÉES_GESTION
#endif

#ifdef STATS_DÉTAILLÉES_GESTION
#    define DÉBUTE_STAT(stat) auto chrono_##stat = kuri::chrono::compte_milliseconde()
#    define TERMINE_STAT(stat)                                                                    \
        stats.stats.fusionne_entrée(GESTION__##stat, {"", chrono_##stat.temps()})
#else
#    define DÉBUTE_STAT(stat)
#    define TERMINE_STAT(stat)
#endif

#define TACHE_AJOUTEE(genre) espace->tache_ajoutee(GenreTâche::genre, m_compilatrice->messagère)
#define TACHE_TERMINEE(genre) espace->tache_terminee(GenreTâche::genre, m_compilatrice->messagère)

/* ------------------------------------------------------------------------- */
/** \name État chargement fichiers
 * \{ */

void ÉtatChargementFichiers::ajoute_unité_pour_charge_ou_importe(UniteCompilation *unité)
{
    if (file_unités_charge_ou_importe == nullptr) {
        file_unités_charge_ou_importe = unité;
        return;
    }

    unité->suivante = file_unités_charge_ou_importe;
    file_unités_charge_ou_importe->précédente = unité;
    file_unités_charge_ou_importe = unité;
}

void ÉtatChargementFichiers::supprime_unité_pour_charge_ou_importe(UniteCompilation *unité)
{
    if (unité->précédente) {
        unité->précédente->suivante = unité->suivante;
    }

    if (unité->suivante) {
        unité->suivante->précédente = unité->précédente;
    }

    if (unité == file_unités_charge_ou_importe) {
        file_unités_charge_ou_importe = unité->suivante;
    }

    unité->précédente = nullptr;
    unité->suivante = nullptr;
}

void ÉtatChargementFichiers::enfile(UniteCompilation *unité)
{
    défile(unité);
    unité->enfilée_dans = &nombre_d_unités_pour_raison[int(unité->donne_raison_d_être())];
    unité->enfilée_dans->compte += 1;
    assert(unité->enfilée_dans->compte >= 1);
}

void ÉtatChargementFichiers::défile(UniteCompilation *unité)
{
    if (unité->enfilée_dans) {
        unité->enfilée_dans->compte -= 1;
        assert(unité->enfilée_dans->compte >= 0);
    }

    unité->enfilée_dans = nullptr;
}

void ÉtatChargementFichiers::ajoute_unité_pour_chargement_fichier(UniteCompilation *unité)
{
    assert(unité->enfilée_dans == nullptr);
    enfile(unité);
}

void ÉtatChargementFichiers::déplace_unité_pour_chargement_fichier(UniteCompilation *unité)
{
    assert(unité->enfilée_dans != nullptr);
    enfile(unité);
}

void ÉtatChargementFichiers::supprime_unité_pour_chargement_fichier(UniteCompilation *unité)
{
    défile(unité);
}

bool ÉtatChargementFichiers::tous_les_fichiers_à_parser_le_sont() const
{
    const int index[3] = {int(RaisonDÊtre::CHARGEMENT_FICHIER),
                          int(RaisonDÊtre::LEXAGE_FICHIER),
                          int(RaisonDÊtre::PARSAGE_FICHIER)};

    /* Vérifie s'il n'y a pas d'instructions de chargement ou d'import à gérer et que les unités de
     * chargement/lexage/parsage ont toutes été traitées. */
    return file_unités_charge_ou_importe == nullptr &&
           std::all_of(std::begin(index), std::end(index), [&](int it) {
               return nombre_d_unités_pour_raison[it].compte == 0;
           });
}

void ÉtatChargementFichiers::imprime_état() const
{
    dbg() << "--------------------------------------------\n"
          << nombre_d_unités_pour_raison[int(RaisonDÊtre::CHARGEMENT_FICHIER)].compte
          << " fichier(s) à charger\n"
          << nombre_d_unités_pour_raison[int(RaisonDÊtre::LEXAGE_FICHIER)].compte
          << " fichier(s) à lexer\n"
          << nombre_d_unités_pour_raison[int(RaisonDÊtre::PARSAGE_FICHIER)].compte
          << " fichier(s) à parser\n"
          << "File d'attente chargement est vide : " << (file_unités_charge_ou_importe == nullptr);
}

/** \} */

GestionnaireCode::GestionnaireCode(Compilatrice *compilatrice)
    : m_compilatrice(compilatrice),
      m_assembleuse(mémoire::loge<AssembleuseArbre>("AssembleuseArbre", allocatrice_noeud))
{
#ifdef TEMPORISE_UNITES_POUR_SIMULER_MOULTFILAGE
    mt = std::mt19937(1337);
#endif
}

GestionnaireCode::~GestionnaireCode()
{
    mémoire::deloge("AssembleuseArbre", m_assembleuse);
}

void GestionnaireCode::espace_créé(EspaceDeTravail *espace)
{
    assert(espace->programme);
    ajoute_programme(espace->programme);
}

void GestionnaireCode::métaprogramme_créé(MetaProgramme *metaprogramme)
{
    assert(metaprogramme->programme);
    ajoute_programme(metaprogramme->programme);
}

void GestionnaireCode::ajoute_programme(Programme *programme)
{
    programmes_en_cours.enfile(programme);
}

void GestionnaireCode::enleve_programme(Programme *programme)
{
    programmes_en_cours.efface_si(
        [&](Programme *programme_liste) { return programme == programme_liste; });
}

static bool est_declaration_variable_globale(NoeudExpression const *noeud)
{
    if (!noeud->est_déclaration_variable()) {
        return false;
    }

    return noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE);
}

static bool ajoute_dépendances_au_programme(GrapheDépendance &graphe,
                                            DonnéesRésolutionDépendances &données_dépendances,
                                            EspaceDeTravail *espace,
                                            Programme &programme,
                                            NoeudExpression *noeud)
{
    auto possède_erreur = false;
    auto &dépendances = données_dépendances.dépendances;

    /* Ajoute les fonctions. */
    kuri::pour_chaque_élément(dépendances.fonctions_utilisées, [&](auto &fonction) {
        if (fonction->possède_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE) &&
            !programme.pour_métaprogramme()) {
            possède_erreur = true;

            /* À FAIRE : site pour la dépendance. */
            espace->rapporte_erreur(fonction,
                                    "Utilisation d'une fonction d'interface de la compilatrice "
                                    "dans un programme final. Cette fonction ne peut qu'être "
                                    "utilisée dans un métaprogramme.");
            return kuri::DécisionItération::Arrête;
        }

        programme.ajoute_fonction(const_cast<NoeudDéclarationEntêteFonction *>(fonction));
        return kuri::DécisionItération::Continue;
    });

    if (possède_erreur) {
        return false;
    }

    /* Ajoute les globales. */
    kuri::pour_chaque_élément(dépendances.globales_utilisées, [&](auto &globale) {
        programme.ajoute_globale(const_cast<NoeudDéclarationVariable *>(globale));
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les types. */
    kuri::pour_chaque_élément(dépendances.types_utilisés, [&](auto &type) {
        programme.ajoute_type(type, RaisonAjoutType::DÉPENDANCE_DIRECTE, noeud);
        return kuri::DécisionItération::Continue;
    });
    kuri::pour_chaque_élément(dépendances.init_de_utilisés, [&](auto &type) {
        programme.ajoute_init_de(type);
        return kuri::DécisionItération::Continue;
    });
    kuri::pour_chaque_élément(dépendances.info_de_utilisés, [&](auto &type) {
        programme.ajoute_info_de(type);
        return kuri::DécisionItération::Continue;
    });

    while (true) {
        auto dépendances_manquantes = programme.dépendances_manquantes();
        if (dépendances_manquantes.taille() == 0) {
            break;
        }

        programme.dépendances_manquantes().efface();

        dépendances_manquantes.pour_chaque_element([&](NoeudDéclaration *decl) {
            auto noeud_dep = graphe.garantie_noeud_dépendance(espace, decl);

            for (auto const &relation : noeud_dep->relations().plage()) {
                auto accepte = relation.type == TypeRelation::UTILISE_TYPE;
                accepte |= relation.type == TypeRelation::UTILISE_FONCTION;
                accepte |= relation.type == TypeRelation::UTILISE_GLOBALE;

                if (!accepte) {
                    continue;
                }

                if (relation.noeud_fin->est_fonction()) {
                    programme.ajoute_fonction(relation.noeud_fin->fonction());
                    données_dépendances.dépendances_épendues.fonctions_utilisées.insère(
                        relation.noeud_fin->fonction());
                }
                else if (relation.noeud_fin->est_globale()) {
                    programme.ajoute_globale(relation.noeud_fin->globale());
                    données_dépendances.dépendances_épendues.globales_utilisées.insère(
                        relation.noeud_fin->globale());
                }
                else if (relation.noeud_fin->est_type()) {
                    auto type = relation.noeud_fin->type();
                    programme.ajoute_type(type, RaisonAjoutType::DÉPENDACE_INDIRECTE, decl);
                    données_dépendances.dépendances_épendues.types_utilisés.insère(type);
                    if (programme.possède_init_types(type)) {
                        if (type->fonction_init) {
                            programme.ajoute_fonction(type->fonction_init);
                        }
                    }
                }
            }
        });
    }

    return true;
}

struct RassembleuseDependances {
    DonnéesDépendance &dépendances;
    Compilatrice *compilatrice;
    EspaceDeTravail *espace;
    NoeudExpression *racine_;

    void ajoute_type(Type *type)
    {
        /* Pour les monomorphisations, la visite du noeud syntaxique peut nous amener à visiter les
         * expressions des types. Or, ces expressions peuvent être celle d'un type à monomorpher
         * (puisque la monomorphisation est la copie de la version polymorphique), par exemple
         * `File($T)`. Même si le type de la variable est correctement résolu comme étant celui
         * d'un monomorphe, l'expression du type est toujours celle d'un type polymorphique. Ces
         * types-là ne devraient être dans aucun programme final, donc nous devons les éviter. */
        if (est_type_polymorphique(type)) {
            return;
        }

        dépendances.types_utilisés.insère(type);
    }

    void ajoute_fonction(NoeudDéclarationEntêteFonction *fonction)
    {
        dépendances.fonctions_utilisées.insère(fonction);
    }

    void ajoute_globale(NoeudDéclarationVariable *globale)
    {
        dépendances.globales_utilisées.insère(globale);
    }

    void ajoute_init_de(Type *type)
    {
        dépendances.init_de_utilisés.insère(type);
    }

    void ajoute_info_de(Type *type)
    {
        dépendances.info_de_utilisés.insère(type);
    }

    void rassemble_dépendances()
    {
        rassemble_dépendances(racine_);
    }

    void rassemble_dépendances(NoeudExpression *racine);
};

/* Traverse l'arbre syntaxique de la racine spécifiée et rassemble les fonctions, types, et
 * globales utilisées. */
void RassembleuseDependances::rassemble_dépendances(NoeudExpression *racine)
{
    auto rassemble_dépendances_transformation = [&](TransformationType transformation,
                                                    Type *type) {
        /* Marque les dépendances sur les fonctions d'interface de kuri. */
        auto interface = espace->interface_kuri;

        if (transformation.type == TypeTransformation::EXTRAIT_UNION) {
            assert(interface->decl_panique_rubrique_union);
            auto type_union = type->comme_type_union();
            if (!type_union->est_nonsure) {
                ajoute_fonction(interface->decl_panique_rubrique_union);
            }
        }
        else if (transformation.type == TypeTransformation::R16_VERS_R32) {
            assert(interface->decl_dls_vers_r32);
            ajoute_fonction(interface->decl_dls_vers_r32);
        }
        else if (transformation.type == TypeTransformation::R16_VERS_R64) {
            assert(interface->decl_dls_vers_r64);
            ajoute_fonction(interface->decl_dls_vers_r64);
        }
        else if (transformation.type == TypeTransformation::R32_VERS_R16) {
            assert(interface->decl_dls_depuis_r32);
            ajoute_fonction(interface->decl_dls_depuis_r32);
        }
        else if (transformation.type == TypeTransformation::R64_VERS_R16) {
            assert(interface->decl_dls_depuis_r64);
            ajoute_fonction(interface->decl_dls_depuis_r64);
        }
        else if (transformation.type == TypeTransformation::CONSTRUIT_EINI) {
            ajoute_info_de(type);
        }
        else if (transformation.type == TypeTransformation::EXTRAIT_EINI) {
            ajoute_info_de(type);
            assert(interface->decl_vérifie_typage_extraction_eini);
            ajoute_fonction(interface->decl_vérifie_typage_extraction_eini);
        }

        /* Nous avons besoin d'un type pointeur pour le type cible pour la génération de
         * RI. À FAIRE: généralise pour toutes les variables. */
        if (transformation.type_cible) {
            auto type_pointeur = espace->typeuse.type_pointeur_pour(
                const_cast<Type *>(transformation.type_cible), false);
            ajoute_type(type_pointeur);
            ajoute_type(const_cast<Type *>(transformation.type_cible));
        }
    };

    auto rassemble_dépendances_transformations =
        [&](kuri::tableau_compresse<TransformationType, int> const &transformations, Type *type) {
            POUR (transformations.plage()) {
                rassemble_dépendances_transformation(it, type);
            }
        };

    visite_noeud(
        racine,
        PreferenceVisiteNoeud::SUBSTITUTION,
        true,
        [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
            /* N'ajoutons pas de dépendances sur les déclarations de types nichées. */
            if ((noeud->est_type_structure() || noeud->est_type_énum()) && noeud != racine) {
                return DecisionVisiteNoeud::IGNORE_ENFANTS;
            }

            /* Ne faisons pas dépendre les types d'eux-mêmes. */
            /* Note: les fonctions polymorphiques n'ont pas de types. */
            if (!(noeud->est_type_structure() || noeud->est_type_énum()) && noeud->type) {
                if (noeud->type->est_type_type_de_données()) {
                    auto type_de_donnees = noeud->type->comme_type_type_de_données();
                    if (type_de_donnees->type_connu) {
                        ajoute_type(type_de_donnees->type_connu);
                    }
                }
                else {
                    ajoute_type(noeud->type);
                }
            }

            if (noeud->est_corps_fonction() && racine != noeud) {
                /* Ignore le corps qui ne fut pas encore typé. */
                return DecisionVisiteNoeud::IGNORE_ENFANTS;
            }

            if (noeud->est_référence_déclaration()) {
                auto ref = noeud->comme_référence_déclaration();

                auto decl = ref->déclaration_référée;

                if (!decl) {
                    return DecisionVisiteNoeud::CONTINUE;
                }

                if (est_declaration_variable_globale(decl)) {
                    ajoute_globale(decl->comme_déclaration_variable());
                }
                else if (decl->est_entête_fonction() &&
                         !decl->comme_entête_fonction()->possède_drapeau(
                             DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
                    auto decl_fonc = decl->comme_entête_fonction();
                    ajoute_fonction(decl_fonc);
                }
            }
            else if (noeud->est_cuisine()) {
                auto cuisine = noeud->comme_cuisine();
                auto expr = cuisine->expression;
                ajoute_fonction(expr->comme_appel()->expression->comme_entête_fonction());
            }
            else if (noeud->est_indexage()) {
                /* Traite les indexages avant les expressions binaires afin de ne pas les traiter
                 * comme tel (est_expression_binaire() retourne vrai pour les indexages). */

                auto indexage = noeud->comme_indexage();
                /* op peut être nul pour les déclaration de type ([..]z32) */
                if (indexage->op && !indexage->op->est_basique) {
                    ajoute_fonction(indexage->op->decl);
                }

                /* Marque les dépendances sur les fonctions d'interface de kuri. */
                auto interface = espace->interface_kuri;

                /* Nous ne devrions pas avoir de référence ici, la validation sémantique s'est
                 * chargée de transtyper automatiquement. */
                auto type_indexe = donne_type_primitif(indexage->opérande_gauche->type);

                switch (type_indexe->genre) {
                    case GenreNoeud::VARIADIQUE:
                    case GenreNoeud::TABLEAU_DYNAMIQUE:
                    {
                        assert(interface->decl_panique_tableau);
                        ajoute_fonction(interface->decl_panique_tableau);
                        break;
                    }
                    case GenreNoeud::TABLEAU_FIXE:
                    {
                        assert(interface->decl_panique_tableau);
                        if (indexage->aide_génération_code != IGNORE_VERIFICATION) {
                            ajoute_fonction(interface->decl_panique_tableau);
                        }
                        break;
                    }
                    case GenreNoeud::CHAINE:
                    {
                        assert(interface->decl_panique_chaine);
                        if (indexage->aide_génération_code != IGNORE_VERIFICATION) {
                            ajoute_fonction(interface->decl_panique_chaine);
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else if (noeud->est_expression_binaire()) {
                auto expression_binaire = noeud->comme_expression_binaire();

                /* op peut être nul pour les déclaration de type (z32 | r32) */
                if (!expression_binaire->op) {
                    return DecisionVisiteNoeud::CONTINUE;
                }

                if (!expression_binaire->op->est_basique) {
                    ajoute_fonction(expression_binaire->op->decl);
                }
            }
            else if (noeud->est_discr()) {
                auto discr = noeud->comme_discr();
                if (discr->op && !discr->op->est_basique) {
                    ajoute_fonction(discr->op->decl);
                }
            }
            else if (noeud->est_construction_tableau()) {
                auto construction_tableau = noeud->comme_construction_tableau();
                ajoute_type(construction_tableau->type);

                /* Ajout également du type de pointeur pour la génération de code C. */
                auto type_feuille =
                    construction_tableau->type->comme_type_tableau_fixe()->type_pointé;
                auto type_ptr = espace->typeuse.type_pointeur_pour(type_feuille);
                ajoute_type(type_ptr);
            }
            else if (noeud->est_tente()) {
                auto tente = noeud->comme_tente();

                if (!tente->expression_piégée) {
                    auto interface = espace->interface_kuri;
                    assert(interface->decl_panique_erreur);
                    ajoute_fonction(interface->decl_panique_erreur);
                }
            }
            else if (noeud->est_référence_rubrique_union()) {
                auto interface = espace->interface_kuri;
                assert(interface->decl_panique_rubrique_union);
                ajoute_fonction(interface->decl_panique_rubrique_union);
            }
            else if (noeud->est_comme()) {
                auto comme = noeud->comme_comme();
                rassemble_dépendances_transformation(comme->transformation,
                                                     comme->expression->type);
            }
            else if (noeud->est_appel()) {
                auto appel = noeud->comme_appel();
                auto appelee = appel->noeud_fonction_appelée;

                if (appelee) {
                    if (appelee->est_entête_fonction()) {
                        ajoute_fonction(appelee->comme_entête_fonction());
                    }
                    else if (appelee->est_type_structure()) {
                        ajoute_type(appelee->comme_type_structure());
                    }
                }
            }
            else if (noeud->est_args_variadiques()) {
                auto args = noeud->comme_args_variadiques();

                /* Création d'un type tableau fixe, pour la génération de code. */
                auto taille_tableau = args->expressions.taille();
                if (taille_tableau != 0) {
                    auto type_tfixe = espace->typeuse.type_tableau_fixe(
                        args->type, taille_tableau, false);
                    ajoute_type(type_tfixe);
                }
            }
            else if (noeud->est_assignation_variable()) {
                auto assignation = noeud->comme_assignation_variable();
                rassemble_dépendances(assignation->expression);
            }
            else if (noeud->est_assignation_multiple()) {
                auto assignation = noeud->comme_assignation_multiple();

                POUR (assignation->données_exprs.plage()) {
                    rassemble_dépendances(it.expression);
                    auto type_expression = it.expression ? it.expression->type : Type::nul();
                    rassemble_dépendances_transformations(it.transformations, type_expression);
                }
            }
            else if (noeud->est_déclaration_variable()) {
                auto declaration = noeud->comme_déclaration_variable();
                assert_rappel(declaration->type,
                              [&]() { dbg() << "Type nul pour " << declaration->ident->nom; });
                ajoute_type(declaration->type);
                rassemble_dépendances(declaration->expression);

                if (!declaration->expression &&
                    !declaration->possède_drapeau(DrapeauxNoeud::EST_PARAMETRE)) {
                    ajoute_init_de(declaration->type);
                }
            }
            else if (noeud->est_déclaration_variable_multiple()) {
                auto declaration = noeud->comme_déclaration_variable_multiple();

                POUR (declaration->données_decl.plage()) {
                    for (auto &var : it.variables.plage()) {
                        ajoute_type(var->type);
                    }
                    rassemble_dépendances(it.expression);
                    auto type_expression = it.expression ? it.expression->type : Type::nul();
                    rassemble_dépendances_transformations(it.transformations, type_expression);
                }
            }
            else if (noeud->est_init_de()) {
                auto init_de = noeud->comme_init_de();
                ajoute_init_de(
                    init_de->expression->type->comme_type_type_de_données()->type_connu);
            }
            else if (noeud->est_info_de()) {
                auto info_de = noeud->comme_info_de();
                ajoute_info_de(
                    info_de->expression->type->comme_type_type_de_données()->type_connu);
            }

            return DecisionVisiteNoeud::CONTINUE;
        });
}

static void rassemble_dépendances(NoeudExpression *racine,
                                  Compilatrice *compilatrice,
                                  EspaceDeTravail *espace,
                                  DonnéesDépendance &dépendances)
{
    RassembleuseDependances rassembleuse{dépendances, compilatrice, espace, racine};
    rassembleuse.rassemble_dépendances();
}

static bool type_requiers_typage(NoeudDéclarationType const *type)
{
    if (type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    if (type->unité) {
        /* Déjà en cours. */
        return false;
    }

    /* Inutile de typer les unions anonymes, ceci fut fait lors de la validation
     * sémantique. */
    if (type->est_type_union() && type->comme_type_union()->est_anonyme) {
        return false;
    }

    if (type->est_type_structure() && type->comme_type_structure()->union_originelle) {
        /* Les structures pour les unions n'en ont pas besoin. */
        return false;
    }

    return true;
}

/* Requiers le typage de toutes les dépendances. */
void GestionnaireCode::garantie_typage_des_dépendances(
    DonnéesDépendance const &données_dépendances, EspaceDeTravail *espace)
{
    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    kuri::pour_chaque_élément(données_dépendances.fonctions_utilisées, [&](auto &fonction) {
        if (!fonction->corps->unité &&
            !fonction->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE |
                                       DrapeauxNoeudFonction::EST_EXTERNE |
                                       DrapeauxNoeudFonction::EST_OPÉRATAUR_SYNTHÉTIQUE)) {
            requiers_typage(espace, fonction->corps);
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_élément(données_dépendances.globales_utilisées, [&](auto &globale) {
        if (!globale->unité) {
            requiers_typage(espace, const_cast<NoeudDéclarationVariable *>(globale));
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_élément(données_dépendances.types_utilisés, [&](auto &type) {
        if (type_requiers_typage(type)) {
            requiers_typage(espace, type);
        }

        if (type->est_type_fonction()) {
            auto type_fonction = type->comme_type_fonction();
            auto type_retour = type_fonction->type_sortie;
            /* Les types fonctions peuvent retourner une référence à la structure que nous essayons
             * de valider :
             *
             * MaStructure :: struct {
             *  un_rappel: fonc ()(bool, MaStructure)
             * }
             *
             * Donc nous ne pouvons pas effectuer le calcul de la taille des tuples lors de leurs
             * création durant la validation sémantique, car leurs tailles dépenderait de la taille
             * finale de la structure dont la taille dépend sur le type fonction qui essaye de
             * retouner la structure par valeur dans son tuple. Ceci causant une dépendance
             * cyclique.
             *
             * Puisque les types fonctions ont une taille équivalente à un pointeur (leurs types de
             * sortie n'ont aucune influence sur leurs taille), nous calculons au fur et à mesure
             * du besoin des types la taille de leurs tuples.
             */
            if (type_retour->est_type_tuple() && type_retour->taille_octet == 0 &&
                !type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
                auto type_tuple = type_retour->comme_type_tuple();

                auto unité = crée_unité(espace, RaisonDÊtre::CALCULE_TAILLE_TYPE, true);
                unité->type = type_tuple;

                POUR (type_tuple->rubriques) {
                    if (!it.type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                        auto attente = Attente::sur_type(it.type);
                        ajoute_requêtes_pour_attente(espace, attente);
                        unité->ajoute_attente(attente);
                    }
                }
            }
        }

        return kuri::DécisionItération::Continue;
    });

    kuri::pour_chaque_élément(dépendances.init_de_utilisés, [&](auto &type) {
        gestionnaire.requiers_initialisation_type(espace, type);
        return kuri::DécisionItération::Continue;
    });
}

/* Déterminé si nous devons ajouter les dépendances du noeud au programme. */
static bool doit_ajouter_les_dépendances_au_programme(NoeudExpression *noeud, Programme *programme)
{
    if (noeud->est_entête_fonction()) {
        return programme->possède(noeud->comme_entête_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entête;
        return programme->possède(entete);
    }

    if (noeud->est_déclaration_variable()) {
        return programme->possède(noeud->comme_déclaration_variable());
    }

    if (noeud->est_type_structure() || noeud->est_type_énum()) {
        return programme->possède(noeud->type);
    }

    if (noeud->est_ajoute_fini() || noeud->est_ajoute_init()) {
        return !programme->pour_métaprogramme();
    }

    return false;
}

/* Construit les dépendances de l'unité (fonctions, globales, types) et crée des unités de typage
 * pour chacune des dépendances non-encore typée. */
void GestionnaireCode::détermine_dépendances(NoeudExpression *noeud,
                                             EspaceDeTravail *espace,
                                             UniteCompilation *unité_pour_ri,
                                             UniteCompilation *unité_pour_noeud_code)
{
    DÉBUTE_STAT(DÉTERMINE_DÉPENDANCES);
    dépendances.reinitialise();

    DÉBUTE_STAT(RASSEMBLE_DÉPENDANCES);
    rassemble_dépendances(noeud, m_compilatrice, espace, dépendances.dépendances);
    TERMINE_STAT(RASSEMBLE_DÉPENDANCES);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    if (!noeud->est_ajoute_fini() && !noeud->est_ajoute_init()) {
        if (noeud->est_déclaration_variable()) {
            pour_chaque_élément(
                dépendances.dépendances.globales_utilisées,
                [&](NoeudDéclarationVariable *globale) {
                    if (globale->expression && globale->expression->est_non_initialisation()) {
                        espace
                            ->rapporte_avertissement(
                                noeud,
                                "Utilisation d'une globale n'étant pas initialisée "
                                "dans l'initialisation d'une autre globale")
                            .ajoute_message(
                                "Note : la globale non-initialisée fut déclarée ici :\n\n")
                            .ajoute_site(globale);
                    }
                    return kuri::DécisionItération::Continue;
                });
        }

        DÉBUTE_STAT(AJOUTE_DÉPENDANCES);
        auto graphe = espace->graphe_dépendance.verrou_ecriture();
        NoeudDépendance *noeud_dépendance = graphe->garantie_noeud_dépendance(espace, noeud);
        graphe->ajoute_dépendances(*noeud_dépendance, dépendances.dépendances);
        TERMINE_STAT(AJOUTE_DÉPENDANCES);
    }

    /* Nous devons faire çà après la création du noeud de dépendance et de ses relations. */
    if (unité_pour_ri) {
        ajoute_attentes_sur_initialisations_types(noeud, unité_pour_ri);

        pour_chaque_élément(dépendances.dépendances.info_de_utilisés, [&](Type *type) {
            auto attente = Attente::sur_info_type(type);
            ajoute_requêtes_pour_attente(unité_pour_ri->espace, attente);
            unité_pour_ri->ajoute_attente(attente);
            return kuri::DécisionItération::Continue;
        });
    }
    if (unité_pour_noeud_code) {
        ajoute_attentes_pour_noeud_code(noeud, unité_pour_noeud_code);
    }

    /* Ajoute les racines aux programmes courants de l'espace. */
    if (noeud->est_entête_fonction() &&
        noeud->comme_entête_fonction()->possède_drapeau(DrapeauxNoeudFonction::EST_RACINE)) {
        DÉBUTE_STAT(AJOUTE_RACINES);
        auto entete = noeud->comme_entête_fonction();
        POUR (programmes_en_cours) {
            if (it->espace() != espace) {
                continue;
            }

            it->ajoute_racine(entete);

            if (entete->corps && !entete->corps->unité) {
                requiers_typage(espace, entete->corps);
            }
        }
        TERMINE_STAT(AJOUTE_RACINES);
    }

    /* Ajoute les dépendances au programme si nécessaire. */
    auto dépendances_ajoutees = false;
    POUR (programmes_en_cours) {
        if (it->espace() != espace) {
            continue;
        }
        if (!doit_ajouter_les_dépendances_au_programme(noeud, it)) {
            continue;
        }
        auto graphe = espace->graphe_dépendance.verrou_ecriture();
        if (!ajoute_dépendances_au_programme(*graphe, dépendances, espace, *it, noeud)) {
            break;
        }
        dépendances_ajoutees = true;
    }

    /* Crée les unités de typage si nécessaire. */
    if (dépendances_ajoutees) {
        DÉBUTE_STAT(GARANTIE_TYPAGE_DÉPENDANCES);
        dépendances.dépendances.fusionne(dépendances.dépendances_épendues);
        garantie_typage_des_dépendances(dépendances.dépendances, espace);
        TERMINE_STAT(GARANTIE_TYPAGE_DÉPENDANCES);
    }
    TERMINE_STAT(DÉTERMINE_DÉPENDANCES);
    noeud->drapeaux |= DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES;
}

UniteCompilation *GestionnaireCode::crée_unité(EspaceDeTravail *espace,
                                               RaisonDÊtre raison,
                                               bool met_en_attente)
{
    auto unité = unités.ajoute_élément(espace);
    unité->mute_raison_d_être(raison);
    if (met_en_attente) {
        ajoute_unité_à_liste_attente(unité);
    }
    return unité;
}

UniteCompilation *GestionnaireCode::crée_unité_pour_fichier(EspaceDeTravail *espace,
                                                            Fichier *fichier,
                                                            RaisonDÊtre raison)
{
    auto unité = crée_unité(espace, raison, true);
    unité->fichier = fichier;
    return unité;
}

UniteCompilation *GestionnaireCode::crée_unité_pour_noeud(EspaceDeTravail *espace,
                                                          NoeudExpression *noeud,
                                                          RaisonDÊtre raison,
                                                          bool met_en_attente)
{
    auto unité = crée_unité(espace, raison, met_en_attente);
    unité->noeud = noeud;
    *donne_adresse_unité(noeud) = unité;
    return unité;
}

void GestionnaireCode::requiers_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
    TACHE_AJOUTEE(CHARGEMENT);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::CHARGEMENT_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->fut_chargé);
    TACHE_AJOUTEE(LEXAGE);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::LEXAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->fut_lexé);
    TACHE_AJOUTEE(PARSAGE);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

static bool est_corps_texte(NoeudExpression *noeud)
{
    if (noeud->est_corps_fonction()) {
        auto corps_fonction = noeud->comme_corps_fonction();
        return corps_fonction->est_corps_texte &&
               !corps_fonction->entête->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);
    }

    if (noeud->est_type_structure()) {
        auto type_structure = noeud->comme_type_structure();
        return type_structure->est_corps_texte && !type_structure->est_polymorphe;
    }

    if (noeud->est_type_union()) {
        auto type_union = noeud->comme_type_union();
        return type_union->est_corps_texte && !type_union->est_polymorphe;
    }

    return false;
}

static void échange_corps_entêtes(NoeudDéclarationEntêteFonction *ancienne_fonction,
                                  NoeudDéclarationEntêteFonction *nouvelle_fonction)
{
    auto nouveau_corps = nouvelle_fonction->corps;
    auto ancien_corps = ancienne_fonction->corps;

    /* Échange les corps. */
    nouvelle_fonction->corps = ancien_corps;
    ancien_corps->entête = nouvelle_fonction;
    ancien_corps->bloc_parent = nouvelle_fonction->bloc_parent;
    ancien_corps->bloc->bloc_parent = nouvelle_fonction->bloc_paramètres;

    ancienne_fonction->corps = nouveau_corps;
    nouveau_corps->entête = ancienne_fonction;
    nouveau_corps->bloc_parent = ancienne_fonction->bloc_parent;
    nouveau_corps->bloc->bloc_parent = ancienne_fonction->bloc_paramètres;

    /* Remplace les références à ancienne_fonction dans nouvelle_fonction->corps par
     * nouvelle_fonction. */
    visite_noeud(nouvelle_fonction->corps,
                 PreferenceVisiteNoeud::ORIGINAL,
                 false,
                 [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
                     if (noeud->est_bloc()) {
                         auto bloc = noeud->comme_bloc();
                         assert(bloc->appartiens_à_fonction == ancienne_fonction);
                         const_cast<NoeudBloc *>(bloc)->appartiens_à_fonction = nouvelle_fonction;
                     }
                     return DecisionVisiteNoeud::CONTINUE;
                 });
}

static void définis_fonction_courante_pour_corps_texte(NoeudDéclarationCorpsFonction *corps)
{
    visite_noeud(corps,
                 PreferenceVisiteNoeud::ORIGINAL,
                 false,
                 [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
                     if (noeud->est_bloc()) {
                         auto bloc = noeud->comme_bloc();
                         assert(bloc->appartiens_à_fonction == nullptr);
                         const_cast<NoeudBloc *>(bloc)->appartiens_à_fonction = corps->entête;
                     }
                     return DecisionVisiteNoeud::CONTINUE;
                 });
}

MetaProgramme *GestionnaireCode::crée_métaprogramme_corps_texte(EspaceDeTravail *espace,
                                                                NoeudBloc *bloc_corps_texte,
                                                                NoeudBloc *bloc_parent,
                                                                const Lexème *lexème)
{
    auto fonction = m_assembleuse->crée_entête_fonction(lexème);
    auto nouveau_corps = fonction->corps;

    assert(m_assembleuse->bloc_courant() == nullptr);
    m_assembleuse->bloc_courant(bloc_parent);

    fonction->bloc_constantes = m_assembleuse->empile_bloc(lexème, fonction, TypeBloc::CONSTANTES);
    fonction->bloc_paramètres = m_assembleuse->empile_bloc(lexème, fonction, TypeBloc::PARAMÈTRES);

    fonction->bloc_parent = bloc_parent;
    nouveau_corps->bloc_parent = fonction->bloc_paramètres;
    /* Le corps de la fonction pour les #corps_texte des structures est celui de la déclaration. */
    nouveau_corps->bloc = bloc_corps_texte;

    /* mise en place du type de la fonction : () -> chaine */
    fonction->drapeaux_fonction |= (DrapeauxNoeudFonction::EST_MÉTAPROGRAMME |
                                    DrapeauxNoeudFonction::FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE);

    auto decl_sortie = m_assembleuse->crée_déclaration_variable(lexème, nullptr, nullptr);
    decl_sortie->ident = m_compilatrice->table_identifiants->identifiant_pour_chaine("__ret0");
    decl_sortie->type = espace->typeuse.type_chaine;
    decl_sortie->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    fonction->params_sorties.ajoute(decl_sortie);
    fonction->param_sortie = decl_sortie;

    auto types_entrees = kuri::tablet<Type *, 6>(0);

    auto type_sortie = espace->typeuse.type_chaine;

    fonction->type = espace->typeuse.type_fonction(types_entrees, type_sortie);
    fonction->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    auto metaprogramme = m_compilatrice->crée_metaprogramme(espace);
    metaprogramme->corps_texte = bloc_corps_texte;
    metaprogramme->fonction = fonction;

    m_assembleuse->dépile_bloc();
    m_assembleuse->dépile_bloc();
    m_assembleuse->dépile_bloc();
    assert(m_assembleuse->bloc_courant() == nullptr);

    return metaprogramme;
}

void GestionnaireCode::requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    if (est_corps_texte(noeud)) {
        if (noeud->est_corps_fonction()) {
            auto corps_fonction = noeud->comme_corps_fonction();
            auto entête = corps_fonction->entête;

            auto métaprogramme = crée_métaprogramme_corps_texte(
                espace, corps_fonction->bloc, entête->bloc_parent, entête->lexème);
            métaprogramme->corps_texte_pour_fonction = entête;

            auto fonction = métaprogramme->fonction;
            échange_corps_entêtes(entête, fonction);

            if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
                fonction->drapeaux_fonction |= DrapeauxNoeudFonction::EST_MONOMORPHISATION;
            }
            else {
                fonction->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_MONOMORPHISATION;
            }
            fonction->site_monomorphisation = entête->site_monomorphisation;

            // préserve les constantes polymorphiques
            if (fonction->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
                POUR (*entête->bloc_constantes->rubriques.verrou_lecture()) {
                    fonction->bloc_constantes->ajoute_rubrique(it);
                }
            }

            /* Puisque nous avons échangé les corps, le noeud pointe vers le corps de la fonction
             * du métaprogramme, dont la validation sera requise plus bas. Nous devons tout de même
             * requérir la validation de l'« ancien corps », mais la faire attendre sur le parsage
             * du fichier du métaprogramme. */
            auto ancien_corps = entête->corps;

            /* Annule le bloc du corps pour détecter les erreurs. Il sera ajourné après le
             * syntaxage du code source retourné par le métaprogramme. */
            ancien_corps->bloc = nullptr;

            TACHE_AJOUTEE(TYPAGE);
            crée_unité_pour_noeud(espace, ancien_corps, RaisonDÊtre::TYPAGE, true);

            auto fichier = m_compilatrice->crée_fichier_pour_metaprogramme(espace, métaprogramme);
            ancien_corps->unité->ajoute_attente(Attente::sur_parsage(fichier));
        }
        else if (noeud->est_déclaration_classe()) {
            auto decl = noeud->comme_déclaration_classe();
            auto métaprogramme = crée_métaprogramme_corps_texte(
                espace, decl->bloc, decl->bloc_parent, decl->lexème);
            auto fonction = métaprogramme->fonction;
            assert(fonction->corps->bloc);
            fonction->corps->est_corps_texte = true;
            définis_fonction_courante_pour_corps_texte(fonction->corps);

            decl->métaprogramme_corps_texte = métaprogramme;
            métaprogramme->corps_texte_pour_structure = decl;

            if (decl->est_monomorphisation) {
                decl->bloc_constantes->rubriques.avec_verrou_ecriture(
                    [fonction](kuri::tableau<NoeudDéclaration *, int> &rubriques) {
                        POUR (rubriques) {
                            fonction->bloc_constantes->ajoute_rubrique(it);
                        }
                    });
                fonction->site_monomorphisation = decl->site_monomorphisation;
            }

            /* Annule le bloc du type pour détecter les erreurs. Il sera ajourné après le
             * syntaxage du code source retourné par le métaprogramme. */
            decl->bloc = nullptr;

            TACHE_AJOUTEE(TYPAGE);
            crée_unité_pour_noeud(espace, decl, RaisonDÊtre::TYPAGE, true);

            auto fichier = m_compilatrice->crée_fichier_pour_metaprogramme(espace, métaprogramme);
            decl->unité->ajoute_attente(Attente::sur_parsage(fichier));

            /* Pour requérir le typage du corps de métaprogramme plus bas. */
            noeud = fonction->corps;
        }
    }

    TACHE_AJOUTEE(TYPAGE);
    crée_unité_pour_noeud(espace, noeud, RaisonDÊtre::TYPAGE, true);
}

void GestionnaireCode::ajoute_attentes_sur_initialisations_types(NoeudExpression *noeud,
                                                                 UniteCompilation *unité)
{
    auto entête = donne_entête_fonction(noeud);
    if (!entête || entête->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE |
                                           DrapeauxNoeudFonction::EST_OPÉRATAUR_SYNTHÉTIQUE)) {
        return;
    }

    auto noeud_dépendance = entête->noeud_dépendance;
    auto types_utilisés = kuri::ensemblon<Type *, 16>();

    POUR (noeud_dépendance->relations().plage()) {
        if (!it.noeud_fin->est_type()) {
            continue;
        }

        auto type_dépendu = it.noeud_fin->type();
        types_utilisés.insère(type_dépendu);
    }

    auto attentes_possibles = kuri::tablet<Attente, 16>();
    attentes_sur_types_si_drapeau_manquant(
        types_utilisés, DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE, attentes_possibles);

    if (attentes_possibles.taille() == 0) {
        return;
    }

    // dbg() << attentes_possibles.taille() << " attentes sur init_de";
    POUR (attentes_possibles) {
        // dbg() << "-- " << chaine_type(it.type());
        it = Attente::sur_initialisation_type(it.type());
        ajoute_requêtes_pour_attente(unité->espace, it);
        unité->ajoute_attente(it);
    }
}

void GestionnaireCode::ajoute_attentes_pour_noeud_code(NoeudExpression *noeud,
                                                       UniteCompilation *unité)
{
    auto types_utilisés = kuri::ensemblon<Type *, 16>();

    visite_noeud(noeud, PreferenceVisiteNoeud::ORIGINAL, true, [&](NoeudExpression const *racine) {
        auto type = racine->type;
        if (type) {
            types_utilisés.insère(type);
        }

        if (racine->est_entête_fonction()) {
            auto entete = racine->comme_entête_fonction();

            POUR ((*entete->bloc_constantes->rubriques.verrou_ecriture())) {
                if (it->type) {
                    types_utilisés.insère(it->type);
                }
            }

            return DecisionVisiteNoeud::IGNORE_ENFANTS;
        }

        return DecisionVisiteNoeud::CONTINUE;
    });

    auto attentes_possibles = kuri::tablet<Attente, 16>();
    attentes_sur_types_si_drapeau_manquant(
        types_utilisés, DrapeauxNoeud::DECLARATION_FUT_VALIDEE, attentes_possibles);

    if (attentes_possibles.taille() == 0) {
        return;
    }

    POUR (attentes_possibles) {
        ajoute_requêtes_pour_attente(unité->espace, it);
        unité->ajoute_attente(it);
    }
}

void GestionnaireCode::requiers_génération_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    TACHE_AJOUTEE(GENERATION_RI);
    auto unité = crée_unité_pour_noeud(espace, noeud, RaisonDÊtre::GENERATION_RI, true);
    ajoute_attentes_sur_initialisations_types(noeud, unité);
}

void GestionnaireCode::requiers_génération_ri_principale_métaprogramme(
    EspaceDeTravail *espace, MetaProgramme *metaprogramme, bool peut_planifier_compilation)
{
    TACHE_AJOUTEE(GENERATION_RI);

    auto unité = crée_unité_pour_noeud(espace,
                                       metaprogramme->fonction,
                                       RaisonDÊtre::GENERATION_RI_PRINCIPALE_MP,
                                       peut_planifier_compilation);

    ajoute_attentes_sur_initialisations_types(metaprogramme->fonction, unité);

    if (!peut_planifier_compilation) {
        assert(espace->métaprogrammes_en_attente_de_crée_contexte_est_ouvert);
        espace->métaprogrammes_en_attente_de_crée_contexte.ajoute(unité);
    }
}

UniteCompilation *GestionnaireCode::crée_unité_pour_message(EspaceDeTravail *espace,
                                                            Message *message)
{
    auto unité = crée_unité(espace, RaisonDÊtre::ENVOIE_MESSAGE, true);
    unité->message = message;
    return unité;
}

void GestionnaireCode::requiers_initialisation_type(EspaceDeTravail *espace, Type *type)
{
    if (!requiers_création_fonction_initialisation(type)) {
        return;
    }

    if (type->possède_drapeau(DrapeauxTypes::INITIALISATION_TYPE_FUT_REQUISE)) {
        return;
    }

    if (m_validation_doit_attendre_sur_lexage) {
        m_fonctions_init_type_requises.ajoute({espace, type});
        return;
    }

    type->drapeaux_type |= DrapeauxTypes::INITIALISATION_TYPE_FUT_REQUISE;

    auto unité = crée_unité(espace, RaisonDÊtre::CREATION_FONCTION_INIT_TYPE, true);
    unité->type = type;

    if (!type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        unité->ajoute_attente(Attente::sur_type(type));
    }

    type->drapeaux_type |= DrapeauxTypes::UNITE_POUR_INITIALISATION_FUT_CREE;
}

void GestionnaireCode::requiers_ri_pour_opérateur_synthétique(
    EspaceDeTravail *espace, NoeudDéclarationEntêteFonction *entête)
{
    assert(entête->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
    assert(entête->possède_drapeau(DrapeauxNoeudFonction::EST_OPÉRATAUR_SYNTHÉTIQUE));
    requiers_génération_ri(espace, entête);
}

UniteCompilation *GestionnaireCode::requiers_noeud_code(EspaceDeTravail *espace,
                                                        NoeudExpression *noeud)
{
    auto unité = crée_unité(espace, RaisonDÊtre::CONVERSION_NOEUD_CODE, true);
    unité->noeud = noeud;
    return unité;
}

void GestionnaireCode::ajoute_unité_à_liste_attente(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::EN_ATTENTE);
    unités_en_attente.ajoute(unité);
}

bool GestionnaireCode::tente_de_garantir_présence_création_contexte(EspaceDeTravail *espace,
                                                                    Programme *programme)
{
    /* NOTE : la déclaration sera automatiquement ajoutée au programme si elle n'existe pas déjà
     * lors de la complétion de son typage. Si elle existe déjà, il faut l'ajouter manuellement.
     */
    auto decl_creation_contexte = espace->interface_kuri->decl_creation_contexte;
    assert(decl_creation_contexte);

    // À FAIRE : déplace ceci quand toutes les entêtes seront validées avant le reste.
    assert(programme->espace() == espace);
    programme->ajoute_fonction(decl_creation_contexte);

    if (!decl_creation_contexte->unité) {
        requiers_typage(espace, decl_creation_contexte);
        return false;
    }

    if (!decl_creation_contexte->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    détermine_dépendances(decl_creation_contexte, espace, nullptr, nullptr);

    if (!decl_creation_contexte->corps->unité) {
        requiers_typage(espace, decl_creation_contexte->corps);
        return false;
    }

    if (!decl_creation_contexte->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    détermine_dépendances(decl_creation_contexte->corps, espace, nullptr, nullptr);

    if (!decl_creation_contexte->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        return false;
    }

    return true;
}

void GestionnaireCode::requiers_compilation_métaprogramme(EspaceDeTravail *espace,
                                                          MetaProgramme *metaprogramme)
{
    assert(metaprogramme->fonction);
    assert(metaprogramme->fonction->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));

    /* Indique directement à l'espace qu'une exécution sera requise afin de ne pas terminér la
     * compilation trop rapidement si le métaprogramme modifie ses options de compilation. */
    TACHE_AJOUTEE(EXECUTION);

    /* Ajoute le programme à la liste des programmes avant de traiter les dépendances. */
    métaprogramme_créé(metaprogramme);

    auto programme = metaprogramme->programme;
    assert(programme->espace() == espace);
    programme->ajoute_fonction(metaprogramme->fonction);

    détermine_dépendances(metaprogramme->fonction, espace, nullptr, nullptr);
    détermine_dépendances(metaprogramme->fonction->corps, espace, nullptr, nullptr);

    auto ri_crée_contexte_est_disponible = tente_de_garantir_présence_création_contexte(espace,
                                                                                        programme);
    requiers_génération_ri_principale_métaprogramme(
        espace, metaprogramme, ri_crée_contexte_est_disponible);
}

void GestionnaireCode::requiers_exécution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    auto unité = crée_unité(espace, RaisonDÊtre::EXECUTION, true);
    unité->metaprogramme = metaprogramme;
    metaprogramme->unité = unité;
}

UniteCompilation *GestionnaireCode::requiers_génération_code_machine(EspaceDeTravail *espace,
                                                                     Programme *programme)
{
    auto unité = crée_unité(espace, RaisonDÊtre::GENERATION_CODE_MACHINE, true);
    unité->programme = programme;
    TACHE_AJOUTEE(GENERATION_CODE_MACHINE);
    return unité;
}

void GestionnaireCode::requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme)
{
    auto unité = crée_unité(espace, RaisonDÊtre::LIAISON_PROGRAMME, true);
    unité->programme = programme;
    TACHE_AJOUTEE(LIAISON_PROGRAMME);
}

void GestionnaireCode::ajoute_requêtes_pour_attente(EspaceDeTravail *espace, Attente attente)
{
    if (attente.est<AttenteSurType>()) {
        Type *type = const_cast<Type *>(attente.type());
        if (type_requiers_typage(type)) {
            requiers_typage(espace, type);
        }
    }
    else if (attente.est<AttenteSurDéclaration>()) {
        NoeudDéclaration *decl = attente.déclaration();
        if (*donne_adresse_unité(decl) == nullptr) {
            requiers_typage(espace, decl);
        }
    }
    else if (attente.est<AttenteSurTypeDéclaration>()) {
        NoeudDéclaration *decl = attente.type_déclaration();
        if (*donne_adresse_unité(decl) == nullptr) {
            requiers_typage(espace, decl);
        }
    }
    else if (attente.est<AttenteSurInitialisationType>()) {
        auto type = const_cast<Type *>(attente.initialisation_type());
        requiers_initialisation_type(espace, type);
    }
    else if (attente.est<AttenteSurInfoType>()) {
        Type *type = const_cast<Type *>(attente.info_type());
        if (type_requiers_typage(type)) {
            requiers_typage(espace, type);
        }
    }
    else if (attente.est<AttenteSurParsage>()) {
        auto fichier = attente.fichier_à_parser();
        // À FAIRE @Rigidité, nous pouvons le requérir plusieurs fois
        if (!fichier->en_lexage) {
            requiers_lexage(espace, fichier);
        }
    }
    else if (attente.est<AttenteSurSynthétisationOpérateur>()) {
        auto opérateur_synthétisé = attente.synthétisation_opérateur();
        requiers_synthétisation_opérateur(espace,
                                          const_cast<OpérateurBinaire *>(opérateur_synthétisé));
    }
}

void GestionnaireCode::imprime_état_parsage() const
{
    m_état_chargement_fichiers.imprime_état();
}

bool GestionnaireCode::tous_les_fichiers_à_parser_le_sont() const
{
    return m_état_chargement_fichiers.tous_les_fichiers_à_parser_le_sont();
}

void GestionnaireCode::flush_noeuds_à_typer()
{
    /* Désactive ceci directement car requiers_initialisation_type y dépends. */
    m_validation_doit_attendre_sur_lexage = false;

    POUR (m_fonctions_init_type_requises) {
        requiers_initialisation_type(it.espace, it.type);
    }
    m_fonctions_init_type_requises.efface();

    POUR (m_noeuds_à_valider) {
        if (*donne_adresse_unité(it.noeud)) {
            continue;
        }

        requiers_typage(it.espace, it.noeud);
    }
    m_noeuds_à_valider.efface();
}

void GestionnaireCode::rassemble_statistiques(Statistiques &statistiques) const
{
    auto mémoire = int64_t(0);
    mémoire += unités.mémoire_utilisée();
    mémoire += unités_en_attente.taille_mémoire();
    mémoire += programmes_en_cours.taille_mémoire();
    mémoire += m_noeuds_à_valider.taille_mémoire();
    mémoire += m_fonctions_init_type_requises.taille_mémoire();
    mémoire += m_nouvelles_unités.taille_mémoire();
    mémoire += unités_temporisées.taille_mémoire();
    mémoire += nouvelles_unités_temporisées.taille_mémoire();

    mémoire += dépendances.dépendances.mémoire_utilisée();
    mémoire += dépendances.dépendances_épendues.mémoire_utilisée();

    POUR_TABLEAU_PAGE (unités) {
        mémoire += it.mémoire_utilisée();
    }

    allocatrice_noeud.rassemble_statistiques(statistiques);

    statistiques.ajoute_mémoire_utilisée("Gestionnaire Code", mémoire);
}

NoeudBloc *GestionnaireCode::crée_bloc_racine(Typeuse &typeuse)
{
#define CREE_DECLARATION_TYPE_PLATEFORME(nom)                                                     \
    résultat->ajoute_rubrique(typeuse.nom);                                                       \
    typeuse.nom->expression_type = m_assembleuse->crée_référence_déclaration(                     \
        nullptr, typeuse.nom->comme_déclaration_type());                                          \
    typeuse.nom->bloc_parent = résultat;

    auto résultat = m_assembleuse->crée_bloc_seul(nullptr, nullptr);

    CREE_DECLARATION_TYPE_PLATEFORME(type_dff_adr);
    CREE_DECLARATION_TYPE_PLATEFORME(type_adr_plt_nat);
    CREE_DECLARATION_TYPE_PLATEFORME(type_adr_plt_rel);
    CREE_DECLARATION_TYPE_PLATEFORME(type_taille_nat);
    CREE_DECLARATION_TYPE_PLATEFORME(type_taille_rel);
    CREE_DECLARATION_TYPE_PLATEFORME(type_nbr_nat);
    CREE_DECLARATION_TYPE_PLATEFORME(type_nbr_rel);
    CREE_DECLARATION_TYPE_PLATEFORME(type_nbf_flt);

#undef CREE_DECLARATION_TYPE_PLATEFORME

    return résultat;
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unité_attendante, Attente attente)
{
    assert(attente.est_valide());
    assert(unité_attendante->est_prête());
    auto espace = unité_attendante->espace;
    ajoute_requêtes_pour_attente(espace, attente);
    unité_attendante->ajoute_attente(attente);
    ajoute_unité_à_liste_attente(unité_attendante);
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unité_attendante,
                                       kuri::tableau_statique<Attente> attentes)
{
    assert(attentes.taille() != 0);
    assert(unité_attendante->est_prête());

    auto espace = unité_attendante->espace;

    POUR (attentes) {
        ajoute_requêtes_pour_attente(espace, it);
        unité_attendante->ajoute_attente(it);
    }

    ajoute_unité_à_liste_attente(unité_attendante);
}

void GestionnaireCode::tâche_unité_terminée(UniteCompilation *unité)
{
#ifdef TEMPORISE_UNITES_POUR_SIMULER_MOULTFILAGE
    auto dist = std::uniform_real_distribution<double>(0.0, 1.0);
    if (unité->fut_temporisée == false && dist(mt) > 0.25) {
        auto info = InfoUnitéTemporisée{};
        info.unité = unité;
        info.cycles_à_temporiser = int(dist(mt) * 1000.0);

        unité->fut_temporisée = true;
        unités_temporisées.ajoute(info);
        return;
    }

    // unité->fut_temporisée = false;
#endif

    switch (unité->donne_raison_d_être()) {
        case RaisonDÊtre::AUCUNE:
        {
            // erreur ?
            break;
        }
        case RaisonDÊtre::CHARGEMENT_FICHIER:
        {
            chargement_fichier_terminé(unité);
            break;
        }
        case RaisonDÊtre::LEXAGE_FICHIER:
        {
            lexage_fichier_terminé(unité);
            break;
        }
        case RaisonDÊtre::PARSAGE_FICHIER:
        {
            parsage_fichier_terminé(unité);
            break;
        }
        case RaisonDÊtre::CREATION_FONCTION_INIT_TYPE:
        {
            fonction_initialisation_type_créée(unité);
            break;
        }
        case RaisonDÊtre::TYPAGE:
        {
            typage_terminé(unité);
            break;
        }
        case RaisonDÊtre::CONVERSION_NOEUD_CODE:
        {
            conversion_noeud_code_terminée(unité);
            break;
        }
        case RaisonDÊtre::ENVOIE_MESSAGE:
        {
            envoi_message_terminé(unité);
            break;
        }
        case RaisonDÊtre::GENERATION_RI:
        {
            generation_ri_terminée(unité);
            break;
        }
        case RaisonDÊtre::GENERATION_RI_PRINCIPALE_MP:
        {
            generation_ri_terminée(unité);
            break;
        }
        case RaisonDÊtre::EXECUTION:
        {
            execution_terminée(unité);
            break;
        }
        case RaisonDÊtre::LIAISON_PROGRAMME:
        {
            liaison_programme_terminée(unité);
            break;
        }
        case RaisonDÊtre::GENERATION_CODE_MACHINE:
        {
            generation_code_machine_terminée(unité);
            break;
        }
        case RaisonDÊtre::CALCULE_TAILLE_TYPE:
        {
            /* Rien à faire pour le moment. */
            break;
        }
        case RaisonDÊtre::SYNTHÉTISATION_OPÉRATEUR:
        {
            synthétisation_opérateur_terminée(unité);
            break;
        }
    }
}

void GestionnaireCode::chargement_fichier_terminé(UniteCompilation *unité)
{
    assert(unité->fichier);
    assert(unité->fichier->fut_chargé);

    auto espace = unité->espace;
    TACHE_TERMINEE(CHARGEMENT);
    m_compilatrice->messagère->ajoute_message_fichier_fermé(espace, unité->fichier->chemin());

    /* Une fois que nous avons fini de charger un fichier, il faut le lexer. */
    unité->mute_raison_d_être(RaisonDÊtre::LEXAGE_FICHIER);
    m_état_chargement_fichiers.déplace_unité_pour_chargement_fichier(unité);
    ajoute_unité_à_liste_attente(unité);
    TACHE_AJOUTEE(LEXAGE);
}

void GestionnaireCode::lexage_fichier_terminé(UniteCompilation *unité)
{
    assert(unité->fichier);
    assert(unité->fichier->fut_lexé);

    auto espace = unité->espace;
    TACHE_TERMINEE(LEXAGE);

    /* Une fois que nous avons lexer un fichier, il faut le parser. */
    unité->mute_raison_d_être(RaisonDÊtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.déplace_unité_pour_chargement_fichier(unité);
    ajoute_unité_à_liste_attente(unité);
    TACHE_AJOUTEE(PARSAGE);
}

static Module *donne_module_existant_pour_importe(NoeudInstructionImporte *inst,
                                                  Module *module_du_fichier)
{
    auto const expression = inst->expression;
    if (expression->lexème->genre != GenreLexème::CHAINE_CARACTERE) {
        /* L'expression est un chemin relatif. */
        return nullptr;
    }

    /* À FAIRE : meilleure mise en cache. */
    auto module = static_cast<Module *>(nullptr);
    pour_chaque_élément(module_du_fichier->modules_importés, [&](ModuleImporté const &module_) {
        if (module_.module->nom() == expression->ident) {
            module = module_.module;
            return kuri::DécisionItération::Arrête;
        }

        return kuri::DécisionItération::Continue;
    });

    return module;
}

void GestionnaireCode::ajoute_noeud_de_haut_niveau(NoeudExpression *it,
                                                   EspaceDeTravail *espace,
                                                   Fichier *fichier)
{
    /* Nous avons sans doute déjà requis le typage de ce noeud. */
    auto adresse_unité = donne_adresse_unité(it);
    if (*adresse_unité) {
        return;
    }

    if (it->est_charge()) {
        auto inst = it->comme_charge();
        auto const lexème = inst->expression->lexème;
        auto const nom = lexème->chaine;
        auto module = fichier->module;
        auto chemin = enchaine(module->chemin(), nom);

        if (chemin.trouve(".kuri") == kuri::chaine::npos) {
            chemin = enchaine(chemin, ".kuri");
        }

        auto opt_chemin = determine_chemin_absolu(espace, chemin, it);
        if (!opt_chemin.has_value()) {
            return;
        }
        auto résultat = espace->sys_module->trouve_ou_crée_fichier(
            module, nom, opt_chemin.value());

        if (std::holds_alternative<FichierNeuf>(résultat)) {
            auto nouveau_fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
            requiers_chargement(espace, nouveau_fichier);
        }
        else {
            auto fichier_existant = static_cast<Fichier *>(std::get<FichierExistant>(résultat));
            if (fichier_existant == fichier) {
                espace->rapporte_erreur(it, "chargement du fichier dans lui-même");
                return;
            }
        }
    }
    else if (it->est_importe()) {
        auto inst = it->comme_importe();
        auto const module_du_fichier = fichier->module;

        auto module = donne_module_existant_pour_importe(inst, module_du_fichier);
        if (!module) {
            const auto lexème = inst->expression->lexème;

            auto info_module = espace->sys_module->trouve_ou_crée_module(
                m_compilatrice->table_identifiants, fichier, lexème->chaine);

            switch (info_module.état) {
                case InfoRequêteModule::État::TROUVÉ:
                {
                    module = info_module.module;
                    if (module->importé) {
                        break;
                    }
                    module->importé = true;
                    m_compilatrice->messagère->ajoute_message_module_ouvert(espace, module);
                    POUR_NOMME (f, module->fichiers) {
                        requiers_chargement(espace, f);
                    }
                    m_compilatrice->messagère->ajoute_message_module_fermé(espace, module);
                    break;
                }
                case InfoRequêteModule::État::CHEMIN_INEXISTANT:
                {
                    espace->rapporte_erreur(inst,
                                            "Le nom du module ne pointe pas vers un dossier.");
                    return;
                }
                case InfoRequêteModule::État::PAS_UN_DOSSIER:
                {
                    kuri::chaine_statique message_chemins_testés;
                    if (info_module.chemins_testés.taille() > 1) {
                        message_chemins_testés = "Le chemin testé fut :\n";
                    }
                    else {
                        message_chemins_testés = "Les chemins testés furent :\n";
                    }

                    auto &e = espace
                                  ->rapporte_erreur(
                                      inst,
                                      "Impossible de trouver un dossier correspondant au module "
                                      "importé.")
                                  .ajoute_message(message_chemins_testés);

                    POUR_NOMME (chemin, info_module.chemins_testés) {
                        e.ajoute_message("    ", chemin, "\n");
                    }
                    return;
                }
                case InfoRequêteModule::État::PAS_DE_FICHIER_MODULE_KURI:
                {
                    espace->rapporte_erreur(inst,
                                            "Impossible d'importer le module, car le dossier "
                                            "ne contient de fichier 'module.kuri'.");
                    return;
                }
            }
        }

        if (module_du_fichier == module) {
            espace->rapporte_erreur(inst, "Import d'un module dans lui-même.\n");
            return;
        }

        if (module_du_fichier->importe_module(module->nom())) {
            if (fichier->source != SourceFichier::CHAINE_AJOUTÉE) {
                /* Ignore les fichiers de chaines ajoutées afin de permettre aux métaprogrammes
                 * de générer ces instructions redondantes. */
                espace->rapporte_info(inst, "Import superflux du module");
            }
        }
        else {
            module_du_fichier->modules_importés.insère({module, inst->est_employé});
        }

        auto noeud_déclaration = inst->noeud_déclaration;
        if (noeud_déclaration->ident == nullptr) {
            noeud_déclaration->ident = module->nom();
            noeud_déclaration->bloc_parent->ajoute_rubrique(noeud_déclaration);
        }
        noeud_déclaration->module = module;

        // m_état_chargement_fichiers.ajoute_unité_pour_charge_ou_importe(*adresse_unité);
    }
    else {
        if (tous_les_fichiers_à_parser_le_sont()) {
            requiers_typage(espace, it);
        }
        else {
            m_noeuds_à_valider.ajoute({espace, it});
        }
    }
}

void GestionnaireCode::parsage_fichier_terminé(UniteCompilation *unité)
{
    assert(unité->fichier);
    assert(unité->fichier->fut_parsé);
    auto espace = unité->espace;
    TACHE_TERMINEE(PARSAGE);
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
    m_état_chargement_fichiers.supprime_unité_pour_chargement_fichier(unité);

    auto fichier = unité->fichier;

    POUR (fichier->noeuds_à_valider) {
        ajoute_noeud_de_haut_niveau(it, espace, fichier);
    }

    /* Il est possible que tous les noeuds de charge et d'import furent géré alors que des fichiers
     * qui ne chargent ni n'importe quoi que ce soit sont encore en parsage. Vérifions si plus rien
     * n'est à charger ici aussi et pas uniquement lors de la notification de typage terminé pour
     * les chargements/imports. */
    if (tous_les_fichiers_à_parser_le_sont()) {
        flush_noeuds_à_typer();
    }
}

static bool noeud_requiers_generation_ri(NoeudExpression *noeud)
{
    if (noeud->est_entête_fonction()) {
        auto entete = noeud->comme_entête_fonction();
        /* La génération de RI pour les fonctions comprend également leurs corps, or le corps
         * n'est peut-être pas encore typé. Les fonctions externes n'ayant pas de corps (même si le
         * pointeur vers le corps est valide), nous devons quand même les envoyer vers la RI afin
         * que leurs déclarations en RI soient disponibles.
         *
         * Pour les métaprogrammes, la RI doit se faire via requiers_compilation_metaprogramme. Il
         * est possible que les métaprogrammes arrivent ici après le typage, notamment pour les
         * #corps_textes.
         */
        return !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME) &&
               !entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) &&
               !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MACRO) &&
               entete->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
               !entete->est_opérateur_pour();
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entête;

        /* Puisque les métaprogrammes peuvent ajouter des chaines à la compilation, nous devons
         * attendre la génération de code final avant de générer la RI pour ces fonctions. */
        if (est_élément(entete->ident,
                        ID::init_execution_kuri,
                        ID::fini_execution_kuri,
                        ID::init_globales_kuri)) {
            return false;
        }

        /* Pour les métaprogrammes, la RI doit se faire via requiers_compilation_metaprogramme. Il
         * est possible que les métaprogrammes arrivent ici après le typage, notamment pour les
         * #corps_textes.
         */
        return !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME) &&
               !entete->est_opérateur_pour() &&
               !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MACRO) &&
               (!entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) ||
                entete->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION));
    }

    if (noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE) && !noeud->est_type_structure() &&
        !noeud->est_type_énum() && !noeud->est_type_union() &&
        !noeud->est_déclaration_bibliothèque() && !noeud->est_déclaration_constante() &&
        !noeud->est_déclaration_module()) {
        if (noeud->est_exécute()) {
            /* Les #exécutes globales sont gérées via les métaprogrammes. */
            return false;
        }
        return true;
    }

    return false;
}

static bool doit_déterminer_les_dépendances(NoeudExpression *noeud)
{
    if (noeud->est_déclaration()) {
        if (est_déclaration_polymorphique(noeud->comme_déclaration())) {
            return false;
        }

        return !(noeud->est_charge() || noeud->est_importe() ||
                 noeud->est_déclaration_bibliothèque() || noeud->est_déclaration_constante() ||
                 noeud->est_déclaration_module());
    }

    if (noeud->est_exécute()) {
        return true;
    }

    if (noeud->est_ajoute_fini()) {
        return true;
    }

    if (noeud->est_ajoute_init()) {
        return true;
    }

    return false;
}

void GestionnaireCode::typage_terminé(UniteCompilation *unité)
{
    DÉBUTE_STAT(TYPAGE_TERMINÉ);
    assert(unité->noeud);

    auto espace = unité->espace;
    auto noeud = unité->noeud;
    if (noeud == espace->fonction_point_d_entree &&
        espace->options.résultat == RésultatCompilation::EXÉCUTABLE) {
        noeud->comme_entête_fonction()->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
        noeud->comme_entête_fonction()->corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }
    else if ((noeud == espace->fonction_point_d_entree_dynamique ||
              noeud == espace->fonction_point_de_sortie_dynamique) &&
             espace->options.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE) {
        noeud->comme_entête_fonction()->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
        noeud->comme_entête_fonction()->corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }

    if (noeud->est_si_statique()) {
        auto bloc_parent = noeud->bloc_parent;
        assert(bloc_parent->type_bloc == TypeBloc::MODULE);

        auto si_statique = noeud->comme_si_statique();
        auto bloc = donne_bloc_à_fusionner(si_statique);

        auto fichier = espace->fichier(si_statique->lexème->fichier);

        if (bloc) {
            POUR (*bloc->rubriques.verrou_ecriture()) {
                bloc_parent->ajoute_rubrique(it);
            }
            POUR (*bloc->expressions.verrou_ecriture()) {
                ajoute_noeud_de_haut_niveau(it, espace, fichier);
            }
        }

        TACHE_TERMINEE(TYPAGE);
        return;
    }

    assert_rappel(unité->noeud->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE), [&] {
        dbg() << "Le noeud de genre " << unité->noeud->genre << " ne fut pas validé !\n"
              << erreur::imprime_site(*unité->espace, unité->noeud);
    });

    UniteCompilation *unité_pour_ri = nullptr;
    UniteCompilation *unité_pour_noeud_code = nullptr;

    /* Envoi un message, nous attendrons dessus si nécessaire. */
    const auto message = m_compilatrice->messagère->ajoute_message_typage_code(espace, noeud);
    const auto doit_envoyer_en_ri = noeud_requiers_generation_ri(noeud);
    if (doit_envoyer_en_ri) {
        TACHE_AJOUTEE(GENERATION_RI);
        unité->mute_raison_d_être(RaisonDÊtre::GENERATION_RI);
        unité_pour_ri = unité;
        ajoute_unité_à_liste_attente(unité);
    }

    if (message) {
        unité_pour_noeud_code = requiers_noeud_code(espace, noeud);
        auto unité_message = crée_unité_pour_message(espace, message);
        unité_message->ajoute_attente(Attente::sur_noeud_code(unité_pour_noeud_code, noeud));
        unité->ajoute_attente(Attente::sur_message(unité_message, message));
    }

    // rassemble toutes les dépendances de la fonction ou de la globale
    DÉBUTE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    auto const détermine_les_dépendances = doit_déterminer_les_dépendances(unité->noeud);
    TERMINE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    if (détermine_les_dépendances) {
        détermine_dépendances(unité->noeud, unité->espace, unité_pour_ri, unité_pour_noeud_code);
    }

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    TACHE_TERMINEE(TYPAGE);

    TERMINE_STAT(TYPAGE_TERMINÉ);
}

static inline bool est_corps_de(NoeudExpression const *noeud,
                                NoeudDéclarationEntêteFonction const *fonction)
{
    if (fonction == nullptr) {
        return false;
    }
    return noeud == fonction->corps;
}

void GestionnaireCode::generation_ri_terminée(UniteCompilation *unité)
{
    assert(unité->noeud);
    assert_rappel(unité->noeud->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&] {
        dbg() << "Le noeud de genre " << unité->noeud->genre << " n'eu pas de RI générée !\n"
              << erreur::imprime_site(*unité->espace, unité->noeud);
    });

    auto espace = unité->espace;
    TACHE_TERMINEE(GENERATION_RI);
    if (espace->optimisations) {
        // À FAIRE(gestion) : tâches d'optimisations
    }

    /* Si nous avons la RI pour #crée_contexte, il nout faut ajouter toutes les unités l'attendant.
     */
    if (est_corps_de(unité->noeud, espace->interface_kuri->decl_creation_contexte)) {
        flush_métaprogrammes_en_attente_de_crée_contexte(espace);
    }

    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::optimisation_terminée(UniteCompilation *unité)
{
    assert(unité->noeud);
    auto espace = unité->espace;
    TACHE_TERMINEE(OPTIMISATION);
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::envoi_message_terminé(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::message_reçu(Message const *message)
{
    const_cast<Message *>(message)->message_reçu = true;
}

void GestionnaireCode::execution_terminée(UniteCompilation *unité)
{
    assert(unité->metaprogramme);
    assert(unité->metaprogramme->fut_exécuté());
    auto espace = unité->espace;
    TACHE_TERMINEE(EXECUTION);
    enleve_programme(unité->metaprogramme->programme);
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

static bool programme_requiers_liaison_exécutable(OptionsDeCompilation const &options)
{
    switch (options.résultat) {
        case RésultatCompilation::RIEN:
        case RésultatCompilation::FICHIER_OBJET:
        {
            return false;
        }
        case RésultatCompilation::BIBLIOTHÈQUE_STATIQUE:
        case RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE:
        case RésultatCompilation::EXÉCUTABLE:
        {
            return true;
        }
    }
    return false;
}

void GestionnaireCode::generation_code_machine_terminée(UniteCompilation *unité)
{
    assert(unité->programme);

    auto programme = unité->programme;
    auto espace = unité->espace;

    if (programme->pour_métaprogramme()) {
        programme->change_de_phase(PhaseCompilation::APRÈS_GÉNÉRATION_OBJET);
        programme->change_de_phase(PhaseCompilation::AVANT_LIAISON_EXÉCUTABLE);
        requiers_liaison_executable(espace, unité->programme);
    }
    else {
        TACHE_TERMINEE(GENERATION_CODE_MACHINE);

        if (programme_requiers_liaison_exécutable(espace->options)) {
            espace->change_de_phase(
                m_compilatrice->messagère, PhaseCompilation::AVANT_LIAISON_EXÉCUTABLE, __func__);
            requiers_liaison_executable(espace, unité->programme);
        }
        else {
            espace->change_de_phase(
                m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE, __func__);
        }
    }

    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::liaison_programme_terminée(UniteCompilation *unité)
{
    assert(unité->programme);

    auto programme = unité->programme;
    auto espace = unité->espace;

    if (programme->pour_métaprogramme()) {
        auto metaprogramme = programme->pour_métaprogramme();
        programme->change_de_phase(PhaseCompilation::APRÈS_LIAISON_EXÉCUTABLE);
        programme->change_de_phase(PhaseCompilation::COMPILATION_TERMINÉE);
        requiers_exécution(unité->espace, metaprogramme);
    }
    else {
        TACHE_TERMINEE(LIAISON_PROGRAMME);
        espace->change_de_phase(
            m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE, __func__);
    }

    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::conversion_noeud_code_terminée(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::fonction_initialisation_type_créée(UniteCompilation *unité)
{
    assert(unité->type->possède_drapeau(DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE));

    auto fonction = unité->type->fonction_init;
    if (fonction->unité) {
        /* Pour les pointeurs, énums, et fonctions, la fonction est partagée, nous ne devrions pas
         * générer la RI plusieurs fois. L'unité de compilation est utilisée pour indiquée que la
         * RI est en cours de génération. */
        return;
    }

    POUR (programmes_en_cours) {
        if (it->espace() == unité->espace) {
            if (it->possède_init_types(unité->type)) {
                // if (it->possède(unité->type)) {
                it->ajoute_fonction(fonction);
            }
        }
    }

    détermine_dépendances(fonction, unité->espace, nullptr, nullptr);
    détermine_dépendances(fonction->corps, unité->espace, nullptr, nullptr);

    unité->mute_raison_d_être(RaisonDÊtre::GENERATION_RI);
    auto espace = unité->espace;
    TACHE_AJOUTEE(GENERATION_RI);
    unité->noeud = fonction;
    fonction->unité = unité;
    ajoute_unité_à_liste_attente(unité);
}

void GestionnaireCode::requiers_synthétisation_opérateur(EspaceDeTravail *espace,
                                                         OpérateurBinaire *opérateur_binaire)
{
    auto unité = crée_unité(espace, RaisonDÊtre::SYNTHÉTISATION_OPÉRATEUR, true);
    unité->opérateur_binaire = opérateur_binaire;
}

void GestionnaireCode::synthétisation_opérateur_terminée(UniteCompilation *unité)
{
    auto opérateur_binaire = unité->opérateur_binaire;
    assert(opérateur_binaire->decl);
    auto entête = opérateur_binaire->decl;
    assert(entête->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
    assert(entête->possède_drapeau(DrapeauxNoeudFonction::EST_OPÉRATAUR_SYNTHÉTIQUE));

    détermine_dépendances(entête, unité->espace, nullptr, nullptr);
    détermine_dépendances(entête->corps, unité->espace, nullptr, nullptr);

    unité->mute_raison_d_être(RaisonDÊtre::GENERATION_RI);
    auto espace = unité->espace;
    TACHE_AJOUTEE(GENERATION_RI);
    unité->noeud = entête;
    entête->unité = unité;
    ajoute_unité_à_liste_attente(unité);
}

void GestionnaireCode::crée_tâches(OrdonnanceuseTache &ordonnanceuse)
{
#ifdef TEMPORISE_UNITES_POUR_SIMULER_MOULTFILAGE
    nouvelles_unités_temporisées.efface();
    POUR (unités_temporisées) {
        it.cycle_courant += 1;
        if (it.cycle_courant > it.cycles_à_temporiser) {
            tâche_unité_terminée(it.unité);
            continue;
        }
        nouvelles_unités_temporisées.ajoute(it);
    }
    unités_temporisées.permute(nouvelles_unités_temporisées);
#endif

    DÉBUTE_STAT(CRÉATION_TÂCHES);
    m_nouvelles_unités.efface();

    if (plus_rien_n_est_à_faire()) {
        ordonnanceuse.marque_compilation_terminee();
        TERMINE_STAT(CRÉATION_TÂCHES);
        return;
    }

#undef DEBUG_UNITES_EN_ATTENTES

#ifdef DEBUG_UNITES_EN_ATTENTES
    std::cerr << "Unités en attente avant la création des tâches : " << unités_en_attente.taille()
              << '\n';
    ordonnanceuse.imprime_donnees_files(std::cerr);
#endif

    POUR (unités_en_attente) {
        if (it->espace->possède_erreur) {
            auto raison = it->donne_raison_d_être();
            if (raison == RaisonDÊtre::PARSAGE_FICHIER ||
                raison == RaisonDÊtre::CHARGEMENT_FICHIER ||
                raison == RaisonDÊtre::LEXAGE_FICHIER) {
                m_état_chargement_fichiers.supprime_unité_pour_chargement_fichier(it);
            }
            it->définis_état(UniteCompilation::État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR);
            continue;
        }

        if (it->donne_raison_d_être() == RaisonDÊtre::AUCUNE) {
            it->espace->rapporte_erreur_sans_site(
                "Erreur interne : obtenu une unité sans raison d'être");
            continue;
        }

        if (it->fut_annulée()) {
            continue;
        }

        /* Il est possible qu'un métaprogramme ajout du code, donc soyons sûr que l'espace est bel
         * et bien dans la phase pour la génération de code. */
        if (it->donne_raison_d_être() == RaisonDÊtre::GENERATION_CODE_MACHINE &&
            it->programme == it->espace->programme) {
            if (it->espace->phase_courante() != PhaseCompilation::AVANT_GÉNÉRATION_OBJET) {
                continue;
            }
        }

        auto const état_attente = it->détermine_état_attentes();

        switch (état_attente) {
            case UniteCompilation::ÉtatAttentes::ATTENTES_BLOQUÉES:
            {
                it->rapporte_erreur();
                unités_en_attente.efface();
                ordonnanceuse.supprime_toutes_les_tâches();
                return;
            }
            case UniteCompilation::ÉtatAttentes::ATTENTES_NON_RÉSOLUES:
            {
                it->cycle += 1;
                m_nouvelles_unités.ajoute(it);
                break;
            }
            case UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES:
            case UniteCompilation::ÉtatAttentes::UN_SYMBOLE_EST_ATTENDU:
            case UniteCompilation::ÉtatAttentes::UN_OPÉRATEUR_EST_ATTENDU:
            {
                it->définis_état(UniteCompilation::État::DONNÉE_À_ORDONNANCEUSE);
                ordonnanceuse.crée_tâche_pour_unité(it);
                break;
            }
        }
    }

    /* Supprime toutes les tâches des espaces erronés. Il est possible qu'une erreur soit lancée
     * durant la création de tâches ci-dessus, et que l'erreur ne génère pas une fin totale de la
     * compilation. Nous ne pouvons faire ceci ailleurs (dans la fonction qui rapporte l'erreur)
     * puisque nous possédons déjà un verrou sur l'ordonnanceuse, et nous risquerions d'avoir un
     * verrou mort. */
    kuri::ensemblon<EspaceDeTravail *, 10> espaces_errones;
    POUR (programmes_en_cours) {
        if (it->espace()->possède_erreur) {
            espaces_errones.insère(it->espace());
        }
    }

    pour_chaque_élément(espaces_errones, [&](EspaceDeTravail *espace) {
        ordonnanceuse.supprime_toutes_les_tâches_pour_espace(
            espace, UniteCompilation::État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR);
        return kuri::DécisionItération::Continue;
    });

    if (m_compilatrice->possède_erreur()) {
        ordonnanceuse.supprime_toutes_les_tâches();
    }

    unités_en_attente.permute(m_nouvelles_unités);

    TERMINE_STAT(CRÉATION_TÂCHES);

#ifdef DEBUG_UNITES_EN_ATTENTES
    std::cerr << "Unités en attente après la création des tâches : " << unités_en_attente.taille()
              << '\n';
    ordonnanceuse.imprime_donnees_files(std::cerr);
    std::cerr << "--------------------------------------------------------\n";
#endif

#if 0
    if (unités_en_attente.taille() == 0 && ordonnanceuse.nombre_de_tâches_en_attente() == 0) {
        dbg() << "Nous avons un problème..."
              << "\n   m_validation_doit_attendre_sur_lexage "
              << m_validation_doit_attendre_sur_lexage
              << "\n   métaprogrammes_en_attente_de_crée_contexte "
              << métaprogrammes_en_attente_de_crée_contexte.taille();

        POUR (programmes_en_cours) {
            if (it->pour_métaprogramme()) {
                dbg() << "    métaprogramme "
                      << it->pour_métaprogramme()->donne_nom_pour_fichier_log()
                      // << "\n        phase : " << it->ajourne_état_compilation().phase_courante()
                      << "\n        état : " << it->pour_métaprogramme()->état;
            }
            else if (it->espace()->phase_courante() != PhaseCompilation::COMPILATION_TERMINÉE) {
                dbg() << "    " << it->espace()->nom << " : " << it->espace()->phase_courante()
                      << "\n        ri générées " << it->ri_générées() << "\n        ri générées "
                      << it->espace()->peut_generer_code_final();
            }
        }
    }
#endif
}

bool GestionnaireCode::plus_rien_n_est_à_faire()
{
    if (m_compilatrice->possède_erreur()) {
        return true;
    }

    if (m_validation_doit_attendre_sur_lexage) {
        return false;
    }

    auto espace_errone_existe = false;

    POUR (programmes_en_cours) {
        auto espace = it->espace();

        /* Vérifie si une erreur existe. */
        if (espace->possède_erreur) {
            /* Nous devons continuer d'envoyer des messages si l'erreur
             * ne provoque pas la fin de la compilation de tous les espaces.
             * À FAIRE : même si un message est ajouté, purge_message provoque
             * une compilation infinie. */
            if (!espace->options.continue_si_erreur) {
                if (espace->metaprogramme) {
                    std::unique_lock verrou(espace->metaprogramme->mutex_file_message);
                    espace->metaprogramme->file_message.efface();
                }
            }

            espace->change_de_phase(
                m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE, __func__);

            if (!espace->options.continue_si_erreur) {
                return true;
            }

            espace_errone_existe = true;
            continue;
        }

        // it->imprime_diagnostique(std::cerr, !it->pour_métaprogramme());

        if (it->pour_métaprogramme()) {
            auto etat = it->ajourne_état_compilation();

            if (etat.phase_courante() == PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE) {
                it->change_de_phase(PhaseCompilation::AVANT_GÉNÉRATION_OBJET);
                requiers_génération_code_machine(espace, it);
            }
        }
        else {
            // auto diagnostique = it->imprime_diagnostique(true);
            // if (diagnostique.taille()) {
            //     dbg() << diagnostique;
            // }

            if (espace->phase_courante() == PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE &&
                it->ri_générées()) {
                finalise_programme_avant_génération_code_machine(espace, it);
            }
        }
    }

    if (espace_errone_existe) {
        programmes_en_cours.efface_si(
            [](Programme *programme) -> bool { return programme->espace()->possède_erreur; });

        if (programmes_en_cours.est_vide()) {
            return true;
        }
    }

    if (!unités_en_attente.est_vide()) {
        return false;
    }

    POUR (programmes_en_cours) {
        if (!it->espace()->métaprogrammes_en_attente_de_crée_contexte.est_vide()) {
            return false;
        }
    }

    return std::all_of(
        programmes_en_cours.begin(), programmes_en_cours.end(), [](Programme const *it) {
            /* Les programmes des métaprogrammes sont enlevés après leurs exécutions. Si nous en
             * avons un, la compilation ne peut se terminée. */
            if (it->pour_métaprogramme()) {
                return false;
            }

            /* Attend que tous les espaces eurent leur compilation terminée. */
            auto espace = it->espace();
            if (espace->phase_courante() != PhaseCompilation::COMPILATION_TERMINÉE) {
                return false;
            }
            return true;
        });
}

void GestionnaireCode::finalise_programme_avant_génération_code_machine(EspaceDeTravail *espace,
                                                                        Programme *programme)
{
    if (espace->options.résultat == RésultatCompilation::RIEN) {
        espace->change_de_phase(
            m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE, __func__);
        return;
    }

    if (!espace->peut_generer_code_final()) {
        return;
    }

    /* Requiers la génération de RI pour les fonctions ajoute_fini et ajoute_init. */
    auto decl_ajoute_fini = espace->interface_kuri->decl_fini_execution_kuri;
    auto decl_ajoute_init = espace->interface_kuri->decl_init_execution_kuri;
    auto decl_init_globales = espace->interface_kuri->decl_init_globales_kuri;

    auto ri_requise = false;
    if (!decl_ajoute_fini->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        requiers_génération_ri(espace, decl_ajoute_fini);
        ri_requise = true;
    }
    if (!decl_ajoute_init->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        requiers_génération_ri(espace, decl_ajoute_init);
        ri_requise = true;
    }
    if (!decl_init_globales->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        requiers_génération_ri(espace, decl_init_globales);
        ri_requise = true;
    }

    if (ri_requise) {
        return;
    }

    /* Tous les métaprogrammes furent exécutés, et la RI pour les fonctions
     * d'initialisation/finition sont générées : nous pouvons générer le code machine. */
    auto message = espace->change_de_phase(
        m_compilatrice->messagère, PhaseCompilation::AVANT_GÉNÉRATION_OBJET, __func__);

    /* Nous avions déjà créé une unité pour générer le code machine, mais un métaprogramme a sans
     * doute ajouté du code. Il faut annuler l'unité précédente qui peut toujours être dans la file
     * d'attente. */
    if (espace->unité_pour_code_machine) {
        espace->unité_pour_code_machine->définis_état(
            UniteCompilation::État::ANNULÉE_CAR_REMPLACÉE);
        TACHE_TERMINEE(GENERATION_CODE_MACHINE);
    }

    auto unité_code_machine = requiers_génération_code_machine(espace, espace->programme);

    espace->unité_pour_code_machine = unité_code_machine;

    if (message) {
        unité_code_machine->ajoute_attente(Attente::sur_message(nullptr, message));
    }
}

void GestionnaireCode::flush_métaprogrammes_en_attente_de_crée_contexte(EspaceDeTravail *espace)
{
    assert(espace->métaprogrammes_en_attente_de_crée_contexte_est_ouvert);
    POUR (espace->métaprogrammes_en_attente_de_crée_contexte) {
        ajoute_unité_à_liste_attente(it);
    }
    espace->métaprogrammes_en_attente_de_crée_contexte.efface();
    espace->métaprogrammes_en_attente_de_crée_contexte_est_ouvert = false;
}

void GestionnaireCode::interception_message_terminée(EspaceDeTravail *espace)
{
    kuri::tableau<UniteCompilation *> nouvelles_unités;
    nouvelles_unités.réserve(unités_en_attente.taille());

    POUR (unités_en_attente) {
        if (it->espace == espace) {
            it->supprime_attentes_sur_messages();

            if (it->donne_raison_d_être() == RaisonDÊtre::ENVOIE_MESSAGE) {
                continue;
            }
        }

        nouvelles_unités.ajoute(it);
    }

    unités_en_attente = nouvelles_unités;
}

void GestionnaireCode::ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace)
{
    auto programme = espace->programme;
    programme->ajourne_pour_nouvelles_options_espace();
    /* À FAIRE : gère proprement tous les cas. */
    if (espace->options.résultat == RésultatCompilation::RIEN) {
        espace->change_de_phase(
            m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE, __func__);
    }
}

void GestionnaireCode::imprime_stats() const
{
#ifdef STATS_DÉTAILLÉES_GESTION
    stats.imprime_stats();
#endif
}
