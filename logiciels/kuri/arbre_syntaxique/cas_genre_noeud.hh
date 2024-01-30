/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#define CAS_POUR_NOEUDS_HORS_TYPES                                                                \
    case GenreNoeud::DECLARATION_BIBLIOTHEQUE:                                                    \
    case GenreNoeud::DECLARATION_CONSTANTE:                                                       \
    case GenreNoeud::DECLARATION_CORPS_FONCTION:                                                  \
    case GenreNoeud::DECLARATION_ENTETE_FONCTION:                                                 \
    case GenreNoeud::DECLARATION_MODULE:                                                          \
    case GenreNoeud::DECLARATION_OPERATEUR_POUR:                                                  \
    case GenreNoeud::DECLARATION_VARIABLE:                                                        \
    case GenreNoeud::DECLARATION_VARIABLE_MULTIPLE:                                               \
    case GenreNoeud::DIRECTIVE_AJOUTE_FINI:                                                       \
    case GenreNoeud::DIRECTIVE_AJOUTE_INIT:                                                       \
    case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:                                                      \
    case GenreNoeud::DIRECTIVE_CUISINE:                                                           \
    case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:                                           \
    case GenreNoeud::DIRECTIVE_EXECUTE:                                                           \
    case GenreNoeud::DIRECTIVE_INTROSPECTION:                                                     \
    case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:                                                    \
    case GenreNoeud::EXPANSION_VARIADIQUE:                                                        \
    case GenreNoeud::EXPRESSION_APPEL:                                                            \
    case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:                                             \
    case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:                                             \
    case GenreNoeud::EXPRESSION_COMME:                                                            \
    case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:                                           \
    case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:                                             \
    case GenreNoeud::EXPRESSION_INDEXAGE:                                                         \
    case GenreNoeud::EXPRESSION_INFO_DE:                                                          \
    case GenreNoeud::EXPRESSION_INIT_DE:                                                          \
    case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:                                                \
    case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:                                              \
    case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:                                                 \
    case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:                                          \
    case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:                                            \
    case GenreNoeud::EXPRESSION_LITTERALE_NUL:                                                    \
    case GenreNoeud::EXPRESSION_LOGIQUE:                                                          \
    case GenreNoeud::EXPRESSION_MEMOIRE:                                                          \
    case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:                                                 \
    case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:                                             \
    case GenreNoeud::EXPRESSION_PARENTHESE:                                                       \
    case GenreNoeud::EXPRESSION_PLAGE:                                                            \
    case GenreNoeud::EXPRESSION_PRISE_ADRESSE:                                                    \
    case GenreNoeud::EXPRESSION_PRISE_REFERENCE:                                                  \
    case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:                                            \
    case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:                                                 \
    case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:                                           \
    case GenreNoeud::EXPRESSION_REFERENCE_TYPE:                                                   \
    case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:                                         \
    case GenreNoeud::EXPRESSION_TAILLE_DE:                                                        \
    case GenreNoeud::EXPRESSION_TYPE_DE:                                                          \
    case GenreNoeud::EXPRESSION_TYPE_FONCTION:                                                    \
    case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:                                           \
    case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:                                                \
    case GenreNoeud::EXPRESSION_TYPE_TRANCHE:                                                     \
    case GenreNoeud::EXPRESSION_VIRGULE:                                                          \
    case GenreNoeud::INSTRUCTION_ARRETE:                                                          \
    case GenreNoeud::INSTRUCTION_BOUCLE:                                                          \
    case GenreNoeud::INSTRUCTION_CHARGE:                                                          \
    case GenreNoeud::INSTRUCTION_COMPOSEE:                                                        \
    case GenreNoeud::INSTRUCTION_CONTINUE:                                                        \
    case GenreNoeud::INSTRUCTION_DIFFERE:                                                         \
    case GenreNoeud::INSTRUCTION_DISCR:                                                           \
    case GenreNoeud::INSTRUCTION_DISCR_ENUM:                                                      \
    case GenreNoeud::INSTRUCTION_DISCR_UNION:                                                     \
    case GenreNoeud::INSTRUCTION_EMPL:                                                            \
    case GenreNoeud::INSTRUCTION_IMPORTE:                                                         \
    case GenreNoeud::INSTRUCTION_NON_INITIALISATION:                                              \
    case GenreNoeud::INSTRUCTION_POUR:                                                            \
    case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:                                                 \
    case GenreNoeud::INSTRUCTION_REPETE:                                                          \
    case GenreNoeud::INSTRUCTION_REPRENDS:                                                        \
    case GenreNoeud::INSTRUCTION_RETIENS:                                                         \
    case GenreNoeud::INSTRUCTION_RETOUR:                                                          \
    case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:                                                 \
    case GenreNoeud::INSTRUCTION_SAUFSI:                                                          \
    case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:                                                 \
    case GenreNoeud::INSTRUCTION_SI:                                                              \
    case GenreNoeud::INSTRUCTION_SI_STATIQUE:                                                     \
    case GenreNoeud::INSTRUCTION_TANTQUE:                                                         \
    case GenreNoeud::INSTRUCTION_TENTE:                                                           \
    case GenreNoeud::OPERATEUR_BINAIRE:                                                           \
    case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:                                               \
    case GenreNoeud::OPERATEUR_UNAIRE

#define CAS_POUR_NOEUDS_TYPES_PERSONALISABLES                                                     \
    case GenreNoeud::DECLARATION_ENUM:                                                            \
    case GenreNoeud::DECLARATION_OPAQUE:                                                          \
    case GenreNoeud::DECLARATION_STRUCTURE:                                                       \
    case GenreNoeud::DECLARATION_UNION:                                                           \
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
    case GenreNoeud::REEL:                                                                        \
    case GenreNoeud::REFERENCE:                                                                   \
    case GenreNoeud::RIEN:                                                                        \
    case GenreNoeud::TABLEAU_DYNAMIQUE:                                                           \
    case GenreNoeud::TABLEAU_FIXE:                                                                \
    case GenreNoeud::TUPLE:                                                                       \
    case GenreNoeud::TYPE_DE_DONNEES:                                                             \
    case GenreNoeud::TYPE_TRANCHE:                                                                \
    case GenreNoeud::VARIADIQUE

#define CAS_POUR_NOEUDS_TYPES                                                                     \
    CAS_POUR_NOEUDS_TYPES_PERSONALISABLES:                                                        \
    CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX
