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

#include "compilatrice.hh"
#include "espace_de_travail.hh"

// À FAIRE(gestion) : retourne des Attentes depuis les fonction d'appariements d'appels ou d'opérateurs
//                    au lieu de true/false

// À FAIRE(gestion) : pour les dépendances, il nous faudrait plutôt les substitutions

// À FAIRE(gestion) : visite_noeud -> paramètre pour choisir les substitions, valeur de retour pour
//                    décider si nous devons visiter les enfants ou non

static void rassemble_dependances(NoeudExpression *racine,
                                  EspaceDeTravail *espace,
                                  DonneesDependance &donnees_dependances)
{
    // À FAIRE(gestion) : pour les déclarations de variables, vérifie que la visite prend en compte
    //                    les expressions multiples
    // À FAIRE(gestion) : pour les structures ou unions, vérifie que les connexions sont bonnes
    //                    entre les types et les ceux des membres (type_compose.membre, type_structure.types_employes)
    // À FAIRE(gestion) : l'utilisation d'un type_de_données brise le graphe de dépendance (les membres des structures de type structure ou énum sont des types_de_données)
    // À FAIRE(gestion) : pour les structures ou unions, les membres constants peuvent être des structures ou unions
    // À FAIRE(gestion) : vérifie la traverse des variables des boucles pour
    // À FAIRE(gestion) : vérifie les dépendances pour les types tableaux ou union anymnome (p.e.: []z32, r32 | r16), il faut utilisé le type, et non le type_de_données
    visite_noeud(racine, [&](NoeudExpression const *noeud) {
        // Note: les fonctions polymorphiques n'ont pas de types.
        if (noeud->type) {
            donnees_dependances.types_utilises.insere(noeud->type);
        }

        if (noeud->est_reference_declaration()) {
            auto ref = noeud->comme_reference_declaration();

            auto decl = ref->declaration_referee;
            if (decl->est_declaration_variable() && decl->possede_drapeau(EST_GLOBALE)) {
                donnees_dependances.globales_utilisees.insere(decl->comme_declaration_variable());
            }
            else if (decl->est_entete_fonction() && !decl->comme_entete_fonction()->est_polymorphe) {
                auto decl_fonc = decl->comme_entete_fonction();
                donnees_dependances.fonctions_utilisees.insere(decl_fonc);
            }
        }
        else if (noeud->est_cuisine()) {
            auto cuisine = noeud->comme_cuisine();
            auto expr = cuisine->expression;
            donnees_dependances.fonctions_utilisees.insere(expr->comme_appel()->expression->comme_entete_fonction());
        }
        else if (noeud->est_expression_binaire()) {
            auto expression_binaire = noeud->comme_expression_binaire();
            if (!expression_binaire->op->est_basique) {
                donnees_dependances.fonctions_utilisees.insere(expression_binaire->op->decl);
            }
        }
        else if (noeud->est_indexage()) {
            auto indexage = noeud->comme_indexage();
            if (!indexage->op->est_basique) {
                donnees_dependances.fonctions_utilisees.insere(indexage->op->decl);
            }

            /* Marque les dépendances sur les fonctions d'interface de kuri. */
            auto interface = espace->interface_kuri;

            /* Nous ne devrions pas avoir de référence ici, la validation sémantique s'est chargée
             * de transtyper automatiquement. */
            auto type_indexe = indexage->operande_gauche->type;
            if (type_indexe->est_opaque()) {
                type_indexe = type_indexe->comme_opaque()->type_opacifie;
            }

            switch (type_indexe->genre) {
                case GenreType::VARIADIQUE:
                case GenreType::TABLEAU_DYNAMIQUE:
                {
                    assert(interface->decl_panique_tableau);
                    donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_tableau);
                    break;
                }
                case GenreType::TABLEAU_FIXE:
                {
                    assert(interface->decl_panique_tableau);
                    if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
                        donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_tableau);
                    }
                    break;
                }
                case GenreType::CHAINE:
                {
                    assert(interface->decl_panique_chaine);
                    donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_chaine);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        else if (noeud->est_discr()) {
            auto discr = noeud->comme_discr();
            if (discr->op && !discr->op->est_basique) {
                donnees_dependances.fonctions_utilisees.insere(discr->op->decl);
            }
        }
        else if (noeud->est_construction_tableau()) {
            auto construction_tableau = noeud->comme_construction_tableau();
            donnees_dependances.types_utilises.insere(construction_tableau->type);

            /* Ajout également du type de pointeur pour la génération de code C. */
            auto type_feuille = construction_tableau->type->comme_tableau_fixe()->type_pointe;
            auto type_ptr = espace->typeuse.type_pointeur_pour(type_feuille);
            donnees_dependances.types_utilises.insere(type_ptr);
        }
        else if (noeud->est_tente()) {
            auto tente = noeud->comme_tente();

            if (!tente->expression_piegee) {
                auto interface = espace->interface_kuri;
                assert(interface->decl_panique_erreur);
                donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_erreur);
            }
        }
        else if (noeud->est_reference_membre_union()) {
            auto interface = espace->interface_kuri;
            assert(interface->decl_panique_membre_union);
            donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_membre_union);
        }
        else if (noeud->est_comme()) {
            auto comme = noeud->comme_comme();

            /* Marque les dépendances sur les fonctions d'interface de kuri. */
            auto interface = espace->interface_kuri;

            if (comme->transformation.type == TypeTransformation::EXTRAIT_UNION) {
                assert(interface->decl_panique_membre_union);
                // À FAIRE(gestion) : uniquement si l'union est nonsûre.
                donnees_dependances.fonctions_utilisees.insere(interface->decl_panique_membre_union);
            }
            else if (comme->transformation.type == TypeTransformation::FONCTION) {
                assert(comme->transformation.fonction);
                donnees_dependances.fonctions_utilisees.insere(comme->transformation.fonction);
            }
        }
        else if (noeud->est_appel()) {
            auto appel = noeud->comme_appel();

            if (appel->noeud_fonction_appelee) {
                donnees_dependances.fonctions_utilisees.insere(appel->noeud_fonction_appelee->comme_entete_fonction());
            }
        }
        else if (noeud->est_args_variadiques()) {
            auto args = noeud->comme_args_variadiques();

            /* Création d'un type tableau fixe, pour la génération de code. */
            auto taille_tableau = args->expressions.taille();
            auto type_tfixe = espace->typeuse.type_tableau_fixe(args->type, taille_tableau);
            donnees_dependances.types_utilises.insere(type_tfixe);
        }
    });
}

static void rassemble_dependances(UniteCompilation *unite, GrapheDependance &graphe, GestionnaireCode &gestionnaire)
{
    assert(unite->noeud);
    auto espace = unite->espace;
    auto noeud = unite->noeud;

    NoeudDependance *noeud_dependance = nullptr;

    DonneesDependance donnees_dependances;
    rassemble_dependances(noeud, espace, donnees_dependances);

    if (noeud->est_declaration_variable()) {
        assert(noeud->possede_drapeau(EST_GLOBALE));
        noeud_dependance = graphe.cree_noeud_globale(noeud->comme_declaration_variable());
    }
    else if (noeud->est_entete_fonction()) {
        noeud_dependance = graphe.cree_noeud_fonction(noeud->comme_entete_fonction());
    }
    else if (noeud->est_corps_fonction()) {
        auto corps = noeud->comme_corps_fonction();
        noeud_dependance = graphe.cree_noeud_fonction(corps->entete);
    }
    else if (noeud->est_execute()) {
        //auto execute = noeud->comme_execute();
        //noeud_dependance = graphe.cree_noeud_fonction(execute->)
        // À FAIRE(gestion) : ajout du métaprogramme au noeud d'exécution
    }
    else if (noeud->est_enum() || noeud->est_structure()) {
        noeud_dependance = graphe.cree_noeud_type(noeud->type);
    }
    else {
        assert(!"Noeud non géré pour les dépendances !\n");
        return;
    }

    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    dls::pour_chaque_element(donnees_dependances.fonctions_utilisees, [&](auto &fonction) {
        if (!fonction->corps->unite && !fonction->est_externe) {
            gestionnaire.requiers_typage(espace, fonction->corps);
        }
        return dls::DecisionIteration::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    dls::pour_chaque_element(donnees_dependances.globales_utilisees, [&](auto &globale) {
        if (!globale->unite) {
            gestionnaire.requiers_typage(espace, const_cast<NoeudExpression *>(globale));
        }
        return dls::DecisionIteration::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    dls::pour_chaque_element(donnees_dependances.types_utilises, [&](auto &type) {
        if (type->decl && !type->decl->unite) {
            // Pour les unions anonymes, nous allons directement à la génération de code RI.
            if (type->est_union() && type->comme_union()->est_anonyme) {
                gestionnaire.requiers_generation_ri(espace, type->decl);
            }
            else {
                gestionnaire.requiers_typage(espace, type->decl);
            }
        }
        return dls::DecisionIteration::Continue;
    });

    graphe.ajoute_dependances(*noeud_dependance, donnees_dependances);
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

void GestionnaireCode::requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    espace->tache_execution_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::EXECUTION);
    unite->metaprogramme = metaprogramme;

    metaprogramme->unite = unite;

    unites_en_attente.ajoute(unite);
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unite_attendante, Attente attente)
{
    assert(attente.est_valide());
    assert(unite_attendante->est_prete());

    // À FAIRE: vérifie que les types ou déclarations attendues ont une unité de compilation
    auto espace = unite_attendante->espace;

    if (attente.attend_sur_type) {
        Type *type = attente.attend_sur_type;

        if (type->est_enum() || type->est_erreur()) {
            if (static_cast<TypeEnum *>(type)->decl->unite == nullptr) {
                requiers_typage(espace, static_cast<TypeEnum *>(type)->decl);
            }
        }
        else if (type->est_structure()) {
            if (type->comme_structure()->decl->unite == nullptr) {
                requiers_typage(espace, type->comme_structure()->decl);
            }
        }
        else if (type->est_union()) {
            if (type->comme_union()->decl->unite == nullptr) {
                requiers_typage(espace, type->comme_union()->decl);
            }
        }
    }
    else if (attente.attend_sur_declaration) {
        NoeudDeclaration *decl = attente.attend_sur_declaration;
        if (decl->unite == nullptr) {
            requiers_typage(espace, decl);
        }
    }

    unite_attendante->mute_attente(attente);
    unites_en_attente.ajoute(unite_attendante);
}

void GestionnaireCode::symbole_defini(IdentifiantCode *ident)
{
    POUR (unites_en_attente.attentes) {
        if (it.unite->attend_sur_symbole() == ident) {
            it.unite->marque_prete();
        }
    }
}

void GestionnaireCode::chargement_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->donnees_constantes->fut_charge);

    // À FAIRE(gestion) : si tous les fichiers sont chargés, envoie un message, change l'état de l'espace

    requiers_lexage(unite->espace, unite->fichier);
}

void GestionnaireCode::parsage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    // À FAIRE(gestion) : assert(unite->fichier->fut_parse);
    // À FAIRE(gestion) : si tous les fichiers sont parsés, envoie un message, change l'état de l'espace
}

void GestionnaireCode::typage_termine(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert(unite->noeud->possede_drapeau(DECLARATION_FUT_VALIDEE));

    // À FAIRE(gestion) : si toutes les unités requérant un typage dans l'espace sont typées, envoie un message

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
    rassemble_dependances(unite, *graphe, *this);
}

void GestionnaireCode::generation_ri_terminee(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert(unite->noeud->possede_drapeau(RI_FUT_GENEREE));

    // À FAIRE(gestion) : si toutes les unités requérant un typage dans l'espace ont eu leurs RI générées, envoie un message
}

void GestionnaireCode::cree_taches(OrdonnanceuseTache &ordonnanceuse)
{
    POUR (unites_en_attente.attentes) {
        auto unite = it.unite;

        if (!unite->est_prete()) {
            continue;
        }

        switch (unite->raison_d_etre()) {
            case RaisonDEtre::CHARGEMENT_FICHIER:
            {
                ordonnanceuse.cree_tache_pour_chargement(unite);
                break;
            }
            case RaisonDEtre::LEXAGE_FICHIER:
            {
                ordonnanceuse.cree_tache_pour_lexage(unite);
                break;
            }
            case RaisonDEtre::PARSAGE_FICHIER:
            {
                ordonnanceuse.cree_tache_pour_parsage(unite);
                break;
            }
            case RaisonDEtre::TYPAGE:
            {
                ordonnanceuse.cree_tache_pour_typage(unite);
                break;
            }
            case RaisonDEtre::GENERATION_RI:
            {
                ordonnanceuse.cree_tache_pour_generation_ri(unite);
                break;
            }
            case RaisonDEtre::EXECUTION:
            {
                ordonnanceuse.cree_tache_pour_execution(unite);
                break;
            }
            case RaisonDEtre::AUCUNE:
            {
                unite->espace->rapporte_erreur_sans_site("Erreur interne : obtenu une unité sans raison d'être");
            }
        }
    }
}
