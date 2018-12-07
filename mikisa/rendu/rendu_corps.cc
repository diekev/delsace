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

static void ajoute_variable_attribut(std::string &tampon, const std::string &nom, const int type)
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

static void ajoute_variable_uniforme(std::string &tampon, const std::string &nom, const int type)
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

static void ajoute_variable_sortie(std::string &tampon, const std::string &nom, const int type)
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

static void ajoute_variable_entree(std::string &tampon, const std::string &nom, const int type)
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
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("MV");
	parametre_programme.ajoute_uniforme("P");
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
		std::vector<dls::math::vec3f> &points,
		std::vector<dls::math::vec3f> &normaux)
{
	points.push_back(liste_points->point(polygone->index_point(0)));
	points.push_back(liste_points->point(polygone->index_point(1)));
	points.push_back(liste_points->point(polygone->index_point(2)));

	if (attr_normaux) {
		if (attr_normaux->portee == ATTR_PORTEE_POINT) {
			normaux.push_back(attr_normaux->vec3(polygone->index_point(0)));
			normaux.push_back(attr_normaux->vec3(polygone->index_point(1)));
			normaux.push_back(attr_normaux->vec3(polygone->index_point(2)));
		}
		else if (attr_normaux->portee == ATTR_PORTEE_POLYGONE) {
			normaux.push_back(attr_normaux->vec3(polygone->index));
			normaux.push_back(attr_normaux->vec3(polygone->index));
			normaux.push_back(attr_normaux->vec3(polygone->index));
		}
	}

	if (polygone->nombre_sommets() == 4) {
		points.push_back(liste_points->point(polygone->index_point(0)));
		points.push_back(liste_points->point(polygone->index_point(2)));
		points.push_back(liste_points->point(polygone->index_point(3)));

		if (attr_normaux) {
			if (attr_normaux->portee == ATTR_PORTEE_POINT) {
				normaux.push_back(attr_normaux->vec3(polygone->index_point(0)));
				normaux.push_back(attr_normaux->vec3(polygone->index_point(2)));
				normaux.push_back(attr_normaux->vec3(polygone->index_point(3)));
			}
			else if (attr_normaux->portee == ATTR_PORTEE_POLYGONE) {
				normaux.push_back(attr_normaux->vec3(polygone->index));
				normaux.push_back(attr_normaux->vec3(polygone->index));
				normaux.push_back(attr_normaux->vec3(polygone->index));
			}
		}
	}
}

void ajoute_polygone_segment(
		Polygone *polygone,
		ListePoints3D *liste_points,
		std::vector<dls::math::vec3f> &points)
{
	for (size_t i = 0; i < polygone->nombre_segments(); ++i) {
		points.push_back(liste_points->point(polygone->index_point(i)));
		points.push_back(liste_points->point(polygone->index_point(i + 1)));
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
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

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

RenduCorps::RenduCorps(Corps *corps)
	: m_corps(corps)
{}

RenduCorps::~RenduCorps()
{
	delete m_tampon_points;
	delete m_tampon_polygones;
	delete m_tampon_segments;
}

void RenduCorps::initialise()
{
	auto liste_points = m_corps->points();
	auto liste_polys = m_corps->polys();

	if (liste_points->taille() == 0ul) {
		return;
	}

	if (liste_polys->taille() != 0ul) {

		auto attr_N = m_corps->attribut("N");
		std::vector<dls::math::vec3f> points_polys;
		std::vector<dls::math::vec3f> points_segment;
		std::vector<dls::math::vec3f> normaux;

		for (Polygone *polygone : liste_polys->polys()) {
			if (polygone->type == POLYGONE_FERME) {
				ajoute_polygone_surface(polygone, liste_points, attr_N, points_polys, normaux);
			}
			else if (polygone->type == POLYGONE_OUVERT) {
				ajoute_polygone_segment(polygone, liste_points, points_segment);
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
		}
	}

	if (m_tampon_segments || m_tampon_polygones) {
		return;
	}

	std::vector<dls::math::vec3f> points;
	points.reserve(liste_points->taille());

	for (Point3D *point : liste_points->points()) {
		points.push_back(dls::math::vec3f(point->x, point->y, point->z));
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
}

void RenduCorps::dessine(const ContexteRendu &contexte)
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
}
