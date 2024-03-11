/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "tache.hh"

#include <ostream>

const char *chaine_genre_tâche(GenreTâche genre)
{
#define ENUMERE_GENRE_TACHE(VERBE, ACTION, CHAINE, INDEX)                                         \
    case GenreTâche::ACTION:                                                                      \
        return CHAINE;
    switch (genre) {
        ENUMERE_GENRES_TACHE(ENUMERE_GENRE_TACHE)
        case GenreTâche::NOMBRE_ELEMENTS:
        {
            break;
        }
    }
#undef ENUMERE_GENRE_TACHE

    return "erreur";
}

std::ostream &operator<<(std::ostream &os, GenreTâche genre)
{
    os << chaine_genre_tâche(genre);
    return os;
}

Tâche Tâche::dors(EspaceDeTravail *espace_)
{
    Tâche t;
    t.genre = GenreTâche::DORS;
    t.espace = espace_;
    return t;
}

Tâche Tâche::compilation_terminée()
{
    Tâche t;
    t.genre = GenreTâche::COMPILATION_TERMINÉE;
    return t;
}

Tâche Tâche::génération_code_machine(EspaceDeTravail *espace_)
{
    Tâche t;
    t.genre = GenreTâche::GENERATION_CODE_MACHINE;
    t.espace = espace_;
    return t;
}

Tâche Tâche::liaison_objet(EspaceDeTravail *espace_)
{
    Tâche t;
    t.genre = GenreTâche::LIAISON_PROGRAMME;
    t.espace = espace_;
    return t;
}
