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

#include "danjo.h"

#include <fstream>
#include <iostream>

#include <QBoxLayout>
#include <QMenu>
#include <QToolBar>

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/fichier.hh"

#include "controles/action.h"
#include "controles_proprietes/controle_propriete.h"

#include "compilation/analyseuse_disposition.h"
#include "compilation/analyseuse_logique.h"
#include "compilation/assembleuse_disposition.h"
#include "compilation/decoupeuse.h"

#include "dialogue.h"
#include "erreur.h"
#include "manipulable.h"

namespace danjo {

GestionnaireInterface::~GestionnaireInterface()
{
    for (const auto &donnees : m_menus) {
        auto menu = donnees.second;

        for (auto &action : menu->actions()) {
            delete action;
        }

        delete menu;
    }

    for (auto &barre_outils : m_barres_outils) {
        for (auto &action : barre_outils->actions()) {
            delete action;
        }

        delete barre_outils;
    }
}

void GestionnaireInterface::parent_dialogue(QWidget *p)
{
    m_parent_dialogue = p;
}

void GestionnaireInterface::ajourne_menu(const dls::chaine &nom)
{
    auto menu = pointeur_menu(nom);

    if (menu == nullptr) {
        return;
    }

    for (auto &pointeur : menu->actions()) {
        Action *action = dynamic_cast<Action *>(pointeur);

        if (action) {
            action->evalue_predicat();
        }
    }
}

/* À FAIRE : passe un script. */
void GestionnaireInterface::recree_menu(const dls::chaine &nom,
                                        const dls::tableau<DonneesAction> &donnees_actions)
{
    auto menu = pointeur_menu(nom);

    if (menu == nullptr) {
        return;
    }

    menu->clear();

    for (const auto &donnees : donnees_actions) {
        Action *action = new Action;
        action->etablie_valeur(donnees.nom);
        action->etablie_attache(donnees.attache);
        action->etablie_metadonnee(donnees.metadonnee);
        action->installe_repondant(donnees.repondant_bouton);

        menu->addAction(action);
    }
}

QMenu *GestionnaireInterface::compile_menu_texte(DonneesInterface &donnees,
                                                 dls::chaine const &texte)
{
    AssembleurDisposition assembleuse(donnees);

    auto texte_ = texte;
    auto tampon = lng::tampon_source(std::move(texte_));
    auto decoupeuse = Decoupeuse(tampon);

    try {
        decoupeuse.decoupe();

        auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
        analyseuse.installe_assembleur(&assembleuse);
        analyseuse.lance_analyse(std::cerr);
    }
    catch (const ErreurFrappe &e) {
        std::cerr << e.quoi();
        return nullptr;
    }
    catch (const ErreurSyntactique &e) {
        std::cerr << e.quoi();
        return nullptr;
    }

    for (const auto &pair : assembleuse.donnees_menus()) {
        m_menus.insere(pair);
    }

    return assembleuse.menu();
}

QMenu *GestionnaireInterface::compile_menu_fichier(DonneesInterface &donnees,
                                                   dls::chaine const &fichier)
{
    return compile_menu_texte(donnees, dls::contenu_fichier(fichier));
}

QBoxLayout *GestionnaireInterface::compile_entreface_texte(DonneesInterface &donnees,
                                                           dls::chaine const &texte,
                                                           int temps)
{
    if (donnees.manipulable == nullptr) {
        m_controles.efface();
        return nullptr;
    }

    AssembleurDisposition assembleuse(donnees);

    auto texte_ = texte;
    auto tampon = lng::tampon_source(std::move(texte_));
    auto decoupeuse = Decoupeuse(tampon);

    try {
        decoupeuse.decoupe();

        auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
        analyseuse.installe_assembleur(&assembleuse);
        analyseuse.lance_analyse(std::cerr);
    }
    catch (const ErreurFrappe &e) {
        std::cerr << e.quoi();
        m_controles.efface();
        return nullptr;
    }
    catch (const ErreurSyntactique &e) {
        std::cerr << e.quoi();
        m_controles.efface();
        return nullptr;
    }

    for (const auto &pair : assembleuse.donnees_menus()) {
        m_menus.insere(pair);
    }

    auto disposition = assembleuse.disposition();

    /* dans le cas où un script est vide, mais nous aurions tout de même des
     * propriétés à dessiner, ajout une disposition pour ne pas crasher */
    if (disposition == nullptr) {
        assembleuse.ajoute_disposition(id_morceau::COLONNE);
        disposition = assembleuse.disposition();
    }

    assembleuse.cree_controles_proprietes_extra();

    disposition->addStretch();

    auto nom = assembleuse.nom_disposition();

    m_dispositions.insere({nom, disposition});
    m_controles = assembleuse.controles;

    return disposition;
}

QBoxLayout *GestionnaireInterface::compile_entreface_fichier(DonneesInterface &donnees,
                                                             dls::chaine const &fichier,
                                                             int temps)
{
    return compile_entreface_texte(donnees, dls::contenu_fichier(fichier), temps);
}

void GestionnaireInterface::ajourne_entreface(Manipulable *manipulable)
{
    if (manipulable == nullptr) {
        return;
    }

    for (auto paire : m_controles) {
        auto controle = paire.second;
        auto prop = manipulable->propriete(paire.first);

        if (prop == nullptr) {
            continue;
        }

        controle->setEnabled(prop->est_visible());
    }
}

void GestionnaireInterface::initialise_entreface_texte(Manipulable *manipulable,
                                                       dls::chaine const &texte)
{
    if (manipulable == nullptr) {
        return;
    }

    DonneesInterface donnees{};
    donnees.manipulable = manipulable;
    AssembleurDisposition assembleuse(donnees, 0, true);

    auto texte_ = texte;
    auto tampon = lng::tampon_source(std::move(texte_));
    auto decoupeuse = Decoupeuse(tampon);

    try {
        decoupeuse.decoupe();

        auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
        analyseuse.installe_assembleur(&assembleuse);
        analyseuse.lance_analyse(std::cerr);
    }
    catch (const ErreurFrappe &e) {
        std::cerr << e.quoi();
    }
    catch (const ErreurSyntactique &e) {
        std::cerr << e.quoi();
    }
}

void GestionnaireInterface::initialise_entreface_fichier(Manipulable *manipulable,
                                                         dls::chaine const &chemin)
{
    if (manipulable == nullptr) {
        return;
    }

    initialise_entreface_texte(manipulable, dls::contenu_fichier(chemin));
}

QMenu *GestionnaireInterface::pointeur_menu(const dls::chaine &nom)
{
    auto iter = m_menus.trouve(nom);

    if (iter == m_menus.fin()) {
        // std::cerr << "Le menu '" << nom << "' est introuvable !\n";
        return nullptr;
    }

    return (*iter).second;
}

QToolBar *GestionnaireInterface::compile_barre_outils_texte(DonneesInterface &donnees,
                                                            dls::chaine const &texte)
{
    AssembleurDisposition assembleur(donnees);

    auto texte_ = texte;
    auto tampon = lng::tampon_source(std::move(texte_));
    auto decoupeuse = Decoupeuse(tampon);

    try {
        decoupeuse.decoupe();

        auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
        analyseuse.installe_assembleur(&assembleur);
        analyseuse.lance_analyse(std::cerr);
    }
    catch (const ErreurFrappe &e) {
        std::cerr << e.quoi();
        return nullptr;
    }
    catch (const ErreurSyntactique &e) {
        std::cerr << e.quoi();
        return nullptr;
    }

    m_barres_outils.ajoute(assembleur.barre_outils());

    return assembleur.barre_outils();
}

QToolBar *GestionnaireInterface::compile_barre_outils_fichier(DonneesInterface &donnees,
                                                              dls::chaine const &fichier)
{
    return compile_barre_outils_texte(donnees, dls::contenu_fichier(fichier));
}

bool GestionnaireInterface::montre_dialogue_texte(DonneesInterface &donnees,
                                                  dls::chaine const &texte)
{
    auto disposition = this->compile_entreface_texte(donnees, texte);

    if (disposition == nullptr) {
        return false;
    }

    auto dialogue = Dialogue(disposition, this->m_parent_dialogue);
    dialogue.show();

    return dialogue.exec() == QDialog::Accepted;
}

bool GestionnaireInterface::montre_dialogue_fichier(DonneesInterface &donnees,
                                                    dls::chaine const &fichier)
{
    return montre_dialogue_texte(donnees, dls::contenu_fichier(fichier));
}

void GestionnaireInterface::ajoute_controle(dls::chaine identifiant, ControlePropriete *controle)
{
    m_controles.insere({identifiant, controle});
}

void GestionnaireInterface::ajourne_controles()
{
    for (auto const &paire : m_controles) {
        auto controle = paire.second;
        auto prop = controle->donne_propriete();
        controle->ajourne_depuis_propriété();
        controle->setEnabled(prop->est_visible());
    }
}

/* ************************************************************************** */

static GestionnaireInterface __gestionnaire;

QBoxLayout *compile_entreface(DonneesInterface &donnees, const char *texte_entree, int temps)
{
    return __gestionnaire.compile_entreface_texte(donnees, texte_entree, temps);
}

QBoxLayout *compile_entreface(DonneesInterface &donnees,
                              const std::filesystem::path &chemin_texte,
                              int temps)
{
    if (donnees.manipulable == nullptr) {
        return nullptr;
    }

    const auto texte_entree = dls::contenu_fichier(chemin_texte.c_str());
    return __gestionnaire.compile_entreface_texte(donnees, texte_entree, temps);
}

QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree)
{
    return __gestionnaire.compile_menu_texte(donnees, texte_entree);
}

void compile_feuille_logique(const char *texte_entree)
{
    auto tampon = lng::tampon_source(texte_entree);
    auto decoupeuse = Decoupeuse(tampon);

    try {
        decoupeuse.decoupe();

        auto analyseuse = AnalyseuseLogique(nullptr, tampon, decoupeuse.morceaux());
        analyseuse.lance_analyse(std::cerr);
    }
    catch (const ErreurFrappe &e) {
        std::cerr << e.quoi();
    }
    catch (const ErreurSyntactique &e) {
        std::cerr << e.quoi();
    }

    /* À FAIRE */
    //	return __gestionnaire.compile_feuille_logique(texte_entree);
}

void initialise_entreface(Manipulable *manipulable, const char *texte_entree)
{
    return __gestionnaire.initialise_entreface_texte(manipulable, texte_entree);
}

bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree)
{
    return __gestionnaire.montre_dialogue_texte(donnees, texte_entree);
}

} /* namespace danjo */
