/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "gestionnaire_code.hh"

#include <iostream>

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "utilitaires/log.hh"

/*
  À FAIRE(gestion) : pour chaque type :
- avoir un lexème sentinel pour l'impression des erreurs si le noeud est crée lors de la
compilation
 */

#undef STATS_DÉTAILLÉES_GESTION

#ifdef STATISTIQUES_DETAILLEES
#    define STATS_DÉTAILLÉES_GESTION
#endif

#ifdef STATS_DÉTAILLÉES_GESTION
#    define DÉBUTE_STAT(stat) auto chrono_##stat = dls::chrono::compte_milliseconde()
#    define TERMINE_STAT(stat)                                                                    \
        stats.stats.fusionne_entrée(GESTION__##stat, {"", chrono_##stat.temps()})
#else
#    define DÉBUTE_STAT(stat)
#    define TERMINE_STAT(stat)
#endif

#define TACHE_AJOUTEE(genre) espace->tache_ajoutee(GenreTâche::genre, m_compilatrice->messagère)
#define TACHE_TERMINEE(genre, envoyer_changement_de_phase)                                        \
    espace->tache_terminee(                                                                       \
        GenreTâche::genre, m_compilatrice->messagère, envoyer_changement_de_phase)

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
      m_assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", allocatrice_noeud))
{
}

GestionnaireCode::~GestionnaireCode()
{
    memoire::deloge("AssembleuseArbre", m_assembleuse);
}

void GestionnaireCode::espace_créé(EspaceDeTravail *espace)
{
    std::unique_lock verrou(m_mutex);
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

    auto dépendances_manquantes = programme.dépendances_manquantes();
    programme.dépendances_manquantes().efface();

    graphe.prépare_visite();

    dépendances_manquantes.pour_chaque_element([&](NoeudDéclaration *decl) {
        auto noeud_dep = graphe.garantie_noeud_dépendance(espace, decl);

        graphe.traverse(noeud_dep, [&](NoeudDépendance const *relation) {
            if (relation->est_fonction()) {
                programme.ajoute_fonction(relation->fonction());
                données_dépendances.dépendances_épendues.fonctions_utilisées.insère(
                    relation->fonction());
            }
            else if (relation->est_globale()) {
                programme.ajoute_globale(relation->globale());
                données_dépendances.dépendances_épendues.globales_utilisées.insère(
                    relation->globale());
            }
            else if (relation->est_type()) {
                programme.ajoute_type(
                    relation->type(), RaisonAjoutType::DÉPENDACE_INDIRECTE, decl);
                données_dépendances.dépendances_épendues.types_utilisés.insère(relation->type());
            }
        });
    });

    return true;
}

struct RassembleuseDependances {
    DonnéesDépendance &dépendances;
    Compilatrice *compilatrice;
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
        auto interface = compilatrice->interface_kuri;

        if (transformation.type == TypeTransformation::EXTRAIT_UNION) {
            assert(interface->decl_panique_membre_union);
            auto type_union = type->comme_type_union();
            if (!type_union->est_nonsure) {
                ajoute_fonction(interface->decl_panique_membre_union);
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

        /* Nous avons besoin d'un type pointeur pour le type cible pour la génération de
         * RI. À FAIRE: généralise pour toutes les variables. */
        if (transformation.type_cible) {
            auto type_pointeur = compilatrice->typeuse.type_pointeur_pour(
                const_cast<Type *>(transformation.type_cible), false, false);
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
                auto interface = compilatrice->interface_kuri;

                /* Nous ne devrions pas avoir de référence ici, la validation sémantique s'est
                 * chargée de transtyper automatiquement. */
                auto type_indexe = indexage->opérande_gauche->type;
                if (type_indexe->est_type_opaque()) {
                    type_indexe = type_indexe->comme_type_opaque()->type_opacifié;
                }

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
                auto type_ptr = compilatrice->typeuse.type_pointeur_pour(type_feuille);
                ajoute_type(type_ptr);
            }
            else if (noeud->est_tente()) {
                auto tente = noeud->comme_tente();

                if (!tente->expression_piégée) {
                    auto interface = compilatrice->interface_kuri;
                    assert(interface->decl_panique_erreur);
                    ajoute_fonction(interface->decl_panique_erreur);
                }
            }
            else if (noeud->est_référence_membre_union()) {
                auto interface = compilatrice->interface_kuri;
                assert(interface->decl_panique_membre_union);
                ajoute_fonction(interface->decl_panique_membre_union);
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
                    auto type_tfixe = compilatrice->typeuse.type_tableau_fixe(
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

            return DecisionVisiteNoeud::CONTINUE;
        });
}

static void rassemble_dépendances(NoeudExpression *racine,
                                  Compilatrice *compilatrice,
                                  DonnéesDépendance &dépendances)
{
    RassembleuseDependances rassembleuse{dépendances, compilatrice, racine};
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
static void garantie_typage_des_dépendances(GestionnaireCode &gestionnaire,
                                            DonnéesDépendance const &dépendances,
                                            EspaceDeTravail *espace)
{
    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    kuri::pour_chaque_élément(dépendances.fonctions_utilisées, [&](auto &fonction) {
        if (!fonction->corps->unité &&
            !fonction->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE |
                                       DrapeauxNoeudFonction::EST_EXTERNE)) {
            gestionnaire.requiers_typage(espace, fonction->corps);
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_élément(dépendances.globales_utilisées, [&](auto &globale) {
        if (!globale->unité) {
            gestionnaire.requiers_typage(espace, const_cast<NoeudDéclarationVariable *>(globale));
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_élément(dépendances.types_utilisés, [&](auto &type) {
        if (type_requiers_typage(type)) {
            gestionnaire.requiers_typage(espace, type);
        }

        gestionnaire.requiers_initialisation_type(espace, type);

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
                calcule_taille_type_composé(type_retour->comme_type_tuple(), false, 0);
            }
        }

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
                                             GrapheDépendance &graphe)
{
    DÉBUTE_STAT(DÉTERMINE_DÉPENDANCES);
    dépendances.reinitialise();

    DÉBUTE_STAT(RASSEMBLE_DÉPENDANCES);
    rassemble_dépendances(noeud, m_compilatrice, dépendances.dépendances);
    TERMINE_STAT(RASSEMBLE_DÉPENDANCES);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    if (!noeud->est_ajoute_fini() && !noeud->est_ajoute_init()) {
        DÉBUTE_STAT(AJOUTE_DÉPENDANCES);
        NoeudDépendance *noeud_dépendance = graphe.garantie_noeud_dépendance(espace, noeud);
        graphe.ajoute_dépendances(*noeud_dépendance, dépendances.dépendances);
        TERMINE_STAT(AJOUTE_DÉPENDANCES);
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
        if (!doit_ajouter_les_dépendances_au_programme(noeud, it)) {
            continue;
        }
        if (!ajoute_dépendances_au_programme(graphe, dépendances, espace, *it, noeud)) {
            break;
        }
        dépendances_ajoutees = true;
    }

    /* Crée les unités de typage si nécessaire. */
    if (dépendances_ajoutees) {
        DÉBUTE_STAT(GARANTIE_TYPAGE_DÉPENDANCES);
        dépendances.dépendances.fusionne(dépendances.dépendances_épendues);
        garantie_typage_des_dépendances(*this, dépendances.dépendances, espace);
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
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(CHARGEMENT);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::CHARGEMENT_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    std::unique_lock verrou(m_mutex);
    assert(fichier->fut_chargé);
    TACHE_AJOUTEE(LEXAGE);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::LEXAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    std::unique_lock verrou(m_mutex);
    assert(fichier->fut_lexé);
    TACHE_AJOUTEE(PARSAGE);
    auto unité = crée_unité_pour_fichier(espace, fichier, RaisonDÊtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(TYPAGE);
    crée_unité_pour_noeud(espace, noeud, RaisonDÊtre::TYPAGE, true);
}

void GestionnaireCode::requiers_génération_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(GENERATION_RI);
    crée_unité_pour_noeud(espace, noeud, RaisonDÊtre::GENERATION_RI, true);
}

void GestionnaireCode::requiers_génération_ri_principale_métaprogramme(
    EspaceDeTravail *espace, MetaProgramme *metaprogramme, bool peut_planifier_compilation)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(GENERATION_RI);

    auto unité = crée_unité_pour_noeud(espace,
                                       metaprogramme->fonction,
                                       RaisonDÊtre::GENERATION_RI_PRINCIPALE_MP,
                                       peut_planifier_compilation);

    if (!peut_planifier_compilation) {
        assert(metaprogrammes_en_attente_de_crée_contexte_est_ouvert);
        métaprogrammes_en_attente_de_crée_contexte.ajoute(unité);
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
    std::unique_lock verrou(m_mutex);

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

UniteCompilation *GestionnaireCode::requiers_noeud_code(EspaceDeTravail *espace,
                                                        NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    auto unité = crée_unité(espace, RaisonDÊtre::CONVERSION_NOEUD_CODE, true);
    unité->noeud = noeud;
    return unité;
}

void GestionnaireCode::ajoute_unité_à_liste_attente(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::EN_ATTENTE);
    unités_en_attente.ajoute_aux_données_globales(unité);
}

bool GestionnaireCode::tente_de_garantir_présence_création_contexte(EspaceDeTravail *espace,
                                                                    Programme *programme,
                                                                    GrapheDépendance &graphe)
{
    /* NOTE : la déclaration sera automatiquement ajoutée au programme si elle n'existe pas déjà
     * lors de la complétion de son typage. Si elle existe déjà, il faut l'ajouter manuellement.
     */
    auto decl_creation_contexte = m_compilatrice->interface_kuri->decl_creation_contexte;
    assert(decl_creation_contexte);

    // À FAIRE : déplace ceci quand toutes les entêtes seront validées avant le reste.
    programme->ajoute_fonction(decl_creation_contexte);

    if (!decl_creation_contexte->unité) {
        requiers_typage(espace, decl_creation_contexte);
        return false;
    }

    if (!decl_creation_contexte->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    détermine_dépendances(decl_creation_contexte, espace, graphe);

    if (!decl_creation_contexte->corps->unité) {
        requiers_typage(espace, decl_creation_contexte->corps);
        return false;
    }

    if (!decl_creation_contexte->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    détermine_dépendances(decl_creation_contexte->corps, espace, graphe);

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
    m_requêtes_compilations_métaprogrammes->ajoute({espace, metaprogramme});
}

void GestionnaireCode::requiers_compilation_métaprogramme_impl(EspaceDeTravail *espace,
                                                               MetaProgramme *metaprogramme)
{
    /* Ajoute le programme à la liste des programmes avant de traiter les dépendances. */
    métaprogramme_créé(metaprogramme);

    auto programme = metaprogramme->programme;
    programme->ajoute_fonction(metaprogramme->fonction);

    auto graphe = m_compilatrice->graphe_dépendance.verrou_ecriture();
    détermine_dépendances(metaprogramme->fonction, espace, *graphe);
    détermine_dépendances(metaprogramme->fonction->corps, espace, *graphe);

    auto ri_crée_contexte_est_disponible = tente_de_garantir_présence_création_contexte(
        espace, programme, *graphe);
    requiers_génération_ri_principale_métaprogramme(
        espace, metaprogramme, ri_crée_contexte_est_disponible);

    if (metaprogramme->corps_texte) {
        if (metaprogramme->corps_texte_pour_fonction) {
            auto recipiente = metaprogramme->corps_texte_pour_fonction;
            assert(!recipiente->corps->unité);
            requiers_typage(espace, recipiente->corps);

            /* Crée un fichier pour le métaprogramme, et fait dépendre le corps de la fonction
             * recipiente du #corps_texte sur le parsage du fichier. Nous faisons ça ici pour nous
             * assurer que personne n'essayera de performer le typage du corps recipient avant que
             * les sources du fichiers ne soient générées, lexées, et parsées. */
            auto fichier = m_compilatrice->crée_fichier_pour_metaprogramme(metaprogramme);
            recipiente->corps->unité->ajoute_attente(Attente::sur_parsage(fichier));
        }
        else if (metaprogramme->corps_texte_pour_structure) {
            /* Les fichiers pour les #corps_texte des structures sont créés lors de la validation
             * sémantique. */
            assert(metaprogramme->fichier);
        }
    }
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
    std::unique_lock verrou(m_mutex);
    auto unité = crée_unité(espace, RaisonDÊtre::GENERATION_CODE_MACHINE, true);
    unité->programme = programme;
    TACHE_AJOUTEE(GENERATION_CODE_MACHINE);
    return unité;
}

void GestionnaireCode::requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme)
{
    std::unique_lock verrou(m_mutex);
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
        /* Ceci est pour gérer les requêtes de fonctions d'initialisation avant la génération de
         * RI. */
        requiers_initialisation_type(espace, type);
    }
    else if (attente.est<AttenteSurDéclaration>()) {
        NoeudDéclaration *decl = attente.déclaration();
        if (*donne_adresse_unité(decl) == nullptr) {
            requiers_typage(espace, decl);
        }
    }
    else if (attente.est<AttenteSurInitialisationType>()) {
        auto type = const_cast<Type *>(attente.initialisation_type());
        requiers_initialisation_type(espace, type);
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
    mémoire += métaprogrammes_en_attente_de_crée_contexte.taille_mémoire();
    mémoire += programmes_en_cours.taille_mémoire();
    mémoire += m_fonctions_parsées.taille_mémoire();
    mémoire += m_noeuds_à_valider.taille_mémoire();
    mémoire += m_fonctions_init_type_requises.taille_mémoire();

    mémoire += dépendances.dépendances.mémoire_utilisée();
    mémoire += dépendances.dépendances_épendues.mémoire_utilisée();

    POUR_TABLEAU_PAGE (unités) {
        mémoire += it.mémoire_utilisée();
    }

    allocatrice_noeud.rassemble_statistiques(statistiques);

    statistiques.ajoute_mémoire_utilisée("Gestionnaire Code", mémoire);
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unité_attendante, Attente attente)
{
    std::unique_lock verrou(m_mutex_unités_terminées);
    assert(attente.est_valide());
    assert(unité_attendante->est_prête());
    auto espace = unité_attendante->espace;
    m_attentes_à_résoudre.ajoute_aux_données_globales({espace, attente});
    unité_attendante->ajoute_attente(attente);
    ajoute_unité_à_liste_attente(unité_attendante);
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unité_attendante,
                                       kuri::tableau_statique<Attente> attentes)
{
    std::unique_lock verrou(m_mutex_unités_terminées);
    assert(attentes.taille() != 0);
    assert(unité_attendante->est_prête());

    auto espace = unité_attendante->espace;

    POUR (attentes) {
        m_attentes_à_résoudre.ajoute_aux_données_globales({espace, it});
        unité_attendante->ajoute_attente(it);
    }

    ajoute_unité_à_liste_attente(unité_attendante);
}

void GestionnaireCode::tâche_unité_terminée(UniteCompilation *unité)
{
    std::unique_lock<std::mutex> verrou(m_mutex_unités_terminées);
    m_unités_terminées.ajoute_aux_données_globales(unité);
}

void GestionnaireCode::chargement_fichier_terminé(UniteCompilation *unité)
{
    assert(unité->fichier);
    assert(unité->fichier->fut_chargé);

    auto espace = unité->espace;
    TACHE_TERMINEE(CHARGEMENT, true);
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
    TACHE_TERMINEE(LEXAGE, true);

    /* Une fois que nous avons lexer un fichier, il faut le parser. */
    unité->mute_raison_d_être(RaisonDÊtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.déplace_unité_pour_chargement_fichier(unité);
    ajoute_unité_à_liste_attente(unité);
    TACHE_AJOUTEE(PARSAGE);
}

void GestionnaireCode::parsage_fichier_terminé(UniteCompilation *unité)
{
    assert(unité->fichier);
    assert(unité->fichier->fut_parsé);
    auto espace = unité->espace;
    TACHE_TERMINEE(PARSAGE, true);
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
    m_état_chargement_fichiers.supprime_unité_pour_chargement_fichier(unité);

    POUR (unité->fichier->noeuds_à_valider) {
        /* Nous avons sans doute déjà requis le typage de ce noeud. */
        auto adresse_unité = donne_adresse_unité(it);
        if (*adresse_unité) {
            continue;
        }

        if (it->est_charge() || it->est_importe()) {
            requiers_typage(espace, it);
            m_état_chargement_fichiers.ajoute_unité_pour_charge_ou_importe(*adresse_unité);
        }
        else {
            m_noeuds_à_valider.ajoute({espace, it});
        }
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
               entete->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
               !entete->est_opérateur_pour();
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entête;

        /* Puisque les métaprogrammes peuvent ajouter des chaines à la compilation, nous devons
         * attendre la génération de code final avant de générer la RI pour ces fonctions. */
        if (dls::outils::est_element(entete->ident,
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
               (!entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) ||
                entete->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION));
    }

    if (noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE) && !noeud->est_type_structure() &&
        !noeud->est_type_énum() && !noeud->est_type_union() &&
        !noeud->est_déclaration_bibliothèque() && !noeud->est_déclaration_constante()) {
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
                 noeud->est_déclaration_bibliothèque() || noeud->est_déclaration_constante());
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

    if (noeud->est_pré_exécutable()) {
        return true;
    }

    return false;
}

static bool declaration_est_invalide(NoeudExpression *decl)
{
    if (decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    auto adresse_unité = donne_adresse_unité(decl);
    if (!adresse_unité) {
        return true;
    }
    auto unité = *adresse_unité;
    if (!unité) {
        /* Pas encore d'unité, nous ne pouvons savoir si la déclaration est valide. */
        return true;
    }

    if (unité->espace->possède_erreur) {
        /* Si l'espace responsable de l'unité de l'entête possède une erreur, nous devons
         * ignorer les entêtes invalides, car sinon la compilation serait infinie. */
        return false;
    }

    return true;
}

static bool verifie_que_toutes_les_entetes_sont_validees(SystèmeModule &sys_module)
{
    POUR_TABLEAU_PAGE (sys_module.modules) {
        /* Il est possible d'avoir un module vide. */
        if (it.fichiers.est_vide()) {
            continue;
        }

        if (it.bloc == nullptr) {
            return false;
        }

        if (it.fichiers_sont_sales) {
            for (auto fichier : it.fichiers) {
                if (!fichier->fut_chargé) {
                    return false;
                }

                if (!fichier->fut_parsé) {
                    return false;
                }
            }
            it.fichiers_sont_sales = false;
        }

        if (it.bloc->membres_sont_sales) {
            for (auto decl : (*it.bloc->membres.verrou_lecture())) {
                if (decl->est_entête_fonction() && declaration_est_invalide(decl)) {
                    return false;
                }
            }
            it.bloc->membres_sont_sales = false;
        }

        if (it.bloc->expressions_sont_sales) {
            for (auto decl : (*it.bloc->expressions.verrou_lecture())) {
                if (decl->est_importe() && declaration_est_invalide(decl)) {
                    return false;
                }
            }
            it.bloc->expressions_sont_sales = false;
        }
    }

    return true;
}

void GestionnaireCode::typage_terminé(UniteCompilation *unité)
{
    DÉBUTE_STAT(TYPAGE_TERMINÉ);
    assert(unité->noeud);
    assert_rappel(unité->noeud->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE), [&] {
        dbg() << "Le noeud de genre " << unité->noeud->genre << " ne fut pas validé !\n"
              << erreur::imprime_site(*unité->espace, unité->noeud);
    });

    auto espace = unité->espace;

    if (unité->noeud->est_charge() || unité->noeud->est_importe()) {
        m_état_chargement_fichiers.supprime_unité_pour_charge_ou_importe(unité);

        if (tous_les_fichiers_à_parser_le_sont()) {
            flush_noeuds_à_typer();
        }
    }

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = m_compilatrice->graphe_dépendance.verrou_ecriture();
    auto noeud = unité->noeud;
    DÉBUTE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    auto const détermine_les_dépendances = doit_déterminer_les_dépendances(unité->noeud);
    TERMINE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    if (détermine_les_dépendances) {
        détermine_dépendances(unité->noeud, unité->espace, *graphe);
    }

    /* Envoi un message, nous attendrons dessus si nécessaire. */
    const auto message = m_compilatrice->messagère->ajoute_message_typage_code(espace, noeud);
    const auto doit_envoyer_en_ri = noeud_requiers_generation_ri(noeud);
    if (doit_envoyer_en_ri) {
        TACHE_AJOUTEE(GENERATION_RI);
        unité->mute_raison_d_être(RaisonDÊtre::GENERATION_RI);
        ajoute_unité_à_liste_attente(unité);
    }

    if (message) {
        auto unité_noeud_code = requiers_noeud_code(espace, noeud);
        auto unité_message = crée_unité_pour_message(espace, message);
        unité_message->ajoute_attente(Attente::sur_noeud_code(unité_noeud_code, noeud));
        unité->ajoute_attente(Attente::sur_message(unité_message, message));
    }

    DÉBUTE_STAT(VÉRIFIE_ENTÊTE_VALIDÉES);
    auto peut_envoyer_changement_de_phase = verifie_que_toutes_les_entetes_sont_validees(
        *m_compilatrice->sys_module.verrou_ecriture());
    TERMINE_STAT(VÉRIFIE_ENTÊTE_VALIDÉES);

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    TACHE_TERMINEE(TYPAGE, peut_envoyer_changement_de_phase);

    if (noeud->est_entête_fonction()) {
        m_fonctions_parsées.ajoute(noeud->comme_entête_fonction());
    }
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
    TACHE_TERMINEE(GENERATION_RI, true);
    if (espace->optimisations) {
        // À FAIRE(gestion) : tâches d'optimisations
    }

    /* Si nous avons la RI pour #crée_contexte, il nout faut ajouter toutes les unités l'attendant.
     */
    if (est_corps_de(unité->noeud,
                     espace->compilatrice().interface_kuri->decl_creation_contexte)) {
        flush_métaprogrammes_en_attente_de_crée_contexte();
    }

    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::optimisation_terminée(UniteCompilation *unité)
{
    assert(unité->noeud);
    auto espace = unité->espace;
    TACHE_TERMINEE(OPTIMISATION, true);
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
    TACHE_TERMINEE(EXECUTION, true);
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
        TACHE_TERMINEE(GENERATION_CODE_MACHINE, true);

        if (programme_requiers_liaison_exécutable(espace->options)) {
            espace->change_de_phase(m_compilatrice->messagère,
                                    PhaseCompilation::AVANT_LIAISON_EXÉCUTABLE);
            requiers_liaison_executable(espace, unité->programme);
        }
        else {
            espace->change_de_phase(m_compilatrice->messagère,
                                    PhaseCompilation::COMPILATION_TERMINÉE);
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
        TACHE_TERMINEE(LIAISON_PROGRAMME, true);
        espace->change_de_phase(m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE);
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
        if (it->possède(unité->type)) {
            it->ajoute_fonction(fonction);
        }
    }

    auto graphe = m_compilatrice->graphe_dépendance.verrou_ecriture();
    détermine_dépendances(fonction, unité->espace, *graphe);
    détermine_dépendances(fonction->corps, unité->espace, *graphe);

    unité->mute_raison_d_être(RaisonDÊtre::GENERATION_RI);
    auto espace = unité->espace;
    TACHE_AJOUTEE(GENERATION_RI);
    unité->noeud = fonction;
    fonction->unité = unité;
    ajoute_unité_à_liste_attente(unité);
}

void GestionnaireCode::crée_tâches_pour_ordonnanceuse()
{
    DÉBUTE_STAT(CRÉATION_TÂCHES);
    unités_en_attente.permute_données_globales_et_locales();
    unités_prêtes.efface();
    unités_à_remettre_en_attente.efface();

#undef DEBUG_UNITES_EN_ATTENTES

#ifdef DEBUG_UNITES_EN_ATTENTES
    if (imprime_débogage) {
        std::cerr << "Unités en attente avant la création des tâches : "
                  << unités_en_attente.taille() << '\n';
    }
#endif

    if (imprime_débogage) {
        //        if (unités_en_attente.taille() == 1) {
        //            auto it = unités_en_attente[0];
        //            std::cerr << it->raison_d_etre() << '\n';
        //            std::cerr << it->chaine_attentes_recursives() << '\n';
        //        }
        // std::cerr << "-------------------- " << __func__ << '\n';
    }
    POUR (unités_en_attente.donne_données_locales()) {
        if (it->espace->possède_erreur) {
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

        if (imprime_débogage &&
            état_attente != UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES) {
            //                std::cerr << "-------------------\n";
            //                std::cerr << it->raison_d_etre() << '\n';
            //                std::cerr << it->chaine_attentes_recursives() << '\n';
            //                if (it->raison_d_etre() == RaisonDÊtre::TYPAGE) {
            //                    if (it->noeud && it->noeud->ident == ID::principale) {
            //                        std::cerr << "Typage de la fonction principale attend sur
            //                        "
            //                                  << it->chaine_attentes_recursives() << '\n';
            //                    }
            //                }
        }

        switch (état_attente) {
            case UniteCompilation::ÉtatAttentes::ATTENTES_BLOQUÉES:
            {
                it->rapporte_erreur();
                unités_en_attente.efface_tout();
                m_compilatrice->ordonnanceuse->supprime_toutes_les_tâches();
                return;
            }
            case UniteCompilation::ÉtatAttentes::ATTENTES_NON_RÉSOLUES:
            {
                it->cycle += 1;
                unités_à_remettre_en_attente.ajoute(it);
                break;
            }
            case UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES:
            case UniteCompilation::ÉtatAttentes::UN_SYMBOLE_EST_ATTENDU:
            {
                unités_prêtes.ajoute(it);
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

    auto ordonnanceuse = m_compilatrice->ordonnanceuse.verrou_ecriture();
#ifdef DEBUG_UNITES_EN_ATTENTES
    if (imprime_débogage) {
        ordonnanceuse->imprime_donnees_files(std::cerr);
    }
#endif
    POUR (unités_prêtes) {
        it->définis_état(UniteCompilation::État::DONNÉE_À_ORDONNANCEUSE);
        ordonnanceuse->crée_tache_pour_unite(it);
    }

    pour_chaque_élément(espaces_errones, [&](EspaceDeTravail *espace) {
        ordonnanceuse->supprime_toutes_les_tâches_pour_espace(
            espace, UniteCompilation::État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR);
        return kuri::DécisionItération::Continue;
    });

    unités_en_attente.ajoute_aux_données_globales(unités_à_remettre_en_attente);

    TERMINE_STAT(CRÉATION_TÂCHES);

#ifdef DEBUG_UNITES_EN_ATTENTES
    if (imprime_débogage) {
        std::cerr << "Unités en attente après la création des tâches : "
                  << unités_en_attente->taille() << '\n';
        ordonnanceuse->imprime_donnees_files(std::cerr);
        std::cerr << "--------------------------------------------------------\n";
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
                m_compilatrice->messagère->purge_messages();
            }

            espace->change_de_phase(m_compilatrice->messagère,
                                    PhaseCompilation::COMPILATION_TERMINÉE);

            if (!espace->options.continue_si_erreur) {
                return true;
            }

            espace_errone_existe = true;
            continue;
        }

        tente_de_garantir_fonction_point_d_entrée(espace);

        if (it->pour_métaprogramme()) {
            auto etat = it->ajourne_état_compilation();

            if (etat.phase_courante() == PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE) {
                it->change_de_phase(PhaseCompilation::AVANT_GÉNÉRATION_OBJET);
                requiers_génération_code_machine(espace, it);
            }
        }
        else {
            //            if (imprime_débogage) {
            //                imprime_diagnostique(it->diagnositique_compilation());
            //                std::cerr << espace->phase_courante() << '\n';
            //            }

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

    if (m_unités_terminées.possède_élément_dans_données_globales()) {
        return false;
    }

    if (unités_en_attente.possède_élément_dans_données_globales() ||
        !métaprogrammes_en_attente_de_crée_contexte.est_vide()) {
        return false;
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

void GestionnaireCode::tente_de_garantir_fonction_point_d_entrée(EspaceDeTravail *espace)
{
    auto copie_et_valide_point_d_entree = [&](NoeudDéclarationEntêteFonction *point_d_entree) {
        auto copie = copie_noeud(m_assembleuse,
                                 point_d_entree,
                                 point_d_entree->bloc_parent,
                                 OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION);
        copie->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
        copie->comme_entête_fonction()->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
        copie->comme_entête_fonction()->corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        requiers_typage(espace, copie);
        return copie->comme_entête_fonction();
    };

    // Ne compile le point d'entrée que pour les exécutables
    if (espace->options.résultat == RésultatCompilation::EXÉCUTABLE) {
        if (espace->fonction_point_d_entree != nullptr) {
            return;
        }

        auto point_d_entree = m_compilatrice->fonction_point_d_entree;
        assert(point_d_entree);
        espace->fonction_point_d_entree = copie_et_valide_point_d_entree(point_d_entree);
    }
    else if (espace->options.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE) {
        if (espace->fonction_point_d_entree_dynamique == nullptr) {
            auto point_d_entree = m_compilatrice->fonction_point_d_entree_dynamique;
            assert(point_d_entree);
            espace->fonction_point_d_entree_dynamique = copie_et_valide_point_d_entree(
                point_d_entree);
        }
        if (espace->fonction_point_de_sortie_dynamique == nullptr) {
            auto point_d_entree = m_compilatrice->fonction_point_de_sortie_dynamique;
            assert(point_d_entree);
            espace->fonction_point_de_sortie_dynamique = copie_et_valide_point_d_entree(
                point_d_entree);
        }
    }
}

void GestionnaireCode::finalise_programme_avant_génération_code_machine(EspaceDeTravail *espace,
                                                                        Programme *programme)
{
    if (espace->options.résultat == RésultatCompilation::RIEN) {
        espace->change_de_phase(m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE);
        return;
    }

    if (!espace->peut_generer_code_final()) {
        return;
    }

    auto modules = programme->modules_utilisés();
    auto executions_requises = false;
    auto executions_en_cours = false;
    modules.pour_chaque_element([&](Module *module) {
        auto exécute = module->directive_pré_exécutable;
        if (!exécute) {
            return;
        }

        if (!module->exécution_directive_requise) {
            /* L'espace du programme est celui qui a créé le métaprogramme lors de la validation de
             * code, mais nous devons avoir le métaprogramme (qui hérite de l'espace du programme)
             * dans l'espace demandant son exécution afin que le compte de tâches d'exécution dans
             * l'espace soit cohérent. */
            exécute->métaprogramme->programme->change_d_espace(espace);
            requiers_compilation_métaprogramme(espace, exécute->métaprogramme);
            module->exécution_directive_requise = true;
            executions_requises = true;
        }

        /* Nous devons attendre la fin de l'exécution de ces métaprogrammes avant de pouvoir généré
         * le code machine. */
        executions_en_cours |= !exécute->métaprogramme->fut_exécuté();
    });

    if (executions_requises || executions_en_cours) {
        return;
    }

    /* Requiers la génération de RI pour les fonctions ajoute_fini et ajoute_init. */
    auto decl_ajoute_fini = m_compilatrice->interface_kuri->decl_fini_execution_kuri;
    auto decl_ajoute_init = m_compilatrice->interface_kuri->decl_init_execution_kuri;
    auto decl_init_globales = m_compilatrice->interface_kuri->decl_init_globales_kuri;

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
    auto message = espace->change_de_phase(m_compilatrice->messagère,
                                           PhaseCompilation::AVANT_GÉNÉRATION_OBJET);

    /* Nous avions déjà créé une unité pour générer le code machine, mais un métaprogramme a sans
     * doute ajouté du code. Il faut annuler l'unité précédente qui peut toujours être dans la file
     * d'attente. */
    if (espace->unité_pour_code_machine) {
        espace->unité_pour_code_machine->définis_état(
            UniteCompilation::État::ANNULÉE_CAR_REMPLACÉE);
        TACHE_TERMINEE(GENERATION_CODE_MACHINE, true);
    }

    auto unité_code_machine = requiers_génération_code_machine(espace, espace->programme);

    espace->unité_pour_code_machine = unité_code_machine;

    if (message) {
        unité_code_machine->ajoute_attente(Attente::sur_message(nullptr, message));
    }
}

void GestionnaireCode::flush_métaprogrammes_en_attente_de_crée_contexte()
{
    assert(metaprogrammes_en_attente_de_crée_contexte_est_ouvert);
    POUR (métaprogrammes_en_attente_de_crée_contexte) {
        ajoute_unité_à_liste_attente(it);
    }
    métaprogrammes_en_attente_de_crée_contexte.efface();
    metaprogrammes_en_attente_de_crée_contexte_est_ouvert = false;
}

void GestionnaireCode::interception_message_terminée(EspaceDeTravail *espace)
{
    m_compilatrice->messagère->termine_interception(espace);
}

void GestionnaireCode::ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace)
{
    std::unique_lock verrou(m_mutex);
    auto programme = espace->programme;
    programme->ajourne_pour_nouvelles_options_espace();
    /* À FAIRE : gère proprement tous les cas. */
    if (espace->options.résultat == RésultatCompilation::RIEN) {
        espace->change_de_phase(m_compilatrice->messagère, PhaseCompilation::COMPILATION_TERMINÉE);
    }
}

void GestionnaireCode::imprime_stats() const
{
#ifdef STATS_DÉTAILLÉES_GESTION
    stats.imprime_stats();
#endif
}

void GestionnaireCode::démarre_boucle_compilation()
{
    while (true) {
#if 0
        auto delta = temps_début_compilation.temps();
        imprime_débogage = delta >= 0.1;
        if (imprime_débogage) {
            temps_début_compilation = dls::chrono::compte_seconde();
        }
#endif

        if (plus_rien_n_est_à_faire()) {
            auto ordonnanceuse = m_compilatrice->ordonnanceuse.verrou_ecriture();
            ordonnanceuse->marque_compilation_terminee();
            break;
        }

        ajoute_types_dans_graphe();
        gère_requête_compilations_métaprogrammes();
        gère_choses_terminées();
        crée_tâches_pour_ordonnanceuse();

#if 0
        imprime_débogage = false;
#endif
    }
}

void GestionnaireCode::gère_choses_terminées()
{
    {
        std::unique_lock verrou(m_mutex_unités_terminées);
        m_attentes_à_résoudre.permute_données_globales_et_locales();
        m_unités_terminées.permute_données_globales_et_locales();
    }

    POUR (m_attentes_à_résoudre.donne_données_locales()) {
        ajoute_requêtes_pour_attente(it.espace, it.attente);
    }

    // std::cerr << "unités terminées : " << m_unités_terminées.taille() << '\n';

    POUR (m_unités_terminées.donne_données_locales()) {
        switch (it->donne_raison_d_être()) {
            case RaisonDÊtre::AUCUNE:
            {
                // erreur ?
                break;
            }
            case RaisonDÊtre::CHARGEMENT_FICHIER:
            {
                chargement_fichier_terminé(it);
                break;
            }
            case RaisonDÊtre::LEXAGE_FICHIER:
            {
                lexage_fichier_terminé(it);
                break;
            }
            case RaisonDÊtre::PARSAGE_FICHIER:
            {
                parsage_fichier_terminé(it);
                break;
            }
            case RaisonDÊtre::CREATION_FONCTION_INIT_TYPE:
            {
                fonction_initialisation_type_créée(it);
                break;
            }
            case RaisonDÊtre::TYPAGE:
            {
                typage_terminé(it);
                break;
            }
            case RaisonDÊtre::CONVERSION_NOEUD_CODE:
            {
                conversion_noeud_code_terminée(it);
                break;
            }
            case RaisonDÊtre::ENVOIE_MESSAGE:
            {
                envoi_message_terminé(it);
                break;
            }
            case RaisonDÊtre::GENERATION_RI:
            {
                generation_ri_terminée(it);
                break;
            }
            case RaisonDÊtre::GENERATION_RI_PRINCIPALE_MP:
            {
                generation_ri_terminée(it);
                break;
            }
            case RaisonDÊtre::EXECUTION:
            {
                execution_terminée(it);
                break;
            }
            case RaisonDÊtre::LIAISON_PROGRAMME:
            {
                liaison_programme_terminée(it);
                break;
            }
            case RaisonDÊtre::GENERATION_CODE_MACHINE:
            {
                generation_code_machine_terminée(it);
                break;
            }
        }
    }
}

void GestionnaireCode::ajoute_types_dans_graphe()
{
    auto &typeuse = m_compilatrice->typeuse;
    typeuse.types_à_insérer_dans_graphe.permute_données_globales_et_locales();
    auto types_à_insérer_dans_graphe = typeuse.types_à_insérer_dans_graphe.donne_données_locales();
    auto graphe = m_compilatrice->graphe_dépendance.verrou_ecriture();

    POUR (types_à_insérer_dans_graphe) {
        graphe->connecte_type_type(it.type_parent, it.type_enfant);
    }
}

void GestionnaireCode::gère_requête_compilations_métaprogrammes()
{
    auto requêtes_compilations_métaprogrammes =
        m_requêtes_compilations_métaprogrammes.verrou_ecriture();

    POUR (*requêtes_compilations_métaprogrammes) {
        requiers_compilation_métaprogramme_impl(it.espace, it.métaprogramme);
    }

    requêtes_compilations_métaprogrammes->efface();
}
