/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include <iosfwd>

struct EspaceDeTravail;
struct UniteCompilation;

#define ENUMERE_TACHES_POSSIBLES(O)                                                               \
    O(CHARGER, CHARGEMENT, "chargement", 0)                                                       \
    O(LEXER, LEXAGE, "lexage", 1)                                                                 \
    O(PARSER, PARSAGE, "parsage", 2)                                                              \
    O(CREER_FONCTION_INIT_TYPE, CREATION_FONCTION_INIT_TYPE, "création fonction init type", 3)    \
    O(TYPER, TYPAGE, "typage", 4)                                                                 \
    O(CONVERTIR_NOEUD_CODE, CONVERSION_NOEUD_CODE, "conversion noeud code", 5)                    \
    O(ENVOYER_MESSAGE, ENVOIE_MESSAGE, "envoie message", 6)                                       \
    O(GENERER_RI, GENERATION_RI, "génération RI", 7)                                              \
    O(EXECUTER, EXECUTION, "exécution", 8)                                                        \
    O(OPTIMISER, OPTIMISATION, "optimisation", 9)                                                 \
    O(GENERER_CODE, GENERATION_CODE_MACHINE, "génération code machine", 10)                       \
    O(LIER_PROGRAMME, LIAISON_PROGRAMME, "liaison programme", 11)

#define ENUMERE_GENRES_TACHE(O)                                                                   \
    O(DORMIR, DORS, "dormir", 0)                                                                  \
    O(COMPILATION_TERMINEE, COMPILATION_TERMINEE, "compilation terminée", 0)                      \
    ENUMERE_TACHES_POSSIBLES(O)

enum class GenreTâche {
#define ENUMERE_GENRE_TACHE(VERBE, ACTION, CHAINE, INDEX) ACTION,
    ENUMERE_GENRES_TACHE(ENUMERE_GENRE_TACHE)
#undef ENUMERE_GENRE_TACHE

        NOMBRE_ELEMENTS
};

const char *chaine_genre_tâche(GenreTâche genre);

std::ostream &operator<<(std::ostream &os, GenreTâche genre);

struct Tâche {
    GenreTâche genre = GenreTâche::DORS;
    UniteCompilation *unité = nullptr;
    EspaceDeTravail *espace = nullptr;

    static Tâche dors(EspaceDeTravail *espace_);

    static Tâche compilation_terminée();

    static Tâche génération_code_machine(EspaceDeTravail *espace_);

    static Tâche liaison_objet(EspaceDeTravail *espace_);
};
