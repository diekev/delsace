/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include <fstream>
#include <iostream>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/badge.hh"

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

#include "adn.hh"
#include "outils_dependants_sur_lexemes.hh"

/* À FAIRE : nous ne pouvons copier les membres des blocs (de constantes ou autres) car la copie
 * des membres des blocs peut créer des doublons lorsque le noeud sera validé car la validation
 * sémantique ajoute les membres pour les déclarations de variables. */
static const char *copie_extra_entete_fonction = R"(
            /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
             * sont placés par la Syntaxeuse directement dans les membres. */
            POUR (*orig->bloc_constantes->membres.verrou_ecriture()) {
                auto copie_membre = copie_noeud(it);
                copie->bloc_constantes->ajoute_membre(copie_membre->comme_déclaration_constante());
            }
            copie->drapeaux_fonction = (orig->drapeaux_fonction & DrapeauxNoeudFonction::BITS_COPIABLES);
            )";

static const char *copie_extra_entête_opérateur_pour = R"(
            if ((m_options & OptionsCopieNoeud::COPIE_PARAMÈTRES_DANS_MEMBRES) != OptionsCopieNoeud(0)) {
                for (int64_t i = 0; i < copie->params.taille(); i++) {
                    copie->bloc_paramètres->membres->ajoute(copie->parametre_entree(i));
                }
                copie->bloc_paramètres->membres->ajoute(copie->param_sortie->comme_base_déclaration_variable());
            }
            )";

static const char *copie_extra_bloc = R"(
            copie->réserve_membres(orig->nombre_de_membres());
            POUR (*copie->expressions.verrou_lecture()) {
                if (it->est_déclaration_type() || it->est_entête_fonction()) {
                    copie->ajoute_membre(it->comme_déclaration());
                }
            })";

static const char *copie_extra_structure = R"(
            nracine->type = nullptr;
            if (orig->bloc_constantes) {
                /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
                 * sont placés par la Syntaxeuse directement dans les membres. */
                POUR (*orig->bloc_constantes->membres.verrou_ecriture()) {
                    auto copie_membre = copie_noeud(it);
                    copie->bloc_constantes->ajoute_membre(copie_membre->comme_déclaration_constante());
                }
            }
)";

static const char *copie_extra_union = R"(
            nracine->type = nullptr;
            if (orig->bloc_constantes) {
                /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
                 * sont placés par la Syntaxeuse directement dans les membres. */
                POUR (*orig->bloc_constantes->membres.verrou_ecriture()) {
                    auto copie_membre = copie_noeud(it);
                    copie->bloc_constantes->ajoute_membre(copie_membre->comme_déclaration_constante());
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

static const IdentifiantADN &type_nominal_membre_pour_noeud_code(Type *type)
{
    if (type->est_tableau()) {
        return type_nominal_membre_pour_noeud_code(type->comme_tableau()->type_pointe);
    }

    if (type->est_pointeur()) {
        return type_nominal_membre_pour_noeud_code(type->comme_pointeur()->type_pointe);
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

    void génère_fichier_prodéclaration(FluxSortieCPP &os)
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

    void genere_fichier_entete_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "biblinternes/outils/assert.hh");
        inclus(os, "biblinternes/moultfilage/synchrone.hh");
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

            protéine->pour_chaque_membre_recursif(
                [&noms_struct, &ensemble_noms](Membre const &membre) {
                    if (!membre.type->est_pointeur()) {
                        return;
                    }

                    const auto type_pointe = membre.type->comme_pointeur()->type_pointe;

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
        os << "enum class PreferenceVisiteNoeud : unsigned char {\n";
        os << "    ORIGINAL,\n";
        os << "    SUBSTITUTION,\n";
        os << "};\n\n";
        os << "void visite_noeud(NoeudExpression const *racine, PreferenceVisiteNoeud preference, "
              "bool ignore_blocs_non_traversables_des_si_statiques, "
              "std::function<DecisionVisiteNoeud(NoeudExpression const *)> const &rappel);\n\n";
    }

    void genere_fichier_source_arbre_syntaxique(FluxSortieCPP &os)
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

        genere_impression_arbre_syntaxique(os);
        genere_visite_noeud(os);
        genere_copie_noeud(os);

        os << "void imprime_genre_noeud_pour_assert(const NoeudExpression *noeud)\n";
        os << "{\n";
        os << R"(    std::cerr << "Le genre de noeud est " << noeud->genre << "\n";)";
        os << "\n";
        os << "}\n\n";
    }

    void genere_impression_arbre_syntaxique(FluxSortieCPP &os)
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

            os << " \" << (racine->ident ? racine->ident->nom : \"\") << \"";

            // À FAIRE : ceci ne prend pas en compte les ancêtres
            if (it->possède_enfants()) {
                os << ">\\n\";\n";
            }
            else {
                os << "/>\\n\";\n";
            }

            génère_code_pour_enfant(os, it, false, [&os](ProtéineStruct &, Membre const &membre) {
                if (membre.type->est_tableau()) {
                    const auto type_tableau = membre.type->comme_tableau();
                    if (type_tableau->est_synchrone) {
                        os << "\t\t\tPOUR ((*racine_typee->" << membre.nom
                           << ".verrou_lecture())) {\n";
                    }
                    else {
                        os << "\t\t\tPOUR (racine_typee->" << membre.nom << ") {\n";
                    }
                    os << "\t\t\t\timprime_arbre(it, os, profondeur + 1, substitution);\n";
                    os << "\t\t\t}\n";
                }
                else {
                    os << "\t\t\timprime_arbre(racine_typee->" << membre.nom
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

    void genere_visite_noeud(FluxSortieCPP &os)
    {
        os << "void visite_noeud(NoeudExpression const *racine, PreferenceVisiteNoeud preference, "
              "bool ignore_blocs_non_traversables_des_si_statiques, "
              "std::function<DecisionVisiteNoeud(NoeudExpression const *)> const &rappel)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tif (preference == PreferenceVisiteNoeud::SUBSTITUTION && racine->substitution) "
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
                os << "\t\t\t\tvisite_noeud(racine_typee->condition, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tif (racine_typee->condition_est_vraie) {\n";
                os << "\t\t\t\t\tvisite_noeud(racine_typee->bloc_si_vrai, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\t}\n\t\t\t\telse {\n";
                os << "\t\t\t\t\tvisite_noeud(racine_typee->bloc_si_faux, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\t}\n";
                os << "\t\t\t}\n";
                os << "\t\t\telse {\n";
            }

            génère_code_pour_enfant(
                os, it, false, [&os, &it](ProtéineStruct &, Membre const &membre) {
                    if (membre.type->est_tableau()) {
                        auto nom_membre = membre.nom.nom();
                        if (it->accede_nom_genre().nom() == "EXPRESSION_APPEL" &&
                            nom_membre == "paramètres") {
                            nom_membre = "paramètres_résolus";
                        }

                        const auto type_tableau = membre.type->comme_tableau();
                        if (type_tableau->est_synchrone) {
                            os << "\t\t\tPOUR ((*racine_typee->" << nom_membre
                               << ".verrou_lecture())) {\n";
                        }
                        else {
                            os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                        }
                        os << "\t\t\t\tvisite_noeud(it, preference, "
                              "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tvisite_noeud(racine_typee->" << membre.nom
                           << ", preference, ignore_blocs_non_traversables_des_si_statiques, "
                              "rappel);\n";
                    }
                });

            if (it->accede_nom_genre().nom() == "INSTRUCTION_BOUCLE") {
                os << "\t\t\tif (preference == PreferenceVisiteNoeud::SUBSTITUTION) {\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_inc, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sansarrêt, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sinon, preference, "
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

    void genere_copie_noeud(FluxSortieCPP &os)
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
                os << "\t\t\tauto copie_entete = "
                      "trouve_copie(orig->entête)->comme_entête_fonction();\n";
                os << "\t\t\tnracine = copie_entete->corps;\n";
            }
            else {
                os << "\t\t\tnracine = assem->crée_noeud<GenreNoeud::" << nom_genre
                   << ">(racine->lexème);\n";
            }

            os << "\t\t\tcopie_membres_de_bases_et_insère(racine, nracine);\n";

            if (!it->possède_enfants() && !it->possède_membre_a_copier()) {
                os << "\t\t\tbreak;\n";
                os << "\t\t}\n";
                continue;
            }

            os << "\t\t\tconst auto copie = nracine->comme_" << nom_comme << "();\n";

            auto copie_noeud = [&](const Membre &enfant) {
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

            it->pour_chaque_copie_extra_recursif([&](const Membre &enfant) {
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
                os << copie_extra_entete_fonction << "\n";
            }
            else if (nom_genre.nom() == "DÉCLARATION_OPÉRATEUR_POUR") {
                os << copie_extra_entete_fonction << "\n";
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

    void genere_fichier_entete_noeud_code(FluxSortieCPP &os)
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

            for (auto &membre : it->membres()) {
                if (!membre.est_code) {
                    continue;
                }
                const auto nom_membre = membre.nom;

                if (nom_membre.nom() == "type") {
                    os << "\tInfoType *type = nullptr;\n";
                    continue;
                }

                if (nom_membre.nom() == "lexème") {
                    os << "\tkuri::chaine_statique chemin_fichier{};\n";
                    os << "\tkuri::chaine_statique nom_fichier{};\n";
                    os << "\tint numero_ligne = 0;\n";
                    os << "\tint numero_colonne = 0;\n";
                    continue;
                }

                if (nom_membre.nom() == "ident") {
                    os << "\tkuri::chaine_statique nom{};\n";
                    continue;
                }

                if (nom_membre.nom() == "valeur" && nom_code.nom() == "NoeudCodeLittéraleChaine") {
                    os << "\tkuri::chaine_statique valeur{};\n";
                    continue;
                }

                os << "\t";

                if (membre.type->est_tableau()) {
                    os << "kuri::tableau<";
                }

                const auto ident_type = type_nominal_membre_pour_noeud_code(membre.type);
                const auto type_membre = ident_type.nom();
                os << type_membre;

                if (membre.type->est_pointeur() ||
                    (membre.type->est_tableau() &&
                     membre.type->comme_tableau()->type_pointe->est_pointeur())) {
                    os << " *";
                }

                if (membre.type->est_tableau()) {
                    os << '>';
                }

                os << ' ' << membre.nom << " = " << membre.type->valeur_defaut();
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

            if (it->nom().nom() == "Annotation") {
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

            if (it->nom().nom() == "Annotation") {
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
        os << "\tInfoType *crée_info_type_pour(Typeuse &typeuse, Type *type);\n\n";
        os << "\tType *convertis_info_type(Typeuse &typeuse, InfoType *type);\n\n";
        os << "\tvoid rassemble_statistiques(Statistiques &stats) const;\n\n";
        os << "\tint64_t mémoire_utilisée() const;\n";

        os << "};\n\n";
    }

    void genere_fichier_source_noeud_code(FluxSortieCPP &os)
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

            os << "\t\t\tauto n = noeuds_code_" << it->accede_nom_comme()
               << ".ajoute_élément();\n";
            // Renseigne directement le noeud code afin d'éviter les boucles infinies résultant en
            // des surempilages d'appels quand nous convertissons notamment les entêtes et les
            // corps de fonctions qui se référencent mutuellement.
            os << "\t\t\tracine->noeud_code = n;\n";

            génère_code_pour_enfant(
                os, it, true, [&os, it, this](ProtéineStruct &, Membre const &membre) {
                    const auto nom_membre = membre.nom;

                    if (nom_membre.nom() == "type") {
                        return;
                    }

                    if (nom_membre.nom() == "lexème") {
                        return;
                    }

                    if (nom_membre.nom() == "ident") {
                        return;
                    }

                    if (nom_membre.nom() == "genre") {
                        return;
                    }

                    if (nom_membre.nom() == "annotations") {
                        os << "\t\t\t\tn->annotations.réserve(racine_typee->annotations.taille());"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(membre.type->accede_nom().nom(),
                                                                nullptr);

                    if (!desc_type) {
                        if (membre.type->est_tableau()) {
                            const auto type_tableau = membre.type->comme_tableau();
                            if (type_tableau->est_synchrone) {
                                os << "\t\t\tPOUR (*racine_typee->" << nom_membre
                                   << ".verrou_lecture()) {\n";
                            }
                            else {
                                os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                            }
                            os << "\t\t\t\tn->" << nom_membre << ".ajoute(it);\n";
                            os << "\t\t\t}\n";
                        }
                        else {
                            if (nom_membre.nom() == "valeur" &&
                                it->accede_nom_code().nom() == "NoeudCodeLittéraleChaine") {
                                os << "\t\t\tn->valeur = { &racine_typee->lexème->chaine[0], "
                                      "racine_typee->lexème->chaine.taille() };\n";
                            }
                            else {
                                os << "\t\t\tn->" << nom_membre << " = racine_typee->"
                                   << nom_membre << ";\n";
                            }
                        }

                        return;
                    }

                    if (membre.type->est_tableau()) {
                        const auto type_tableau = membre.type->comme_tableau();
                        if (type_tableau->est_synchrone) {
                            os << "\t\t\tPOUR (*racine_typee->" << nom_membre
                               << ".verrou_lecture()) {\n";
                        }
                        else {
                            os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                        }
                        os << "\t\t\t\tn->" << nom_membre
                           << ".ajoute(convertis_noeud_syntaxique(espace, it)";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom() != "") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ");\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tif (racine_typee->" << nom_membre << ") {\n";
                        os << "\t\t\t\tn->" << nom_membre
                           << " = convertis_noeud_syntaxique(espace, racine_typee->" << nom_membre
                           << ")";
                        if (desc_type->accede_nom_code().nom() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom() != "") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ";\n";
                        os << "\t\t\t}\n";
                    }
                });

            os << "\t\t\tn->genre = racine_typee->genre;\n";
            os << "\t\t\tn->type = crée_info_type_pour(espace->compilatrice().typeuse, "
                  "racine_typee->type);\n";
            os << "\t\t\tif (racine_typee->ident) { n->nom = racine_typee->ident->nom; } else if "
                  "(racine_typee->lexème) { n->nom = racine_typee->lexème->chaine; }\n";

            os << "\t\t\tnoeud = n;\n";
            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";

        os << "\tconst auto lexeme = racine->lexème;\n";
        os << "\t// lexeme peut-être nul pour les blocs\n";
        os << "\tif (lexeme) {\n";
        os << "\t\tconst auto fichier = espace->compilatrice().fichier(lexeme->fichier);\n";
        os << "\t\tnoeud->chemin_fichier = fichier->chemin();\n";
        os << "\t\tnoeud->nom_fichier = fichier->nom();\n";
        os << "\t\tnoeud->numero_ligne = lexeme->ligne + 1;\n";
        os << "\t\tnoeud->numero_colonne = lexeme->colonne;\n";
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
                os, it, true, [&os, it, this](ProtéineStruct &, Membre const &membre) {
                    const auto nom_membre = membre.nom;

                    if (nom_membre.nom() == "type") {
                        return;
                    }

                    if (nom_membre.nom() == "lexème") {
                        return;
                    }

                    if (nom_membre.nom() == "ident") {
                        return;
                    }

                    if (nom_membre.nom() == "genre") {
                        return;
                    }

                    if (nom_membre.nom() == "annotations") {
                        os << "\t\t\t\tn->annotations.réserve(static_cast<int>(racine_typee->"
                              "annotations.taille()));"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(membre.type->accede_nom().nom(),
                                                                nullptr);

                    if (!desc_type) {
                        if (membre.type->est_tableau()) {
                            os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                            os << "\t\t\t\tn->" << nom_membre << membre.type->accesseur()
                               << "ajoute(it);\n";
                            os << "\t\t\t}\n";
                        }
                        else {
                            if (nom_membre.nom() == "valeur" &&
                                it->accede_nom_code().nom() == "NoeudCodeLittéraleChaine") {
                                os << "\t\tn->valeur = "
                                      "gérante_chaine.ajoute_chaine(racine_typee->valeur);\n";
                            }
                            else {
                                os << "\t\t\tn->" << nom_membre << " = racine_typee->"
                                   << nom_membre << ";\n";
                            }
                        }

                        return;
                    }

                    if (membre.type->est_tableau()) {
                        os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                        os << "\t\t\t\tn->" << nom_membre << membre.type->accesseur();
                        os << "ajoute(convertis_noeud_code(espace, gérante_chaine, it)";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ");\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tif (racine_typee->" << nom_membre << ") {\n";
                        os << "\t\t\t\tn->" << nom_membre
                           << " = convertis_noeud_code(espace, gérante_chaine, racine_typee->"
                           << nom_membre << ")";

                        if (desc_type->accede_nom_code().nom() != "NoeudCode") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ";\n";
                        os << "\t\t\t}\n";
                    }
                });

            os << "\t\t\tn->genre = racine_typee->genre;\n";
            os << "\t\t\tn->type = convertis_info_type(espace->compilatrice().typeuse, "
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

        os << "\tmem += allocatrice_infos_types.memoire_utilisee();\n";

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
                                 std::function<void(ProtéineStruct &, const Membre &)> rappel)
    {
        auto possède_enfants = false;
        auto noeud_courant = racine;

        while (noeud_courant) {
            if (noeud_courant->possède_enfants()) {
                possède_enfants = true;
                break;
            }

            if (inclus_code && !possède_enfants) {
                for (const auto &membre : noeud_courant->membres()) {
                    if (membre.est_code) {
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
            for (const auto &membre : noeud_courant->membres()) {
                if (membre.est_enfant || (inclus_code && membre.est_code)) {
                    rappel(*noeud_courant, membre);
                }
            }

            noeud_courant = noeud_courant->mere();
        }
    }

    void genere_fichier_source_assembleuse(FluxSortieCPP &os)
    {
        inclus(os, "assembleuse.hh");

        const char *empile_bloc = R"(
NoeudBloc *AssembleuseArbre::empile_bloc(Lexème const *lexeme, NoeudDéclarationEntêteFonction *appartiens_à_fonction)
{
    auto bloc = crée_noeud<GenreNoeud::INSTRUCTION_COMPOSÉE>(lexeme)->comme_bloc();
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
        résultat->lexème = lexeme;
        résultat->ident = lexeme->ident;
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
               << "(const Lexème *lexeme";

            auto membres_construction = it->donne_membres_pour_construction();
            POUR_NOMME (membre, membres_construction) {
                os << ", " << *membre.type << " " << membre.nom;
            }

            os << ")\n{\n";

            if (it->nom().nom() == "NoeudExpressionRéférence") {
                os << recycle_référence;
            }

            if (membres_construction.est_vide()) {
                os << "\treturn crée_noeud<GenreNoeud::" << nom_genre << ">(lexeme)->comme_"
                   << it->accede_nom_comme() << "();\n";
            }
            else {
                os << "\tauto résultat = crée_noeud<GenreNoeud::" << nom_genre
                   << ">(lexeme)->comme_" << it->accede_nom_comme() << "();\n";

                POUR_NOMME (membre, membres_construction) {
                    os << "\trésultat->" << membre.nom << " = " << membre.nom << ";\n";
                }

                os << "\treturn résultat;\n";
            }

            os << "}\n";
        }
    }

    void genere_fichier_entete_assembleuse(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        inclus(os, "allocatrice.hh");
        inclus(os, "structures/pile.hh");
        os << "struct NoeudDéclarationTypeComposé;\n";
        os << "using TypeCompose = NoeudDéclarationTypeComposé;\n";
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

    NoeudBloc *empile_bloc(Lexème const *lexeme, NoeudDéclarationEntêteFonction *appartiens_à_fonction);

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
        m_blocs.depile();
    }

    /* Utilisation d'un gabarit car à part pour les copies, nous connaissons
     * toujours le genre de noeud à créer, et spécialiser cette fonction nous
     * économise pas mal de temps d'exécution, au prix d'un exécutable plus gros. */
    template <GenreNoeud genre>
    NoeudExpression *crée_noeud(Lexème const *lexeme)
    {
        auto noeud = m_allocatrice.crée_noeud<genre>();
        noeud->genre = genre;
        noeud->lexème = lexeme;
        noeud->bloc_parent = bloc_courant();

        if (noeud->lexème && (noeud->lexème->genre == GenreLexème::CHAINE_CARACTERE)) {
            noeud->ident = lexeme->ident;
        }

        if (genre == GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION) {
            auto entete = noeud->comme_entête_fonction();
            entete->corps->lexème = lexeme;
            entete->corps->ident = lexeme->ident;
            entete->corps->bloc_parent = entete->bloc_parent;
        }

        if (genre == GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE) {
            /* transfère l'index car les lexèmes peuvent être partagés lors de la simplification du code ou des exécutions */
            noeud->comme_littérale_chaine()->valeur = lexeme->index_chaine;
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
               << "(const Lexème *lexeme";

            auto membres_construction = it->donne_membres_pour_construction();
            POUR_NOMME (membre, membres_construction) {
                os << ", " << *membre.type << " " << membre.nom;
            }

            os << ");\n";
        }

        const char *decls_extras = R"(
    NoeudSi *crée_si(const Lexème *lexeme, GenreNoeud genre_noeud);
    NoeudDéclarationVariable *crée_déclaration_variable(NoeudExpressionRéférence *ref);
    NoeudAssignation *crée_incrementation(const Lexème *lexeme, NoeudExpression *valeur);
    NoeudAssignation *crée_decrementation(const Lexème *lexeme, NoeudExpression *valeur);
    NoeudBloc *crée_bloc_seul(const Lexème *lexeme, NoeudBloc *bloc_parent);
    NoeudDéclarationVariable *crée_déclaration_variable(const Lexème *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression);
    NoeudDéclarationVariable *crée_déclaration_variable(NoeudExpressionRéférence *ref, NoeudExpression *expression);
    NoeudExpressionLittéraleEntier *crée_littérale_entier(const Lexème *lexeme, Type *type, uint64_t valeur);
    NoeudExpressionLittéraleBool *crée_littérale_bool(const Lexème *lexeme, Type *type, bool valeur);
    NoeudExpressionLittéraleRéel *crée_littérale_réel(const Lexème *lexeme, Type *type, double valeur);
    NoeudExpression *crée_référence_type(const Lexème *lexeme, Type *type);
    NoeudExpressionAppel *crée_appel(const Lexème *lexeme, NoeudExpression *appelee, Type *type);
    NoeudExpressionBinaire *crée_indexage(const Lexème *lexeme, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification);
    NoeudExpressionBinaire *crée_expression_binaire(const Lexème *lexeme, OpérateurBinaire const *op, NoeudExpression *expr1, NoeudExpression *expr2);
    NoeudExpressionMembre *crée_référence_membre(const Lexème *lexeme, NoeudExpression *accede, Type *type, int index);
    NoeudExpressionRéférence *crée_référence_déclaration(const Lexème *lexeme, NoeudDéclaration *decl);
    NoeudExpressionAppel *crée_construction_structure(const Lexème *lexeme, TypeCompose *type);
)";

        os << decls_extras;

        os << "};\n";
    }

    void genere_fichier_source_allocatrice(FluxSortieCPP &os)
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
                                   kuri::chaine_statique nom_membre) -> kuri::chaine {
            return enchaine("taille_max_", nom_comme, "_", nom_membre);
        };

        POUR (protéines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }
            if (!it->possède_tableau()) {
                continue;
            }

            it->pour_chaque_membre_recursif([&](const Membre &membre) {
                if (membre.type->est_tableau() || membre.nom.nom() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              membre.nom.nom());
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

            os << "auto memoire_" << nom_comme << " = int64_t(0);\n";
            os << "pour_chaque_élément(m_noeuds_" << nom_comme << ", [&](";
            os << it->nom() << " const &noeud) {\n";
            it->pour_chaque_membre_recursif([&](const Membre &membre) {
                if (membre.type->est_tableau()) {
                    const auto nom_membre = membre.nom;
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              nom_membre.nom());
                    os << nom_tableau << " = std::max(" << nom_tableau << ", noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille());\n";
                    os << "memoire_" << nom_comme << " += noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille_mémoire();\n";
                    os << "mémoire_gaspillée"
                       << " += noeud." << nom_membre;
                    os << membre.type->accesseur() << "gaspillage_mémoire();\n";
                }
                else if (membre.nom.nom() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom(),
                                                              membre.nom.nom());

                    os << "if (noeud.monomorphisations) {\n";
                    os << "memoire_" << nom_comme
                       << " += noeud.monomorphisations->mémoire_utilisée();\n";
                    os << nom_tableau << " = std::max(" << nom_tableau
                       << ", noeud.monomorphisations->nombre_items_max());\n";
                    os << "}\n";
                }
            });
            os << "});\n";

            os << "\tstats_arbre.fusionne_entrée({" << '"' << it->nom() << '"' << ", "
               << "0, "
               << "memoire_" << nom_comme << "});\n";
        }

        os << "\tstats_gaspillage.fusionne_entrée({\"Tableaux Arbre\", 1, mémoire_gaspillée});\n";

        os << "auto &stats_tableaux = stats.stats_tableaux;\n";
        noms_tableaux.pour_chaque_element([&](kuri::chaine_statique it) {
            os << "stats_tableaux.fusionne_entrée({\"" << it << "\", " << it << "});\n";
        });

        os << "}\n";
    }

    void genere_fichier_entete_allocatrice(FluxSortieCPP &os)
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
                    os << "\t\t\t\tauto entete = m_noeuds_entête_fonction.ajoute_élément();\n";
                }
                else {
                    os << "\t\t\t\tauto entete = m_noeuds_opérateur_pour.ajoute_élément();\n";
                }
                os << "\t\t\t\tauto corps  = m_noeuds_corps_fonction.ajoute_élément();\n";
                os << "\t\t\t\tentete->corps = corps;\n";
                os << "\t\t\t\tcorps->entête = entete;\n";
                os << "\t\t\t\treturn entete;\n";
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
    void genere_fichier_kuri_noeud_code(FluxSortieKuri &os,
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
    fichier.tampon_ = lng::tampon_source(texte.c_str());
    fichier.chemin_ = chemin_adn;

    auto gérante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
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
        generatrice.genere_fichier_source_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_expression.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_code.cc") {
        {
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
            auto flux = FluxSortieCPP(fichier_sortie);
            generatrice.genere_fichier_source_noeud_code(flux);
            if (!remplace_si_différent(nom_fichier_tmp, argv[1])) {
                return 1;
            }
        }
        {
            // Génère le fichier de message pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.remplace_nom_fichier("../modules/Compilatrice/code.kuri");
            std::ofstream fichier_sortie(vers_std_path(nom_fichier_sortie));
            auto flux = FluxSortieKuri(fichier_sortie);
            GeneratriceCodeKuri generatrice_kuri;
            generatrice_kuri.genere_fichier_kuri_noeud_code(flux, syntaxeuse.protéines);
        }
    }
    else if (nom_fichier_sortie.nom_fichier() == "noeud_code.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_noeud_code(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "assembleuse.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_source_assembleuse(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "assembleuse.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_assembleuse(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "allocatrice.cc") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_source_allocatrice(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "allocatrice.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_allocatrice(flux);
    }
    else if (nom_fichier_sortie.nom_fichier() == "prodeclaration.hh") {
        std::ofstream fichier_sortie(vers_std_path(nom_fichier_tmp));
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.génère_fichier_prodéclaration(flux);
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
