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

#pragma once

#include <set>
#include <stack>
#include <string>
#include <vector>

struct Preproces {
	/* pile pour les dossiers courants */
	std::stack<std::string> dossier_courant;

	/* ensemble des chemins d'inclusions */
	std::set<std::string> chemin_inclusions;

	/* ensemble de fichiers déjà chargés, duplicat de liste_fichier en bas, mais
	 * nous permet de chercher plus vite si un fichier à déjà été chargé */
	std::set<std::string> fichiers;

	/* ensemble des fichiers visités afin de vérifier si un fichier a déjà été
	 * chargé ou non ; permet de détecter des dépendances cycliques entre les
	 * fichiers. */
	std::set<std::string> fichiers_visites;

	/* le tampon construit par le chargement de tous les fichiers */
	std::string tampon;

	/* numéro ligne << 32 | index fichier (dans liste_fichier) */
	std::vector<size_t> donnees_lignes;

	/* liste des fichiers à utiliser avec donness_lignes */
	std::vector<std::string> liste_fichier;

	/* nombre total de lignes dans le tampon, sans les lignes vides ou les
	 * lignes de commentaires */
	size_t nombre_lignes_total = 0;
};

void charge_fichier(Preproces &preproces, const std::string &chemin);

void imprime_tampon(Preproces &preproces, std::ostream &os);

void imprime_donnees_lignes(Preproces &preproces, std::ostream &os);
