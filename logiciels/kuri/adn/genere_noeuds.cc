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

#include <filesystem>
#include <fstream>

#include "biblinternes/chrono/outils.hh"
#include "biblinternes/outils/badge.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/ensemble.hh"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "structures/table_hachage.hh"

#include "adn.hh"
#include "outils.hh"

static const char *copie_extra_declaration_variable = R"(
			/* n'oublions pas de mettre en place les déclarations */
			if (copie->valeur->est_reference_declaration()) {
			   copie->valeur->comme_reference_declaration()->declaration_referee = copie;
			}
			else if (copie->valeur->est_virgule()) {
			   auto virgule = orig->valeur->comme_virgule();
			   auto nvirgule = copie->valeur->comme_virgule();

			   auto index = 0;
			   POUR (nvirgule->expressions) {
				   auto it_orig = virgule->expressions[index]->comme_reference_declaration();
                   if (it_orig->declaration_referee) {
				       it->comme_reference_declaration()->declaration_referee = copie_noeud(assem, it_orig->declaration_referee, bloc_parent)->comme_declaration_variable();
                   }
                   index += 1;
			   }
			})";

static const char *copie_extra_entete_fonction = R"(
			copie->bloc_constantes->possede_contexte = orig->bloc_constantes->possede_contexte;
			copie->bloc_parametres->possede_contexte = orig->bloc_parametres->possede_contexte;
			if (!copie->est_declaration_type) {
				if (orig->params_sorties.taille() > 1) {
					copie->param_sortie = copie_noeud(assem, orig->param_sortie, bloc_parent)->comme_declaration_variable();
				}
				else {
					copie->param_sortie = copie->params_sorties[0]->comme_declaration_variable();
				}
			}
			/* copie le corps du noeud directement */
			{
				auto expr_corps = orig->corps;
				auto nexpr_corps = copie->corps;
				nexpr_corps->bloc_parent = bloc_parent;
				nexpr_corps->drapeaux = (expr_corps->drapeaux & ~DECLARATION_FUT_VALIDEE);
				nexpr_corps->est_corps_texte = expr_corps->est_corps_texte;
				nexpr_corps->arbre_aplatis.reserve(expr_corps->arbre_aplatis.taille());
				nexpr_corps->bloc = static_cast<NoeudBloc *>(copie_noeud(assem, expr_corps->bloc, bloc_parent));
			})";

static const char *copie_extra_bloc = R"(
			copie->membres->reserve(orig->membres->taille());
			POUR (*copie->expressions.verrou_lecture()) {
				if (it->est_declaration()) {
					copie->membres->ajoute(it->comme_declaration());
				}
			})";

struct GeneratriceCodeCPP {
    kuri::tableau<Proteine *> proteines{};
    kuri::tableau<ProteineStruct *> proteines_struct{};
    kuri::table_hachage<kuri::chaine_statique, ProteineStruct *> table_desc{};

    void genere_fichier_entete_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        os << "#include \"biblinternes/outils/assert.hh\"\n";
        os << "#include \"biblinternes/moultfilage/synchrone.hh\"\n";
        os << "#include \"structures/chaine.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"structures/tableau.hh\"\n";
        os << "#include \"structures/tableau_compresse.hh\"\n";
        os << "#include \"compilation/transformation_type.hh\"\n";
        os << "#include \"parsage/lexemes.hh\"\n";
        os << "#include \"expression.hh\"\n";
        os << "#include \"utilitaires.hh\"\n";
        os << "template <typename T> struct Monomorphisations;\n";
        os << "template <typename T> using tableau_synchrone = "
              "dls::outils::Synchrone<kuri::tableau<T, int>>;\n";

        // Prodéclarations des structures
        dls::ensemble<kuri::chaine> noms_struct;

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

                    if (nom_type == "Monomorphisations") {
                        return;
                    }

                    noms_struct.insere(nom_type);
                });

                noms_struct.insere(it->nom().nom_cpp());
            }
        }

        POUR (noms_struct) {
            os << "struct " << it << ";\n";
        }
        os << "\n";

        // Les structures C++
        POUR (proteines) {
            it->genere_code_cpp(os, true);
        }

        POUR (proteines_struct) {
            it->genere_code_cpp_apres_declaration(os);
        }

        // Impression de l'arbre
        os << "void imprime_arbre_substitue(const NoeudExpression *racine, std::ostream &os, int "
              "profondeur);\n\n";
        os << "void imprime_arbre(NoeudExpression const *racine, std::ostream &os, int "
              "profondeur, bool substitution = false);\n\n";

        // Copie de l'arbre
        os << "NoeudExpression *copie_noeud(AssembleuseArbre *assem, NoeudExpression const "
              "*racine, NoeudBloc *bloc_parent);\n\n";

        // Calcul de l'étendue
        os << "struct Etendue {\n";
        os << "\tlong pos_min = 0;\n";
        os << "\tlong pos_max = 0;\n";
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
              "std::function<DecisionVisiteNoeud(NoeudExpression const *)> const &rappel);\n\n";
    }

    void genere_fichier_source_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "#include \"noeud_expression.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"parsage/outils_lexemes.hh\"\n";
        os << "#include \"assembleuse.hh\"\n";
        os << "#include <iostream>\n";

        POUR (proteines) {
            it->genere_code_cpp(os, false);
        }

        genere_impression_arbre_syntaxique(os);
        genere_calcul_etendue_noeud(os);
        genere_visite_noeud(os);
        genere_copie_noeud(os);
    }

    void genere_impression_arbre_syntaxique(FluxSortieCPP &os)
    {
        os << "void imprime_tab(std::ostream &os, int n)\n";
        os << "{\n";
        os << "\tfor (int i = 0; i < n; ++i) { os << ' ' << ' '; }\n";
        os << "}\n\n";

        os << "void imprime_arbre_substitue(const NoeudExpression *racine, std::ostream &os, int "
              "profondeur)\n";
        os << "{\n";
        os << "\timprime_arbre(racine, os, profondeur, true);\n";
        os << "}\n\n";

        os << "void imprime_arbre(NoeudExpression const *racine, std::ostream &os, int "
              "profondeur, bool substitution)\n";
        os << "{\n";
        os << "\tif (!racine) {\n";
        os << "\t\treturn;\n";
        os << "\t}\n";
        os << "\tif (substitution && racine->substitution) {\n";
        os << "\t\timprime_arbre(racine->substitution, os, profondeur, substitution);\n";
        os << "\t}\n";
        os << "\tswitch (racine->genre) {\n";

        POUR (proteines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            os << "\t\tcase GenreNoeud::" << it->accede_nom_genre() << ":\n";
            os << "\t\t{\n";

            os << "\t\t\timprime_tab(os, profondeur);\n";
            os << "\t\t\tos << \"<" << it->accede_nom_comme();

            // À FAIRE : ceci ne prend pas en compte les ancêtres
            if (it->possede_enfants()) {
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

            if (it->possede_enfants()) {
                os << "\t\t\timprime_tab(os, profondeur);\n";
                os << "\t\t\tos << \"</" << it->accede_nom_comme() << ">\\n\";\n";
            }

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "}\n\n";
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
                        os << "\t\t\t\tvisite_noeud(it, preference, rappel);\n";
                        os << "\t\t\t}\n";
                    }
                    else {
                        os << "\t\t\tvisite_noeud(racine_typee->" << membre.nom
                           << ", preference, rappel);\n";
                    }
                });

            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "}\n";
    }

    void genere_copie_noeud(FluxSortieCPP &os)
    {
        os << "NoeudExpression *copie_noeud(AssembleuseArbre *assem, const NoeudExpression "
              "*racine, NoeudBloc *bloc_parent)\n";
        os << "{\n";
        os << "\tif(!racine) {\n";
        os << "\t\treturn nullptr;\n";
        os << "\t}\n";
        os << "\tNoeudExpression *nracine = nullptr;\n";
        os << "\tswitch(racine->genre) {\n";

        POUR (proteines_struct) {
            if (it->accede_nom_genre().est_nul()) {
                continue;
            }

            const auto nom_genre = it->accede_nom_genre();
            const auto nom_comme = it->accede_nom_comme();
            os << "\t\tcase GenreNoeud::" << nom_genre << ":\n";
            os << "\t\t{\n";

            if (nom_genre.nom_cpp() == "DECLARATION_CORPS_FONCTION") {
                os << "\t\t\tassert_rappel(false, [&]() { std::cerr << \"Tentative de copie d'un "
                      "corps de fonction seul\\n\"; });\n";
                os << "\t\t\tbreak;\n";
                os << "\t\t}\n";
                continue;
            }

            os << "\t\t\tnracine = assem->cree_noeud<GenreNoeud::" << nom_genre
               << ">(racine->lexeme);\n";
            os << "\t\t\tnracine->ident = racine->ident;\n";
            os << "\t\t\tnracine->type = racine->type;\n";
            os << "\t\t\tnracine->bloc_parent = bloc_parent;\n";
            os << "\t\t\tnracine->drapeaux = (racine->drapeaux & ~DECLARATION_FUT_VALIDEE);\n";

            if (!it->possede_enfants() && !it->possede_membre_a_copier()) {
                os << "\t\t\tbreak;\n";
                os << "\t\t}\n";
                continue;
            }

            os << "\t\t\tconst auto orig = racine->comme_" << nom_comme << "();\n";
            os << "\t\t\tconst auto copie = nracine->comme_" << nom_comme << "();\n";

            // Pour les structure et les fonctions, il nous faut proprement gérer les blocs parents

            if (nom_genre.nom_cpp() == "DECLARATION_STRUCTURE") {
                os << "\t\t\tif (orig->bloc_constantes) {\n";
                os << "\t\t\t\tcopie->bloc_constantes = copie_noeud(assem, orig->bloc_constantes, "
                      "bloc_parent)->comme_bloc();\n";
                os << "\t\t\t\tbloc_parent = copie->bloc_constantes;\n";
                os << "\t\t\t}\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION") {
                os << "\t\t\tcopie->bloc_constantes = assem->cree_bloc_seul(nullptr, "
                      "bloc_parent);\n";
                os << "\t\t\tcopie->bloc_parametres = assem->cree_bloc_seul(nullptr, "
                      "copie->bloc_constantes);\n";
                os << "\t\t\tbloc_parent = copie->bloc_parametres;\n";
            }
            else if (nom_genre.nom_cpp() == "INSTRUCTION_COMPOSEE") {
                os << "\t\t\tbloc_parent = const_cast<NoeudBloc *>(copie);\n";
            }

            it->pour_chaque_enfant_recursif([&](const Membre &enfant) {
                const auto nom_enfant = enfant.nom;

                // Les corps des fonctions sont copiées en même temps, et non à travers un appel
                // séparé
                if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION" &&
                    nom_enfant.nom_cpp() == "corps") {
                    return;
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

                    os << "copie_noeud(assem, it, bloc_parent)";

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

                    os << "copie_noeud(assem, orig->" << nom_enfant << ", bloc_parent)";

                    if (enfant.type->accede_nom().nom_cpp() != "NoeudExpression") {
                        os << ")";
                    }

                    os << ";\n";
                }
            });

            it->pour_chaque_copie_extra_recursif([&](const Membre &enfant) {
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

            if (nom_genre.nom_cpp() == "DECLARATION_VARIABLE") {
                os << copie_extra_declaration_variable << "\n";
            }
            else if (dls::outils::est_element(nom_genre.nom_cpp(),
                                              "INSTRUCTION_POUR",
                                              "INSTRUCTION_BOUCLE",
                                              "INSTRUCTION_REPETE",
                                              "INSTRUCTION_TANTQUE")) {
                os << "\t\t\tcopie->bloc->appartiens_a_boucle = copie;\n";
            }
            else if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION") {
                os << copie_extra_entete_fonction << "\n";
            }
            else if (nom_genre.nom_cpp() == "INSTRUCTION_COMPOSEE") {
                os << copie_extra_bloc << "\n";
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
        os << "#include \"biblinternes/structures/tableau_page.hh\"\n";
        os << "#include \"structures/chaine_statique.hh\"\n";
        os << "#include \"structures/tableau.hh\"\n";
        os << "#include \"infos_types.hh\"\n";
        os << "struct EspaceDeTravail;\n";
        os << "struct InfoType;\n";
        os << "struct Statistiques;\n";

        // Prodéclarations des structures
        dls::ensemble<kuri::chaine> noms_struct;

        POUR (proteines_struct) {
            const auto nom_code = it->accede_nom_code();
            if (!nom_code.est_nul()) {
                noms_struct.insere(nom_code.nom_cpp());
            }
        }

        POUR (noms_struct) {
            os << "struct " << it << ";\n";
        }
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

            // À FAIRE : spécialise le nom du genre, voir aussi adn.cc
            if (!it->accede_nom_genre().est_nul()) {
                os << "\t" << it->accede_nom_code()
                   << "() { genre = GenreNoeud::" << it->accede_nom_genre() << "; }\n";
                os << "\tCOPIE_CONSTRUCT(" << it->accede_nom_code() << ");\n";
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

                const auto type_membre = supprime_accents(membre.type->accede_nom().nom_cpp());

                if (type_membre == "NoeudExpression" || type_membre == "NoeudDeclaration") {
                    os << "NoeudCode";
                }
                else if (type_membre == "NoeudBloc") {
                    os << "NoeudCodeBloc";
                }
                else if (type_membre == "NoeudPaireDiscr") {
                    os << "NoeudCodePaireDiscr";
                }
                else if (type_membre == "NoeudDeclarationVariable") {
                    os << "NoeudCodeDeclarationVariable";
                }
                else if (type_membre == "NoeudDeclarationCorpsFonction") {
                    os << "NoeudCodeCorpsFonction";
                }
                else if (type_membre == "NoeudDeclarationEnteteFonction") {
                    os << "NoeudCodeEnteteFonction";
                }
                else if (type_membre == "chaine_statique") {
                    os << "kuri::chaine_statique";
                }
                else {
                    os << type_membre;
                }

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
               << R"((), [this]() { std::cerr << "Le genre de noeud est " << this->genre << "\n"; }))"
               << ";\n";
            os << "\treturn static_cast<" << nom_noeud << " *>(this);\n";
            os << "}\n\n";

            os << "inline const " << nom_noeud << " *NoeudCode::comme_" << nom_comme
               << "() const\n";
            os << "{\n";
            os << "\tassert_rappel(est_" << nom_comme
               << R"((), [this]() { std::cerr << "Le genre de noeud est " << this->genre << "\n"; }))"
               << ";\n";
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
        os << "\tInfoType *cree_info_type_pour(Type *type);\n\n";
        os << "\tType *convertis_info_type(Typeuse &typeuse, InfoType *type);\n\n";
        os << "\tvoid rassemble_statistiques(Statistiques &stats) const;\n\n";
        os << "\tlong memoire_utilisee() const;\n";

        os << "};\n\n";
    }

    void genere_fichier_source_noeud_code(FluxSortieCPP &os)
    {
        os << "#include \"noeud_code.hh\"\n";
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
            // corps de fonctions qui se référéncent mutuellement.
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
            os << "\t\t\tn->type = cree_info_type_pour(racine_typee->type);\n";
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
        os << "\t\tconst auto fichier = espace->fichier(lexeme->fichier);\n";
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
            os << "\t\t\tn->type = convertis_info_type(espace->typeuse, racine_typee->type);\n";

            os << "\t\t\tnoeud = n;\n";
            os << "\t\t\tbreak;\n";
            os << "\t\t}\n";
        }

        os << "\t}\n";
        os << "\treturn noeud;\n";
        os << "}\n\n";

        os << "long ConvertisseuseNoeudCode::memoire_utilisee() const\n";
        os << "{\n";

        os << "\tauto mem = 0l;\n";

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
    }

    void genere_code_pour_enfant(FluxSortieCPP &os,
                                 ProteineStruct *racine,
                                 bool inclus_code,
                                 std::function<void(ProteineStruct &, const Membre &)> rappel)
    {
        auto possede_enfants = false;
        auto noeud_courant = racine;

        while (noeud_courant) {
            if (noeud_courant->possede_enfants()) {
                possede_enfants = true;
                break;
            }

            if (inclus_code && !possede_enfants) {
                for (const auto &membre : noeud_courant->membres()) {
                    if (membre.est_code) {
                        possede_enfants = true;
                        break;
                    }
                }
            }

            noeud_courant = noeud_courant->mere();
        }

        if (!possede_enfants) {
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
NoeudBloc *AssembleuseArbre::empile_bloc(Lexeme const *lexeme)
{
	auto bloc = static_cast<NoeudBloc *>(cree_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme));
	bloc->bloc_parent = bloc_courant();
	if (bloc->bloc_parent) {
		bloc->possede_contexte = bloc->bloc_parent->possede_contexte;
	}
	else {
		/* vrai si le bloc ne possède pas de parent (bloc de module) */
		bloc->possede_contexte = true;
	}
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

            os << it->nom() << " *AssembleuseArbre::cree_" << it->accede_nom_comme()
               << "(const Lexeme *lexeme)\n";
            os << "{\n";
            os << "\treturn cree_noeud<GenreNoeud::" << nom_genre << ">(lexeme)->comme_"
               << it->accede_nom_comme() << "();\n";
            os << "}\n";
        }
    }

    void genere_fichier_entete_assembleuse(FluxSortieCPP &os)
    {
        os << "#pragma once\n";
        os << "#include \"allocatrice.hh\"\n";
        os << "#include \"biblinternes/structures/pile.hh\"\n";
        os << "struct TypeCompose;\n";
        os << "struct AssembleuseArbre {\n";
        os << "private:\n";
        os << "\tAllocatriceNoeud &m_allocatrice;\n";
        os << "\tdls::pile<NoeudBloc *> m_blocs{};\n";
        os << "public:\n";

        const char *methodes = R"(
	explicit AssembleuseArbre(AllocatriceNoeud &allocatrice)
		: m_allocatrice(allocatrice)
	{}

	NoeudBloc *empile_bloc(Lexeme const *lexeme);

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
	NoeudExpression *cree_noeud(Lexeme const *lexeme)
	{
		auto noeud = m_allocatrice.cree_noeud<genre>();
		noeud->genre = genre;
		noeud->lexeme = lexeme;
		noeud->bloc_parent = bloc_courant();

		if (noeud->lexeme && (noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE || noeud->lexeme->genre == GenreLexeme::EXTERNE)) {
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

            os << "\t" << it->nom() << " *cree_" << it->accede_nom_comme()
               << "(const Lexeme *lexeme);\n";
        }

        const char *decls_extras = R"(
	NoeudSi *cree_si(const Lexeme *lexeme, GenreNoeud genre_noeud);
	NoeudDeclarationVariable *cree_declaration_variable(NoeudExpressionReference *ref);
	NoeudAssignation *cree_assignation_variable(const Lexeme *lexeme, NoeudExpression *assignee, NoeudExpression *expression);
	NoeudAssignation *cree_incrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudAssignation *cree_decrementation(const Lexeme *lexeme, NoeudExpression *valeur);
	NoeudBloc *cree_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent);
	NoeudDeclarationVariable *cree_declaration_variable(const Lexeme *lexeme, Type *type, IdentifiantCode *ident, NoeudExpression *expression);
	NoeudDeclarationVariable *cree_declaration_variable(NoeudExpressionReference *ref, NoeudExpression *expression);
	NoeudExpressionLitteraleEntier *cree_litterale_entier(const Lexeme *lexeme, Type *type, unsigned long valeur);
	NoeudExpressionLitteraleReel *cree_litterale_reel(const Lexeme *lexeme, Type *type, double valeur);
	NoeudExpression *cree_reference_type(const Lexeme *lexeme, Type *type);
	NoeudExpressionAppel *cree_appel(const Lexeme *lexeme, NoeudExpression *appelee, Type *type);
	NoeudExpressionBinaire *cree_indexage(const Lexeme *lexeme, NoeudExpression *expr1, NoeudExpression *expr2, bool ignore_verification);
	NoeudExpressionBinaire *cree_expression_binaire(const Lexeme *lexeme, OperateurBinaire const *op, NoeudExpression *expr1, NoeudExpression *expr2);
	NoeudExpressionMembre *cree_reference_membre(const Lexeme *lexeme, NoeudExpression *accede, Type *type, int index);
	NoeudExpressionReference *cree_reference_declaration(const Lexeme *lexeme, NoeudDeclaration *decl);
	NoeudExpressionAppel *cree_construction_structure(const Lexeme *lexeme, TypeCompose *type);
)";

        os << decls_extras;

        os << "};\n";
    }

    void genere_fichier_source_allocatrice(FluxSortieCPP &os)
    {
        os << "#include \"allocatrice.hh\"\n";
        os << "#include \"statistiques/statistiques.hh\"\n";
        os << "long AllocatriceNoeud::nombre_noeuds() const\n";
        os << "{\n";
        os << "\tauto nombre = 0l;\n";
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

            os << "\tstats_arbre.fusionne_entree({" << '"' << it->nom() << '"' << ", "
               << "m_noeuds_" << it->accede_nom_comme() << ".taille(), "
               << "m_noeuds_" << it->accede_nom_comme() << ".memoire_utilisee()});\n";
        }

        // stats pour les tableaux
        auto noms_tableaux = dls::ensemble<kuri::chaine>();

        POUR (proteines_struct) {
            if (!it->possede_tableau()) {
                continue;
            }

            it->pour_chaque_membre_recursif([&it, &os, &noms_tableaux](const Membre &membre) {
                if (membre.type->est_tableau()) {
                    const auto nom_tableau = enchaine("taille_max_",
                                                      it->accede_nom_comme().nom_cpp(),
                                                      "_",
                                                      membre.nom.nom_cpp());
                    os << "auto " << nom_tableau << " = 0;\n";
                    noms_tableaux.insere(nom_tableau);
                }
            });
        }

        POUR (proteines_struct) {
            if (!it->possede_tableau()) {
                continue;
            }

            const auto nom_comme = it->accede_nom_comme();

            os << "auto memoire_" << nom_comme << " = 0l;\n";
            os << "pour_chaque_element(m_noeuds_" << nom_comme << ", [&](";
            os << it->nom() << " const &noeud) {\n";
            it->pour_chaque_membre_recursif([&](const Membre &membre) {
                if (membre.type->est_tableau()) {
                    const auto nom_membre = membre.nom;
                    const auto nom_tableau = enchaine("taille_max_",
                                                      it->accede_nom_comme().nom_cpp(),
                                                      "_",
                                                      nom_membre.nom_cpp());
                    os << nom_tableau << " = std::max(" << nom_tableau << ", noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille());\n";
                    os << "memoire_" << nom_comme << " += noeud." << nom_membre;
                    os << membre.type->accesseur() << "taille_memoire();\n";
                }
            });
            os << "});\n";

            os << "\tstats_arbre.fusionne_entree({" << '"' << it->nom() << '"' << ", "
               << "0, "
               << "memoire_" << nom_comme << "});\n";
        }

        os << "auto &stats_tableaux = stats.stats_tableaux;\n";
        POUR (noms_tableaux) {
            os << "stats_tableaux.fusionne_entree({\"" << it << "\", " << it << "});\n";
        }

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

        os << "\ttableau_page<Monomorphisations<NoeudDeclarationEnteteFonction>> "
              "m_monomorphisations_fonctions{};\n";
        os << "\ttableau_page<Monomorphisations<NoeudStruct>> m_monomorphisations_structs{};\n";

        os << "\n";
        os << "public:\n";

        os << "\ttemplate <GenreNoeud genre>\n";
        os << "\tNoeudExpression *cree_noeud()\n";
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
            if (nom_genre.nom_cpp() == "DECLARATION_ENTETE_FONCTION") {
                os << "\t\t\t\tauto entete = m_noeuds_entete_fonction.ajoute_element();\n";
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
        os << "\t}\n";

        const char *cree_monomorphisations = R"(
	Monomorphisations<NoeudDeclarationEnteteFonction> *cree_monomorphisations_fonction()
	{
		return m_monomorphisations_fonctions.ajoute_element();
	}

	Monomorphisations<NoeudStruct> *cree_monomorphisations_struct()
	{
		return m_monomorphisations_structs.ajoute_element();
	}
)";

        os << cree_monomorphisations;

        os << "\n";
        os << "\tlong nombre_noeuds() const;\n\n";

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
    auto nom_fichier_sortie = std::filesystem::path(argv[1]);

    auto texte = charge_contenu_fichier(chemin_adn);
    auto donnees_fichier = DonneesConstantesFichier();
    donnees_fichier.tampon = lng::tampon_source(texte.c_str());

    auto fichier = Fichier();
    fichier.donnees_constantes = &donnees_fichier;
    fichier.donnees_constantes->chemin = chemin_adn;

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto rappel_erreur = [](kuri::chaine message) { std::cerr << message << '\n'; };

    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, rappel_erreur};

    auto lexeuse = Lexeuse(contexte_lexage, &donnees_fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possede_erreur()) {
        return 1;
    }

    auto syntaxeuse = SyntaxeuseADN(&fichier);
    syntaxeuse.analyse();

    if (syntaxeuse.possede_erreur()) {
        return 1;
    }

    auto table_desc = kuri::table_hachage<kuri::chaine_statique, ProteineStruct *>();

    POUR (syntaxeuse.proteines) {
        if (!it->comme_struct()) {
            continue;
        }

        table_desc.insere(it->nom().nom_kuri(), it->comme_struct());
    }

    GeneratriceCodeCPP generatrice;
    generatrice.proteines = syntaxeuse.proteines;
    generatrice.table_desc = table_desc;

    POUR (generatrice.proteines) {
        if (it->comme_struct()) {
            generatrice.proteines_struct.ajoute(it->comme_struct());
        }
    }

    if (nom_fichier_sortie.filename() == "noeud_expression.cc") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_source_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.filename() == "noeud_expression.hh") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_arbre_syntaxique(flux);
    }
    else if (nom_fichier_sortie.filename() == "noeud_code.cc") {
        {
            std::ofstream fichier_sortie(argv[1]);
            auto flux = FluxSortieCPP(fichier_sortie);
            generatrice.genere_fichier_source_noeud_code(flux);
        }
        {
            // Génère le fichier de message pour le module Compilatrice
            // Apparemment, ce n'est pas possible de le faire via CMake
            nom_fichier_sortie.replace_filename("../modules/Compilatrice/code.kuri");
            std::ofstream fichier_sortie(nom_fichier_sortie);
            auto flux = FluxSortieKuri(fichier_sortie);
            GeneratriceCodeKuri generatrice_kuri;
            generatrice_kuri.genere_fichier_kuri_noeud_code(flux, syntaxeuse.proteines);
        }
    }
    else if (nom_fichier_sortie.filename() == "noeud_code.hh") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_noeud_code(flux);
    }
    else if (nom_fichier_sortie.filename() == "assembleuse.cc") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_source_assembleuse(flux);
    }
    else if (nom_fichier_sortie.filename() == "assembleuse.hh") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_assembleuse(flux);
    }
    else if (nom_fichier_sortie.filename() == "allocatrice.cc") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_source_allocatrice(flux);
    }
    else if (nom_fichier_sortie.filename() == "allocatrice.hh") {
        std::ofstream fichier_sortie(argv[1]);
        auto flux = FluxSortieCPP(fichier_sortie);
        generatrice.genere_fichier_entete_allocatrice(flux);
    }
    else {
        std::cerr << "Chemin de fichier " << argv[1] << " inconnu !\n";
        return 1;
    }

    return 0;
}
