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

/*
  À FAIRE(gestion) : pour chaque type :
- création d'une déclaration de type (ajout d'un noeud syntaxique: NoeudDéclarationType, d'où
dérivront les structures et les énums) (TACHE_CREATION_DECLARATION_TYPE)
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

#define TACHE_AJOUTEE(genre) espace->tache_ajoutee(GenreTache::genre, m_compilatrice->messagere)
#define TACHE_TERMINEE(genre, envoyer_changement_de_phase)                                        \
    espace->tache_terminee(                                                                       \
        GenreTache::genre, m_compilatrice->messagere, envoyer_changement_de_phase)

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
    const int index[3] = {int(RaisonDEtre::CHARGEMENT_FICHIER),
                          int(RaisonDEtre::LEXAGE_FICHIER),
                          int(RaisonDEtre::PARSAGE_FICHIER)};

    /* Vérifie s'il n'y a pas d'instructions de chargement ou d'import à gérer et que les unités de
     * chargement/lexage/parsage ont toutes été traitées. */
    return file_unités_charge_ou_importe == nullptr &&
           std::all_of(std::begin(index), std::end(index), [&](int it) {
               return nombre_d_unités_pour_raison[it].compte == 0;
           });
}

void ÉtatChargementFichiers::imprime_état() const
{
    std::cerr << "--------------------------------------------\n";
    std::cerr << nombre_d_unités_pour_raison[int(RaisonDEtre::CHARGEMENT_FICHIER)].compte
              << " fichier(s) à charger\n";
    std::cerr << nombre_d_unités_pour_raison[int(RaisonDEtre::LEXAGE_FICHIER)].compte
              << " fichier(s) à lexer\n";
    std::cerr << nombre_d_unités_pour_raison[int(RaisonDEtre::PARSAGE_FICHIER)].compte
              << " fichier(s) à parser\n";

    std::cerr << "File d'attente chargement est vide : "
              << (file_unités_charge_ou_importe == nullptr) << '\n';
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

void GestionnaireCode::espace_cree(EspaceDeTravail *espace)
{
    std::unique_lock verrou(m_mutex);
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

    if (noeud->possède_drapeau(DrapeauxNoeud::EST_CONSTANTE)) {
        return false;
    }

    return noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE);
}

static bool ajoute_dependances_au_programme(GrapheDependance &graphe,
                                            DonnneesResolutionDependances &données_dependances,
                                            EspaceDeTravail *espace,
                                            Programme &programme,
                                            NoeudExpression *noeud)
{
    auto possède_erreur = false;
    auto &dependances = données_dependances.dependances;

    /* Ajoute les fonctions. */
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
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

        programme.ajoute_fonction(const_cast<NoeudDeclarationEnteteFonction *>(fonction));
        return kuri::DécisionItération::Continue;
    });

    if (possède_erreur) {
        return false;
    }

    /* Ajoute les globales. */
    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        programme.ajoute_globale(const_cast<NoeudDeclarationVariable *>(globale));
        return kuri::DécisionItération::Continue;
    });

    /* Ajoute les types. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        programme.ajoute_type(type, RaisonAjoutType::DÉPENDANCE_DIRECTE, noeud);
        return kuri::DécisionItération::Continue;
    });

    auto dependances_manquantes = programme.dépendances_manquantes();
    programme.dépendances_manquantes().efface();

    graphe.prepare_visite();

    dependances_manquantes.pour_chaque_element([&](NoeudDeclaration *decl) {
        auto noeud_dep = graphe.garantie_noeud_dépendance(espace, decl);

        graphe.traverse(noeud_dep, [&](NoeudDependance const *relation) {
            if (relation->est_fonction()) {
                programme.ajoute_fonction(relation->fonction());
                données_dependances.dependances_ependues.fonctions_utilisees.insere(
                    relation->fonction());
            }
            else if (relation->est_globale()) {
                programme.ajoute_globale(relation->globale());
                données_dependances.dependances_ependues.globales_utilisees.insere(
                    relation->globale());
            }
            else if (relation->est_type()) {
                programme.ajoute_type(
                    relation->type(), RaisonAjoutType::DÉPENDACE_INDIRECTE, decl);
                données_dependances.dependances_ependues.types_utilises.insere(relation->type());
            }
        });
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

    auto rassemble_dependances_transformations =
        [&](kuri::tableau_compresse<TransformationType, int> const &transformations, Type *type) {
            POUR (transformations.plage()) {
                rassemble_dependances_transformation(it, type);
            }
        };

    visite_noeud(
        racine,
        PreferenceVisiteNoeud::SUBSTITUTION,
        true,
        [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
            /* N'ajoutons pas de dépendances sur les déclarations de types nichées. */
            if ((noeud->est_type_structure() || noeud->est_type_enum()) && noeud != racine) {
                return DecisionVisiteNoeud::IGNORE_ENFANTS;
            }

            /* Ne faisons pas dépendre les types d'eux-mêmes. */
            /* Note: les fonctions polymorphiques n'ont pas de types. */
            if (!(noeud->est_type_structure() || noeud->est_type_enum()) && noeud->type) {
                if (noeud->type->est_type_type_de_donnees()) {
                    auto type_de_donnees = noeud->type->comme_type_type_de_donnees();
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
                         !decl->comme_entete_fonction()->possède_drapeau(
                             DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
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
                if (type_indexe->est_type_opaque()) {
                    type_indexe = type_indexe->comme_type_opaque()->type_opacifie;
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
                auto type_feuille =
                    construction_tableau->type->comme_type_tableau_fixe()->type_pointe;
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
                    else if (appelee->est_type_structure()) {
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

/* Requiers le typage de toutes les dépendances. */
static void garantie_typage_des_dependances(GestionnaireCode &gestionnaire,
                                            DonneesDependance const &dependances,
                                            EspaceDeTravail *espace)
{
    /* Requiers le typage du corps de toutes les fonctions utilisées. */
    kuri::pour_chaque_element(dependances.fonctions_utilisees, [&](auto &fonction) {
        if (!fonction->corps->unité &&
            !fonction->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE |
                                       DrapeauxNoeudFonction::EST_EXTERNE)) {
            gestionnaire.requiers_typage(espace, fonction->corps);
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de toutes les déclarations utilisées. */
    kuri::pour_chaque_element(dependances.globales_utilisees, [&](auto &globale) {
        if (!globale->unité) {
            gestionnaire.requiers_typage(espace, const_cast<NoeudDeclarationVariable *>(globale));
        }
        return kuri::DécisionItération::Continue;
    });

    /* Requiers le typage de tous les types utilisés. */
    kuri::pour_chaque_element(dependances.types_utilises, [&](auto &type) {
        auto decl = decl_pour_type(type);
        if (decl && !decl->comme_declaration_type()->unité) {
            // Inutile de typer les unions anonymes, ceci fut fait lors de la validation
            // sémantique.
            if (!(type->est_type_union() && type->comme_type_union()->est_anonyme) &&
                !(type->est_type_structure() && type->comme_type_structure()->union_originelle)) {
                gestionnaire.requiers_typage(espace, decl);
            }
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
                (type->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
                calcule_taille_type_compose(type_retour->comme_type_tuple(), false, 0);
            }
        }

        return kuri::DécisionItération::Continue;
    });
}

/* Détermine si nous devons ajouter les dépendances du noeud au programme. */
static bool doit_ajouter_les_dependances_au_programme(NoeudExpression *noeud, Programme *programme)
{
    if (noeud->est_entete_fonction()) {
        return programme->possède(noeud->comme_entete_fonction());
    }

    if (noeud->est_corps_fonction()) {
        auto entete = noeud->comme_corps_fonction()->entete;
        return programme->possède(entete);
    }

    if (noeud->est_declaration_variable()) {
        return programme->possède(noeud->comme_declaration_variable());
    }

    if (noeud->est_type_structure() || noeud->est_type_enum()) {
        return programme->possède(noeud->type);
    }

    if (noeud->est_ajoute_fini() || noeud->est_ajoute_init()) {
        return !programme->pour_métaprogramme();
    }

    return false;
}

/* Construit les dépendances de l'unité (fonctions, globales, types) et crée des unités de typage
 * pour chacune des dépendances non-encore typée. */
void GestionnaireCode::determine_dependances(NoeudExpression *noeud,
                                             EspaceDeTravail *espace,
                                             GrapheDependance &graphe)
{
    DÉBUTE_STAT(DÉTERMINE_DÉPENDANCES);
    dependances.reinitialise();

    DÉBUTE_STAT(RASSEMBLE_DÉPENDANCES);
    rassemble_dependances(noeud, m_compilatrice, dependances.dependances);
    TERMINE_STAT(RASSEMBLE_DÉPENDANCES);

    /* Ajourne le graphe de dépendances avant de les épendres, afin de ne pas ajouter trop de
     * relations dans le graphe. */
    if (!noeud->est_ajoute_fini() && !noeud->est_ajoute_init()) {
        DÉBUTE_STAT(AJOUTE_DÉPENDANCES);
        NoeudDependance *noeud_dependance = graphe.garantie_noeud_dépendance(espace, noeud);
        graphe.ajoute_dependances(*noeud_dependance, dependances.dependances);
        TERMINE_STAT(AJOUTE_DÉPENDANCES);
    }

    /* Ajoute les racines aux programmes courants de l'espace. */
    if (noeud->est_entete_fonction() &&
        noeud->comme_entete_fonction()->possède_drapeau(DrapeauxNoeudFonction::EST_RACINE)) {
        DÉBUTE_STAT(AJOUTE_RACINES);
        auto entete = noeud->comme_entete_fonction();
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
    auto dependances_ajoutees = false;
    POUR (programmes_en_cours) {
        if (!doit_ajouter_les_dependances_au_programme(noeud, it)) {
            continue;
        }
        if (!ajoute_dependances_au_programme(graphe, dependances, espace, *it, noeud)) {
            break;
        }
        dependances_ajoutees = true;
    }

    /* Crée les unités de typage si nécessaire. */
    if (dependances_ajoutees) {
        DÉBUTE_STAT(GARANTIE_TYPAGE_DÉPENDANCES);
        dependances.dependances.fusionne(dependances.dependances_ependues);
        garantie_typage_des_dependances(*this, dependances.dependances, espace);
        TERMINE_STAT(GARANTIE_TYPAGE_DÉPENDANCES);
    }
    TERMINE_STAT(DÉTERMINE_DÉPENDANCES);
    noeud->drapeaux |= DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES;
}

UniteCompilation *GestionnaireCode::crée_unite(EspaceDeTravail *espace,
                                               RaisonDEtre raison,
                                               bool met_en_attente)
{
    auto unite = unites.ajoute_element(espace);
    unite->mute_raison_d_être(raison);
    if (met_en_attente) {
        ajoute_unité_à_liste_attente(unite);
    }
    return unite;
}

UniteCompilation *GestionnaireCode::crée_unite_pour_fichier(EspaceDeTravail *espace,
                                                            Fichier *fichier,
                                                            RaisonDEtre raison)
{
    auto unite = crée_unite(espace, raison, true);
    unite->fichier = fichier;
    return unite;
}

UniteCompilation *GestionnaireCode::crée_unite_pour_noeud(EspaceDeTravail *espace,
                                                          NoeudExpression *noeud,
                                                          RaisonDEtre raison,
                                                          bool met_en_attente)
{
    auto unite = crée_unite(espace, raison, met_en_attente);
    unite->noeud = noeud;
    *donne_adresse_unité(noeud) = unite;
    return unite;
}

void GestionnaireCode::requiers_chargement(EspaceDeTravail *espace, Fichier *fichier)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(CHARGEMENT);
    auto unité = crée_unite_pour_fichier(espace, fichier, RaisonDEtre::CHARGEMENT_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_lexage(EspaceDeTravail *espace, Fichier *fichier)
{
    std::unique_lock verrou(m_mutex);
    assert(fichier->fut_chargé);
    TACHE_AJOUTEE(LEXAGE);
    auto unité = crée_unite_pour_fichier(espace, fichier, RaisonDEtre::LEXAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_parsage(EspaceDeTravail *espace, Fichier *fichier)
{
    std::unique_lock verrou(m_mutex);
    assert(fichier->fut_lexé);
    TACHE_AJOUTEE(PARSAGE);
    auto unité = crée_unite_pour_fichier(espace, fichier, RaisonDEtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.ajoute_unité_pour_chargement_fichier(unité);
}

void GestionnaireCode::requiers_typage(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(TYPAGE);
    crée_unite_pour_noeud(espace, noeud, RaisonDEtre::TYPAGE, true);
}

void GestionnaireCode::requiers_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(GENERATION_RI);
    crée_unite_pour_noeud(espace, noeud, RaisonDEtre::GENERATION_RI, true);
}

void GestionnaireCode::requiers_generation_ri_principale_metaprogramme(
    EspaceDeTravail *espace, MetaProgramme *metaprogramme, bool peut_planifier_compilation)
{
    std::unique_lock verrou(m_mutex);
    TACHE_AJOUTEE(GENERATION_RI);

    auto unite = crée_unite_pour_noeud(espace,
                                       metaprogramme->fonction,
                                       RaisonDEtre::GENERATION_RI_PRINCIPALE_MP,
                                       peut_planifier_compilation);

    if (!peut_planifier_compilation) {
        assert(metaprogrammes_en_attente_de_crée_contexte_est_ouvert);
        metaprogrammes_en_attente_de_crée_contexte.ajoute(unite);
    }
}

UniteCompilation *GestionnaireCode::crée_unite_pour_message(EspaceDeTravail *espace,
                                                            Message *message)
{
    auto unite = crée_unite(espace, RaisonDEtre::ENVOIE_MESSAGE, true);
    unite->message = message;
    return unite;
}

void GestionnaireCode::requiers_initialisation_type(EspaceDeTravail *espace, Type *type)
{
    std::unique_lock verrou(m_mutex);
    if ((type->drapeaux & INITIALISATION_TYPE_FUT_REQUISE) != 0) {
        return;
    }

    if (!requiers_création_fonction_initialisation(type)) {
        return;
    }

    if (m_validation_doit_attendre_sur_lexage) {
        m_fonctions_init_type_requises.ajoute({espace, type});
        return;
    }

    type->drapeaux |= INITIALISATION_TYPE_FUT_REQUISE;

    auto unite = crée_unite(espace, RaisonDEtre::CREATION_FONCTION_INIT_TYPE, true);
    unite->type = type;

    if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
        unite->ajoute_attente(Attente::sur_type(type));
    }

    type->drapeaux |= UNITE_POUR_INITIALISATION_FUT_CREE;
}

UniteCompilation *GestionnaireCode::requiers_noeud_code(EspaceDeTravail *espace,
                                                        NoeudExpression *noeud)
{
    std::unique_lock verrou(m_mutex);
    auto unite = crée_unite(espace, RaisonDEtre::CONVERSION_NOEUD_CODE, true);
    unite->noeud = noeud;
    return unite;
}

void GestionnaireCode::ajoute_unité_à_liste_attente(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::EN_ATTENTE);
    unites_en_attente.ajoute_aux_données_globales(unité);
}

bool GestionnaireCode::tente_de_garantir_presence_creation_contexte(EspaceDeTravail *espace,
                                                                    Programme *programme,
                                                                    GrapheDependance &graphe)
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

    determine_dependances(decl_creation_contexte, espace, graphe);

    if (!decl_creation_contexte->corps->unité) {
        requiers_typage(espace, decl_creation_contexte->corps);
        return false;
    }

    if (!decl_creation_contexte->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    determine_dependances(decl_creation_contexte->corps, espace, graphe);

    if (!decl_creation_contexte->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        return false;
    }

    return true;
}

void GestionnaireCode::requiers_compilation_metaprogramme(EspaceDeTravail *espace,
                                                          MetaProgramme *metaprogramme)
{
    assert(metaprogramme->fonction);
    assert(metaprogramme->fonction->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));

    /* Indique directement à l'espace qu'une exécution sera requise afin de ne pas terminer la
     * compilation trop rapidement si le métaprogramme modifie ses options de compilation. */
    TACHE_AJOUTEE(EXECUTION);
    m_requêtes_compilations_métaprogrammes->ajoute({espace, metaprogramme});
}

void GestionnaireCode::requiers_compilation_metaprogramme_impl(EspaceDeTravail *espace,
                                                               MetaProgramme *metaprogramme)
{
    /* Ajoute le programme à la liste des programmes avant de traiter les dépendances. */
    metaprogramme_cree(metaprogramme);

    auto programme = metaprogramme->programme;
    programme->ajoute_fonction(metaprogramme->fonction);

    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();
    determine_dependances(metaprogramme->fonction, espace, *graphe);
    determine_dependances(metaprogramme->fonction->corps, espace, *graphe);

    auto ri_crée_contexte_est_disponible = tente_de_garantir_presence_creation_contexte(
        espace, programme, *graphe);
    requiers_generation_ri_principale_metaprogramme(
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

void GestionnaireCode::requiers_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme)
{
    auto unite = crée_unite(espace, RaisonDEtre::EXECUTION, true);
    unite->metaprogramme = metaprogramme;
    metaprogramme->unite = unite;
}

UniteCompilation *GestionnaireCode::requiers_generation_code_machine(EspaceDeTravail *espace,
                                                                     Programme *programme)
{
    std::unique_lock verrou(m_mutex);
    auto unite = crée_unite(espace, RaisonDEtre::GENERATION_CODE_MACHINE, true);
    unite->programme = programme;
    TACHE_AJOUTEE(GENERATION_CODE_MACHINE);
    return unite;
}

void GestionnaireCode::requiers_liaison_executable(EspaceDeTravail *espace, Programme *programme)
{
    std::unique_lock verrou(m_mutex);
    auto unite = crée_unite(espace, RaisonDEtre::LIAISON_PROGRAMME, true);
    unite->programme = programme;
    TACHE_AJOUTEE(LIAISON_PROGRAMME);
}

void GestionnaireCode::ajoute_requêtes_pour_attente(EspaceDeTravail *espace, Attente attente)
{
    if (attente.est<AttenteSurType>()) {
        Type *type = const_cast<Type *>(attente.type());
        auto decl = decl_pour_type(type);
        if (decl && decl->comme_declaration_type()->unité == nullptr) {
            requiers_typage(espace, decl);
        }
        /* Ceci est pour gérer les requêtes de fonctions d'initialisation avant la génération de
         * RI. */
        requiers_initialisation_type(espace, type);
    }
    else if (attente.est<AttenteSurDeclaration>()) {
        NoeudDeclaration *decl = attente.declaration();
        if (*donne_adresse_unité(decl) == nullptr) {
            requiers_typage(espace, decl);
        }
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

void GestionnaireCode::mets_en_attente(UniteCompilation *unite_attendante, Attente attente)
{
    std::unique_lock verrou(m_mutex_unités_terminées);
    assert(attente.est_valide());
    assert(unite_attendante->est_prête());
    auto espace = unite_attendante->espace;
    m_attentes_à_résoudre.ajoute_aux_données_globales({espace, attente});
    unite_attendante->ajoute_attente(attente);
    ajoute_unité_à_liste_attente(unite_attendante);
}

void GestionnaireCode::mets_en_attente(UniteCompilation *unite_attendante,
                                       kuri::tableau_statique<Attente> attentes)
{
    std::unique_lock verrou(m_mutex_unités_terminées);
    assert(attentes.taille() != 0);
    assert(unite_attendante->est_prête());

    auto espace = unite_attendante->espace;

    POUR (attentes) {
        m_attentes_à_résoudre.ajoute_aux_données_globales({espace, it});
        unite_attendante->ajoute_attente(it);
    }

    ajoute_unité_à_liste_attente(unite_attendante);
}

void GestionnaireCode::tâche_unité_terminée(UniteCompilation *unité)
{
    std::unique_lock<std::mutex> verrou(m_mutex_unités_terminées);
    m_unités_terminées.ajoute_aux_données_globales(unité);
}

void GestionnaireCode::chargement_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_chargé);

    auto espace = unite->espace;
    TACHE_TERMINEE(CHARGEMENT, true);
    m_compilatrice->messagere->ajoute_message_fichier_fermé(espace, unite->fichier->chemin());

    /* Une fois que nous avons fini de charger un fichier, il faut le lexer. */
    unite->mute_raison_d_être(RaisonDEtre::LEXAGE_FICHIER);
    m_état_chargement_fichiers.déplace_unité_pour_chargement_fichier(unite);
    ajoute_unité_à_liste_attente(unite);
    TACHE_AJOUTEE(LEXAGE);
}

void GestionnaireCode::lexage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_lexé);

    auto espace = unite->espace;
    TACHE_TERMINEE(LEXAGE, true);

    /* Une fois que nous avons lexer un fichier, il faut le parser. */
    unite->mute_raison_d_être(RaisonDEtre::PARSAGE_FICHIER);
    m_état_chargement_fichiers.déplace_unité_pour_chargement_fichier(unite);
    ajoute_unité_à_liste_attente(unite);
    TACHE_AJOUTEE(PARSAGE);
}

void GestionnaireCode::parsage_fichier_termine(UniteCompilation *unite)
{
    assert(unite->fichier);
    assert(unite->fichier->fut_parsé);
    auto espace = unite->espace;
    TACHE_TERMINEE(PARSAGE, true);
    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
    m_état_chargement_fichiers.supprime_unité_pour_chargement_fichier(unite);

    POUR (unite->fichier->noeuds_à_valider) {
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
        return !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME) &&
               !entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) &&
               entete->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
               !entete->est_operateur_pour();
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
        return !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME) &&
               !entete->est_operateur_pour() &&
               (!entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) ||
                entete->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION));
    }

    if (noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE) && !noeud->est_type_structure() &&
        !noeud->est_type_enum() && !noeud->est_declaration_bibliotheque()) {
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
        if (est_déclaration_polymorphique(noeud->comme_declaration())) {
            return false;
        }

        return !(noeud->est_charge() || noeud->est_importe() ||
                 noeud->est_declaration_bibliotheque());
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
    if (decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    auto adresse_unité = donne_adresse_unité(decl);
    if (!adresse_unité) {
        return true;
    }
    auto unite = *adresse_unité;
    if (!unite) {
        /* Pas encore d'unité, nous ne pouvons savoir si la déclaration est valide. */
        return true;
    }

    if (unite->espace->possède_erreur) {
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
                if (decl->est_entete_fonction() && declaration_est_invalide(decl)) {
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

void GestionnaireCode::typage_termine(UniteCompilation *unite)
{
    DÉBUTE_STAT(TYPAGE_TERMINÉ);
    assert(unite->noeud);
    assert_rappel(unite->noeud->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " ne fut pas validé !\n";
        std::cerr << erreur::imprime_site(*unite->espace, unite->noeud);
    });

    auto espace = unite->espace;

    if (unite->noeud->est_charge() || unite->noeud->est_importe()) {
        m_état_chargement_fichiers.supprime_unité_pour_charge_ou_importe(unite);

        if (tous_les_fichiers_à_parser_le_sont()) {
            flush_noeuds_à_typer();
        }
    }

    // rassemble toutes les dépendances de la fonction ou de la globale
    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();
    auto noeud = unite->noeud;
    DÉBUTE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    auto const détermine_dépendances = doit_determiner_les_dependances(unite->noeud);
    TERMINE_STAT(DOIT_DÉTERMINER_DÉPENDANCES);
    if (détermine_dépendances) {
        determine_dependances(unite->noeud, unite->espace, *graphe);
    }

    /* Envoi un message, nous attendrons dessus si nécessaire. */
    const auto message = m_compilatrice->messagere->ajoute_message_typage_code(espace, noeud);
    const auto doit_envoyer_en_ri = noeud_requiers_generation_ri(noeud);
    if (doit_envoyer_en_ri) {
        TACHE_AJOUTEE(GENERATION_RI);
        unite->mute_raison_d_être(RaisonDEtre::GENERATION_RI);
        ajoute_unité_à_liste_attente(unite);
    }

    if (message) {
        auto unite_noeud_code = requiers_noeud_code(espace, noeud);
        auto unite_message = crée_unite_pour_message(espace, message);
        unite_message->ajoute_attente(Attente::sur_noeud_code(unite_noeud_code, noeud));
        unite->ajoute_attente(Attente::sur_message(unite_message, message));
    }

    DÉBUTE_STAT(VÉRIFIE_ENTÊTE_VALIDÉES);
    auto peut_envoyer_changement_de_phase = verifie_que_toutes_les_entetes_sont_validees(
        *m_compilatrice->sys_module.verrou_ecriture());
    TERMINE_STAT(VÉRIFIE_ENTÊTE_VALIDÉES);

    /* Décrémente ceci après avoir ajouté le message de typage de code
     * pour éviter de prévenir trop tôt un métaprogramme. */
    TACHE_TERMINEE(TYPAGE, peut_envoyer_changement_de_phase);

    if (noeud->est_entete_fonction()) {
        m_fonctions_parsees.ajoute(noeud->comme_entete_fonction());
    }
    TERMINE_STAT(TYPAGE_TERMINÉ);
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
    assert_rappel(unite->noeud->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&] {
        std::cerr << "Le noeud de genre " << unite->noeud->genre << " n'eu pas de RI générée !\n";
        std::cerr << erreur::imprime_site(*unite->espace, unite->noeud);
    });

    auto espace = unite->espace;
    TACHE_TERMINEE(GENERATION_RI, true);
    if (espace->optimisations) {
        // À FAIRE(gestion) : tâches d'optimisations
    }

    /* Si nous avons la RI pour #crée_contexte, il nout faut ajouter toutes les unités l'attendant.
     */
    if (est_corps_de(unite->noeud,
                     espace->compilatrice().interface_kuri->decl_creation_contexte)) {
        flush_metaprogrammes_en_attente_de_crée_contexte();
    }

    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::optimisation_terminee(UniteCompilation *unite)
{
    assert(unite->noeud);
    auto espace = unite->espace;
    TACHE_TERMINEE(OPTIMISATION, true);
    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::envoi_message_termine(UniteCompilation *unité)
{
    unité->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::message_recu(Message const *message)
{
    const_cast<Message *>(message)->message_recu = true;
}

void GestionnaireCode::execution_terminee(UniteCompilation *unite)
{
    assert(unite->metaprogramme);
    assert(unite->metaprogramme->fut_execute);
    auto espace = unite->espace;
    TACHE_TERMINEE(EXECUTION, true);
    enleve_programme(unite->metaprogramme->programme);
    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

static bool programme_requiers_liaison_exécutable(OptionsDeCompilation const &options)
{
    switch (options.resultat) {
        case ResultatCompilation::RIEN:
        case ResultatCompilation::FICHIER_OBJET:
        {
            return false;
        }
        case ResultatCompilation::BIBLIOTHEQUE_STATIQUE:
        case ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE:
        case ResultatCompilation::EXECUTABLE:
        {
            return true;
        }
    }
    return false;
}

void GestionnaireCode::generation_code_machine_terminee(UniteCompilation *unite)
{
    assert(unite->programme);

    auto programme = unite->programme;
    auto espace = unite->espace;

    if (programme->pour_métaprogramme()) {
        programme->change_de_phase(PhaseCompilation::APRES_GENERATION_OBJET);
        programme->change_de_phase(PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
        requiers_liaison_executable(espace, unite->programme);
    }
    else {
        TACHE_TERMINEE(GENERATION_CODE_MACHINE, true);

        if (programme_requiers_liaison_exécutable(espace->options)) {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::AVANT_LIAISON_EXECUTABLE);
            requiers_liaison_executable(espace, unite->programme);
        }
        else {
            espace->change_de_phase(m_compilatrice->messagere,
                                    PhaseCompilation::COMPILATION_TERMINEE);
        }
    }

    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::liaison_programme_terminee(UniteCompilation *unite)
{
    assert(unite->programme);

    auto programme = unite->programme;
    auto espace = unite->espace;

    if (programme->pour_métaprogramme()) {
        auto metaprogramme = programme->pour_métaprogramme();
        programme->change_de_phase(PhaseCompilation::APRES_LIAISON_EXECUTABLE);
        programme->change_de_phase(PhaseCompilation::COMPILATION_TERMINEE);
        requiers_execution(unite->espace, metaprogramme);
    }
    else {
        TACHE_TERMINEE(LIAISON_PROGRAMME, true);
        espace->change_de_phase(m_compilatrice->messagere, PhaseCompilation::COMPILATION_TERMINEE);
    }

    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::conversion_noeud_code_terminee(UniteCompilation *unite)
{
    unite->définis_état(UniteCompilation::État::COMPILATION_TERMINÉE);
}

void GestionnaireCode::fonction_initialisation_type_creee(UniteCompilation *unite)
{
    assert((unite->type->drapeaux & INITIALISATION_TYPE_FUT_CREEE) != 0);

    auto fonction = unite->type->fonction_init;
    if (fonction->unité) {
        /* Pour les pointeurs, énums, et fonctions, la fonction est partagée, nous ne devrions pas
         * générer la RI plusieurs fois. L'unité de compilation est utilisée pour indiquée que la
         * RI est en cours de génération. */
        return;
    }

    POUR (programmes_en_cours) {
        if (it->possède(unite->type)) {
            it->ajoute_fonction(fonction);
        }
    }

    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();
    determine_dependances(fonction, unite->espace, *graphe);
    determine_dependances(fonction->corps, unite->espace, *graphe);

    unite->mute_raison_d_être(RaisonDEtre::GENERATION_RI);
    auto espace = unite->espace;
    TACHE_AJOUTEE(GENERATION_RI);
    unite->noeud = fonction;
    fonction->unité = unite;
    ajoute_unité_à_liste_attente(unite);
}

void GestionnaireCode::crée_tâches_pour_ordonnanceuse()
{
    unites_en_attente.permute_données_globales_et_locales();
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
    POUR (unites_en_attente.donne_données_locales()) {
        if (it->espace->possède_erreur) {
            it->définis_état(UniteCompilation::État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR);
            continue;
        }

        if (it->donne_raison_d_être() == RaisonDEtre::AUCUNE) {
            it->espace->rapporte_erreur_sans_site(
                "Erreur interne : obtenu une unité sans raison d'être");
            continue;
        }

        if (it->fut_annulée()) {
            continue;
        }

        /* Il est possible qu'un métaprogramme ajout du code, donc soyons sûr que l'espace est bel
         * et bien dans la phase pour la génération de code. */
        if (it->donne_raison_d_être() == RaisonDEtre::GENERATION_CODE_MACHINE &&
            it->programme == it->espace->programme) {
            if (it->espace->phase_courante() != PhaseCompilation::AVANT_GENERATION_OBJET) {
                continue;
            }
        }

        auto const état_attente = it->détermine_état_attentes();

        if (imprime_débogage &&
            état_attente != UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES) {
            //                std::cerr << "-------------------\n";
            //                std::cerr << it->raison_d_etre() << '\n';
            //                std::cerr << it->chaine_attentes_recursives() << '\n';
            //                if (it->raison_d_etre() == RaisonDEtre::TYPAGE) {
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
                unites_en_attente.efface_tout();
                m_compilatrice->ordonnanceuse->supprime_toutes_les_taches();
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
            espaces_errones.insere(it->espace());
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

    pour_chaque_element(espaces_errones, [&](EspaceDeTravail *espace) {
        ordonnanceuse->supprime_toutes_les_taches_pour_espace(
            espace, UniteCompilation::État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR);
        return kuri::DécisionItération::Continue;
    });

    unites_en_attente.ajoute_aux_données_globales(unités_à_remettre_en_attente);

#ifdef DEBUG_UNITES_EN_ATTENTES
    if (imprime_débogage) {
        std::cerr << "Unités en attente après la création des tâches : "
                  << unites_en_attente->taille() << '\n';
        ordonnanceuse->imprime_donnees_files(std::cerr);
        std::cerr << "--------------------------------------------------------\n";
    }
#endif
}

bool GestionnaireCode::plus_rien_n_est_a_faire()
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

        if (it->pour_métaprogramme()) {
            auto etat = it->ajourne_etat_compilation();

            if (etat.phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE) {
                it->change_de_phase(PhaseCompilation::AVANT_GENERATION_OBJET);
                requiers_generation_code_machine(espace, it);
            }
        }
        else {
            //            if (imprime_débogage) {
            //                imprime_diagnostique(it->diagnositique_compilation());
            //                std::cerr << espace->phase_courante() << '\n';
            //            }

            if (espace->phase_courante() == PhaseCompilation::GENERATION_CODE_TERMINEE &&
                it->ri_generees()) {
                finalise_programme_avant_generation_code_machine(espace, it);
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

    if (unites_en_attente.possède_élément_dans_données_globales() ||
        !metaprogrammes_en_attente_de_crée_contexte.est_vide()) {
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
            if (espace->phase_courante() != PhaseCompilation::COMPILATION_TERMINEE) {
                return false;
            }
            return true;
        });
}

void GestionnaireCode::tente_de_garantir_fonction_point_d_entree(EspaceDeTravail *espace)
{
    auto copie_et_valide_point_d_entree = [&](NoeudDeclarationEnteteFonction *point_d_entree) {
        auto copie = copie_noeud(m_assembleuse,
                                 point_d_entree,
                                 point_d_entree->bloc_parent,
                                 OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION);
        copie->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
        copie->comme_entete_fonction()->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
        copie->comme_entete_fonction()->corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        requiers_typage(espace, copie);
        return copie->comme_entete_fonction();
    };

    // Ne compile le point d'entrée que pour les exécutables
    if (espace->options.resultat == ResultatCompilation::EXECUTABLE) {
        if (espace->fonction_point_d_entree != nullptr) {
            return;
        }

        auto point_d_entree = m_compilatrice->fonction_point_d_entree;
        assert(point_d_entree);
        espace->fonction_point_d_entree = copie_et_valide_point_d_entree(point_d_entree);
    }
    else if (espace->options.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
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
    auto executions_en_cours = false;
    modules.pour_chaque_element([&](Module *module) {
        auto execute = module->directive_pré_exécutable;
        if (!execute) {
            return;
        }

        if (!module->exécution_directive_requise) {
            /* L'espace du programme est celui qui a créé le métaprogramme lors de la validation de
             * code, mais nous devons avoir le métaprogramme (qui hérite de l'espace du programme)
             * dans l'espace demandant son exécution afin que le compte de taches d'exécution dans
             * l'espace soit cohérent. */
            execute->metaprogramme->programme->change_d_espace(espace);
            requiers_compilation_metaprogramme(espace, execute->metaprogramme);
            module->exécution_directive_requise = true;
            executions_requises = true;
        }

        /* Nous devons attendre la fin de l'exécution de ces métaprogrammes avant de pouvoir généré
         * le code machine. */
        executions_en_cours |= !execute->metaprogramme->fut_execute;
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
        requiers_generation_ri(espace, decl_ajoute_fini);
        ri_requise = true;
    }
    if (!decl_ajoute_init->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
        requiers_generation_ri(espace, decl_ajoute_init);
        ri_requise = true;
    }
    if (!decl_init_globales->corps->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
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
        espace->unite_pour_code_machine->définis_état(
            UniteCompilation::État::ANNULÉE_CAR_REMPLACÉE);
        TACHE_TERMINEE(GENERATION_CODE_MACHINE, true);
    }

    auto unite_code_machine = requiers_generation_code_machine(espace, espace->programme);

    espace->unite_pour_code_machine = unite_code_machine;

    if (message) {
        unite_code_machine->ajoute_attente(Attente::sur_message(nullptr, message));
    }
}

void GestionnaireCode::flush_metaprogrammes_en_attente_de_crée_contexte()
{
    assert(metaprogrammes_en_attente_de_crée_contexte_est_ouvert);
    POUR (metaprogrammes_en_attente_de_crée_contexte) {
        ajoute_unité_à_liste_attente(it);
    }
    metaprogrammes_en_attente_de_crée_contexte.efface();
    metaprogrammes_en_attente_de_crée_contexte_est_ouvert = false;
}

void GestionnaireCode::interception_message_terminee(EspaceDeTravail *espace)
{
    m_compilatrice->messagere->termine_interception(espace);
}

void GestionnaireCode::ajourne_espace_pour_nouvelles_options(EspaceDeTravail *espace)
{
    std::unique_lock verrou(m_mutex);
    auto programme = espace->programme;
    programme->ajourne_pour_nouvelles_options_espace();
    /* À FAIRE : gère proprement tous les cas. */
    if (espace->options.resultat == ResultatCompilation::RIEN) {
        espace->change_de_phase(m_compilatrice->messagere, PhaseCompilation::COMPILATION_TERMINEE);
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

        if (plus_rien_n_est_a_faire()) {
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
            case RaisonDEtre::AUCUNE:
            {
                // erreur ?
                break;
            }
            case RaisonDEtre::CHARGEMENT_FICHIER:
            {
                chargement_fichier_termine(it);
                break;
            }
            case RaisonDEtre::LEXAGE_FICHIER:
            {
                lexage_fichier_termine(it);
                break;
            }
            case RaisonDEtre::PARSAGE_FICHIER:
            {
                parsage_fichier_termine(it);
                break;
            }
            case RaisonDEtre::CREATION_FONCTION_INIT_TYPE:
            {
                fonction_initialisation_type_creee(it);
                break;
            }
            case RaisonDEtre::TYPAGE:
            {
                typage_termine(it);
                break;
            }
            case RaisonDEtre::CONVERSION_NOEUD_CODE:
            {
                conversion_noeud_code_terminee(it);
                break;
            }
            case RaisonDEtre::ENVOIE_MESSAGE:
            {
                envoi_message_termine(it);
                break;
            }
            case RaisonDEtre::GENERATION_RI:
            {
                generation_ri_terminee(it);
                break;
            }
            case RaisonDEtre::GENERATION_RI_PRINCIPALE_MP:
            {
                generation_ri_terminee(it);
                break;
            }
            case RaisonDEtre::EXECUTION:
            {
                execution_terminee(it);
                break;
            }
            case RaisonDEtre::LIAISON_PROGRAMME:
            {
                liaison_programme_terminee(it);
                break;
            }
            case RaisonDEtre::GENERATION_CODE_MACHINE:
            {
                generation_code_machine_terminee(it);
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
    auto graphe = m_compilatrice->graphe_dependance.verrou_ecriture();

    POUR (types_à_insérer_dans_graphe) {
        graphe->connecte_type_type(it.type_parent, it.type_enfant);
    }
}

void GestionnaireCode::gère_requête_compilations_métaprogrammes()
{
    auto requêtes_compilations_métaprogrammes =
        m_requêtes_compilations_métaprogrammes.verrou_ecriture();

    POUR (*requêtes_compilations_métaprogrammes) {
        requiers_compilation_metaprogramme_impl(it.espace, it.métaprogramme);
    }

    requêtes_compilations_métaprogrammes->efface();
}
