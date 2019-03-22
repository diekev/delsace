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

#include "rendu_corps.h"

#include <ego/outils.h>
#include <numeric>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "coeur/corps/corps.h"
#include "coeur/corps/volume.hh"

#include "coeur/attribut.h"

/* ************************************************************************** */

#if 0
enum {
	VARIABLE_TYPE_VEC2,
	VARIABLE_TYPE_VEC3,
	VARIABLE_TYPE_MAT4,
	VARIABLE_TYPE_TEXTURE_2D,
	VARIABLE_TYPE_TEXTURE_3D,
};

struct VariableAttribut {
	int location;
	int type;
	std::string nom;
};

struct VariableGenerique {
	int type;
	std::string nom;
};

struct DonneesScripts {
	std::vector<VariableAttribut> variables_attribut;
	std::vector<VariableGenerique> variables_uniforme;
	std::vector<VariableGenerique> variables_entree;
	std::vector<VariableGenerique> variables_sortie;
};

static void ajoute_variable_attribut(std::string &tampon, std::string const &nom, const int type)
{
	switch (type) {
		case VARIABLE_TYPE_VEC2:
			tampon += "layout (location = 0) in vec2 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_VEC3:
			tampon += "layout (location = 0) in vec3 " + nom + ";\n";
			break;
		default:
			return;
	}
}

static void ajoute_variable_uniforme(std::string &tampon, std::string const &nom, const int type)
{
	switch (type) {
		case VARIABLE_TYPE_VEC2:
			tampon += "uniform vec2 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_VEC3:
			tampon += "uniform vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_MAT4:
			tampon += "uniform vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_2D:
			tampon += "uniform sampler2D " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_3D:
			tampon += "uniform sampler3D " + nom + ";\n";
			break;
		default:
			return;
	}
}

static void ajoute_variable_sortie(std::string &tampon, std::string const &nom, const int type)
{
	switch (type) {
		case VARIABLE_TYPE_VEC2:
			tampon += "smooth out vec2 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_VEC3:
			tampon += "smooth out vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_MAT4:
			tampon += "smooth out vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_2D:
			tampon += "smooth out sampler2D " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_3D:
			tampon += "smooth out sampler3D " + nom + ";\n";
			break;
		default:
			return;
	}
}

static void ajoute_variable_entree(std::string &tampon, std::string const &nom, const int type)
{
	switch (type) {
		case VARIABLE_TYPE_VEC2:
			tampon += "smooth in vec2 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_VEC3:
			tampon += "smooth in vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_MAT4:
			tampon += "smooth in vec3 " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_2D:
			tampon += "smooth in sampler2D " + nom + ";\n";
			break;
		case VARIABLE_TYPE_TEXTURE_3D:
			tampon += "smooth in sampler3D " + nom + ";\n";
			break;
		default:
			return;
	}
}

void compile_sources_nuanceur(Maillage *maillage)
{
	std::string source_vertex;
	std::string source_fragment;

	// ajoute_variable_attribut(nom, type);
	source_vertex += "layout (location = 0) in vec3 sommets;";
	source_vertex += "layout (location = 1) in vec3 normaux;";

	// ajoute_variable_sortie(nom, type);
	source_vertex += "smooth out vec3 sommet;";
	source_vertex += "smooth out vec3 normal;";

	// ajoute_variable_entree(nom, type);
	source_fragment += "smooth in vec3 sommet;";
	source_fragment += "smooth in vec3 normal;";

	// ajoute_variable_uniforme(nom, type);
	source_vertex += "uniform mat4 MVP;";

	// intérieur fonction principale
	source_vertex += "sommet = sommets;";
	source_vertex += "normal = normaux;";
	source_vertex += "gl_Position = normaux;";

	if (maillage->texture()) {
		source_fragment += "uniform sampler2D image;";

		auto texture = maillage->texture();

		switch (texture->projection()) {
			case PROJECTION_CAMERA:
				source_fragment += "uniform mat4 camera_mat;";
				source_fragment += "uniform mat4 camera_proj;";
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_camera(camera_mat, camera_proj, sommet, image);";
				break;
			case PROJECTION_CUBIQUE:
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_cubique(sommet, image);";
				break;
			case PROJECTION_CYLINDRIQUE:
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_cylindrique(camera_mat, camera_proj, sommet, image);";
				break;
			case PROJECTION_SPHERIQUE:
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_sherique(camera_mat, camera_proj, sommet, image);";
				break;
			case PROJECTION_UV:
				source_vertex += "layout(location = 2) in vec2 uvs;";
				source_vertex += "smooth out vec2 UV;";

				// intérieur fonction principale
				source_vertex += "UV = uvs;";

				source_fragment += "smooth in vec2 UV;";

				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_uv(sommet, image, UV);";
				break;
			case PROJECTION_TRIPLANAIRE:
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_triplanaire(sommet, image);";
				break;
			case PROJECTION_PLANAIRE:
				// intérieur fonction principale
				source_fragment += "couleur_fragment = projection_planaire(sommet, image, 0);";
				break;
		}
	}
}
#endif

/* ************************************************************************** */

static TamponRendu *cree_tampon_surface(bool possede_uvs)
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/diffus.vert"));

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/diffus.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normaux");
	parametre_programme.ajoute_attribut("uvs");
	parametre_programme.ajoute_attribut("couleurs");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("MV");
	parametre_programme.ajoute_uniforme("P");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("image");
	parametre_programme.ajoute_uniforme("direction_camera");
	parametre_programme.ajoute_uniforme("methode");
	parametre_programme.ajoute_uniforme("taille_texture");
	parametre_programme.ajoute_uniforme("possede_uvs");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("possede_uvs", possede_uvs);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

void ajoute_polygone_surface(
		Polygone *polygone,
		ListePoints3D *liste_points,
		Attribut *attr_normaux,
		Attribut *attr_couleurs,
		std::vector<dls::math::vec3f> &points,
		std::vector<dls::math::vec3f> &normaux,
		std::vector<dls::math::vec3f> &couleurs)
{
	for (long i = 2; i < polygone->nombre_sommets(); ++i) {
		points.push_back(liste_points->point(polygone->index_point(0)));
		points.push_back(liste_points->point(polygone->index_point(i - 1)));
		points.push_back(liste_points->point(polygone->index_point(i)));

		if (attr_normaux) {
			if (attr_normaux->portee == portee_attr::POINT) {
				normaux.push_back(attr_normaux->vec3(polygone->index_point(0)));
				normaux.push_back(attr_normaux->vec3(polygone->index_point(i - 1)));
				normaux.push_back(attr_normaux->vec3(polygone->index_point(i)));
			}
			else if (attr_normaux->portee == portee_attr::PRIMITIVE) {
				normaux.push_back(attr_normaux->vec3(static_cast<long>(polygone->index)));
				normaux.push_back(attr_normaux->vec3(static_cast<long>(polygone->index)));
				normaux.push_back(attr_normaux->vec3(static_cast<long>(polygone->index)));
			}
			else if (attr_normaux->portee == portee_attr::CORPS) {
				normaux.push_back(attr_normaux->vec3(0));
				normaux.push_back(attr_normaux->vec3(0));
				normaux.push_back(attr_normaux->vec3(0));
			}
		}

		if (attr_couleurs) {
			if (attr_couleurs->portee == portee_attr::POINT) {
				couleurs.push_back(attr_couleurs->vec3(polygone->index_point(0)));
				couleurs.push_back(attr_couleurs->vec3(polygone->index_point(i - 1)));
				couleurs.push_back(attr_couleurs->vec3(polygone->index_point(i)));
			}
			else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
				couleurs.push_back(attr_couleurs->vec3(static_cast<long>(polygone->index)));
				couleurs.push_back(attr_couleurs->vec3(static_cast<long>(polygone->index)));
				couleurs.push_back(attr_couleurs->vec3(static_cast<long>(polygone->index)));
			}
		}
		else {
			couleurs.push_back(dls::math::vec3f(0.8f));
			couleurs.push_back(dls::math::vec3f(0.8f));
			couleurs.push_back(dls::math::vec3f(0.8f));
		}
	}
}

void ajoute_polygone_segment(
		Polygone *polygone,
		ListePoints3D *liste_points,
		Attribut *attr_couleurs,
		std::vector<dls::math::vec3f> &points,
		std::vector<dls::math::vec3f> &couleurs)
{
	for (long i = 0; i < polygone->nombre_segments(); ++i) {
		points.push_back(liste_points->point(polygone->index_point(i)));
		points.push_back(liste_points->point(polygone->index_point(i + 1)));

		if (attr_couleurs) {
			if (attr_couleurs->portee == portee_attr::POINT) {
				couleurs.push_back(attr_couleurs->vec3(polygone->index_point(0)));
				couleurs.push_back(attr_couleurs->vec3(polygone->index_point(i + 1)));
			}
			else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
				couleurs.push_back(attr_couleurs->vec3(static_cast<long>(polygone->index)));
				couleurs.push_back(attr_couleurs->vec3(static_cast<long>(polygone->index)));
			}
		}
		else {
			couleurs.push_back(dls::math::vec3f(0.8f));
			couleurs.push_back(dls::math::vec3f(0.8f));
		}
	}
}

static TamponRendu *cree_tampon_segments()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("couleur_sommet");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");
	parametre_programme.ajoute_uniforme("possede_couleur_sommet");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

static auto slice(
		dls::math::vec3f const &view_dir,
		size_t m_axis,
		TamponRendu *m_renderbuffer,
		dls::math::vec3f const &m_min,
		dls::math::vec3f const &m_max)
{
	auto axis = axe_dominant_abs(view_dir);

	if (m_axis == axis) {
		return;
	}

	auto m_dimensions = m_max - m_min;
	auto m_num_slices = 256ul;
	auto m_elements = m_num_slices * 6;

	m_axis = axis;
	auto depth = m_min[m_axis];
	auto slice_size = m_dimensions[m_axis] / static_cast<float>(m_num_slices);

	/* always process slices in back to front order! */
	if (view_dir[m_axis] > 0.0f) {
		depth = m_max[m_axis];
		slice_size = -slice_size;
	}

	const dls::math::vec3f vertices[3][4] = {
		{
			dls::math::vec3f(0.0f, m_min[1], m_min[2]),
			dls::math::vec3f(0.0f, m_max[1], m_min[2]),
			dls::math::vec3f(0.0f, m_max[1], m_max[2]),
			dls::math::vec3f(0.0f, m_min[1], m_max[2])
		},
		{
			dls::math::vec3f(m_min[0], 0.0f, m_min[2]),
			dls::math::vec3f(m_min[0], 0.0f, m_max[2]),
			dls::math::vec3f(m_max[0], 0.0f, m_max[2]),
			dls::math::vec3f(m_max[0], 0.0f, m_min[2])
		},
		{
			dls::math::vec3f(m_min[0], m_min[1], 0.0f),
			dls::math::vec3f(m_min[0], m_max[1], 0.0f),
			dls::math::vec3f(m_max[0], m_max[1], 0.0f),
			dls::math::vec3f(m_max[0], m_min[1], 0.0f)
		}
	};


	std::vector<GLuint> indices(m_elements);
	unsigned idx = 0;
	size_t idx_count = 0;

	std::vector<dls::math::vec3f> points;
	points.reserve(m_num_slices * 4);

	for (auto slice(0ul); slice < m_num_slices; slice++) {
		dls::math::vec3f v0 = vertices[m_axis][0];
		dls::math::vec3f v1 = vertices[m_axis][1];
		dls::math::vec3f v2 = vertices[m_axis][2];
		dls::math::vec3f v3 = vertices[m_axis][3];

		v0[m_axis] = depth;
		v1[m_axis] = depth;
		v2[m_axis] = depth;
		v3[m_axis] = depth;

		points.push_back(v0); //  * glm::mat3(m_inv_matrix)
		points.push_back(v1);
		points.push_back(v2);
		points.push_back(v3);

		indices[idx_count++] = idx + 0;
		indices[idx_count++] = idx + 1;
		indices[idx_count++] = idx + 2;
		indices[idx_count++] = idx + 0;
		indices[idx_count++] = idx + 2;
		indices[idx_count++] = idx + 3;

		depth += slice_size;
		idx += 4;
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = points.data();
	parametres_tampon.taille_octet_sommets = points.size() * sizeof(dls::math::vec3f);
	parametres_tampon.elements = idx_count;
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = idx_count * sizeof(GLuint);

	m_renderbuffer->remplie_tampon(parametres_tampon);
}

static auto cree_tampon_volume(Volume *volume, dls::math::vec3f const &view_dir)
{
	auto grille = volume->grille;

	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/volume.vert"));

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				numero7::ego::util::str_from_file("nuanceurs/volume.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("vertex");
	parametre_programme.ajoute_uniforme("sommets");
	parametre_programme.ajoute_uniforme("offset");
	parametre_programme.ajoute_uniforme("dimension");
	parametre_programme.ajoute_uniforme("volume");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	tampon->ajoute_texture_3d();
	auto texture = tampon->texture_3d();

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("volume", texture->number());
	programme->uniforme("offset", grille->min.x, grille->min.y, grille->min.z);
	programme->uniforme("dimension", grille->dim.x, grille->dim.y, grille->dim.z);
	programme->desactive();

	/* crée vertices */
	slice(view_dir, -1ul, tampon, grille->min, grille->max);

	/* crée texture 3d */

	texture->bind();
	texture->setType(GL_FLOAT, GL_RED, GL_RED);
	texture->setMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	texture->setWrapping(GL_CLAMP_TO_BORDER);

	auto res_grille = grille->resolution();
	int res[3] = {
		static_cast<int>(res_grille[0]),
		static_cast<int>(res_grille[1]),
		static_cast<int>(res_grille[2])
	};

	if (volume->grille->type() == type_volume::SCALAIRE) {
		auto grille_scalaire = dynamic_cast<Grille<float> *>(volume->grille);
		texture->fill(grille_scalaire->donnees(), res);
	}

	texture->generateMipMap(0, 4);
	texture->unbind();

	return tampon;
}

/* ************************************************************************** */

RenduCorps::RenduCorps(Corps *corps)
	: m_corps(corps)
{}

RenduCorps::~RenduCorps()
{
	delete m_tampon_points;
	delete m_tampon_polygones;
	delete m_tampon_segments;
	delete m_tampon_volume;
}

void RenduCorps::initialise(ContexteRendu const &contexte)
{
	auto liste_points = m_corps->points();
	auto liste_prims = m_corps->prims();

	if (liste_points->taille() == 0l && liste_prims->taille() == 0l) {
		return;
	}

	std::vector<bool> point_utilise(static_cast<size_t>(liste_points->taille()), false);

	if (liste_prims->taille() != 0l) {
		auto attr_N = m_corps->attribut("N");
		auto attr_C = m_corps->attribut("C");
		std::vector<dls::math::vec3f> points_polys;
		std::vector<dls::math::vec3f> points_segment;
		std::vector<dls::math::vec3f> normaux;
		std::vector<dls::math::vec3f> couleurs_polys;
		std::vector<dls::math::vec3f> couleurs_segment;

		for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
			auto prim = liste_prims->prim(ip);
			if (prim->type_prim() == type_primitive::POLYGONE) {
				auto polygone = dynamic_cast<Polygone *>(prim);
				if (polygone->type == type_polygone::FERME) {
					ajoute_polygone_surface(polygone, liste_points, attr_N, attr_C, points_polys, normaux, couleurs_polys);
				}
				else if (polygone->type == type_polygone::OUVERT) {
					ajoute_polygone_segment(polygone, liste_points, attr_C, points_segment, couleurs_segment);
				}

				for (auto i = 0; i < polygone->nombre_sommets(); ++i) {
					point_utilise[static_cast<size_t>(polygone->index_point(i))] = true;
				}
			}
			else if (prim->type_prim() == type_primitive::VOLUME) {
				if (m_tampon_volume == nullptr) {
					m_tampon_volume = cree_tampon_volume(dynamic_cast<Volume *>(prim), contexte.vue());
				}
			}
		}

		if (points_polys.size() != 0) {
			m_tampon_polygones = cree_tampon_surface(false);

			ParametresTampon parametres_tampon;
			parametres_tampon.attribut = "sommets";
			parametres_tampon.dimension_attribut = 3;
			parametres_tampon.pointeur_sommets = points_polys.data();
			parametres_tampon.taille_octet_sommets = points_polys.size() * sizeof(dls::math::vec3f);
			parametres_tampon.elements = points_polys.size();

			m_tampon_polygones->remplie_tampon(parametres_tampon);

			if (normaux.size() != 0) {
				parametres_tampon.attribut = "normaux";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = normaux.data();
				parametres_tampon.taille_octet_donnees_extra = normaux.size() * sizeof(dls::math::vec3f);
				parametres_tampon.elements = normaux.size();

				m_tampon_polygones->remplie_tampon_extra(parametres_tampon);
			}

			if (couleurs_polys.size() != 0) {
				parametres_tampon.attribut = "couleurs";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = couleurs_polys.data();
				parametres_tampon.taille_octet_donnees_extra = couleurs_polys.size() * sizeof(dls::math::vec3f);
				parametres_tampon.elements = couleurs_polys.size();

				m_tampon_polygones->remplie_tampon_extra(parametres_tampon);
			}
		}

		if (points_segment.size() != 0) {
			m_tampon_segments = cree_tampon_segments();

			ParametresTampon parametres_tampon;
			parametres_tampon.attribut = "sommets";
			parametres_tampon.dimension_attribut = 3;
			parametres_tampon.pointeur_sommets = points_segment.data();
			parametres_tampon.taille_octet_sommets = points_segment.size() * sizeof(dls::math::vec3f);
			parametres_tampon.elements = points_segment.size();

			m_tampon_segments->remplie_tampon(parametres_tampon);

			if (couleurs_segment.size() != 0) {
				parametres_tampon.attribut = "couleur_sommet";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = couleurs_segment.data();
				parametres_tampon.taille_octet_donnees_extra = couleurs_segment.size() * sizeof(dls::math::vec3f);
				parametres_tampon.elements = couleurs_segment.size();

				m_tampon_segments->remplie_tampon_extra(parametres_tampon);

				auto programme = m_tampon_segments->programme();
				programme->active();
				programme->uniforme("possede_couleur_sommet", 1);
				programme->desactive();
			}
		}
	}

//	if (m_tampon_segments || m_tampon_polygones) {
//		return;
//	}

	std::vector<dls::math::vec3f> points;
	std::vector<dls::math::vec3f> couleurs;
	points.reserve(static_cast<size_t>(liste_points->taille()));
	couleurs.reserve(static_cast<size_t>(liste_points->taille()));

	auto attr_C = m_corps->attribut("C");

	for (auto i = 0; i < liste_points->taille(); ++i) {
		if (point_utilise[static_cast<size_t>(i)]) {
			continue;
		}

		points.push_back(liste_points->point(i));

		if ((attr_C != nullptr) && (attr_C->portee == portee_attr::POINT)) {
			couleurs.push_back(attr_C->vec3(i));
		}
	}

	if (points.empty()) {
		return;
	}

	m_tampon_points = cree_tampon_segments();

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = points.data();
	parametres_tampon.taille_octet_sommets = points.size() * sizeof(dls::math::vec3f);
	parametres_tampon.elements = points.size();

	ParametresDessin parametres_dessin;
	parametres_dessin.taille_point(2.0);
	parametres_dessin.type_dessin(GL_POINTS);
	m_tampon_points->parametres_dessin(parametres_dessin);

	m_tampon_points->remplie_tampon(parametres_tampon);

	if (couleurs.size() != 0ul) {
		parametres_tampon.attribut = "couleur_sommet";
		parametres_tampon.dimension_attribut = 3;
		parametres_tampon.pointeur_donnees_extra = couleurs.data();
		parametres_tampon.taille_octet_donnees_extra = couleurs.size() * sizeof(dls::math::vec3f);
		parametres_tampon.elements = couleurs.size();

		m_tampon_points->remplie_tampon_extra(parametres_tampon);
		auto programme = m_tampon_points->programme();
		programme->active();
		programme->uniforme("possede_couleur_sommet", 1);
		programme->desactive();
	}
}

void RenduCorps::dessine(ContexteRendu const &contexte)
{
	if (m_tampon_points != nullptr) {
		m_tampon_points->dessine(contexte);
	}

	if (m_tampon_polygones != nullptr) {
		auto programme = m_tampon_polygones->programme();
		programme->active();
		programme->uniforme("methode", -1);
		programme->desactive();

		m_tampon_polygones->dessine(contexte);
	}

	if (m_tampon_segments != nullptr) {
		ParametresDessin parametres_dessin;
		parametres_dessin.taille_point(2.0);
		parametres_dessin.type_dessin(GL_POINTS);
		m_tampon_segments->parametres_dessin(parametres_dessin);

		m_tampon_segments->dessine(contexte);

		parametres_dessin.taille_point(1.0);
		parametres_dessin.type_dessin(GL_LINES);
		m_tampon_segments->parametres_dessin(parametres_dessin);

		m_tampon_segments->dessine(contexte);
	}

	if (m_tampon_volume != nullptr) {
		ParametresDessin parametres_dessin;
		parametres_dessin.taille_point(2.0);
		parametres_dessin.type_dessin(GL_TRIANGLES);
		m_tampon_volume->parametres_dessin(parametres_dessin);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		m_tampon_volume->dessine(contexte);
		glDisable(GL_BLEND);
	}
}
