/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either verifon 2
 * of the License, or (at your option) any later verifon.
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

#include "import_objet.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "biblinternes/outils/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "adaptrice_creation.h"

namespace objets {

static void lis_normal_sommet(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	float x, y, z;
	is >> x >> y >> z;

	adaptrice->ajoute_normal(x, y, z);
}

static void lis_parametres_sommet(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	float x, y, z;
	is >> x >> y >> z;

	adaptrice->ajoute_parametres_sommet(x, y, z);
}

static void lis_coord_texture_sommet(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	float u, v, w;
	is >> u >> v >> w;

	adaptrice->ajoute_coord_uv_sommet(u, v, w);
}

static void lis_sommet(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	float x, y, z;
	is >> x >> y >> z;

	adaptrice->ajoute_sommet(x, y, z);
}

static void lis_polygone(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	dls::chaine info_poly;
	dls::tableau<int> index_polygones;
	dls::tableau<int> index_normaux;
	dls::tableau<int> index_coords_uv;

	auto ptr_index = static_cast<int *>(nullptr);
	auto ptr_normaux = static_cast<int *>(nullptr);
	auto ptr_coords = static_cast<int *>(nullptr);

	while (is >> info_poly) {
		auto morceaux = dls::morcelle(info_poly, '/');

		switch (morceaux.taille()) {
			case 1:
			{
				index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);

				ptr_index = index_polygones.donnees();

				break;
			}
			case 2:
			{
				index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);
				index_coords_uv.ajoute(std::stoi(morceaux[1].c_str()) - 1);

				ptr_index = index_polygones.donnees();
				ptr_coords = index_coords_uv.donnees();

				break;
			}
			case 3:
			{
				index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);

				if (!morceaux[1].est_vide()) {
					index_coords_uv.ajoute(std::stoi(morceaux[1].c_str()) - 1);
					ptr_coords = index_coords_uv.donnees();
				}

				index_normaux.ajoute(std::stoi(morceaux[2].c_str()) - 1);

				ptr_index = index_polygones.donnees();
				ptr_normaux = index_normaux.donnees();

				break;
			}
		}
	}

	adaptrice->ajoute_polygone(
				ptr_index,
				ptr_coords,
				ptr_normaux,
				index_polygones.taille());
}

static void lis_objet(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	dls::chaine nom_objet;
	is >> nom_objet;

	adaptrice->ajoute_objet(nom_objet);
}

static void lis_ligne(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	dls::chaine info_poly;
	dls::tableau<int> index;

	while (is >> info_poly) {
		index.ajoute(std::stoi(info_poly.c_str()) - 1);
	}

	adaptrice->ajoute_ligne(index.donnees(), static_cast<size_t>(index.taille()));
}

static void lis_groupes_geometries(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	dls::chaine groupe;
	dls::tableau<dls::chaine> groupes;

	while (is >> groupe) {
		groupes.ajoute(groupe);
	}

	adaptrice->groupes(groupes);
}

static void lis_groupe_nuancage(AdaptriceCreationObjet *adaptrice, std::istringstream &is)
{
	dls::chaine groupe;
	is >> groupe;

	if (groupe == "off") {
		return;
	}

	int index = std::stoi(groupe.c_str());

	adaptrice->groupe_nuancage(index);
}

void charge_fichier_OBJ(AdaptriceCreationObjet *adaptrice, dls::chaine const &chemin)
{
	std::ifstream ifs;
	ifs.open(chemin.c_str());

	if (!ifs.is_open()) {
		return;
	}

	std::string ligne;
	dls::chaine entete;

	while (std::getline(ifs, ligne)) {
		std::istringstream is(ligne);
		is >> entete;

		if (entete == "vn") {
			lis_normal_sommet(adaptrice, is);
		}
		else if (entete == "vt") {
			lis_coord_texture_sommet(adaptrice, is);
		}
		else if (entete == "vp") {
			lis_parametres_sommet(adaptrice, is);
		}
		else if (entete == "v") {
			lis_sommet(adaptrice, is);
		}
		else if (ligne[0] == 'f') {
			lis_polygone(adaptrice, is);
		}
		else if (ligne[0] == 'l') {
			lis_ligne(adaptrice, is);
		}
		else if (entete == "o") {
			lis_objet(adaptrice, is);
		}
		else if (entete == "#") {
			continue;
		}
		else if (entete == "g") {
			lis_groupes_geometries(adaptrice, is);
		}
		else if (entete == "s") {
			lis_groupe_nuancage(adaptrice, is);
		}
		else if (entete == "mttlib") {
			/* À FAIRE */
		}
		else if (entete == "newmtl") {
			/* À FAIRE */
		}
		else if (entete == "usemtl") {
			/* À FAIRE */
		}
	};
}

/* ************************************************************************** */

static bool est_entete_ascii(uint8_t *entete)
{
	return     entete[0] == 's'
			&& entete[1] == 'o'
			&& entete[2] == 'l'
			&& entete[3] == 'i'
			&& entete[4] == 'd';
}

static void charge_STL_ascii(AdaptriceCreationObjet *adaptrice, std::ifstream &fichier)
{
	/* lis la première ligne, doit être 'solid nom' */
	std::string tampon;
	std::getline(fichier, tampon);

	dls::chaine mot;

	int sommets[3] = { 0, 1, 2 };
	int normaux[3] = { 0, 0, 0 };

	while (std::getline(fichier, tampon)) {
		std::istringstream is(tampon);
		is >> mot;

		if (mot == "facet") {
			is >> mot;

			if (mot != "normal") {
				std::cerr << "Attendu le mot 'normal' après 'facet' !\n";
				break;
			}

			float nx, ny, nz;
			is >> nx >> ny >> nz;

			adaptrice->ajoute_normal(nx, ny, nz);
		}
		else if (mot == "outer") {
			is >> mot;

			if (mot != "loop") {
				std::cerr << "Attendu le mot 'loop' après 'outer' !\n";
				break;
			}
		}
		else if (mot == "vertex") {
			float vx, vy, vz;
			is >> vx >> vy >> vz;

			adaptrice->ajoute_sommet(vx, vy, vz);
		}
		else if (mot == "endloop") {
			/* R-À-F */
		}
		else if (mot == "endfacet") {
			adaptrice->ajoute_polygone(sommets, nullptr, normaux, 3);

			for (int i = 0; i < 3; ++i) {
				sommets[i] += 3;
				normaux[i] += 1;
			}
		}
		else if (mot == "endsolid") {
			/* R-À-F */
		}
		else {
			std::cerr << "Mot clé '" << mot << "' inattendu !\n";
			break;
		}
	}
}

static void charge_STL_binaire(AdaptriceCreationObjet *adaptrice, std::ifstream &fichier)
{
	/* lis l'en-tête */
	uint8_t entete[80];
	fichier.read(reinterpret_cast<char *>(entete), 80);

	/* lis nombre triangle */
	uint32_t nombre_triangles;
	fichier.read(reinterpret_cast<char *>(&nombre_triangles), sizeof(uint32_t));

	adaptrice->reserve_polygones(nombre_triangles);
	adaptrice->reserve_sommets(nombre_triangles * 3);
	adaptrice->reserve_normaux(nombre_triangles);

	/* lis triangles */
	float nor[3];
	float v0[3];
	float v1[3];
	float v2[3];
	uint16_t mot_controle;

	int sommets[3] = { 0, 1, 2 };
	int normaux[3] = { 0, 0, 0 };

	for (uint32_t i = 0; i < nombre_triangles; ++i) {
		fichier.read(reinterpret_cast<char *>(nor), sizeof(float) * 3);
		fichier.read(reinterpret_cast<char *>(v0), sizeof(float) * 3);
		fichier.read(reinterpret_cast<char *>(v1), sizeof(float) * 3);
		fichier.read(reinterpret_cast<char *>(v2), sizeof(float) * 3);
		fichier.read(reinterpret_cast<char *>(&mot_controle), sizeof(uint16_t));

		adaptrice->ajoute_normal(nor[0], nor[1], nor[2]);
		adaptrice->ajoute_sommet(v0[0], v0[1], v0[2]);
		adaptrice->ajoute_sommet(v1[0], v1[1], v1[2]);
		adaptrice->ajoute_sommet(v2[0], v2[1], v2[2]);

		adaptrice->ajoute_polygone(sommets, nullptr, normaux, 3);

		for (uint32_t e = 0; e < 3; ++e) {
			sommets[e] += 3;
			normaux[e] += 1;
		}
	}
}

void charge_fichier_STL(AdaptriceCreationObjet *adaptrice, dls::chaine const &chemin)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		return;
	}

	/* lis en-tête */
	uint8_t entete[5];
	fichier.read(reinterpret_cast<char *>(entete), 5);

	/* rembobine */
	fichier.seekg(0);

	if (est_entete_ascii(entete)) {
		charge_STL_ascii(adaptrice, fichier);
	}
	else {
		charge_STL_binaire(adaptrice, fichier);
	}
}

}  /* namespace objets */
