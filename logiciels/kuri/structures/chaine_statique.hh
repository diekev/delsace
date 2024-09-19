/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <cstring>    /* pour la déclaration de std::strlen */
#include <functional> /* pour la déclaration de std::hash */

#include "biblinternes/structures/vue_chaine_compacte.hh"

namespace kuri {

/* ATTENTION: cette structure doit avoir la même IBA que le type « chaine » du langage,
 * car nous l'utilisons dans les fonctions d'interface de la Compilatrice.
 */
struct chaine_statique {
  private:
    const char *pointeur_ = nullptr;
    int64_t taille_ = 0;

  public:
    chaine_statique() = default;

    chaine_statique(chaine_statique const &) = default;
    chaine_statique &operator=(chaine_statique const &) = default;

    chaine_statique(dls::vue_chaine_compacte chn) : chaine_statique(chn.pointeur(), chn.taille())
    {
    }

    chaine_statique(const char *ptr, int64_t taille) : pointeur_(ptr), taille_(taille)
    {
    }

    chaine_statique(const char *ptr) : chaine_statique(ptr, static_cast<int64_t>(std::strlen(ptr)))
    {
    }

    template <uint64_t N>
    chaine_statique(const char (&ptr)[N]) : chaine_statique(ptr, static_cast<int64_t>(N))
    {
    }

    const char *pointeur() const
    {
        return pointeur_;
    }

    int64_t taille() const
    {
        return taille_;
    }

    explicit operator bool() const
    {
        return taille() != 0;
    }

    chaine_statique sous_chaine(int index) const
    {
        return chaine_statique(pointeur() + index, taille() - index);
    }

    chaine_statique sous_chaine(int64_t début, int64_t fin) const
    {
        assert(début >= 0 && début < taille());
        assert(fin >= 0 && fin <= taille());
        assert(début <= fin);
        return chaine_statique(pointeur() + début, fin - début);
    }
};

bool operator<(chaine_statique const &c1, chaine_statique const &c2);

bool operator>(chaine_statique const &c1, chaine_statique const &c2);

bool operator==(chaine_statique const &vc1, chaine_statique const &vc2);

bool operator==(chaine_statique const &vc1, char const *vc2);

bool operator==(char const *vc1, chaine_statique const &vc2);

bool operator!=(chaine_statique const &vc1, chaine_statique const &vc2);

bool operator!=(chaine_statique const &vc1, char const *vc2);

bool operator!=(char const *vc1, chaine_statique const &vc2);

std::ostream &operator<<(std::ostream &os, chaine_statique const &vc);

}  // namespace kuri

namespace std {

template <>
struct hash<kuri::chaine_statique> {
    std::size_t operator()(kuri::chaine_statique const &chn) const
    {
        auto h = std::hash<std::string_view>{};
        return h(std::string_view(chn.pointeur(), static_cast<size_t>(chn.taille())));
    }
};

} /* namespace std */
