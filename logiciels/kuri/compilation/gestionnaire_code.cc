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

// À FAIRE(gestion) : retourne des Attentes depuis les fonction d'appariements d'appels ou
//                    d'opérateurs au lieu de true/false

// À FAIRE(gestion) : pour les dépendances, il nous faudrait plutôt les substitutions

// À FAIRE(gestion) : visite_noeud -> paramètre pour choisir les substitions, valeur de retour pour
//                    décider si nous devons visiter les enfants ou non

/* Traverse l'arbre syntaxique de la racine spécifiée et rassemble les fonctions, types, et
 * globales utilisées. */
static void rassemble_dependances(NoeudExpression *racine,
                                  EspaceDeTravail *espace,
                                  DonneesDependance &dependances)
{
    // À FAIRE(gestion) : pour les déclarations de variables, vérifie que la visite prend en compte
    //                    les expressions multiples

    // À FAIRE(gestion) : pour les structures ou unions, vérifie que les connexions sont bonnes
    //                    entre les types et les ceux des membres (type_compose.membre,
    //                    type_structure.types_employes)

    // À FAIRE(gestion) : l'utilisation d'un type_de_données brise le graphe de dépendance (les
    //                    membres des types structure ou énum sont des types_de_données)

    // À FAIRE(gestion) : pour les structures ou unions, les membres constants peuvent être des
    //                    structures ou unions

    // À FAIRE(gestion) : vérifie la traverse des variables des boucles pour

    // À FAIRE(gestion) : vérifie les dépendances pour les types tableaux ou union anymnome (p.e.:
    //                    []z32, r32 | r16), il faut utilisé le type, et non le type_de_données
    visite_noeud(racine, [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
        // Note: les fonctions polymorphiques n'ont pas de types.
        if (noeud->type) {
            dependances.types_utilises.insere(noeud->type);
        }

        if (noeud->est_entete_fonction()) {
            /* Visite manuellement les enfants des entêtes, car nous irions visiter le corps qui
             * ne fut pas encore typé. */
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

            if (decl->est_declaration_variable() && decl->possede_drapeau(EST_GLOBALE)) {
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
            /* Traite les indexages avant les expressions binaires afin de ne pas les traiter comme
             * tel (est_expression_binaire() retourne vrai pour les indexages). */

            auto indexage = noeud->comme_indexage();
            /* op peut être nul pour les déclaration de type ([]z32) */
            if (!indexage->op) {
                return DecisionVisiteNoeud::CONTINUE;
            }

            if (!indexage->op->est_basique) {
                dependances.fonctions_utilisees.insere(indexage->op->decl);
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
                    dependances.fonctions_utilisees.insere(interface->decl_panique_tableau);
                    break;
                }
                case GenreType::TABLEAU_FIXE:
                {
                    assert(interface->decl_panique_tableau);
                    if (indexage->aide_generation_code != IGNORE_VERIFICATION) {
                        dependances.fonctions_utilisees.insere(interface->decl_panique_tableau);
                    }
                    break;
                }
                case GenreType::CHAINE:
                {
                    assert(interface->decl_panique_chaine);
                    dependances.fonctions_utilisees.insere(interface->decl_panique_chaine);
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
                dependances.fonctions_utilisees.insere(comme->transformation.fonction);
            }
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
            auto type_tfixe = espace->typeuse.type_tableau_fixe(args->type, taille_tableau);
            dependances.types_utilises.insere(type_tfixe);
        }

        return DecisionVisiteNoeud::CONTINUE;
    });
}

/* Crée un noeud de dépendance pour le noeud spécifié en paramètre, et retourne un pointeur vers
 * celui-ci. Retourne nul si le noeud n'est pas supposé avoir un noeud de dépendance. */
static NoeudDependance *garantie_noeud_dependance(NoeudExpression *noeud, GrapheDependance &graphe)
{
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

/* Construit les dépendances de l'unité (fonctions, globales, types) et crée des unités de typage
 * pour chacune des dépendances non-encore typée. */
static void rassemble_dependances(UniteCompilation *unite,
                                  GrapheDependance &graphe,
                                  GestionnaireCode &gestionnaire)
{
    assert(unite->noeud);
    auto espace = unite->espace;
    auto noeud = unite->noeud;

    DonneesDependance dependances;
    rassemble_dependances(noeud, espace, dependances);

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

    NoeudDependance *noeud_dependance = garantie_noeud_dependance(noeud, graphe);
    graphe.ajoute_dependances(*noeud_dependance, dependances);

    garantie_typage_des_dependances(gestionnaire, dependances, espace);
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
        auto decl = decl_pour_type(type);
        if (decl && decl->unite == nullptr) {
            requiers_typage(espace, decl);
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
        if (it.unite->attend_sur_symbole(ident)) {
            it.unite->marque_prete();
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
    auto noeud = unite->noeud;
    if (noeud->est_entete_fonction() && est_identifiant_interface(noeud->ident)) {
        POUR (unites_en_attente.attentes) {
            auto unite_en_attente = it.unite;
            if (unite_en_attente->attend_sur_interface_kuri(noeud->ident)) {
                unite_en_attente->marque_prete();
            }
        }
    }

    if (noeud->est_declaration()) {
        POUR (unites_en_attente.attentes) {
            auto unite_en_attente = it.unite;
            if (unite_en_attente->attend_sur_declaration(noeud->comme_declaration())) {
                unite_en_attente->marque_prete();
            }
        }
    }

    if (noeud->est_structure() || noeud->est_enum()) {
        POUR (unites_en_attente.attentes) {
            auto unite_en_attente = it.unite;
            if (unite_en_attente->attend_sur_type(noeud->type)) {
                unite_en_attente->marque_prete();
            }
        }
    }

    if (noeud->est_importe()) {
        auto importe = noeud->comme_importe();
        POUR (unites_en_attente.attentes) {
            auto unite_en_attente = it.unite;
            if (unite_en_attente->attend_sur_symbole(importe->expression->ident)) {
                unite_en_attente->marque_prete();
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

    return true;
}

static void imprime_evenement(UniteCompilation *unite, const char *evenement)
{
    std::cerr << evenement << " pour :\n";

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
        noeud->est_corps_fonction()) {
        rassemble_dependances(unite, *graphe, *this);
    }

    marque_unites_dependantes_pretes(unite);

    bool doit_envoyer_en_ri = false;

    if (noeud_requiers_generation_ri(noeud)) {
        espace->tache_ri_ajoutee(m_compilatrice->messagere);
        unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
        doit_envoyer_en_ri = true;
    }

    if (!noeud->est_execute()) {
        auto message_enfile = m_compilatrice->messagere->ajoute_message_typage_code(
            unite->espace, static_cast<NoeudDeclaration *>(noeud));

        if (message_enfile && doit_envoyer_en_ri) {
            mets_en_attente(unite, Attente::sur_message(message_enfile));
            doit_envoyer_en_ri = false;
        }
    }

    if (doit_envoyer_en_ri) {
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
    assert(unite->noeud->possede_drapeau(RI_FUT_GENEREE));

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
        auto unite = it.unite;
        if (unite->attend_sur_message(message)) {
            unite->marque_prete();
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
}

void GestionnaireCode::cree_taches(OrdonnanceuseTache &ordonnanceuse)
{
    FileDAttente nouvelles_unites;

    POUR (unites_en_attente.attentes) {
        auto unite = it.unite;

        if (!unite->est_prete()) {
            nouvelles_unites.ajoute(unite);
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
                unite->espace->rapporte_erreur_sans_site(
                    "Erreur interne : obtenu une unité sans raison d'être");
            }
        }
    }

    unites_en_attente = nouvelles_unites;
}
