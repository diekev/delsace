/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "monomorphisations.hh"

#include "arbre_syntaxique/copieuse.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "utilitaires/log.hh"

#include "compilatrice.hh"
#include "contexte.hh"
#include "espace_de_travail.hh"
#include "portee.hh"
#include "typage.hh"
#include "validation_semantique.hh"

kuri::chaine_statique chaine_pour_genre_item(GenreItem genre)
{
    switch (genre) {
        case GenreItem::INDÉFINI:
        {
            return "INDÉFINI";
        }
        case GenreItem::TYPE_DE_DONNÉES:
        {
            return "TYPE_DE_DONNÉES";
        }
        case GenreItem::VALEUR:
        {
            return "VALEUR";
        }
    }
    return "ERREUR_GENRE_ITEM_INCONNU";
}

std::ostream &operator<<(std::ostream &os, const ItemMonomorphisation &item)
{
    os << item.ident->nom << " " << chaine_type(item.type);
    if (item.genre == GenreItem::VALEUR) {
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

kuri::tableau<ItemMonomorphisation, int> Monomorphisations::donne_items_pour(
    NoeudExpression *noeud) const
{
    kuri::tableau<ItemMonomorphisation, int> résultat;

    POUR (*monomorphisations.verrou_lecture()) {
        if (it.second == noeud) {
            résultat = it.premier;
            break;
        }
    }

    return résultat;
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

    if (nombre_monomorphisations == 1) {
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

static NoeudBloc *bloc_constantes_pour(NoeudExpression const *noeud)
{
    if (noeud->est_entête_fonction()) {
        return noeud->comme_entête_fonction()->bloc_constantes;
    }
    if (noeud->est_déclaration_classe()) {
        return noeud->comme_déclaration_classe()->bloc_constantes;
    }
    assert_rappel(false, [&]() {
        dbg() << "[bloc_constantes_pour] Obtenu un noeud de genre " << noeud->genre;
    });
    return nullptr;
}

static std::pair<NoeudExpression *, bool> monomorphise_au_besoin(
    AssembleuseArbre *assembleuse,
    NoeudExpression const *a_copier,
    Monomorphisations *monomorphisations,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto monomorphisation = monomorphisations->trouve_monomorphisation(items_monomorphisation);
    if (monomorphisation) {
        return {monomorphisation, false};
    }

    auto copie = copie_noeud(
        assembleuse, a_copier, a_copier->bloc_parent, OptionsCopieNoeud::AUCUNE);
    auto bloc_constantes = bloc_constantes_pour(copie);

    /* Ajourne les constantes dans le bloc. */
    POUR (items_monomorphisation) {
        auto decl_constante =
            trouve_dans_bloc_seul(bloc_constantes, it.ident)->comme_déclaration_constante();
        decl_constante->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
        decl_constante->drapeaux &= ~(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE |
                                      DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE);
        decl_constante->type = const_cast<Type *>(it.type);

        if (it.genre == GenreItem::VALEUR) {
            decl_constante->valeur_expression = it.valeur;
        }
    }

    monomorphisations->ajoute(items_monomorphisation, copie);
    return {copie, true};
}

std::pair<NoeudDéclarationEntêteFonction *, bool> monomorphise_au_besoin(
    Contexte *contexte,
    NoeudDéclarationEntêteFonction const *decl,
    NoeudExpression *site,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(
        contexte->assembleuse, decl, decl->monomorphisations, std::move(items_monomorphisation));

    auto entête = copie->comme_entête_fonction();

    if (!copie_nouvelle) {
        return {entête, false};
    }

    entête->drapeaux_fonction |= DrapeauxNoeudFonction::EST_MONOMORPHISATION;
    entête->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_POLYMORPHIQUE;
    entête->site_monomorphisation = site;

    /* Supprime les valeurs polymorphiques.
     * À FAIRE : optimise en utilisant un drapeau sur l'entête pour dire que les paramètres
     * contiennent une déclaration de valeur ou de type polymorphique. */
    auto nouveau_params = kuri::tablet<NoeudExpression *, 6>();
    POUR (entête->params) {
        auto decl_constante = trouve_dans_bloc_seul(entête->bloc_constantes, it->ident);
        if (decl_constante) {
            continue;
        }

        nouveau_params.ajoute(it);
    }

    if (nouveau_params.taille() != entête->params.taille()) {
        POUR_INDICE (nouveau_params) {
            static_cast<void>(it);
            entête->params[indice_it] = nouveau_params[indice_it];
        }
        entête->params.redimensionne(int(nouveau_params.taille()));
    }

    auto espace = contexte->espace;
    espace->compilatrice().gestionnaire_code->requiers_typage(espace, entête);
    return {entête, true};
}

NoeudDéclarationClasse *monomorphise_au_besoin(
    Contexte *contexte,
    NoeudDéclarationClasse const *decl_struct,
    NoeudExpression *site,
    kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
{
    auto [copie, copie_nouvelle] = monomorphise_au_besoin(contexte->assembleuse,
                                                          decl_struct,
                                                          decl_struct->monomorphisations,
                                                          std::move(items_monomorphisation));

    auto structure = copie->comme_déclaration_classe();

    if (!copie_nouvelle) {
        return structure;
    }

    structure->est_polymorphe = false;
    structure->est_monomorphisation = true;
    structure->polymorphe_de_base = decl_struct;
    structure->site_monomorphisation = site;

    auto espace = contexte->espace;
    espace->compilatrice().gestionnaire_code->requiers_typage(espace, structure);

    return structure;
}
