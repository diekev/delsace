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

#pragma once

#include "scene.hh"

namespace vision {
class Camera3D;
}  /* namespace vision */

/* Notes sur le design d'Arnold qui inspire un peu notre moeteur de
 * rendu, d'autres moteurs seront étudiés afin de mieux savoir ce qu'il
 * se passe dans l'industrie et avoir un système plus robuste.
 *
 * Tout est un noeud (maillage, points, volume, surface implicite,
 * caméra, instance, courbes, procéduraux, etc.), pour simplifier le
 * code. Les noeuds procédureaux peuvent avoir des enfants, de manière
 * récursive. Une procédure peut être un cache Alembic, ou une
 * description de scène. La création récursive d'enfant peut être utile
 * pour créer des plumes ou des poils. Les volumes requiers de définir
 * des intervals le long des rayons pour les marcher.
 *
 * Chaque noeud possède son propre arbre HBE. Une traductrice de scène
 * crée tous les noeuds, et rassemble les noeuds racines de l'arbre HBE
 * principal dans une liste. L'arbre HBE principal (4-wide, SAH) est
 * construit par ascendance depuis les noeuds feuilles.
 *
 * La géométrie d'un noeud n'est évaluée (subdivision de triangles et
 * courbes, déplacements, ajustements des normaux) que lorsqu'un noeud
 * de l'arbre HBE de la scène est touché par un rayon, ceci pour
 * accélerer le temps d'affichage du premier pixel, et pour éviter de
 * travailler sur des objets n'étant jamais touché.
 *
 * ATTENTION :
 * - ceci requiers de ne pas bloquer sur les threads
 * - les premier thread initie la construction, les autres qui touchent
 *   également le noeud le rejoignent si nécessaire
 *
 * Une fois la géométrie d'un noeud évaluée, son arbre HBE est crée.
 *
 * Dans notre moteur, pour le moment, les arbres HBE des noeuds
 * géométries sont créés avant le rendu.
 *
 * Source https://www.arnoldrenderer.com/research/Arnold_TOG2018.pdf
 */

namespace kdo {

class MoteurRendu;
class StructureAcceleration;

struct ParametresRendu {
	unsigned int nombre_echantillons = 32;
	unsigned int nombre_rebonds = 5;
	unsigned int resolution = 0;
	unsigned int hauteur_carreau = 32;
	unsigned int largeur_carreau = 32;
	double biais_ombre = 1e-4;

	Scene scene{};

#ifdef NOUVELLE_CAMERA
	CameraPerspective *camera = nullptr;
#else
	vision::Camera3D *camera = nullptr;
#endif

	ParametresRendu();
	~ParametresRendu();

	ParametresRendu(ParametresRendu const &) = delete;
	ParametresRendu &operator=(ParametresRendu const &) = delete;
};

struct InformationsRendu {
	/* Le temps écoulé depuis le début du rendu. */
	double temps_ecoule = 0.0;

	/* Le temps restant estimé pour finir le rendu. */
	double temps_restant = 0.0;

	/* Le temps pris pour calculer le dernier échantillon tiré. */
	double temps_echantillon = 0.0;

	/* L'échantillon courant. */
	unsigned int echantillon = 0;
};

struct Koudou {
	MoteurRendu *moteur_rendu;
	ParametresRendu parametres_rendu{};

	InformationsRendu informations_rendu{};

	vision::Camera3D *camera;

	Koudou();

	Koudou(Koudou const &) = delete;
	Koudou &operator=(Koudou const &) = delete;

	~Koudou();
};

}  /* namespace kdo */
