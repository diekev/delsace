/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include "chaine.hh"
#include "tableau.hh"

struct ChaineUTF8 {
    kuri::chaine chaine{};
    int64_t m_taille = 0;

    ChaineUTF8();

    ChaineUTF8(char const *chn);

    ChaineUTF8(kuri::chaine const &chn);

    ChaineUTF8(kuri::chaine_statique chn);

    int64_t taille() const;

    void calcule_taille();
};

std::ostream &operator<<(std::ostream &os, ChaineUTF8 const &chaine);

enum class Alignement {
    GAUCHE,
    DROITE,
};

struct Tabuleuse {
    struct Ligne {
        kuri::tableau<ChaineUTF8> colonnes{};
    };

    kuri::tableau<Ligne> lignes{};
    kuri::tableau<Alignement> alignements{};
    int64_t nombre_colonnes = 0;

    Tabuleuse(std::initializer_list<ChaineUTF8> const &titres);

    void alignement(int idx, Alignement a);

    void ajoute_ligne(std::initializer_list<ChaineUTF8> const &valeurs);
};

void imprime_tableau(Tabuleuse &tableau);
