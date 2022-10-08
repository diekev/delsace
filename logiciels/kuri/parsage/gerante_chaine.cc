/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "gerante_chaine.hh"

long GeranteChaine::ajoute_chaine(const kuri::chaine &chaine)
{
    return ajoute_chaine(kuri::chaine_statique(chaine));
}

long GeranteChaine::ajoute_chaine(kuri::chaine_statique chaine)
{
    if ((enchaineuse.tampon_courant->occupe + chaine.taille()) >= Enchaineuse::TAILLE_TAMPON) {
        enchaineuse.ajoute_tampon();
    }

    // calcul l'adresse de la chaine
    auto adresse = (enchaineuse.nombre_tampons() - 1) * Enchaineuse::TAILLE_TAMPON +
                   enchaineuse.tampon_courant->occupe;

    enchaineuse.ajoute(chaine);

    return adresse | (chaine.taille() << 32);
}

kuri::chaine_statique GeranteChaine::chaine_pour_adresse(long adresse) const
{
    assert(adresse >= 0);

    auto taille = (adresse >> 32) & 0xffffffff;
    adresse = (adresse & 0xfffffff);

    auto tampon_courant = &enchaineuse.m_tampon_base;

    while (adresse >= Enchaineuse::TAILLE_TAMPON) {
        adresse -= Enchaineuse::TAILLE_TAMPON;
        tampon_courant = tampon_courant->suivant;
    }

    assert(tampon_courant);
    return {&tampon_courant->donnees[adresse], taille};
}

long GeranteChaine::memoire_utilisee() const
{
    return enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;
}
