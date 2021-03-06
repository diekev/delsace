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
#include "biblinternes/outils/fichier.hh"

#include "coeur/jorjala.hh"
#include "operatrices/operatrices_cycles.hh"

#include "../editrice_noeud.h"

/* ************************************************************************** */

VueEditeurNoeud::VueEditeurNoeud(Jorjala &jorjala,
		EditriceGraphe *base,
		QWidget *parent)
	: QGraphicsView(parent)
	, m_jorjala(jorjala)
	, m_base(base)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	danjo::DonneesInterface donnees{};
	donnees.manipulable = nullptr;
	donnees.conteneur = nullptr;
	donnees.repondant_bouton = m_jorjala.repondant_commande();

	auto gestionnaire = m_jorjala.gestionnaire_entreface;

	m_menu_ajout_noeud_composite = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_ajouter_noeud_composite.jo");
	m_menu_ajout_noeud_objet = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_ajouter_noeud_objet.jo");
	m_menu_ajout_noeud_detail = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_ajouter_noeud_detail.jo");
	m_menu_ajout_noeud_rendu = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_ajouter_noeud_rendu.jo");
	m_menu_graphe_objet = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_graphe_objet.jo");
	m_menu_ajout_noeud_simulation = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_ajouter_noeud_simulation.jo");
	m_menu_graphe_composite = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_graphe_composite.jo");
	m_menu_graphe_nuanceur = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_graphe_nuanceur.jo");
	m_menu_graphe_rendu = gestionnaire->compile_menu_fichier(donnees, "entreface/menu_graphe_rendu.jo");

	auto texte_menu_cycles = genere_menu_noeuds_cycles();
	m_menu_ajout_noeud_cycles = gestionnaire->compile_menu_texte(donnees, texte_menu_cycles);
}

VueEditeurNoeud::~VueEditeurNoeud()
{
	delete m_menu_graphe_rendu;
	delete m_menu_graphe_nuanceur;
	delete m_menu_graphe_composite;
	delete m_menu_ajout_noeud_rendu;
	delete m_menu_ajout_noeud_composite;
	delete m_menu_ajout_noeud_objet;
	delete m_menu_ajout_noeud_detail;
	delete m_menu_ajout_noeud_cycles;
	delete m_menu_graphe_objet;
	delete m_menu_ajout_noeud_simulation;
}

void VueEditeurNoeud::keyPressEvent(QKeyEvent *event)
{
	m_base->rend_actif();

	if (event->key() == Qt::Key_Tab) {
		switch (m_jorjala.graphe->type) {
			case type_graphe::RACINE_COMPOSITE:
				m_menu_graphe_composite->popup(QCursor::pos());
				break;
			case type_graphe::RACINE_NUANCEUR:
				m_menu_graphe_nuanceur->popup(QCursor::pos());
				break;
			case type_graphe::RACINE_RENDU:
				m_menu_graphe_rendu->popup(QCursor::pos());
				break;
			case type_graphe::RENDU:
				m_menu_ajout_noeud_rendu->popup(QCursor::pos());
				break;
			case type_graphe::COMPOSITE:
				m_menu_ajout_noeud_composite->popup(QCursor::pos());
				break;
			case type_graphe::RACINE_OBJET:
				m_menu_graphe_objet->popup(QCursor::pos());
				break;
			case type_graphe::OBJET:
				m_menu_ajout_noeud_objet->popup(QCursor::pos());
				break;
			case type_graphe::DETAIL:
				m_menu_ajout_noeud_detail->popup(QCursor::pos());
				break;
			case type_graphe::CYCLES:
				m_menu_ajout_noeud_cycles->popup(QCursor::pos());
				break;
			case type_graphe::INVALIDE:
				break;
		}
	}
	else {
		DonneesCommande donnees;
		donnees.cle = event->key();

		m_jorjala.repondant_commande()->appele_commande("graphe", donnees);
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

	m_jorjala.repondant_commande()->appele_commande("graphe", donnees);
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

	m_jorjala.repondant_commande()->ajourne_commande_modale(donnees);
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

	m_jorjala.repondant_commande()->appele_commande("graphe", donnees);
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

	m_jorjala.repondant_commande()->appele_commande("graphe", donnees);
}

void VueEditeurNoeud::mouseReleaseEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	auto const position = mapToScene(event->pos());

	DonneesCommande donnees;
	donnees.x = static_cast<float>(position.x());
	donnees.y = static_cast<float>(position.y());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_jorjala.repondant_commande()->acheve_commande_modale(donnees);
}

bool VueEditeurNoeud::focusNextPrevChild(bool /*next*/)
{
	/* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
	 * sur les éléments enfants du conteneur de contrôles. */
	return false;
}
