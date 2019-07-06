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

#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/transformation/transformation.h"

#include "coeur/lumiere.h"
#include "coeur/maillage.h"
#include "coeur/nuanceur.h"
#include "coeur/scene.h"

/* ************************************************************************** */

template <typename T>
static auto moyenne(
		dls::math::vec3<T> const &v0,
		dls::math::vec3<T> const &v1,
		dls::math::vec3<T> const &v2)
{
	return (v0 + v1 + v2) / static_cast<T>(3);
}

/* ************************************************************************** */

RenduMaillage::RenduMaillage(Maillage *maillage)
	: m_maillage(maillage)
{}

RenduMaillage::~RenduMaillage()
{
	delete m_tampon_surface;
	delete m_tampon_normal;
}

void RenduMaillage::genere_tampon_surface()
{
	if (m_tampon_surface != nullptr) {
		return;
	}

	m_tampon_surface = new TamponRendu;

	m_tampon_surface->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/diffus.vert"));

	m_tampon_surface->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/diffus.frag"));

	m_tampon_surface->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normal");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	for (size_t i = 0; i < 8; ++i) {
		parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].couleur");
		parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].position");
		parametre_programme.ajoute_uniforme("lumieres[" + std::to_string(i) + "].type");
	}

	m_tampon_surface->parametres_programme(parametre_programme);

	auto programme = m_tampon_surface->programme();
	programme->active();
	programme->uniforme("couleur", 1.0f, 1.0f, 1.0f, 1.0f);
	programme->desactive();

	auto nombre_sommets = std::distance(m_maillage->begin(), m_maillage->end());

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(static_cast<size_t>(nombre_sommets * 3));

	std::vector<dls::math::vec3f> normaux;
	normaux.reserve(static_cast<size_t>(nombre_sommets * 3));

	/* OpenGL ne travaille qu'avec des floats. */
	for (const Triangle *triangle : *m_maillage) {
		sommets.push_back(dls::math::vec3f(static_cast<float>(triangle->v0.x), static_cast<float>(triangle->v0.y), static_cast<float>(triangle->v0.z)));
		sommets.push_back(dls::math::vec3f(static_cast<float>(triangle->v1.x), static_cast<float>(triangle->v1.y), static_cast<float>(triangle->v1.z)));
		sommets.push_back(dls::math::vec3f(static_cast<float>(triangle->v2.x), static_cast<float>(triangle->v2.y), static_cast<float>(triangle->v2.z)));

		auto normal = dls::math::vec3f(static_cast<float>(triangle->normal.x), static_cast<float>(triangle->normal.y), static_cast<float>(triangle->normal.z));

		normaux.push_back(normal);
		normaux.push_back(normal);
		normaux.push_back(normal);
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

	m_tampon_surface->remplie_tampon(parametres_tampon);

	parametres_tampon.attribut = "normal";
	parametres_tampon.pointeur_donnees_extra = normaux.data();
	parametres_tampon.taille_octet_donnees_extra = normaux.size() * sizeof(dls::math::vec3f);

	m_tampon_surface->remplie_tampon_extra(parametres_tampon);
}

void RenduMaillage::genere_tampon_normal()
{
	if (m_tampon_normal != nullptr) {
		return;
	}

	m_tampon_normal = new TamponRendu;

	m_tampon_normal->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::ego::util::str_from_file("nuanceurs/simple.vert"));

	m_tampon_normal->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::ego::util::str_from_file("nuanceurs/simple.frag"));

	m_tampon_normal->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normal");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	m_tampon_normal->parametres_programme(parametre_programme);

	auto programme = m_tampon_normal->programme();
	programme->active();
	programme->uniforme("couleur", 0.5f, 1.0f, 0.5f, 1.0f);
	programme->desactive();

	auto nombre_triangle = std::distance(m_maillage->begin(), m_maillage->end());

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(static_cast<size_t>(nombre_triangle * 2));

	for (const Triangle *triangle : *m_maillage) {
		auto const &N = normalise(triangle->normal);
		auto const &V = moyenne(triangle->v0, triangle->v1, triangle->v2);

		auto const &NV = V + N;

		sommets.push_back(dls::math::vec3f(static_cast<float>(V.x), static_cast<float>(V.y), static_cast<float>(V.z)));
		sommets.push_back(dls::math::vec3f(static_cast<float>(NV.x), static_cast<float>(NV.y), static_cast<float>(NV.z)));
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

	m_tampon_normal->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	m_tampon_normal->parametres_dessin(parametres_dessin);
}

void RenduMaillage::initialise()
{
	genere_tampon_surface();
	genere_tampon_normal();
}

void RenduMaillage::dessine(ContexteRendu const &contexte, Scene const &scene)
{
	Spectre spectre(1.0f);
	auto nuanceur = m_maillage->nuanceur();

	switch (nuanceur->type) {
		case TypeNuanceur::DIFFUS:
			spectre = dynamic_cast<NuanceurDiffus *>(nuanceur)->spectre;
			break;
		case TypeNuanceur::ANGLE_VUE:
			spectre = dynamic_cast<NuanceurAngleVue *>(nuanceur)->spectre;
			break;
		default:
			break;
	}

	auto programme = m_tampon_surface->programme();
	programme->active();
	programme->uniforme("couleur", spectre[0], spectre[1], spectre[2], 1.0f);

	/* À FAIRE : trouve les lumières les plus proches. */
	auto const nombre_lumiere = std::min(8ul, scene.lumieres.size());

	for (size_t i = 0; i < nombre_lumiere; ++i) {
		auto lumiere = scene.lumieres[i];
		spectre = lumiere->spectre;
		auto transform = lumiere->transformation.matrice();
		programme->uniforme("lumieres[" + std::to_string(i) + "].couleur", spectre[0], spectre[1], spectre[2], 1.0f);

		if (lumiere->type == type_lumiere::DISTANTE) {
			programme->uniforme("lumieres[" + std::to_string(i) + "].position", 0.0f, 0.0f, 1.0f);
		}
		else {
			programme->uniforme("lumieres[" + std::to_string(i) + "].position", float(transform[0][3]), float(transform[1][3]), float(transform[2][3]));
		}

		programme->uniforme("lumieres[" + std::to_string(i) + "].type", static_cast<int>(lumiere->type));
	}

	programme->desactive();

	m_tampon_surface->dessine(contexte);

	if (m_maillage->dessine_normaux()) {
		m_tampon_normal->dessine(contexte);
	}
}

dls::math::mat4x4d RenduMaillage::matrice() const
{
	return m_maillage->transformation().matrice();
}
