/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "identifiant.hh"

TableIdentifiant::TableIdentifiant()
{
    initialise_identifiants(*this);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_chaine(kuri::chaine_statique nom)
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

    auto vue_nom = kuri::chaine_statique(ptr, nom.taille());
    return ajoute_identifiant(vue_nom);
}

int64_t TableIdentifiant::taille() const
{
    return table.taille();
}

int64_t TableIdentifiant::memoire_utilisee() const
{
    auto memoire = int64_t(0);
    memoire += identifiants.memoire_utilisee();
    memoire += table.taille_mémoire();
    memoire += enchaineuse.mémoire_utilisée();

    POUR_TABLEAU_PAGE (identifiants) {
        memoire += it.nom_broye.taille();
    }

    return memoire;
}

IdentifiantCode *TableIdentifiant::ajoute_identifiant(kuri::chaine_statique nom)
{
    auto ident = identifiants.ajoute_element();
    ident->nom = nom;

    table.insère(nom, ident);

    return ident;
}

/* ************************************************************************** */

namespace ID {
#define DEFINIS_IDENTIFIANT_COMMUN_SIMPLE(x) IdentifiantCode *x;
#define DEFINIS_IDENTIFIANT_COMMUN(x, y) IdentifiantCode *x;
#include "identifiant.def"
#undef DEFINIS_IDENTIFIANT_COMMUN
#undef DEFINIS_IDENTIFIANT_COMMUN_SIMPLE
}  // namespace ID

void initialise_identifiants(TableIdentifiant &table)
{
#define DEFINIS_IDENTIFIANT_COMMUN_SIMPLE(x) ID::x = table.identifiant_pour_chaine(#x);
#define DEFINIS_IDENTIFIANT_COMMUN(x, y) ID::x = table.identifiant_pour_chaine(y);
#include "identifiant.def"
#undef DEFINIS_IDENTIFIANT_COMMUN
#undef DEFINIS_IDENTIFIANT_COMMUN_SIMPLE
}
