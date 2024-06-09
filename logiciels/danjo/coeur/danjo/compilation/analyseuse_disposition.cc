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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "analyseuse_disposition.h"

#include <iostream>

#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "assembleuse_disposition.h"

// #define DEBOGUE_ANALYSEUR

namespace danjo {

/* ************************************************************************** */

bool est_identifiant_controle(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::ENTIER:
        case id_morceau::DECIMAL:
        case id_morceau::ETIQUETTE:
        case id_morceau::LISTE:
        case id_morceau::ENUM:
        case id_morceau::CASE:
        case id_morceau::CHAINE:
        case id_morceau::FICHIER_ENTREE:
        case id_morceau::FICHIER_SORTIE:
        case id_morceau::COULEUR:
        case id_morceau::VECTEUR:
        case id_morceau::COURBE_COULEUR:
        case id_morceau::COURBE_VALEUR:
        case id_morceau::RAMPE_COULEUR:
        case id_morceau::TEXTE:
        case id_morceau::LISTE_MANIP:
            return true;
        default:
            return false;
    }
}

bool est_identifiant_propriete(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::INFOBULLE:
        case id_morceau::MIN:
        case id_morceau::MAX:
        case id_morceau::VALEUR:
        case id_morceau::ATTACHE:
        case id_morceau::PRECISION:
        case id_morceau::PAS:
        case id_morceau::ITEMS:
        case id_morceau::METADONNEE:
        case id_morceau::ICONE:
        case id_morceau::FILTRES:
        case id_morceau::SUFFIXE:
        case id_morceau::ACTIVABLE:
        case id_morceau::ANIMABLE:
            return true;
        default:
            return false;
    }
}

static auto valeurs_possibles = dls::cree_dico(
    dls::paire{id_morceau::ENTIER,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::SUFFIXE,
                            id_morceau::ANIMABLE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::DECIMAL,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::SUFFIXE,
                            id_morceau::PRECISION,
                            id_morceau::ANIMABLE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::VECTEUR,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::SUFFIXE,
                            id_morceau::ANIMABLE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::COULEUR,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::COURBE_COULEUR,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::ANIMABLE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::COURBE_VALEUR,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::ANIMABLE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::RAMPE_COULEUR,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::MIN,
                            id_morceau::MAX,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::CASE,
               dls::magasin(id_morceau::ATTACHE, id_morceau::INFOBULLE, id_morceau::VALEUR)},
    dls::paire{id_morceau::ENUM,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ITEMS,
                            id_morceau::NOM,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::CHAINE,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::TEXTE,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::FICHIER_ENTREE,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::FILTRES,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::FICHIER_SORTIE,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::FILTRES,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::LISTE_MANIP,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::LISTE,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::ETIQUETTE,
               dls::magasin(id_morceau::ATTACHE, id_morceau::INFOBULLE, id_morceau::VALEUR)},
    dls::paire{id_morceau::ACTION,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ICONE,
                            id_morceau::METADONNEE,
                            id_morceau::ACTIVABLE)},
    dls::paire{id_morceau::BOUTON,
               dls::magasin(id_morceau::ATTACHE,
                            id_morceau::INFOBULLE,
                            id_morceau::VALEUR,
                            id_morceau::ICONE,
                            id_morceau::METADONNEE,
                            id_morceau::ACTIVABLE)});

bool valideuse_propriete::cherche_magasin(id_morceau id)
{
    auto const &magasin_valeur = valeurs_possibles.trouve(id);

    if (magasin_valeur.est_finie()) {
        return false;
    }

    this->magasin = &magasin_valeur.front().second;

    return this->magasin != nullptr;
}

bool valideuse_propriete::est_propriete_valide(id_morceau id)
{
    if (this->magasin == nullptr) {
        return false;
    }

    return this->magasin->possede(id);
}

/* ************************************************************************** */

AnalyseuseDisposition::AnalyseuseDisposition(lng::tampon_source const &tampon,
                                             dls::tableau<DonneesMorceaux> &identifiants)
    : base_analyseuse(tampon, identifiants)
{
}

void AnalyseuseDisposition::installe_assembleur(AssembleurDisposition *assembleur)
{
    m_assembleur = assembleur;
}

void AnalyseuseDisposition::lance_analyse(std::ostream &os)
{
    if (m_identifiants.est_vide()) {
        return;
    }

    m_position = 0;

    if (m_assembleur == nullptr) {
        throw "Un assembleur doit être installé avant de générer l'entreface !";
    }

    if (est_identifiant(id_morceau::DISPOSITION)) {
        analyse_script_disposition();
    }
    else if (est_identifiant(id_morceau::MENU)) {
        analyse_script_menu();
    }
    else if (est_identifiant(id_morceau::BARRE_OUTILS)) {
        analyse_script_barre_outils();
    }
    else {
        /* avance car lance_erreur dépend de position qui est retourne
         * m_position - 1, et nous sommes à la position 0. */
        avance();
        lance_erreur("Le script ne commence par aucun mot-clé connu !");
    }
}

void AnalyseuseDisposition::analyse_script_disposition()
{
#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << " début\n";
#endif

    if (!requiers_identifiant(id_morceau::DISPOSITION)) {
        lance_erreur("Le script doit commencer par 'disposition' !");
    }

    if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
        lance_erreur("Attendu le nom de la disposition après 'disposition' !");
    }

    m_assembleur->nom_disposition(dls::chaine{donnees().chaine});

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après le nom de la disposition !");
    }

    /* Ajout d'une disposition par défaut. */
    m_assembleur->ajoute_disposition(id_morceau::COLONNE);

    analyse_disposition();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermante à la fin du script !");
    }

#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseDisposition::analyse_script_menu()
{
    if (!requiers_identifiant(id_morceau::MENU)) {
        lance_erreur("Attendu la déclaration menu !");
    }

    if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
        lance_erreur("Attendu le nom du menu après 'menu' !");
    }

    const auto nom = donnees().chaine;

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après le nom du menu !");
    }

    m_assembleur->ajoute_menu(dls::chaine{nom});

    analyse_menu();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermante à la fin du script !");
    }

    m_assembleur->sort_menu();
}

void AnalyseuseDisposition::analyse_script_barre_outils()
{
    if (!requiers_identifiant(id_morceau::BARRE_OUTILS)) {
        lance_erreur("Attendu la déclaration 'barre_outils' !");
    }

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après 'barre_outils' !");
    }

    m_assembleur->ajoute_barre_outils();

    analyse_barre_outils();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermante à la fin du script !");
    }
}

void AnalyseuseDisposition::analyse_barre_outils()
{
    if (est_identifiant(id_morceau::ACTION)) {
        analyse_action();
    }
    else {
        return;
    }

    analyse_barre_outils();
}

void AnalyseuseDisposition::analyse_menu()
{
    if (est_identifiant(id_morceau::ACTION)) {
        analyse_action();
    }
    else if (est_identifiant(id_morceau::MENU)) {
        analyse_script_menu();
    }
    else if (est_identifiant(id_morceau::SEPARATEUR)) {
        m_assembleur->ajoute_separateur();
        avance();
    }
    else {
        return;
    }

    analyse_menu();
}

void AnalyseuseDisposition::analyse_action()
{
    if (!requiers_identifiant(id_morceau::ACTION)) {
        lance_erreur("Attendu la déclaration d'une action !");
    }

    if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
        lance_erreur("Attendu l'ouverture d'une paranthèse après 'action' !");
    }

    /*auto action =*/m_assembleur->ajoute_action();

    if (!m_valideuse.cherche_magasin(id_morceau::ACTION)) {
        lance_erreur("Impossible de trouver le magasin de propriétés pour l'action");
    }

#if 0
    while (!fini()) {
        if (!est_identifiant_propriete(identifiant_courant())) {
            lance_erreur("Attendu la déclaration d'un identifiant !");
        }

        const auto identifiant_propriete = identifiant_courant();

        avance();
        auto est_drapeaux = dls::outils::est_element(
            identifiant_propriete, id_morceau::ANIMABLE, id_morceau::ACTIVABLE);
        if (!est_drapeaux && !requiers_identifiant(id_morceau::EGAL)) {
            lance_erreur("Attendu la déclaration '=' !");
        }

        switch (identifiant_propriete) {
            default:
            {
                lance_erreur("Propriété invalide pour une action");
                break;
            }

            case id_morceau::ATTACHE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                action->etablie_attache();
                break;
            }
            case id_morceau::INFOBULLE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                action->etablie_infobulle();
                break;
            }
            case id_morceau::VALEUR:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                action->etablie_valeur();
                break;
            }
            case id_morceau::ICONE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                action->etablie_icone();
                break;
            }
            case id_morceau::METADONNEE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                action->etablie_metadonnee();
                break;
            }
            case id_morceau::ACTIVABLE:
            {
                break;
            }
        }
    }
#endif

    analyse_propriete(id_morceau::ACTION);

    if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
        lance_erreur("Attendu la fermeture d'une paranthèse !");
    }
}

void AnalyseuseDisposition::analyse_disposition()
{
    if (est_identifiant(id_morceau::LIGNE)) {
        analyse_ligne();
    }
    else if (est_identifiant(id_morceau::COLONNE)) {
        analyse_colonne();
    }
    else if (est_identifiant(id_morceau::DOSSIER)) {
        analyse_dossier();
    }
    else if (est_identifiant(id_morceau::BOUTON)) {
        analyse_bouton();
    }
    else if (est_identifiant_controle(identifiant_courant())) {
        analyse_controle();
    }
    else {
        return;
    }

    analyse_disposition();
}

void AnalyseuseDisposition::analyse_ligne()
{
#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << '\n';
#endif

    if (!requiers_identifiant(id_morceau::LIGNE)) {
        lance_erreur("Attendu la déclaration 'ligne' !");
    }

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après la déclaration 'ligne' !");
    }

    m_assembleur->ajoute_disposition(id_morceau::LIGNE);

    analyse_disposition();

    m_assembleur->sors_disposition();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur(
            "Attendu une accolade fermante après la déclaration du contenu de la 'ligne' !");
    }

    analyse_disposition();
}

void AnalyseuseDisposition::analyse_colonne()
{
#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << '\n';
#endif

    if (!requiers_identifiant(id_morceau::COLONNE)) {
        lance_erreur("Attendu la déclaration 'colonne' !");
    }

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après la déclaration 'colonne' !");
    }

    m_assembleur->ajoute_disposition(id_morceau::COLONNE);

    analyse_disposition();

    m_assembleur->sors_disposition();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur(
            "Attendu une accolade fermante après la déclaration du contenu de la 'colonne' !");
    }

    analyse_disposition();
}

void AnalyseuseDisposition::analyse_dossier()
{
    if (!requiers_identifiant(id_morceau::DOSSIER)) {
        lance_erreur("Attendu la déclaration 'dossier' !");
    }

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après la déclaration 'dossier' !");
    }

    m_assembleur->ajoute_dossier();

    analyse_onglet();

    m_assembleur->finalise_dossier();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur(
            "Attendu une accolade fermante après la déclaration du contenu de 'dossier' !");
    }

    analyse_disposition();
}

void AnalyseuseDisposition::analyse_onglet()
{
    /* Soit le dossier est vide, soit il n'y a plus d'onglets. */
    if (!est_identifiant(id_morceau::ONGLET)) {
        return;
    }

    if (!requiers_identifiant(id_morceau::ONGLET)) {
        lance_erreur("Attendu la déclaration 'onglet' !");
    }

    if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
        lance_erreur("Attendu une chaîne de caractère après 'onglet' !");
    }

    const auto &nom = donnees().chaine;

    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouvrante après le nom de 'onglet' !");
    }

    m_assembleur->ajoute_onglet(dls::chaine{nom});

    analyse_disposition();

    m_assembleur->finalise_onglet();

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur(
            "Attendu une accolade fermante après la déclaration du contenu de 'onglet' !");
    }

    analyse_onglet();
}

void AnalyseuseDisposition::analyse_controle()
{
#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << '\n';
#endif

    if (!est_identifiant_controle(identifiant_courant())) {
        lance_erreur("Attendu la déclaration d'un contrôle !");
    }

    const auto identifiant_controle = identifiant_courant();

    avance();

    if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
        lance_erreur("Attendu une parenthèse ouvrante après la déclaration du contrôle !");
    }

    m_assembleur->ajoute_controle(identifiant_controle);

    if (!m_valideuse.cherche_magasin(identifiant_controle)) {
        lance_erreur("Impossible de trouver le magasin de propriétés pour le controle");
    }

    analyse_propriete(identifiant_controle);

    if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
        lance_erreur(
            "Attendu une parenthèse fermante après la déclaration du contenu du contrôle !");
    }

    m_assembleur->finalise_controle();
}

void AnalyseuseDisposition::analyse_bouton()
{
    if (!requiers_identifiant(id_morceau::BOUTON)) {
        lance_erreur("Attendu la déclaration d'un bouton !");
    }

    if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
        lance_erreur("Attendu une parenthèse ouvrante après la déclaration 'bouton' !");
    }

    m_assembleur->ajoute_bouton();

    if (!m_valideuse.cherche_magasin(id_morceau::BOUTON)) {
        lance_erreur("Impossible de trouver le magasin de propriétés pour le bouton");
    }

    analyse_propriete(id_morceau::BOUTON);

    if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
        lance_erreur(
            "Attendu une parenthèse fermante après la déclaration du contenu du 'bouton' !");
    }
}

void AnalyseuseDisposition::analyse_propriete(id_morceau type_controle)
{
#ifdef DEBOGUE_ANALYSEUR
    std::cout << __func__ << '\n';
#endif

    if (!est_identifiant_propriete(identifiant_courant())) {
        lance_erreur("Attendu la déclaration d'un identifiant !");
    }

    const auto identifiant_propriete = identifiant_courant();

    if (!m_valideuse.est_propriete_valide(identifiant_propriete)) {
        std::cerr << "'Attention : propriété '" << chaine_identifiant(identifiant_propriete)
                  << "' inutile pour type '" << chaine_identifiant(type_controle) << "'\n";
    }

    avance();

    auto est_drapeaux = dls::outils::est_element(
        identifiant_propriete, id_morceau::ANIMABLE, id_morceau::ACTIVABLE);

    if (!est_drapeaux && !requiers_identifiant(id_morceau::EGAL)) {
        lance_erreur("Attendu la déclaration '=' !");
    }

    if (identifiant_propriete == id_morceau::ITEMS) {
        analyse_liste_item();
    }
    else {
        auto valeur_negative = false;

        switch (identifiant_propriete) {
            default:
                break;
            case id_morceau::ANIMABLE:
            case id_morceau::ACTIVABLE:
            {
                if (!dls::outils::est_element(
                        identifiant_propriete, id_morceau::BOUTON, id_morceau::ACTION)) {
                    m_assembleur->propriete_controle(identifiant_propriete, "");
                }

                break;
            }
            case id_morceau::VALEUR:
            {
                switch (type_controle) {
                    default:
                        break;
                    case id_morceau::ENTIER:
                    case id_morceau::DECIMAL:
                    {
                        if (est_identifiant(id_morceau::MOINS)) {
                            valeur_negative = true;
                            avance();
                        }

                        if (!est_identifiant(id_morceau::NOMBRE_ENTIER) &&
                            !est_identifiant(id_morceau::NOMBRE_REEL)) {
                            lance_erreur("Attendu un nombre !");
                        }

                        avance();

                        break;
                    }
                    case id_morceau::FICHIER_ENTREE:
                    case id_morceau::FICHIER_SORTIE:
                    case id_morceau::CHAINE:
                    case id_morceau::TEXTE:
                    case id_morceau::ETIQUETTE:
                    case id_morceau::ENUM:
                    case id_morceau::LISTE:
                    case id_morceau::COULEUR:
                    case id_morceau::VECTEUR:
                    case id_morceau::BOUTON:
                    case id_morceau::ACTION:
                    {
                        if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                            lance_erreur("Attendu une chaine littérale !");
                        }

                        break;
                    }
                    case id_morceau::CASE:
                    {
                        if (!est_identifiant(id_morceau::VRAI) &&
                            !est_identifiant(id_morceau::FAUX)) {
                            lance_erreur("Attendu l'identifiant vrai ou faux !");
                        }

                        avance();
                        break;
                    }
                }

                break;
            }
            case id_morceau::ATTACHE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
                    lance_erreur("Attendu une chaine de caractère !");
                }

                break;
            }
            case id_morceau::PRECISION:
            case id_morceau::MIN:
            case id_morceau::MAX:
            case id_morceau::PAS:
            {
                if (est_identifiant(id_morceau::MOINS)) {
                    valeur_negative = true;
                    avance();
                }

                if (!est_identifiant(id_morceau::NOMBRE_ENTIER) &&
                    !est_identifiant(id_morceau::NOMBRE_REEL)) {
                    lance_erreur("Attendu un nombre !");
                }

                avance();

                break;
            }
            case id_morceau::INFOBULLE:
            case id_morceau::FILTRES:
            case id_morceau::METADONNEE:
            case id_morceau::ICONE:
            case id_morceau::SUFFIXE:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale !");
                }

                break;
            }
        }

        if (type_controle == id_morceau::BOUTON) {
            m_assembleur->propriete_bouton(identifiant_propriete, dls::chaine{donnees().chaine});
        }
        else if (type_controle == id_morceau::ACTION) {
            m_assembleur->propriete_action(identifiant_propriete, dls::chaine{donnees().chaine});
        }
        else {
            const auto valeur = valeur_negative ? "-" + dls::chaine{donnees().chaine} :
                                                  dls::chaine{donnees().chaine};

            m_assembleur->propriete_controle(identifiant_propriete, valeur);
        }
    }

    if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
        /* Fin des identifiants propriétés. */
        recule();
        return;
    }

    analyse_propriete(type_controle);
}

void AnalyseuseDisposition::analyse_liste_item()
{
    if (!requiers_identifiant(id_morceau::CROCHET_OUVRANT)) {
        lance_erreur("Attendu un crochet ouvert !");
    }

    analyse_item();

    if (!requiers_identifiant(id_morceau::CROCHET_FERMANT)) {
        lance_erreur("Attendu un crochet fermé !");
    }
}

/* { nom="", valeur=""}, */
void AnalyseuseDisposition::analyse_item()
{
    if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
        lance_erreur("Attendu une accolade ouverte !");
    }

    if (!requiers_identifiant(id_morceau::NOM)) {
        lance_erreur("Attendu la déclaration 'nom' après l'accolade ouverte !");
    }

    if (!requiers_identifiant(id_morceau::EGAL)) {
        lance_erreur("Attendu la déclaration '=' !");
    }

    if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
        lance_erreur("Attendu une chaîne de caractère après '=' !");
    }

    const auto nom = donnees().chaine;

    if (!requiers_identifiant(id_morceau::VIRGULE)) {
        lance_erreur("Attendu une virgule !");
    }

    if (!requiers_identifiant(id_morceau::VALEUR)) {
        lance_erreur("Attendu la déclaration 'nom' après l'accolade ouverte !");
    }

    if (!requiers_identifiant(id_morceau::EGAL)) {
        lance_erreur("Attendu la déclaration '=' !");
    }

    if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
        lance_erreur("Attendu une chaîne de caractère après '=' !");
    }

    const auto valeur = donnees().chaine;

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermée !");
    }

    m_assembleur->ajoute_item_liste(dls::chaine{nom}, dls::chaine{valeur});

    if (!requiers_identifiant(id_morceau::VIRGULE)) {
        /* Fin des identifiants item. */
        recule();
        return;
    }

    analyse_item();
}

} /* namespace danjo */
