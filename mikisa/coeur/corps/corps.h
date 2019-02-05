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

#include <unordered_map>
#include <vector>

#include "../attribut.h"
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
class GroupePrimitive;

/**
 * La structure Corps représente une partie constituante d'un objet. Le Corps
 * peut-être un maillage, ou un volume, ou autre (voir énumération ci-dessus).
 * Cette structure n'est qu'une base, définissant les propriétés générales de
 * tous les types de corps.
 */
struct Corps {
	/* transformation */
	math::transformation transformation = math::transformation();
	dls::math::point3f pivot        = dls::math::point3f(0.0f);
	dls::math::point3f position     = dls::math::point3f(0.0f);
	dls::math::point3f echelle      = dls::math::point3f(1.0f);
	dls::math::point3f rotation     = dls::math::point3f(0.0f);
	float echelle_uniforme              = 1.0f;

	/* boîte englobante */
	dls::math::point3f min    = dls::math::point3f(0.0f);
	dls::math::point3f max    = dls::math::point3f(0.0f);
	dls::math::point3f taille = dls::math::point3f(0.0f);

	/* autres propriétés */
	std::string nom = "corps";

	int type = CORPS_NUL;

	using plage_attributs = plage_iterable<std::vector<Attribut *>::iterator>;
	using plage_const_attributs = plage_iterable<std::vector<Attribut *>::const_iterator>;

	Corps() = default;
	virtual ~Corps();

	bool possede_attribut(std::string const &nom_attribut);

	Attribut *ajoute_attribut(
			std::string const &nom_attribut,
			type_attribut type_,
			portee_attr portee = portee_attr::POINT,
			bool force_vide = false);

	void supprime_attribut(std::string const &nom_attribut);

	Attribut *attribut(std::string const &nom_attribut) const;

	size_t ajoute_point(float x, float y, float z);

	void enleve_point(size_t i);

	/**
	 * Retourne l'index du point se trouvant aux coordonnées x, y, z spécifiée.
	 * Si aucun point ne s'y trouve, retourne -1.
	 */
	size_t index_point(float x, float y, float z);

	void ajoute_primitive(Primitive *p);

	ListePoints3D *points();

	const ListePoints3D *points() const;

	ListePrimitives *prims();

	const ListePrimitives *prims() const;

	void reinitialise();

	Corps *copie() const;

	void copie_vers(Corps *corps) const;

	plage_attributs attributs();

	plage_const_attributs attributs() const;

	/* Groupes points. */

	using plage_grp_pnts = plage_iterable<std::vector<GroupePoint *>::iterator>;
	using plage_const_grp_pnts = plage_iterable<std::vector<GroupePoint *>::const_iterator>;

	GroupePoint *ajoute_groupe_point(std::string const &nom_groupe);

	GroupePoint *groupe_point(const std::string &nom_groupe) const;

	plage_grp_pnts groupes_points();

	plage_const_grp_pnts groupes_points() const;

	/* Groupes primitives. */

	using plage_grp_prims = plage_iterable<std::vector<GroupePrimitive *>::iterator>;
	using plage_const_grp_prims = plage_iterable<std::vector<GroupePrimitive *>::const_iterator>;

	GroupePrimitive *ajoute_groupe_primitive(std::string const &nom_groupe);

	plage_grp_prims groupes_prims();

	plage_const_grp_prims groupes_prims() const;


protected:
	std::vector<Attribut *> m_attributs{};

private:
	ListePoints3D m_points{};
	ListePrimitives m_prims{};

	std::vector<GroupePoint *> m_groupes_points{};
	std::vector<GroupePrimitive *> m_groupes_prims{};
};
