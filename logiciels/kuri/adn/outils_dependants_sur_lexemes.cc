/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "outils_dependants_sur_lexemes.hh"

#include <iostream>

#include "biblinternes/langage/erreur.hh"

#include "parsage/modules.hh"
#include "parsage/site_source.hh"

#include "structures/enchaineuse.hh"

void imprime_erreur(SiteSource site, kuri::chaine message)
{
    auto fichier = site.fichier;
    auto index_ligne = site.index_ligne;
    auto index_colonne = site.index_colonne;

    auto ligne_courante = fichier->tampon()[index_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : " << fichier->chemin() << ":" << index_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = int64_t(0); i < index_colonne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += lng::decalage_pour_caractere(ligne_courante, i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << message;

    enchaineuse.imprime_dans_flux(std::cerr);
}
