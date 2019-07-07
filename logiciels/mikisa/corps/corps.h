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

#include "biblinternes/transformation/transformation.h"

#include "biblinternes/structures/tableau.hh"

#include "attribut.h"
#include "groupes.h"
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
	dls::chaine nom = "corps";

	int type = CORPS_NUL;

	using plage_attributs = dls::outils::plage_iterable<dls::tableau<Attribut *>::iteratrice>;
	using plage_const_attributs = dls::outils::plage_iterable<dls::tableau<Attribut *>::const_iteratrice>;

	Corps() = default;
	virtual ~Corps();

	bool possede_attribut(dls::chaine const &nom_attribut);

	void ajoute_attribut(Attribut *attr);

	Attribut *ajoute_attribut(
			dls::chaine const &nom_attribut,
			type_attribut type_,
			portee_attr portee = portee_attr::POINT,
			bool force_vide = false);

	void supprime_attribut(dls::chaine const &nom_attribut);

	Attribut *attribut(dls::chaine const &nom_attribut) const;

	long ajoute_point(dls::math::vec3f const &pos);

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

	/**
	 * Retourne le point à l'index précisé transformé pour être dans l'espace
	 * mondiale. Aucune vérification de limite n'est effectuée sur l'index. Si
	 * l'index est hors de limite, le programme crashera sans doute.
	 */
	dls::math::vec3f point_transforme(long i) const;

	ListePrimitives *prims();

	const ListePrimitives *prims() const;

	void reinitialise();

	Corps *copie() const;

	void copie_vers(Corps *corps) const;

	plage_attributs attributs();

	plage_const_attributs attributs() const;

	/* Groupes points. */

	using plage_grp_pnts = dls::outils::plage_iterable<dls::tableau<GroupePoint>::iteratrice>;
	using plage_const_grp_pnts = dls::outils::plage_iterable<dls::tableau<GroupePoint>::const_iteratrice>;

	GroupePoint *ajoute_groupe_point(dls::chaine const &nom_groupe);

	GroupePoint *groupe_point(const dls::chaine &nom_groupe) const;

	plage_grp_pnts groupes_points();

	plage_const_grp_pnts groupes_points() const;

	/* Groupes primitives. */

	using plage_grp_prims = dls::outils::plage_iterable<dls::tableau<GroupePrimitive>::iteratrice>;
	using plage_const_grp_prims = dls::outils::plage_iterable<dls::tableau<GroupePrimitive>::const_iteratrice>;

	GroupePrimitive *ajoute_groupe_primitive(dls::chaine const &nom_groupe);

	GroupePrimitive *groupe_primitive(dls::chaine const &nom_groupe) const;

	plage_grp_prims groupes_prims();

	plage_const_grp_prims groupes_prims() const;


protected:
	dls::tableau<Attribut *> m_attributs{};

private:
	ListePoints3D m_points{};
	ListePrimitives m_prims{};

	dls::tableau<GroupePoint> m_groupes_points{};
	dls::tableau<GroupePrimitive> m_groupes_prims{};
};
