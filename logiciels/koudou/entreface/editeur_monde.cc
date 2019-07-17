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

#include "editeur_monde.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/texture/texture.h"

#include "coeur/evenement.h"
#include "coeur/koudou.h"

VueMonde::VueMonde(Monde *monde)
	: m_monde(monde)
{
//	ProprieteEnumerante enum_texture;
//	enum_texture.ajoute("Couleur", static_cast<int>(TypeTexture::COULEUR));
//	enum_texture.ajoute("Image", static_cast<int>(TypeTexture::IMAGE));

	ajoute_propriete("type_texture", danjo::TypePropriete::ENUM);
//	etablie_valeur_enum(enum_texture);

	ajoute_propriete("image", danjo::TypePropriete::FICHIER_ENTREE);

	ajoute_propriete("couleur", danjo::TypePropriete::COULEUR);
}

void VueMonde::ajourne_donnees()
{
	auto const type_texture = static_cast<TypeTexture>(0); //static_cast<TypeTexture>(evalue_enum("type_texture"));

	if (!m_monde->texture || m_monde->texture->type() != type_texture) {
		supprime_texture(m_monde->texture);

		if (type_texture == TypeTexture::COULEUR) {
			auto couleur = evalue_couleur("couleur");

			auto texture = new TextureCouleur();
			texture->etablie_spectre(Spectre::depuis_rgb(&couleur[0]));

			m_monde->texture = texture;
		}
		else {
			auto chemin = evalue_chaine("image");
			auto texture = charge_texture(chemin.c_str());

			m_monde->texture = texture;
		}
	}
	else if (m_monde->texture->type() == type_texture) {
		if (type_texture == TypeTexture::COULEUR) {
			auto couleur = evalue_couleur("couleur");

			auto texture = dynamic_cast<TextureCouleur *>(m_monde->texture);
			texture->etablie_spectre(Spectre::depuis_rgb(&couleur[0]));
		}
		else {
			supprime_texture(m_monde->texture);

			auto chemin = evalue_chaine("image");
			auto texture = charge_texture(chemin.c_str());

			m_monde->texture = texture;
		}
	}
}

bool VueMonde::ajourne_proprietes()
{
	auto const type_texture = static_cast<TypeTexture>(0); //static_cast<TypeTexture>(evalue_enum("type_texture"));

//	rend_visible("image", type_texture == TypeTexture::IMAGE);
//	rend_visible("couleur", type_texture == TypeTexture::COULEUR);

	if (type_texture == TypeTexture::COULEUR) {
		if (m_monde->texture) {
			auto texture = dynamic_cast<TextureCouleur *>(m_monde->texture);

			/* À FAIRE : pour quelque raison le type de l'entreface et de la
			 * texture ne sont pas les mêmes. */
			if (!texture) {
				return true;
			}

			auto couleur = texture->spectre();

			valeur_couleur("couleur", dls::phys::couleur32(couleur[0], couleur[1], couleur[2], 1.0f));
		}
	}

	return true;
}

/* ************************************************************************** */

EditeurMonde::EditeurMonde(Koudou *koudou, QWidget *parent)
	: BaseEditrice(*koudou, parent)
	, m_vue(new VueMonde(&koudou->parametres_rendu.scene.monde))
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

EditeurMonde::~EditeurMonde()
{
	delete m_vue;
}

void EditeurMonde::ajourne_etat(int evenement)
{
	m_vue->ajourne_proprietes();

	if (evenement == type_evenement::rafraichissement) {
		//cree_controles(m_assembleur_controles, m_vue);
		//m_assembleur_controles.setContext(this, SLOT(ajourne_monde()));
	}
	else {
		//ajourne_controles(m_assembleur_controles, m_vue);
	}
}

void EditeurMonde::ajourne_monde()
{
	m_vue->ajourne_donnees();
	m_koudou->notifie_observatrices(type_evenement::rendu);
}
