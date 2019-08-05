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

#include "editeur_objet.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/outils/constantes.h"

#include "coeur/evenement.h"
#include "coeur/koudou.h"
#include "coeur/lumiere.h"
#include "coeur/maillage.h"
#include "coeur/objet.h"

VueObjet::VueObjet()
	: m_objet(nullptr)
{
	ajoute_propriete("position", danjo::TypePropriete::VECTEUR);
//	etablie_valeur_vec3_defaut(dls::math::vec3f(0.0f));
//	etablie_min_max(-10.0f, 10.0f);

	ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR);
//	etablie_valeur_vec3_defaut(dls::math::vec3f(0.0f));
//	etablie_min_max(0.0f, 360.0f);

	ajoute_propriete("échelle", danjo::TypePropriete::VECTEUR);
//	etablie_valeur_vec3_defaut(dls::math::vec3f(1.0f));
//	etablie_min_max(0.0f, 10.0f);

	/* Propriétés maillages */
	ajoute_propriete("dessine_normaux", danjo::TypePropriete::BOOL);
//	etablie_valeur_bool_defaut(false);
}

void VueObjet::objet(kdo::Objet *o)
{
	m_objet = o;
}

void VueObjet::ajourne_donnees()
{
	if (m_objet == nullptr) {
		return;
	}

	auto pos = evalue_vecteur("position");
	auto rot = evalue_vecteur("rotation");
	auto ech = evalue_vecteur("échelle");

	auto transformation = math::construit_transformation(pos, rot, ech);

	m_objet->transformation = transformation;

	if (m_objet->type == kdo::TypeObjet::LUMIERE) {
		m_objet->lumiere->transformation = transformation;
	}
	else if (m_objet->type == kdo::TypeObjet::MAILLAGE) {
		m_objet->maillage->transformation(transformation);

		m_objet->maillage->dessine_normaux(evalue_bool("dessine_normaux"));
	}
}

bool VueObjet::ajourne_proprietes()
{
	auto const est_maillage = (m_objet->type == kdo::TypeObjet::MAILLAGE);

	if (est_maillage) {
		valeur_bool("dessine_normaux", m_objet->maillage->dessine_normaux());
	}

	//rend_visible("dessine_normaux", est_maillage);

	return true;
}

/* ************************************************************************** */

EditeurObjet::EditeurObjet(kdo::Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
	, m_vue(new VueObjet())
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

EditeurObjet::~EditeurObjet()
{
	delete m_vue;
}

void EditeurObjet::ajourne_etat(int evenement)
{
	auto &scene = m_koudou->parametres_rendu.scene;

	if (scene.objets.est_vide()) {
		return;
	}

	if (scene.objet_actif == nullptr) {
		return;
	}

	m_vue->objet(scene.objet_actif);
	m_vue->ajourne_proprietes();

	auto creation = (evenement == (type_evenement::objet | type_evenement::ajoute));
	creation |= (evenement == (type_evenement::objet | type_evenement::selectione));
	creation |= (evenement == (static_cast<type_evenement>(-1)));

	if (creation) {
		//cree_controles(m_assembleur_controles, m_vue);
		//m_assembleur_controles.setContext(this, SLOT(ajourne_maillage()));
	}
	else {
		//ajourne_controles(m_assembleur_controles, m_vue);
	}
}

void EditeurObjet::ajourne_maillage()
{
	m_vue->ajourne_donnees();
	m_koudou->notifie_observatrices(type_evenement::objet | type_evenement::modifie);
}
