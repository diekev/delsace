/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "identifiant.hh"

TableIdentifiant::TableIdentifiant()
{
    initialise_identifiants(*this);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_chaine(const dls::vue_chaine_compacte &nom)
{
    auto trouve = false;
    auto iter = table.trouve(nom, trouve);

    if (trouve) {
        return iter;
    }

    return ajoute_identifiant(nom);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_nouvelle_chaine(kuri::chaine const &nom)
{
    auto trouve = false;
    auto iter = table.trouve(nom, trouve);

    if (trouve) {
        return iter;
    }

    auto tampon_courant = enchaineuse.tampon_courant;

    if (tampon_courant->occupe + nom.taille() > Enchaineuse::TAILLE_TAMPON) {
        enchaineuse.ajoute_tampon();
        tampon_courant = enchaineuse.tampon_courant;
    }

    auto ptr = &tampon_courant->donnees[tampon_courant->occupe];
    enchaineuse.ajoute(nom);

    auto vue_nom = dls::vue_chaine_compacte(ptr, nom.taille());
    return ajoute_identifiant(vue_nom);
}

long TableIdentifiant::taille() const
{
    return table.taille();
}

long TableIdentifiant::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += identifiants.memoire_utilisee();
    memoire += table.taille() *
               (taille_de(dls::vue_chaine_compacte) + taille_de(IdentifiantCode *));
    memoire += enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;
    return memoire;
}

IdentifiantCode *TableIdentifiant::ajoute_identifiant(const dls::vue_chaine_compacte &nom)
{
    auto ident = identifiants.ajoute_element();
    ident->nom = {&nom[0], nom.taille()};

    table.insere(nom, ident);

    return ident;
}

/* ************************************************************************** */

namespace ID {
#define ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(x, y) IdentifiantCode *x;
ENUMERE_IDENTIFIANTS_COMMUNS
#undef ENUMERE_IDENTIFIANT_COMMUN_SIMPLE
}  // namespace ID

void initialise_identifiants(TableIdentifiant &table)
{
#define ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(x, y) ID::x = table.identifiant_pour_chaine(y);
    ENUMERE_IDENTIFIANTS_COMMUNS
#undef ENUMERE_IDENTIFIANT_COMMUN_SIMPLE
}
