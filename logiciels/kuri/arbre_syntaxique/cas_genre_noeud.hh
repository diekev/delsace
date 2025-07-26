/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#define CAS_POUR_NOEUDS_HORS_TYPES                                                                \
    case GenreNoeud::COMMENTAIRE:                                                                 \
    case GenreNoeud::DÉCLARATION_BIBLIOTHÈQUE:                                                    \
    case GenreNoeud::DÉCLARATION_CONSTANTE:                                                       \
    case GenreNoeud::DÉCLARATION_CORPS_FONCTION:                                                  \
    case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:                                                 \
    case GenreNoeud::DÉCLARATION_MODULE:                                                          \
    case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:                                                  \
    case GenreNoeud::DÉCLARATION_VARIABLE:                                                        \
    case GenreNoeud::DÉCLARATION_VARIABLE_MULTIPLE:                                               \
    case GenreNoeud::DIRECTIVE_AJOUTE_FINI:                                                       \
    case GenreNoeud::DIRECTIVE_AJOUTE_INIT:                                                       \
    case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:                                                      \
    case GenreNoeud::DIRECTIVE_CUISINE:                                                           \
    case GenreNoeud::DIRECTIVE_DÉPENDANCE_BIBLIOTHÈQUE:                                           \
    case GenreNoeud::DIRECTIVE_EXÉCUTE:                                                           \
    case GenreNoeud::DIRECTIVE_FONCTION:                                                          \
    case GenreNoeud::DIRECTIVE_INTROSPECTION:                                                     \
    case GenreNoeud::DIRECTIVE_INSÈRE:                                                            \
    case GenreNoeud::DIRECTIVE_PRÉ_EXÉCUTABLE:                                                    \
    case GenreNoeud::EXPANSION_VARIADIQUE:                                                        \
    case GenreNoeud::EXPRESSION_APPEL:                                                            \
    case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:                                             \
    case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:                                             \
    case GenreNoeud::EXPRESSION_ASSIGNATION_LOGIQUE:                                              \
    case GenreNoeud::EXPRESSION_COMME:                                                            \
    case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:                                           \
    case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:                                             \
    case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU_TYPÉ:                                        \
    case GenreNoeud::EXPRESSION_INDEXAGE:                                                         \
    case GenreNoeud::EXPRESSION_INFO_DE:                                                          \
    case GenreNoeud::EXPRESSION_INIT_DE:                                                          \
    case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:                                                \
    case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:                                              \
    case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:                                                 \
    case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:                                          \
    case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:                                            \
    case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:                                                    \
    case GenreNoeud::EXPRESSION_LOGIQUE:                                                          \
    case GenreNoeud::EXPRESSION_MÉMOIRE:                                                          \
    case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:                                                 \
    case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:                                             \
    case GenreNoeud::EXPRESSION_PARENTHÈSE:                                                       \
    case GenreNoeud::EXPRESSION_PLAGE:                                                            \
    case GenreNoeud::EXPRESSION_PRISE_ADRESSE:                                                    \
    case GenreNoeud::EXPRESSION_PRISE_RÉFÉRENCE:                                                  \
    case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:                                            \
    case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:                                                 \
    case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE_UNION:                                           \
    case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:                                                   \
    case GenreNoeud::EXPRESSION_SÉLECTION:                                                        \
    case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:                                         \
    case GenreNoeud::EXPRESSION_TAILLE_DE:                                                        \
    case GenreNoeud::EXPRESSION_TYPE_DE:                                                          \
    case GenreNoeud::EXPRESSION_TYPE_FONCTION:                                                    \
    case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:                                           \
    case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:                                                \
    case GenreNoeud::EXPRESSION_TYPE_TRANCHE:                                                     \
    case GenreNoeud::EXPRESSION_VIRGULE:                                                          \
    case GenreNoeud::INSTRUCTION_ARRÊTE:                                                          \
    case GenreNoeud::INSTRUCTION_BOUCLE:                                                          \
    case GenreNoeud::INSTRUCTION_CHARGE:                                                          \
    case GenreNoeud::INSTRUCTION_COMPOSÉE:                                                        \
    case GenreNoeud::INSTRUCTION_CONTINUE:                                                        \
    case GenreNoeud::INSTRUCTION_DIFFÈRE:                                                         \
    case GenreNoeud::INSTRUCTION_DISCR:                                                           \
    case GenreNoeud::INSTRUCTION_DISCR_ÉNUM:                                                      \
    case GenreNoeud::INSTRUCTION_DISCR_UNION:                                                     \
    case GenreNoeud::INSTRUCTION_EMPL:                                                            \
    case GenreNoeud::INSTRUCTION_IMPORTE:                                                         \
    case GenreNoeud::INSTRUCTION_NON_INITIALISATION:                                              \
    case GenreNoeud::INSTRUCTION_POUR:                                                            \
    case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:                                                 \
    case GenreNoeud::INSTRUCTION_RÉPÈTE:                                                          \
    case GenreNoeud::INSTRUCTION_REPRENDS:                                                        \
    case GenreNoeud::INSTRUCTION_RETOUR:                                                          \
    case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:                                                 \
    case GenreNoeud::INSTRUCTION_SAUFSI:                                                          \
    case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:                                                 \
    case GenreNoeud::INSTRUCTION_SI:                                                              \
    case GenreNoeud::INSTRUCTION_SI_STATIQUE:                                                     \
    case GenreNoeud::INSTRUCTION_TANTQUE:                                                         \
    case GenreNoeud::INSTRUCTION_TENTE:                                                           \
    case GenreNoeud::OPÉRATEUR_BINAIRE:                                                           \
    case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:                                               \
    case GenreNoeud::OPÉRATEUR_UNAIRE

#define CAS_POUR_NOEUDS_TYPES_PERSONALISABLES                                                     \
    case GenreNoeud::DÉCLARATION_ÉNUM:                                                            \
    case GenreNoeud::DÉCLARATION_OPAQUE:                                                          \
    case GenreNoeud::DÉCLARATION_STRUCTURE:                                                       \
    case GenreNoeud::DÉCLARATION_UNION:                                                           \
    case GenreNoeud::ENUM_DRAPEAU:                                                                \
    case GenreNoeud::ERREUR

#define CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX                                                        \
    case GenreNoeud::BOOL:                                                                        \
    case GenreNoeud::CHAINE:                                                                      \
    case GenreNoeud::EINI:                                                                        \
    case GenreNoeud::ENTIER_CONSTANT:                                                             \
    case GenreNoeud::ENTIER_NATUREL:                                                              \
    case GenreNoeud::ENTIER_RELATIF:                                                              \
    case GenreNoeud::FONCTION:                                                                    \
    case GenreNoeud::OCTET:                                                                       \
    case GenreNoeud::POINTEUR:                                                                    \
    case GenreNoeud::POLYMORPHIQUE:                                                               \
    case GenreNoeud::RÉEL:                                                                        \
    case GenreNoeud::RÉFÉRENCE:                                                                   \
    case GenreNoeud::RIEN:                                                                        \
    case GenreNoeud::TABLEAU_DYNAMIQUE:                                                           \
    case GenreNoeud::TABLEAU_FIXE:                                                                \
    case GenreNoeud::TUPLE:                                                                       \
    case GenreNoeud::TYPE_ADRESSE_FONCTION:                                                       \
    case GenreNoeud::TYPE_DE_DONNÉES:                                                             \
    case GenreNoeud::TYPE_TRANCHE:                                                                \
    case GenreNoeud::VARIADIQUE

#define CAS_POUR_NOEUDS_TYPES                                                                     \
    CAS_POUR_NOEUDS_TYPES_PERSONALISABLES:                                                        \
    CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX
