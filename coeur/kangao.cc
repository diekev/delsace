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

#include "kangao.h"

#include <fstream>
#include <iostream>

#include "interne/assembleur_disposition.h"
#include "interne/analyseur.h"
#include "interne/decoupeur.h"

#include "erreur.h"

QBoxLayout *compile_interface(DonneesInterface &donnnes, const char *texte_entree)
{
	if (donnnes.manipulable == nullptr) {
		return nullptr;
	}

	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

	Analyseur analyseur;
	analyseur.installe_assembleur(&assembleur);

	Decoupeur decoupeur(texte_entree);

	try {
		decoupeur.decoupe();
		analyseur.lance_analyse(decoupeur.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	return assembleur.disposition();
}

QBoxLayout *compile_interface(
		DonneesInterface &donnnes,
		const std::experimental::filesystem::path &chemin_texte)
{
	if (donnnes.manipulable == nullptr) {
		return nullptr;
	}

	std::ifstream entree;
	entree.open(chemin_texte.c_str());

	std::string texte_entree;
	std::string temp;

	while (std::getline(entree, temp)) {
		texte_entree += temp;
	}

	return compile_interface(donnnes, texte_entree.c_str());
}

QMenu *compile_menu(DonneesInterface &donnnes, const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

	Analyseur analyseur;
	analyseur.installe_assembleur(&assembleur);

	Decoupeur decoupeur(texte_entree);

	try {
		decoupeur.decoupe();
		analyseur.lance_analyse(decoupeur.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	return assembleur.menu();
}
