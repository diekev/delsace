/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "gestionnaire_code.hh"

#include "biblinternes/outils/assert.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "programme.hh"

// À FAIRE(gestion) : message pour la génération de code machine

// À FAIRE(gestion) : message pour la liaison du programme finale

// À FAIRE(gestion) : message pour l'écriture du fichier objet

// À FAIRE(gestion) : message pour la fin de la compilation

/*
  À FAIRE(gestion) : pour chaque type :
- création d'une déclaration de type (ajout d'un noeud syntaxique: NoeudDéclarationType, d'où
dérivront les structures et les énums) (TACHE_CREATION_DECLARATION_TYPE)
- création d'une fonction d'initialisation (TACHE_CREATION_FONCTION_INITIALISATION)
- avoir un lexème sentinel pour l'impression des erreurs si le noeud est crée lors de la
compilation
 */

/*
  À FAIRE(gestion) : métaprogrammes
  - ajout d'un programme aux métaprogrammes
  - si une unité dépend sur l'exécution d'un métaprogramme mets-la en attente
  - quand le programme du métaprogramme est compilé, crée une unité pour l'exécution du
  métaprogramme
  - questions ouvertes :
    -- qui crée le métaprogramme
    -- qui notifie le gestionnaire qu'un métaprogramme fut créé (pour ajouter son programme à
  liste)
    -- comment gérer les cas où un métaprogramme est créé, mais que les dépendances furent déjà
       compilées (comme decl_creation_contexte)
 */

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

    if (noeud->possede_drapeau(EST_DECLARATION_TYPE_OPAQUE)) {
        return false;
    }

    if (noeud->possede_drapeau(EST_CONSTANTE)) {
        return false;
    }

    return noeud->possede_drapeau(EST_GLOBALE);
}

static void ajoute_dependances_au_programme(DonneesDependance const &dependances,
                                            Programme &programme)
{
    /* Ajoute les fonctions. */
    dls::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        programme.ajoute_fonction(const_cast<NoeudDeclarationEnteteFonction *>(fonction));
        return dls::DecisionIteration::Continue;
    });

    /* Ajoute les globales. */
    dls::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        programme.ajoute_globale(const_cast<NoeudDeclarationVariable *>(globale));
        return dls::DecisionIteration::Continue;
    });

    /* Ajoute les types. */
    dls::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        programme.ajoute_type(type);
        return dls::DecisionIteration::Continue;
    });
}

/* Traverse l'arbre syntaxique de la racine spécifiée et rassemble les fonctions, types, et
 * globales utilisées. */
static void rassemble_dependances(NoeudExpression *racine,
                                  EspaceDeTravail *espace,
                                  DonneesDependance &dependances)
{
    // À FAIRE(gestion) : pour les structures ou unions, vérifie que les connexions sont bonnes
    //                    entre les types et les ceux des membres (type_compose.membre,
    //                    type_structure.types_employes)

    // À FAIRE(gestion) : l'utilisation d'un type_de_données brise le graphe de dépendance (les
    //                    membres des types structure ou énum sont des types_de_données)

    // À FAIRE(gestion) : pour les structures ou unions, les membres constants peuvent être des
    //                    structures ou unions

    // À FAIRE(gestion) : vérifie les dépendances pour les types tableaux ou union anymnome (p.e.:
    //                    []z32, r32 | r16), il faut utilisé le type, et non le type_de_données
    visite_noeud(
        racine,
        PreferenceVisiteNoeud::SUBSTITUTION,
        [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
            // Note: les fonctions polymorphiques n'ont pas de types.
            if (noeud->type) {
                if (noeud->type->est_type_de_donnees()) {
                    auto type_de_donnees = noeud->type->comme_type_de_donnees();
                    if (type_de_donnees->type_connu) {
                        dependances.types_utilises.insere(type_de_donnees->type_connu);
                    }
                }
                else {
                    dependances.types_utilises.insere(noeud->type);
                }
            }

            if (noeud->est_entete_fonction()) {
                /* Visite manuellement les enfants des entêtes, car nous irions visiter le corps
                 * qui ne fut pas encore typé. */
                auto entete = noeud->comme_entete_fonction();

                POUR (entete->params) {
                    rassemble_dependances(it, espace, dependances);
                }

                POUR (entete->params_sorties) {
                    rassemble_dependances(it, espace, dependances);
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
                    dependances.globales_utilisees.insere(decl->comme_declaration_variable());
                }
                else if (decl->est_entete_fonction() &&
                         !decl->comme_entete_fonction()->est_polymorphe) {
                    auto decl_fonc = decl->comme_entete_fonction();
                    dependances.fonctions_utilisees.insere(decl_fonc);
                }
            }
            else if (noeud->est_cuisine()) {
                auto cuisine = noeud->comme_cuisine();
                auto expr = cuisine->expression;
                dependances.fonctions_utilisees.insere(
                    expr->comme_appel()->expression->comme_entete_fonction());
            }
            else if (noeud->est_indexage()) {
                /* Traite les indexages avant les expressions binaires afin de ne pas les traiter
                 * comme tel (est_expression_binaire() retourne vrai pour les indexages). */

                auto indexage = noeud->comme_indexage();
                /* op peut être nul pour les déclaration de type ([]z32) */
                if (indexage->op && !indexage->op->est_basique) {
                    dependances.fonctions_utilisees.insere(indexage->op->decl);
                }

                /* Marque les dépendances sur les fonctions d'interface de kuri. */
                auto interface = espace->interface_kuri;

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
                        dependances.fonctions_utilisees.insere(interface->decl_panique_tableau);
                        break;
                    }
                    case GenreType::TABLEAU_FIXE:
                    {
                        assert(interface->decl_panique_tableau);
                        if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
                            dependances.fonctions_utilisees.insere(
                                interface->decl_panique_tableau);
                        }
                        break;
                    }
                    case GenreType::CHAINE:
                    {
                        assert(interface->decl_panique_chaine);
                        if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
                            dependances.fonctions_utilisees.insere(interface->decl_panique_chaine);
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
                    dependances.fonctions_utilisees.insere(expression_binaire->op->decl);
                }
            }
            else if (noeud->est_discr()) {
                auto discr = noeud->comme_discr();
                if (discr->op && !discr->op->est_basique) {
                    dependances.fonctions_utilisees.insere(discr->op->decl);
                }
            }
            else if (noeud->est_construction_tableau()) {
                auto construction_tableau = noeud->comme_construction_tableau();
                dependances.types_utilises.insere(construction_tableau->type);

                /* Ajout également du type de pointeur pour la génération de code C. */
                auto type_feuille = construction_tableau->type->comme_tableau_fixe()->type_pointe;
                auto type_ptr = espace->typeuse.type_pointeur_pour(type_feuille);
                dependances.types_utilises.insere(type_ptr);
            }
            else if (noeud->est_tente()) {
                auto tente = noeud->comme_tente();

                if (!tente->expression_piegee) {
                    auto interface = espace->interface_kuri;
                    assert(interface->decl_panique_erreur);
                    dependances.fonctions_utilisees.insere(interface->decl_panique_erreur);
                }
            }
            else if (noeud->est_reference_membre_union()) {
                auto interface = espace->interface_kuri;
                assert(interface->decl_panique_membre_union);
                dependances.fonctions_utilisees.insere(interface->decl_panique_membre_union);
            }
            else if (noeud->est_comme()) {
                auto comme = noeud->comme_comme();

                /* Marque les dépendances sur les fonctions d'interface de kuri. */
                auto interface = espace->interface_kuri;

                if (comme->transformation.type == TypeTransformation::EXTRAIT_UNION) {
                    assert(interface->decl_panique_membre_union);
                    // À FAIRE(gestion) : uniquement si l'union est nonsûre.
                    dependances.fonctions_utilisees.insere(interface->decl_panique_membre_union);
                }
                else if (comme->transformation.type == TypeTransformation::FONCTION) {
                    assert(comme->transformation.fonction);
                    dependances.fonctions_utilisees.insere(
                        const_cast<NoeudDeclarationEnteteFonction *>(
                            comme->transformation.fonction));
                }

                /* Nous avons besoin d'un type pointeur pour le type cible pour la génération de
                 * RI. À FAIRE: généralise pour toutes les variables. */
                auto type_pointeur = espace->typeuse.type_pointeur_pour(
                    comme->transformation.type_cible, false, false);
                dependances.types_utilises.insere(type_pointeur);
            }
            else if (noeud->est_appel()) {
                auto appel = noeud->comme_appel();

                if (appel->noeud_fonction_appelee) {
                    dependances.fonctions_utilisees.insere(
                        appel->noeud_fonction_appelee->comme_entete_fonction());
                }
            }
            else if (noeud->est_args_variadiques()) {
                auto args = noeud->comme_args_variadiques();

                /* Création d'un type tableau fixe, pour la génération de code. */
                auto taille_tableau = args->expressions.taille();
                auto type_tfixe = espace->typeuse.type_tableau_fixe(
                    args->type, taille_tableau, false);
                dependances.types_utilises.insere(type_tfixe);
            }
            else if (noeud->est_assignation_variable()) {
                auto assignation = noeud->comme_assignation_variable();

                POUR (assignation->donnees_exprs.plage()) {
                    rassemble_dependances(it.expression, espace, dependances);
                }
            }
            else if (noeud->est_declaration_variable()) {
                auto declaration = noeud->comme_declaration_variable();

                POUR (declaration->donnees_decl.plage()) {
                    rassemble_dependances(it.expression, espace, dependances);
                }
            }

            return DecisionVisiteNoeud::CONTINUE;
        });
}

/* Crée un noeud de dépendance pour le noeud spécifié en paramètre, et retourne un pointeur vers
 * celui-ci. Retourne nul si le noeud n'est pas supposé avoir un noeud de dépendance. */
static NoeudDependance *garantie_noeud_dependance(NoeudExpression *noeud, GrapheDependance &graphe)
{
    /* N'utilise pas est_declaration_variable_globale car nous voulons également les opaques et les
     * constantes. */
    if (noeud->est_declaration_variable()) {
        assert(noeud->possede_drapeau(EST_GLOBALE));
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

    if (noeud->est_enum() || noeud->est_structure()) {
        return graphe.cree_noeud_type(noeud->type);
    }

    assert(!"Noeud non géré pour les dépendances !\n");
    return nullptr;
}

static NoeudDeclaration *decl_pour_type(const Type *type)
{
    if (type->est_structure()) {
        return type->comme_structure()->decl;
    }

    if (type->est_enum()) {
        return type->comme_enum()->decl;
    }

    if (type->est_erreur()) {
        return type->comme_erreur()->decl;
    }

    if (type->est_union()) {
        return type->comme_union()->decl;
    }

    return nullptr;
}

/* Requiers le typage de toutes les dépendances. */
static void garantie_typage_des_dependances(GestionnaireCode &gestionnaire,
                                            DonneesDependance const &dependances,
                                            EspaceDeTravail *espace)
{
    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    dls::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        if (!fonction->corps->unite && !fonction->est_externe) {
//            std::cerr << "Garantie le typage du corps de :\n";
//            erreur::imprime_site(*espace, fonction);
            gestionnaire.requiers_typage(espace, fonction->corps);
        }
        return dls::DecisionIteration::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    dls::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        if (!globale->unite) {
            gestionnaire.requiers_typage(espace, const_cast<NoeudDeclarationVariable *>(globale));
        }
        return dls::DecisionIteration::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    dls::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        auto decl = decl_pour_type(type);
        if (decl && !decl->unite) {
            // Pour les unions anonymes, nous allons directement à la génération de code RI.
            if (type->est_union() && type->comme_union()->est_anonyme) {
                gestionnaire.requiers_generation_ri(espace, decl);
            }
            else {
                gestionnaire.requiers_typage(espace, decl);
            }
        }
        return dls::DecisionIteration::Continue;
    });
}

/* Traverse le graphe de dépendances pour chaque type présents dans les dépendances courantes, et
 * ajoutes les dépedances de ces types aux dépendances.
 * Le but de cette fonction est de s'assurer que toutes les dépendances des types sont ajoutées aux
 * programmes. */
static void epends_dependances_types(GrapheDependance &graphe, DonneesDependance &dependances)
{
    dls::ensemblon<Type *, 16> types_utilises;
    dls::ensemblon<NoeudDeclarationEnteteFonction *, 16> fonctions_utilisees{};
    dls::ensemblon<NoeudDeclarationVariable *, 16> globales_utilisees{};

    /* Traverse le graphe pour chaque dépendance sur un type. */
    dls::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        types_utilises.insere(type);

        auto noeud_dependance = graphe.cree_noeud_type(type);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_type()) {
                types_utilises.insere(relation->type());
            }
        });

        return dls::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    dls::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        fonctions_utilisees.insere(fonction);
        types_utilises.insere(fonction->type);

        auto noeud_dependance = graphe.cree_noeud_fonction(fonction);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_fonction()) {
                fonctions_utilisees.insere(relation->fonction());
            }
            else if (relation->est_globale()) {
                globales_utilisees.insere(relation->globale());
            }
            else if (relation->est_type()) {
                types_utilises.insere(relation->type());
            }
        });

        return dls::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    dls::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        globales_utilisees.insere(globale);
        types_utilises.insere(globale->type);

        auto noeud_dependance = graphe.cree_noeud_globale(globale);
        graphe.traverse(noeud_dependance, [&](NoeudDependance const *relation) {
            if (relation->est_fonction()) {
                fonctions_utilisees.insere(relation->fonction());
            }
            else if (relation->est_globale()) {
                globales_utilisees.insere(relation->globale());
            }
            else if (relation->est_type()) {
                types_utilises.insere(relation->type());
            }
        });

        return dls::DecisionIteration::Continue;
    });

    /* Réinitialise le graphe pour les traversées futures. */
    POUR_TABLEAU_PAGE (graphe.noeuds) {
        it.fut_visite = false;
    }

    /* Ajoute les nouveaux types aux dépendances courantes. */
    dls::pour_chaque_element(types_utilises, [&](auto &type) {
        if (type->est_type_de_donnees()) {
            auto type_de_donnees = type->comme_type_de_donnees();
            if (type_de_donnees->type_connu) {
                dependances.types_utilises.insere(type_de_donnees->type_connu);
            }
        }
        else {
            dependances.types_utilises.insere(type);
        }
        return dls::DecisionIteration::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    dls::pour_chaque_element(fonctions_utilisees, [&](auto &fonction) {
        dependances.fonctions_utilisees.insere(fonction);
        return dls::DecisionIteration::Continue;
    });

    /* Ajoute les nouveaux types aux dépendances courantes. */
    dls::pour_chaque_element(globales_utilisees, [&](auto &globale) {
        dependances.globales_utilisees.insere(globale);
        return dls::DecisionIteration::Continue;
    });
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

    return false;
}

/* Construit les dépendances de l'unité (fonctions, globales, types) et crée des unités de typage
 * pour chacune des dépendances non-encore typée. */
void GestionnaireCode::determine_dependances(NoeudExpression *noeud,
                                             EspaceDeTravail *espace,
                                             GrapheDependance &graphe)
{
    dependances.efface();
    rassemble_dependances(noeud, espace, dependances);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    NoeudDependance *noeud_dependance = garantie_noeud_dependance(noeud, graphe);
    graphe.ajoute_dependances(*noeud_dependance, dependances);

    epends_dependances_types(graphe, dependances);

#if 0
    if (noeud->ident == ID::principale) {
        std::cerr << "Dépendances de la fonction principale :\n";

        std::cerr << "fonctions :\n";
        dls::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
            erreur::imprime_site(*espace, fonction);
            return dls::DecisionIteration::Continue;
        });

        std::cerr << "globales :\n";
        /* Requiers le typage de toutes les déclarations utilisées. */
        dls::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
            erreur::imprime_site(*espace, globale);
            return dls::DecisionIteration::Continue;
        });

        std::cerr << "types :\n";
        /* Requiers le typage de tous les types utilisés. */
        dls::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
            std::cerr << chaine_type(type) << '\n';
            return dls::DecisionIteration::Continue;
        });
    }
#endif

    /* Ajoute les racines aux programmes courants. */
    if (noeud->est_entete_fonction() && noeud->possede_drapeau(EST_RACINE)) {
        POUR (programmes_en_cours) {
            it->ajoute_racine(noeud->comme_entete_fonction());
        }
    }

    /* Ajoute les dépendances au programme si nécessaire. */
    auto dependances_ajoutees = false;
    POUR (programmes_en_cours) {
        if (doit_ajouter_les_dependances_au_programme(noeud, it)) {
            ajoute_dependances_au_programme(dependances, *it);
            dependances_ajoutees = true;
        }
    }

    /* Crée les unités de typage si nécessaire. */
    if (dependances_ajoutees) {
        garantie_typage_des_dependances(*this, dependances, espace);
    }
#if 0
    else {
        std::cerr << "Ne doit pas ajouter les dépendances au programme\n";
        if (noeud->est_corps_fonction()) {
            erreur::imprime_site(*unite->espace, noeud->comme_corps_fonction()->entete);
        }
    }
#endif
}

void GestionnaireCode::requiers_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
    espace->tache_chargement_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::CHARGEMENT_FICHIER);
    unite->fichier = fichier;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->donnees_constantes->fut_charge);
    espace->tache_lexage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::LEXAGE_FICHIER);
    unite->fichier = fichier;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    assert(fichier->donnees_constantes->fut_lexe);
    espace->tache_parsage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::PARSAGE_FICHIER);
    unite->fichier = fichier;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    espace->tache_typage_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::TYPAGE);
    unite->noeud = noeud;

    noeud->unite = unite;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
    unite->noeud = noeud;

    noeud->unite = unite;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_generation_ri_principale_metaprogramme(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI_PRINCIPALE_MP);
    unite->noeud = metaprogramme->fonction;

    metaprogramme->fonction->unite = unite;

    unites_en_attente.ajoute(unite);
}

std::optional<Attente> GestionnaireCode::tente_de_garantir_presence_creation_contexte(
        EspaceDeTravail *espace,
        Programme *programme,
        GrapheDependance &graphe)
{
    /* NOTE : la déclaration sera automatiquement ajoutée au programme si elle n'existe pas déjà
     * lors de la complétion de son typage. Si elle existe déjà, il faut l'ajouter manuellement.
     */
    auto decl_creation_contexte = espace->interface_kuri->decl_creation_contexte;

    if (!decl_creation_contexte) {
        return Attente::sur_interface_kuri(ID::cree_contexte);
    }

    programme->ajoute_fonction(decl_creation_contexte);

    if (!decl_creation_contexte->unite) {
        requiers_typage(espace, decl_creation_contexte);
        return Attente::sur_declaration(decl_creation_contexte);
    }

    if (!decl_creation_contexte->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
        return Attente::sur_declaration(decl_creation_contexte);
    }

    determine_dependances(decl_creation_contexte, espace, graphe);

    if (!decl_creation_contexte->corps->unite) {
        requiers_typage(espace, decl_creation_contexte->corps);
        return Attente::sur_declaration(decl_creation_contexte->corps);
    }

    if (decl_creation_contexte->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
        return Attente::sur_declaration(decl_creation_contexte->corps);
    }

    determine_dependances(decl_creation_contexte->corps, espace, graphe);

    if (!decl_creation_contexte->corps->possede_drapeau(RI_FUT_GENEREE)) {
        return Attente::sur_ri(&decl_creation_contexte->atome);
    }

    return {};
}

void GestionnaireCode::requiers_compilation_metaprogramme(EspaceDeTravail *espace,
                                                          MetaProgramme *metaprogramme)
{
    assert(metaprogramme->fonction);
    assert(metaprogramme->fonction->possede_drapeau(DECLARATION_FUT_VALIDEE));

    auto programme = metaprogramme->programme;
    programme->ajoute_fonction(metaprogramme->fonction);

    auto graphe = espace->graphe_dependance.verrou_ecriture();
    determine_dependances(metaprogramme->fonction, espace, *graphe);
    determine_dependances(metaprogramme->fonction->corps, espace, *graphe);

    requiers_generation_ri_principale_metaprogramme(espace, metaprogramme);

    auto attente = tente_de_garantir_presence_creation_contexte(espace, programme, *graphe);
    if (attente.has_value()) {
        metaprogramme->fonction->unite->mute_attente(attente.value());
    }

    metaprogramme_cree(metaprogramme);
}

void GestionnaireCode::requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    espace->tache_execution_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::EXECUTION);
    unite->metaprogramme = metaprogramme;

    metaprogramme->unite = unite;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_generation_code_machine(EspaceDeTravail *espace,
                                                        Programme *programme)
{
    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::GENERATION_CODE_MACHINE);
    unite->programme = programme;
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme)
{
    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::LIAISON_PROGRAMME);
    unite->programme = programme;
    unites_en_attente.ajoute(unite);
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
            // std::cerr << "Requiers le typage de " << chaine_type(type) << '\n';
            requiers_typage(espace, decl);
        }
    }
    else if (attente.est<AttenteSurDeclaration>()) {
        NoeudDeclaration *decl = attente.declaration();
        if (decl->unite == nullptr) {
            // std::cerr << "Requiers le typage de " << decl->ident->nom << '\n';
            requiers_typage(espace, decl);
        }
    }

    unite_attendante->mute_attente(attente);
    unites_en_attente.ajoute(unite_attendante);
}

void GestionnaireCode::symbole_defini(IdentifiantCode *ident)
{
    POUR (unites_en_attente.attentes) {
        if (it->attend_sur_symbole(ident)) {
            it->marque_prete();
        }
    }
}

void GestionnaireCode::chargement_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->donnees_constantes->fut_charge);

    // À FAIRE(gestion) : si tous les fichiers sont chargés, envoie un message, change l'état de
    // l'espace
    auto espace = unite->espace;
    espace->tache_chargement_terminee(m_compilatrice->messagere, unite->fichier);

    unite->mute_raison_d_etre(RaisonDEtre::LEXAGE_FICHIER);
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::lexage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->donnees_constantes->fut_lexe);

    auto espace = unite->espace;
    espace->tache_lexage_terminee(m_compilatrice->messagere);
    unite->mute_raison_d_etre(RaisonDEtre::PARSAGE_FICHIER);
    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::parsage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    // À FAIRE(gestion) : assert(unite->fichier->fut_parse);
    // À FAIRE(gestion) : si tous les fichiers sont parsés, envoie un message, change l'état de
    // l'espace
    auto espace = unite->espace;
    espace->tache_parsage_terminee(m_compilatrice->messagere);
}

#define ENUMERE_FONCTION_INTERFACE(O)                                                             \
    O(panique)                                                                                    \
    O(panique_hors_memoire)                                                                       \
    O(panique_depassement_limites_tableau)                                                        \
    O(panique_depassement_limites_chaine)                                                         \
    O(panique_membre_union)                                                                       \
    O(panique_erreur_non_geree)                                                                   \
    O(__rappel_panique_defaut)                                                                    \
    O(cree_contexte)                                                                              \
    O(DLS_vers_r32)                                                                               \
    O(DLS_vers_r64)                                                                               \
    O(DLS_depuis_r32)                                                                             \
    O(DLS_depuis_r64)

#define ENUMERE_TYPE_INTERFACE(O)                                                                 \
    O(InfoType)                                                                                   \
    O(InfoTypeEnum)                                                                               \
    O(InfoTypeStructure)                                                                          \
    O(InfoTypeUnion)                                                                              \
    O(InfoTypeMembreStructure)                                                                    \
    O(InfoTypeEntier)                                                                             \
    O(InfoTypeTableau)                                                                            \
    O(InfoTypePointeur)                                                                           \
    O(InfoTypeFonction)                                                                           \
    O(PositionCodeSource)                                                                         \
    O(InfoFonctionTraceAppel)                                                                     \
    O(TraceAppel)                                                                                 \
    O(BaseAllocatrice)                                                                            \
    O(InfoAppelTraceAppel)                                                                        \
    O(tockageTemporaire)                                                                          \
    O(InfoTypeOpaque)

static bool est_identifiant_interface(const IdentifiantCode *ident)
{
#define COMPARE_IDENT(ident_interface)                                                            \
    if (ident == ID::ident_interface) {                                                           \
        return true;                                                                              \
    }

    ENUMERE_FONCTION_INTERFACE(COMPARE_IDENT)

#undef COMPARE_IDENT
    return false;
}

void GestionnaireCode::marque_unites_dependantes_pretes(UniteCompilation *unite)
{
    if (unite->metaprogramme) {
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_metaprogramme(unite->metaprogramme)) {
                it->marque_prete();
            }
        }

        return;
    }

    auto noeud = unite->noeud;

    Atome **ri = nullptr;
    if (unite->est_pour_generation_ri() || unite->est_pour_generation_ri_principale_mp()) {
        if (noeud->est_corps_fonction()) {
            ri = &noeud->comme_corps_fonction()->entete->atome;
        }
        else if (noeud->est_entete_fonction()) {
            ri = &noeud->comme_entete_fonction()->atome;
        }
    }

    if (noeud->est_declaration() && est_identifiant_interface(noeud->ident)) {
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_interface_kuri(noeud->ident)) {
                /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il nous
                 * faut le code. */
                // std::cerr << "Marque prête une unité dépendant sur l'interface  " << noeud->ident->nom << '\n';
                if (noeud->ident == ID::cree_contexte) {
                    if (noeud->est_entete_fonction()) {
                        it->mute_attente(Attente::sur_ri(&noeud->comme_entete_fonction()->atome));
                    }
                }
                else {
                    it->marque_prete();
                }
            }
        }
    }

    if (noeud->est_entete_fonction() && noeud->ident) {
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_symbole(noeud->ident)) {
                // std::cerr << "Marque prête une unité dépendant sur le symbole " << noeud->ident->nom << '\n';
                it->marque_prete();
            }
        }
    }

    if (noeud->est_declaration()) {
        POUR (unites_en_attente.attentes) {
            if (!it->attend_sur_declaration(noeud->comme_declaration())) {
                continue;
            }

            // std::cerr << "Marque prête une unité dépendant sur la déclaration de " << noeud->ident->nom << '\n';
            if (noeud->ident == ID::cree_contexte) {
                if (noeud->est_entete_fonction()) {
                    it->mute_attente(Attente::sur_ri(&noeud->comme_entete_fonction()->atome));
                }
            }
            else {
                it->marque_prete();
            }
        }
    }

    if (noeud->est_structure() || noeud->est_enum()) {
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_type(noeud->type)) {
                it->marque_prete();
            }

            if (it->attend_sur_declaration(noeud->comme_declaration())) {
                // std::cerr << "Marque prête une unité dépendant sur la déclaration de " << noeud->ident->nom << '\n';
                it->marque_prete();
            }
        }
    }

    if (noeud->est_importe()) {
        auto importe = noeud->comme_importe();
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_symbole(importe->expression->ident)) {
                it->marque_prete();
            }
        }
    }

    if (ri) {
        POUR (unites_en_attente.attentes) {
            if (it->attend_sur_ri(ri)) {
                it->marque_prete();
            }
        }
    }
}

static bool noeud_requiers_generation_ri(NoeudExpression *noeud)
{
    if (noeud->est_entete_fonction()) {
        auto entete = noeud->comme_entete_fonction();
        /* La génération de RI pour les fonctions comprend également leurs coprs, or le corps
         * n'est peut-être pas encore typé. Les fonctions externes n'ayant pas de corps (même si le
         * pointeur vers le corps est valide), nous devons quand même les envoyer vers la RI afin
         * que leurs déclarations en RI soient disponibles. */
        return entete->est_externe;
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entete;
        return (!entete->est_polymorphe || entete->est_monomorphisation);
    }

    if (noeud->est_structure()) {
        auto structure = noeud->comme_structure();
        return !structure->est_polymorphe;
    }

    if (noeud->est_enum()) {
        return true;
    }

    if (noeud->possede_drapeau(EST_GLOBALE)) {
        if (noeud->est_execute()) {
            /* Les #exécutes globales sont gérées via les métaprogrammes. */
            return false;
        }
        return true;
    }

    if (noeud->possede_drapeau(EST_DECLARATION_TYPE_OPAQUE)) {
        return true;
    }

    return false;
}

static void imprime_evenement(UniteCompilation *unite, const char *evenement)
{
    std::cerr << evenement << " pour ";

    auto noeud = unite->noeud;
    if (noeud) {
        std::cerr << noeud->genre << ' ';

        if (noeud->est_entete_fonction() && noeud->comme_entete_fonction()->est_externe) {
            std::cerr << "(externe) ";
        }
    }
    std::cerr << ":\n";
    erreur::imprime_site(*unite->espace, unite->noeud);
}

void GestionnaireCode::typage_termine(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert_rappel(unite->noeud->possede_drapeau(DECLARATION_FUT_VALIDEE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " ne fut pas validé !\n";
        erreur::imprime_site(*unite->espace, unite->noeud);
    });

    // imprime_evenement(unite, "typage terminé");

    auto espace = unite->espace;

    // À FAIRE(gestion) : si toutes les unités requérant un typage dans l'espace sont typées,
    // envoie un message

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
    auto noeud = unite->noeud;
    if ((noeud->est_declaration() && !(noeud->est_charge() || noeud->est_importe())) ||
        noeud->est_corps_fonction() || noeud->est_execute()) {
        determine_dependances(unite->noeud, unite->espace, *graphe);
    }

    marque_unites_dependantes_pretes(unite);

    bool doit_envoyer_en_ri = false;

    if (noeud_requiers_generation_ri(noeud)) {
        espace->tache_ri_ajoutee(m_compilatrice->messagere);
        unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
        doit_envoyer_en_ri = true;
    }

#if 0  //  À FAIRE(gestion)
    if (noeud->est_corps_fonction() && noeud->comme_corps_fonction()->entete->ident == ID::principale) {
        imprime_evenement(unite, "typage terminé");

        if (doit_envoyer_en_ri) {
            imprime_evenement(unite, "ri requise");
        }
    }

    if (!noeud->est_execute()) {
        auto message_enfile = m_compilatrice->messagere->ajoute_message_typage_code(
            unite->espace, static_cast<NoeudDeclaration *>(noeud));

        if (message_enfile && doit_envoyer_en_ri) {
            mets_en_attente(unite, Attente::sur_message(message_enfile));
            doit_envoyer_en_ri = false;
        }
    }
#endif

    if (doit_envoyer_en_ri) {
        //imprime_evenement(unite, "ri requise");
        assert(unite->raison_d_etre() == RaisonDEtre::GENERATION_RI);
        unites_en_attente.ajoute(unite);
    }

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    espace->tache_typage_terminee(m_compilatrice->messagere);

#if 0 /* Ancienne logique en cas de non-complétion de la tâche. */
    if (unite->etat() != UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME &&
        espace->parsage_termine() &&
        (unite->index_courant == unite->index_precedent)) {
        unite->cycle += 1;
    }

    if (unite->index_courant > unite->index_precedent) {
        unite->cycle = 0;
    }

    unite->index_precedent = unite->index_courant;

    taches_typage.enfile(tache_terminee);
#endif
}

void GestionnaireCode::generation_ri_terminee(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert_rappel(unite->noeud->possede_drapeau(RI_FUT_GENEREE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " n'eu pas de RI générée !\n";
        erreur::imprime_site(*unite->espace, unite->noeud);
    });

    // À FAIRE(gestion) : si toutes les unités requérant un typage dans l'espace ont eu leurs RI
    // générées, envoie un message

    /* Marque les unités dépendantes du noeud comme prêtes. */
    marque_unites_dependantes_pretes(unite);

    auto espace = unite->espace;
    espace->tache_ri_terminee(m_compilatrice->messagere);
    if (espace->optimisations) {
//        tache_terminee.genre = GenreTache::OPTIMISATION;
//        taches_optimisation.enfile(tache_terminee);
    }

    POUR (programmes_en_cours) {
        auto espace_du_programme = it->espace();

        if (espace_du_programme->possede_erreur) {
            continue;
        }

        if (it->pour_metaprogramme()) {
            auto etat = it->ajourne_etat_compilation();

            if (etat.phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE) {
                it->change_de_phase(PhaseCompilation::AVANT_GENERATION_OBJET);
                requiers_generation_code_machine(espace_du_programme, it);
            }
        }
        else {
            if (espace->phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE &&
                    it->ri_generees()) {
                if (espace->options.resultat == ResultatCompilation::RIEN) {
                    espace->change_de_phase(m_compilatrice->messagere,
                                            PhaseCompilation::COMPILATION_TERMINEE);
                }
                else {
                    espace->change_de_phase(m_compilatrice->messagere,
                                            PhaseCompilation::AVANT_GENERATION_OBJET);
                    requiers_generation_code_machine(espace, espace->programme);
                }
            }
        }
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
    POUR (unites_en_attente.attentes) {
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
    marque_unites_dependantes_pretes(unite);
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

        if (espace->options.resultat == ResultatCompilation::EXECUTABLE) {
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

void GestionnaireCode::cree_taches(OrdonnanceuseTache &ordonnanceuse)
{
    FileDAttente nouvelles_unites;

    if (plus_rien_n_est_a_faire()) {
        ordonnanceuse.marque_compilation_terminee();
    }

    POUR (unites_en_attente.attentes) {
        it->marque_prete_si_attente_resolue();

        if (!it->est_prete()) {
//            imprime_evenement(unite, "remise dans la liste d'attente");
//            std::cerr << "-- attent sur    : " << unite->commentaire() << '\n';
//            std::cerr << "-- raison d'être : " << unite->raison_d_etre() << '\n';
//            std::cerr << "-- attente récursive : \n" << chaine_attentes_recursives(unite) << '\n';
            it->cycle += 1;

            if (it->cycle > 1000) {
                it->rapporte_erreur();
                // À FAIRE(gestion) : verrou mort pour l'effacement des tâches
                unites_en_attente.attentes.efface();
                ordonnanceuse.supprime_toutes_les_taches();
                return;
            }

            // if (!unite->est_bloquee()) {
                nouvelles_unites.ajoute(it);
                continue;
            //}
        }

        if (it->raison_d_etre() == RaisonDEtre::AUCUNE) {
            it->espace->rapporte_erreur_sans_site(
                "Erreur interne : obtenu une unité sans raison d'être");
            continue;
        }

        ordonnanceuse.cree_tache_pour_unite(it);
    }

    unites_en_attente = nouvelles_unites;
}

bool GestionnaireCode::plus_rien_n_est_a_faire() const
{
    if (!unites_en_attente.est_vide()) {
        return false;
    }

    POUR (programmes_en_cours) {
        /* Les programmes des métaprogrammes sont enlevés après leurs exécutions. Si nous en
         * avons un, la compilation ne peut se terminée. */
        if (it->pour_metaprogramme()) {
            return false;
        }

        /* Attend que tous les espaces eurent leur compilation terminée. */
        auto espace = it->espace();
        if (espace->phase_courante() != PhaseCompilation::COMPILATION_TERMINEE) {
            return false;
        }
    }

    return true;
}
