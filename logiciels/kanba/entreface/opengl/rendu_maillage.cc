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

#include "biblinternes/ego/outils.h"
#include <numeric>

#include "biblinternes/opengl/atlas_texture.h"
#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/texture/texture.h"
#include "biblinternes/structures/dico.hh"

#include "coeur/maillage.h"

/* ************************************************************************** */

#undef BOMBAGE_TEXTURE

#ifdef BOMBAGE_TEXTURE
static void genere_texture(dls::ego::Texture2D *texture, const void *donnes, GLint taille[2])
{
	texture->free(true);
	dls::ego::util::GPU_check_errors("Erreur lors de la suppression de la texture");
	texture->attache();
	dls::ego::util::GPU_check_errors("Erreur lors de l'attache de la texture");
	texture->setType(GL_FLOAT, GL_RGB, GL_RGB);
	dls::ego::util::GPU_check_errors("Erreur lors du typage de la texture");
	texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	dls::ego::util::GPU_check_errors("Erreur lors du filtrage de la texture");
	texture->setWrapping(GL_REPEAT);
	dls::ego::util::GPU_check_errors("Erreur lors du wrapping de la texture");
	texture->fill(donnes, taille);
	dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
	texture->detache();
	dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}
#else
static void genere_texture(AtlasTexture *atlas, const void *donnes, GLint taille[3])
{
	atlas->detruit(true);
	dls::ego::util::GPU_check_errors("Erreur lors de la suppression de la texture");
	atlas->attache();
	dls::ego::util::GPU_check_errors("Erreur lors de l'attache de la texture");
	atlas->typage(GL_FLOAT, GL_RGBA, GL_RGBA);
	dls::ego::util::GPU_check_errors("Erreur lors du typage de la texture");
	atlas->filtre_min_mag(GL_NEAREST, GL_NEAREST);
	dls::ego::util::GPU_check_errors("Erreur lors du filtrage de la texture");
	atlas->enveloppage(GL_CLAMP);
	dls::ego::util::GPU_check_errors("Erreur lors du wrapping de la texture");
	atlas->rempli(donnes, taille);
	dls::ego::util::GPU_check_errors("Erreur lors du remplissage de la texture");
	atlas->detache();
	dls::ego::util::GPU_check_errors("Erreur lors de la détache de la texture");
}
#endif

/* ************************************************************************** */

TamponRendu *cree_tampon_arrete()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->desactive();

	return tampon;
}

TamponRendu *genere_tampon_arrete(Maillage *maillage)
{
	auto const nombre_arretes = maillage->nombre_arretes();
	auto const nombre_elements = nombre_arretes * 2;
	auto tampon = cree_tampon_arrete();

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	/* OpenGL ne travaille qu'avec des floats. */
	for (size_t	i = 0; i < nombre_arretes; ++i) {
		auto const arrete = maillage->arrete(i);

		sommets.push_back(arrete->s[0]->pos);
		sommets.push_back(arrete->s[1]->pos);
	}

	std::vector<unsigned int> indices(sommets.size());
	std::iota(indices.begin(), indices.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	tampon->remplie_tampon(parametres_tampon);

	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

TamponRendu *cree_tampon_normal()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/simple.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.5f, 1.0f, 0.5f, 1.0f);
	programme->desactive();

	return tampon;
}

TamponRendu *genere_tampon_normal(Maillage *maillage)
{
	auto const nombre_polygones = maillage->nombre_polygones();
	auto const nombre_elements = nombre_polygones * 2;
	auto tampon = cree_tampon_normal();

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	/* OpenGL ne travaille qu'avec des floats. */
	for (size_t	i = 0; i < nombre_polygones; ++i) {
		auto const polygone = maillage->polygone(i);
		auto V = polygone->s[0]->pos;
		V += polygone->s[1]->pos;
		V += polygone->s[2]->pos;
		auto poids = 3.0f;

		if (polygone->s[3] != nullptr) {
			V += polygone->s[3]->pos;
			poids = 4.0f;
		}

		V /= poids;

		auto const N = normalise(polygone->nor);

		sommets.push_back(V);
		sommets.push_back(V + 0.1f * N);
	}

	std::vector<unsigned int> indices(sommets.size());
	std::iota(indices.begin(), indices.end(), 0);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = indices.data();
	parametres_tampon.taille_octet_index = indices.size() * sizeof(unsigned int);
	parametres_tampon.elements = indices.size();

	tampon->remplie_tampon(parametres_tampon);

	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

TamponRendu *creer_tampon()
{
	auto tampon = new TamponRendu;

#ifdef BOMBAGE_TEXTURE
	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/texture_bombee.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/texture_bombee.frag"));
#else
	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/diffus.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/diffus.frag"));
#endif

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normal");
	parametre_programme.ajoute_attribut("uvs");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("texture_poly");
	parametre_programme.ajoute_uniforme("taille_u");
	parametre_programme.ajoute_uniforme("taille_v");

	tampon->parametres_programme(parametre_programme);

#ifdef BOMBAGE_TEXTURE
	tampon->ajoute_texture();
	auto texture = tampon->texture();
#else
	tampon->ajoute_atlas();
	auto texture = tampon->atlas();
#endif

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 1.0f, 1.0f, 1.0f, 1.0f);
	programme->uniforme("taille_u", 1.0f);
	programme->uniforme("taille_v", 1.0f);
#ifdef BOMBAGE_TEXTURE
	programme->uniforme("texture_poly", texture->number());
#else
	programme->uniforme("texture_poly", texture->nombre());
#endif
	programme->desactive();

	return tampon;
}

TamponRendu *genere_tampon(Maillage *maillage, std::vector<uint> const &id_polys)
{
	auto nombre_elements = id_polys.size() * 6;
	auto tampon = creer_tampon();

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(nombre_elements);

	std::vector<dls::math::vec3f> uvs;
	uvs.reserve(nombre_elements);

	std::vector<dls::math::vec3f> normaux;
	normaux.reserve(nombre_elements);

	auto index_poly = 0.0f;

	for (size_t	i : id_polys) {
		auto const poly = maillage->polygone(i);

		sommets.push_back(poly->s[0]->pos);
		sommets.push_back(poly->s[1]->pos);
		sommets.push_back(poly->s[2]->pos);

		normaux.push_back(poly->nor);
		normaux.push_back(poly->nor);
		normaux.push_back(poly->nor);

		if (poly->s[3] != nullptr) {
			sommets.push_back(poly->s[0]->pos);
			sommets.push_back(poly->s[2]->pos);
			sommets.push_back(poly->s[3]->pos);

			uvs.push_back(dls::math::vec3f(0.0f, 0.0f, index_poly));
			uvs.push_back(dls::math::vec3f(0.0f, 1.0f, index_poly));
			uvs.push_back(dls::math::vec3f(1.0f, 1.0f, index_poly));

			uvs.push_back(dls::math::vec3f(0.0f, 0.0f, index_poly));
			uvs.push_back(dls::math::vec3f(1.0f, 1.0f, index_poly));
			uvs.push_back(dls::math::vec3f(1.0f, 0.0f, index_poly));

			normaux.push_back(poly->nor);
			normaux.push_back(poly->nor);
			normaux.push_back(poly->nor);
		}
		else {
			uvs.push_back(dls::math::vec3f(0.0f, 0.0f, index_poly));
			uvs.push_back(dls::math::vec3f(0.0f, 1.0f, index_poly));
			uvs.push_back(dls::math::vec3f(1.0f, 0.0f, index_poly));
		}

		index_poly += 1.0f;
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.elements = sommets.size();
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);

	tampon->remplie_tampon(parametres_tampon);

	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon de sommets");

	parametres_tampon.attribut = "normal";
	parametres_tampon.pointeur_donnees_extra = normaux.data();
	parametres_tampon.taille_octet_donnees_extra = normaux.size() * sizeof(dls::math::vec3f);

	tampon->remplie_tampon_extra(parametres_tampon);
	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon de normal");

	parametres_tampon.attribut = "uvs";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.taille_octet_donnees_extra = uvs.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_donnees_extra = uvs.data();

	tampon->remplie_tampon_extra(parametres_tampon);

	dls::ego::util::GPU_check_errors("Erreur lors de la création du tampon d'uvs");

	return tampon;
}

/* ************************************************************************** */

RenduMaillage::RenduMaillage(Maillage *maillage)
	: m_maillage(maillage)
{}

RenduMaillage::~RenduMaillage()
{
	supprime_tampons();
}

void RenduMaillage::initialise()
{
	supprime_tampons();

	auto nombre_polys = m_maillage->nombre_polygones();

	auto nombre_quads = 0;
	auto nombre_tris = 0;

	dls::dico<std::pair<uint, uint>, std::vector<uint>> vecteurs_polys;

	for (size_t	i = 0; i < nombre_polys; ++i) {
		auto const poly = m_maillage->polygone(i);
		((poly->s[3] != nullptr) ? nombre_quads : nombre_tris) += 1;

		auto const &paire = std::make_pair(poly->res_u, poly->res_v);

		vecteurs_polys[paire].push_back(static_cast<uint>(i));
	}

	std::cout << "Nombre de seaux : " << vecteurs_polys.taille() << '\n';

	std::cout << "Nombre de quads : " << nombre_quads
			  << ", nombre de triangles : " << nombre_tris << '\n';

	Page page;
	page.tampon = nullptr;

	GLint max_textures;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, &max_textures);

	for (auto const &id_polys : vecteurs_polys) {
		for (uint i : id_polys.second) {
			if (static_cast<int>(page.polys.size()) >= max_textures) {
				m_pages.push_back(page);
				page.polys.clear();
			}

			page.polys.push_back(i);
		}

		m_pages.push_back(page);
		page.polys.clear();
	}

	std::cerr << "Il y a " << m_pages.size() << " pages\n";

	for (auto &pages : m_pages) {
		pages.tampon = genere_tampon(m_maillage, pages.polys);
	}

	/* Création texture */
	ajourne_texture();

	m_tampon_arrete = genere_tampon_arrete(m_maillage);
	m_tampon_normal = genere_tampon_normal(m_maillage);
}

void RenduMaillage::ajourne_texture()
{
#if 0
	int decalages[5][2] {
		{ 0, 0 },
		{ 0, NOMBRE_TEXELS - 1 },
		{ NOMBRE_TEXELS - 1, 0 },
		{ NOMBRE_TEXELS - 1, NOMBRE_TEXELS - 1 },
		{ 0, 0 },
	};

	/* Copie les texels se situant sur les polygones adjacents. */
	for (size_t i = 0; i < m_maillage->nombre_polygones(); ++i) {
		auto poly = m_maillage->polygone(i);

		for (int a = 0; a < 4; ++a) {
			auto arrete = poly->a[a];
			if (arrete->opposee == nullptr) {
				std::cerr << "Arrete n'a pas d'opposée !\n";
				continue;
			}

			auto poly_adjacent = arrete->opposee->p;
			auto couleur = poly_adjacent->tampon[NOMBRE_TEXELS / 2][NOMBRE_TEXELS / 2];

			for (int j = decalages[a][0]; j < decalages[a][1] + 1; ++j) {
				for (int k = decalages[a + 1][0]; k < decalages[a + 1][1] + 1; ++k) {
					poly->tampon[j][k] = couleur;
				}
			}
		}
	}
#endif
#ifdef BOMBAGE_TEXTURE
	TextureImage *texture_image = charge_texture("/home/kevin/Téléchargements/Tileable metal scratch rust texture (8).jpg");

	GLint taille_texture[2] = {
		static_cast<GLint>(texture_image->largeur()),
		static_cast<GLint>(texture_image->hauteur())
	};

	auto donnees = texture_image->donnees();

	for (auto const &pages : m_pages) {
		auto texture = pages.tampon->texture();
		genere_texture(texture, donnees, taille_texture);
	}

	delete texture_image;
#else
	auto const largeur = m_maillage->largeur_texture();
	auto const &canaux = m_maillage->canaux_texture();

	auto tampon = canaux.tampon_diffusion;

	if (tampon == nullptr) {
		return;
	}

	for (auto const &pages : m_pages) {
		auto poly = m_maillage->polygone(pages.polys[0]);

		GLint taille_texture[3] = {
			static_cast<GLint>(poly->res_u),
			static_cast<GLint>(poly->res_v),
			static_cast<GLint>(pages.polys.size())
		};

//		std::cerr << "Création d'une texture de "
//				  << taille_texture[0] << "x"
//				  << taille_texture[1] << "x"
//				  << taille_texture[2] << '\n';

		std::vector<dls::math::vec4f> image(static_cast<size_t>(taille_texture[0] * taille_texture[1] * taille_texture[2]));
		auto donnees = image.data();

		/* Copie les texels dans l'atlas OpenGL. */
		auto ip = 0;
		for (size_t i : pages.polys) {
			auto poly_page = m_maillage->polygone(i);
			auto index_poly = (poly_page->x + poly_page->y * (m_maillage->largeur_texture()));
			auto tampon_poly = tampon + index_poly;

			auto donnees_image = &donnees[ip++ * taille_texture[0] * taille_texture[1]];

			for (size_t j = 0; j < poly_page->res_u; ++j) {
				for (size_t k = 0; k < poly_page->res_v; ++k) {
					donnees_image[j + k * static_cast<size_t>(taille_texture[0])] = tampon_poly[j + k * static_cast<size_t>(largeur)];
				}
			}
		}

		auto texture = pages.tampon->atlas();
		genere_texture(texture, image.data(), taille_texture);
	}

	dls::ego::util::GPU_check_errors("Erreur lors de la génération de la texture");
#endif
	m_maillage->marque_texture_surrannee(false);
}

void RenduMaillage::supprime_tampons()
{
	for (auto const &page : m_pages) {
		delete page.tampon;
	}

	delete m_tampon_arrete;
	delete m_tampon_normal;
}

void RenduMaillage::dessine(ContexteRendu const &contexte)
{
	if (m_maillage->texture_surrannee()) {
		ajourne_texture();
	}

	for (auto const &page : m_pages) {
		page.tampon->dessine(contexte);
	}

	if (contexte.dessine_arretes()) {
		m_tampon_arrete->dessine(contexte);
	}

	if (contexte.dessine_normaux()) {
		m_tampon_normal->dessine(contexte);
	}
}

dls::math::mat4x4d RenduMaillage::matrice() const
{
	return m_maillage->transformation().matrice();
}

Maillage *RenduMaillage::maillage() const
{
	return m_maillage;
}
