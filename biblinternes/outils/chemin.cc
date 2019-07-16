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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "chemin.hh"

#include "chaine.hh"

namespace dls {

chaine chemin_relatif(chaine const &chemin)
{
	chaine cr;

	for (auto const &morceau : morcelle(chemin, '/')) {
		cr += morceau;
		cr += '/';
	}

	return cr;
}

/* ************************************************************************** */

void corrige_chemin_pour_temps(chaine &chemin, const int image)
{
	/* Trouve le dernier point. */
	auto pos_dernier_point = chemin.trouve_dernier_de('.');

	if (pos_dernier_point == chaine::npos || pos_dernier_point == 0) {
		//std::cerr << "Ne peut pas trouver le dernier point !\n";
		return;
	}

	//std::cerr << "Trouver le dernier point à la position : " << pos_dernier_point << '\n';

	/* Trouve le point précédent. */
	auto pos_point_precedent = pos_dernier_point - 1;

	while (pos_point_precedent > 0 && ::isdigit(chemin[pos_point_precedent])) {
		pos_point_precedent -= 1;
	}

	if (pos_point_precedent == chaine::npos || pos_point_precedent == 0) {
		//std::cerr << "Ne peut pas trouver le point précédent !\n";
		return;
	}

	if (chemin[pos_point_precedent] == '/') {
		//std::cerr << "Le chemin n'a pas de nom !\n";
		return;
	}

	//std::cerr << "Trouver l'avant dernier point à la position : " << pos_point_precedent << '\n';

	auto taille_nombre_image = pos_dernier_point - (pos_point_precedent + 1);

	//std::cerr << "Nombre de caractères pour l'image : " << taille_nombre_image << '\n';

	auto chaine_image = chaine(std::to_string(image));

	chaine_image.insere(0, taille_nombre_image - chaine_image.taille(), '0');

	chemin.remplace(pos_point_precedent + 1, chaine_image.taille(), chaine_image);
	//std::cerr << "Nouveau nom " << chemin << '\n';
}

void corrige_chemin_pour_ecriture(chaine &chemin, int const temps)
{
	auto const pos_debut = chemin.trouve_premier_de('#');

	if (pos_debut == chaine::npos) {
		return;
	}

	auto pos_fin = pos_debut + 1;

	while (chemin[pos_fin] == '#') {
		pos_fin++;
	}

	auto compte = pos_fin - pos_debut;

	auto chaine_nombre = chaine(std::to_string(temps));
	chaine_nombre.insere(0, compte - chaine_nombre.taille(), '0');

	chemin.remplace(pos_debut, chaine_nombre.taille(), chaine_nombre);
}

}  /* namespace dls */
