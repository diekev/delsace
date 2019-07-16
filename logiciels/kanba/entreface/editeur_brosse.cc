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

#include "editeur_brosse.h"

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QVBoxLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/outils/fichier.hh"

#include "coeur/brosse.h"
#include "coeur/kanba.h"
#include "coeur/melange.h"

static dls::chaine nom_mode_fusion(TypeMelange type_melange)
{
	switch (type_melange) {
		default:
		case TypeMelange::NORMAL:
			return "normal";
		case TypeMelange::ADDITION:
			return "addition";
		case TypeMelange::SOUSTRACTION:
			return "soustraction";
		case TypeMelange::MULTIPLICATION:
			return "multiplication";
		case TypeMelange::DIVISION:
			return "division";
	}
}

static TypeMelange mode_fusion_depuis_nom(dls::chaine const &nom)
{
	if (nom == "normal") {
		return TypeMelange::NORMAL;
	}

	if (nom == "addition") {
		return TypeMelange::ADDITION;
	}

	if (nom == "soustraction") {
		return TypeMelange::SOUSTRACTION;
	}

	if (nom == "multiplication") {
		return TypeMelange::MULTIPLICATION;
	}

	if (nom == "division") {
		return TypeMelange::DIVISION;
	}

	return TypeMelange::NORMAL;
}

VueBrosse::VueBrosse(Kanba *kanba)
	: m_kanba(kanba)
{
	ajoute_propriete("couleur_brosse", danjo::TypePropriete::COULEUR, dls::math::vec4f(1.0f));
	ajoute_propriete("rayon", danjo::TypePropriete::ENTIER, 35);
	ajoute_propriete("opacité", danjo::TypePropriete::DECIMAL, 1.0f);
	ajoute_propriete("mode_fusion", danjo::TypePropriete::ENUM);
}

void VueBrosse::ajourne_donnees()
{
	auto couleur = evalue_couleur("couleur_brosse");
	m_kanba->brosse->couleur = dls::math::vec4f(couleur.r, couleur.v, couleur.b, couleur.a);
	m_kanba->brosse->rayon = evalue_entier("rayon");
	m_kanba->brosse->opacite = evalue_decimal("opacité");
	m_kanba->brosse->mode_fusion = mode_fusion_depuis_nom(evalue_enum("mode_fusion"));
}

bool VueBrosse::ajourne_proprietes()
{
	auto couleur = m_kanba->brosse->couleur;
	valeur_couleur("couleur_brosse", dls::phys::couleur32(couleur.x, couleur.y, couleur.z, couleur.w));
	valeur_decimal("opacité", m_kanba->brosse->opacite);
	valeur_entier("rayon", m_kanba->brosse->rayon);
	valeur_chaine("mode_fusion", nom_mode_fusion(m_kanba->brosse->mode_fusion));

	return true;
}

EditeurBrosse::EditeurBrosse(Kanba *kanba, QWidget *parent)
	: BaseEditrice(*kanba, parent)
	, m_vue(new VueBrosse(kanba))
	, m_widget(new QWidget())
	, m_conteneur_disposition(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QVBoxLayout(m_widget))
{
	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);

	m_glayout->addWidget(m_conteneur_disposition);
}

EditeurBrosse::~EditeurBrosse()
{
	delete m_vue;
	delete m_glayout;
	delete m_scroll;
}

void EditeurBrosse::ajourne_etat(int evenement)
{
	m_vue->ajourne_proprietes();

	danjo::DonneesInterface donnees{};
	donnees.conteneur = this;
	donnees.manipulable = m_vue;
	donnees.repondant_bouton = nullptr;

	auto const contenu_fichier = dls::contenu_fichier("scripts/brosse.jo");
	auto disposition = danjo::compile_entreface(donnees, contenu_fichier.c_str());

	if (m_conteneur_disposition->layout()) {
		QWidget tmp;
		tmp.setLayout(m_conteneur_disposition->layout());
	}

	m_conteneur_disposition->setLayout(disposition);
}

void EditeurBrosse::ajourne_manipulable()
{
	m_vue->ajourne_donnees();
}
