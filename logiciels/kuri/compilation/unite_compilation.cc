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

#include "unite_compilation.hh"

#include "biblinternes/structures/ensemble.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "metaprogramme.hh"
#include "typage.hh"

static constexpr auto CYCLES_MAXIMUM = 10;

const char *chaine_etat_unite(UniteCompilation::Etat etat)
{
#define ENUMERE_ETAT_UNITE_EX(etat)                                                               \
    case UniteCompilation::Etat::etat:                                                            \
        return #etat;
    switch (etat) {
        ENUMERE_ETATS_UNITE
    }
#undef ENUMERE_ETAT_UNITE_EX

    return "erreur";
}

std::ostream &operator<<(std::ostream &os, UniteCompilation::Etat etat)
{
    os << chaine_etat_unite(etat);
    return os;
}

void UniteCompilation::marque_attente(Attente attente)
{
    if (attente.attend_sur_type) {
        attend_sur_type(attente.attend_sur_type);
    }
    else if (attente.attend_sur_interface_kuri) {
        attend_sur_interface_kuri(attente.attend_sur_interface_kuri);
    }
}

bool UniteCompilation::est_bloquee() const
{
    switch (etat()) {
        case UniteCompilation::Etat::ATTEND_SUR_TYPE:
        {
            return false;
        }
        case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
        {
            return false;
        }
        case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
        {
            return cycle > CYCLES_MAXIMUM;
        }
        case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
        {
            return cycle > CYCLES_MAXIMUM;
        }
        case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
        {
            return false;
        }
        case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
        {
            return false;
        }
        case UniteCompilation::Etat::PRETE:
        {
            return cycle > CYCLES_MAXIMUM;
        }
    }

    return false;
}

kuri::chaine UniteCompilation::commentaire() const
{
    switch (etat()) {
        case UniteCompilation::Etat::ATTEND_SUR_TYPE:
        {
            return chaine_type(type_attendu);
        }
        case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
        {
            return declaration_attendue->ident->nom;
        }
        case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
        {
            return symbole_attendu->ident->nom;
        }
        case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
        {
            return enchaine("opérateur ", operateur_attendu->lexeme->chaine);
        }
        case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
        {
            auto resultat = Enchaineuse();
            resultat << "métaprogramme";

            if (metaprogramme_attendu->corps_texte) {
                resultat << " #corps_texte pour ";

                if (metaprogramme_attendu->corps_texte_pour_fonction) {
                    resultat << metaprogramme->corps_texte_pour_fonction->ident->nom;
                }
                else if (metaprogramme_attendu->corps_texte_pour_structure) {
                    resultat << metaprogramme_attendu->corps_texte_pour_structure->ident->nom;
                }
                else {
                    resultat << " ERREUR COMPILATRICE";
                }
            }
            else {
                resultat << " " << metaprogramme_attendu;
            }

            return resultat.chaine();
        }
        case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
        {
            return fonction_interface_attendue;
        }
        case UniteCompilation::Etat::PRETE:
        {
            return "prête";
        }
    }

    return "";
}

UniteCompilation *UniteCompilation::unite_attendue() const
{
    switch (etat()) {
        case UniteCompilation::Etat::ATTEND_SUR_TYPE:
        {
            if (type_attendu->est_structure()) {
                auto type_structure = type_attendu->comme_structure();
                return type_structure->decl->unite;
            }
            else if (type_attendu->est_union()) {
                auto type_union = type_attendu->comme_union();
                return type_union->decl->unite;
            }
            else if (type_attendu->est_enum()) {
                auto type_enum = type_attendu->comme_enum();
                return type_enum->decl->unite;
            }
            else if (type_attendu->est_erreur()) {
                auto type_erreur = type_attendu->comme_erreur();
                return type_erreur->decl->unite;
            }

            return nullptr;
        }
        case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
        {
            return declaration_attendue->unite;
        }
        case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
        {
            return nullptr;
        }
        case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
        {
            return metaprogramme_attendu->unite;
        }
        case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
        case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
        case UniteCompilation::Etat::PRETE:
        {
            return nullptr;
        }
    }

    return nullptr;
}

kuri::chaine chaine_attentes_recursives(UniteCompilation *unite)
{
    if (!unite) {
        return "    L'unité est nulle !\n";
    }

    Enchaineuse fc;

    auto attendue = unite->unite_attendue();
    auto commentaire = unite->commentaire();

    if (!attendue) {
        fc << "    " << commentaire << " est bloquée !\n";
    }

    dls::ensemble<UniteCompilation *> unite_visite;

    while (attendue) {
        if (attendue->etat() == UniteCompilation::Etat::PRETE) {
            fc << "    " << commentaire << " est prête !\n";
            break;
        }

        if (unite_visite.trouve(attendue) != unite_visite.fin()) {
            fc << "    erreur : dépendance cyclique !\n";
            break;
        }

        fc << "    " << commentaire << " attend sur ";
        commentaire = attendue->commentaire();
        fc << commentaire << '\n';

        unite_visite.insere(attendue);

        attendue = attendue->unite_attendue();
    }

    return fc.chaine();
}
