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

#include "outils.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QLayout>
#include <QWidget>
#pragma GCC diagnostic pop

#include "coeur/persona.h"

#include "controles/assembleur_controles.h"
#include "controles/usine_controles.h"

void vide_agencement(QLayout *agencement)
{
	QLayoutItem *item;

	while ((item = agencement->takeAt(0)) != nullptr) {
		if (item->layout()) {
			vide_agencement(item->layout());
			delete item->layout();
		}

		if (item->widget()) {
			delete item->widget();
		}

		delete item;
	}
}

void cree_controles(AssembleurControles &assembleur, Persona *persona)
{
	assembleur.clear();

	for (Propriete &prop : persona->proprietes()) {
		assert(!prop.donnee.empty());

		switch (prop.type) {
			case TypePropriete::BOOL:
			{
				controle_bool(assembleur, &prop);
				break;
			}
			case TypePropriete::FLOAT:
			{
				controle_float(assembleur, &prop);
				break;
			}
			case TypePropriete::INT:
			{
				controle_int(assembleur, &prop);
				break;
			}
			case TypePropriete::ENUM:
			{
				controle_enum(assembleur, &prop);
				break;
			}
			case TypePropriete::VEC3:
			{
				controle_vec3(assembleur, &prop);
				break;
			}
			case TypePropriete::FICHIER_ENTREE:
			{
				controle_fichier_entree(assembleur, &prop);
				break;
			}
			case TypePropriete::FICHIER_SORTIE:
			{
				controle_fichier_sortie(assembleur, &prop);
				break;
			}
			case TypePropriete::STRING:
			{
				controle_chaine_caractere(assembleur, &prop);
				break;
			}
			case TypePropriete::LISTE:
			{
				controle_liste(assembleur, &prop);
				break;
			}
			case TypePropriete::COULEUR:
			{
				controle_couleur(assembleur, &prop);
				break;
			}
		}

		if (!prop.infobulle.est_vide()) {
			infobulle_controle(assembleur, prop.infobulle.c_str());
		}

		assembleur.setVisible(prop.nom_entreface.c_str(), prop.visible);
	}
}

void ajourne_controles(AssembleurControles &assembleur, Persona *persona)
{
	for (Propriete &prop : persona->proprietes()) {
		assembleur.setVisible(prop.nom_entreface.c_str(), prop.visible);
	}
}
