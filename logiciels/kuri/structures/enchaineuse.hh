/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/flux_chaine.hh"
#include "structures/chaine.hh"
#include "structures/chaine_statique.hh"

struct Enchaineuse {
    static constexpr auto TAILLE_TAMPON = 16 * 1024;

    struct Tampon {
        char donnees[TAILLE_TAMPON];
        int occupe = 0;
        Tampon *suivant = nullptr;
    };

    Tampon m_tampon_base{};
    Tampon *tampon_courant = nullptr;

    Enchaineuse();

    Enchaineuse(Enchaineuse const &) = delete;
    Enchaineuse &operator=(Enchaineuse const &) = delete;

    ~Enchaineuse();

    void ajoute(kuri::chaine_statique const &chn);

    void ajoute_inverse(const kuri::chaine_statique &chn);

    void ajoute(const char *c_str, long N);

    void pousse_caractere(char c);

    void imprime_dans_flux(std::ostream &flux);

    void ajoute_tampon();

    int nombre_tampons() const;

    int nombre_tampons_alloues() const;

    long taille_chaine() const;

    kuri::chaine chaine() const;

    void permute(Enchaineuse &autre);

    void reinitialise();

    /**
     * Retourne une chaine_statique provenant du premier tampon de l'enchaineuse.
     * Aucune vérification n'est faite pour savoir si le tampon contient une chaine
     * complète ; les appelants doivent savoir si la chaine est suffisament petite
     * pour tenir dans un seul tampon.
     *
     * Ceci peut être utilisé en conjonction avec #reinitialise() pour lors de la
     * génération de plusieurs petites chaines.
     */
    kuri::chaine_statique chaine_statique() const;
};

template <typename T>
Enchaineuse &operator<<(Enchaineuse &enchaineuse, T const &valeur)
{
    dls::flux_chaine flux;
    flux << valeur;

    for (auto c : flux.chn()) {
        enchaineuse.pousse_caractere(c);
    }

    return enchaineuse;
}

template <>
inline Enchaineuse &operator<<(Enchaineuse &enchaineuse, char const &valeur)
{
    enchaineuse.pousse_caractere(valeur);
    return enchaineuse;
}

template <size_t N>
Enchaineuse &operator<<(Enchaineuse &enchaineuse, const char (&c)[N])
{
    enchaineuse.ajoute(c, static_cast<long>(N));
    return enchaineuse;
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, kuri::chaine_statique const &chn);

Enchaineuse &operator<<(Enchaineuse &enchaineuse, kuri::chaine const &chn);

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const char *chn);

template <typename... Ts>
kuri::chaine enchaine(Ts &&...ts)
{
    Enchaineuse enchaineuse;
    ((enchaineuse << ts), ...);
    return enchaineuse.chaine();
}
