/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "monomorphisations.hh"

#include "parsage/identifiant.hh"

#include "typage.hh"
#include "utilitaires/log.hh"

std::ostream &operator<<(std::ostream &os, const ItemMonomorphisation &item)
{
    os << item.ident->nom << " " << chaine_type(item.type);
    if (!item.est_type) {
        os << " " << item.valeur;
    }
    return os;
}

NoeudExpression *Monomorphisations::trouve_monomorphisation(
    kuri::tableau_statique<ItemMonomorphisation> items) const
{
    auto monomorphisations_ = monomorphisations.verrou_lecture();

    POUR (*monomorphisations_) {
        if (it.premier.taille() != items.taille()) {
            continue;
        }

        auto trouve = true;

        for (auto i = 0; i < items.taille(); ++i) {
            if (it.premier[i] != items[i]) {
                trouve = false;
                break;
            }
        }

        if (!trouve) {
            continue;
        }

        return it.second;
    }

    return nullptr;
}

void Monomorphisations::ajoute(const tableau_items &items, NoeudExpression *noeud)
{
    monomorphisations->ajoute({items, noeud});
}

int64_t Monomorphisations::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += monomorphisations->taille() *
                (taille_de(NoeudExpression *) + taille_de(tableau_items));

    POUR (*monomorphisations.verrou_lecture()) {
        résultat += it.premier.taille() * (taille_de(ItemMonomorphisation));
    }

    return résultat;
}

int Monomorphisations::taille() const
{
    return monomorphisations->taille();
}

int Monomorphisations::nombre_items_max() const
{
    int n = 0;

    POUR (*monomorphisations.verrou_lecture()) {
        if (it.premier.taille() > n) {
            n = it.premier.taille();
        }
    }

    return n;
}

void Monomorphisations::imprime(std::ostream &os) const
{
    Enchaineuse enchaineuse;
    imprime(enchaineuse);
    enchaineuse.imprime_dans_flux(os);
}

void Monomorphisations::imprime(Enchaineuse &os, int indentations) const
{
    auto monomorphisations_ = monomorphisations.verrou_lecture();
    if (monomorphisations_->taille() == 0) {
        os << "Il n'y a aucune monomorphisation connue !\n";
        return;
    }

    auto nombre_monomorphisations = monomorphisations_->taille();

    if (nombre_monomorphisations) {
        os << chaine_indentations(indentations) << "Une monomorphisation connue :\n";
    }
    else {
        os << chaine_indentations(indentations) << "Les monomorphisations connues sont :\n";
    }

    POUR (*monomorphisations_) {
        for (auto i = 0; i < it.premier.taille(); ++i) {
            os << chaine_indentations(indentations + 1) << it.premier[i] << '\n';
        }

        nombre_monomorphisations--;
        if (nombre_monomorphisations) {
            os << chaine_indentations(indentations + 1) << "-------------------\n";
        }
    }
}
