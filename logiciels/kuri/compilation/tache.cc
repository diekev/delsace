/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "tache.hh"

#include <ostream>

const char *chaine_genre_tache(GenreTache genre)
{
#define ENUMERE_GENRE_TACHE(VERBE, ACTION, CHAINE, INDEX)                                         \
    case GenreTache::ACTION:                                                                      \
        return CHAINE;
    switch (genre) {
        ENUMERE_GENRES_TACHE(ENUMERE_GENRE_TACHE)
        case GenreTache::NOMBRE_ELEMENTS:
        {
            break;
        }
    }
#undef ENUMERE_GENRE_TACHE

    return "erreur";
}

std::ostream &operator<<(std::ostream &os, GenreTache genre)
{
    os << chaine_genre_tache(genre);
    return os;
}

Tache Tache::dors(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::DORS;
    t.espace = espace_;
    return t;
}

Tache Tache::compilation_terminee()
{
    Tache t;
    t.genre = GenreTache::COMPILATION_TERMINEE;
    return t;
}

Tache Tache::genere_fichier_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::GENERATION_CODE_MACHINE;
    t.espace = espace_;
    return t;
}

Tache Tache::liaison_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::LIAISON_PROGRAMME;
    t.espace = espace_;
    return t;
}
