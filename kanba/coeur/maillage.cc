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

#include "maillage.h"

#include <algorithm>
#include <random>

#include <delsace/math/bruit.hh>

#include "bibliotheques/texture/texture.h"

#include "outils_couleur.h"
#include "paqueuse_texture.h"

/* ************************************************************************** */

static unsigned int prochain_multiple_de_2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

void ajoute_calque_procedurale(Maillage *maillage)
{
	dls::math::BruitPerlin3D bruit;

	const auto nombre_polygones = maillage->nombre_polygones();
	const auto largeur = maillage->largeur_texture();
	auto tampon = static_cast<dls::math::vec4f *>(maillage->calque_actif()->tampon);

	for (size_t i = 0; i < nombre_polygones; ++i) {
		auto poly = maillage->polygone(i);

		const auto &s0 = poly->s[0]->pos;
		const auto &s1 = poly->s[1]->pos;
		const auto &s3 = poly->s[3]->pos;

		const auto &cote0 = s1 - s0;
		const auto &cote1 = s3 - s0;

		const auto &dU = cote0 / static_cast<float>(poly->res_u);
		const auto &dV = cote1 / static_cast<float>(poly->res_v);

		const auto index_poly = poly->x + poly->y * largeur;
		auto tampon_poly = tampon + index_poly;

		for (unsigned j = 0; j < poly->res_v; ++j) {
			for (unsigned k = 0; k < poly->res_u; ++k) {
				auto index = j * static_cast<unsigned>(largeur) + k;

				auto coord = s0 + dU * static_cast<float>(j) + dV * static_cast<float>(k);

				auto couleur = bruit(coord * 10.f);

				tampon_poly[index] = dls::math::vec4f(couleur, couleur, couleur, 1.0f);
			}
		}
	}
}

void ajoute_calque_echiquier(Maillage *maillage)
{
	const auto nombre_polygones = maillage->nombre_polygones();
	const auto largeur = maillage->largeur_texture();
	auto tampon = static_cast<dls::math::vec4f *>(maillage->calque_actif()->tampon);

	std::mt19937 rng(19937);
	std::uniform_real_distribution<float> dist(0.0f, 360.0f);

	for (size_t i = 0; i < nombre_polygones; ++i) {
		auto poly = maillage->polygone(i);

		auto hue = dist(rng);
		auto sat = 1.0f;
		auto val = 1.0f;
		auto couleur1 = dls::math::vec4f(1.0f);
		auto couleur2 = dls::math::vec4f(1.0f);

		converti_hsv_rvb(couleur1.x, couleur1.y, couleur1.z, hue, sat, val);
		sat = 0.5f;
		converti_hsv_rvb(couleur2.x, couleur2.y, couleur2.z, hue, sat, val);

		//std::cerr << "Couleur " << couleur1 << '\n';

		const auto index_poly = poly->x + poly->y * largeur;
		auto tampon_poly = tampon + index_poly;

		for (size_t j = 0; j < poly->res_v; ++j) {
			for (size_t k = 0; k < poly->res_u; ++k) {
				auto index = j * static_cast<size_t>(largeur) + k;

				if ((j + k) % 2 == 0) {
					tampon_poly[index] = couleur1;
				}
				else {
					tampon_poly[index] = couleur2;
				}
			}
		}
	}
}

auto echantillone_texture(TextureImage *texture_image, const dls::math::vec2f &uv)
{
	auto x = static_cast<int>(uv.x) * texture_image->largeur();
	auto y = static_cast<int>(uv.y) * texture_image->hauteur();

	if (x >= texture_image->largeur()) {
		x %= texture_image->largeur();
	}

	while (x < 0) {
		x += texture_image->largeur();
	}

	if (y >= texture_image->hauteur()) {
		y %= texture_image->hauteur();
	}

	while (y < 0) {
		y += texture_image->hauteur();
	}

	auto spectre = texture_image->echantillone_uv(x, y);
	return dls::math::vec4f(spectre[0], spectre[1], spectre[2], 1.0f);
}

void ajoute_calque_projection_triplanaire(Maillage *maillage)
{
	TextureImage *texture_image = charge_texture("/home/kevin/Téléchargements/Tileable metal scratch rust texture (8).jpg");

	const auto nombre_polygones = maillage->nombre_polygones();
	const auto largeur = maillage->largeur_texture();
	auto tampon = static_cast<dls::math::vec4f *>(maillage->calque_actif()->tampon);

	for (size_t i = 0; i < nombre_polygones; ++i) {
		auto poly = maillage->polygone(i);

		const auto angle_xy = std::abs(poly->nor.z);// abs(produit_scalaire(poly->nor, dls::math::vec3f(0.0, 0.0, 1.0)));
		const auto angle_xz = std::abs(poly->nor.y);// abs(produit_scalaire(poly->nor, dls::math::vec3f(0.0, 1.0, 0.0)));
		const auto angle_yz = std::abs(poly->nor.x);// abs(produit_scalaire(poly->nor, dls::math::vec3f(1.0, 0.0, 0.0)));

		auto poids = (angle_xy + angle_xz + angle_yz);

		if (poids == 0.0f) {
			poids = 1.0f;
		}
		else {
			poids = 1.0f / poids;
		}

		const auto &v0 = poly->s[0]->pos;
		const auto &v1 = poly->s[1]->pos;
		const auto &v3 = ((poly->s[3] != nullptr) ? poly->s[3]->pos : poly->s[2]->pos);

		const auto &e1 = v1 - v0;
		const auto &e2 = v3 - v0;

		const auto &du = e1 / static_cast<float>(poly->res_u);
		const auto &dv = e2 / static_cast<float>(poly->res_v);

		const auto index_poly = poly->x + poly->y * largeur;
		auto tampon_poly = tampon + index_poly;

		for (size_t j = 0; j < poly->res_v; ++j) {
			for (size_t k = 0; k < poly->res_u; ++k) {
				auto index = j * static_cast<size_t>(largeur) + k;

				const auto &sommet = v0 + static_cast<float>(j) * dv + static_cast<float>(k) * du;

				const auto UV_xy = dls::math::vec2f(sommet.x, sommet.y);
				const auto UV_xz = dls::math::vec2f(sommet.x, sommet.z);
				const auto UV_yz = dls::math::vec2f(sommet.y, sommet.z);

				const auto couleur_xy = echantillone_texture(texture_image, UV_xy);
				const auto couleur_xz = echantillone_texture(texture_image, UV_xz);
				const auto couleur_yz = echantillone_texture(texture_image, UV_yz);

				const auto couleur = (couleur_xy * angle_xy + couleur_xz * angle_xz + couleur_yz * angle_yz) * poids;
				tampon_poly[index] = couleur;
			}
		}
	}

	delete texture_image;
}

#undef TAILLE_UNIFORME

void assigne_texels_resolution(Maillage *maillage, unsigned int texels_par_cm)
{
	texels_par_cm = texels_par_cm * 10;

	/* calcule la resolution UV de chaque polygone en fonction de la densité */
	std::map<std::pair<unsigned int, unsigned int>, unsigned int> tableau_compte_faces;

	auto nombre_texels = 0u;

	for (size_t i = 0; i < maillage->nombre_polygones(); ++i) {
		auto p = maillage->polygone(i);
#ifdef TAILLE_UNIFORME
		p->res_u = 16;
		p->res_v = 16;
#else
		const auto cote0 = longueur(p->s[1]->pos - p->s[0]->pos);
		const auto cote1 = longueur(p->s[2]->pos - p->s[1]->pos);

		const auto res_u = static_cast<unsigned int>(cote0) * texels_par_cm;
		const auto res_v = static_cast<unsigned int>(cote1) * texels_par_cm;

		p->res_u = std::max(1u, prochain_multiple_de_2(res_u));
		p->res_v = std::max(1u, prochain_multiple_de_2(res_v));

		p->res_u = std::min(p->res_u, p->res_v);
		p->res_v = p->res_u;
#endif
		//auto paire = std::make_pair(std::max(p->res_u, p->res_v), std::min(p->res_u, p->res_v));
		auto paire = std::make_pair(p->res_u, p->res_v);

		nombre_texels += p->res_u * p->res_v;

		auto iter = tableau_compte_faces.find(paire);

		if (iter == tableau_compte_faces.end()) {
			tableau_compte_faces.insert({paire, 1});
		}
		else {
			tableau_compte_faces[paire] += 1;
		}
	}

	for (const auto &paire : tableau_compte_faces) {
		std::cerr << "Il y a " << paire.second << " polygones de " << paire.first.first << 'x' << paire.first.second << '\n';
	}

	std::cerr << "Il y a " << nombre_texels << " texels en tout.\n";
}

/* ************************************************************************** */

Maillage::Maillage()
	: m_transformation(dls::math::mat4x4d(1.0))
	, m_texture_surrannee(true)
	, m_largeur_texture(0)
	, m_nom("maillage")
{}

Maillage::~Maillage()
{
	for (auto &arrete : m_arretes) {
		delete arrete;
	}

	for (auto &sommet : m_sommets) {
		delete sommet;
	}

	for (auto &poly : m_polys) {
		delete poly;
	}
}

void Maillage::ajoute_sommet(const dls::math::vec3f &coord)
{
	auto sommet = new Sommet();
	sommet->pos = coord;
	sommet->index = m_sommets.size();
	m_sommets.push_back(sommet);
}

void Maillage::ajoute_sommets(const dls::math::vec3f *sommets, size_t nombre)
{
	m_sommets.reserve(m_sommets.size() + nombre);

	for (size_t i = 0; i < nombre; ++i) {
		ajoute_sommet(sommets[i]);
	}
}

void Maillage::ajoute_quad(const size_t s0, const size_t s1, const size_t s2, const size_t s3)
{
	auto poly = new Polygone();
	poly->s[0] = m_sommets[s0];
	poly->s[1] = m_sommets[s1];
	poly->s[2] = m_sommets[s2];
	poly->s[3] = ((s3 == -1ul) ? nullptr : m_sommets[s3]);

	const auto nombre_sommet = ((s3 == -1ul) ? 3ul : 4ul);

	for (size_t i = 0; i < nombre_sommet; ++i) {
		auto arrete = new Arrete();
		arrete->p = poly;

		arrete->s[0] = poly->s[i];
		arrete->s[1] = poly->s[(i + 1) % nombre_sommet];

		arrete->index = i;

		auto iter = m_tableau_arretes.find(std::make_pair(arrete->s[1]->index, arrete->s[0]->index));

		if (iter != m_tableau_arretes.end()) {
			arrete->opposee = iter->second;
			arrete->opposee->opposee = arrete;
		}
		else {
			arrete->opposee = nullptr;
		}

		m_tableau_arretes.insert({std::make_pair(arrete->s[0]->index, arrete->s[1]->index), arrete});

		poly->a[i] = arrete;

		m_arretes.push_back(arrete);
	}

	auto c1 = poly->s[1]->pos - poly->s[0]->pos;
	auto c2 = poly->s[2]->pos - poly->s[0]->pos;
	poly->nor = dls::math::normalise(dls::math::produit_croix(c1, c2));

	poly->index = m_polys.size();

	m_polys.push_back(poly);
}

#define COULEUR_ALEATOIRE

void Maillage::cree_tampon()
{
	assigne_texels_resolution(this, 1);

	std::cerr << "Tri des polygones...\n";

	std::sort(m_polys.begin(), m_polys.end(), [](const Polygone *a, const Polygone *b)
	{
		return a->res_u > b->res_u && a->res_v > b->res_v;
	});

	std::cerr << "Réindexage des polygones...\n";

	for (size_t i = 0; i < m_polys.size(); ++i) {
		m_polys[i]->index = i;
	}

	std::cerr << "Démarrage empaquettage...\n";

	PaqueuseTexture paqueuse;
	paqueuse.empaquete(m_polys);

	m_largeur_texture = paqueuse.largeur();
	m_canaux.hauteur = static_cast<size_t>(paqueuse.hauteur());
	m_canaux.largeur = static_cast<size_t>(paqueuse.largeur());
	auto calque = ajoute_calque(m_canaux, TypeCanal::DIFFUSION);
	calque_actif(calque);

	std::cerr << "Nombre de texels dans le tampon : " << paqueuse.hauteur() * paqueuse.largeur() << '\n';
	std::cerr << "Taille image : " << paqueuse.largeur() << "x" << paqueuse.hauteur() << '\n';

#ifndef COULEUR_ALEATOIRE
	ajoute_calque_echiquier(this);
#else
	ajoute_calque_procedurale(this);
#endif

	fusionne_calques(m_canaux);
}

Calque *Maillage::calque_actif()
{
	return m_calque_actif;
}

const CanauxTexture &Maillage::canaux_texture() const
{
	return m_canaux;
}

CanauxTexture &Maillage::canaux_texture()
{
	return m_canaux;
}

const std::string &Maillage::nom() const
{
	return m_nom;
}

void Maillage::nom(const std::string &nom)
{
	m_nom = nom;
}

bool Maillage::texture_surrannee() const
{
	return m_texture_surrannee;
}

void Maillage::marque_texture_surrannee(bool ouinon)
{
	m_texture_surrannee = ouinon;
}

unsigned int Maillage::largeur_texture() const
{
	return m_largeur_texture;
}

void Maillage::calque_actif(Calque *calque)
{
	if (m_calque_actif != nullptr) {
		m_calque_actif->drapeaux &= ~CALQUE_ACTIF;
	}

	m_calque_actif = calque;

	if (m_calque_actif != nullptr) {
		m_calque_actif->drapeaux |= CALQUE_ACTIF;
	}
}

void Maillage::transformation(const math::transformation &transforme)
{
	m_transformation = transforme;
}

const math::transformation &Maillage::transformation() const
{
	return m_transformation;
}

size_t Maillage::nombre_sommets() const
{
	return m_sommets.size();
}

size_t Maillage::nombre_polygones() const
{
	return m_polys.size();
}

const Sommet *Maillage::sommet(size_t i) const
{
	return m_sommets[i];
}

const Polygone *Maillage::polygone(size_t i) const
{
	return m_polys[i];
}

size_t Maillage::nombre_arretes() const
{
	return m_arretes.size();
}

Arrete *Maillage::arrete(size_t i)
{
	return m_arretes[i];
}

const Arrete *Maillage::arrete(size_t i) const
{
	return m_arretes[i];
}

Polygone *Maillage::polygone(size_t i)
{
	return m_polys[i];
}
