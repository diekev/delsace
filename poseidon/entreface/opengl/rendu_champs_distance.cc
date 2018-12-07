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

#include "rendu_champs_distance.h"

#include <ego/outils.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"

#include "coeur/fluide.h"
#include "coeur/maillage.h"

/* ************************************************************************** */

static TamponRendu *cree_tampon()
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
	parametres_dessin.type_dessin(GL_POINTS);
	parametres_dessin.taille_point(3.0f);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 1.0f, 0.0f, 1.0f, 1.0f);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

RenduChampsDistance::RenduChampsDistance(Fluide *fluide)
	: m_fluide(fluide)
{}

RenduChampsDistance::~RenduChampsDistance()
{
	delete m_tampon;
}

void RenduChampsDistance::initialise()
{
	const auto &min_domaine = m_fluide->domaine->min();
	const auto &taille_domaine = m_fluide->domaine->taille();

	auto dh = dls::math::vec3f(
				  taille_domaine.x / float(m_fluide->res.x),
				  taille_domaine.y / float(m_fluide->res.y),
				  taille_domaine.z / float(m_fluide->res.z));

	auto dh2 = dh * 0.5f;

	std::vector<dls::math::vec3f> sommets;
	sommets.reserve(m_fluide->res.x * m_fluide->res.y * m_fluide->res.z);

	for (size_t x = 0; x < m_fluide->res.x; ++x) {
		for (size_t y = 0; y < m_fluide->res.y; ++y) {
			for (size_t z = 0; z < m_fluide->res.z; ++z) {
				if (m_fluide->phi.valeur(x, y, z) > 0.250f) {
					continue;
				}

				auto pos = min_domaine;
				pos.x += static_cast<float>(x) * dh.x + dh2.x;
				pos.y += static_cast<float>(y) * dh.y + dh2.y;
				pos.z += static_cast<float>(z) * dh.z + dh2.z;

				sommets.push_back(pos);
			}
		}
	}

	m_tampon = cree_tampon();

	ParametresTampon parametres;
	parametres.attribut = "sommets";
	parametres.dimension_attribut = 3;
	parametres.elements = sommets.size();
	parametres.pointeur_sommets = sommets.data();
	parametres.taille_octet_sommets = sommets.size() * sizeof(dls::math::vec3f);

	m_tampon->remplie_tampon(parametres);
}

void RenduChampsDistance::dessine(const ContexteRendu &contexte)
{
	m_tampon->dessine(contexte);
}
