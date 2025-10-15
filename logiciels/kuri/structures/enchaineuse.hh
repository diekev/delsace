/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <sstream>

#include "structures/chaine.hh"
#include "structures/chaine_statique.hh"

#include "utilitaires/macros.hh"

struct Enchaineuse {
    static constexpr auto TAILLE_TAMPON = 16 * 1024;

    struct Tampon {
        char donnees[TAILLE_TAMPON];
        int occupe = 0;
        Tampon *suivant = nullptr;
    };

    bool format_réel_court = false;
    int ligne_courante = 0;

    Tampon m_tampon_base{};
    Tampon *tampon_courant = nullptr;

    Enchaineuse();

    Enchaineuse(Enchaineuse const &) = delete;
    Enchaineuse &operator=(Enchaineuse const &) = delete;

    ~Enchaineuse();

    void ajoute(kuri::chaine_statique const &chn);

    void ajoute_inverse(const kuri::chaine_statique &chn);

    void ajoute(const char *c_str, int64_t N);

    void ajoute_caractère(char c);

    void imprime_dans_flux(std::ostream &flux);

    void ajoute_tampon();

    int nombre_tampons() const;

    int nombre_tampons_alloues() const;

    int64_t mémoire_utilisée() const;

    int64_t taille_chaine() const;

    kuri::chaine chaine() const;

    void permute(Enchaineuse &autre);

    void réinitialise();

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

    /**
     * Ajoute la chaine en un seul morceau dans l'enchaineuse, et retourne une
     * nouvelle chaine pour les données ajoutées.
     */
    kuri::chaine_statique ajoute_chaine_statique(kuri::chaine_statique chaine);

    /* Fonctions pour les formattages d'erreurs. */
    void imprime_caractère_vide(const int64_t nombre, kuri::chaine_statique chaine);
    void imprime_tilde(kuri::chaine_statique chaine);
    void imprime_tilde(kuri::chaine_statique chaine, int64_t début, int64_t fin);
};

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wbool-compare"
#endif
template <typename T>
unsigned nombre_chiffre_base_10_pro(T v)
{
    unsigned résultat = 1;

    for (;;) {
        if (PROBABLE(v < 10)) {
            return résultat;
        }

        if (PROBABLE(v < 100)) {
            return résultat + 1;
        }

        if (PROBABLE(v < 1000)) {
            return résultat + 2;
        }

        if (PROBABLE(v < 10000)) {
            return résultat + 3;
        }

        résultat += 4;
        v /= 10000;
    }
}

inline unsigned nombre_chiffre_base_10_pro(int8_t v)
{
    if (PROBABLE(v < 10)) {
        return 1;
    }

    if (PROBABLE(v < 100)) {
        return 2;
    }

    return 3;
}

inline unsigned nombre_chiffre_base_10_pro(uint8_t v)
{
    if (PROBABLE(v < 10)) {
        return 1;
    }

    if (PROBABLE(v < 100)) {
        return 2;
    }

    return 3;
}

template <typename T>
unsigned nombre_vers_chaine(char *tampon, T valeur)
{
    auto const n = nombre_chiffre_base_10_pro(valeur);

    auto pos = n - 1;

    while (valeur >= 10) {
        auto const q = valeur / 10;
        auto const r = valeur % 10;
        tampon[pos--] = static_cast<char>('0' + r);
        valeur = T(q);
    }

    tampon[0] = static_cast<char>('0' + valeur);
    return n;
}
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

template <typename T>
Enchaineuse &operator<<(Enchaineuse &enchaineuse, T valeur)
{
    if constexpr (std::is_same_v<bool, T>) {
        enchaineuse.ajoute((valeur) ? "1" : "0");
        return enchaineuse;
    }

    if constexpr (std::is_signed_v<T>) {
        if (valeur < 0) {
            enchaineuse.ajoute_caractère('-');
            valeur = -valeur;
        }
    }

    if constexpr (std::is_integral_v<T>) {
        char tampon[32];
        auto const n = nombre_vers_chaine(tampon, valeur);
        enchaineuse.ajoute(kuri::chaine_statique(tampon, n));
        return enchaineuse;
    }

    if constexpr (std::is_floating_point_v<T>) {
        char tampon[128];
        if (enchaineuse.format_réel_court) {
            auto const n = snprintf(tampon, 128, "%.3f", double(valeur));
            enchaineuse.ajoute(kuri::chaine_statique(tampon, n));
        }
        else {
            auto const n = snprintf(tampon, 128, "%.19f", double(valeur));
            enchaineuse.ajoute(kuri::chaine_statique(tampon, n));
        }
        return enchaineuse;
    }

    std::stringstream flux;
    flux << valeur;

    auto const chn = flux.str();
    enchaineuse.ajoute(chn.c_str(), static_cast<int64_t>(chn.size()));
    return enchaineuse;
}

template <typename T>
inline Enchaineuse &operator<<(Enchaineuse &enchaineuse, T *valeur)
{
    char tampon[32];
    auto const n = nombre_vers_chaine(tampon, reinterpret_cast<uint64_t>(valeur));
    enchaineuse.ajoute(kuri::chaine_statique(tampon, n));
    return enchaineuse;
}

template <>
inline Enchaineuse &operator<<(Enchaineuse &enchaineuse, char valeur)
{
    enchaineuse.ajoute_caractère(valeur);
    return enchaineuse;
}

template <size_t N>
Enchaineuse &operator<<(Enchaineuse &enchaineuse, const char (&c)[N])
{
    enchaineuse.ajoute(c, static_cast<int64_t>(N));
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
