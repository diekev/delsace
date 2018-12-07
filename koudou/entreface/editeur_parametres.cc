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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
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

#include "coeur/koudou.h"
#include "coeur/moteur_rendu.h"

#include "outils.h"

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

VueParametres::VueParametres(Koudou *koudou)
	: m_koudou(koudou)
{
	ProprieteEnumerante enum_resolution;
	enum_resolution.ajoute("HD 720 (1280x720)", RES_720);
	enum_resolution.ajoute("HD 1080 (1920×1080)", RES_1080);
	enum_resolution.ajoute("2K FLAT (1998×1080)", RES_2K_FLAT);
	enum_resolution.ajoute("2K Digital Cinema (2048×1080)", RES_2K_DC);
	enum_resolution.ajoute("4K UHDTV (3840×2160)", RES_4K_UHDTV);
	enum_resolution.ajoute("4K Digital Cinema (4096×2160)", RES_4K_DC);
	enum_resolution.ajoute("8K UHDTV (7680×4320)", RES_8K_UHDTV);

	ajoute_propriete("resolution", "Résolution image", TypePropriete::ENUM);
	etablie_valeur_enum(enum_resolution);

	ajoute_propriete("largeur_carreau", "Largeur carreau", TypePropriete::INT);
	etablie_valeur_int_defaut(32);
	etablie_min_max(1, 100);

	ajoute_propriete("hauteur_carreau", "Hauteur carreau", TypePropriete::INT);
	etablie_valeur_int_defaut(32);
	etablie_min_max(1, 100);

	ajoute_propriete("echantillons", "Échantillons", TypePropriete::INT);
	etablie_valeur_int_defaut(32);
	etablie_min_max(1, 100);

	ajoute_propriete("rebonds", "Rebonds", TypePropriete::INT);
	etablie_valeur_int_defaut(5);
	etablie_min_max(1, 100);
}

void VueParametres::ajourne_donnees()
{
	m_koudou->parametres_rendu.nombre_echantillons = static_cast<unsigned>(evalue_int("echantillons"));
	m_koudou->parametres_rendu.nombre_rebonds = static_cast<unsigned>(evalue_int("rebonds"));
	m_koudou->parametres_rendu.largeur_carreau = static_cast<unsigned>(evalue_int("largeur_carreau"));
	m_koudou->parametres_rendu.hauteur_carreau = static_cast<unsigned>(evalue_int("hauteur_carreau"));

	const auto resolution = static_cast<unsigned>(evalue_enum("resolution"));

	if (m_koudou->parametres_rendu.resolution != resolution) {
		const auto largeur = numero7::math::Largeur(RESOLUTIONS[resolution][0]);
		const auto hauteur = numero7::math::Hauteur(RESOLUTIONS[resolution][1]);

		m_koudou->moteur_rendu->pointeur_pellicule()->redimensionne(hauteur, largeur);
	}

	m_koudou->parametres_rendu.resolution = resolution;
}

bool VueParametres::ajourne_proprietes()
{
	ajourne_valeur_int("echantillons", static_cast<int>(m_koudou->parametres_rendu.nombre_echantillons));
	ajourne_valeur_int("rebonds", static_cast<int>(m_koudou->parametres_rendu.nombre_rebonds));
	ajourne_valeur_int("resolution", static_cast<int>(m_koudou->parametres_rendu.resolution));
	ajourne_valeur_int("largeur_carreau", static_cast<int>(m_koudou->parametres_rendu.largeur_carreau));
	ajourne_valeur_int("hauteur_carreau", static_cast<int>(m_koudou->parametres_rendu.hauteur_carreau));

	return true;
}

/* ************************************************************************** */

EditeurParametres::EditeurParametres(Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
	, m_vue(new VueParametres(koudou))
	, m_widget(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QGridLayout(m_widget))
	, m_assembleur_controles(m_glayout)
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
}

void EditeurParametres::ajourne_etat(int /*evenement*/)
{
	m_vue->ajourne_proprietes();
	cree_controles(m_assembleur_controles, m_vue);
	m_assembleur_controles.setContext(this, SLOT(ajourne_vue()));
}

void EditeurParametres::ajourne_vue()
{
	m_vue->ajourne_donnees();
}
