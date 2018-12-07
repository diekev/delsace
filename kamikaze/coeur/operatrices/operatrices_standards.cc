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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_standards.h"

#include <delsace/math/bruit.hh>

#include "../bibliotheques/objets/adaptrice_creation.h"
#include "../bibliotheques/objets/creation.h"

#include "sdk/context.h"
#include "sdk/mesh.h"
#include "sdk/primitive.h"
#include "sdk/prim_points.h"
#include "sdk/segmentprim.h"

#include "sdk/outils/géométrie.h"

#include <random>
#include <sstream>

/* ************************************************************************** */

class AdaptriceCreationMaillage final : public objets::AdaptriceCreationObjet {
public:
	void ajoute_sommet(const float x, const float y, const float z, const float w = 1.0f)
	{
		points->push_back(dls::math::vec3f(x, y, z));
	}

	void ajoute_normal(const float x, const float y, const float z) {}

	void ajoute_coord_uv_sommet(const float u, const float v, const float w = 0.0f) {}

	void ajoute_parametres_sommet(const float x, const float y, const float z) {}

	void ajoute_polygone(const int *index_sommet, const int */*index_uv*/, const int */*index_normal*/, size_t nombre)
	{
		if (nombre == 4) {
			polys->push_back(dls::math::vec4i(index_sommet[0], index_sommet[1], index_sommet[2], index_sommet[3]));
		}
		else if (nombre == 3) {
			polys->push_back(dls::math::vec4i(index_sommet[0], index_sommet[1], index_sommet[2], static_cast<int>(INVALID_INDEX)));
		}
	}

	void ajoute_ligne(const int *index, size_t nombre) {}

	void ajoute_objet(const std::string &nom) {}

	void reserve_polygones(const size_t nombre) override {}

	void reserve_sommets(const size_t nombre) override {}

	void reserve_normaux(const size_t nombre) override {}

	void reserve_uvs(const size_t nombre) override {}

	void groupes(const std::vector<std::string> &noms) override {}

	void groupe_nuancage(const int index) override {}

	PointList *points{};
	PolygonList *polys{};
};

/* ************************************************************************** */

static const char *NOM_SORTIE = "Sortie";
static const char *AIDE_SORTIE = "Créer un noeud de sortie.";

OperatriceSortie::OperatriceSortie(Noeud *noeud, const Context &contexte)
	: Operatrice(noeud, contexte)
{
	entrees(1);
}

const char *OperatriceSortie::nom_entree(size_t)
{
	return "Entrée";
}

const char *OperatriceSortie::nom()
{
	return NOM_SORTIE;
}

void OperatriceSortie::execute(const Context &contexte, double temps)
{
	m_collection->free_all();
	entree(0)->requiers_collection(m_collection, contexte, temps);
}

/* ************************************************************************** */

static const char *NOM_CREATION_BOITE = "Création boîte";
static const char *AIDE_CREATION_BOITE = "Créer une boîte.";

class OperatriceCreationBoite final : public Operatrice {
public:
	OperatriceCreationBoite(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("taille", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0));
		ajoute_propriete("centre", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("échelle", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_boite.jo";
	}

	const char *nom_sortie(size_t /*index*/)
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_BOITE;
	}

	void execute(const Context &/*contexte*/, double /*temps*/)
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto echelle = evalue_decimal("échelle");
		const auto taille = evalue_vecteur("taille") * echelle;
		const auto centre = evalue_vecteur("centre");

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_boite(&adaptrice,
						   taille.x, taille.y, taille.z,
						   centre.x, centre.y, centre.z);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_TRANSFORMATION = "Transformation";
static const char *AIDE_TRANSFORMATION = "Transformer les matrices des primitives d'entrées.";

class OperatriceTransformation : public Operatrice {
public:
	OperatriceTransformation(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("ordre_transformation", danjo::TypePropriete::ENUM, std::string("pre"));
		ajoute_propriete("ordre_rotation", danjo::TypePropriete::ENUM, std::string("xyz"));
		ajoute_propriete("translation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("rotation", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("taille", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0));
		ajoute_propriete("pivot", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("échelle", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("inverse", danjo::TypePropriete::BOOL, false);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_transformation.jo";
	}

	const char *nom_entree(size_t /*index*/)
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/)
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_TRANSFORMATION;
	}

	void execute(const Context &contexte, double temps)
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		const auto translate = dls::math::vec3d(evalue_vecteur("translation"));
		const auto rotate = dls::math::vec3d(evalue_vecteur("rotation"));
		const auto scale = dls::math::vec3d(evalue_vecteur("taille"));
		const auto pivot = dls::math::vec3d(evalue_vecteur("pivot"));
		const auto uniform_scale = static_cast<double>(evalue_decimal("échelle"));
		const auto transform_type = evalue_enum("ordre_transformation");
		const auto rot_order = evalue_enum("ordre_rotation");
		auto index_ordre = -1;

		if (rot_order == "xyz") {
			index_ordre = 0;
		}
		else if (rot_order == "xzy") {
			index_ordre = 1;
		}
		else if (rot_order == "yxz") {
			index_ordre = 2;
		}
		else if (rot_order == "yzx") {
			index_ordre = 3;
		}
		else if (rot_order == "zxy") {
			index_ordre = 4;
		}
		else if (rot_order == "zyx") {
			index_ordre = 5;
		}

		/* determine the rotatation order */
		size_t rot_ord[6][3] = {
			{ 0, 1, 2 }, // X Y Z
			{ 0, 2, 1 }, // X Z Y
			{ 1, 0, 2 }, // Y X Z
			{ 1, 2, 0 }, // Y Z X
			{ 2, 0, 1 }, // Z X Y
			{ 2, 1, 0 }, // Z Y X
		};

		dls::math::vec3d axis[3] = {
			dls::math::vec3d(1.0f, 0.0f, 0.0f),
			dls::math::vec3d(0.0f, 1.0f, 0.0f),
			dls::math::vec3d(0.0f, 0.0f, 1.0f),
		};

		const auto X = rot_ord[index_ordre][0];
		const auto Y = rot_ord[index_ordre][1];
		const auto Z = rot_ord[index_ordre][2];

		for (auto &prim : primitive_iterator(this->m_collection)) {
			auto matrix = dls::math::mat4x4d(1.0);
			auto const angle_x = dls::math::degrees_vers_radians(rotate[X]);
			auto const angle_y = dls::math::degrees_vers_radians(rotate[Y]);
			auto const angle_z = dls::math::degrees_vers_radians(rotate[Z]);

			if (transform_type == "pre") {
				matrix = dls::math::pre_translation(matrix, pivot);
				matrix = dls::math::pre_rotation(matrix, angle_x, axis[X]);
				matrix = dls::math::pre_rotation(matrix, angle_y, axis[Y]);
				matrix = dls::math::pre_rotation(matrix, angle_z, axis[Z]);
				matrix = dls::math::pre_dimension(matrix, scale * uniform_scale);
				matrix = dls::math::pre_translation(matrix, -pivot);
				matrix = dls::math::pre_translation(matrix, translate);
				matrix = matrix * prim->matrix();
			}
			else {
				matrix = dls::math::post_translation(matrix, pivot);
				matrix = dls::math::post_rotation(matrix, angle_x, axis[X]);
				matrix = dls::math::post_rotation(matrix, angle_y, axis[Y]);
				matrix = dls::math::post_rotation(matrix, angle_z, axis[Z]);
				matrix = dls::math::post_dimension(matrix, scale * uniform_scale);
				matrix = dls::math::post_translation(matrix, -pivot);
				matrix = dls::math::post_translation(matrix, translate);
				matrix = prim->matrix() * matrix;
			}

			prim->matrix(matrix);
		}
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_TORUS = "Création torus";
static const char *AIDE_CREATION_TORUS = "Créer un torus.";

class OperatriceCreationTorus : public Operatrice {
public:
	OperatriceCreationTorus(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("centre", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("rayon_majeur", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("rayon_mineur", danjo::TypePropriete::DECIMAL, 0.25f);
		ajoute_propriete("segments_majeurs", danjo::TypePropriete::ENTIER, 48);
		ajoute_propriete("segments_mineurs", danjo::TypePropriete::ENTIER, 24);
		ajoute_propriete("échelle", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_torus.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_TORUS;
	}

	void execute(const Context &/*contexte*/, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto centre = evalue_vecteur("centre");
		const auto echelle = evalue_decimal("échelle");

		const auto rayon_mineur = evalue_decimal("rayon_mineur") * echelle;
		const auto rayon_majeur = evalue_decimal("rayon_majeur") * echelle;
		const auto segment_mineur = static_cast<size_t>(evalue_entier("segments_mineurs"));
		const auto segment_majeur = static_cast<size_t>(evalue_entier("segments_majeurs"));

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_torus(&adaptrice,
						   rayon_mineur, rayon_majeur,
						   segment_mineur, segment_majeur,
						   centre.x, centre.y, centre.z);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_GRILLE = "Création grille";
static const char *AIDE_CREATION_GRILLE = "Créer une grille.";

class OperatriceCreationGrille : public Operatrice {
public:
	OperatriceCreationGrille(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("centre", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0));
		ajoute_propriete("taille", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0));
		ajoute_propriete("lignes", danjo::TypePropriete::ENTIER, 2);
		ajoute_propriete("colonnes", danjo::TypePropriete::ENTIER, 2);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_grille.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_GRILLE;
	}

	void execute(const Context &/*contexte*/, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto taille = evalue_vecteur("taille");
		const auto centre = evalue_vecteur("centre");

		const auto lignes = static_cast<size_t>(evalue_entier("lignes"));
		const auto colonnes = static_cast<size_t>(evalue_entier("colonnes"));

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_grille(&adaptrice,
							taille.x, taille.y,
							lignes,
							colonnes,
							centre.x, centre.y, centre.z);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_CERCLE = "Création cercle";
static const char *AIDE_CREATION_CERCLE = "Créer un cercle.";

class OperatriceCreationCercle : public Operatrice {
public:
	OperatriceCreationCercle(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("vertices", danjo::TypePropriete::ENTIER, 32);
		ajoute_propriete("rayon", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_cercle.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_CERCLE;
	}

	void execute(const Context &contexte, double /*temps*/) override
	{
		if (m_collection == nullptr) {
			m_collection = new PrimitiveCollection(contexte.primitive_factory);
		}

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto segments = static_cast<size_t>(evalue_entier("vertices"));
		const auto rayon = evalue_decimal("rayon");

		/* À FAIRE : centre */

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_cercle(&adaptrice, segments, rayon);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_TUBE = "Création tube";
static const char *AIDE_CREATION_TUBE = "Créer un tube.";

class OperatriceCreationTube : public Operatrice {
public:
	OperatriceCreationTube(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("vertices", danjo::TypePropriete::ENTIER, 32);
		ajoute_propriete("rayon", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("profondeur", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_tube.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_TUBE;
	}

	void execute(const Context &/*contexte*/, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto segments = static_cast<size_t>(evalue_entier("vertices"));
		const auto rayon = evalue_decimal("rayon");
		const auto profondeur = evalue_decimal("profondeur");

		/* À FAIRE : centre */

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_cylindre(&adaptrice, segments, rayon, rayon, profondeur);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_CONE = "Création cone";
static const char *AIDE_CREATION_CONE = "Créer un cone.";

class OperatriceCreationCone : public Operatrice {
public:
	OperatriceCreationCone(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("vertices", danjo::TypePropriete::ENTIER, 32);
		ajoute_propriete("rayon_mineur", danjo::TypePropriete::DECIMAL, 0.0f);
		ajoute_propriete("rayon_majeur", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("profondeur", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_cone.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_CONE;
	}

	void execute(const Context &/*contexte*/, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto segments = static_cast<size_t>(evalue_entier("vertices"));
		const auto rayon1 = evalue_decimal("rayon_mineur");
		const auto rayon2 = evalue_decimal("rayon_majeur");
		const auto profondeur = evalue_decimal("profondeur");

		/* À FAIRE : centre */

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_cylindre(&adaptrice, segments, rayon2, rayon1, profondeur);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_ICOSPHERE = "Création icosphère";
static const char *AIDE_CREATION_ICOSPHERE = "Crées une icosphère.";

class OperatriceCreationIcoSphere : public Operatrice {
public:
	OperatriceCreationIcoSphere(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("rayon", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_icosphere.jo";
	}

	const char *nom() override
	{
		return NOM_CREATION_ICOSPHERE;
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	void execute(const Context &contexte, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("Mesh");
		auto mesh = static_cast<Mesh *>(prim);

		const auto rayon = evalue_decimal("rayon");

		/* À FAIRE : centre */

		AdaptriceCreationMaillage adaptrice;
		adaptrice.points = mesh->points();
		adaptrice.polys = mesh->polys();

		objets::cree_icosphere(&adaptrice, rayon);

		mesh->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_NORMAL = "Normal";
static const char *AIDE_NORMAL = "Éditer les normales.";

class OperatriceNormal : public Operatrice {
public:
	OperatriceNormal(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("inverse", danjo::TypePropriete::BOOL, false);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_normal.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_NORMAL;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		const auto flip = evalue_bool("inverse");

		for (auto &prim : primitive_iterator(this->m_collection, Mesh::id)) {
			auto mesh = static_cast<Mesh *>(prim);
			auto normals = mesh->attribute("normal", ATTR_TYPE_VEC3);
			auto points = mesh->points();

			normals->resize(points->size());

			auto polys = mesh->polys();

			for (size_t i = 0, ie = points->size(); i < ie ; ++i) {
				normals->vec3(i, dls::math::vec3f(0.0f));
			}

			calcule_normales(*points, *polys, *normals, flip);
		}
	}
};

/* ************************************************************************** */

static const char *NOM_BRUIT = "Bruit";
static const char *AIDE_BRUIT = "Ajouter du bruit.";

class OperatriceBruitage : public Operatrice {
	dls::math::BruitPerlin3D m_bruit_perlin{};
	dls::math::BruitFlux3D m_bruit_flux{};

public:
	OperatriceBruitage(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("bruit", danjo::TypePropriete::ENUM, std::string("simplex"));
		ajoute_propriete("direction", danjo::TypePropriete::ENUM, std::string("x"));
		ajoute_propriete("taille", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("octaves", danjo::TypePropriete::ENTIER, 1.0f);
		ajoute_propriete("frequence", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("amplitude", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("persistence", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("lacunarité", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("temps", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_bruit.jo";
	}

	bool ajourne_proprietes() override
	{
		const auto bruit = evalue_enum("bruit");

		rend_propriete_visible("temps", bruit == "flux");

		return true;
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_BRUIT;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		const auto taille = evalue_decimal("taille");
		const auto taille_inverse = (taille > 0.0f) ? 1.0f / taille : 0.0f;
		const auto octaves = static_cast<size_t>(evalue_entier("octaves"));
		const auto lacunarity = evalue_decimal("lacunarité");
		const auto persistence = evalue_decimal("persistence");
		const auto ofrequency = evalue_decimal("frequence");
		const auto oamplitude = evalue_decimal("amplitude");
		const auto direction = evalue_enum("direction");
		const auto bruit = evalue_enum("bruit");

		if (bruit == "flux") {
			const auto temps_bruit = evalue_decimal("temps");
			m_bruit_flux.change_temps(temps_bruit);
		}

		for (auto prim : primitive_iterator(this->m_collection)) {
			PointList *points;

			Attribute *normales = nullptr;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);
				points = mesh->points();
				normales = mesh->attribute("normal", ATTR_TYPE_VEC3);

				if (direction == "normal" && (normales == nullptr || normales->size() == 0)) {
					this->ajoute_avertissement("Absence de normales pour calculer le bruit !");
					continue;
				}
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto prim_points = static_cast<PrimPoints *>(prim);
				points = prim_points->points();

				if (direction == "normal") {
					this->ajoute_avertissement("On ne peut calculer le bruit suivant la normale sur un nuage de points !");
					continue;
				}
			}
			else {
				continue;
			}

			for (size_t i = 0, e = points->size(); i < e; ++i) {
				auto &point = (*points)[i];
				const auto x = point.x * taille_inverse;
				const auto y = point.y * taille_inverse;
				const auto z = point.z * taille_inverse;
				auto valeur = 0.0f;

				auto frequency = ofrequency;
				auto amplitude = oamplitude;

				for (size_t j = 0; j < octaves; ++j) {
					if (bruit == "simplex") {
						valeur += (amplitude * dls::math::bruit_simplex_3d(x * frequency, y * frequency, z * frequency));
					}
					else if (bruit == "perlin") {
						valeur += (amplitude * m_bruit_perlin(x * frequency, y * frequency, z * frequency));
					}
					else {
						valeur += (amplitude * m_bruit_flux(x * frequency, y * frequency, z * frequency));
					}

					frequency *= lacunarity;
					amplitude *= persistence;
				}

				if (direction == "x") {
					point.x += valeur;
				}
				else if (direction == "y") {
					point.y += valeur;
				}
				else if (direction == "z") {
					point.z += valeur;
				}
				else if (direction == "toutes") {
					point.x += valeur;
					point.y += valeur;
					point.z += valeur;
				}
				else if (direction == "normal") {
					const auto normale = normales->vec3(i);
					point.x += valeur * normale.x;
					point.y += valeur * normale.y;
					point.z += valeur * normale.z;
				}
			}
		}
	}
};

/* ************************************************************************** */

static const char *NOM_COULEUR = "Couleur";
static const char *AIDE_COULEUR = "Ajouter de la couleur.";

class OperatriceCouleur : public Operatrice {
public:
	OperatriceCouleur(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("portée", danjo::TypePropriete::ENUM, std::string("vertices"));
		ajoute_propriete("méthode", danjo::TypePropriete::ENUM, std::string("unique"));
		ajoute_propriete("couleur", danjo::TypePropriete::COULEUR, dls::math::vec3f(0.5f, 0.5f, 0.5f));
		ajoute_propriete("graine", danjo::TypePropriete::ENTIER, 1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_couleur.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_COULEUR;
	}

	bool ajourne_proprietes() override
	{
		auto method = evalue_enum("méthode");

		if (method == "unique") {
			rend_propriete_visible("couleur", true);
			rend_propriete_visible("graine", false);
		}
		else if (method == "aléatoire") {
			rend_propriete_visible("graine", true);
			rend_propriete_visible("couleur", false);
		}

		return true;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		const auto &methode = evalue_enum("méthode");
		const auto &portee = evalue_enum("portée");
		const auto &graine = evalue_entier("graine");

		std::mt19937 rng(static_cast<size_t>(19937 + graine));
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		for (auto prim : primitive_iterator(this->m_collection)) {
			Attribute *colors;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);
				colors = mesh->add_attribute("color", ATTR_TYPE_VEC4, mesh->points()->size());
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto prim_points = static_cast<PrimPoints *>(prim);
				colors = prim_points->add_attribute("color", ATTR_TYPE_VEC4, prim_points->points()->size());
			}
			else {
				continue;
			}

			if (methode == "unique") {
				const auto &color = evalue_couleur("color");

				for (size_t i = 0, e = colors->size(); i < e; ++i) {
					colors->vec4(i, dls::math::vec4f(color.r, color.v, color.b, color.a));
				}
			}
			else if (methode == "aléatoire") {
				if (portee == "vertices") {
					for (size_t i = 0, e = colors->size(); i < e; ++i) {
						colors->vec4(i, dls::math::vec4f{dist(rng), dist(rng), dist(rng), 1.0f});
					}
				}
				else if (portee == "primitive") {
					const auto &color = dls::math::vec4f{dist(rng), dist(rng), dist(rng), 1.0f};

					for (size_t i = 0, e = colors->size(); i < e; ++i) {
						colors->vec4(i, color);
					}
				}
			}
		}
	}
};

/* ************************************************************************** */

static const char *NOM_FUSION_COLLECTION = "Fusion collections";
static const char *AIDE_FUSION_COLLECTION = "Fusionner des collections.";

class OperatriceFusionCollection : public Operatrice {
public:
	OperatriceFusionCollection(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(2);
		sorties(1);
	}

	const char *nom_entree(size_t index) override
	{
		if (index == 0) {
			return "Entrée 1";
		}

		return "Entrée 2";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_FUSION_COLLECTION;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto collection2 = this->entree(1)->requiers_collection(nullptr, contexte, temps);

		if (collection2 == nullptr) {
			return;
		}

		m_collection->merge_collection(*collection2);
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_NUAGE_POINT = "Création nuage point";
static const char *AIDE_CREATION_NUAGE_POINT = "Création d'un nuage de point.";

class OperatriceCreationNuagePoint : public Operatrice {
public:
	OperatriceCreationNuagePoint(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		sorties(1);

		ajoute_propriete("nombre_points", danjo::TypePropriete::ENTIER, 1000);
		ajoute_propriete("limite_min", danjo::TypePropriete::VECTEUR, dls::math::vec3f(-1.0));
		ajoute_propriete("limite_max", danjo::TypePropriete::VECTEUR, dls::math::vec3f(1.0));
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_nuage_point.jo";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_NUAGE_POINT;
	}

	void execute(const Context &contexte, double /*temps*/) override
	{
		m_collection->free_all();

		auto prim = m_collection->build("PrimPoints");
		auto points = static_cast<PrimPoints *>(prim);

		const auto &nombre_points = static_cast<size_t>(evalue_entier("nombre_points"));

		auto point_list = points->points();
		point_list->resize(nombre_points);

		const auto &limite_min = evalue_vecteur("limite_min");
		const auto &limite_max = evalue_vecteur("limite_max");

		std::uniform_real_distribution<float> dist_x(limite_min[0], limite_max[0]);
		std::uniform_real_distribution<float> dist_y(limite_min[1], limite_max[1]);
		std::uniform_real_distribution<float> dist_z(limite_min[2], limite_max[2]);
		std::mt19937 rng_x(19937);
		std::mt19937 rng_y(19937 + 1);
		std::mt19937 rng_z(19937 + 2);

		for (size_t i = 0; i < nombre_points; ++i) {
			const auto &point = dls::math::vec3f(dist_x(rng_x), dist_y(rng_y), dist_z(rng_z));
			(*point_list)[i] = point;
		}

		points->tagUpdate();
	}
};

/* ************************************************************************** */

static const char *NOM_CREATION_ATTRIBUT = "Création attribut";
static const char *AIDE_CREATION_ATTRIBUT = "Création d'un attribut.";

static AttributeType type_attribut_depuis_chaine(const std::string &chaine)
{
	if (chaine == "octet") {
		return ATTR_TYPE_BYTE;
	}
	else if (chaine == "entier") {
		return ATTR_TYPE_INT;
	}
	else if (chaine == "décimal") {
		return ATTR_TYPE_FLOAT;
	}
	else if (chaine == "chaine") {
		return ATTR_TYPE_STRING;
	}
	else if (chaine == "vecteur_2d") {
		return ATTR_TYPE_VEC2;
	}
	else if (chaine == "vecteur_3d") {
		return ATTR_TYPE_VEC3;
	}
	else if (chaine == "vecteur_4d") {
		return ATTR_TYPE_VEC4;
	}
	else if (chaine == "matrice_3x3") {
		return ATTR_TYPE_MAT3;
	}
	else if (chaine == "matrice_4x4") {
		return ATTR_TYPE_MAT4;
	}

	return ATTR_TYPE_INT;
}

class OperatriceCreationAttribut : public Operatrice {
public:
	OperatriceCreationAttribut(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("nom_attribut", danjo::TypePropriete::CHAINE_CARACTERE, std::string(""));
		ajoute_propriete("type_attribut", danjo::TypePropriete::ENUM, std::string("octet"));
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_suppression_attribut.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_ATTRIBUT;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto nom_attribut = evalue_chaine("nom_attribut");
		auto type_attribut = evalue_enum("type_attribut");
		auto type = type_attribut_depuis_chaine(type_attribut);

		for (Primitive *prim : primitive_iterator(m_collection)) {
			if (prim->has_attribute(nom_attribut, type)) {
				std::stringstream ss;
				ss << prim->name() << " already has an attribute named " << nom_attribut;

				this->ajoute_avertissement(ss.str());
				continue;
			}

			size_t attrib_size = 1;

			if (prim->typeID() == Mesh::id) {
				auto mesh = static_cast<Mesh *>(prim);

				if (type_attribut == "vecteur_3d") {
					attrib_size = mesh->points()->size();
				}
			}
			else if (prim->typeID() == PrimPoints::id) {
				auto point_cloud = static_cast<PrimPoints *>(prim);

				if (type_attribut == "vecteur_3d") {
					attrib_size = point_cloud->points()->size();
				}
			}

			prim->add_attribute(nom_attribut, type, attrib_size);
		}
	}
};

/* ************************************************************************** */

static const char *NOM_SUPPRESSION_ATTRIBUT = "Suppression attribut";
static const char *AIDE_SUPPRESSION_ATTRIBUT = "Suppression d'un attribut.";

class OperatriceSuppressionAttribut : public Operatrice {
public:
	OperatriceSuppressionAttribut(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("nom_attribut", danjo::TypePropriete::CHAINE_CARACTERE, std::string(""));
		ajoute_propriete("type_attribut", danjo::TypePropriete::ENUM, std::string("octet"));
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_suppression_attribut.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_SUPPRESSION_ATTRIBUT;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto nom_attribut = evalue_chaine("nom_attribut");
		auto type_attribut = evalue_enum("type_attribut");
		auto type = type_attribut_depuis_chaine(type_attribut);

		for (Primitive *prim : primitive_iterator(m_collection)) {
			if (!prim->has_attribute(nom_attribut, type)) {
				continue;
			}

			prim->remove_attribute(nom_attribut, type);
		}
	}
};

/* ************************************************************************** */

static const char *NOM_RANDOMISATION_ATTRIBUT = "Randomisation attribut";
static const char *AIDE_RANDOMISATION_ATTRIBUT = "Randomisation d'un attribut.";

/* À FAIRE : distribution exponentielle, lognormal, cauchy, discrète */

class OperatriceRandomisationAttribut : public Operatrice {
public:
	OperatriceRandomisationAttribut(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("nom_attribut", danjo::TypePropriete::CHAINE_CARACTERE, std::string(""));
		ajoute_propriete("type_attribut", danjo::TypePropriete::ENUM, std::string("octet"));
		ajoute_propriete("distribution", danjo::TypePropriete::ENUM, std::string("constant"));
		ajoute_propriete("valeur", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("valeur_min", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("valeur_max", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("moyenne", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("écart_type", danjo::TypePropriete::DECIMAL, 1.0f);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_randomise_attribut.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_RANDOMISATION_ATTRIBUT;
	}

	bool ajourne_proprietes() override
	{
		const auto distribution = evalue_enum("distribution");

		rend_propriete_visible("valeur", distribution == "constante");
		rend_propriete_visible("min_value", distribution == "uniforme");
		rend_propriete_visible("max_value", distribution == "uniforme");
		rend_propriete_visible("mean", distribution == "gaussienne");
		rend_propriete_visible("stddev", distribution == "gaussienne");

		return true;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto name = evalue_chaine("nom_attribut");
		auto type_attribut = evalue_enum("type_attribut");
		auto distribution = evalue_enum("distribution");
		auto value = evalue_decimal("valeur");
		auto min_value = evalue_decimal("valeur_min");
		auto max_value = evalue_decimal("valeur_max");
		auto mean = evalue_decimal("moyenne");
		auto stddev = evalue_decimal("écart_type");
		auto type = type_attribut_depuis_chaine(type_attribut);

		std::mt19937 rng(19993754);

		if (type != ATTR_TYPE_VEC3) {
			std::stringstream ss;
			ss << "Only 3D Vector attributes are supported for now!";

			this->ajoute_avertissement(ss.str());
			return;
		}

		for (Primitive *prim : primitive_iterator(m_collection)) {
			auto attribute = prim->attribute(name, type);

			if (!attribute) {
				std::stringstream ss;
				ss << prim->name() << " does not have an attribute named \"" << name
				   << "\" of type " << static_cast<int>(type);

				this->ajoute_avertissement(ss.str());
				continue;
			}

			if (distribution == "constante") {
				for (size_t i = 0; i < attribute->size(); ++i) {
					attribute->vec3(i, dls::math::vec3f{value, value, value});
				}
			}
			else if (distribution == "uniforme") {
				std::uniform_real_distribution<float> dist(min_value, max_value);

				for (size_t i = 0; i < attribute->size(); ++i) {
					attribute->vec3(i, dls::math::vec3f{dist(rng), dist(rng), dist(rng)});
				}
			}
			else if (distribution == "gaussienne") {
				std::normal_distribution<float> dist(mean, stddev);

				for (size_t i = 0; i < attribute->size(); ++i) {
					attribute->vec3(i, dls::math::vec3f{dist(rng), dist(rng), dist(rng)});
				}

			}
		}
	}
};

/* ************************************************************************** */

struct Triangle {
	dls::math::vec3f v0{}, v1{}, v2{};
};

std::vector<Triangle> convertis_maillage_triangles(const Mesh *maillage_entree)
{
	std::vector<Triangle> triangles;
	const auto points = maillage_entree->points();
	const auto polygones = maillage_entree->polys();

	/* Convertis le maillage en triangles. */
	auto nombre_triangles = 0ul;

	for (auto i = 0ul; i < polygones->size(); ++i) {
		const auto polygone = (*polygones)[i];

		nombre_triangles += static_cast<size_t>((polygone[3] == static_cast<int>(INVALID_INDEX)) ? 1 : 2);
	}

	triangles.reserve(nombre_triangles);

	for (auto i = 0ul; i < polygones->size(); ++i) {
		const auto polygone = (*polygones)[i];

		Triangle triangle;
		triangle.v0 = (*points)[static_cast<size_t>(polygone[0])];
		triangle.v1 = (*points)[static_cast<size_t>(polygone[1])];
		triangle.v2 = (*points)[static_cast<size_t>(polygone[2])];

		triangles.push_back(triangle);

		if (polygone[3] != static_cast<int>(INVALID_INDEX)) {
			Triangle triangle2;
			triangle2.v0 = (*points)[static_cast<size_t>(polygone[0])];
			triangle2.v1 = (*points)[static_cast<size_t>(polygone[2])];
			triangle2.v2 = (*points)[static_cast<size_t>(polygone[3])];

			triangles.push_back(triangle2);
		}
	}

	return triangles;
}

static const char *NOM_CREATION_SEGMENTS = "Création courbes";
static const char *AIDE_CREATION_SEGMENTS = "Création de courbes.";

class OperatriceCreationCourbes : public Operatrice {
public:
	OperatriceCreationCourbes(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("méthode", danjo::TypePropriete::ENUM, std::string("vertices"));
		ajoute_propriete("graine", danjo::TypePropriete::ENTIER, 1);
		ajoute_propriete("nombre_courbes", danjo::TypePropriete::ENTIER, 100);
		ajoute_propriete("segments", danjo::TypePropriete::ENTIER, 1);
		ajoute_propriete("taille", danjo::TypePropriete::DECIMAL, 1.0f);
		ajoute_propriete("direction", danjo::TypePropriete::ENUM, std::string("normal"));
		ajoute_propriete("normal", danjo::TypePropriete::VECTEUR, dls::math::vec3f(0.0f, 1.0f, 0.0f));
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_courbes.jo";
	}

	bool ajourne_proprietes() override
	{
		auto methode = evalue_enum("méthode");
		rend_propriete_visible("graine", methode == "polygones");
		rend_propriete_visible("nombre_courbes", methode == "polygones");

		const auto direction = evalue_enum("direction");
		rend_propriete_visible("normale", direction == "personnalisée");

		return true;
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_CREATION_SEGMENTS;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto iter = primitive_iterator(m_collection, Mesh::id);

		if (iter.get() == nullptr) {
			this->ajoute_avertissement("No input mesh found!");
			return;
		}

		const auto input_mesh = static_cast<Mesh *>(iter.get());
		const auto input_points = input_mesh->points();

		const auto segment_number = static_cast<size_t>(evalue_entier("segments"));
		const auto segment_normal = evalue_vecteur("normal");
		const auto segment_size = evalue_decimal("taille");
		const auto methode = evalue_enum("méthode");
		const auto direction = evalue_enum("direction");

		auto segment_prim = static_cast<SegmentPrim *>(m_collection->build("SegmentPrim"));
		auto output_edges = segment_prim->edges();
		auto output_points = segment_prim->points();

		auto num_points = 0ul;
		auto total_points = 0ul;

		const auto direction_normal = (direction == "normal");

		if (methode == "vertices") {
			Attribute *normales = nullptr;

			if (direction_normal) {
				normales = input_mesh->attribute("normal", ATTR_TYPE_VEC3);

				if (normales == nullptr || normales->size() == 0) {
					this->ajoute_avertissement("Il n'y a pas de données de normales sur les vertex d'entrées !");
					return;
				}
			}

			total_points = input_points->size() * (segment_number + 1);

			output_edges->reserve(input_points->size() * segment_number);
			output_points->reserve(total_points);
			auto head = 0;
			dls::math::vec3f normale;

			for (size_t i = 0; i < input_points->size(); ++i) {
				auto point = (*input_points)[i];
				normale = direction_normal ? normales->vec3(i) : segment_normal;

				output_points->push_back(point);
				++num_points;

				for (size_t j = 0; j < segment_number; ++j, ++num_points) {
					point += (segment_size * normale);
					output_points->push_back(point);

					output_edges->push_back(dls::math::vec2i{head, ++head});
				}

				++head;
			}
		}
		else if (methode == "polygones") {
			auto triangles = convertis_maillage_triangles(input_mesh);

			const auto nombre_courbes = static_cast<size_t>(evalue_entier("nombre_courbes"));
			const auto nombre_polys = triangles.size();

			output_edges->reserve((nombre_courbes * segment_number) * nombre_polys);
			output_points->reserve(total_points);

			total_points = nombre_polys * nombre_courbes * (segment_number + 1);

			const auto graine = evalue_entier("graine");

			std::mt19937 rng(static_cast<size_t>(19937 + graine));
			std::uniform_real_distribution<float> dist(0.0f, 1.0f);

			auto head = 0;

			dls::math::vec3f normale;

			for (const Triangle &triangle : triangles) {
				const auto v0 = triangle.v0;
				const auto v1 = triangle.v1;
				const auto v2 = triangle.v2;

				const auto e0 = v1 - v0;
				const auto e1 = v2 - v0;

				normale = (direction_normal) ? dls::math::normalise(normale_triangle(v0, v1, v2)) : segment_normal;

				for (size_t j = 0; j < nombre_courbes; ++j) {
					/* Génère des coordonnées barycentriques aléatoires. */
					auto r = dist(rng);
					auto s = dist(rng);

					if (r + s >= 1.0f) {
						r = 1.0f - r;
						s = 1.0f - s;
					}

					auto pos = v0 + r * e0 + s * e1;

					output_points->push_back(pos);
					++num_points;

					for (size_t k = 0; k < segment_number; ++k, ++num_points) {
						pos += (segment_size * normale);
						output_points->push_back(pos);

						output_edges->push_back(dls::math::vec2i{head, ++head});
					}

					++head;
				}
			}
		}

		if (num_points != total_points) {
			std::stringstream ss;

			if (num_points < total_points) {
				ss << "Overallocation of points, allocated: " << total_points
				   << ", final total: " << num_points;
			}
			else if (num_points > total_points) {
				ss << "Underallocation of points, allocated: " << total_points
				   << ", final total: " << num_points;
			}

			this->ajoute_avertissement(ss.str());
		}
	}
};


/* ************************************************************************** */

static const char *NOM_TAMPON = "Tampon";
static const char *AIDE_TAMPON = "Met la collection d'entrée dans un tampon pour ne plus la recalculer.";

class OperatriceTampon : public Operatrice {
	PrimitiveCollection *m_collecion_tampon = nullptr;

public:
	OperatriceTampon(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		m_collecion_tampon = new PrimitiveCollection(contexte.primitive_factory);
	}

	OperatriceTampon(OperatriceTampon const &) = default;
	OperatriceTampon &operator=(OperatriceTampon const &) = default;

	~OperatriceTampon() override
	{
		delete m_collecion_tampon;
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_TAMPON;
	}

	void execute(const Context &contexte, double temps) override
	{
		/* À FAIRE : généraliser à tous les noeuds + meilleur solution. */
		m_collection->free_all();

		if (this->besoin_execution()) {
			m_collecion_tampon->free_all();
			entree(0)->requiers_collection(m_collecion_tampon, contexte, temps);
		}

		auto collection_temporaire = m_collecion_tampon->copy();
		m_collection->merge_collection(*collection_temporaire);
		delete collection_temporaire;
	}
};

/* ************************************************************************** */

static const char *NOM_COMMUTATEUR = "Commutateur";
static const char *AIDE_COMMUTATEUR = "Change la direction de l'évaluation du"
									  "graphe en fonction de la prise choisie.";

class OperatriceCommutateur : public Operatrice {
public:
	OperatriceCommutateur(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(2);
		sorties(1);

		ajoute_propriete("prise", danjo::TypePropriete::ENTIER, 0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_commutateur.jo";
	}

	const char *nom_entree(size_t index) override
	{
		if (index == 0) {
			return "Entrée 0";
		}

		return "Entrée 1";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_COMMUTATEUR;
	}

	void execute(const Context &contexte, double temps) override
	{
		const auto prise = static_cast<size_t>(evalue_entier("prise"));
		entree(prise)->requiers_collection(m_collection, contexte, temps);
	}
};

/* ************************************************************************** */

static const char *NOM_DISPERSION_POINTS = "Dispersion Points";
static const char *AIDE_DISPERSION_POINTS = "Disperse des points sur une surface.";

class OperatriceDispersionPoints : public Operatrice {
public:
	OperatriceDispersionPoints(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("graine", danjo::TypePropriete::ENTIER, 1);
		ajoute_propriete("nombre_points_polys", danjo::TypePropriete::ENTIER, 100);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dispersion_points.jo";
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		return NOM_DISPERSION_POINTS;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);

		auto iter = primitive_iterator(m_collection, Mesh::id);

		if (iter.get() == nullptr) {
			this->ajoute_avertissement("Il n'y a pas de mesh dans la collecion d'entrée !");
			return;
		}

		const auto maillage_entree = static_cast<Mesh *>(iter.get());

		auto triangles = convertis_maillage_triangles(maillage_entree);

		auto nuage_points = static_cast<PrimPoints *>(m_collection->build("PrimPoints"));
		auto points_sorties = nuage_points->points();

		const auto nombre_points_polys = static_cast<size_t>(evalue_entier("nombre_points_polys"));
		const auto nombre_points = triangles.size() * nombre_points_polys;

		points_sorties->reserve(nombre_points);

		const auto graine = evalue_entier("graine");

		std::mt19937 rng(static_cast<size_t>(19937 + graine));
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		for (const Triangle &triangle : triangles) {
			const auto v0 = triangle.v0;
			const auto v1 = triangle.v1;
			const auto v2 = triangle.v2;

			const auto e0 = v1 - v0;
			const auto e1 = v2 - v0;

			for (size_t j = 0; j < nombre_points_polys; ++j) {
				/* Génère des coordonnées barycentriques aléatoires. */
				auto r = dist(rng);
				auto s = dist(rng);

				if (r + s >= 1.0f) {
					r = 1.0f - r;
					s = 1.0f - s;
				}

				auto pos = v0 + r * e0 + s * e1;

				points_sorties->push_back(pos);
			}
		}
	}
};

/* ************************************************************************** */
#if 0
static const char *NOM_ = "";
static const char *AIDE_ = "";

class OperatriceModele : public Operatrice {
public:
	OperatriceModele(Noeud *noeud, const Context &contexte)
		: Operatrice(noeud, contexte)
	{
		entrees(1);
		sorties(1);
	}

	const char *nom_entree(size_t /*index*/) override
	{
		return "Entrée";
	}

	const char *nom_sortie(size_t /*index*/) override
	{
		return "Sortie";
	}

	const char *nom() override
	{
		retrun NOM_;
	}

	void execute(const Context &contexte, double temps) override
	{
		entree(0)->requiers_collection(m_collection, contexte, temps);
	}
};
#endif

/* ************************************************************************** */

void enregistre_operatrices_integres(UsineOperatrice *usine)
{
	/* Opérateurs géométrie. */

	auto categorie = "Géométrie";

	usine->enregistre_type(NOM_CREATION_BOITE,
						   cree_description<OperatriceCreationBoite>(NOM_CREATION_BOITE,
																	AIDE_CREATION_BOITE,
																	categorie));

	usine->enregistre_type(NOM_CREATION_TORUS,
						   cree_description<OperatriceCreationTorus>(NOM_CREATION_TORUS,
																	AIDE_CREATION_TORUS,
																	categorie));

	usine->enregistre_type(NOM_CREATION_CERCLE,
						   cree_description<OperatriceCreationCercle>(NOM_CREATION_CERCLE,
																	 AIDE_CREATION_CERCLE,
																	 categorie));

	usine->enregistre_type(NOM_CREATION_GRILLE,
						   cree_description<OperatriceCreationGrille>(NOM_CREATION_GRILLE,
																	 AIDE_CREATION_GRILLE,
																	 categorie));

	usine->enregistre_type(NOM_CREATION_TUBE,
						   cree_description<OperatriceCreationTube>(NOM_CREATION_TUBE,
																   AIDE_CREATION_TUBE,
																   categorie));

	usine->enregistre_type(NOM_CREATION_CONE,
						   cree_description<OperatriceCreationCone>(NOM_CREATION_CONE,
																   AIDE_CREATION_CONE,
																   categorie));

	usine->enregistre_type(NOM_CREATION_ICOSPHERE,
						   cree_description<OperatriceCreationIcoSphere>(NOM_CREATION_ICOSPHERE,
																		AIDE_CREATION_ICOSPHERE,
																		categorie));

	usine->enregistre_type(NOM_TRANSFORMATION,
						   cree_description<OperatriceTransformation>(NOM_TRANSFORMATION,
																	 AIDE_TRANSFORMATION,
																	 categorie));

	usine->enregistre_type(NOM_NORMAL,
						   cree_description<OperatriceNormal>(NOM_NORMAL,
															 AIDE_NORMAL,
															 categorie));

	usine->enregistre_type(NOM_BRUIT,
						   cree_description<OperatriceBruitage>(NOM_BRUIT,
															AIDE_BRUIT,
															categorie));

	usine->enregistre_type(NOM_COULEUR,
						   cree_description<OperatriceCouleur>(NOM_COULEUR,
															  AIDE_COULEUR,
															  categorie));

	usine->enregistre_type(NOM_FUSION_COLLECTION,
						   cree_description<OperatriceFusionCollection>(NOM_FUSION_COLLECTION,
																	   AIDE_FUSION_COLLECTION,
																	   categorie));

	usine->enregistre_type(NOM_CREATION_NUAGE_POINT,
						   cree_description<OperatriceCreationNuagePoint>(NOM_CREATION_NUAGE_POINT,
																		 AIDE_CREATION_NUAGE_POINT,
																		 categorie));

	usine->enregistre_type(NOM_CREATION_SEGMENTS,
						   cree_description<OperatriceCreationCourbes>(NOM_CREATION_SEGMENTS,
																	   AIDE_CREATION_SEGMENTS,
																	   categorie));

	usine->enregistre_type(NOM_DISPERSION_POINTS,
						   cree_description<OperatriceDispersionPoints>(NOM_DISPERSION_POINTS,
																	   AIDE_DISPERSION_POINTS,
																	   categorie));

	/* Opérateurs attributs. */

	categorie = "Attributs";

	usine->enregistre_type(NOM_CREATION_ATTRIBUT,
						   cree_description<OperatriceCreationAttribut>(NOM_CREATION_ATTRIBUT,
																	   AIDE_CREATION_ATTRIBUT,
																	   categorie));

	usine->enregistre_type(NOM_SUPPRESSION_ATTRIBUT,
						   cree_description<OperatriceSuppressionAttribut>(NOM_SUPPRESSION_ATTRIBUT,
																		  AIDE_SUPPRESSION_ATTRIBUT,
																		  categorie));

	usine->enregistre_type(NOM_RANDOMISATION_ATTRIBUT,
						   cree_description<OperatriceRandomisationAttribut>(NOM_RANDOMISATION_ATTRIBUT,
																			AIDE_RANDOMISATION_ATTRIBUT,
																			categorie));

	/* Opérateurs autres. */

	categorie = "Autre";

	usine->enregistre_type(NOM_SORTIE,
						   cree_description<OperatriceSortie>(NOM_SORTIE,
															 AIDE_SORTIE,
															 categorie));

	usine->enregistre_type(NOM_TAMPON,
						   cree_description<OperatriceTampon>(NOM_TAMPON,
															 AIDE_TAMPON,
															 categorie));

	usine->enregistre_type(NOM_COMMUTATEUR,
						   cree_description<OperatriceCommutateur>(NOM_COMMUTATEUR,
																  AIDE_COMMUTATEUR,
																  categorie));
}
