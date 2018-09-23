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

#include "bibliotheques/transformation/transformation.h"

#include <math/point3.h>
#include <math/vec3.h>

#include <unordered_map>
#include <vector>

#include "listes.h"

enum {
	CORPS_NUL,
	CORPS_MAILLAGE,
	CORPS_NUAGE_POINTS,  /* À FAIRE : nuage de points, particules */
	CORPS_COURBE,        /* À FAIRE : bézier3d, poil */
	CORPS_SURFACE,       /* À FAIRE : NURBS */
	CORPS_VOLUME,        /* À FAIRE : OpenVDB */
	CORPS_PANCARTE,      /* À FAIRE : pancarte alignée à la vue/caméra */
};

class Attribut;
class GroupePoint;
class GroupePolygone;

/**
 * La structure Corps représente une partie constituante d'un objet. Le Corps
 * peut-être un maillage, ou un volume, ou autre (voir énumération ci-dessus).
 * Cette structure n'est qu'une base, définissant les propriétés générales de
 * tous les types de corps.
 */
struct Corps {
	/* transformation */
	math::transformation transformation = math::transformation();
	numero7::math::point3f pivot        = numero7::math::point3f(0.0f);
	numero7::math::point3f position     = numero7::math::point3f(0.0f);
	numero7::math::point3f echelle      = numero7::math::point3f(1.0f);
	numero7::math::point3f rotation     = numero7::math::point3f(0.0f);
	float echelle_uniforme              = 1.0f;

	/* boîte englobante */
	numero7::math::point3f min    = numero7::math::point3f(0.0f);
	numero7::math::point3f max    = numero7::math::point3f(0.0f);
	numero7::math::point3f taille = numero7::math::point3f(0.0f);

	/* autres propriétés */
	std::string nom = "corps";

	int type = CORPS_NUL;

	Corps() = default;
	virtual ~Corps();

	bool possede_attribut(const std::string &nom);

	Attribut *ajoute_attribut(const std::string &nom, int type, int portee = 0, size_t taille = 0);

	void supprime_attribut(const std::string &nom);

	Attribut *attribut(const std::string &nom) const;

	GroupePolygone *ajoute_groupe_polygone(const std::string &nom);

	int ajoute_point(float x, float y, float z);

	/**
	 * Retourne l'index du point se trouvant aux coordonnées x, y, z spécifiée.
	 * Si aucun point ne s'y trouve, retourne -1.
	 */
	int index_point(float x, float y, float z);

	void ajoute_polygone(Polygone *p);

	ListePoints3D *points();

	const ListePoints3D *points() const;

	ListePolygones *polys();

	const ListePolygones *polys() const;

	void reinitialise();

	Corps *copie() const;

	void copie_vers(Corps *corps) const;

protected:
	std::vector<Attribut *> m_attributs;

private:
	ListePoints3D m_points;
	ListePolygones m_polys;
	std::unordered_map<std::string, GroupePolygone *> m_groupes_polygones;
};
