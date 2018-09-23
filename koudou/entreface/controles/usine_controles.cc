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

#include "usine_controles.h"

#include <QStandardItemModel>

#include "coeur/persona.h"

#include "assembleur_controles.h"
#include "controles.h"

void controle_int(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleInt;
	controle->pointeur(std::experimental::any_cast<int>(&prop->donnee));
	controle->setRange(prop->min, prop->max);
	controle->setValue(std::experimental::any_cast<int>(prop->donnee));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_float(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleFloat;
	controle->pointeur(std::experimental::any_cast<float>(&prop->donnee));
	controle->setRange(prop->min, prop->max);
	controle->setValue(std::experimental::any_cast<float>(prop->donnee));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_enum(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleEnum;
	controle->pointeur(std::experimental::any_cast<int>(&prop->donnee));

	for (const PaireNomValeur &item : prop->items_enumeration.paires) {
		controle->addItem(item.nom.c_str(), QVariant(item.valeur));
	}

	controle->setCurrentIndex(std::experimental::any_cast<int>(prop->donnee));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void bitfield_controle(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleEnum;
	controle->pointeur(std::experimental::any_cast<int>(&prop->donnee));

	auto model = new QStandardItemModel(prop->items_enumeration.paires.size(), 1);
	auto index = 0;

	for (const PaireNomValeur &enum_item : prop->items_enumeration.paires) {
		//controle->addItem(item.name.c_str(), QVariant(item.value));
		auto item = new QStandardItem(enum_item.nom.c_str());
//		item->setText(enum_item.name.c_str());
		item->setCheckable(true);
		item->setCheckState(Qt::Unchecked);
		item->setData(QVariant(enum_item.valeur));
//		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
//		item->setData(Qt::Unchecked, Qt::CheckStateRole);
		model->setItem(index++, 0, item);
	}

	controle->setModel(model);

//	controle->setCurrentIndex(default_value);

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_chaine_caractere(AssembleurControles &assembleur, Propriete *prop)
{
	auto ptr = std::experimental::any_cast<std::string>(&prop->donnee);
	auto controle = new ControleChaineCaractere;
	controle->pointeur(ptr);

	if (ptr->length() == 0) {
		controle->setPlaceholderText(std::experimental::any_cast<std::string>(prop->valeur_defaut).c_str());
	}
	else {
		controle->setText(ptr->c_str());
	}

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_bool(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleBool;
	controle->pointeur(std::experimental::any_cast<bool>(&prop->donnee));
	controle->setChecked(std::experimental::any_cast<bool>(prop->donnee));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void infobulle_controle(AssembleurControles &assembleur, const char *tooltip)
{
	assembleur.setTooltip(tooltip);
}

void controle_vec3(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleVec3;
	controle->setMinMax(prop->min, prop->max);
	controle->pointeur(&(std::experimental::any_cast<glm::vec3>(&prop->donnee)->x));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_couleur(AssembleurControles &assembleur, Propriete *prop)
{
	auto controle = new ControleCouleur;
	controle->setMinMax(prop->min, prop->max);
	controle->pointeur(&(std::experimental::any_cast<glm::vec4>(&prop->donnee)->x));

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_fichier_entree(AssembleurControles &assembleur, Propriete *prop)
{
	auto ptr = std::experimental::any_cast<std::string>(&prop->donnee);
	auto controle = new ControleFichier(true);
	controle->pointeur(ptr);
	controle->setValue(ptr->c_str());

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_fichier_sortie(AssembleurControles &assembleur, Propriete *prop)
{
	auto ptr = std::experimental::any_cast<std::string>(&prop->donnee);
	auto controle = new ControleFichier(false);
	controle->pointeur(ptr);
	controle->setValue(ptr->c_str());

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}

void controle_liste(AssembleurControles &assembleur, Propriete *prop)
{
	auto ptr = std::experimental::any_cast<std::string>(&prop->donnee);
	auto controle = new ControleListe();
	controle->pointeur(ptr);
	controle->setValue(ptr->c_str());

	for (const PaireNomValeur &item : prop->items_enumeration.paires) {
		controle->addField(item.nom.c_str());
	}

	assembleur.addWidget(controle, prop->nom_entreface.c_str());
}
