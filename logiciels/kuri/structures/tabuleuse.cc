/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#include "tabuleuse.hh"

#include <iostream>

#include "utilitaires/unicode.hh"

ChaineUTF8::ChaineUTF8() = default;

ChaineUTF8::ChaineUTF8(const char *chn) : chaine(chn)
{
    calcule_taille();
}

ChaineUTF8::ChaineUTF8(kuri::chaine const &chn) : chaine(chn)
{
    calcule_taille();
}

ChaineUTF8::ChaineUTF8(kuri::chaine_statique chn) : chaine(chn)
{
    calcule_taille();
}

int64_t ChaineUTF8::taille() const
{
    return m_taille;
}

void ChaineUTF8::calcule_taille()
{
    for (auto i = 0l; i < chaine.taille();) {
        auto n = unicode::nombre_octets(&chaine[i]);
        m_taille += 1;
        i += n;
    }
}

std::ostream &operator<<(std::ostream &os, const ChaineUTF8 &chaine)
{
    os << chaine.chaine;
    return os;
}

Tabuleuse::Tabuleuse(const std::initializer_list<ChaineUTF8> &titres)
    : nombre_colonnes(static_cast<int64_t>(titres.size()))
{
    ajoute_ligne(titres);

    for (auto i = 0; i < nombre_colonnes; ++i) {
        alignements.ajoute(Alignement::GAUCHE);
    }
}

void Tabuleuse::alignement(int idx, Alignement a)
{
    alignements[idx] = a;
}

void Tabuleuse::ajoute_ligne(const std::initializer_list<ChaineUTF8> &valeurs)
{
    assert(static_cast<int64_t>(valeurs.size()) <= nombre_colonnes);

    auto ligne = Ligne{};

    for (auto const &valeur : valeurs) {
        ligne.colonnes.ajoute(valeur);
    }

    lignes.ajoute(ligne);
}

static void imprime_ligne_demarcation(kuri::tableau<int64_t> const &tailles_max_colonnes)
{
    for (auto i = 0; i < tailles_max_colonnes.taille(); ++i) {
        std::cout << '+' << '-';

        for (auto j = 0; j < tailles_max_colonnes[i]; ++j) {
            std::cout << '-';
        }

        std::cout << '-';
    }

    std::cout << '+' << '\n';
}

static void imprime_ligne(Tabuleuse::Ligne const &ligne,
                          kuri::tableau<int64_t> const &tailles_max_colonnes,
                          kuri::tableau<Alignement> const &alignements)
{
    for (auto i = 0; i < ligne.colonnes.taille(); ++i) {
        auto const &colonne = ligne.colonnes[i];

        std::cout << '|' << ' ';

        if (alignements[i] == Alignement::DROITE) {
            for (auto j = 0; j < tailles_max_colonnes[i] - colonne.taille(); ++j) {
                std::cout << ' ';
            }
        }

        std::cout << colonne;

        if (alignements[i] == Alignement::GAUCHE) {
            for (auto j = colonne.taille(); j < tailles_max_colonnes[i]; ++j) {
                std::cout << ' ';
            }
        }

        std::cout << ' ';
    }

    for (auto i = ligne.colonnes.taille(); i < tailles_max_colonnes.taille(); ++i) {
        std::cout << '|' << ' ';

        for (auto j = 0; j < tailles_max_colonnes[i]; ++j) {
            std::cout << ' ';
        }

        std::cout << ' ';
    }

    std::cout << '|' << '\n';
}

void imprime_tableau(Tabuleuse &tableau)
{
    // pour chaque ligne, calcul la taille maximale de la colonne
    kuri::tableau<int64_t> tailles_max_colonnes{};
    int64_t nombre_colonnes = 0;

    for (auto const &ligne : tableau.lignes) {
        nombre_colonnes = std::max(nombre_colonnes, ligne.colonnes.taille());
    }

    tailles_max_colonnes.redimensionne(nombre_colonnes);

    POUR (tailles_max_colonnes) {
        it = 0;
    }

    for (auto const &ligne : tableau.lignes) {
        for (auto i = 0; i < ligne.colonnes.taille(); ++i) {
            tailles_max_colonnes[i] = std::max(tailles_max_colonnes[i],
                                               ligne.colonnes[i].taille());
        }
    }

    /* ajout de marges */
    auto taille_ligne = int64_t(0);
    for (auto const &taille : tailles_max_colonnes) {
        taille_ligne += taille + 2;
    }

    /* impression */
    imprime_ligne_demarcation(tailles_max_colonnes);
    imprime_ligne(tableau.lignes[0], tailles_max_colonnes, tableau.alignements);
    imprime_ligne_demarcation(tailles_max_colonnes);

    for (auto i = 1; i < tableau.lignes.taille(); ++i) {
        imprime_ligne(tableau.lignes[i], tailles_max_colonnes, tableau.alignements);
    }

    imprime_ligne_demarcation(tailles_max_colonnes);
}
