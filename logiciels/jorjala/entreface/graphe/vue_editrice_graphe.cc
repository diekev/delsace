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

#include "vue_editrice_graphe.h"

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/jorjala.hh"

#include "../gestion_entreface.hh"
#include "../editrice_noeud.h"

/* ************************************************************************** */

VueEditeurNoeud::VueEditeurNoeud(JJL::Jorjala &jorjala,
		EditriceGraphe *base,
		QWidget *parent)
	: QGraphicsView(parent)
	, m_jorjala(jorjala)
	, m_base(base)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

VueEditeurNoeud::~VueEditeurNoeud()
{
}

void VueEditeurNoeud::keyPressEvent(QKeyEvent *event)
{
    m_base->rend_actif();

    if (event->key() == Qt::Key_Tab) {
        auto menu = menu_pour_graphe();

        if (!menu) {
            return;
        }

        menu->popup(QCursor::pos());
	}
#if 0
	else {
		DonneesCommande donnees;
		donnees.cle = event->key();

        repondant_commande(m_jorjala)->appele_commande("graphe", donnees);
	}
#endif
}

void VueEditeurNoeud::wheelEvent(QWheelEvent *event)
{
    gere_molette_souris(m_jorjala, event, "graphe");
}

void VueEditeurNoeud::mouseMoveEvent(QMouseEvent *event)
{
	if (event->button() != 0) {
		m_base->rend_actif();
	}

	auto const position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.souris = Qt::LeftButton;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(m_jorjala)->ajourne_commande_modale(donnees);
}

void VueEditeurNoeud::mousePressEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	/* À FAIRE : menu contextuel */
	//			m_gestionnaire->ajourne_menu("Éditeur Noeud");
	//			m_menu_contexte->popup(event->globalPos());

	auto const position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.souris = event->button();
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(m_jorjala)->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_base->rend_actif();

    auto const position = mapToScene(event->pos());

    DonneesCommande donnees;
    donnees.double_clique = true;
    donnees.souris = Qt::LeftButton;
    donnees.x = static_cast<float>(position.x());
    donnees.y = static_cast<float>(position.y());
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(m_jorjala)->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseReleaseEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	auto const position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(m_jorjala)->acheve_commande_modale(donnees);
}

bool VueEditeurNoeud::focusNextPrevChild(bool /*next*/)
{
	/* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
	 * sur les éléments enfants du conteneur de contrôles. */
    return false;
}

static std::string texte_danjo_pour_menu_catégorisation(JJL::CategorisationNoeuds catégorisation, const std::string &identifiant)
{
    std::stringstream ss;

    ss << "menu \"" << identifiant << "\" {\n";

    for (auto catégorie : catégorisation.catégories()) {
        ss << "  menu \"" << catégorie.nom().vers_std_string() << "\" {\n";

        for (auto noeud : catégorie.noeuds()) {
            ss << "    action(valeur=\"" << noeud.vers_std_string() << "\"; attache=ajouter_noeud; métadonnée=\"" << noeud.vers_std_string() << "\")\n";
        }

        ss << "  }\n";
    }

    ss << "}\n";

    return ss.str();
}

QMenu *VueEditeurNoeud::menu_pour_graphe()
{
    auto graphe = m_jorjala.graphe();
    auto catégorisation = graphe.catégorisation_noeuds();
    auto identifiant = graphe.identifiant_graphe().vers_std_string();
    if (catégorisation == nullptr) {
        return nullptr;
    }

    auto menu_existant = m_menus.find(identifiant);
    if (menu_existant != m_menus.end()) {
        return menu_existant->second;
    }

    auto texte = texte_danjo_pour_menu_catégorisation(catégorisation, identifiant);
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);

    auto menu = gestionnaire->compile_menu_texte(donnees, texte);
    if (!menu) {
        return nullptr;
    }

    m_menus[identifiant] = menu;

    return menu;
}
