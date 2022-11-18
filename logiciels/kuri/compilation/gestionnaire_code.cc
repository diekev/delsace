/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "gestionnaire_code.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "programme.hh"

/*
  À FAIRE(gestion) : pour chaque type :
- création d'une déclaration de type (ajout d'un noeud syntaxique: NoeudDéclarationType, d'où
dérivront les structures et les énums) (TACHE_CREATION_DECLARATION_TYPE)
- avoir un lexème sentinel pour l'impression des erreurs si le noeud est crée lors de la
compilation
 */

GestionnaireCode::GestionnaireCode(Compilatrice *compilatrice)
    : m_compilatrice(compilatrice),
      m_assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", allocatrice_noeud))
{
}

GestionnaireCode::~GestionnaireCode()
{
    memoire::deloge("AssembleuseArbre", m_assembleuse);
}

void GestionnaireCode::espace_cree(EspaceDeTravail *espace)
{
    assert(espace->programme);
    ajoute_programme(espace->programme);
}

void GestionnaireCode::metaprogramme_cree(MetaProgramme *metaprogramme)
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
    if (!noeud->est_declaration_variable()) {
        return false;
    }

    if (noeud->possede_drapeau(EST_CONSTANTE)) {
        return false;
    }

    return noeud->possede_drapeau(EST_GLOBALE);
}

static bool ajoute_dependances_au_programme(DonneesDependance const &dependances,
                                            EspaceDeTravail *espace,
                                            Programme &programme)
{
    auto possede_erreur = false;

    /* Ajoute les fonctions. */
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        if (fonction->possede_drapeau(COMPILATRICE) && !programme.pour_metaprogramme()) {
            possede_erreur = true;

            /* À FAIRE : site pour la dépendance. */
            espace->rapporte_erreur(fonction,
                                    "Utilisation d'une fonction d'interface de la compilatrice "
                                    "dans un programme final. Cette fonction ne peut qu'être "
                                    "utilisée dans un métaprogramme.");
            return kuri::DecisionIteration::Arrete;
        }

        programme.ajoute_fonction(const_cast<NoeudDeclarationEnteteFonction *>(fonction));
        return kuri::DecisionIteration::Continue;
    });

    if (possede_erreur) {
        return false;
    }

    /* Ajoute les globales. */
    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        programme.ajoute_globale(const_cast<NoeudDeclarationVariable *>(globale));
        return kuri::DecisionIteration::Continue;
    });

    /* Ajoute les types. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        programme.ajoute_type(type);
        return kuri::DecisionIteration::Continue;
    });

    return true;
}

struct RassembleuseDependances {
    DonneesDependance &dependances;
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

        dependances.types_utilises.insere(type);
    }

    void ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
    {
        dependances.fonctions_utilisees.insere(fonction);
    }

    void ajoute_globale(NoeudDeclarationVariable *globale)
    {
        dependances.globales_utilisees.insere(globale);
    }

    void rassemble_dependances()
    {
        rassemble_dependances(racine_);
    }

    void rassemble_dependances(NoeudExpression *racine);
};

/* Traverse l'arbre syntaxique de la racine spécifiée et rassemble les fonctions, types, et
 * globales utilisées. */
void RassembleuseDependances::rassemble_dependances(NoeudExpression *racine)
{
    auto rassemble_dependances_transformation = [&](TransformationType transformation,
                                                    Type *type) {
        /* Marque les dépendances sur les fonctions d'interface de kuri. */
        auto interface = compilatrice->interface_kuri;

        if (transformation.type == TypeTransformation::EXTRAIT_UNION) {
            assert(interface->decl_panique_membre_union);
            auto type_union = type->comme_union();
            if (!type_union->est_nonsure) {
                ajoute_fonction(interface->decl_panique_membre_union);
            }
        }
        else if (transformation.type == TypeTransformation::FONCTION) {
            assert(transformation.fonction);
            ajoute_fonction(const_cast<NoeudDeclarationEnteteFonction *>(transformation.fonction));
        }

        /* Nous avons besoin d'un type pointeur pour le type cible pour la génération de
         * RI. À FAIRE: généralise pour toutes les variables. */
        if (transformation.type_cible) {
            auto type_pointeur = compilatrice->typeuse.type_pointeur_pour(
                transformation.type_cible, false, false);
            ajoute_type(type_pointeur);
            ajoute_type(transformation.type_cible);
        }
    };

    auto rassemble_dependances_transformations =
        [&](kuri::tableau_compresse<TransformationType, int> const &transformations, Type *type) {
            POUR (transformations.plage()) {
                rassemble_dependances_transformation(it, type);
            }
        };

    visite_noeud(
        racine,
        PreferenceVisiteNoeud::SUBSTITUTION,
        [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
            // Note: les fonctions polymorphiques n'ont pas de types.
            if (noeud->type) {
                if (noeud->type->est_type_de_donnees()) {
                    auto type_de_donnees = noeud->type->comme_type_de_donnees();
                    if (type_de_donnees->type_connu) {
                        ajoute_type(type_de_donnees->type_connu);
                    }
                }
                else {
                    ajoute_type(noeud->type);
                }
            }

            if (noeud->est_entete_fonction()) {
                /* Visite manuellement les enfants des entêtes, car nous irions visiter le corps
                 * qui ne fut pas encore typé. */
                auto entete = noeud->comme_entete_fonction();

                POUR (entete->params) {
                    rassemble_dependances(it);
                }

                POUR (entete->params_sorties) {
                    rassemble_dependances(it);
                }

                return DecisionVisiteNoeud::IGNORE_ENFANTS;
            }

            if (noeud->est_reference_declaration()) {
                auto ref = noeud->comme_reference_declaration();

                auto decl = ref->declaration_referee;

                if (!decl) {
                    return DecisionVisiteNoeud::CONTINUE;
                }

                if (est_declaration_variable_globale(decl)) {
                    ajoute_globale(decl->comme_declaration_variable());
                }
                else if (decl->est_entete_fonction() &&
                         !decl->comme_entete_fonction()->est_polymorphe) {
                    auto decl_fonc = decl->comme_entete_fonction();
                    ajoute_fonction(decl_fonc);
                }
            }
            else if (noeud->est_cuisine()) {
                auto cuisine = noeud->comme_cuisine();
                auto expr = cuisine->expression;
                ajoute_fonction(expr->comme_appel()->expression->comme_entete_fonction());
            }
            else if (noeud->est_indexage()) {
                /* Traite les indexages avant les expressions binaires afin de ne pas les traiter
                 * comme tel (est_expression_binaire() retourne vrai pour les indexages). */

                auto indexage = noeud->comme_indexage();
                /* op peut être nul pour les déclaration de type ([]z32) */
                if (indexage->op && !indexage->op->est_basique) {
                    ajoute_fonction(indexage->op->decl);
                }

                /* Marque les dépendances sur les fonctions d'interface de kuri. */
                auto interface = compilatrice->interface_kuri;

                /* Nous ne devrions pas avoir de référence ici, la validation sémantique s'est
                 * chargée de transtyper automatiquement. */
                auto type_indexe = indexage->operande_gauche->type;
                if (type_indexe->est_opaque()) {
                    type_indexe = type_indexe->comme_opaque()->type_opacifie;
                }

                switch (type_indexe->genre) {
                    case GenreType::VARIADIQUE:
                    case GenreType::TABLEAU_DYNAMIQUE:
                    {
                        assert(interface->decl_panique_tableau);
                        ajoute_fonction(interface->decl_panique_tableau);
                        break;
                    }
                    case GenreType::TABLEAU_FIXE:
                    {
                        assert(interface->decl_panique_tableau);
                        if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
                            ajoute_fonction(interface->decl_panique_tableau);
                        }
                        break;
                    }
                    case GenreType::CHAINE:
                    {
                        assert(interface->decl_panique_chaine);
                        if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
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
                auto type_feuille = construction_tableau->type->comme_tableau_fixe()->type_pointe;
                auto type_ptr = compilatrice->typeuse.type_pointeur_pour(type_feuille);
                ajoute_type(type_ptr);
            }
            else if (noeud->est_tente()) {
                auto tente = noeud->comme_tente();

                if (!tente->expression_piegee) {
                    auto interface = compilatrice->interface_kuri;
                    assert(interface->decl_panique_erreur);
                    ajoute_fonction(interface->decl_panique_erreur);
                }
            }
            else if (noeud->est_reference_membre_union()) {
                auto interface = compilatrice->interface_kuri;
                assert(interface->decl_panique_membre_union);
                ajoute_fonction(interface->decl_panique_membre_union);
            }
            else if (noeud->est_comme()) {
                auto comme = noeud->comme_comme();
                rassemble_dependances_transformation(comme->transformation,
                                                     comme->expression->type);
            }
            else if (noeud->est_appel()) {
                auto appel = noeud->comme_appel();
                auto appelee = appel->noeud_fonction_appelee;

                if (appelee) {
                    if (appelee->est_entete_fonction()) {
                        ajoute_fonction(appelee->comme_entete_fonction());
                    }
                    else if (appelee->est_structure()) {
                        ajoute_type(appelee->type);
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

                POUR (assignation->donnees_exprs.plage()) {
                    rassemble_dependances(it.expression);
                    auto type_expression = it.expression ? it.expression->type : Type::nul();
                    rassemble_dependances_transformations(it.transformations, type_expression);
                }
            }
            else if (noeud->est_declaration_variable()) {
                auto declaration = noeud->comme_declaration_variable();

                POUR (declaration->donnees_decl.plage()) {
                    for (auto &var : it.variables.plage()) {
                        ajoute_type(var->type);
                    }
                    rassemble_dependances(it.expression);
                    auto type_expression = it.expression ? it.expression->type : Type::nul();
                    rassemble_dependances_transformations(it.transformations, type_expression);
                }
            }

            return DecisionVisiteNoeud::CONTINUE;
        });
}

static void rassemble_dependances(NoeudExpression *racine,
                                  Compilatrice *compilatrice,
                                  DonneesDependance &dependances)
{
    RassembleuseDependances rassembleuse{dependances, compilatrice, racine};
    rassembleuse.rassemble_dependances();
}

/* Crée un noeud de dépendance pour le noeud spécifié en paramètre, et retourne un pointeur vers
 * celui-ci. Retourne nul si le noeud n'est pas supposé avoir un noeud de dépendance. */
static NoeudDependance *garantie_noeud_dependance(EspaceDeTravail *espace,
                                                  NoeudExpression *noeud,
                                                  GrapheDependance &graphe)
{
    /* N'utilise pas est_declaration_variable_globale car nous voulons également les opaques et les
     * constantes. */
    if (noeud->est_declaration_variable()) {
        assert_rappel(noeud->possede_drapeau(EST_GLOBALE), [&]() {
            erreur::imprime_site(*espace, noeud);
            std::cerr << *noeud;
        });
        return graphe.cree_noeud_globale(noeud->comme_declaration_variable());
    }

    if (noeud->est_entete_fonction()) {
        return graphe.cree_noeud_fonction(noeud->comme_entete_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        return graphe.cree_noeud_fonction(corps->entete);
    }

    if (noeud->est_execute()) {
        auto execute = noeud->comme_execute();
        assert(execute->metaprogramme);
        auto metaprogramme = execute->metaprogramme;
        assert(metaprogramme->fonction);
        return graphe.cree_noeud_fonction(metaprogramme->fonction);
    }

    if (noeud->est_declaration_type()) {
        return graphe.cree_noeud_type(noeud->type);
    }

    assert(!"Noeud non géré pour les dépendances !\n");
    return nullptr;
}

/* Requiers le typage de toutes les dépendances. */
static void garantie_typage_des_dependances(GestionnaireCode &gestionnaire,
                                            DonneesDependance const &dependances,
                                            EspaceDeTravail *espace)
{
    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        if (!fonction->corps->unite && !fonction->est_externe) {
            gestionnaire.requiers_typage(espace, fonction->corps);
        }
        return kuri::DecisionIteration::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        if (!globale->unite) {
            gestionnaire.requiers_typage(espace, const_cast<NoeudDeclarationVariable *>(globale));
        }
        return kuri::DecisionIteration::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        auto decl = decl_pour_type(type);
        if (decl && !decl->unite) {
            // Inutile de typer les unions anonymes, ceci fut fait lors de la validation
            // sémantique.
            if (!(type->est_union() && type->comme_union()->est_anonyme)) {
                gestionnaire.requiers_typage(espace, decl);
            }
        }

        if ((type->drapeaux & INITIALISATION_TYPE_FUT_CREEE) == 0) {
            gestionnaire.requiers_initialisation_type(espace, type);
        }

        return kuri::DecisionIteration::Continue;
    });
}

/* Traverse le graphe de dépendances pour chaque type présents dans les dépendances courantes, et
 * ajoutes les dépedances de ces types aux dépendances.
 * Le but de cette fonction est de s'assurer que toutes les dépendances des types sont ajoutées aux
 * programmes. */
static bool epends_dependances_types(GrapheDependance &graphe,
                                     DonnneesResolutionDependances &donnees_resolution,
                                     Programme *programme)
{
    auto &dependances = donnees_resolution.dependances;
    auto programme_possede_deja_les_types = true;
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        if (!programme->possede(type)) {
            programme_possede_deja_les_types = false;
            return kuri::DecisionIteration::Arrete;
        }

        return kuri::DecisionIteration::Continue;
    });

    if (programme_possede_deja_les_types) {
        /* Le programme possède déjà tous les types, ce qui veut dire que leurs dépendances furent
         * déjà ajoutées au programme. */
        return false;
    }

    auto &dependances_ependues = donnees_resolution.dependances_ependues;

    /* Traverse le graphe pour chaque dépendance sur un type. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        dependances_ependues.types_utilises.insere(type);

        auto noeud_dependance = graphe.cree_noeud_type(type);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_type()) {
                dependances_ependues.types_utilises.insere(relation->type());
            }
        });

        return kuri::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        dependances_ependues.fonctions_utilisees.insere(fonction);
        dependances_ependues.types_utilises.insere(fonction->type);

        auto noeud_dependance = graphe.cree_noeud_fonction(fonction);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_fonction()) {
                dependances_ependues.fonctions_utilisees.insere(relation->fonction());
            }
            else if (relation->est_globale()) {
                dependances_ependues.globales_utilisees.insere(relation->globale());
            }
            else if (relation->est_type()) {
                dependances_ependues.types_utilises.insere(relation->type());
            }
        });

        return kuri::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        dependances_ependues.globales_utilisees.insere(globale);
        dependances_ependues.types_utilises.insere(globale->type);

        auto noeud_dependance = graphe.cree_noeud_globale(globale);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_fonction()) {
                dependances_ependues.fonctions_utilisees.insere(relation->fonction());
            }
            else if (relation->est_globale()) {
                dependances_ependues.globales_utilisees.insere(relation->globale());
            }
            else if (relation->est_type()) {
                dependances_ependues.types_utilises.insere(relation->type());
            }
        });

        return kuri::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    dependances.fusionne(dependances_ependues);
    return true;
}

/* Détermine si nous devons ajouter les dépendances du noeud au programme. */
static bool doit_ajouter_les_dependances_au_programme(NoeudExpression *noeud, Programme *programme)
{
    if (noeud->est_entete_fonction()) {
        return programme->possede(noeud->comme_entete_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entete;
        return programme->possede(entete);
    }

    if (noeud->est_declaration_variable()) {
        return programme->possede(noeud->comme_declaration_variable());
    }

    if (noeud->est_structure() || noeud->est_enum()) {
        return programme->possede(noeud->type);
    }

    if (noeud->est_ajoute_fini() || noeud->est_ajoute_init()) {
        return !programme->pour_metaprogramme();
    }

    return false;
}

/* Construit les dépendances de l'unité (fonctions, globales, types) et crée des unités de typage
 * pour chacune des dépendances non-encore typée. */
void GestionnaireCode::determine_dependances(NoeudExpression *noeud,
                                             EspaceDeTravail *espace,
                                             GrapheDependance &graphe)
{
    dependances.reinitialise();
    rassemble_dependances(noeud, m_compilatrice, dependances.dependances);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    if (!noeud->est_ajoute_fini() && !noeud->est_ajoute_init()) {
        NoeudDependance *noeud_dependance = garantie_noeud_dependance(espace, noeud, graphe);
        graphe.ajoute_dependances(*noeud_dependance, dependances.dependances);
    }

    /* Ajoute les racines aux programmes courants de l'espace. */
    if (noeud->est_entete_fonction() && noeud->possede_drapeau(EST_RACINE)) {
        auto entete = noeud->comme_entete_fonction();
        POUR (programmes_en_cours) {
            if (it->espace() != espace) {
                continue;
            }

            it->ajoute_racine(entete);

            if (entete->corps && !entete->corps->unite) {
                requiers_typage(espace, entete->corps);
            }
        }
    }

    /* Ajoute les dépendances au programme si nécessaire. */
    auto dependances_ajoutees = false;
    auto dependances_ependues = false;
    POUR (programmes_en_cours) {
        if (!doit_ajouter_les_dependances_au_programme(noeud, it)) {
            continue;
        }
        if (!dependances_ependues) {
            dependances_ependues = epends_dependances_types(graphe, dependances, it);
        }
        if (!ajoute_dependances_au_programme(dependances.dependances, espace, *it)) {
            break;
        }
        dependances_ajoutees = true;
    }

    /* Crée les unités de typage si nécessaire. */
    if (dependances_ajoutees) {
        garantie_typage_des_dependances(*this, dependances.dependances, espace);
    }
}

UniteCompilation *GestionnaireCode::cree_unite(EspaceDeTravail *espace,
                                               RaisonDEtre raison,
                                               bool met_en_attente)
{
    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(raison);
    if (met_en_attente) {
        unites_en_attente.ajoute(unite);
    }
    return unite;
}

void GestionnaireCode::cree_unite_pour_fichier(EspaceDeTravail *espace,
                                               Fichier *fichier,
                                               RaisonDEtre raison)
{
    auto unite = cree_unite(espace, raison, true);
    unite->fichier = fichier;
}

UniteCompilation *GestionnaireCode::cree_unite_pour_noeud(EspaceDeTravail *espace,
                                                          NoeudExpression *noeud,
                                                          RaisonDEtre raison,
                                                          bool met_en_attente)
{
    auto unite = cree_unite(espace, raison, met_en_attente);
    unite->noeud = noeud;
    noeud->unite = unite;
    return unite;
}

void GestionnaireCode::requiers_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
    espace->tache_chargement_ajoutee(m_compilatrice->messagere);
    cree_unite_pour_fichier(espace, fichier, RaisonDEtre::CHARGEMENT_FICHIER);
}

void GestionnaireCode::requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->fut_charge);
    espace->tache_lexage_ajoutee(m_compilatrice->messagere);
    cree_unite_pour_fichier(espace, fichier, RaisonDEtre::LEXAGE_FICHIER);
}

void GestionnaireCode::requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->fut_lexe);
    espace->tache_parsage_ajoutee(m_compilatrice->messagere);
    cree_unite_pour_fichier(espace, fichier, RaisonDEtre::PARSAGE_FICHIER);
}

void GestionnaireCode::requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    espace->tache_typage_ajoutee(m_compilatrice->messagere);
    cree_unite_pour_noeud(espace, noeud, RaisonDEtre::TYPAGE, true);
}

void GestionnaireCode::requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);
    cree_unite_pour_noeud(espace, noeud, RaisonDEtre::GENERATION_RI, true);
}

void GestionnaireCode::requiers_generation_ri_principale_metaprogramme(
    EspaceDeTravail *espace, MetaProgramme *metaprogramme, bool peut_planifier_compilation)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);

    auto unite = cree_unite_pour_noeud(espace,
                                       metaprogramme->fonction,
                                       RaisonDEtre::GENERATION_RI_PRINCIPALE_MP,
                                       peut_planifier_compilation);

    if (!peut_planifier_compilation) {
        assert(metaprogrammes_en_attente_de_cree_contexte_est_ouvert);
        metaprogrammes_en_attente_de_cree_contexte.ajoute(unite);
    }
}

UniteCompilation *GestionnaireCode::cree_unite_pour_message(EspaceDeTravail *espace,
                                                            Message *message)
{
    auto unite = cree_unite(espace, RaisonDEtre::ENVOIE_MESSAGE, true);
    unite->message = message;
    return unite;
}

void GestionnaireCode::requiers_initialisation_type(EspaceDeTravail *espace, Type *type)
{
    if ((type->drapeaux & UNITE_POUR_INITIALISATION_FUT_CREE) != 0) {
        return;
    }

    auto unite = cree_unite(espace, RaisonDEtre::CREATION_FONCTION_INIT_TYPE, true);
    unite->type = type;

    if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
        unite->mute_attente(Attente::sur_type(type));
    }

    type->drapeaux |= UNITE_POUR_INITIALISATION_FUT_CREE;
}

void GestionnaireCode::requiers_noeud_code(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    auto unite = cree_unite(espace, RaisonDEtre::CONVERSION_NOEUD_CODE, true);
    unite->noeud = noeud;
}

bool GestionnaireCode::tente_de_garantir_presence_creation_contexte(EspaceDeTravail *espace,
                                                                    Programme *programme,
                                                                    GrapheDependance &graphe)
{
    /* NOTE : la déclaration sera automatiquement ajoutée au programme si elle n'existe pas déjà
     * lors de la complétion de son typage. Si elle existe déjà, il faut l'ajouter manuellement.
     */
    auto decl_creation_contexte = m_compilatrice->interface_kuri->decl_creation_contexte;

    if (!decl_creation_contexte) {
        return false;
    }

    programme->ajoute_fonction(decl_creation_contexte);

    if (!decl_creation_contexte->unite) {
        requiers_typage(espace, decl_creation_contexte);
        return false;
    }

    if (!decl_creation_contexte->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    determine_dependances(decl_creation_contexte, espace, graphe);

    if (!decl_creation_contexte->corps->unite) {
        requiers_typage(espace, decl_creation_contexte->corps);
        return false;
    }

    if (!decl_creation_contexte->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    determine_dependances(decl_creation_contexte->corps, espace, graphe);

    if (!decl_creation_contexte->corps->possede_drapeau(RI_FUT_GENEREE)) {
        return false;
    }

    return true;
}

void GestionnaireCode::requiers_compilation_metaprogramme(EspaceDeTravail *espace,
                                                          MetaProgramme *metaprogramme)
{
    assert(metaprogramme->fonction);
    assert(metaprogramme->fonction->possede_drapeau(DECLARATION_FUT_VALIDEE));

    /* Indique directement à l'espace qu'une exécution sera requise afin de ne pas terminer la
     * compilation trop rapidement si le métaprogramme modifie ses options de compilation. */
    espace->tache_execution_ajoutee(m_compilatrice->messagere);

    /* Ajoute le programme à la liste des programmes avant de traiter les dépendances. */
    metaprogramme_cree(metaprogramme);

    auto programme = metaprogramme->programme;
    programme->ajoute_fonction(metaprogramme->fonction);

    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();
    determine_dependances(metaprogramme->fonction, espace, *graphe);
    determine_dependances(metaprogramme->fonction->corps, espace, *graphe);

    auto ri_cree_contexte_est_disponible = tente_de_garantir_presence_creation_contexte(
        espace, programme, *graphe);
    requiers_generation_ri_principale_metaprogramme(
        espace, metaprogramme, ri_cree_contexte_est_disponible);

    if (metaprogramme->corps_texte) {
        if (metaprogramme->corps_texte_pour_fonction) {
            auto recipiente = metaprogramme->corps_texte_pour_fonction;
            assert(!recipiente->corps->unite);
            requiers_typage(espace, recipiente->corps);

            /* Crée un fichier pour le métaprogramme, et fait dépendre le corps de la fonction
             * recipiente du #corps_texte sur le parsage du fichier. Nous faisons ça ici pour nous
             * assurer que personne n'essayera de performer le typage du corps recipient avant que
             * les sources du fichiers ne soient générées, lexées, et parsées. */
            auto fichier = m_compilatrice->cree_fichier_pour_metaprogramme(metaprogramme);
            recipiente->corps->unite->mute_attente(Attente::sur_parsage(fichier));
        }
        else if (metaprogramme->corps_texte_pour_structure) {
            /* Les fichiers pour les #corps_texte des structures sont créés lors de la validation
             * sémantique. */
            assert(metaprogramme->fichier);
        }
    }
}

void GestionnaireCode::requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    auto unite = cree_unite(espace, RaisonDEtre::EXECUTION, true);
    unite->metaprogramme = metaprogramme;
    metaprogramme->unite = unite;
}

UniteCompilation *GestionnaireCode::requiers_generation_code_machine(EspaceDeTravail *espace,
                                                                     Programme *programme)
{
    auto unite = cree_unite(espace, RaisonDEtre::GENERATION_CODE_MACHINE, true);
    unite->programme = programme;
    return unite;
}

void GestionnaireCode::requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme)
{
    auto unite = cree_unite(espace, RaisonDEtre::LIAISON_PROGRAMME, true);
    unite->programme = programme;
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unite_attendante, Attente attente)
{
    assert(attente.est_valide());
    assert(unite_attendante->est_prete());

    auto espace = unite_attendante->espace;

    if (attente.est<AttenteSurType>()) {
        Type *type = attente.type();
        auto decl = decl_pour_type(type);
        if (decl && decl->unite == nullptr) {
            requiers_typage(espace, decl);
        }
        /* Ceci est pour gérer les requêtes de fonctions d'initialisation avant la génération de
         * RI. */
        if ((type->drapeaux & INITIALISATION_TYPE_FUT_CREEE) == 0 &&
            !est_type_polymorphique(type)) {
            requiers_initialisation_type(espace, type);
        }
    }
    else if (attente.est<AttenteSurDeclaration>()) {
        NoeudDeclaration *decl = attente.declaration();
        if (decl->unite == nullptr) {
            requiers_typage(espace, decl);
        }
    }

    unite_attendante->mute_attente(attente);
    unites_en_attente.ajoute(unite_attendante);
}

void GestionnaireCode::chargement_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_charge);

    auto espace = unite->espace;
    espace->tache_chargement_terminee(m_compilatrice->messagere, unite->fichier);

    unite->mute_raison_d_etre(RaisonDEtre::LEXAGE_FICHIER);
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::lexage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_lexe);

    auto espace = unite->espace;
    espace->tache_lexage_terminee(m_compilatrice->messagere);

    unite->mute_raison_d_etre(RaisonDEtre::PARSAGE_FICHIER);
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::parsage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_parse);
    auto espace = unite->espace;
    espace->tache_parsage_terminee(m_compilatrice->messagere);
}

static bool noeud_requiers_generation_ri(NoeudExpression *noeud)
{
    if (noeud->est_entete_fonction()) {
        auto entete = noeud->comme_entete_fonction();
        /* La génération de RI pour les fonctions comprend également leurs corps, or le corps
         * n'est peut-être pas encore typé. Les fonctions externes n'ayant pas de corps (même si le
         * pointeur vers le corps est valide), nous devons quand même les envoyer vers la RI afin
         * que leurs déclarations en RI soient disponibles.
         *
         * Pour les métaprogrammes, la RI doit se faire via requiers_compilation_metaprogramme. Il
         * est possible que les métaprogrammes arrivent ici après le typage, notamment pour les
         * #corps_textes.
         */
        return !entete->est_metaprogramme && entete->est_externe;
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entete;

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
        return !entete->est_metaprogramme &&
               (!entete->est_polymorphe || entete->est_monomorphisation);
    }

    if (noeud->possede_drapeau(EST_GLOBALE) && !noeud->est_structure() && !noeud->est_enum()) {
        if (noeud->est_execute()) {
            /* Les #exécutes globales sont gérées via les métaprogrammes. */
            return false;
        }
        return true;
    }

    return false;
}

static bool doit_determiner_les_dependances(NoeudExpression *noeud)
{
    if (noeud->est_declaration()) {
        return !(noeud->est_charge() || noeud->est_importe());
    }

    if (noeud->est_execute()) {
        return true;
    }

    if (noeud->est_ajoute_fini()) {
        return true;
    }

    if (noeud->est_ajoute_init()) {
        return true;
    }

    if (noeud->est_pre_executable()) {
        return true;
    }

    return false;
}

static bool declaration_est_invalide(NoeudExpression *decl)
{
    if (decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    auto const unite = decl->unite;
    if (!unite) {
        /* Pas encore d'unité, nous ne pouvons savoir si la déclaration est valide. */
        return true;
    }

    if (unite->espace->possede_erreur) {
        /* Si l'espace responsable de l'unité de l'entête possède une erreur, nous devons
         * ignorer les entêtes invalides, car sinon la compilation serait infinie. */
        return false;
    }

    return true;
}

static bool verifie_que_toutes_les_entetes_sont_validees(SystemeModule const &sys_module)
{
    kuri::ensemble<Module *> modules_visites;

    POUR_TABLEAU_PAGE (sys_module.modules) {
        for (auto fichier : it.fichiers) {
            if (!fichier->fut_charge) {
                return false;
            }

            if (!fichier->fut_parse) {
                return false;
            }
        }

        if (it.bloc == nullptr) {
            return false;
        }

        for (auto decl : (*it.bloc->membres.verrou_lecture())) {
            if (decl->est_entete_fonction() && declaration_est_invalide(decl)) {
                return false;
            }
        }

        for (auto decl : (*it.bloc->expressions.verrou_lecture())) {
            if (decl->est_importe() && declaration_est_invalide(decl)) {
                return false;
            }
        }
    }

    return true;
}

void GestionnaireCode::typage_termine(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert_rappel(unite->noeud->possede_drapeau(DECLARATION_FUT_VALIDEE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " ne fut pas validé !\n";
        erreur::imprime_site(*unite->espace, unite->noeud);
    });

    auto espace = unite->espace;

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();
    auto noeud = unite->noeud;
    if (doit_determiner_les_dependances(unite->noeud)) {
        determine_dependances(unite->noeud, unite->espace, *graphe);
    }

    /* Envoi un message, nous attendrons dessus si nécessaire. */
    const auto message = m_compilatrice->messagere->ajoute_message_typage_code(espace, noeud);
    const auto doit_envoyer_en_ri = noeud_requiers_generation_ri(noeud);
    if (doit_envoyer_en_ri) {
        espace->tache_ri_ajoutee(m_compilatrice->messagere);
        unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
        unites_en_attente.ajoute(unite);
    }

    if (message) {
        requiers_noeud_code(espace, noeud);
        auto unite_message = cree_unite_pour_message(espace, message);
        unite_message->mute_attente(Attente::sur_noeud_code(&noeud->noeud_code));
        unite->mute_attente(Attente::sur_message(message));
    }

    auto peut_envoyer_changement_de_phase = verifie_que_toutes_les_entetes_sont_validees(
        *m_compilatrice->sys_module.verrou_lecture());

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    espace->tache_typage_terminee(m_compilatrice->messagere, peut_envoyer_changement_de_phase);

    if (noeud->est_entete_fonction()) {
        m_fonctions_parsees.ajoute(noeud->comme_entete_fonction());
    }
}

static inline bool est_corps_de(NoeudExpression const *noeud,
                                NoeudDeclarationEnteteFonction const *fonction)
{
    if (fonction == nullptr) {
        return false;
    }
    return noeud == fonction->corps;
}

void GestionnaireCode::generation_ri_terminee(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert_rappel(unite->noeud->possede_drapeau(RI_FUT_GENEREE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " n'eu pas de RI générée !\n";
        erreur::imprime_site(*unite->espace, unite->noeud);
    });

    auto espace = unite->espace;
    espace->tache_ri_terminee(m_compilatrice->messagere);
    if (espace->optimisations) {
        // À FAIRE(gestion) : tâches d'optimisations
    }

    /* Si nous avons la RI pour #crée_contexte, il nout faut ajouter toutes les unités l'attendant.
     */
    if (est_corps_de(unite->noeud,
                     espace->compilatrice().interface_kuri->decl_creation_contexte)) {
        flush_metaprogrammes_en_attente_de_cree_contexte();
    }
}

void GestionnaireCode::optimisation_terminee(UniteCompilation *unite)
{
    assert(unite->noeud);
    auto espace = unite->espace;
    espace->tache_optimisation_terminee(m_compilatrice->messagere);
}

void GestionnaireCode::message_recu(Message const *message)
{
    POUR (unites_en_attente) {
        if (it->attend_sur_message(message)) {
            it->marque_prete();
        }
    }
}

void GestionnaireCode::execution_terminee(UniteCompilation *unite)
{
    assert(unite->metaprogramme);
    assert(unite->metaprogramme->fut_execute);
    auto espace = unite->espace;
    espace->tache_execution_terminee(m_compilatrice->messagere);
    enleve_programme(unite->metaprogramme->programme);
}

void GestionnaireCode::generation_code_machine_terminee(UniteCompilation *unite)
{
    assert(unite->programme);

    auto programme = unite->programme;
    auto espace = unite->espace;

    if (programme->pour_metaprogramme()) {
        programme->change_de_phase(PhaseCompilation::APRES_GENERATION_OBJET);
        programme->change_de_phase(PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
        requiers_liaison_executable(espace, unite->programme);
    }
    else {
        espace->tache_generation_objet_terminee(m_compilatrice->messagere);

        if (espace->options.resultat != ResultatCompilation::RIEN) {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
            requiers_liaison_executable(espace, unite->programme);
        }
        else {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);
        }
    }
}

void GestionnaireCode::liaison_programme_terminee(UniteCompilation *unite)
{
    assert(unite->programme);

    auto programme = unite->programme;
    auto espace = unite->espace;

    if (programme->pour_metaprogramme()) {
        auto metaprogramme = programme->pour_metaprogramme();
        programme->change_de_phase(PhaseCompilation::APRES_LIAISON_EXECUTABLE);
        programme->change_de_phase(PhaseCompilation::COMPILATION_TERMINEE);
        requiers_execution(unite->espace, metaprogramme);
    }
    else {
        espace->tache_liaison_executable_terminee(m_compilatrice->messagere);
        espace->change_de_phase(m_compilatrice->messagere, PhaseCompilation::COMPILATION_TERMINEE);
    }
}

void GestionnaireCode::conversion_noeud_code_terminee(UniteCompilation *unite)
{
    auto noeud = unite->noeud;

    POUR (unites_en_attente) {
        if (it->attend_sur_noeud_code(&noeud->noeud_code)) {
            assert(it->raison_d_etre() == RaisonDEtre::ENVOIE_MESSAGE);
            auto message = it->message;
            static_cast<MessageTypageCodeTermine *>(message)->code = noeud->noeud_code;
            it->marque_prete();
        }
    }
}

void GestionnaireCode::fonction_initialisation_type_creee(UniteCompilation *unite)
{
    assert((unite->type->drapeaux & INITIALISATION_TYPE_FUT_CREEE) != 0);
    unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
    unite->espace->tache_ri_ajoutee(m_compilatrice->messagere);
    unite->noeud = unite->type->fonction_init;
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::cree_taches(OrdonnanceuseTache &ordonnanceuse)
{
    kuri::tableau<UniteCompilation *> nouvelles_unites;

    if (plus_rien_n_est_a_faire()) {
        ordonnanceuse.marque_compilation_terminee();
        return;
    }

#undef DEBUG_UNITES_EN_ATTENTES

#ifdef DEBUG_UNITES_EN_ATTENTES
    std::cerr << "Unités en attente avant la création des tâches : " << unites_en_attente.taille()
              << '\n';
    ordonnanceuse.imprime_donnees_files(std::cerr);
#endif

    POUR (unites_en_attente) {
        if (it->espace->possede_erreur) {
            continue;
        }

        it->marque_prete_si_attente_resolue();

        if (!it->est_prete()) {
            it->cycle += 1;

            if (it->est_bloquee()) {
                it->rapporte_erreur();
                unites_en_attente.efface();
                ordonnanceuse.supprime_toutes_les_taches();
                return;
            }

            nouvelles_unites.ajoute(it);
            continue;
        }

        if (it->raison_d_etre() == RaisonDEtre::AUCUNE) {
            it->espace->rapporte_erreur_sans_site(
                "Erreur interne : obtenu une unité sans raison d'être");
            continue;
        }

        if (it->annule) {
            continue;
        }

        /* Il est possible qu'un métaprogramme ajout du code, donc soyons sûr que l'espace est bel
         * et bien dans la phase pour la génération de code. */
        if (it->raison_d_etre() == RaisonDEtre::GENERATION_CODE_MACHINE &&
            it->programme == it->espace->programme) {
            if (it->espace->phase_courante() != PhaseCompilation::AVANT_GENERATION_OBJET) {
                continue;
            }
        }

        ordonnanceuse.cree_tache_pour_unite(it);
    }

    /* Supprime toutes les tâches des espaces erronés. Il est possible qu'une erreur soit lancée
     * durant la création de tâches ci-dessus, et que l'erreur ne génère pas une fin totale de la
     * compilation. Nous ne pouvons faire ceci ailleurs (dans la fonction qui rapporte l'erreur)
     * puisque nous possédons déjà un verrou sur l'ordonnanceuse, et nous risquerions d'avoir un
     * verrou mort. */
    kuri::ensemblon<EspaceDeTravail *, 10> espaces_errones;
    POUR (programmes_en_cours) {
        if (it->espace()->possede_erreur) {
            espaces_errones.insere(it->espace());
        }
    }

    pour_chaque_element(espaces_errones, [&](EspaceDeTravail *espace) {
        ordonnanceuse.supprime_toutes_les_taches_pour_espace(espace);
        return kuri::DecisionIteration::Continue;
    });

    if (m_compilatrice->possede_erreur()) {
        ordonnanceuse.supprime_toutes_les_taches();
    }

    unites_en_attente = nouvelles_unites;

#ifdef DEBUG_UNITES_EN_ATTENTES
    std::cerr << "Unités en attente après la création des tâches : " << unites_en_attente.taille()
              << '\n';
    ordonnanceuse.imprime_donnees_files(std::cerr);
    std::cerr << "--------------------------------------------------------\n";
#endif
}

bool GestionnaireCode::plus_rien_n_est_a_faire()
{
    if (m_compilatrice->possede_erreur()) {
        return true;
    }

    auto espace_errone_existe = false;

    POUR (programmes_en_cours) {
        auto espace = it->espace();

        /* Vérifie si une erreur existe. */
        if (espace->possede_erreur) {
            /* Nous devons continuer d'envoyer des messages si l'erreur
             * ne provoque pas la fin de la compilation de tous les espaces.
             * À FAIRE : même si un message est ajouté, purge_message provoque
             * une compilation infinie. */
            if (!espace->options.continue_si_erreur) {
                m_compilatrice->messagere->purge_messages();
            }

            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);

            if (!espace->options.continue_si_erreur) {
                return true;
            }

            espace_errone_existe = true;
            continue;
        }

        tente_de_garantir_fonction_point_d_entree(espace);

        if (it->pour_metaprogramme()) {
            auto etat = it->ajourne_etat_compilation();

            if (etat.phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE) {
                it->change_de_phase(PhaseCompilation::AVANT_GENERATION_OBJET);
                requiers_generation_code_machine(espace, it);
            }
        }
        else {
            if (espace->phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE &&
                it->ri_generees()) {
                finalise_programme_avant_generation_code_machine(espace, it);
            }
        }
    }

    if (espace_errone_existe) {
        programmes_en_cours.efface_si(
            [](Programme *programme) -> bool { return programme->espace()->possede_erreur; });

        if (programmes_en_cours.est_vide()) {
            return true;
        }
    }

    if (!unites_en_attente.est_vide() || !metaprogrammes_en_attente_de_cree_contexte.est_vide()) {
        return false;
    }

    bool ok = true;
    POUR (programmes_en_cours) {
        auto espace = it->espace();

        /* Les programmes des métaprogrammes sont enlevés après leurs exécutions. Si nous en
         * avons un, la compilation ne peut se terminée. */
        if (it->pour_metaprogramme()) {
            ok = false;
            continue;
        }

        /* Attend que tous les espaces eurent leur compilation terminée. */
        if (espace->phase_courante() != PhaseCompilation::COMPILATION_TERMINEE) {
            ok = false;
            continue;
        }
    }

    return ok;
}

void GestionnaireCode::tente_de_garantir_fonction_point_d_entree(EspaceDeTravail *espace)
{
    // Ne compile le point d'entrée que pour les exécutables
    if (espace->options.resultat != ResultatCompilation::EXECUTABLE) {
        return;
    }

    if (espace->fonction_point_d_entree != nullptr) {
        return;
    }

    auto point_d_entree = m_compilatrice->fonction_point_d_entree;
    if (point_d_entree == nullptr) {
        return;
    }

    auto copie = copie_noeud(m_assembleuse, point_d_entree, point_d_entree->bloc_parent);
    copie->drapeaux |= (DECLARATION_FUT_VALIDEE | EST_RACINE);
    copie->comme_entete_fonction()->corps->drapeaux |= DECLARATION_FUT_VALIDEE;

    requiers_typage(espace, copie);
    espace->fonction_point_d_entree = copie->comme_entete_fonction();
}

void GestionnaireCode::finalise_programme_avant_generation_code_machine(EspaceDeTravail *espace,
                                                                        Programme *programme)
{
    if (espace->options.resultat == ResultatCompilation::RIEN) {
        espace->change_de_phase(m_compilatrice->messagere, PhaseCompilation::COMPILATION_TERMINEE);
        return;
    }

    if (!espace->peut_generer_code_final()) {
        return;
    }

    auto modules = programme->modules_utilises();
    auto executions_requises = false;
    modules.pour_chaque_element([&](Module *module) {
        auto execute = module->directive_pre_executable;
        if (execute && !module->execution_directive_requise) {
            /* L'espace du programme est celui qui a créé le métaprogramme lors de la validation de
             * code, mais nous devons avoir le métaprogramme (qui hérite de l'espace du programme)
             * dans l'espace demandant son exécution afin que le compte de taches d'exécution dans
             * l'espace soit cohérent. */
            execute->metaprogramme->programme->change_d_espace(espace);
            requiers_compilation_metaprogramme(espace, execute->metaprogramme);
            module->execution_directive_requise = true;
            executions_requises = true;
        }
    });

    if (executions_requises) {
        return;
    }

    /* Requiers la génération de RI pour les fonctions ajoute_fini et ajoute_init. */
    auto decl_ajoute_fini = m_compilatrice->interface_kuri->decl_fini_execution_kuri;
    auto decl_ajoute_init = m_compilatrice->interface_kuri->decl_init_execution_kuri;
    auto decl_init_globales = m_compilatrice->interface_kuri->decl_init_globales_kuri;

    auto ri_requise = false;
    if (!decl_ajoute_fini->corps->possede_drapeau(RI_FUT_GENEREE)) {
        requiers_generation_ri(espace, decl_ajoute_fini);
        ri_requise = true;
    }
    if (!decl_ajoute_init->corps->possede_drapeau(RI_FUT_GENEREE)) {
        requiers_generation_ri(espace, decl_ajoute_init);
        ri_requise = true;
    }
    if (!decl_init_globales->corps->possede_drapeau(RI_FUT_GENEREE)) {
        requiers_generation_ri(espace, decl_init_globales);
        ri_requise = true;
    }

    if (ri_requise) {
        return;
    }

    /* Tous les métaprogrammes furent exécutés, et la RI pour les fonctions
     * d'initialisation/finition sont générées : nous pouvons générer le code machine. */
    auto message = espace->change_de_phase(m_compilatrice->messagere,
                                           PhaseCompilation::AVANT_GENERATION_OBJET);

    /* Nous avions déjà créé une unité pour générer le code machine, mais un métaprogramme a sans
     * doute ajouté du code. Il faut annuler l'unité précédente qui peut toujours être dans la file
     * d'attente. */
    if (espace->unite_pour_code_machine) {
        espace->unite_pour_code_machine->annule = true;
    }

    auto unite_code_machine = requiers_generation_code_machine(espace, espace->programme);

    espace->unite_pour_code_machine = unite_code_machine;

    if (message) {
        unite_code_machine->mute_attente(Attente::sur_message(message));
    }
}

void GestionnaireCode::flush_metaprogrammes_en_attente_de_cree_contexte()
{
    assert(metaprogrammes_en_attente_de_cree_contexte_est_ouvert);
    POUR (metaprogrammes_en_attente_de_cree_contexte) {
        unites_en_attente.ajoute(it);
    }
    metaprogrammes_en_attente_de_cree_contexte.efface();
    metaprogrammes_en_attente_de_cree_contexte_est_ouvert = false;
}

void GestionnaireCode::interception_message_terminee(EspaceDeTravail *espace)
{
    m_compilatrice->messagere->termine_interception(espace);

    kuri::tableau<UniteCompilation *> nouvelles_unites;

    POUR (unites_en_attente) {
        if (it->raison_d_etre() == RaisonDEtre::ENVOIE_MESSAGE) {
            continue;
        }

        if (it->attend_sur_un_message()) {
            it->marque_prete();
        }

        nouvelles_unites.ajoute(it);
    }

    unites_en_attente = nouvelles_unites;
}

void GestionnaireCode::ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace)
{
    auto programme = espace->programme;
    programme->ajourne_pour_nouvelles_options_espace();
}
