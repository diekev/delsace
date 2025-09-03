/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "outils_dependants_sur_lexemes.hh"

#include <iostream>

#include "parsage/modules.hh"
#include "parsage/site_source.hh"

#include "structures/enchaineuse.hh"

void imprime_erreur(SiteSource site, kuri::chaine message)
{
    auto fichier = site.fichier;
    auto indice_ligne = site.indice_ligne;
    auto indice_colonne = site.indice_colonne;

    auto ligne_courante = fichier->tampon()[indice_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : " << fichier->chemin() << ":" << indice_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = int64_t(0); i < indice_colonne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += ligne_courante.décalage_pour_caractère(i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << message;

    enchaineuse.imprime_dans_flux(std::cerr);
}
