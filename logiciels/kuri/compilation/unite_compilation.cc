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

const char *chaine_rainson_d_etre(RaisonDEtre raison_d_etre)
{
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine)                                              \
    case RaisonDEtre::Genre:                                                                      \
        return chaine;
    switch (raison_d_etre) {
        ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
    }
#undef ENUMERE_RAISON_D_ETRE_EX
    return "ceci ne devrait pas arriver";
}

std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre)
{
    return os << chaine_rainson_d_etre(raison_d_etre);
}

bool UniteCompilation::est_bloquee() const
{
    if (m_attente.attend_sur_type) {
        return false;
    }

    if (m_attente.attend_sur_symbole) {
        /* À FAIRE : vérifie que tous les fichiers ont été chargés, lexés, et parsés. */
        return cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.attend_sur_declaration) {
        /* À FAIRE : vérifie que tous les fichiers ont été chargés, lexés, et parsés. */
        return false;
    }

    if (m_attente.attend_sur_operateur) {
        /* À FAIRE : vérifie que tous les fichiers ont été chargés, lexés, et parsés. */
        return cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.attend_sur_metaprogramme) {
        /* À FAIRE : vérifie que le métaprogramme est en cours d'exécution ? */
        return false;
    }

    if (m_attente.attend_sur_interface_kuri) {
        /* À FAIRE : vérifie que tous les fichiers ont été chargés, lexés, et parsés. */
        return false;
    }

    if (m_attente.attend_sur_message) {
        return false;
    }

    return false;
}

kuri::chaine UniteCompilation::commentaire() const
{
    if (m_attente.attend_sur_type) {
        auto type_attendu = m_attente.attend_sur_type;
        return chaine_type(type_attendu);
    }

    if (m_attente.attend_sur_symbole) {
        return m_attente.attend_sur_symbole->ident->nom;
    }

    if (m_attente.attend_sur_declaration) {
        return m_attente.attend_sur_declaration->ident->nom;
    }

    if (m_attente.attend_sur_operateur) {
        return enchaine("opérateur ", m_attente.attend_sur_operateur->lexeme->chaine);
    }

    if (m_attente.attend_sur_metaprogramme) {
        auto metaprogramme_attendu = m_attente.attend_sur_metaprogramme;
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

    if (m_attente.attend_sur_interface_kuri) {
        return m_attente.attend_sur_interface_kuri;
    }

    if (m_attente.attend_sur_message) {
        return "message";
    }

    return "ERREUR COMPILATRICE";
}

UniteCompilation *UniteCompilation::unite_attendue() const
{
    if (m_attente.attend_sur_type) {
        auto type_attendu = m_attente.attend_sur_type;
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
        assert(!m_attente.est_valide());
        return nullptr;
    }

    if (m_attente.attend_sur_symbole) {
        return nullptr;
    }

    if (m_attente.attend_sur_declaration) {
        return m_attente.attend_sur_declaration->unite;
    }

    if (m_attente.attend_sur_operateur) {
        return m_attente.attend_sur_declaration->unite;
    }

    if (m_attente.attend_sur_metaprogramme) {
        auto metaprogramme_attendu = m_attente.attend_sur_metaprogramme;
        return metaprogramme_attendu->unite;
    }

    if (m_attente.attend_sur_interface_kuri) {
        return nullptr;
    }

    if (m_attente.attend_sur_message) {
        return nullptr;
    }

    assert(!m_attente.est_valide());
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
        if (attendue->est_prete()) {
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
