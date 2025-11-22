/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "enchaineuse.hh"

#include <string>

#include "utilitaires/logeuse_memoire.hh"

Enchaineuse::Enchaineuse() : tampon_courant(&m_tampon_base)
{
}

Enchaineuse::~Enchaineuse()
{
    auto tampon = m_tampon_base.suivant;

    while (tampon != nullptr) {
        auto tmp = tampon;
        tampon = tampon->suivant;

        mémoire::deloge("Tampon", tmp);
    }
}

void Enchaineuse::ajoute(const kuri::chaine_statique &chn)
{
    ajoute(chn.pointeur(), chn.taille());
}

void Enchaineuse::ajoute_inverse(const kuri::chaine_statique &chn)
{
    for (auto i = chn.taille() - 1; i >= 0; --i) {
        ajoute_caractère(chn.pointeur()[i]);
    }
}

void Enchaineuse::ajoute(const char *c_str, int64_t N)
{
    auto tampon = tampon_courant;

    for (auto i = 0; i < N; i++) {
        if (c_str[i] == '\n') {
            ligne_courante += 1;
        }
    }

    if (tampon->occupe + N < TAILLE_TAMPON) {
        memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(N));
        tampon->occupe += static_cast<int>(N);
    }
    else {
        auto taille_a_ecrire = N;
        auto taille_max = TAILLE_TAMPON - tampon->occupe;

        if (taille_max != 0) {
            memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(taille_max));
            tampon->occupe += taille_max;
            taille_a_ecrire -= taille_max;
        }

        auto décalage = taille_max;
        while (taille_a_ecrire > 0) {
            ajoute_tampon();
            tampon = tampon_courant;

            auto taille_ecrite = std::min(taille_a_ecrire, static_cast<int64_t>(TAILLE_TAMPON));

            memcpy(&tampon->donnees[0], c_str + décalage, static_cast<size_t>(taille_ecrite));
            tampon->occupe += static_cast<int>(taille_ecrite);

            taille_a_ecrire -= taille_ecrite;
            décalage += static_cast<int>(taille_ecrite);
        }
    }
}

void Enchaineuse::ajoute_caractère(char c)
{
    auto tampon = tampon_courant;

    if (tampon->occupe == TAILLE_TAMPON) {
        ajoute_tampon();
        tampon = tampon_courant;
    }

    if (c == '\n') {
        ligne_courante += 1;
    }

    tampon->donnees[tampon->occupe++] = c;
}

void Enchaineuse::imprime_dans_flux(std::ostream &flux)
{
    auto tampon = &m_tampon_base;

    while (tampon != nullptr) {
        flux.write(&tampon->donnees[0], tampon->occupe);
        tampon = tampon->suivant;
    }
}

void Enchaineuse::ajoute_tampon()
{
    if (tampon_courant->suivant) {
        tampon_courant = tampon_courant->suivant;
        return;
    }

    auto tampon = mémoire::loge<Tampon>("Tampon");
    tampon_courant->suivant = tampon;
    tampon_courant = tampon;
}

int Enchaineuse::nombre_tampons() const
{
    auto compte = 1;
    auto tampon = m_tampon_base.suivant;

    while (tampon != nullptr) {
        compte += 1;
        tampon = tampon->suivant;
    }

    return compte;
}

int Enchaineuse::nombre_tampons_alloues() const
{
    return nombre_tampons() - 1;
}

int64_t Enchaineuse::mémoire_utilisée() const
{
    return nombre_tampons_alloues() * TAILLE_TAMPON;
}

int64_t Enchaineuse::taille_chaine() const
{
    auto taille = 0l;
    auto tampon = &m_tampon_base;

    while (tampon) {
        taille += tampon->occupe;
        tampon = tampon->suivant;
    }

    return taille;
}

kuri::chaine Enchaineuse::chaine() const
{
    auto taille = taille_chaine();

    if (taille == 0) {
        return "";
    }

    auto résultat = kuri::chaine();
    résultat.redimensionne(taille);

    auto tampon = &m_tampon_base;
    auto décalage = 0;

    while (tampon) {
        memcpy(&résultat[décalage], &tampon->donnees[0], static_cast<size_t>(tampon->occupe));
        décalage += tampon->occupe;
        tampon = tampon->suivant;
    }

    return résultat;
}

kuri::chaine_statique Enchaineuse::chaine_statique() const
{
    auto taille = taille_chaine();
    return {&m_tampon_base.donnees[0], taille};
}

kuri::chaine_statique Enchaineuse::ajoute_chaine_statique(kuri::chaine_statique chaine)
{
    /* Avons nous de la place pour la chaine dans le tampon courant ? */
    if (tampon_courant->occupe + chaine.taille() >= TAILLE_TAMPON) {
        /* Ajoute un tampon. */
        ajoute_tampon();
    }

    auto ptr = &tampon_courant->donnees[tampon_courant->occupe];
    memcpy(ptr, chaine.pointeur(), static_cast<size_t>(chaine.taille()));
    tampon_courant->occupe += static_cast<int>(chaine.taille());
    return {ptr, chaine.taille()};
}

void Enchaineuse::permute(Enchaineuse &autre)
{
    if (tampon_courant != &m_tampon_base && autre.tampon_courant != &autre.m_tampon_base) {
        std::swap(tampon_courant, autre.tampon_courant);
    }

    for (auto i = 0; i < TAILLE_TAMPON; ++i) {
        std::swap(m_tampon_base.donnees[i], autre.m_tampon_base.donnees[i]);
    }

    std::swap(m_tampon_base.occupe, autre.m_tampon_base.occupe);
    std::swap(m_tampon_base.suivant, autre.m_tampon_base.suivant);
}

void Enchaineuse::réinitialise()
{
    auto t = &m_tampon_base;
    while (t) {
        t->occupe = 0;
        t = t->suivant;
    }

    tampon_courant = &m_tampon_base;
}

void Enchaineuse::imprime_caractère_vide(const int64_t nombre, kuri::chaine_statique chaine)
{
    for (auto i = int64_t(0); i < std::min(nombre, chaine.taille());) {
        if (chaine[i] == '\t') {
            this->ajoute_caractère('\t');
        }
        else {
            this->ajoute_caractère(' ');
        }

        i += chaine.décalage_pour_caractère(i);
    }
}

void Enchaineuse::imprime_tilde(kuri::chaine_statique chaine)
{
    imprime_tilde(chaine, 0, chaine.taille() - 1);
}

void Enchaineuse::imprime_tilde(kuri::chaine_statique chaine, int64_t début, int64_t fin)
{
    for (auto i = début; i < fin;) {
        this->ajoute_caractère('~');
        i += chaine.décalage_pour_caractère(i);
    }
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const kuri::chaine_statique &chn)
{
    enchaineuse.ajoute(chn.pointeur(), chn.taille());
    return enchaineuse;
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const kuri::chaine &chn)
{
    enchaineuse.ajoute(chn.pointeur(), chn.taille());
    return enchaineuse;
}

Enchaineuse &operator<<(Enchaineuse &enchaineuse, const char *chn)
{
    auto ptr = chn;

    while (*chn != '\0') {
        ++chn;
    }

    enchaineuse.ajoute(ptr, chn - ptr);

    return enchaineuse;
}
