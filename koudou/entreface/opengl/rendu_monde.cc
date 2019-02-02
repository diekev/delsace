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

#include "rendu_monde.h"

#include <ego/outils.h>
#include <GL/glew.h>

#include "bibliotheques/objets/adaptrice_creation.h"
#include "bibliotheques/objets/creation.h"
#include "bibliotheques/opengl/tampon_rendu.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/texture/texture.h"

#include "coeur/koudou.h"

/* ************************************************************************** */

class AdaptriceCreation final : public objets::AdaptriceCreationObjet {
public:
	std::vector<dls::math::vec3f> sommets{};
	std::vector<unsigned int> index{};

	void ajoute_sommet(const float x, const float y, const float z, const float w = 1.0f) override;

	void ajoute_normal(const float x, const float y, const float z) override;

	void ajoute_coord_uv_sommet(const float u, const float v, const float w = 0.0f) override;

	void ajoute_parametres_sommet(const float x, const float y, const float z) override;

	void ajoute_polygone(const int *index_sommet, const int *index_uv, const int *index_normal, long nombre) override;

	void ajoute_ligne(const int *idx, size_t nombre) override;

	void ajoute_objet(const std::string &nom) override;

	void reserve_polygones(long const nombre) override;

	void reserve_sommets(long const nombre) override;

	void reserve_normaux(long const nombre) override;

	void reserve_uvs(long const nombre) override;

	void groupes(const std::vector<std::string> &noms) override;

	void groupe_nuancage(const int idx) override;
};

void AdaptriceCreation::ajoute_sommet(const float x, const float y, const float z, const float w)
{
	INUTILISE(w);
	sommets.push_back(dls::math::vec3f(x, y, z));
}

void AdaptriceCreation::ajoute_normal(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceCreation::ajoute_coord_uv_sommet(const float u, const float v, const float w)
{
	INUTILISE(u);
	INUTILISE(v);
	INUTILISE(w);
}

void AdaptriceCreation::ajoute_parametres_sommet(const float x, const float y, const float z)
{
	INUTILISE(x);
	INUTILISE(y);
	INUTILISE(z);
}

void AdaptriceCreation::ajoute_polygone(const int *index_sommet, const int *index_uv, const int *index_normal, long nombre)
{
	INUTILISE(index_uv);
	INUTILISE(index_normal);

	index.push_back(static_cast<unsigned>(index_sommet[0]));
	index.push_back(unsigned(index_sommet[1]));
	index.push_back(unsigned(index_sommet[2]));

	if (nombre == 4) {
		index.push_back(unsigned(index_sommet[0]));
		index.push_back(unsigned(index_sommet[2]));
		index.push_back(unsigned(index_sommet[3]));
	}
}

void AdaptriceCreation::ajoute_ligne(const int *idx, size_t nombre)
{
	INUTILISE(idx);
	INUTILISE(nombre);
}

void AdaptriceCreation::ajoute_objet(const std::string &nom)
{
	INUTILISE(nom);
}

void AdaptriceCreation::reserve_polygones(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceCreation::reserve_sommets(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceCreation::reserve_normaux(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceCreation::reserve_uvs(long const nombre)
{
	INUTILISE(nombre);
}

void AdaptriceCreation::groupes(const std::vector<std::string> &noms)
{
	INUTILISE(noms);
}

void AdaptriceCreation::groupe_nuancage(const int idx)
{
	INUTILISE(idx);
}

/* ************************************************************************** */

static TamponRendu *cree_tampon(TypeTexture type_texture)
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				numero7::ego::util::str_from_file("nuanceurs/simple.vert"));

	if (type_texture == TypeTexture::COULEUR) {
		tampon->charge_source_programme(
					numero7::ego::Nuanceur::FRAGMENT,
					numero7::ego::util::str_from_file("nuanceurs/couleur.frag"));
	}
	else {
		tampon->charge_source_programme(
					numero7::ego::Nuanceur::FRAGMENT,
					numero7::ego::util::str_from_file("nuanceurs/texture.frag"));
	}

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");

	if (type_texture == TypeTexture::COULEUR) {
		parametre_programme.ajoute_uniforme("couleur");
	}
	else {
		parametre_programme.ajoute_uniforme("image");
	}

	tampon->parametres_programme(parametre_programme);

	if (type_texture == TypeTexture::COULEUR) {
		auto programme = tampon->programme();
		programme->active();
		programme->uniforme("couleur", 1.0f, 0.0f, 1.0f, 1.0f);
		programme->desactive();
	}
	else {
		tampon->ajoute_texture();

		auto texture = tampon->texture();

		auto programme = tampon->programme();
		programme->active();
		programme->uniforme("image", texture->number());
		programme->desactive();
	}

	return tampon;
}

static void genere_texture(numero7::ego::Texture2D *texture, const void *data, GLint size[2])
{
	texture->free(true);
	texture->bind();
	texture->setType(GL_FLOAT, GL_RGB, GL_RGB);
	texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	texture->setWrapping(GL_CLAMP);
	texture->fill(data, size);
	texture->unbind();
}

/* ************************************************************************** */

RenduMonde::RenduMonde(Koudou *koudou)
	: m_monde(&koudou->parametres_rendu.scene.monde)
	, m_ancien_type(0)
{
	m_tampon = cree_tampon(static_cast<TypeTexture>(m_ancien_type));

	/* Par défaut les sphères UV ont une résolution de 48 sur l'axe U et 24 sur
	 * l'axe V. Donc on reserve 48 * 24 * le nombre de données intéressées. */
	AdaptriceCreation adaptrice;
	adaptrice.sommets.reserve(48 * 24 * 4);
	adaptrice.index.reserve(48 * 24 * 12);

	objets::cree_sphere_uv(&adaptrice, 100.0f);

	m_sommets = adaptrice.sommets;
	m_index = adaptrice.index;

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = m_sommets.data();
	parametres_tampon.taille_octet_sommets = m_sommets.size() * sizeof(dls::math::vec3f);
	parametres_tampon.pointeur_index = m_index.data();
	parametres_tampon.taille_octet_index = m_index.size() * sizeof(unsigned int);
	parametres_tampon.elements = m_index.size();

	m_tampon->remplie_tampon(parametres_tampon);

	ajourne();
}

RenduMonde::~RenduMonde()
{
	delete m_tampon;
}

void RenduMonde::ajourne()
{
	auto texture = m_monde->texture;

	if (texture != nullptr) {
		if (texture->type() != static_cast<TypeTexture>(m_ancien_type)) {
			delete m_tampon;
			m_tampon = cree_tampon(texture->type());

			ParametresTampon parametres_tampon;
			parametres_tampon.attribut = "sommets";
			parametres_tampon.dimension_attribut = 3;
			parametres_tampon.pointeur_sommets = m_sommets.data();
			parametres_tampon.taille_octet_sommets = m_sommets.size() * sizeof(dls::math::vec3f);
			parametres_tampon.pointeur_index = m_index.data();
			parametres_tampon.taille_octet_index = m_index.size() * sizeof(unsigned int);
			parametres_tampon.elements = m_index.size();

			m_tampon->remplie_tampon(parametres_tampon);

			m_ancien_type = static_cast<int>(texture->type());
			m_ancien_chemin = "";
		}

		if (texture->type() == TypeTexture::IMAGE) {
			auto texture_image = dynamic_cast<TextureImage *>(texture);

			if (texture_image->chemin() != m_ancien_chemin) {
				auto donnees = texture_image->donnees();
				int taille[2] = { texture_image->largeur(), texture_image->hauteur() };

				genere_texture(m_tampon->texture(), donnees, taille);

				m_ancien_chemin = texture_image->chemin();
			}
		}
		else {
			auto spectre = texture->echantillone(dls::math::vec3d(0.0));

			auto programme = m_tampon->programme();
			programme->active();
			programme->uniforme("couleur", spectre[0], spectre[1], spectre[2], 1.0f);
			programme->desactive();
		}
	}
}

void RenduMonde::dessine(const ContexteRendu &contexte)
{
	m_tampon->dessine(contexte);
}
