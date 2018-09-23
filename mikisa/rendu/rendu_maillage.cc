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

#include "rendu_maillage.h"

#include <ego/utils.h>
#include <numeric>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "coeur/corps/corps.h"
#include "coeur/corps/maillage.h"

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

static void genere_texture(
		numero7::ego::Texture2D *texture,
		const void *donnes,
		GLint *taille,
		int enveloppage,
		int entrepolation)
{
	texture->free(true);
	texture->bind();
	texture->setType(GL_FLOAT, GL_RGB, GL_RGB);

	if (entrepolation == ENTREPOLATION_LINEAIRE) {
		texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	}
	else if (entrepolation == ENTREPOLATION_VOISINAGE_PROCHE) {
		texture->setMinMagFilter(GL_NEAREST, GL_NEAREST);
	}

	if (enveloppage == ENVELOPPAGE_REPETITION) {
		texture->setWrapping(GL_REPEAT);
	}
	else if (enveloppage == ENVELOPPAGE_REPETITION_MIRROIR) {
		texture->setWrapping(GL_MIRRORED_REPEAT);
	}
	else if (enveloppage == ENVELOPPAGE_RESTRICTION) {
		texture->setWrapping(GL_CLAMP_TO_EDGE);
	}

	texture->fill(donnes, taille);
	texture->unbind();
}

TamponRendu *cree_tampon_surface(bool possede_uvs)
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::VERTEX_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/diffus.vert"));

	tampon->charge_source_programme(
				numero7::ego::FRAGMENT_SHADER,
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
	programme->enable();
	programme->uniform("possede_uvs", possede_uvs);
	programme->disable();

	return tampon;
}

TamponRendu *genere_tampon_surface(Maillage *maillage)
{
	const auto nombre_polygones = maillage->nombre_polygones();
	const auto nombre_elements = nombre_polygones * 4;

	const auto attr_normaux = maillage->attribut("N");
	const auto attr_uvs = maillage->attribut("UV");

	const auto possede_uvs = attr_uvs != nullptr;
	const auto possede_normaux = attr_normaux != nullptr;

	auto tampon = cree_tampon_surface(possede_uvs);

	std::vector<numero7::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	std::vector<numero7::math::vec3f> normaux;
	normaux.reserve(nombre_elements);

	std::vector<numero7::math::vec2f> uvs;
	uvs.reserve(nombre_elements);

	/* OpenGL ne travaille qu'avec des floats. */
	for (size_t	i = 0; i < nombre_polygones; ++i) {
		const auto poly = maillage->polygone(i);

		sommets.push_back(poly->s[0]->pos);
		sommets.push_back(poly->s[1]->pos);
		sommets.push_back(poly->s[2]->pos);

		if (possede_normaux) {
			if (attr_normaux->portee == ATTR_PORTEE_POINT) {
				normaux.push_back(attr_normaux->vec3(poly->s[0]->index));
				normaux.push_back(attr_normaux->vec3(poly->s[1]->index));
				normaux.push_back(attr_normaux->vec3(poly->s[2]->index));
			}
			else if (attr_normaux->portee == ATTR_PORTEE_POLYGONE) {
				normaux.push_back(attr_normaux->vec3(i));
				normaux.push_back(attr_normaux->vec3(i));
				normaux.push_back(attr_normaux->vec3(i));
			}
		}

		if (possede_uvs) {
			uvs.push_back(attr_uvs->vec2(poly->uvs[0]));
			uvs.push_back(attr_uvs->vec2(poly->uvs[1]));
			uvs.push_back(attr_uvs->vec2(poly->uvs[2]));
		}

		if (poly->s[3]) {
			sommets.push_back(poly->s[0]->pos);
			sommets.push_back(poly->s[2]->pos);
			sommets.push_back(poly->s[3]->pos);

			if (possede_normaux) {
				if (attr_normaux->portee == ATTR_PORTEE_POINT) {
					normaux.push_back(attr_normaux->vec3(poly->s[0]->index));
					normaux.push_back(attr_normaux->vec3(poly->s[2]->index));
					normaux.push_back(attr_normaux->vec3(poly->s[3]->index));
				}
				else if (attr_normaux->portee == ATTR_PORTEE_POLYGONE) {
					normaux.push_back(attr_normaux->vec3(i));
					normaux.push_back(attr_normaux->vec3(i));
					normaux.push_back(attr_normaux->vec3(i));
				}
			}

			if (possede_uvs) {
				uvs.push_back(attr_uvs->vec2(poly->uvs[0]));
				uvs.push_back(attr_uvs->vec2(poly->uvs[2]));
				uvs.push_back(attr_uvs->vec2(poly->uvs[3]));
			}
		}
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(numero7::math::vec3f);
	parametres_tampon.elements = sommets.size();

	tampon->remplie_tampon(parametres_tampon);

	parametres_tampon.attribut = "normaux";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_donnees_extra = normaux.data();
	parametres_tampon.taille_octet_donnees_extra = normaux.size() * sizeof(numero7::math::vec3f);
	parametres_tampon.elements = normaux.size();

	tampon->remplie_tampon_extra(parametres_tampon);

	if (possede_uvs) {
		parametres_tampon.attribut = "uvs";
		parametres_tampon.dimension_attribut = 2;
		parametres_tampon.pointeur_donnees_extra = uvs.data();
		parametres_tampon.taille_octet_donnees_extra = uvs.size() * sizeof(numero7::math::vec2f);
		parametres_tampon.elements = uvs.size();

		tampon->remplie_tampon_extra(parametres_tampon);
	}

	numero7::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	return tampon;
}

/* ************************************************************************** */

RenduMaillage::RenduMaillage(Maillage *maillage)
	: m_maillage(maillage)
{}

RenduMaillage::~RenduMaillage()
{
	delete m_tampon;
}

void RenduMaillage::initialise()
{
	m_tampon = genere_tampon_surface(m_maillage);

	if (m_maillage->texture()) {
		auto texture_image = m_maillage->texture();
		m_tampon->ajoute_texture();
		auto texture = m_tampon->texture();

		GLint taille_texture[2] = {
			texture_image->largeur(),
			texture_image->hauteur()
		};

		genere_texture(texture,
					   texture_image->donnees(),
					   taille_texture,
					   texture_image->enveloppage(),
					   texture_image->entrepolation());
	}
}

void RenduMaillage::dessine(const ContexteRendu &contexte)
{
	auto programme = m_tampon->programme();
	programme->enable();

	if (m_tampon->texture()) {
		programme->uniform("image", m_tampon->texture()->number());
	}

	auto texture_image = m_maillage->texture();

	if (texture_image) {
		programme->uniform("methode", texture_image->projection());
		programme->uniform("taille_texture", texture_image->taille().x, texture_image->taille().y);

		if (texture_image->camera()) {
			auto camera = texture_image->camera();
			glUniformMatrix4fv((*programme)("MV"), 1, GL_FALSE, glm::value_ptr(camera->MV()));
			glUniformMatrix4fv((*programme)("P"), 1, GL_FALSE, glm::value_ptr(camera->P()));
			programme->uniform("direction_camera", camera->dir().x, camera->dir().y, camera->dir().z);
		}
	}
	else {
		programme->uniform("methode", -1);
	}

	programme->disable();

	m_tampon->dessine(contexte);
}

/* ************************************************************************** */

void ajoute_polygone_surface(
		Polygone *polygone,
		ListePoints3D *liste_points,
		Attribut *attr_normaux,
		std::vector<numero7::math::vec3f> &points,
		std::vector<numero7::math::vec3f> &normaux)
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
		std::vector<numero7::math::vec3f> &points)
{
	for (int i = 0; i < polygone->nombre_segments(); ++i) {
		points.push_back(liste_points->point(polygone->index_point(i)));
		points.push_back(liste_points->point(polygone->index_point(i + 1)));
	}
}

static TamponRendu *cree_tampon_segments()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::VERTEX_SHADER,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				numero7::ego::FRAGMENT_SHADER,
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
	programme->enable();
	programme->uniform("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->disable();

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
		std::vector<numero7::math::vec3f> points_polys;
		std::vector<numero7::math::vec3f> points_segment;
		std::vector<numero7::math::vec3f> normaux;

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
			parametres_tampon.taille_octet_sommets = points_polys.size() * sizeof(numero7::math::vec3f);
			parametres_tampon.elements = points_polys.size();

			m_tampon_polygones->remplie_tampon(parametres_tampon);

			if (normaux.size() != 0) {
				parametres_tampon.attribut = "normaux";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = normaux.data();
				parametres_tampon.taille_octet_donnees_extra = normaux.size() * sizeof(numero7::math::vec3f);
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
			parametres_tampon.taille_octet_sommets = points_segment.size() * sizeof(numero7::math::vec3f);
			parametres_tampon.elements = points_segment.size();

			m_tampon_segments->remplie_tampon(parametres_tampon);
		}
	}

	if (m_tampon_segments || m_tampon_polygones) {
		return;
	}

	std::vector<numero7::math::vec3f> points;
	points.reserve(liste_points->taille());

	for (Point3D *point : liste_points->points()) {
		points.push_back(numero7::math::vec3f(point->x, point->y, point->z));
	}

	m_tampon_points = cree_tampon_segments();

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = points.data();
	parametres_tampon.taille_octet_sommets = points.size() * sizeof(numero7::math::vec3f);
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
		programme->enable();
		programme->uniform("methode", -1);
		programme->disable();

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
