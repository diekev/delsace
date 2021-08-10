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

#include "espace_de_travail.hh"
#include "metaprogramme.hh"
#include "typage.hh"

static constexpr auto CYCLES_MAXIMUM = 1000;

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
    if (attente_est_bloquee()) {
        return true;
    }

    auto attendue = unite_attendue();
    while (attendue) {
        if (attendue->attente_est_bloquee()) {
            return true;
        }
        attendue = attendue->unite_attendue();
    }

    return false;
}

bool UniteCompilation::attente_est_bloquee() const
{
    if (m_attente.est<AttenteSurType>()) {
        auto p = espace->phase_courante();
        return p >= PhaseCompilation::PARSAGE_TERMINE && cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.est<AttenteSurSymbole>()) {
        auto p = espace->phase_courante();
        return p >= PhaseCompilation::PARSAGE_TERMINE && cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        auto p = espace->phase_courante();
        return p >= PhaseCompilation::PARSAGE_TERMINE && cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.est<AttenteSurOperateur>()) {
        auto p = espace->phase_courante();
        return p >= PhaseCompilation::PARSAGE_TERMINE && cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        /* À FAIRE : vérifie que le métaprogramme est en cours d'exécution ? */
        return false;
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        auto p = espace->phase_courante();
        return p >= PhaseCompilation::PARSAGE_TERMINE && cycle > CYCLES_MAXIMUM;
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return false;
    }

    return false;
}

kuri::chaine UniteCompilation::commentaire() const
{
    if (m_attente.est<AttenteSurType>()) {
        auto type_attendu = m_attente.type();
        return enchaine("(type) ", chaine_type(type_attendu));
    }

    if (m_attente.est<AttenteSurSymbole>()) {
        return enchaine("(symbole) ", m_attente.symbole()->ident->nom);
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        return enchaine("(decl) ", m_attente.declaration()->ident->nom);
    }

    if (m_attente.est<AttenteSurOperateur>()) {
        return enchaine("opérateur ", m_attente.operateur()->lexeme->chaine);
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = m_attente.metaprogramme();
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

    if (m_attente.est<AttenteSurRI>()) {
        auto ri_attendue = *m_attente.ri();
        if (ri_attendue == nullptr) {
            return "RI, mais la RI ne fut pas générée !";
        }

        if (ri_attendue->est_fonction()) {
            auto fonction = static_cast<AtomeFonction *>(ri_attendue);
            if (fonction->decl) {
                auto decl = fonction->decl;
                if (decl->ident) {
                    return enchaine("RI de ", fonction->decl->ident->nom);
                }
                /* Utilisation du lexème, par exemple pour les opérateurs. */
                return enchaine("RI de ", fonction->decl->lexeme->chaine);
            }
            return enchaine("RI d'une fonction inconnue");
        }

        if (ri_attendue->est_globale()) {
            auto globale = static_cast<AtomeGlobale *>(ri_attendue);
            if (globale->ident) {
                return enchaine("RI de la globale ", globale->ident->nom);
            }
            return enchaine("RI d'une globale anonyme");
        }

        return enchaine("RI de quelque chose inconnue");
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        return enchaine("(interface kuri) ", m_attente.interface_kuri()->nom);
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return "message";
    }

    if (m_attente.est<AttenteSurLexage>()) {
        return "lexage fichier";
    }

    if (m_attente.est<AttenteSurParsage>()) {
        return "parsage fichier";
    }

    if (m_attente.est<AttenteSurChargement>()) {
        return "chargement fichier";
    }

    return "ERREUR COMPILATRICE";
}

UniteCompilation *UniteCompilation::unite_attendue() const
{
    if (m_attente.est<AttenteSurType>()) {
        auto type_attendu = m_attente.type();

        if (!type_attendu) {
            return nullptr;
        }

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

    if (m_attente.est<AttenteSurSymbole>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        return m_attente.declaration()->unite;
    }

    if (m_attente.est<AttenteSurOperateur>()) {
        return m_attente.operateur()->unite;
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = m_attente.metaprogramme();
        // À FAIRE(gestion) : le métaprogramme attend sur l'unité de la fonction
        // il nous faudra sans doute une raison pour l'attente (RI, CODE, etc.).
        return metaprogramme_attendu->fonction->unite;
    }

    if (m_attente.est<AttenteSurRI>()) {
        auto ri_attendue = *m_attente.ri();
        if (ri_attendue && ri_attendue->est_fonction()) {
            auto fonction = static_cast<AtomeFonction *>(ri_attendue);
            if (fonction->decl) {
                return fonction->decl->unite;
            }
        }
        return nullptr;
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurParsage>()) {
        return nullptr;
    }

    assert(!m_attente.est_valide());
    return nullptr;
}

void UniteCompilation::rapporte_erreur() const
{
    if (m_attente.est<AttenteSurSymbole>()) {
        espace->rapporte_erreur(m_attente.symbole(),
                                "Trop de cycles : arrêt de la compilation sur un symbole inconnu");
    }
    else if (m_attente.est<AttenteSurDeclaration>()) {
        auto decl = m_attente.declaration();
        auto unite_decl = decl->unite;
        auto erreur = espace->rapporte_erreur(
            decl,
            "Je ne peux pas continuer la compilation car une déclaration ne peut être typée.");

        // À FAIRE : ne devrait pas arriver
        if (unite_decl) {
            erreur.ajoute_message("Note : l'unité de compilation est dans cette état :\n")
                .ajoute_message(chaine_attentes_recursives(this))
                .ajoute_message("\n");
        }
    }
    else if (m_attente.est<AttenteSurType>()) {
        auto site = noeud;
        if (site->est_corps_fonction()) {
            auto corps = site->comme_corps_fonction();
            site = corps->arbre_aplatis[index_courant];
        }

        espace
            ->rapporte_erreur(site,
                              "Je ne peux pas continuer la compilation car je n'arrive "
                              "pas à déterminer un type pour l'expression",
                              erreur::Genre::TYPE_INCONNU)
            .ajoute_message("Note : le type attendu est ")
            .ajoute_message(chaine_type(m_attente.type()))
            .ajoute_message("\n")
            .ajoute_message("Note : l'unité de compilation est dans cette état :\n")
            .ajoute_message(chaine_attentes_recursives(this))
            .ajoute_message("\n");
    }
    else if (m_attente.est<AttenteSurInterfaceKuri>()) {
        espace
            ->rapporte_erreur(noeud,
                              "Trop de cycles : arrêt de la compilation car une "
                              "déclaration attend sur une interface de Kuri")
            .ajoute_message(
                "Note : l'interface attendue est ", m_attente.interface_kuri()->nom, "\n");
    }
    else if (m_attente.est<AttenteSurOperateur>()) {
        auto operateur_attendu = m_attente.operateur();
        if (operateur_attendu->genre == GenreNoeud::OPERATEUR_BINAIRE) {
            auto expression_operation = static_cast<NoeudExpressionBinaire *>(operateur_attendu);
            auto type1 = expression_operation->operande_gauche->type;
            auto type2 = expression_operation->operande_droite->type;
            espace
                ->rapporte_erreur(operateur_attendu,
                                  "Je ne peux pas continuer la compilation car je "
                                  "n'arrive pas à déterminer quel opérateur appeler.",
                                  erreur::Genre::TYPE_INCONNU)
                .ajoute_message("Le type à gauche de l'opérateur est ")
                .ajoute_message(chaine_type(type1))
                .ajoute_message("\nLe type à droite de l'opérateur est ")
                .ajoute_message(chaine_type(type2))
                .ajoute_message("\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
                .ajoute_conseil("Si vous voulez performer une opération sur des types "
                                "non-communs, vous pouvez définir vos propres opérateurs avec "
                                "la syntaxe suivante :\n\nopérateur op :: fonc (a: type1, b: "
                                "type2) -> type_retour\n{\n\t...\n}\n");
        }
        else {
            auto expression_operation = static_cast<NoeudExpressionUnaire *>(operateur_attendu);
            auto type = expression_operation->operande->type;
            espace
                ->rapporte_erreur(operateur_attendu,
                                  "Je ne peux pas continuer la compilation car je "
                                  "n'arrive pas à déterminer quel opérateur appeler.",
                                  erreur::Genre::TYPE_INCONNU)
                .ajoute_message("\nLe type à droite de l'opérateur est ")
                .ajoute_message(chaine_type(type))
                .ajoute_message("\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
                .ajoute_conseil("Si vous voulez performer une opération sur des types "
                                "non-communs, vous pouvez définir vos propres opérateurs avec "
                                "la syntaxe suivante :\n\nopérateur op :: fonc (a: type) -> "
                                "type_retour\n{\n\t...\n}\n");
        }
    }
    else {
        espace
            ->rapporte_erreur(noeud,
                              "Je ne peux pas continuer la compilation car une unité est "
                              "bloqué dans un cycle")
            .ajoute_message("\nNote : l'unité est dans l'état : ")
            .ajoute_message(chaine_attentes_recursives(this))
            .ajoute_message("\n");
    }
}

kuri::chaine chaine_attentes_recursives(UniteCompilation const *unite)
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

void UniteCompilation::marque_prete_si_attente_resolue()
{
    if (est_prete()) {
        return;
    }

    if (m_attente.est<AttenteSurType>()) {
        if ((m_attente.type()->drapeaux & TYPE_FUT_VALIDE) != 0) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurSymbole>()) {
        auto p = espace->phase_courante();
        // À FAIRE : granularise ceci pour ne pas tenter de recompiler quelque chose
        // si le symbole ne fut pas encore défini (par exemple en utilisant un ensemble de symboles
        // définis depuis le dernier ajournement, dans GestionnaireCode::cree_taches).
        if (p < PhaseCompilation::PARSAGE_TERMINE) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        auto declaration_attendue = m_attente.declaration();
        if (declaration_attendue->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            if (declaration_attendue->est_entete_fonction() &&
                declaration_attendue->ident == ID::cree_contexte) {
                /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il
                 * nous faut le code. */
                mute_attente(
                    Attente::sur_ri(&declaration_attendue->comme_entete_fonction()->atome));
            }
            else {
                marque_prete();
            }
        }
        return;
    }

    /* À FAIRE(gestion) : détermine comment détecter la disponibilité d'un opérateur.
     *
     * Il y a deux cas spéciaux, en dehors de la définition par l'utilisateur d'un opérateur :
     * - les énumérations, et
     * - les types de bases (hors tableaux, etc.).
     *
     * Il faudra sans doute ajouter un noeud syntaxique pour contenir les données des
     * opérateurs et créer un tel noeud pour tous les types, qui devra ensuite passer par la
     * validation de code.
     */
    if (m_attente.est<AttenteSurOperateur>()) {
        auto p = espace->phase_courante();
        if (p < PhaseCompilation::PARSAGE_TERMINE) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = m_attente.metaprogramme();
        if (metaprogramme_attendu->fut_execute) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        auto interface_attendue = m_attente.interface_kuri();
        auto decl = espace->interface_kuri->declaration_pour_ident(interface_attendue);
        if (decl && decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            if (decl->ident == ID::cree_contexte) {
                /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il
                 * nous faut le code. */
                mute_attente(Attente::sur_ri(&decl->atome));
            }
            else {
                marque_prete();
            }
        }
        return;
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return;
    }

    if (m_attente.est<AttenteSurChargement>()) {
        auto fichier_attendu = m_attente.fichier_a_charger();
        if (fichier_attendu->donnees_constantes->fut_charge) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurLexage>()) {
        auto fichier_attendu = m_attente.fichier_a_lexer();
        if (fichier_attendu->donnees_constantes->fut_lexe) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurParsage>()) {
        auto fichier_attendu = m_attente.fichier_a_parser();
        if (fichier_attendu->fut_parse) {
            marque_prete();
        }
        return;
    }

    if (m_attente.est<AttenteSurRI>()) {
        auto ri_attendue = m_attente.ri();
        if (*ri_attendue && (*ri_attendue)->ri_generee) {
            marque_prete();
        }
        return;
    }
}
