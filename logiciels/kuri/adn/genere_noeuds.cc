/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include <fstream>
#include <iostream>

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "structures/chemin_systeme.hh"
#include "structures/ensemble.hh"
#include "structures/table_hachage.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/chrono.hh"

#include "adn.hh"
#include "outils_dependants_sur_lexemes.hh"

/* À FAIRE : nous ne pouvons copier les rubriques des blocs (de constantes ou autres) car la copie
 * des rubriques des blocs peut créer des doublons lorsque le noeud sera validé car la validation
 * sémantique ajoute les rubriques pour les déclarations de variables. */
static const char *copie_extra_entête_fonction = R"(
            /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
             * sont placés par la Syntaxeuse directement dans les rubriques. */
            POUR (*orig->bloc_constantes->rubriques.verrou_écriture()) {
                auto copie_rubrique = copie_noeud(it);
                copie->bloc_constantes->ajoute_rubrique(copie_rubrique->comme_déclaration_constante());
            }
            copie->drapeaux_fonction = (orig->drapeaux_fonction & DrapeauxNoeudFonction::BITS_COPIABLES);
            )";

static const char *copie_extra_entête_opérateur_pour = R"(
            if ((m_options & OptionsCopieNoeud::COPIE_PARAMÈTRES_DANS_RUBRIQUES) != OptionsCopieNoeud(0)) {
                for (int64_t i = 0; i < copie->params.taille(); i++) {
                    copie->bloc_paramètres->rubriques->ajoute(copie->paramètre_entrée(i));
                }
                copie->bloc_paramètres->rubriques->ajoute(copie->param_sortie->comme_base_déclaration_variable());
            }
            )";

static const char *copie_extra_bloc = R"(
            copie->réserve_rubriques(orig->nombre_de_rubriques());
            POUR (*copie->expressions.verrou_lecture()) {
                if (it->est_déclaration_type() || it->est_entête_fonction()) {
                    copie->ajoute_rubrique(it->comme_déclaration());
                }
            })";

static const char *copie_extra_structure = R"(
            nracine->type = nullptr;
            if (orig->bloc_constantes) {
                /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
                 * sont placés par la Syntaxeuse directement dans les rubriques. */
                POUR (*orig->bloc_constantes->rubriques.verrou_écriture()) {
                    auto copie_rubrique = copie_noeud(it);
                    copie->bloc_constantes->ajoute_rubrique(copie_rubrique->comme_déclaration_constante());
                }
            }
)";

static const char *copie_extra_union = R"(
            nracine->type = nullptr;
            if (orig->bloc_constantes) {
                /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
                 * sont placés par la Syntaxeuse directement dans les rubriques. */
                POUR (*orig->bloc_constantes->rubriques.verrou_écriture()) {
                    auto copie_rubrique = copie_noeud(it);
                    copie->bloc_constantes->ajoute_rubrique(copie_rubrique->comme_déclaration_constante());
                }
            }
)";

static const char *copie_extra_énum = R"(
            nracine->type = nullptr;
)";

/* Les déclarations référées doivent être copiées avec soin : il ne faut copier les déclarations
 * qui font partie de la fonction ou de la structure copiée. Autrement, nous risquerions de copier
 * tout le module. */
static const char *copie_déclaration_référée = R"(
            auto référence_existante = trouve_copie(orig->déclaration_référée);
            if (référence_existante) {
                copie->déclaration_référée = référence_existante->comme_déclaration();
            }
            else if (orig->déclaration_référée && orig->déclaration_référée->possède_drapeau(DrapeauxNoeud::EST_DÉCLARATION_EXPRESSION_VIRGULE)) {
                copie->déclaration_référée = copie_noeud(orig->déclaration_référée)->comme_déclaration();
            }
            else {
                copie->déclaration_référée = orig->déclaration_référée;
            }
)";

const auto source_création_tableau_noeuds_code = R"(
            if (racine_typee == racine) {
                auto noeuds_statique = mémoire::loge_tableau<NoeudCode *>("", this->noeuds.taille());
                n->noeuds = kuri::tableau_statique(noeuds_statique, this->noeuds.taille());
                POUR (this->noeuds) {
                    *noeuds_statique++ = it;
                }
                this->noeuds.efface();
            }
)";

static const IdentifiantADN &type_nominal_rubrique_pour_noeud_code(Type *type)
{
    if (type->est_tableau()) {
        return type_nominal_rubrique_pour_noeud_code(type->comme_tableau()->type_pointe);
    }

    if (type->est_pointeur()) {
        return type_nominal_rubrique_pour_noeud_code(type->comme_pointeur()->type_pointe);
    }

    auto type_nominal = type->comme_nominal();

    if (!type_nominal->est_protéine) {
        return type_nominal->nom_cpp;
    }

    auto protéine = type_nominal->est_protéine->comme_struct();
    if (!protéine) {
        return type_nominal->nom_cpp;
    }

    return protéine->accede_nom_code();
}

static ProtéineStruct const *donne_protéine_ou_mère_la_plus_ancienne(
    ProtéineStruct const *protéine)
{
    auto résultat = protéine;
    while (résultat->mere()) {
        résultat = résultat->mere();
    }
    return résultat;
}

static bool est_protéine_noeud_expression(Protéine const *protéine)
{
    auto protéine_struct = protéine->comme_struct();
    if (!protéine_struct) {
        return false;
    }

    protéine_struct = donne_protéine_ou_mère_la_plus_ancienne(protéine_struct);
    return protéine_struct->nom().nom() == "NoeudExpression";
}

struct GeneratriceCodeCPP {
    kuri::tableau<Protéine *> protéines{};
    kuri::tableau<ProtéineStruct *> protéines_struct{};
    kuri::table_hachage<kuri::chaine_statique, ProtéineStruct *> table_desc{"Protéines"};

    kuri::ensemble<kuri::chaine_statique> donne_ensemble_noms_noeuds_expression()
    {
        kuri::ensemble<kuri::chaine_statique> résultat;

        POUR (protéines) {
            if (!est_protéine_noeud_expression(it)) {
                continue;
            }
            résultat.insère(it->nom().nom());
        }

        return résultat;
    }

    void génère_fichier_prodeclaration(FluxSortieCPP &os)
    {
        os << "#pragma once\n";

        auto ensemble_noms = donne_ensemble_noms_noeuds_expression();
        auto noms = ensemble_noms.donne_tableau();

        kuri::tri_rapide(kuri::tableau_statique<kuri::chaine_statique>(noms),
                         [](auto &a, auto &b) { return a < b; });

        POUR (noms) {
            os << "struct " << it << ";\n";
        }
    }

    void génère_fichier_entête_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "utilitaires/macros.hh");
        inclus(os, "utilitaires/synchrone.hh");
        inclus(os, "structures/chaine.hh");
        inclus(os, "structures/chaine_statique.hh");
        inclus(os, "structures/tableau.hh");
        inclus(os, "structures/tableau_compresse.hh");
        inclus(os, "structures/table_hachage.hh");
        inclus(os, "compilation/transformation_type.hh");
        inclus(os, "parsage/lexemes.hh");
        inclus(os, "expression.hh");
        inclus(os, "prodeclaration.hh");
        inclus(os, "utilitaires.hh");
        os << "class Broyeuse;\n";

        // Prodéclarations des structures
        kuri::ensemble<kuri::chaine> noms_struct;
        noms_struct.insère("Enchaineuse");

        auto ensemble_noms = donne_ensemble_noms_noeuds_expression();

        POUR (protéines) {
            auto protéine = it->comme_struct();
            if (!protéine) {
                continue;
            }

            protéine->pour_chaque_rubrique_recursif(
                [&noms_struct, &ensemble_noms](Rubrique const &rubrique) {
                    if (!rubrique.type->est_pointeur()) {
                        return;
                    }

                    const auto type_pointe = rubrique.type->comme_pointeur()->type_pointe;

                    if (!type_pointe->est_nominal()) {
                        return;
                    }

                    const auto type_nominal = type_pointe->comme_nominal();
                    const auto nom_type = type_nominal->nom_cpp.nom();

                    if (!ensemble_noms.possède(nom_type)) {
                        noms_struct.insère(nom_type);
                    }
                });

            auto nom_protéine = it->nom().nom();
            if (!ensemble_noms.possède(nom_protéine)) {
                noms_struct.insère(nom_protéine);
            }
        }

        noms_struct.pour_chaque_element(
            [&](kuri::chaine_statique it) { os << "struct " << it << ";\n"; });
        os << "\n";

        // Les structures C++
        POUR (protéines) {
            it->génère_code_cpp(os, true);
        }

        os << "void imprime_genre_noeud_pour_assert(const NoeudExpression *noeud);\n\n";

        POUR (protéines_struct) {
            it->génère_code_cpp_apres_déclaration(os);
        }

        // Impression de l'arbre
        os << "void imprime_arbre_substitue(const NoeudExpression *racine, Enchaineuse &os, int "
              "profondeur);\n\n";
        os << "void imprime_arbre(NoeudExpression const *racine, Enchaineuse &os, int "
              "profondeur, bool substitution = false);\n\n";
        os << "void imprime_arbre_substitue(const NoeudExpression *racine, std::ostream &os, int "
              "profondeur);\n\n";
        os << "void imprime_arbre(NoeudExpression const *racine, std::ostream &os, int "
              "profondeur, bool substitution = false);\n\n";
        os << "kuri::chaine imprime_arbre_substitue(const NoeudExpression *racine, int "
              "profondeur);\n\n";
        os << "kuri::chaine imprime_arbre(NoeudExpression const *racine, int profondeur, bool "
              "substitution = false);\n\n";

        // Calcul de l'étendue
        os << "enum class DecisionVisiteNoeud : unsigned char {\n";
        os << "    CONTINUE,\n";
        os << "    IGNORE_ENFANTS,\n";
        os << "};\n\n";
        os << "enum class PréférenceVisiteNoeud : unsigned char {\n";
        os << "    ORIGINAL,\n";
        os << "    SUBSTITUTION,\n";
        os << "};\n\n";
        os << "void visite_noeud(NoeudExpression const *racine, PréférenceVisiteNoeud préférence, "
              "bool ignore_blocs_non_traversables_des_si_statiques, "
              "std::function<DecisionVisiteNoeud(NoeudExpression const *)> const &rappel);\n\n";
    }

    void génère_fichier_source_arbre_syntaxique(FluxSortieCPP &os)
    {
        inclus(os, "noeud_expression.hh");
        inclus(os, "utilitaires/log.hh");
        inclus(os, "structures/chaine_statique.hh");
        inclus(os, "structures/enchaineuse.hh");
        inclus(os, "parsage/identifiant.hh");
        inclus(os, "parsage/outils_lexemes.hh");
        inclus(os, "assembleuse.hh");
        inclus(os, "copieuse.hh");
        os << "#include <iostream>\n";

        POUR (protéines) {
            it->génère_code_cpp(os, false);
        }

        génère_impression_arbre_syntaxique(os);
        génère_visite_noeud(os);
        génère_copie_noeud(os);

        os << "void imprime_genre_noeud_pour_assert(const NoeudExpression *noeud)\n";
        os << "{\n";
        os << R"(    std::cerr << "Le genre de noeud est " << noeud->genre << "\n";)";
        os << "\n";
        os << "}\n\n";
    }

    void génère_impression_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "void imprime_arbre_substitue(const NoeudExpression *racine, Enchaineuse &os, int "
              "profondeur)\n";
        os << "{\n";
        os << "\timprime_arbre(racine, os, profondeur, true);\n";
        os << "}\n\n";

        os << "void imprime_arbre(NoeudExpression const *racine, Enchaineuse &os, int "
              "profondeur, bool substitution)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tif (substitution && racine->substitution) {\n";
        os << "\t\timprime_arbre(racine->substitution, os, profondeur, substitution);\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (protéines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            os << "\t\t\tos << chaine_indentations(profondeur);\n";
            os << "\t\t\tos << \"<" << it->accede_nom_comme();

            // os << " \" << (racine->ident ? racine->ident->nom : \"\") << \"";

            // À FAIRE : ceci ne prend pas en compte les ancêtres
            if (it->possède_enfants()) {
                os << ">\\n\";\n";
            }
            else {
                os << "/>\\n\";\n";
            }

            génère_code_pour_enfant(
                os, it, false, [&os](ProtéineStruct &, Rubrique const &rubrique) {
                    if (rubrique.type->est_tableau()) {
                        const auto type_tableau = rubrique.type->comme_tableau();
                        if (type_tableau->est_synchrone) {
                            os << "\t\t\tPOUR ((*racine_typee->" << rubrique.nom
                               << ".verrou_lecture())) {\n";
                        }
                        else {
                            os << "\t\t\tPOUR (racine_typee->" << rubrique.nom << ") {\n";
                        }
                        os << "\t\t\t\timprime_arbre(it, os, profondeur + 1, substitution);\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\timprime_arbre(racine_typee->" << rubrique.nom
                           << ", os, profondeur + 1, substitution);\n";
                    }
                });

            if (it->possède_enfants()) {
                os << "\t\t\tos << chaine_indentations(profondeur);\n";
                os << "\t\t\tos << \"</" << it->accede_nom_comme() << ">\\n\";\n";
            }

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "}\n\n";

        static const char *implémentations_supplémentaires = R"(
void imprime_arbre_substitue(const NoeudExpression *racine, std::ostream &os, int profondeur)
{
    os << imprime_arbre_substitue(racine, profondeur);
}
void imprime_arbre(NoeudExpression const *racine, std::ostream &os, int profondeur, bool substitution)
{
    os << imprime_arbre(racine, profondeur, substitution);
}
kuri::chaine imprime_arbre_substitue(const NoeudExpression *racine, int profondeur)
{
    Enchaineuse enchaineuse;
    imprime_arbre_substitue(racine, enchaineuse, profondeur);
    return enchaineuse.chaine();
}
kuri::chaine imprime_arbre(NoeudExpression const *racine, int profondeur, bool substitution)
{
    Enchaineuse enchaineuse;
    imprime_arbre(racine, enchaineuse, profondeur, substitution);
    return enchaineuse.chaine();
}
)";
        os << implémentations_supplémentaires;
    }

    void génère_visite_noeud(FluxSortieCPP &os)
    {
        os << "void visite_noeud(NoeudExpression const *racine, PréférenceVisiteNoeud préférence, "
              "bool ignore_blocs_non_traversables_des_si_statiques, "
              "std::function<DecisionVisiteNoeud(NoeudExpression const *)> const &rappel)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tif (préférence == PréférenceVisiteNoeud::SUBSTITUTION && racine->substitution) "
              "{\n";
        os << "\t\tracine = racine->substitution;\n";
        os << "\t}\n";
        os << "\tauto decision = rappel(racine);\n";
        os << "\tif (decision == DecisionVisiteNoeud::IGNORE_ENFANTS) {\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (protéines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            if (it->accede_nom_genre().nom() == "INSTRUCTION_SI_STATIQUE" ||
                it->accede_nom_genre().nom() == "INSTRUCTION_SAUFSI_STATIQUE") {
                os << "\t\t\tif (ignore_blocs_non_traversables_des_si_statiques) {\n";
                os << "\t\t\t\tauto racine_typee = racine->comme_si_statique();\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->condition, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tif (racine_typee->condition_est_vraie) {\n";
                os << "\t\t\t\t\tvisite_noeud(racine_typee->bloc_si_vrai, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\t}\n\t\t\t\telse {\n";
                os << "\t\t\t\t\tvisite_noeud(racine_typee->bloc_si_faux, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\t}\n";
                os << "\t\t\t}\n";
                os << "\t\t\telse {\n";
            }

            génère_code_pour_enfant(
                os, it, false, [&os, &it](ProtéineStruct &, Rubrique const &rubrique) {
                    if (rubrique.type->est_tableau()) {
                        auto nom_rubrique = rubrique.nom.nom();
                        if (it->accede_nom_genre().nom() == "EXPRESSION_APPEL" &&
                            nom_rubrique == "paramètres") {
                            nom_rubrique = "paramètres_résolus";
                        }

                        const auto type_tableau = rubrique.type->comme_tableau();
                        if (type_tableau->est_synchrone) {
                            os << "\t\t\tPOUR ((*racine_typee->" << nom_rubrique
                               << ".verrou_lecture())) {\n";
                        }
                        else {
                            os << "\t\t\tPOUR (racine_typee->" << nom_rubrique << ") {\n";
                        }
                        os << "\t\t\t\tvisite_noeud(it, préférence, "
                              "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tvisite_noeud(racine_typee->" << rubrique.nom
                           << ", préférence, ignore_blocs_non_traversables_des_si_statiques, "
                              "rappel);\n";
                    }
                });

            if (it->accede_nom_genre().nom() == "INSTRUCTION_BOUCLE") {
                os << "\t\t\tif (préférence == PréférenceVisiteNoeud::SUBSTITUTION) {\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_inc, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sansarrêt, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sinon, préférence, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t}\n";
            }
            else if (it->accede_nom_genre().nom() == "INSTRUCTION_SI_STATIQUE" ||
                     it->accede_nom_genre().nom() == "INSTRUCTION_SAUFSI_STATIQUE") {
                os << "\t\t\t}\n";
            }

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "}\n";
    }

    void génère_copie_noeud(FluxSortieCPP &os)
    {
        os << "NoeudExpression *Copieuse::copie_noeud(const NoeudExpression *racine)\n";
        os << "{\n";
        os << "\tif(!racine) {\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";
        os << "\tNoeudExpression *nracine = trouve_copie(racine);\n";
        os << "\tif(nracine) {\n";
        os << "\t\treturn nracine;\n";
        os << "\t}\n";
        os << "\tswitch(racine->genre) {\n";

        POUR (protéines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            const auto nom_genre = it->accede_nom_genre();
            const auto nom_comme = it->accede_nom_comme();
            os << "\t\tcase GenreNoeud::" << nom_genre << ":\n";
            os << "\t\t{\n";
            os << "\t\t\tconst auto orig = racine->comme_" << nom_comme << "();\n";

            /* Les corps sont créés directement avec les entêtes. */
            if (nom_genre.nom() == "DÉCLARATION_CORPS_FONCTION") {
                os << "\t\t\tauto copie_entête = "
                      "trouve_copie(orig->entête)->comme_entête_fonction();\n";
                os << "\t\t\tnracine = copie_entête->corps;\n";
            }
            else {
                os << "\t\t\tnracine = assem->crée_noeud<GenreNoeud::" << nom_genre
                   << ">(racine->lexème);\n";
            }

            os << "\t\t\tcopie_rubriques_de_bases_et_insère(racine, nracine);\n";

            if (!it->possède_enfants() && !it->possède_rubrique_a_copier()) {
                os << "\t\t\tbreak;\n";
                os << "\t\t}\n";
                continue;
            }

            os << "\t\t\tconst auto copie = nracine->comme_" << nom_comme << "();\n";

            auto copie_noeud = [&](const Rubrique &enfant) {
                const auto nom_enfant = enfant.nom;

                if (it->accede_nom_genre().nom() == "EXPRESSION_RÉFÉRENCE_DÉCLARATION") {
                    if (enfant.nom.nom() == "déclaration_référée") {
                        os << copie_déclaration_référée;
                        return;
                    }
                }

                os << "\t\t\t";

                if (enfant.type->est_tableau()) {
                    const auto type_tableau = enfant.type->comme_tableau();
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "réserve("
                       << "orig->" << nom_enfant << enfant.type->accesseur() << "taille());\n";

                    os << "\t\t\t";

                    if (type_tableau->est_synchrone) {
                        os << "POUR (*orig->" << nom_enfant << ".verrou_lecture()) {\n";
                    }
                    else {
                        os << "POUR (orig->" << nom_enfant << ") {\n";
                    }

                    os << "\t\t\t\t";
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "ajoute(";

                    if (enfant.type->accede_nom().nom() != "NoeudExpression") {
                        os << "static_cast<" << enfant.type->accede_nom() << " *>(";
                    }

                    os << "copie_noeud(it)";

                    if (enfant.type->accede_nom().nom() != "NoeudExpression") {
                        os << ")";
                    }

                    os << ");\n";

                    os << "\t\t\t}\n";
                }
                else {
                    os << "copie->" << nom_enfant << " = ";

                    if (enfant.type->accede_nom().nom() != "NoeudExpression") {
                        os << "static_cast<" << enfant.type->accede_nom() << " *>(";
                    }

                    os << "copie_noeud(orig->" << nom_enfant << ")";

                    if (enfant.type->accede_nom().nom() != "NoeudExpression") {
                        os << ")";
                    }

                    os << ";\n";
                }
            };

            it->pour_chaque_enfant_recursif(copie_noeud);

            it->pour_chaque_copie_extra_recursif([&](const Rubrique &enfant) {
                if (est_type_noeud(enfant.type)) {
                    copie_noeud(enfant);
                    return;
                }

                const auto nom_enfant = enfant.nom;
                os << "\t\t\t";

                if (enfant.type->est_tableau()) {
                    const auto type_tableau = enfant.type->comme_tableau();
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "réserve("
                       << "orig->" << nom_enfant << enfant.type->accesseur() << "taille());\n";

                    os << "\t\t\t";

                    if (type_tableau->est_synchrone) {
                        os << "POUR (*orig->" << nom_enfant << ".verrou_lecture()) {\n";
                    }
                    else {
                        os << "POUR (orig->" << nom_enfant << ") {\n";
                    }

                    os << "\t\t\t\t";
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "ajoute(it);\n";
                    os << "\t\t\t}\n";
                }
                else {
                    os << "copie->" << nom_enfant << " = "
                       << "orig->" << nom_enfant << ";\n";
                }
            });

            if (nom_genre.nom() == "DÉCLARATION_ENTÊTE_FONCTION") {
                os << copie_extra_entête_fonction << "\n";
                os << "\t\tif (orig->possède_drapeau(DrapeauxNoeudFonction::EST_MACRO)) {\n";
                os << copie_extra_entête_opérateur_pour << "\n";
                os << "\t\t}\n";
            }
            else if (nom_genre.nom() == "DÉCLARATION_OPÉRATEUR_POUR") {
                os << copie_extra_entête_fonction << "\n";
                os << copie_extra_entête_opérateur_pour << "\n";
            }
            else if (nom_genre.nom() == "DÉCLARATION_STRUCTURE") {
                os << copie_extra_structure;
            }
            else if (nom_genre.nom() == "DÉCLARATION_UNION") {
                os << copie_extra_union;
            }
            else if (nom_genre.nom() == "INSTRUCTION_COMPOSÉE") {
                os << copie_extra_bloc << "\n";
            }
            else if (nom_genre.nom() == "DÉCLARATION_ÉNUM" || nom_genre.nom() == "ERREUR" ||
                     nom_genre.nom() == "ENUM_DRAPEAU") {
                os << copie_extra_énum << "\n";
            }

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "\treturn nracine;\n";
        os << "}\n";
    }

    void génère_fichier_entête_noeud_code(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "noeud_expression.hh");
        inclus(os, "structures/tableau_page.hh");
        inclus(os, "structures/chaine_statique.hh");
        inclus(os, "structures/tableau.hh");
        inclus(os, "infos_types.hh");
        os << "struct EspaceDeTravail;\n";
        os << "struct InfoType;\n";
        os << "struct Statistiques;\n";

        // Prodéclarations des structures
        kuri::ensemble<kuri::chaine> noms_struct;

        POUR (protéines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (!nom_code.est_nul()) {
                noms_struct.insère(nom_code.nom());
            }
        }

        noms_struct.pour_chaque_element(
            [&](kuri::chaine_statique it) { os << "struct " << it << ";\n"; });
        os << "struct GeranteChaine;\n";
        os << "\n";

        // Les structures C++ pour NoeudCode
        POUR (protéines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            os << "struct " << it->accede_nom_code();

            if (it->mere() != nullptr) {
                auto classe_mere = it->mere();

                /* Certaines classes mères (comme NoeudDéclaration) ne sont pas
                 * convertis en NoeudCode, donc prends la plus ancienne ancêtre
                 * dérivant aussi un NoeudCode. */
                while (classe_mere) {
                    const auto nom_code_classe_mere = classe_mere->accede_nom_code();

                    if (!nom_code_classe_mere.est_nul()) {
                        os << " : public " << classe_mere->accede_nom_code();
                        break;
                    }

                    classe_mere = classe_mere->mere();
                }
            }

            os << " {\n";

            if (!it->accede_nom_genre().est_nul()) {
                os << "\t" << it->accede_nom_code()
                   << "() { genre =  " << it->enum_discriminante()->nom()
                   << "::" << it->accede_nom_genre() << "; }\n";
                os << "\tEMPECHE_COPIE(" << it->accede_nom_code() << ");\n";
                os << "\n";
            }

            for (auto &rubrique : it->rubriques()) {
                if (!rubrique.est_code) {
                    continue;
                }
                const auto nom_rubrique = rubrique.nom;

                if (nom_rubrique.nom() == "type") {
                    os << "\tInfoType *type = nullptr;\n";
                    continue;
                }

                if (nom_rubrique.nom() == "lexème") {
                    os << "\tkuri::chaine_statique chemin_fichier{};\n";
                    os << "\tkuri::chaine_statique nom_fichier{};\n";
                    os << "\tint numéro_ligne = 0;\n";
                    os << "\tint numéro_colonne = 0;\n";
                    continue;
                }

                if (nom_rubrique.nom() == "ident") {
                    os << "\tkuri::chaine_statique nom{};\n";
                    continue;
                }

                if (nom_rubrique.nom() == "valeur" &&
                    nom_code.nom() == "NoeudCodeLittéraleChaine") {
                    os << "\tkuri::chaine_statique valeur{};\n";
                    continue;
                }

                os << "\t";

                if (rubrique.type->est_tableau()) {
                    os << "kuri::tableau<";
                }

                const auto ident_type = type_nominal_rubrique_pour_noeud_code(rubrique.type);
                const auto type_rubrique = ident_type.nom();
                os << type_rubrique;

                if (rubrique.type->est_pointeur() ||
                    (rubrique.type->est_tableau() &&
                     rubrique.type->comme_tableau()->type_pointe->est_pointeur())) {
                    os << " *";
                }

                if (rubrique.type->est_tableau()) {
                    os << '>';
                }

                os << ' ' << rubrique.nom << " = " << rubrique.type->valeur_defaut();
                os << ";\n";
            }

            // Déclarations des fonctions de transtypage
            if (nom_code.nom() == "NoeudCode") {
                for (const auto &noeud : protéines_struct) {
                    const auto nom_comme = noeud->accede_nom_comme();

                    if (nom_comme.est_nul()) {
                        continue;
                    }

                    const auto nom_noeud = noeud->accede_nom_code();

                    if (nom_noeud.est_nul()) {
                        continue;
                    }

                    génère_déclaration_fonctions_discrimination(os, nom_noeud, nom_comme);
                }
            }
            else if (nom_code.nom() == "NoeudCodeCorpsFonction") {
                os << "    kuri::tableau_statique<NoeudCode *> noeuds{};\n";
            }

            os << "};\n\n";
        }

        os << "void imprime_genre_noeud_pour_assert(const NoeudCode *noeud);\n\n";

        // Implémente les fonctions de transtypage
        POUR (protéines_struct) {
            const auto nom_comme = it->accede_nom_comme();

            if (nom_comme.est_nul()) {
                continue;
            }

            const auto nom_noeud = it->accede_nom_code();

            if (nom_noeud.est_nul()) {
                continue;
            }

            génère_définition_fonctions_discrimination(os, "NoeudCode", *it, true);
        }

        // Implémente une convertisseuse

        os << "struct ConvertisseuseNoeudCode {\n";
        // les allocations de noeuds codes
        os << "\t// Allocations des noeuds codes\n";
        POUR (protéines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            os << "\tkuri::tableau_page<" << nom_code << "> noeuds_code_" << it->accede_nom_comme()
               << "{};\n";
        }
        os << "\n";

        // les allocations de noeuds expressions
        os << "\t// Allocations des noeuds expressions\n";
        // À FAIRE : blocs parent, corps fonctions
        POUR (protéines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            os << "\tkuri::tableau_page<" << it->nom() << "> noeuds_expr_"
               << it->accede_nom_comme() << "{};\n";
        }
        os << "\n";

        // Les allocations des infos types
        os << "\t// Les allocations des infos types\n";
        os << "\tAllocatriceInfosType allocatrice_infos_types{};\n\n";

        os << "\tNoeudExpression *convertis_noeud_code(EspaceDeTravail *espace, GeranteChaine "
              "&gérante_chaine, NoeudCode *racine);\n\n";
        os << "\tNoeudCode *convertis_noeud_syntaxique(EspaceDeTravail *espace, NoeudExpression "
              "*racine);\n\n";
        os << "\tInfoType *crée_info_type_pour(EspaceDeTravail *espace, Typeuse &typeuse, Type "
              "*type);\n\n";
        os << "\tType *convertis_info_type(Typeuse &typeuse, InfoType *type);\n\n";
        os << "\tvoid rassemble_statistiques(Statistiques &stats) const;\n\n";
        os << "\tint64_t mémoire_utilisée() const;\n";
        os << "\tkuri::tableau<NoeudCode *> noeuds{};\n";

        os << "};\n\n";
    }

    void génère_fichier_source_noeud_code(FluxSortieCPP &os)
    {
        inclus(os, "noeud_code.hh");
        inclus_système(os, "iostream");
        inclus(os, "compilation/compilatrice.hh");
        inclus(os, "compilation/espace_de_travail.hh");
        inclus(os, "parsage/identifiant.hh");
        inclus(os, "parsage/gerante_chaine.hh");
        os << "NoeudCode *ConvertisseuseNoeudCode::convertis_noeud_syntaxique(EspaceDeTravail "
              "*espace, NoeudExpression *racine)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";
        os << "\tif (racine->noeud_code) {\n";
        os << "\t\tthis->noeuds.ajoute(racine->noeud_code);\n";
        os << "\t\treturn racine->noeud_code;\n";
        os << "\t}\n";
        os << "\tNoeudCode *noeud = nullptr;\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << nom_genre << ":\n";
            os << "\t\t{\n";

            const auto nom_noeud_code = it->accede_nom_code();

            if (nom_noeud_code.est_nul()) {
                os << "\t\t\treturn nullptr;\n";
                os << "\t\t}\n";
                continue;
            }

            if (nom_genre.nom() == "DÉCLARATION_CORPS_FONCTION") {
                os << "\t\t\tthis->noeuds.efface();\n";
            }

            os << "\t\t\tauto n = noeuds_code_" << it->accede_nom_comme()
               << ".ajoute_élément();\n";
            os << "\t\t\tthis->noeuds.ajoute(n);\n";
            // Renseigne directement le noeud code afin d'éviter les boucles infinies résultant en
            // des surempilages d'appels quand nous convertissons notamment les entêtes et les
            // corps de fonctions qui se référencent mutuellement.
            os << "\t\t\tracine->noeud_code = n;\n";

            génère_code_pour_enfant(
                os, it, true, [&os, it, this](ProtéineStruct &, Rubrique const &rubrique) {
                    const auto nom_rubrique = rubrique.nom;

                    if (nom_rubrique.nom() == "type") {
                        return;
                    }

                    if (nom_rubrique.nom() == "lexème") {
                        return;
                    }

                    if (nom_rubrique.nom() == "ident") {
                        return;
                    }

                    if (nom_rubrique.nom() == "genre") {
                        return;
                    }

                    if (nom_rubrique.nom() == "annotations") {
                        os << "\t\t\t\tn->annotations.réserve(racine_typee->annotations.taille());"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(rubrique.type->accede_nom().nom(),
                                                                nullptr);

                    if (!desc_type) {
                        if (rubrique.type->est_tableau()) {
                            const auto type_tableau = rubrique.type->comme_tableau();
                            if (type_tableau->est_synchrone) {
                                os << "\t\t\tPOUR (*racine_typee->" << nom_rubrique
                                   << ".verrou_lecture()) {\n";
                            }
                            else {
                                os << "\t\t\tPOUR (racine_typee->" << nom_rubrique << ") {\n";
                            }
                            os << "\t\t\t\tn->" << nom_rubrique << ".ajoute(it);\n";
                            os << "\t\t\t}\n";
                        }
                        else {
                            if (nom_rubrique.nom() == "valeur" &&
                                it->accede_nom_code().nom() == "NoeudCodeLittéraleChaine") {
                                os << "\t\t\tn->valeur = racine_typee->lexème->chaine;\n";
                            }
                            else {
                                os << "\t\t\tn->" << nom_rubrique << " = racine_typee->"
                                   << nom_rubrique << ";\n";
                            }
                        }

                        return;
                    }

                    if (rubrique.type->est_tableau()) {
                        const auto type_tableau = rubrique.type->comme_tableau();
                        if (type_tableau->est_synchrone) {
                            os << "\t\t\tPOUR (*racine_typee->" << nom_rubrique
                               << ".verrou_lecture()) {\n";
                        }
                        else {
                            os << "\t\t\tPOUR (racine_typee->" << nom_rubrique << ") {\n";
                        }
                        os << "\t\t\t\tn->" << nom_rubrique
                           << ".ajoute(convertis_noeud_syntaxique(espace, it)";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom() != "") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ");\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tif (racine_typee->" << nom_rubrique << ") {\n";
                        os << "\t\t\t\tn->" << nom_rubrique
                           << " = convertis_noeud_syntaxique(espace, racine_typee->"
                           << nom_rubrique << ")";
                        if (desc_type->accede_nom_code().nom() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom() != "") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ";\n";
                        os << "\t\t\t}\n";
                    }
                });

            os << "\t\t\tn->genre = racine_typee->genre;\n";
            os << "\t\t\tif (racine_typee->est_déclaration_type()) {\n";
            os << "\t\t\t\tn->type = crée_info_type_pour(espace, espace->typeuse, "
                  "racine_typee->comme_déclaration_type());\n";
            os << "\t\t\t}\n";
            os << "\t\t\telse {\n";
            os << "\t\t\t\tn->type = crée_info_type_pour(espace, espace->typeuse, "
                  "racine_typee->type);\n";
            os << "\t\t\t}\n";
            os << "\t\t\tif (racine_typee->ident) { n->nom = racine_typee->ident->nom; } else if "
                  "(racine_typee->lexème) { n->nom = racine_typee->lexème->chaine; }\n";

            if (nom_genre.nom() == "DÉCLARATION_CORPS_FONCTION") {
                os << source_création_tableau_noeuds_code;
            }
            else if (nom_genre.nom() == "DÉCLARATION_ENTÊTE_FONCTION") {
                os << "n->est_polymorphique = "
                      "racine_typee->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);\n";
            }

            os << "\t\t\tnoeud = n;\n";
            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";

        os << "\tconst auto lexème = racine->lexème;\n";
        os << "\t// lexème peut-être nul pour les blocs\n";
        os << "\tif (lexème) {\n";
        os << "\t\tconst auto fichier = espace->fichier(lexème->fichier);\n";
        os << "\t\tnoeud->chemin_fichier = fichier->chemin();\n";
        os << "\t\tnoeud->nom_fichier = fichier->nom();\n";
        os << "\t\tnoeud->numéro_ligne = lexème->ligne + 1;\n";
        os << "\t\tnoeud->numéro_colonne = lexème->colonne;\n";
        os << "\t}\n";
        os << "\treturn noeud;\n";
        os << "}\n\n";

        // convertis_noeud_expression

        os << "NoeudExpression *ConvertisseuseNoeudCode::convertis_noeud_code(EspaceDeTravail "
              "*espace, GeranteChaine &gérante_chaine, NoeudCode *racine)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";
        os << "\tNoeudExpression *noeud = nullptr;\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (protéines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            const auto nom_noeud_code = it->accede_nom_code();

            if (nom_noeud_code.est_nul()) {
                os << "\t\t\treturn nullptr;\n";
                os << "\t\t}\n";
                continue;
            }

            os << "\t\t\tauto n = noeuds_expr_" << it->accede_nom_comme()
               << ".ajoute_élément();\n";

            génère_code_pour_enfant(
                os, it, true, [&os, it, this](ProtéineStruct &, Rubrique const &rubrique) {
                    const auto nom_rubrique = rubrique.nom;

                    if (nom_rubrique.nom() == "type") {
                        return;
                    }

                    if (nom_rubrique.nom() == "lexème") {
                        return;
                    }

                    if (nom_rubrique.nom() == "ident") {
                        return;
                    }

                    if (nom_rubrique.nom() == "genre") {
                        return;
                    }

                    if (nom_rubrique.nom() == "annotations") {
                        os << "\t\t\t\tn->annotations.réserve(static_cast<int>(racine_typee->"
                              "annotations.taille()));"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(rubrique.type->accede_nom().nom(),
                                                                nullptr);

                    if (!desc_type) {
                        if (rubrique.type->est_tableau()) {
                            os << "\t\t\tPOUR (racine_typee->" << nom_rubrique << ") {\n";
                            os << "\t\t\t\tn->" << nom_rubrique << rubrique.type->accesseur()
                               << "ajoute(it);\n";
                            os << "\t\t\t}\n";
                        }
                        else {
                            if (nom_rubrique.nom() == "valeur" &&
                                it->accede_nom_code().nom() == "NoeudCodeLittéraleChaine") {
                                os << "\t\tn->valeur = "
                                      "gérante_chaine.ajoute_chaine(racine_typee->valeur);\n";
                            }
                            else {
                                os << "\t\t\tn->" << nom_rubrique << " = racine_typee->"
                                   << nom_rubrique << ";\n";
                            }
                        }

                        return;
                    }

                    if (rubrique.type->est_tableau()) {
                        os << "\t\t\tPOUR (racine_typee->" << nom_rubrique << ") {\n";
                        os << "\t\t\t\tn->" << nom_rubrique << rubrique.type->accesseur();
                        os << "ajoute(convertis_noeud_code(espace, gérante_chaine, it)";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ");\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tif (racine_typee->" << nom_rubrique << ") {\n";
                        os << "\t\t\t\tn->" << nom_rubrique
                           << " = convertis_noeud_code(espace, gérante_chaine, racine_typee->"
                           << nom_rubrique << ")";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ";\n";
                        os << "\t\t\t}\n";
                    }
                });

            os << "\t\t\tn->genre = racine_typee->genre;\n";
            os << "\t\t\tn->type = convertis_info_type(espace->typeuse, "
                  "racine_typee->type);\n";

            os << "\t\t\tnoeud = n;\n";
            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "\treturn noeud;\n";
        os << "}\n\n";

        os << "int64_t ConvertisseuseNoeudCode::mémoire_utilisée() const\n";
        os << "{\n";

        os << "\tauto mem = int64_t(0);\n";

        POUR (protéines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            os << "\tmem += noeuds_code_" << it->accede_nom_comme() << ".mémoire_utilisée();\n";
            os << "\tmem += noeuds_expr_" << it->accede_nom_comme() << ".mémoire_utilisée();\n";
        }

        os << "\tmem += allocatrice_infos_types.mémoire_utilisée();\n";

        os << "\treturn mem;\n";
        os << "}\n";

        os << "void imprime_genre_noeud_pour_assert(const NoeudCode *noeud)\n";
        os << "{\n";
        os << R"(    std::cerr << "Le genre de noeud est " << noeud->genre << "\n";)";
        os << "\n";
        os << "}\n\n";
    }

    void génère_code_pour_enfant(FluxSortieCPP &os,
                                 ProtéineStruct *racine,
                                 bool inclus_code,
                                 std::function<void(ProtéineStruct &, const Rubrique &)> rappel)
    {
        auto possède_enfants = false;
        auto noeud_courant = racine;

        while (noeud_courant) {
            if (noeud_courant->possède_enfants()) {
                possède_enfants = true;
                break;
            }

            if (inclus_code && !possède_enfants) {
                for (const auto &rubrique : noeud_courant->rubriques()) {
                    if (rubrique.est_code) {
                        possède_enfants = true;
                        break;
                    }
                }
            }

            noeud_courant = noeud_courant->mere();
        }

        if (!possède_enfants) {
            return;
        }

        os << "\t\t\tauto racine_typee = racine->comme_" << racine->accede_nom_comme() << "();\n";

        noeud_courant = racine;

        while (noeud_courant) {
            for (const auto &rubrique : noeud_courant->rubriques()) {
                if (rubrique.est_enfant || (inclus_code && rubrique.est_code)) {
                    rappel(*noeud_courant, rubrique);
                }
            }

            noeud_courant = noeud_courant->mere();
        }
    }

    void génère_fichier_source_assembleuse(FluxSortieCPP &os)
    {
        inclus(os, "assembleuse.hh");

        const char *empile_bloc = R"(
NoeudBloc *AssembleuseArbre::empile_bloc(Lexème const *lexème, NoeudDéclarationEntêteFonction *appartiens_à_fonction, TypeBloc type_bloc)
{
    auto bloc = crée_noeud<GenreNoeud::INSTRUCTION_COMPOSÉE>(lexème)->comme_bloc();
    bloc->type_bloc = type_bloc;
    bloc->appartiens_à_fonction = appartiens_à_fonction;
    bloc->bloc_parent = bloc_courant();
    m_blocs.empile(bloc);
    return bloc;
}
)";
        const char *recycle_référence = R"(
    if (!m_références.est_vide()) {
        auto résultat = m_références.dernier_élément();
        m_références.supprime_dernier();
        new (résultat) NoeudExpressionRéférence;
        résultat->lexème = lexème;
        résultat->ident = lexème->ident;
        résultat->bloc_parent = bloc_courant();
        return résultat;
    }
)";

        os << empile_bloc;

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << it->nom() << " *AssembleuseArbre::crée_" << it->accede_nom_comme()
               << "(const Lexème *lexème";

            auto rubriques_construction = it->donne_rubriques_pour_construction();
            POUR_NOMME (rubrique, rubriques_construction) {
                os << ", " << *rubrique.type << " " << rubrique.nom;
            }

            os << ")\n{\n";

            if (it->nom().nom() == "NoeudExpressionRéférence") {
                os << recycle_référence;
            }

            if (rubriques_construction.est_vide()) {
                os << "\treturn crée_noeud<GenreNoeud::" << nom_genre << ">(lexème)->comme_"
                   << it->accede_nom_comme() << "();\n";
            }
            else {
                os << "\tauto résultat = crée_noeud<GenreNoeud::" << nom_genre
                   << ">(lexème)->comme_" << it->accede_nom_comme() << "();\n";

                POUR_NOMME (rubrique, rubriques_construction) {
                    os << "\trésultat->" << rubrique.nom << " = " << rubrique.nom << ";\n";
                }

                os << "\treturn résultat;\n";
            }

            os << "}\n";
        }
    }

    void génère_fichier_entête_assembleuse(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "allocatrice.hh");
        inclus(os, "structures/pile.hh");
        os << "struct NoeudDéclarationTypeComposé;\n";
        os << "using TypeComposé = NoeudDéclarationTypeComposé;\n";
        os << "struct AssembleuseArbre {\n";
        os << "private:\n";
        os << "\tAllocatriceNoeud &m_allocatrice;\n";
        os << "\tkuri::pile<NoeudBloc *> m_blocs{};\n";
        os << "\tkuri::tableau<NoeudExpressionRéférence *> m_références{};\n";
        os << "public:\n";

        const char *methodes = R"(
    explicit AssembleuseArbre(AllocatriceNoeud &allocatrice)
        : m_allocatrice(allocatrice)
    {}

    NoeudBloc *empile_bloc(Lexème const *lexème, NoeudDéclarationEntêteFonction *appartiens_à_fonction, TypeBloc type_bloc);

    NoeudBloc *bloc_courant() const
    {
        if (m_blocs.est_vide()) {
            return nullptr;
        }

        return m_blocs.haut();
    }

    void bloc_courant(NoeudBloc *bloc)
    {
        m_blocs.empile(bloc);
    }

    void dépile_tout()
    {
        m_blocs.efface();
    }

    void dépile_bloc()
    {
        m_blocs.dépile();
    }

    /* Utilisation d'un gabarit car à part pour les copies, nous connaissons
     * toujours le genre de noeud à créer, et spécialiser cette fonction nous
     * économise pas mal de temps d'exécution, au prix d'un exécutable plus gros. */
    template <GenreNoeud genre>
    NoeudExpression *crée_noeud(Lexème const *lexème)
    {
        auto noeud = m_allocatrice.crée_noeud<genre>();
        noeud->genre = genre;
        noeud->lexème = lexème;
        noeud->bloc_parent = bloc_courant();

        if (noeud->lexème && (noeud->lexème->genre == GenreLexème::CHAINE_CARACTERE)) {
            if (noeud->est_declaration_symbole()) {
                noeud->comme_declaration_symbole()->ident = lexème->ident;
            }
            else if (noeud->est_reference_declaration()) {
                noeud->comme_reference_declaration()->ident = lexème->ident;
            }
            else if (noeud->est_directive_instrospection()) {
                noeud->comme_directive_instrospection()->ident = lexème->ident;
            }
            else if (noeud->est_boucle()) {
                noeud->comme_pour()->ident = lexème->ident;
            }
            else if (noeud->est_boucle()) {
                noeud->comme_pour()->ident = lexème->ident;
            }
            else if (noeud->est_reference_membre()) {
                noeud->comme_reference_membre()->ident = lexème->ident;
            }
        }

        if (genre == GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION) {
            auto entête = noeud->comme_entête_fonction();
            entête->corps->lexème = lexème;
            entête->corps->ident = lexème->ident;
            entête->corps->bloc_parent = entête->bloc_parent;
        }

        if (genre == GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE) {
            /* transfère l'index car les lexèmes peuvent être partagés lors de la simplification du code ou des exécutions */
            noeud->comme_littérale_chaine()->valeur = lexème->indice_chaine;
        }

        return noeud;
    }

    void recycle_référence(NoeudExpressionRéférence *référence)
    {
        m_références.ajoute(référence);
    }
)";

        os << methodes;

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\t" << it->nom() << " *crée_" << it->accede_nom_comme()
               << "(const Lexème *lexème";

            auto rubriques_construction = it->donne_rubriques_pour_construction();
            POUR_NOMME (rubrique, rubriques_construction) {
                os << ", " << *rubrique.type << " " << rubrique.nom;
            }

            os << ");\n";
        }

        const char *decls_extras = R"(
    NoeudSi *crée_si(const Lexème *lexème, GenreNoeud genre_noeud);
    NoeudDéclarationVariable *crée_déclaration_variable(NoeudExpressionRéférence *ref);
    NoeudAssignation *crée_incrementation(const Lexème *lexème, NoeudExpression *valeur);
    NoeudAssignation *crée_decrementation(const Lexème *lexème, NoeudExpression *valeur);
    NoeudBloc *crée_bloc_seul(const Lexème *lexème, NoeudBloc *bloc_parent);
    NoeudDéclarationVariable *crée_déclaration_variable(const Lexème *lexème, Type *type, IdentifiantCode *ident, NoeudExpression *expression);
    NoeudDéclarationVariable *crée_déclaration_variable(NoeudExpressionRéférence *ref, NoeudExpression *expression);
    NoeudExpressionLittéraleEntier *crée_littérale_entier(const Lexème *lexème, Type *type, uint64_t valeur);
    NoeudExpressionLittéraleBool *crée_littérale_bool(const Lexème *lexème, Type *type, bool valeur);
    NoeudExpressionLittéraleRéel *crée_littérale_réel(const Lexème *lexème, Type *type, double valeur);
    NoeudExpression *crée_référence_type(const Lexème *lexème, Type *type);
    NoeudExpressionAppel *crée_appel(const Lexème *lexème, NoeudExpression *appelee, Type *type);
    NoeudExpressionBinaire *crée_indexage(const Lexème *lexème, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification);
    NoeudExpressionBinaire *crée_expression_binaire(const Lexème *lexème, OpérateurBinaire const *op, NoeudExpression *expr1, NoeudExpression *expr2);
    NoeudExpressionRubrique *crée_référence_rubrique(const Lexème *lexème, NoeudExpression *accede, Type *type, int index);
    NoeudExpressionRéférence *crée_référence_déclaration(const Lexème *lexème, NoeudDéclaration *decl);
    NoeudExpressionAppel *crée_construction_structure(const Lexème *lexème, TypeComposé *type);
)";

        os << decls_extras;

        os << "};\n";
    }

    void génère_fichier_source_allocatrice(FluxSortieCPP &os)
    {
        inclus(os, "allocatrice.hh");
        inclus(os, "statistiques/statistiques.hh");
        os << "int64_t AllocatriceNoeud::nombre_noeuds() const\n";
        os << "{\n";
        os << "\tauto nombre = int64_t(0);\n";
        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\tnombre += m_noeuds_" << it->accede_nom_comme() << ".taille();\n";
        }
        os << "\treturn nombre;\n";
        os << "}\n";

        os << "void AllocatriceNoeud::rassemble_statistiques(Statistiques &stats) const\n";
        os << "{\n";
        os << "\tauto &stats_arbre = stats.stats_arbre;\n";
        os << "\tauto &stats_gaspillage = stats.stats_gaspillage;\n";
        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\tstats_arbre.fusionne_entrée({" << '"' << it->nom() << '"' << ", "
               << "m_noeuds_" << it->accede_nom_comme() << ".taille(), "
               << "m_noeuds_" << it->accede_nom_comme() << ".mémoire_utilisée()});\n";
            os << "\tstats_gaspillage.fusionne_entrée({" << '"' << it->nom() << '"' << ", "
               << "1, "
               << "m_noeuds_" << it->accede_nom_comme() << ".gaspillage_mémoire()});\n";
        }

        // stats pour les tableaux
        auto noms_tableaux = kuri::ensemble<kuri::chaine>();

        auto crée_nom_tableau = [](kuri::chaine_statique nom_comme,
                                   kuri::chaine_statique nom_rubrique) -> kuri::chaine {
            return enchaine("taille_max_", nom_comme, "_", nom_rubrique);
        };

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }
            if (!it->possède_tableau()) {
                continue;
            }

            it->pour_chaque_rubrique_recursif([&](const Rubrique &rubrique) {
                if (rubrique.type->est_tableau() || rubrique.nom.nom() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              rubrique.nom.nom());
                    os << "auto " << nom_tableau << " = 0;\n";
                    noms_tableaux.insère(nom_tableau);
                }
            });
        }

        os << "auto mémoire_gaspillée = 0;\n";

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }
            if (!it->possède_tableau()) {
                continue;
            }

            const auto nom_comme = it->accede_nom_comme();

            os << "auto mémoire_" << nom_comme << " = int64_t(0);\n";
            os << "pour_chaque_élément(m_noeuds_" << nom_comme << ", [&](";
            os << it->nom() << " const &noeud) {\n";
            it->pour_chaque_rubrique_recursif([&](const Rubrique &rubrique) {
                if (rubrique.type->est_tableau()) {
                    const auto nom_rubrique = rubrique.nom;
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              nom_rubrique.nom());
                    os << nom_tableau << " = std::max(" << nom_tableau << ", noeud."
                       << nom_rubrique;
                    os << rubrique.type->accesseur() << "taille());\n";
                    os << "mémoire_" << nom_comme << " += noeud." << nom_rubrique;
                    os << rubrique.type->accesseur() << "taille_mémoire();\n";
                    os << "mémoire_gaspillée"
                       << " += noeud." << nom_rubrique;
                    os << rubrique.type->accesseur() << "gaspillage_mémoire();\n";
                }
                else if (rubrique.nom.nom() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              rubrique.nom.nom());

                    os << "if (noeud.monomorphisations) {\n";
                    os << "mémoire_" << nom_comme
                       << " += noeud.monomorphisations->mémoire_utilisée();\n";
                    os << nom_tableau << " = std::max(" << nom_tableau
                       << ", noeud.monomorphisations->nombre_items_max());\n";
                    os << "}\n";
                }
            });
            os << "});\n";

            os << "\tstats_arbre.fusionne_entrée({" << '"' << it->nom() << '"' << ", "
               << "0, "
               << "mémoire_" << nom_comme << "});\n";
        }

        os << "\tstats_gaspillage.fusionne_entrée({\"Tableaux Arbre\", 1, mémoire_gaspillée});\n";

        os << "auto &stats_tableaux = stats.stats_tableaux;\n";
        noms_tableaux.pour_chaque_element([&](kuri::chaine_statique it) {
            os << "stats_tableaux.fusionne_entrée({\"" << it << "\", " << it << "});\n";
        });

        os << "}\n";
    }

    void génère_fichier_entête_allocatrice(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "structures/tableau_page.hh");
        inclus(os, "compilation/monomorphisations.hh");
        inclus(os, "noeud_expression.hh");
        os << "struct Statistiques;\n";
        os << "struct AllocatriceNoeud {\n";
        os << "private:\n";

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\tkuri::tableau_page<" << it->nom() << "> m_noeuds_" << it->accede_nom_comme()
               << "{};\n";
        }

        os << "\tkuri::tableau_page<DonnéesSymboleExterne> m_données_symbole_externe{};\n";
        os << "\tkuri::tableau_page<Monomorphisations> m_monomorphisations_fonctions{};\n";
        os << "\tkuri::tableau_page<Monomorphisations> m_monomorphisations_structs{};\n";
        os << "\tkuri::tableau_page<Monomorphisations> m_monomorphisations_unions{};\n";

        // XXX - réusinage types
        os << "\tfriend struct Typeuse;\n";

        os << "\n";
        os << "public:\n";

        os << "\ttemplate <GenreNoeud genre>\n";
        os << "\tNoeudExpression *crée_noeud()\n";
        os << "\t{\n";

        os << "\t\tswitch (genre) {\n";

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\t\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t\t{\n";

            // Entêtes et corps alloués ensembles
            if (nom_genre.nom() == "DÉCLARATION_ENTÊTE_FONCTION" ||
                nom_genre.nom() == "DÉCLARATION_OPÉRATEUR_POUR") {
                if (nom_genre.nom() == "DÉCLARATION_ENTÊTE_FONCTION") {
                    os << "\t\t\t\tauto entête = m_noeuds_entête_fonction.ajoute_élément();\n";
                }
                else {
                    os << "\t\t\t\tauto entête = m_noeuds_opérateur_pour.ajoute_élément();\n";
                }
                os << "\t\t\t\tauto corps  = m_noeuds_corps_fonction.ajoute_élément();\n";
                os << "\t\t\t\tentête->corps = corps;\n";
                os << "\t\t\t\tcorps->entête = entête;\n";
                os << "\t\t\t\treturn entête;\n";
            }
            else if (nom_genre.nom() == "DÉCLARATION_CORPS_FONCTION") {
                os << "\t\t\t\treturn nullptr;\n";
            }
            else {
                os << "\t\t\t\treturn m_noeuds_" << it->accede_nom_comme()
                   << ".ajoute_élément();\n";
            }

            os << "\t\t\t}\n";
        }

        os << "\t\t}\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";

        const char *crée_monomorphisations = R"(
    DonnéesSymboleExterne *crée_données_symbole_externe()
    {
        return m_données_symbole_externe.ajoute_élément();
    }

    Monomorphisations *crée_monomorphisations_fonction()
    {
        return m_monomorphisations_fonctions.ajoute_élément();
    }

    Monomorphisations *crée_monomorphisations_struct()
    {
        return m_monomorphisations_structs.ajoute_élément();
    }

    Monomorphisations *crée_monomorphisations_union()
    {
        return m_monomorphisations_unions.ajoute_élément();
    }
)";

        os << crée_monomorphisations;

        os << "\n";
        os << "\tint64_t nombre_noeuds() const;\n\n";

        os << "\tvoid rassemble_statistiques(Statistiques &stats) const;\n";

        os << "};\n";
    }
};

struct GeneratriceCodeKuri {
    void génère_fichier_kuri_noeud_code(FluxSortieKuri &os,
                                        kuri::tableau<Protéine *> const &protéines)
    {
        // Les structures Kuri pour NoeudCode
        POUR (protéines) {
            if (it->comme_struct()) {
                auto prot = it->comme_struct();

                if (prot->paire()) {
                    prot->paire()->génère_code_kuri(os);
                    continue;
                }
            }

            it->génère_code_kuri(os);
        }
    }
};

int main(int argc, char **argv)
{
    if (argc != 4) {
        std::cerr << "Utilisation: " << argv[0] << " nom_fichier_sortie\n";
        return 1;
    }

    const auto chemin_adn = argv[3];
    auto nom_fichier_sortie = kuri::chemin_systeme(argv[1]);

    auto texte = charge_contenu_fichier(chemin_adn);

    auto fichier = Fichier();
    fichier.tampon_ = TamponSource(texte);
    fichier.chemin_ = chemin_adn;

    auto gérante_chaine = kuri::Synchrone<GeranteChaine>();
    auto table_identifiants = kuri::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gérante_chaine, table_identifiants, imprime_erreur};

    auto lexeuse = Lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possède_erreur()) {
        return 1;
    }

    auto syntaxeuse = SyntaxeuseADN(&fichier);
    syntaxeuse.analyse();

    if (syntaxeuse.possède_erreur()) {
        return 1;
    }

    auto table_desc = kuri::table_hachage<kuri::chaine_statique, ProtéineStruct *>(
        "Protéines structures");

    POUR (syntaxeuse.protéines) {
        if (!it->comme_struct()) {
            continue;
        }

        table_desc.insère(it->nom().nom(), it->comme_struct());
    }

    GeneratriceCodeCPP generatrice;
    generatrice.protéines = syntaxeuse.protéines;
    generatrice.table_desc = table_desc;

    POUR (generatrice.protéines) {
        if (it->comme_struct()) {
            generatrice.protéines_struct.ajoute(it->comme_struct());
        }
    }

    auto nom_fichier_tmp = kuri::chemin_systeme::chemin_temporaire(
        nom_fichier_sortie.nom_fichier());

    if (nom_fichier_sortie.nom_fichier() == "noeud_expression.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_source_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_expression.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_entête_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_code.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            generatrice.génère_fichier_source_noeud_code(flux);
            if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
                return 1;
            }
        }
        {
            // Génère le fichier de code pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/code.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            GeneratriceCodeKuri generatrice_kuri;
            generatrice_kuri.génère_fichier_kuri_noeud_code(flux, syntaxeuse.protéines);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_code.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_entête_noeud_code(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "assembleuse.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_source_assembleuse(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "assembleuse.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_entête_assembleuse(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "allocatrice.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_source_allocatrice(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "allocatrice.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_entête_allocatrice(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "prodeclaration.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_prodeclaration(flux);
    }
    else {
        std::cerr << "Chemin de fichier " << argv[1] << " inconnu !\n";
        return 1;
    }

    if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
