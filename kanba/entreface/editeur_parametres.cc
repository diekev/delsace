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

#include "editeur_parametres.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "coeur/kanba.h"

/* ************************************************************************** */

enum {
	RES_720 = 0,
	RES_1080,
	RES_2K_FLAT,
	RES_2K_DC,
	RES_4K_UHDTV,
	RES_4K_DC,
	RES_8K_UHDTV,
};

static const int RESOLUTIONS[][2] = {
	{ 1280,  720 },
	{ 1920, 1080 },
	{ 1998, 1080 },
	{ 2048, 1080 },
	{ 3840, 2160 },
	{ 4096, 2160 },
	{ 7680, 4320 },
};

VueParametres::VueParametres(Kanba *kanba)
	: m_kanba(kanba)
{
#if 0
	danjo::ProprieteEnumerante enum_resolution;
	enum_resolution.ajoute("HD 720 (1280x720)", RES_720);
	enum_resolution.ajoute("HD 1080 (1920×1080)", RES_1080);
	enum_resolution.ajoute("2K FLAT (1998×1080)", RES_2K_FLAT);
	enum_resolution.ajoute("2K Digital Cinema (2048×1080)", RES_2K_DC);
	enum_resolution.ajoute("4K UHDTV (3840×2160)", RES_4K_UHDTV);
	enum_resolution.ajoute("4K Digital Cinema (4096×2160)", RES_4K_DC);
	enum_resolution.ajoute("8K UHDTV (7680×4320)", RES_8K_UHDTV);

	ajoute_propriete("resolution", "Résolution image", danjo::TypePropriete::ENUM);
	etablie_valeur_enum(enum_resolution);

	ajoute_propriete("echantillons", "Échantillons", danjo::TypePropriete::ENTIER);
	etablie_valeur_int_defaut(32);
	etablie_min_max(1, 100);

	ajoute_propriete("rebonds", "Rebonds", danjo::TypePropriete::ENTIER);
	etablie_valeur_int_defaut(5);
	etablie_min_max(1, 100);
#endif
}

void VueParametres::ajourne_donnees()
{
#if 0
	m_kanba->parametres_rendu.nombre_echantillons = evalue_int("echantillons");
	m_kanba->parametres_rendu.nombre_rebonds = evalue_int("rebonds");

	const auto resolution = evalue_enum("resolution");

	if (m_kanba->parametres_rendu.resolution != resolution) {
		const auto largeur = numero7::math::Largeur(RESOLUTIONS[resolution][0]);
		const auto hauteur = numero7::math::Hauteur(RESOLUTIONS[resolution][1]);

		m_kanba->moteur_rendu->pointeur_pellicule()->redimensionne(hauteur, largeur);
	}

	m_kanba->parametres_rendu.resolution = resolution;
#endif
}

bool VueParametres::ajourne_proprietes()
{
#if 0
	ajourne_valeur_int("echantillons", m_kanba->parametres_rendu.nombre_echantillons);
	ajourne_valeur_int("rebonds", m_kanba->parametres_rendu.nombre_rebonds);
	ajourne_valeur_int("resolution", m_kanba->parametres_rendu.resolution);
#endif

	return true;
}

/* ************************************************************************** */

EditeurParametres::EditeurParametres(Kanba *kanba, QWidget *parent)
	: BaseEditrice(*kanba, parent)
	, m_vue(new VueParametres(kanba))
	, m_widget(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QGridLayout(m_widget))
{
	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);
}

EditeurParametres::~EditeurParametres()
{
	delete m_vue;
	delete m_scroll;
	delete m_glayout;
}

void EditeurParametres::ajourne_etat(int /*evenement*/)
{
	m_vue->ajourne_proprietes();
//	cree_controles(m_assembleur_controles, m_vue);
//	m_assembleur_controles.setContext(this, SLOT(ajourne_vue()));
}

void EditeurParametres::ajourne_manipulable()
{
	m_vue->ajourne_donnees();
}
