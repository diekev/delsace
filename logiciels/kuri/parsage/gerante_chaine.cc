/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "gerante_chaine.hh"

int64_t GeranteChaine::ajoute_chaine(const kuri::chaine &chaine)
{
    return ajoute_chaine(kuri::chaine_statique(chaine));
}

int64_t GeranteChaine::ajoute_chaine(kuri::chaine_statique chaine)
{
    if ((enchaineuse.tampon_courant->occupe + chaine.taille()) >= Enchaineuse::TAILLE_TAMPON) {
        enchaineuse.ajoute_tampon();
    }

    // calcul l'adresse de la chaine
    auto adresse = (enchaineuse.nombre_tampons() - 1) * Enchaineuse::TAILLE_TAMPON +
                   enchaineuse.tampon_courant->occupe;

    enchaineuse.ajoute(chaine);

    auto result = int64_t((size_t(chaine.taille()) << 32) | (size_t(adresse) & 0xffffffff));

    // std::cerr << __func__ << " : " << chaine << " taille " << chaine.taille() << " adresse " << result << '\n';
    return (result);
}

kuri::chaine_statique GeranteChaine::chaine_pour_adresse(int64_t adresse) const
{
    assert(adresse >= 0);

    /* MSVC convertis l'adresse implicitement vers un nombre naturel, ce qui cause une erreur de
     * compilation. Convertissons explicitement. */
    auto adresse_naturelle = size_t(adresse);
    auto taille = (adresse_naturelle >> 32) & 0xffffffff;
    adresse_naturelle = (adresse_naturelle & 0xfffffff);

    auto tampon_courant = &enchaineuse.m_tampon_base;

    while (adresse_naturelle >= size_t(Enchaineuse::TAILLE_TAMPON)) {
        adresse_naturelle -= size_t(Enchaineuse::TAILLE_TAMPON);
        tampon_courant = tampon_courant->suivant;
    }

    assert(tampon_courant);
    return {&tampon_courant->donnees[adresse_naturelle], int64_t(taille)};
}

int64_t GeranteChaine::memoire_utilisee() const
{
    return enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;
}
