/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include <fstream>
#include <iostream>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/badge.hh"
#include "biblinternes/outils/conditions.h"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "structures/chemin_systeme.hh"
#include "structures/ensemble.hh"
#include "structures/table_hachage.hh"

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
                copie->bloc_constantes->ajoute_membre(copie_membre->comme_declaration_variable());
            }
            copie->drapeaux_fonction = (orig->drapeaux_fonction & DrapeauxNoeudFonction::BITS_COPIABLES);
            )";

static const char *copie_extra_entête_opérateur_pour = R"(
            if ((m_options & OptionsCopieNoeud::COPIE_PARAMÈTRES_DANS_MEMBRES) != OptionsCopieNoeud(0)) {
                for (int64_t i = 0; i < copie->params.taille(); i++) {
                    copie->bloc_parametres->membres->ajoute(copie->parametre_entree(i));
                }
                copie->bloc_parametres->membres->ajoute(copie->param_sortie->comme_declaration_variable());
            }
            )";

static const char *copie_extra_bloc = R"(
            copie->reserve_membres(orig->nombre_de_membres());
            POUR (*copie->expressions.verrou_lecture()) {
                if (it->est_declaration_type() || it->est_entete_fonction()) {
                    copie->ajoute_membre(it->comme_declaration());
                }
            })";

static const char *copie_extra_structure = R"(
            nracine->type = nullptr;
            if (orig->bloc_constantes) {
                /* La copie d'un bloc ne copie que les expressions mais les paramètres polymorphiques
                 * sont placés par la Syntaxeuse directement dans les membres. */
                POUR (*orig->bloc_constantes->membres.verrou_ecriture()) {
                    auto copie_membre = copie_noeud(it);
                    copie->bloc_constantes->ajoute_membre(copie_membre->comme_declaration_variable());
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
            auto référence_existante = trouve_copie(orig->declaration_referee);
            if (référence_existante) {
                copie->declaration_referee = référence_existante->comme_declaration();
            }
            else if (orig->declaration_referee && orig->declaration_referee->possède_drapeau(DrapeauxNoeud::EST_DÉCLARATION_EXPRESSION_VIRGULE)) {
                copie->declaration_referee = copie_noeud(orig->declaration_referee)->comme_declaration();
            }
            else {
                copie->declaration_referee = orig->declaration_referee;
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

    if (!type_nominal->est_proteine) {
        return type_nominal->nom_cpp;
    }

    auto proteine = type_nominal->est_proteine->comme_struct();
    if (!proteine) {
        return type_nominal->nom_cpp;
    }

    return proteine->accede_nom_code();
}

struct GeneratriceCodeCPP {
    kuri::tableau<Proteine *> proteines{};
    kuri::tableau<ProteineStruct *> proteines_struct{};
    kuri::table_hachage<kuri::chaine_statique, ProteineStruct *> table_desc{"Protéines"};

    void genere_fichier_entete_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        os << "#include \"biblinternes/outils/assert.hh\"\n";
        os << "#include \"biblinternes/moultfilage/synchrone.hh\"\n";
        os << "#include \"structures/chaine.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"structures/tableau.hh\"\n";
        os << "#include \"structures/tableau_compresse.hh\"\n";
        os << "#include \"structures/table_hachage.hh\"\n";
        os << "#include \"compilation/transformation_type.hh\"\n";
        os << "#include \"parsage/lexemes.hh\"\n";
        os << "#include \"expression.hh\"\n";
        os << "#include \"utilitaires.hh\"\n";
        os << "class Broyeuse;\n";
        os << "struct Enchaineuse;\n";

        // Prodéclarations des structures
        kuri::ensemble<kuri::chaine> noms_struct;

        POUR (proteines) {
            if (it->comme_struct()) {
                auto proteine = it->comme_struct();

                proteine->pour_chaque_membre_recursif([&noms_struct](Membre const &membre) {
                    if (!membre.type->est_pointeur()) {
                        return;
                    }

                    const auto type_pointe = membre.type->comme_pointeur()->type_pointe;

                    if (!type_pointe->est_nominal()) {
                        return;
                    }

                    const auto type_nominal = type_pointe->comme_nominal();
                    const auto nom_type = type_nominal->nom_cpp.nom_cpp();

                    noms_struct.insère(nom_type);
                });

                noms_struct.insère(it->nom().nom_cpp());
            }
        }

        noms_struct.pour_chaque_element(
            [&](kuri::chaine_statique it) { os << "struct " << it << ";\n"; });
        os << "\n";

        // Les structures C++
        POUR (proteines) {
            it->genere_code_cpp(os, true);
        }

        os << "void imprime_genre_noeud_pour_assert(const NoeudExpression *noeud);\n\n";

        POUR (proteines_struct) {
            it->genere_code_cpp_apres_declaration(os);
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
        os << "struct Etendue {\n";
        os << "\tint64_t pos_min = 0;\n";
        os << "\tint64_t pos_max = 0;\n";
        os << "\n";
        os << "\tvoid fusionne(Etendue autre)\n";
        os << "\t{\n";
        os << "\t\tpos_min = (pos_min < autre.pos_min) ? pos_min : autre.pos_min;\n";
        os << "\t\tpos_max = (pos_max > autre.pos_max) ? pos_max : autre.pos_max;\n";
        os << "\t}\n";
        os << "};\n\n";
        os << "Etendue calcule_etendue_noeud(NoeudExpression const *racine);\n\n";
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
        os << "#include \"noeud_expression.hh\"\n";
        os << "#include \"compilation/log.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"structures/enchaineuse.hh\"\n";
        os << "#include \"parsage/identifiant.hh\"\n";
        os << "#include \"parsage/outils_lexemes.hh\"\n";
        os << "#include \"assembleuse.hh\"\n";
        os << "#include \"copieuse.hh\"\n";
        os << "#include <iostream>\n";

        POUR (proteines) {
            it->genere_code_cpp(os, false);
        }

        genere_impression_arbre_syntaxique(os);
        genere_calcul_etendue_noeud(os);
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

        POUR (proteines_struct) {
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

            genere_code_pour_enfant(os, it, false, [&os](ProteineStruct &, Membre const &membre) {
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

    void genere_calcul_etendue_noeud(FluxSortieCPP &os)
    {
        os << "Etendue calcule_etendue_noeud(NoeudExpression const *racine)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn {};\n";
        os << "\t}\n";
        os << "\tconst auto lexeme = racine->lexeme;\n";
        os << "\tif (!lexeme) {\n";
        os << "\t\treturn {};\n";
        os << "\t}\n";
        os << "\tconst auto pos = position_lexeme(*lexeme);\n";
        os << "\tauto etendue = Etendue{};\n";
        os << "\tetendue.pos_min = pos.pos;\n";
        os << "\tetendue.pos_max = pos.pos + lexeme->chaine.taille();\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (proteines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            genere_code_pour_enfant(os, it, false, [&os](ProteineStruct &, Membre const &membre) {
                if (membre.type->est_tableau()) {
                    const auto type_tableau = membre.type->comme_tableau();
                    if (type_tableau->est_synchrone) {
                        os << "\t\t\tPOUR ((*racine_typee->" << membre.nom
                           << ".verrou_lecture())) {\n";
                    }
                    else {
                        os << "\t\t\tPOUR (racine_typee->" << membre.nom << ") {\n";
                    }
                    os << "\t\t\t\tconst auto etendue_" << membre.nom
                       << " = calcule_etendue_noeud(it);\n";
                    os << "\t\t\t\tetendue.fusionne(etendue_" << membre.nom << ");\n";
                    os << "\t\t\t}\n";
                }
                else {
                    os << "\t\t\tconst auto etendue_" << membre.nom
                       << " = calcule_etendue_noeud(racine_typee->" << membre.nom << ");\n";
                    os << "\t\t\tetendue.fusionne(etendue_" << membre.nom << ");\n";
                }
            });

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "\treturn etendue;\n";
        os << "}\n";
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

        POUR (proteines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            if (it->accede_nom_genre().nom_cpp() == "INSTRUCTION_SI_STATIQUE" ||
                it->accede_nom_genre().nom_cpp() == "INSTRUCTION_SAUFSI_STATIQUE") {
                os << "\t\t\tif (ignore_blocs_non_traversables_des_si_statiques) {\n";
                os << "\t\t\t\tconst auto racine_typee = racine->comme_si_statique();\n";
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

            genere_code_pour_enfant(
                os, it, false, [&os, &it](ProteineStruct &, Membre const &membre) {
                    if (membre.type->est_tableau()) {
                        auto nom_membre = membre.nom.nom_cpp();
                        if (it->accede_nom_genre().nom_cpp() == "EXPRESSION_APPEL" &&
                            nom_membre == "parametres") {
                            nom_membre = "parametres_resolus";
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

            if (it->accede_nom_genre().nom_cpp() == "INSTRUCTION_BOUCLE") {
                os << "\t\t\tif (preference == PreferenceVisiteNoeud::SUBSTITUTION) {\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_pre, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_inc, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sansarret, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t\tvisite_noeud(racine_typee->bloc_sinon, preference, "
                      "ignore_blocs_non_traversables_des_si_statiques, rappel);\n";
                os << "\t\t\t}\n";
            }
            else if (it->accede_nom_genre().nom_cpp() == "INSTRUCTION_SI_STATIQUE" ||
                     it->accede_nom_genre().nom_cpp() == "INSTRUCTION_SAUFSI_STATIQUE") {
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

        POUR (proteines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            const auto nom_genre = it->accede_nom_genre();
            const auto nom_comme = it->accede_nom_comme();
            os << "\t\tcase GenreNoeud::" << nom_genre << ":\n";
            os << "\t\t{\n";
            os << "\t\t\tconst auto orig = racine->comme_" << nom_comme << "();\n";

            /* Les corps sont créés directement avec les entêtes. */
            if (nom_genre.nom_cpp() == "DECLARATION_CORPS_FONCTION") {
                os << "\t\t\tauto copie_entete = "
                      "trouve_copie(orig->entete)->comme_entete_fonction();\n";
                os << "\t\t\tnracine = copie_entete->corps;\n";
            }
            else {
                os << "\t\t\tnracine = assem->crée_noeud<GenreNoeud::" << nom_genre
                   << ">(racine->lexeme);\n";
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

                if (it->accede_nom_genre().nom_cpp() == "EXPRESSION_REFERENCE_DECLARATION") {
                    if (enfant.nom.nom_cpp() == "declaration_referee") {
                        os << copie_déclaration_référée;
                        return;
                    }
                }

                os << "\t\t\t";

                if (enfant.type->est_tableau()) {
                    const auto type_tableau = enfant.type->comme_tableau();
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "reserve("
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

                    if (enfant.type->accede_nom().nom_cpp() != "NoeudExpression") {
                        os << "static_cast<" << enfant.type->accede_nom() << " *>(";
                    }

                    os << "copie_noeud(it)";

                    if (enfant.type->accede_nom().nom_cpp() != "NoeudExpression") {
                        os << ")";
                    }

                    os << ");\n";

                    os << "\t\t\t}\n";
                }
                else {
                    os << "copie->" << nom_enfant << " = ";

                    if (enfant.type->accede_nom().nom_cpp() != "NoeudExpression") {
                        os << "static_cast<" << enfant.type->accede_nom() << " *>(";
                    }

                    os << "copie_noeud(orig->" << nom_enfant << ")";

                    if (enfant.type->accede_nom().nom_cpp() != "NoeudExpression") {
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
                    os << "copie->" << nom_enfant << enfant.type->accesseur() << "reserve("
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

            if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION") {
                os << copie_extra_entete_fonction << "\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_OPERATEUR_POUR") {
                os << copie_extra_entete_fonction << "\n";
                os << copie_extra_entête_opérateur_pour << "\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_STRUCTURE") {
                os << copie_extra_structure;
            }
            else if (nom_genre.nom_cpp() == "INSTRUCTION_COMPOSEE") {
                os << copie_extra_bloc << "\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_ENUM") {
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
        os << "#include \"noeud_expression.hh\"\n";
        os << "#include <iostream>\n";
        os << "#include \"biblinternes/structures/tableau_page.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"structures/tableau.hh\"\n";
        os << "#include \"infos_types.hh\"\n";
        os << "struct EspaceDeTravail;\n";
        os << "struct InfoType;\n";
        os << "struct Statistiques;\n";

        // Prodéclarations des structures
        kuri::ensemble<kuri::chaine> noms_struct;

        POUR (proteines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (!nom_code.est_nul()) {
                noms_struct.insère(nom_code.nom_cpp());
            }
        }

        noms_struct.pour_chaque_element(
            [&](kuri::chaine_statique it) { os << "struct " << it << ";\n"; });
        os << "struct GeranteChaine;\n";
        os << "\n";

        // Les structures C++ pour NoeudCode
        POUR (proteines_struct) {
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

                if (nom_membre.nom_cpp() == "type") {
                    os << "\tInfoType *type = nullptr;\n";
                    continue;
                }

                if (nom_membre.nom_cpp() == "lexeme") {
                    os << "\tkuri::chaine_statique chemin_fichier{};\n";
                    os << "\tkuri::chaine_statique nom_fichier{};\n";
                    os << "\tint numero_ligne = 0;\n";
                    os << "\tint numero_colonne = 0;\n";
                    continue;
                }

                if (nom_membre.nom_cpp() == "ident") {
                    os << "\tkuri::chaine_statique nom{};\n";
                    continue;
                }

                if (nom_membre.nom_cpp() == "valeur" &&
                    nom_code.nom_cpp() == "NoeudCodeLitteraleChaine") {
                    os << "\tkuri::chaine_statique valeur{};\n";
                    continue;
                }

                os << "\t";

                if (membre.type->est_tableau()) {
                    os << "kuri::tableau<";
                }

                const auto ident_type = type_nominal_membre_pour_noeud_code(membre.type);
                const auto type_membre = supprime_accents(ident_type.nom_cpp());
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
            if (nom_code.nom_cpp() == "NoeudCode") {
                for (const auto &noeud : proteines_struct) {
                    const auto nom_comme = noeud->accede_nom_comme();

                    if (nom_comme.est_nul()) {
                        continue;
                    }

                    const auto nom_noeud = noeud->accede_nom_code();

                    if (nom_noeud.est_nul()) {
                        continue;
                    }

                    os << "\n";
                    os << "\tinline bool est_" << nom_comme << "() const;\n";
                    os << "\tinline " << nom_noeud << " *comme_" << nom_comme << "();\n";
                    os << "\tinline const " << nom_noeud << " *comme_" << nom_comme
                       << "() const;\n";
                }
            }

            os << "};\n\n";
        }

        os << "void imprime_genre_noeud_pour_assert(const NoeudCode *noeud);\n\n";

        // Implémente les fonctions de transtypage
        POUR (proteines_struct) {
            const auto nom_comme = it->accede_nom_comme();

            if (nom_comme.est_nul()) {
                continue;
            }

            const auto nom_noeud = it->accede_nom_code();

            if (nom_noeud.est_nul()) {
                continue;
            }

            if (it->est_racine_soushierachie() && it->accede_nom_genre().est_nul()) {
                os << "inline bool NoeudCode::est_" << nom_comme << "() const\n";
                os << "{\n";
                os << "\t ";

                auto separateur = "return";

                for (auto &derive : it->derivees()) {
                    os << separateur << " this->est_" << derive->accede_nom_comme() << "()";
                    separateur = "|| ";
                }

                os << ";\n";
                os << "}\n\n";
            }
            else {
                os << "inline bool NoeudCode::est_" << nom_comme << "() const\n";
                os << "{\n";

                if (it->est_racine_soushierachie()) {
                    os << "\treturn this->genre == GenreNoeud::" << it->accede_nom_genre();

                    for (auto &derive : it->derivees()) {
                        os << " || this->genre == GenreNoeud::" << derive->accede_nom_genre();
                    }

                    os << ";\n";
                }
                else {
                    os << "\treturn this->genre == GenreNoeud::" << it->accede_nom_genre()
                       << ";\n";
                }

                os << "}\n\n";
            }

            os << "inline " << nom_noeud << " *NoeudCode::comme_" << nom_comme << "()\n";
            os << "{\n";
            os << "\tassert_rappel(est_" << nom_comme
               << "(), [this]() { imprime_genre_noeud_pour_assert(this); });\n";
            os << "\treturn static_cast<" << nom_noeud << " *>(this);\n";
            os << "}\n\n";

            os << "inline const " << nom_noeud << " *NoeudCode::comme_" << nom_comme
               << "() const\n";
            os << "{\n";
            os << "\tassert_rappel(est_" << nom_comme
               << "(), [this]() { imprime_genre_noeud_pour_assert(this); });\n";
            os << "\treturn static_cast<const " << nom_noeud << " *>(this);\n";
            os << "}\n\n";
        }

        // Implémente une convertisseuse

        os << "struct ConvertisseuseNoeudCode {\n";
        // les allocations de noeuds codes
        os << "\t// Allocations des noeuds codes\n";
        POUR (proteines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            if (it->nom().nom_cpp() == "Annotation") {
                continue;
            }

            os << "\ttableau_page<" << nom_code << "> noeuds_code_" << it->accede_nom_comme()
               << "{};\n";
        }
        os << "\n";

        // les allocations de noeuds expressions
        os << "\t// Allocations des noeuds expressions\n";
        // À FAIRE : blocs parent, corps fonctions
        POUR (proteines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            if (it->nom().nom_cpp() == "Annotation") {
                continue;
            }

            os << "\ttableau_page<" << it->nom() << "> noeuds_expr_" << it->accede_nom_comme()
               << ";\n";
        }
        os << "\n";

        // Les allocations des infos types
        os << "\t// Les allocations des infos types\n";
        os << "\tAllocatriceInfosType allocatrice_infos_types{};\n\n";

        os << "\tNoeudExpression *convertis_noeud_code(EspaceDeTravail *espace, GeranteChaine "
              "&gerante_chaine, NoeudCode *racine);\n\n";
        os << "\tNoeudCode *convertis_noeud_syntaxique(EspaceDeTravail *espace, NoeudExpression "
              "*racine);\n\n";
        os << "\tInfoType *crée_info_type_pour(Type *type);\n\n";
        os << "\tType *convertis_info_type(Typeuse &typeuse, InfoType *type);\n\n";
        os << "\tvoid rassemble_statistiques(Statistiques &stats) const;\n\n";
        os << "\tint64_t memoire_utilisee() const;\n";

        os << "};\n\n";
    }

    void genere_fichier_source_noeud_code(FluxSortieCPP &os)
    {
        os << "#include \"noeud_code.hh\"\n";
        os << "#include \"compilation/compilatrice.hh\"\n";
        os << "#include \"compilation/espace_de_travail.hh\"\n";
        os << "#include \"parsage/identifiant.hh\"\n";
        os << "#include \"parsage/gerante_chaine.hh\"\n";
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

        POUR (proteines_struct) {
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
               << ".ajoute_element();\n";
            // Renseigne directement le noeud code afin d'éviter les boucles infinies résultant en
            // des surempilages d'appels quand nous convertissons notamment les entêtes et les
            // corps de fonctions qui se référencent mutuellement.
            os << "\t\t\tracine->noeud_code = n;\n";

            genere_code_pour_enfant(
                os, it, true, [&os, it, this](ProteineStruct &, Membre const &membre) {
                    const auto nom_membre = membre.nom;

                    if (nom_membre.nom_cpp() == "type") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "lexeme") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "ident") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "genre") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "annotations") {
                        os << "\t\t\t\tn->annotations.reserve(racine_typee->annotations.taille());"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(
                        membre.type->accede_nom().nom_kuri(), nullptr);

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
                            if (nom_membre.nom_cpp() == "valeur" &&
                                it->accede_nom_code().nom_cpp() == "NoeudCodeLitteraleChaine") {
                                os << "\t\t\tn->valeur = { &racine_typee->lexeme->chaine[0], "
                                      "racine_typee->lexeme->chaine.taille() };\n";
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

                        if (desc_type->accede_nom_code().nom_cpp() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom_cpp() != "") {
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
                        if (desc_type->accede_nom_code().nom_cpp() != "NoeudCode" &&
                            desc_type->accede_nom_code().nom_cpp() != "") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ";\n";
                        os << "\t\t\t}\n";
                    }
                });

            os << "\t\t\tn->genre = racine_typee->genre;\n";
            os << "\t\t\tn->type = crée_info_type_pour(racine_typee->type);\n";
            os << "\t\t\tif (racine_typee->ident) { n->nom = racine_typee->ident->nom; } else if "
                  "(racine_typee->lexeme) { n->nom = racine_typee->lexeme->chaine; }\n";

            os << "\t\t\tnoeud = n;\n";
            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";

        os << "\tconst auto lexeme = racine->lexeme;\n";
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
              "*espace, GeranteChaine &gerante_chaine, NoeudCode *racine)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";
        os << "\tNoeudExpression *noeud = nullptr;\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (proteines_struct) {
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
               << ".ajoute_element();\n";

            genere_code_pour_enfant(
                os, it, true, [&os, it, this](ProteineStruct &, Membre const &membre) {
                    const auto nom_membre = membre.nom;

                    if (nom_membre.nom_cpp() == "type") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "lexeme") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "ident") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "genre") {
                        return;
                    }

                    if (nom_membre.nom_cpp() == "annotations") {
                        os << "\t\t\t\tn->annotations.reserve(static_cast<int>(racine_typee->"
                              "annotations.taille()));"
                              "\n";
                        os << "\t\t\tPOUR (racine_typee->annotations) {\n";
                        os << "\t\t\t\tn->annotations.ajoute({it.nom, it.valeur});\n";
                        os << "\t\t\t}\n";
                        return;
                    }

                    const auto desc_type = table_desc.valeur_ou(
                        membre.type->accede_nom().nom_kuri(), nullptr);

                    if (!desc_type) {
                        if (membre.type->est_tableau()) {
                            os << "\t\t\tPOUR (racine_typee->" << nom_membre << ") {\n";
                            os << "\t\t\t\tn->" << nom_membre << membre.type->accesseur()
                               << "ajoute(it);\n";
                            os << "\t\t\t}\n";
                        }
                        else {
                            if (nom_membre.nom_cpp() == "valeur" &&
                                it->accede_nom_code().nom_cpp() == "NoeudCodeLitteraleChaine") {
                                os << "\t\tn->valeur = "
                                      "gerante_chaine.ajoute_chaine(racine_typee->valeur);\n";
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
                        os << "ajoute(convertis_noeud_code(espace, gerante_chaine, it)";

                        if (desc_type->accede_nom_code().nom_cpp() != "NoeudCode") {
                            os << "->comme_" << desc_type->accede_nom_comme() << "()";
                        }

                        os << ");\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tif (racine_typee->" << nom_membre << ") {\n";
                        os << "\t\t\t\tn->" << nom_membre
                           << " = convertis_noeud_code(espace, gerante_chaine, racine_typee->"
                           << nom_membre << ")";

                        if (desc_type->accede_nom_code().nom_cpp() != "NoeudCode") {
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

        os << "int64_t ConvertisseuseNoeudCode::memoire_utilisee() const\n";
        os << "{\n";

        os << "\tauto mem = int64_t(0);\n";

        POUR (proteines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (nom_code.est_nul()) {
                continue;
            }

            os << "\tmem += noeuds_code_" << it->accede_nom_comme() << ".memoire_utilisee();\n";
            os << "\tmem += noeuds_expr_" << it->accede_nom_comme() << ".memoire_utilisee();\n";
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

    void genere_code_pour_enfant(FluxSortieCPP &os,
                                 ProteineStruct *racine,
                                 bool inclus_code,
                                 std::function<void(ProteineStruct &, const Membre &)> rappel)
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

        os << "\t\t\tconst auto racine_typee = racine->comme_" << racine->accede_nom_comme()
           << "();\n";

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
        os << "#include \"assembleuse.hh\"\n";

        const char *empile_bloc = R"(
NoeudBloc *AssembleuseArbre::empile_bloc(Lexeme const *lexeme, NoeudDeclarationEnteteFonction *appartiens_à_fonction)
{
	auto bloc = static_cast<NoeudBloc *>(crée_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme));
    bloc->appartiens_à_fonction = appartiens_à_fonction;
	bloc->bloc_parent = bloc_courant();
	m_blocs.empile(bloc);
	return bloc;
}
)";

        os << empile_bloc;

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << it->nom() << " *AssembleuseArbre::crée_" << it->accede_nom_comme()
               << "(const Lexeme *lexeme)\n";
            os << "{\n";
            os << "\treturn crée_noeud<GenreNoeud::" << nom_genre << ">(lexeme)->comme_"
               << it->accede_nom_comme() << "();\n";
            os << "}\n";
        }
    }

    void genere_fichier_entete_assembleuse(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        os << "#include \"allocatrice.hh\"\n";
        os << "#include \"structures/pile.hh\"\n";
        os << "struct TypeCompose;\n";
        os << "struct AssembleuseArbre {\n";
        os << "private:\n";
        os << "\tAllocatriceNoeud &m_allocatrice;\n";
        os << "\tkuri::pile<NoeudBloc *> m_blocs{};\n";
        os << "public:\n";

        const char *methodes = R"(
	explicit AssembleuseArbre(AllocatriceNoeud &allocatrice)
		: m_allocatrice(allocatrice)
	{}

    NoeudBloc *empile_bloc(Lexeme const *lexeme, NoeudDeclarationEnteteFonction *appartiens_à_fonction);

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

	void depile_tout()
	{
		m_blocs.efface();
	}

	void depile_bloc()
	{
		m_blocs.depile();
	}

	/* Utilisation d'un gabarit car à part pour les copies, nous connaissons
	 * toujours le genre de noeud à créer, et spécialiser cette fonction nous
	 * économise pas mal de temps d'exécution, au prix d'un exécutable plus gros. */
	template <GenreNoeud genre>
	NoeudExpression *crée_noeud(Lexeme const *lexeme)
	{
		auto noeud = m_allocatrice.crée_noeud<genre>();
		noeud->genre = genre;
		noeud->lexeme = lexeme;
		noeud->bloc_parent = bloc_courant();

        if (noeud->lexeme && (noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE)) {
			noeud->ident = lexeme->ident;
		}

		if (genre == GenreNoeud::DECLARATION_ENTETE_FONCTION) {
			auto entete = noeud->comme_entete_fonction();
			entete->corps->lexeme = lexeme;
			entete->corps->ident = lexeme->ident;
			entete->corps->bloc_parent = entete->bloc_parent;
		}

		if (genre == GenreNoeud::EXPRESSION_LITTERALE_CHAINE) {
			/* transfère l'index car les lexèmes peuvent être partagés lors de la simplification du code ou des exécutions */
			noeud->comme_litterale_chaine()->valeur = lexeme->index_chaine;
		}

		return noeud;
	}
)";

        os << methodes;

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\t" << it->nom() << " *crée_" << it->accede_nom_comme()
               << "(const Lexeme *lexeme);\n";
        }

        const char *decls_extras = R"(
	NoeudSi *crée_si(const Lexeme *lexeme, GenreNoeud genre_noeud);
	NoeudDeclarationVariable *crée_declaration_variable(NoeudExpressionReference *ref);
	NoeudAssignation *crée_assignation_variable(const Lexeme *lexeme, NoeudExpression *assignee, NoeudExpression *expression);
	NoeudAssignation *crée_incrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudAssignation *crée_decrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudBloc *crée_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent);
	NoeudDeclarationVariable *crée_declaration_variable(const Lexeme *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression);
	NoeudDeclarationVariable *crée_declaration_variable(NoeudExpressionReference *ref, NoeudExpression *expression);
	NoeudExpressionLitteraleEntier *crée_litterale_entier(const Lexeme *lexeme, Type *type, uint64_t valeur);
    NoeudExpressionLitteraleBool *crée_litterale_bool(const Lexeme *lexeme, Type *type, bool valeur);
	NoeudExpressionLitteraleReel *crée_litterale_reel(const Lexeme *lexeme, Type *type, double valeur);
	NoeudExpression *crée_reference_type(const Lexeme *lexeme, Type *type);
	NoeudExpressionAppel *crée_appel(const Lexeme *lexeme, NoeudExpression *appelee, Type *type);
	NoeudExpressionBinaire *crée_indexage(const Lexeme *lexeme, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification);
	NoeudExpressionBinaire *crée_expression_binaire(const Lexeme *lexeme, OpérateurBinaire const *op, NoeudExpression *expr1, NoeudExpression *expr2);
	NoeudExpressionMembre *crée_reference_membre(const Lexeme *lexeme, NoeudExpression *accede, Type *type, int index);
	NoeudExpressionReference *crée_reference_declaration(const Lexeme *lexeme, NoeudDeclaration *decl);
	NoeudExpressionAppel *crée_construction_structure(const Lexeme *lexeme, TypeCompose *type);
)";

        os << decls_extras;

        os << "};\n";
    }

    void genere_fichier_source_allocatrice(FluxSortieCPP &os)
    {
        os << "#include \"allocatrice.hh\"\n";
        os << "#include \"statistiques/statistiques.hh\"\n";
        os << "int64_t AllocatriceNoeud::nombre_noeuds() const\n";
        os << "{\n";
        os << "\tauto nombre = int64_t(0);\n";
        POUR (proteines_struct) {
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
        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\tstats_arbre.fusionne_entrée({" << '"' << it->nom() << '"' << ", "
               << "m_noeuds_" << it->accede_nom_comme() << ".taille(), "
               << "m_noeuds_" << it->accede_nom_comme() << ".memoire_utilisee()});\n";
        }

        // stats pour les tableaux
        auto noms_tableaux = kuri::ensemble<kuri::chaine>();

        auto crée_nom_tableau = [](kuri::chaine_statique nom_comme,
                                   kuri::chaine_statique nom_membre) -> kuri::chaine {
            return enchaine("taille_max_", nom_comme, "_", nom_membre);
        };

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }
            if (!it->possède_tableau()) {
                continue;
            }

            it->pour_chaque_membre_recursif([&](const Membre &membre) {
                if (membre.type->est_tableau() || membre.nom.nom_cpp() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom_cpp(),
                                                              membre.nom.nom_cpp());
                    os << "auto " << nom_tableau << " = 0;\n";
                    noms_tableaux.insère(nom_tableau);
                }
            });
        }

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }
            if (!it->possède_tableau()) {
                continue;
            }

            const auto nom_comme = it->accede_nom_comme();

            os << "auto memoire_" << nom_comme << " = int64_t(0);\n";
            os << "pour_chaque_element(m_noeuds_" << nom_comme << ", [&](";
            os << it->nom() << " const &noeud) {\n";
            it->pour_chaque_membre_recursif([&](const Membre &membre) {
                if (membre.type->est_tableau()) {
                    const auto nom_membre = membre.nom;
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom_cpp(),
                                                              nom_membre.nom_cpp());
                    os << nom_tableau << " = std::max(" << nom_tableau << ", noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille());\n";
                    os << "memoire_" << nom_comme << " += noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille_memoire();\n";
                }
                else if (membre.nom.nom_cpp() == "monomorphisations") {
                    const auto nom_tableau = crée_nom_tableau(it->accede_nom_comme().nom_cpp(),
                                                              membre.nom.nom_cpp());

                    os << "if (noeud.monomorphisations) {\n";
                    os << "memoire_" << nom_comme
                       << " += noeud.monomorphisations->memoire_utilisee();\n";
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

        os << "auto &stats_tableaux = stats.stats_tableaux;\n";
        noms_tableaux.pour_chaque_element([&](kuri::chaine_statique it) {
            os << "stats_tableaux.fusionne_entrée({\"" << it << "\", " << it << "});\n";
        });

        os << "}\n";
    }

    void genere_fichier_entete_allocatrice(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        os << "#include \"biblinternes/structures/tableau_page.hh\"\n";
        os << "#include \"compilation/monomorphisations.hh\"\n";
        os << "#include \"noeud_expression.hh\"\n";
        os << "struct Statistiques;\n";
        os << "struct AllocatriceNoeud {\n";
        os << "private:\n";

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\ttableau_page<" << it->nom() << "> m_noeuds_" << it->accede_nom_comme()
               << "{};\n";
        }

        os << "\ttableau_page<Monomorphisations> m_monomorphisations_fonctions{};\n";
        os << "\ttableau_page<Monomorphisations> m_monomorphisations_structs{};\n";

        os << "\n";
        os << "public:\n";

        os << "\ttemplate <GenreNoeud genre>\n";
        os << "\tNoeudExpression *crée_noeud()\n";
        os << "\t{\n";

        os << "\t\tswitch (genre) {\n";

        POUR (proteines_struct) {
            const auto nom_genre = it->accede_nom_genre();
            if (nom_genre.est_nul()) {
                continue;
            }

            os << "\t\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t\t{\n";

            // Entêtes et corps alloués ensembles
            if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION" ||
                nom_genre.nom_cpp() == "DECLARATION_OPERATEUR_POUR") {
                if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION") {
                    os << "\t\t\t\tauto entete = m_noeuds_entete_fonction.ajoute_element();\n";
                }
                else {
                    os << "\t\t\t\tauto entete = m_noeuds_operateur_pour.ajoute_element();\n";
                }
                os << "\t\t\t\tauto corps  = m_noeuds_corps_fonction.ajoute_element();\n";
                os << "\t\t\t\tentete->corps = corps;\n";
                os << "\t\t\t\tcorps->entete = entete;\n";
                os << "\t\t\t\treturn entete;\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_CORPS_FONCTION") {
                os << "\t\t\t\treturn nullptr;\n";
            }
            else {
                os << "\t\t\t\treturn m_noeuds_" << it->accede_nom_comme()
                   << ".ajoute_element();\n";
            }

            os << "\t\t\t}\n";
        }

        os << "\t\t}\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";

        const char *crée_monomorphisations = R"(
    Monomorphisations *crée_monomorphisations_fonction()
	{
		return m_monomorphisations_fonctions.ajoute_element();
	}

    Monomorphisations *crée_monomorphisations_struct()
	{
		return m_monomorphisations_structs.ajoute_element();
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
                                        kuri::tableau<Proteine *> const &proteines)
    {
        // Les structures Kuri pour NoeudCode
        POUR (proteines) {
            if (it->comme_struct()) {
                auto prot = it->comme_struct();

                if (prot->paire()) {
                    prot->paire()->genere_code_kuri(os);
                    continue;
                }
            }

            it->genere_code_kuri(os);
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

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, imprime_erreur};

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

    auto table_desc = kuri::table_hachage<kuri::chaine_statique, ProteineStruct *>(
        "Protéines structures");

    POUR (syntaxeuse.proteines) {
        if (!it->comme_struct()) {
            continue;
        }

        table_desc.insère(it->nom().nom_kuri(), it->comme_struct());
    }

    GeneratriceCodeCPP generatrice;
    generatrice.proteines = syntaxeuse.proteines;
    generatrice.table_desc = table_desc;

    POUR (generatrice.proteines) {
        if (it->comme_struct()) {
            generatrice.proteines_struct.ajoute(it->comme_struct());
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
            if (!remplace_si_different(nom_fichier_tmp, argv[1])) {
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
            generatrice_kuri.genere_fichier_kuri_noeud_code(flux, syntaxeuse.proteines);
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
    else {
        std::cerr << "Chemin de fichier " << argv[1] << " inconnu !\n";
        return 1;
    }

    if (!remplace_si_different(nom_fichier_tmp, argv[1])) {
        return 1;
    }

    return 0;
}
