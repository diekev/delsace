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

#include <danjo/danjo.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#pragma GCC diagnostic pop

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"

#include "coeur/mikisa.h"

#include "../editrice_noeud.h"

/* ************************************************************************** */

VueEditeurNoeud::VueEditeurNoeud(Mikisa *mikisa,
		EditriceGraphe *base,
		QWidget *parent)
	: QGraphicsView(parent)
	, m_mikisa(mikisa)
	, m_base(base)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_mikisa->repondant_commande();

	auto gestionnaire = m_mikisa->gestionnaire_entreface;

	auto texte_entree = danjo::contenu_fichier("entreface/menu_ajouter_noeud_composite.jo");
	m_menu_ajout_noeud_composite = gestionnaire->compile_menu_entrerogeable(donnees, texte_entree.c_str());

	texte_entree = danjo::contenu_fichier("entreface/menu_ajouter_noeud_objet.jo");
	m_menu_ajout_noeud_objet = gestionnaire->compile_menu_entrerogeable(donnees, texte_entree.c_str());

	texte_entree = danjo::contenu_fichier("entreface/menu_ajouter_noeud_point3d.jo");
	m_menu_ajout_noeud_point3d = gestionnaire->compile_menu_entrerogeable(donnees, texte_entree.c_str());

	texte_entree = danjo::contenu_fichier("entreface/menu_ajouter_noeud_scene.jo");
	m_menu_ajout_noeud_scene = gestionnaire->compile_menu_entrerogeable(donnees, texte_entree.c_str());
}

VueEditeurNoeud::~VueEditeurNoeud()
{
	delete m_menu_ajout_noeud_composite;
	delete m_menu_ajout_noeud_objet;
	delete m_menu_ajout_noeud_point3d;
	delete m_menu_ajout_noeud_scene;
}

void VueEditeurNoeud::keyPressEvent(QKeyEvent *event)
{
	m_base->rend_actif();

	if (event->key() == Qt::Key_Tab) {
		switch (m_mikisa->contexte) {
			case GRAPHE_COMPOSITE:
				m_menu_ajout_noeud_composite->popup(QCursor::pos());
				break;
			case GRAPHE_SCENE:
				m_menu_ajout_noeud_scene->popup(QCursor::pos());
				break;
			case GRAPHE_OBJET:
				m_menu_ajout_noeud_objet->popup(QCursor::pos());
				break;
			case GRAPHE_MAILLAGE:
				m_menu_ajout_noeud_point3d->popup(QCursor::pos());
				break;
		}
	}
	else {
		DonneesCommande donnees;
		donnees.cle = event->key();

		m_mikisa->repondant_commande()->appele_commande("graphe", donnees);
	}
}

void VueEditeurNoeud::wheelEvent(QWheelEvent *event)
{
	m_base->rend_actif();

	DonneesCommande donnees;
	donnees.double_clique = true;
	donnees.souris = Qt::MiddleButton;
	donnees.x = static_cast<float>(event->angleDelta().x());
	donnees.y = static_cast<float>(event->angleDelta().y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa->repondant_commande()->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseMoveEvent(QMouseEvent *event)
{
	if (event->button() != 0) {
		m_base->rend_actif();
	}

	const auto position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.souris = Qt::LeftButton;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa->repondant_commande()->ajourne_commande_modale(donnees);
}

void VueEditeurNoeud::mousePressEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	/* À FAIRE : menu contextuel */
	//			m_gestionnaire->ajourne_menu("Éditeur Noeud");
	//			m_menu_contexte->popup(event->globalPos());

	const auto position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.souris = event->button();
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa->repondant_commande()->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseDoubleClickEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	const auto position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.double_clique = true;
	donnees.souris = Qt::LeftButton;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa->repondant_commande()->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseReleaseEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	const auto position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa->repondant_commande()->acheve_commande_modale(donnees);
}

bool VueEditeurNoeud::focusNextPrevChild(bool /*next*/)
{
	/* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
	 * sur les éléments enfants du conteneur de contrôles. */
	return false;
}
