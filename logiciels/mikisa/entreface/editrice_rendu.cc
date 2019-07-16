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

#include "editrice_rendu.h"

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QHBoxLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/repondant_commande.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/evenement.h"
#include "coeur/mikisa.h"

EditriceRendu::EditriceRendu(Mikisa &mikisa, QWidget *parent)
	: BaseEditrice(mikisa, parent)
	, m_widget(new QWidget())
	, m_conteneur_disposition(new QWidget())
	, m_scroll(new QScrollArea())
	, m_disposition_widget(new QVBoxLayout(m_widget))
{
	m_widget->setSizePolicy(m_frame->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_main_layout->addWidget(m_scroll);

	m_disposition_widget->addWidget(m_conteneur_disposition);
}

void EditriceRendu::ajourne_etat(int evenement)
{
	if (evenement != type_evenement::rafraichissement) {
		return;
	}

	auto const &texte = dls::contenu_fichier("entreface/disposition_rendu.jo");

	if (texte.est_vide()) {
		return;
	}

	danjo::DonneesInterface donnees{};
	donnees.manipulable = &m_manipulable;
	donnees.conteneur = this;
	donnees.repondant_bouton = m_mikisa.repondant_commande();

	auto disposition = m_mikisa.gestionnaire_entreface->compile_entreface(donnees, texte.c_str());

	if (m_conteneur_disposition->layout()) {
		/* Qt ne permet d'extrait la disposition d'un widget que si celle-ci est
		 * assignée à un autre widget. Donc pour détruire la disposition précédente
		 * nous la reparentons à un widget temporaire qui la détruira dans son
		 * destructeur. */
		QWidget temp;
		temp.setLayout(m_conteneur_disposition->layout());
	}

	m_conteneur_disposition->setLayout(disposition);
}

void EditriceRendu::ajourne_manipulable()
{
	m_mikisa.nom_calque_sortie = m_manipulable.evalue_chaine("nom_calque");
	m_mikisa.chemin_sortie = m_manipulable.evalue_chaine("chemin");
}

void EditriceRendu::obtiens_liste(
		dls::chaine const &/*attache*/,
		dls::tableau<dls::chaine> &chaines)
{
	/* À FAIRE */
	chaines.pousse("image");
}
