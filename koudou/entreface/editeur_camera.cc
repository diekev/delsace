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

#include "editeur_camera.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "coeur/evenement.h"
#include "coeur/koudou.h"

#include "outils.h"

/* ************************************************************************** */

VueCamera::VueCamera(ProjectiveCamera *camera)
	: m_camera(camera)
{
	ajoute_propriete("position", "Position", TypePropriete::VEC3);
	etablie_valeur_vec3_defaut(dls::math::vec3f{0.0f, 0.0f, 0.0f});
	etablie_min_max(-10.0f, 10.0f);

	ajoute_propriete("rotation", "Rotation", TypePropriete::VEC3);
	etablie_valeur_vec3_defaut(dls::math::vec3f{0.0f, 0.0f, 0.0f});
	etablie_min_max(0.0f, 360.0f);

	ajoute_propriete("ouverture", "Ouverture obturateur", TypePropriete::FLOAT);
	etablie_valeur_float_defaut(0.0f);
	etablie_min_max(0.0f, 1.0f);

	ajoute_propriete("fermeture", "Fermeture obturateur", TypePropriete::FLOAT);
	etablie_valeur_float_defaut(1.0f);
	etablie_min_max(0.0f, 1.0f);

	ajoute_propriete("rayon", "Rayon lentille", TypePropriete::FLOAT);
	etablie_valeur_float_defaut(0.0f);
	etablie_min_max(0.0f, 1.0f);

	ajoute_propriete("distance", "Distance focale", TypePropriete::FLOAT);
	etablie_valeur_float_defaut(0.0f);
	etablie_min_max(0.0f, 1.0f);

	ajoute_propriete("champs_de_vue", "Champs de vue", TypePropriete::FLOAT);
	etablie_valeur_float_defaut(60.0f);
	etablie_min_max(0.0f, 360.0f);
}

void VueCamera::ajourne_donnees()
{
#ifdef NOUVELLE_CAMERA
	const auto position = evalue_vec3("position");
	const auto rotation = evalue_vec3("rotation");

	m_camera->position(dls::math::vec3d(position.x, position.y, position.z));
	m_camera->rotation(dls::math::vec3d(rotation.x * POIDS_DEG_RAD, rotation.y * POIDS_DEG_RAD, rotation.z * POIDS_DEG_RAD));
	m_camera->distance_focale(evalue_float("distance"));
	m_camera->champs_de_vue(evalue_float("champs_de_vue"));
	m_camera->rayon_lentille(evalue_float("rayon"));
	m_camera->ouverture_obturateur(evalue_float("ouverture"));
	m_camera->fermeture_obturateur(evalue_float("fermeture"));

	m_camera->ajourne();
#endif
}

bool VueCamera::ajourne_proprietes()
{
#ifdef NOUVELLE_CAMERA
	ajourne_valeur_float("distance", m_camera->distance_focale());
	ajourne_valeur_float("champs_de_vue", m_camera->champs_de_vue());
	ajourne_valeur_float("rayon", m_camera->rayon_lentille());
	ajourne_valeur_float("ouverture", m_camera->ouverture_obturateur());
	ajourne_valeur_float("fermeture", m_camera->fermeture_obturateur());

	const auto position = m_camera->position();
	const auto rotation = m_camera->rotation();

	ajourne_valeur_vec3("position", dls::math::vec3f(position.x, position.y, position.z));
	ajourne_valeur_vec3("rotation", dls::math::vec3f(rotation.x * POIDS_RAD_DEG, rotation.y * POIDS_RAD_DEG, rotation.z * POIDS_RAD_DEG));
#endif

	return true;
}

/* ************************************************************************** */

EditeurCamera::EditeurCamera(Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
	, m_widget(new QWidget())
	, m_scroll(new QScrollArea())
	, m_glayout(new QGridLayout(m_widget))
	, m_assembleur_controles(m_glayout)
{
#ifdef NOUVELLE_CAMERA
	m_vue = new VueCamera(koudou->parametres_rendu.camera);
#else
	m_vue = new VueCamera(nullptr);
#endif

	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);
}

EditeurCamera::~EditeurCamera()
{
	delete m_vue;
}

void EditeurCamera::ajourne_etat(int evenement)
{
	/* À FAIRE : trouver une meilleure manière de rafraîchir l'entreface et les
	 * données du programme quand un bouton de l'entreface change. */
	if (evenement != type_evenement::rafraichissement) {
		return;
	}

	m_vue->ajourne_proprietes();
	cree_controles(m_assembleur_controles, m_vue);
	m_assembleur_controles.setContext(this, SLOT(ajourne_camera()));
}

void EditeurCamera::ajourne_camera()
{
	m_vue->ajourne_donnees();
	m_koudou->notifie_auditeurs(type_evenement::camera | type_evenement::modifie);
}
