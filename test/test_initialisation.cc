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

#include "test_initialisation.h"

#include "coeur/danjo/danjo.h"
#include "coeur/danjo/manipulable.h"

static bool possede_propriete(danjo::Manipulable &manipulable, const std::string &nom)
{
	return manipulable.propriete(nom) != nullptr;
}

void test_initialisation(numero7::test_unitaire::ControleurUnitaire &controleur)
{
	auto manipulable = danjo::Manipulable();

	const auto texte_entree = danjo::contenu_fichier("exemples/disposition_test.jo");
	danjo::initialise_entreface(&manipulable, texte_entree.c_str());

	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "taille_x"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "taille_y"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "type_fichier"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "liste_calque"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "nom_fichier"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "fichier_in"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "fichier_ex"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "direction"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "rampe1"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "arriere_plan"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "cocher"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "courbe_1"));
	CU_VERIFIE_CONDITION(controleur, possede_propriete(manipulable, "courbe_2"));
}
