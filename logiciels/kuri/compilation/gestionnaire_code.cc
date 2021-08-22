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

/*
  À FAIRE(gestion) : pour chaque type :
- création d'une déclaration de type (ajout d'un noeud syntaxique: NoeudDéclarationType, d'où
dérivront les structures et les énums) (TACHE_CREATION_DECLARATION_TYPE)
- création d'une fonction d'initialisation (TACHE_CREATION_FONCTION_INITIALISATION)
- avoir un lexème sentinel pour l'impression des erreurs si le noeud est crée lors de la
compilation
 */

#if 0
#    include "arbre_syntaxique/assembleuse.hh"

void cree_noeud_initialisation_type(Type *type, AssembleuseArbre *assembleuse, Typeuse &typeuse)
{
    static Lexeme lexeme_entete = {};
    static Lexeme lexeme_corps = {};
    auto entete = assembleuse->cree_entete_fonction(&lexeme_entete);
    auto corps = assembleuse->cree_corps_fonction(&lexeme_corps);

    corps->entete = entete;
    entete->corps = corps;

    entete->drapeaux |= FORCE_ENLIGNE;

    assembleuse->bloc_courant(corps->bloc);

    // À FAIRE : type de la fonction

    // À FAIRE : paramètres

    static Lexeme lexeme_decl = {};
    auto decl_param = assembleuse->cree_declaration_variable(
        &lexeme_decl, typeuse.type_pointeur_pour(type), ID::pointeur, nullptr);

    auto ref_param = assembleuse->cree_reference_declaration(&lexeme_decl, decl_param);

    switch (type->genre) {
        case GenreType::RIEN:
        case GenreType::POLYMORPHIQUE:
        {
            break;
        }
        case GenreType::EINI:
        case GenreType::CHAINE:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        {
            static Lexeme lexeme = {};
            auto type_compose = static_cast<TypeCompose *>(type);

            auto index_membre = 0;
            POUR (type_compose->membres) {
                if ((it.drapeaux & TypeCompose::Membre::EST_CONSTANT) == 0) {
                    // *param.membre;
                    auto ref_membre = assembleuse->cree_reference_membre(
                        &lexeme, ref_param, it.type, index_membre);
                    static Lexeme lexeme_fois_unaire = {};
                    lexeme_fois_unaire.genre = GenreLexeme::FOIS_UNAIRE;
                    auto prise_adresse = assembleuse->cree_expression_unaire(&lexeme_fois_unaire);
                    prise_adresse->operande = ref_membre;

                    // initialise_XXXX(*param.membre);

                    auto appel = assembleuse->cree_appel(&lexeme, , typeuse[TypeBase::RIEN]);
                }
                index_membre += 1;
            }

            break;
        }
        case GenreType::BOOL:
        {
            static Lexeme litteral_bool = {};
            litteral_bool.genre = GenreLexeme::FAUX;
            auto valeur_defaut = assembleuse->cree_litterale_bool(&litteral_bool);
            break;
        }
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        case GenreType::TYPE_DE_DONNEES:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = assembleuse->cree_litterale_entier(&litteral, type, 0);
            break;
        }
        case GenreType::REEL:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = assembleuse->cree_litterale_reel(&litteral, type, 0);
            break;
        }
        case GenreType::REFERENCE:
        {
            break;
        }
        case GenreType::POINTEUR:
        case GenreType::FONCTION:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = assembleuse->cree_litterale_nul(&litteral);
            break;
        }
        case GenreType::UNION:
        {
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            // il nous faut créer une boucle sur la taille du tableau.
            // pour 0 ... taille - 1 { initialise_(*tableau[it]); }
            static Lexeme lexeme = {};
            auto pour = assembleuse->cree_pour(&lexeme);
            pour->expression = ;
            pour->bloc = assembleuse->cree_bloc(&lexeme);

            auto mem = assembleuse->cree_memoire(&lexeme);
            mem->expression = ref_param;

            auto ref_it = assembleuse->cree_reference_declaration(&lexeme);
            ref_it->ident = ID::it;

            auto indexage = assembleuse->cree_indexage(&lexeme, mem, ref_it, true);

            auto prise_mem = assembleuse->cree_expression_unaire(&lexeme);

            // initialise_XXXX(*tableau[it]);
            auto appel = assembleuse->cree_appel(&lexeme, , typeuse[TypeBase::RIEN]);

            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            static Lexeme litteral = {};
            auto type_enum = static_cast<TypeEnum *>(type);
            auto valeur_defaut = assembleuse->cree_litterale_entier(
                &litteral, type_enum->type_donnees, 0);
            break;
        }
        case GenreType::OPAQUE:
        {
            break;
        }
        case GenreType::TUPLE:
        {
            break;
        }
    }
}
#endif

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
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        programme.ajoute_fonction(const_cast<NoeudDeclarationEnteteFonction *>(fonction));
        return kuri::DecisionIteration::Continue;
    });

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
}

/* Traverse l'arbre syntaxique de la racine spécifiée et rassemble les fonctions, types, et
 * globales utilisées. */
static void rassemble_dependances(NoeudExpression *racine,
                                  EspaceDeTravail *espace,
                                  DonneesDependance &dependances)
{
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
                    auto type_union = comme->expression->type->comme_union();
                    if (!type_union->est_nonsure) {
                        dependances.fonctions_utilisees.insere(
                            interface->decl_panique_membre_union);
                    }
                }
                else if (comme->transformation.type == TypeTransformation::FONCTION) {
                    assert(comme->transformation.fonction);
                    dependances.fonctions_utilisees.insere(
                        const_cast<NoeudDeclarationEnteteFonction *>(
                            comme->transformation.fonction));
                }

                /* Nous avons besoin d'un type pointeur pour le type cible pour la génération de
                 * RI. À FAIRE: généralise pour toutes les variables. */
                if (comme->transformation.type_cible) {
                    auto type_pointeur = espace->typeuse.type_pointeur_pour(
                        comme->transformation.type_cible, false, false);
                    dependances.types_utilises.insere(type_pointeur);
                    dependances.types_utilises.insere(comme->transformation.type_cible);
                }
            }
            else if (noeud->est_appel()) {
                auto appel = noeud->comme_appel();
                auto appelee = appel->noeud_fonction_appelee;

                if (appelee) {
                    if (appelee->est_entete_fonction()) {
                        dependances.fonctions_utilisees.insere(appelee->comme_entete_fonction());
                    }
                    else if (appelee->est_structure()) {
                        dependances.types_utilises.insere(appelee->type);
                    }
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
            // Pour les unions anonymes, nous allons directement à la génération de code RI.
            if (type->est_union() && type->comme_union()->est_anonyme) {
                gestionnaire.requiers_generation_ri(espace, decl);
            }
            else {
                gestionnaire.requiers_typage(espace, decl);
            }
        }
        return kuri::DecisionIteration::Continue;
    });
}

/* Traverse le graphe de dépendances pour chaque type présents dans les dépendances courantes, et
 * ajoutes les dépedances de ces types aux dépendances.
 * Le but de cette fonction est de s'assurer que toutes les dépendances des types sont ajoutées aux
 * programmes. */
static void epends_dependances_types(GrapheDependance &graphe,
                                     DonnneesResolutionDependances &donnees_resolution)
{
    auto &dependances = donnees_resolution.dependances;
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
    dependances.reinitialise();
    rassemble_dependances(noeud, espace, dependances.dependances);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    NoeudDependance *noeud_dependance = garantie_noeud_dependance(noeud, graphe);
    graphe.ajoute_dependances(*noeud_dependance, dependances.dependances);

    epends_dependances_types(graphe, dependances);

    /* Ajoute les racines aux programmes courants de l'espace. */
    if (noeud->est_entete_fonction() && noeud->possede_drapeau(EST_RACINE)) {
        POUR (programmes_en_cours) {
            if (it->espace() != espace) {
                continue;
            }
            it->ajoute_racine(noeud->comme_entete_fonction());
        }
    }

    /* Ajoute les dépendances au programme si nécessaire. */
    auto dependances_ajoutees = false;
    POUR (programmes_en_cours) {
        if (doit_ajouter_les_dependances_au_programme(noeud, it)) {
            ajoute_dependances_au_programme(dependances.dependances, *it);
            dependances_ajoutees = true;
        }
    }

    /* Crée les unités de typage si nécessaire. */
    if (dependances_ajoutees) {
        garantie_typage_des_dependances(*this, dependances.dependances, espace);
    }
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

void GestionnaireCode::requiers_generation_ri_principale_metaprogramme(
    EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    espace->tache_ri_ajoutee(m_compilatrice->messagere);

    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI_PRINCIPALE_MP);
    unite->noeud = metaprogramme->fonction;

    metaprogramme->fonction->unite = unite;

    unites_en_attente.ajoute(unite);
}

UniteCompilation *GestionnaireCode::cree_unite_pour_message(EspaceDeTravail *espace,
                                                            Message *message)
{
    auto unite = unites.ajoute_element(espace);

    unite->mute_raison_d_etre(RaisonDEtre::ENVOIE_MESSAGE);
    unite->message = message;

    unites_en_attente.ajoute(unite);
    return unite;
}

void GestionnaireCode::requiers_noeud_code(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    auto unite = unites.ajoute_element(espace);

    unite->mute_raison_d_etre(RaisonDEtre::CONVERSION_NOEUD_CODE);
    unite->noeud = noeud;

    unites_en_attente.ajoute(unite);
}

std::optional<Attente> GestionnaireCode::tente_de_garantir_presence_creation_contexte(
    EspaceDeTravail *espace, Programme *programme, GrapheDependance &graphe)
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

    if (!decl_creation_contexte->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
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

    /* Indique directement à l'espace qu'une exécution sera requise afin de ne pas terminer la
     * compilation trop rapidement si le métaprogramme modifie ses options de compilation. */
    espace->tache_execution_ajoutee(m_compilatrice->messagere);

    /* Ajoute le programme à la liste des programmes avant de traiter les dépendances. */
    metaprogramme_cree(metaprogramme);

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

    if (metaprogramme->corps_texte) {
        if (metaprogramme->corps_texte_pour_fonction) {
            auto recipiente = metaprogramme->corps_texte_pour_fonction;
            assert(!recipiente->corps->unite);
            requiers_typage(espace, recipiente->corps);

            /* Crée un fichier pour le métaprogramme, et fait dépendre le corps de la fonction
             * recipiente du #corps_texte sur le parsage du fichier. Nous faisons ça ici pour nous
             * assurer que personne n'essayera de performer le typage du corps recipient avant que
             * les sources du fichiers ne soient générées, lexées, et parsées. */
            auto fichier = espace->cree_fichier_pour_metaprogramme(metaprogramme);
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
            requiers_typage(espace, decl);
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
    assert(unite->fichier->donnees_constantes->fut_charge);

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
        /* Pour les métaprogrammes, la RI doit se faire via requiers_compilation_metaprogramme. Il
         * est possible que les métaprogrammes arrivent ici après le typage, notamment pour les
         * #corps_textes.
         */
        return !entete->est_metaprogramme &&
               (!entete->est_polymorphe || entete->est_monomorphisation);
    }

    if (noeud->est_structure()) {
        auto structure = noeud->comme_structure();
        // Les déclarations de types polymorphiques sont dans le Programme...
        if (structure->est_polymorphe) {
            structure->type->drapeaux |= RI_TYPE_FUT_GENEREE;
        }
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

void GestionnaireCode::typage_termine(UniteCompilation *unite)
{
    assert(unite->noeud);
    assert_rappel(unite->noeud->possede_drapeau(DECLARATION_FUT_VALIDEE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " ne fut pas validé !\n";
        erreur::imprime_site(*unite->espace, unite->noeud);
    });

    auto espace = unite->espace;

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = unite->espace->graphe_dependance.verrou_ecriture();
    auto noeud = unite->noeud;
    if ((noeud->est_declaration() && !(noeud->est_charge() || noeud->est_importe())) ||
        noeud->est_corps_fonction() || noeud->est_execute()) {
        determine_dependances(unite->noeud, unite->espace, *graphe);
    }

    /* Envoi un message, nous attendrons dessus si nécessaire. */
    const auto message = m_compilatrice->messagere->ajoute_message_typage_code(espace, noeud);
    const auto doit_envoyer_en_ri = noeud_requiers_generation_ri(noeud);
    if (doit_envoyer_en_ri) {
        espace->tache_ri_ajoutee(m_compilatrice->messagere);
        unite->mute_raison_d_etre(RaisonDEtre::GENERATION_RI);
        unites_en_attente.ajoute(unite);
        if (message) {
            requiers_noeud_code(espace, noeud);
            auto unite_message = cree_unite_pour_message(espace, message);
            unite_message->mute_attente(Attente::sur_noeud_code(&noeud->noeud_code));
            unite->mute_attente(Attente::sur_message(message));
        }
    }

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    espace->tache_typage_terminee(m_compilatrice->messagere);
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

void GestionnaireCode::cree_taches(OrdonnanceuseTache &ordonnanceuse)
{
    kuri::tableau<UniteCompilation *> nouvelles_unites;

    if (plus_rien_n_est_a_faire()) {
        ordonnanceuse.marque_compilation_terminee();
        return;
    }

    POUR (unites_en_attente) {
        it->marque_prete_si_attente_resolue();

        if (!it->est_prete()) {
            it->cycle += 1;

            if (it->est_bloquee()) {
                it->rapporte_erreur();
                // À FAIRE(gestion) : verrou mort pour l'effacement des tâches
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

        ordonnanceuse.cree_tache_pour_unite(it);
    }

    unites_en_attente = nouvelles_unites;
}

bool GestionnaireCode::plus_rien_n_est_a_faire()
{
    POUR (programmes_en_cours) {
        auto espace = it->espace();

        /* Vérifie si une erreur existe. */
        if (espace->possede_erreur && !espace->options.continue_si_erreur) {
            m_compilatrice->messagere->purge_messages();
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);
            return true;
        }

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
                if (espace->options.resultat == ResultatCompilation::RIEN) {
                    espace->change_de_phase(m_compilatrice->messagere,
                                            PhaseCompilation::COMPILATION_TERMINEE);
                }
                else if (espace->peut_generer_code_final()) {
                    espace->change_de_phase(m_compilatrice->messagere,
                                            PhaseCompilation::AVANT_GENERATION_OBJET);
                    requiers_generation_code_machine(espace, espace->programme);
                }
            }
        }
    }

    if (!unites_en_attente.est_vide()) {
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
