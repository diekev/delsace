/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

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

int64_t TableIdentifiant::taille() const
{
    return table.taille();
}

int64_t TableIdentifiant::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += identifiants.memoire_utilisee();
    memoire += table.taille() *
               (taille_de(dls::vue_chaine_compacte) + taille_de(IdentifiantCode *));
    memoire += enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;

    POUR_TABLEAU_PAGE (identifiants) {
        memoire += it.nom_broye.taille();
    }

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
