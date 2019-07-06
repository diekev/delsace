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

#include "editeur_material.h"

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
#include "coeur/maillage.h"
#include "coeur/nuanceur.h"
#include "coeur/objet.h"

#include "outils.h"

/* ************************************************************************** */

static auto converti_couleur(Spectre const &spectre)
{
	return dls::math::vec4f(spectre[0], spectre[1], spectre[2], 1.0f);
}

VueMaterial::VueMaterial(Nuanceur *nuaceur)
	: m_persona_diffus(new Persona)
	, m_persona_angle_vue(new Persona)
	, m_persona_reflection(new Persona)
	, m_persona_refraction(new Persona)
	, m_persona_volume(new Persona)
	, m_persona_emission(new Persona)
	, m_nuanceur(nuaceur)
{
	m_persona_diffus->ajoute_propriete("spectre", "Spectre", TypePropriete::COULEUR);
	m_persona_diffus->etablie_min_max(0.0f, 1.0f);

	m_persona_angle_vue->ajoute_propriete("spectre", "Spectre", TypePropriete::COULEUR);
	m_persona_angle_vue->etablie_min_max(0.0f, 1.0f);

	m_persona_refraction->ajoute_propriete("index", "Index de réfraction", TypePropriete::FLOAT);
	m_persona_refraction->etablie_min_max(0.0f, 2.0f);
	m_persona_refraction->etablie_valeur_float_defaut(1.45f);

	m_persona_volume->ajoute_propriete("absorption", "Absorption", TypePropriete::COULEUR);
	m_persona_volume->etablie_min_max(0.0f, 1.0f);
	m_persona_volume->ajoute_propriete("diffusion", "Diffusion", TypePropriete::COULEUR);
	m_persona_volume->etablie_min_max(0.0f, 1.0f);

	m_persona_emission->ajoute_propriete("spectre", "Spectre", TypePropriete::COULEUR);
	m_persona_emission->etablie_min_max(0.0f, 1.0f);
	m_persona_emission->ajoute_propriete("exposition", "Exposition", TypePropriete::FLOAT);
	m_persona_emission->etablie_min_max(0.0f, 32.0f);
}

VueMaterial::~VueMaterial()
{
	delete m_persona_diffus;
	delete m_persona_angle_vue;
	delete m_persona_reflection;
	delete m_persona_refraction;
	delete m_persona_volume;
	delete m_persona_emission;
}

void VueMaterial::nuanceur(Nuanceur *nuanceur)
{
	m_nuanceur = nuanceur;
}

void VueMaterial::ajourne_donnees()
{
	switch (m_nuanceur->type) {
		case TypeNuanceur::DIFFUS:
		{
			auto nuanceur = static_cast<NuanceurDiffus *>(m_nuanceur);
			auto couleur = m_persona_diffus->evalue_couleur("spectre");
			nuanceur->spectre = Spectre::depuis_rgb(couleur.x, couleur.y, couleur.z);
			break;
		}
		case TypeNuanceur::ANGLE_VUE:
		{
			auto nuanceur = static_cast<NuanceurAngleVue *>(m_nuanceur);
			auto couleur = m_persona_angle_vue->evalue_couleur("spectre");
			nuanceur->spectre = Spectre::depuis_rgb(couleur.x, couleur.y, couleur.z);
			break;
		}
		case TypeNuanceur::REFLECTION:
		{
			break;
		}
		case TypeNuanceur::REFRACTION:
		{
			auto nuanceur = static_cast<NuanceurRefraction *>(m_nuanceur);
			auto index = m_persona_refraction->evalue_float("index");
			nuanceur->index_refraction = index;
			break;
		}
		case TypeNuanceur::VOLUME:
		{
			auto nuanceur = static_cast<NuanceurVolume *>(m_nuanceur);
			auto sigma_a = m_persona_volume->evalue_couleur("absorption");
			nuanceur->sigma_a = Spectre::depuis_rgb(sigma_a.x, sigma_a.y, sigma_a.z);
			auto sigma_s = m_persona_volume->evalue_couleur("diffusion");
			nuanceur->sigma_s = Spectre::depuis_rgb(sigma_s.x, sigma_s.y, sigma_s.z);
			break;
		}
		case TypeNuanceur::EMISSION:
		{
			auto nuanceur = static_cast<NuanceurEmission *>(m_nuanceur);
			auto couleur = m_persona_emission->evalue_couleur("spectre");
			nuanceur->spectre = Spectre::depuis_rgb(couleur.x, couleur.y, couleur.z);
			nuanceur->exposition = m_persona_emission->evalue_float("exposition");
			break;
		}
	}
}

bool VueMaterial::ajourne_proprietes()
{
	switch (m_nuanceur->type) {
		case TypeNuanceur::DIFFUS:
		{
			auto nuanceur = static_cast<NuanceurDiffus *>(m_nuanceur);
			m_persona_diffus->ajourne_valeur_couleur("spectre", converti_couleur(nuanceur->spectre));
			break;
		}
		case TypeNuanceur::ANGLE_VUE:
		{
			auto nuanceur = static_cast<NuanceurAngleVue *>(m_nuanceur);
			m_persona_angle_vue->ajourne_valeur_couleur("spectre", converti_couleur(nuanceur->spectre));
			break;
		}
		case TypeNuanceur::REFLECTION:
		{
			break;
		}
		case TypeNuanceur::REFRACTION:
		{
			auto nuanceur = static_cast<NuanceurRefraction *>(m_nuanceur);
			m_persona_refraction->ajourne_valeur_float("index", static_cast<float>(nuanceur->index_refraction));
			break;
		}
		case TypeNuanceur::VOLUME:
		{
			auto nuanceur = static_cast<NuanceurVolume *>(m_nuanceur);
			m_persona_volume->ajourne_valeur_couleur("absorption", converti_couleur(nuanceur->sigma_a));
			m_persona_volume->ajourne_valeur_couleur("diffusion", converti_couleur(nuanceur->sigma_s));
			break;
		}
		case TypeNuanceur::EMISSION:
		{
			auto nuanceur = static_cast<NuanceurEmission *>(m_nuanceur);
			m_persona_emission->ajourne_valeur_couleur("spectre", converti_couleur(nuanceur->spectre));
			m_persona_emission->ajourne_valeur_float("exposition", static_cast<float>(nuanceur->exposition));
			break;
		}
	}

	return true;
}

Persona *VueMaterial::persona() const
{
	switch (m_nuanceur->type) {
		case TypeNuanceur::DIFFUS:
			return m_persona_diffus;
		case TypeNuanceur::ANGLE_VUE:
			return m_persona_angle_vue;
		case TypeNuanceur::REFLECTION:
			return m_persona_reflection;
		case TypeNuanceur::REFRACTION:
			return m_persona_refraction;
		case TypeNuanceur::VOLUME:
			return m_persona_volume;
		case TypeNuanceur::EMISSION:
			return m_persona_emission;
	}

	return nullptr;
}

/* ************************************************************************** */

EditeurMaterial::EditeurMaterial(Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
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

EditeurMaterial::~EditeurMaterial()
{
	delete m_vue;
}

void EditeurMaterial::ajourne_etat(int evenement)
{
	auto creation = (evenement == (type_evenement::objet | type_evenement::ajoute));
	creation |= (evenement == (type_evenement::objet | type_evenement::selectione));
	creation |= (evenement == (static_cast<type_evenement>(-1)));

	if (!creation) {
		return;
	}

	auto &scene = m_koudou->parametres_rendu.scene;

	if (scene.objets.empty()) {
		return;
	}

	if (scene.objet_actif == nullptr || scene.objet_actif->nuanceur == nullptr) {
		return;
	}

	auto nuanceur = scene.objet_actif->nuanceur;

	if (m_vue == nullptr) {
		m_vue = new VueMaterial(nuanceur);
	}
	else {
		m_vue->nuanceur(nuanceur);
	}

	m_vue->ajourne_proprietes();

	if (m_vue->persona() == nullptr) {
		return;
	}

	cree_controles(m_assembleur_controles, m_vue->persona());
	m_assembleur_controles.setContext(this, SLOT(ajourne_material()));
}

void EditeurMaterial::ajourne_material()
{
	m_vue->ajourne_donnees();
	m_koudou->notifie_observatrices(type_evenement::materiau | type_evenement::modifie);
}
