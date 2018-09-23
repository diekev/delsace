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

#include "rendu_camera.h"

#include <ego/utils.h>
#include <math/conversion_point_vecteur.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include "bibliotheques/vision/camera.h"

#include "tampon_rendu.h"

#if 0
/* Les données de sommets et d'index ont été généré en modelant une caméra dans
 * Blender et en imprimant les valeurs de sommets et d'index avec un script
 * python. Le fichier Blender et le script y associé se trouve dans le dossier
 * "autres_fichiers".
 */

static glm::vec3 donnees_sommets[62] = {
	glm::vec3( 0.0913547277450562,  0.1717057973146439, -0.1565216928720474),
	glm::vec3( 0.4345150887966156, -0.1413376480340958, -0.1565216928720474),
	glm::vec3(-0.0000010877847672, -0.1413376182317734, -0.1565216928720474),
	glm::vec3(-0.0000009983778000,  0.1717058569192886, -0.1565216928720474),
	glm::vec3( 0.0913548022508621,  0.1717057228088379,  0.1565217226743698),
	glm::vec3( 0.4345149993896484, -0.1413377225399017,  0.1565217226743698),
	glm::vec3(-0.0000011324882507, -0.1413375884294510,  0.1565217226743698),
	glm::vec3(-0.0000010430812836,  0.1717057973146439,  0.1565217226743698),
	glm::vec3( 0.4345150887966156, -0.0470707491040230,  0.1565217226743698),
	glm::vec3( 0.4345150887966156, -0.0470706671476364, -0.1565216928720474),
	glm::vec3(-0.2774190008640289, -0.1334706246852875,  0.1289154142141342),
	glm::vec3(-0.2774190008640289, -0.1334706246852875, -0.1289153844118118),
	glm::vec3(-0.1517763733863831,  0.0765047967433929,  0.0000000062044418),
	glm::vec3( 0.0000000000000000,  0.0765047967433929, -0.0000000029156393),
	glm::vec3(-0.1517763733863831,  0.0540970489382744, -0.0540970452129841),
	glm::vec3( 0.0000000000000000,  0.0540970489382744, -0.0540970526635647),
	glm::vec3(-0.1517763733863831, -0.0000000056461431, -0.0765047818422318),
	glm::vec3( 0.0000000000000000, -0.0000000056461431, -0.0765047967433929),
	glm::vec3(-0.1517763733863831, -0.0540970563888550, -0.0540970452129841),
	glm::vec3( 0.0000000000000000, -0.0540970563888550, -0.0540970526635647),
	glm::vec3(-0.1517763733863831, -0.0765047818422318,  0.0000000153245239),
	glm::vec3( 0.0000000000000000, -0.0765047818422318,  0.0000000062044418),
	glm::vec3(-0.1517763733863831, -0.0540970489382744,  0.0540970601141453),
	glm::vec3( 0.0000000149011612, -0.0540970489382744,  0.0540970563888550),
	glm::vec3(-0.1517763733863831, -0.0000000056461431,  0.0765047818422318),
	glm::vec3( 0.0000000000000000, -0.0000000056461431,  0.0765047818422318),
	glm::vec3(-0.1517763733863831,  0.0540970489382744,  0.0540970601141453),
	glm::vec3( 0.0000000149011612,  0.0540970489382744,  0.0540970563888550),
	glm::vec3(-0.2774190008640289,  0.1243601739406586, -0.1289153844118118),
	glm::vec3(-0.2774190008640289,  0.1243601739406586,  0.1289154142141342),
	glm::vec3(-0.1517820656299591, -0.1334706246852875, -0.1289153844118118),
	glm::vec3(-0.1517820656299591, -0.1334706246852875,  0.1289154142141342),
	glm::vec3(-0.1517820656299591,  0.1243601739406586, -0.1289153844118118),
	glm::vec3(-0.1517820656299591,  0.1243601739406586,  0.1289154142141342),
	glm::vec3(-0.4064837694168091, -0.1951998621225357,  0.1906446516513824),
	glm::vec3(-0.4064837694168091, -0.1951998621225357, -0.1906446218490601),
	glm::vec3(-0.4064837694168091,  0.1860894113779068, -0.1906446218490601),
	glm::vec3(-0.4064837694168091,  0.1860894113779068,  0.1906446516513824),
	glm::vec3( 0.3019568324089050,  0.5180336833000183,  0.0711456686258316),
	glm::vec3( 0.3019568324089050,  0.5180336833000183,  0.1566156297922134),
	glm::vec3( 0.6140545010566711,  0.3190738558769226,  0.0711456686258316),
	glm::vec3( 0.6140545010566711,  0.3190738558769226,  0.1566156297922134),
	glm::vec3( 0.6662012934684753,  0.2238018661737442,  0.0711456686258316),
	glm::vec3( 0.6662012934684753,  0.2238018661737442,  0.1566156297922134),
	glm::vec3( 0.6637259125709534,  0.1152204424142838,  0.0711456686258316),
	glm::vec3( 0.6637259125709534,  0.1152204424142838,  0.1566156297922134),
	glm::vec3( 0.6072913408279419,  0.0224239118397236,  0.0711456686258316),
	glm::vec3( 0.6072913408279419,  0.0224239118397236,  0.1566156297922134),
	glm::vec3( 0.5120193362236023, -0.0297229494899511,  0.0711456686258316),
	glm::vec3( 0.5120193362236023, -0.0297229494899511,  0.1566156297922134),
	glm::vec3( 0.4034379422664642, -0.0272474717348814,  0.0711456686258316),
	glm::vec3( 0.4034379422664642, -0.0272474717348814,  0.1566156297922134),
	glm::vec3( 0.0913402885198593,  0.1717123538255692,  0.0711456686258316),
	glm::vec3( 0.0913402885198593,  0.1717123538255692,  0.1566156297922134),
	glm::vec3( 0.0391933917999268,  0.2669843137264252,  0.0711456686258316),
	glm::vec3( 0.0391933917999268,  0.2669843137264252,  0.1566156297922134),
	glm::vec3( 0.0416688546538353,  0.3755657076835632,  0.0711456686258316),
	glm::vec3( 0.0416688546538353,  0.3755657076835632,  0.1566156297922134),
	glm::vec3( 0.0981033071875572,  0.4683622717857361,  0.0711456686258316),
	glm::vec3( 0.0981033071875572,  0.4683622717857361,  0.1566156297922134),
	glm::vec3( 0.1933752894401550,  0.5205091834068298,  0.0711456686258316),
	glm::vec3( 0.1933752894401550,  0.5205091834068298,  0.1566156297922134),
};

static unsigned int donnees_index[296] = {
	0, 9, 9, 1, 1, 2, 2, 3,
	4, 7, 7, 6, 6, 5, 5, 8,
	0, 4, 4, 8, 8, 5, 5, 1, 1, 9,
	1, 5, 5, 6, 6, 2,
	2, 6, 6, 7, 7, 3,
	4, 0, 0, 3, 3, 7,
	35, 34, 34, 37, 37, 36,
	12, 13, 13, 15, 15, 14,
	33, 29, 29, 10, 10, 31,
	14, 15, 15, 17, 17, 16,
	28, 32, 32, 30, 30, 11,
	16, 17, 17, 19, 19, 18,
	30, 31, 31, 10, 10, 11,
	18, 19, 19, 21, 21, 20,
	10, 29, 29, 37, 37, 34,
	32, 33, 33, 31, 31, 30,
	20, 21, 21, 23, 23, 22,
	29, 28, 28, 36, 36, 37,
	28, 29, 29, 33, 33, 32,
	22, 23, 23, 25, 25, 24,
	15, 13, 13, 27, 27, 25, 25, 23, 23, 21, 21, 19, 19, 17,
	11, 10, 10, 34, 34, 35,
	24, 25, 25, 27, 27, 26,
	28, 11, 11, 35, 35, 36,
	26, 27, 27, 13, 13, 12,
	12, 14, 14, 16, 16, 18, 18, 20, 20, 22, 22, 24, 24, 26,
	38, 39, 39, 41, 41, 40,
	40, 41, 41, 43, 43, 42,
	42, 43, 43, 45, 45, 44,
	44, 45, 45, 47, 47, 46,
	46, 47, 47, 49, 49, 48,
	48, 49, 49, 51, 51, 50,
	50, 51, 51, 53, 53, 52,
	52, 53, 53, 55, 55, 54,
	54, 55, 55, 57, 57, 56,
	56, 57, 57, 59, 59, 58,
	41, 39, 39, 61, 61, 59, 59, 57, 57, 55, 55, 53, 53, 51, 51, 49, 49, 47, 47, 45, 45, 43,
	58, 59, 59, 61, 61, 60,
	60, 61, 61, 39, 39, 38,
	38, 40, 40, 42, 42, 44, 44, 46, 46, 48, 48, 50, 50, 52, 52, 54, 54, 56, 56, 58, 58, 60
};

void RenduCamera::initialise()
{
	m_tampon = cree_tampon(glm::vec4(1.0), 1.0);

	std::vector<glm::vec3> sommets(
				std::begin(donnees_sommets), std::end(donnees_sommets));

	std::vector<unsigned int> index(
				std::begin(donnees_index), std::end(donnees_index));

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = sommets.data();
	parametres_tampon.taille_octet_sommets = sommets.size() * sizeof(glm::vec3);
	parametres_tampon.pointeur_index = index.data();
	parametres_tampon.taille_octet_index = index.size() * sizeof(unsigned int);
	parametres_tampon.elements = index.size();

	m_tampon->remplie_tampon(parametres_tampon);
}
#endif

/* ************************************************************************** */

static TamponRendu *cree_tampon(const numero7::math::vec4f &couleur)
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

	auto programme = tampon->programme();
	programme->enable();
	programme->uniform("couleur", couleur.x, couleur.y, couleur.z, couleur.w);
	programme->disable();

	return tampon;
}

/* ************************************************************************** */

RenduCamera::RenduCamera(vision::Camera3D *camera)
	: m_camera(camera)
{}

RenduCamera::~RenduCamera()
{
	delete m_tampon_aretes;
	delete m_tampon_polys;
}

void RenduCamera::initialise()
{
	numero7::math::point3f points[5] = {
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f },
	};

	for (int i = 0; i < 5; ++i) {
		points[i] = m_camera->pos_monde(points[i]);
	}

	unsigned int indices_arretes[] = {
		0, 1,
		0, 2,
		0, 3,
		0, 4,
		1, 2,
		2, 3,
		3, 4,
		4, 1,
	};

	m_tampon_aretes = cree_tampon(numero7::math::vec4f(1.0f));

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = &points[0];
	parametres_tampon.taille_octet_sommets = 5 * sizeof(numero7::math::point3f);
	parametres_tampon.pointeur_index = &indices_arretes[0];
	parametres_tampon.taille_octet_index = 16 * sizeof(unsigned int);
	parametres_tampon.elements = 16;

	m_tampon_aretes->remplie_tampon(parametres_tampon);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);

	m_tampon_aretes->parametres_dessin(parametres_dessin);

	unsigned int indices_polys[] = {
		0, 2, 1,
		0, 3, 2,
		0, 4, 3,
		0, 1, 4,
	};

	m_tampon_polys = cree_tampon(numero7::math::vec4f(0.0f, 0.0f, 0.0f, 0.5f));

	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = &points[0];
	parametres_tampon.taille_octet_sommets = 5 * sizeof(numero7::math::point3f);
	parametres_tampon.pointeur_index = &indices_polys[0];
	parametres_tampon.taille_octet_index = 12 * sizeof(unsigned int);
	parametres_tampon.elements = 12;

	m_tampon_polys->remplie_tampon(parametres_tampon);
}

void RenduCamera::dessine(const ContexteRendu &contexte)
{
	glEnable(GL_BLEND);

	m_tampon_aretes->dessine(contexte);
	m_tampon_polys->dessine(contexte);

	glDisable(GL_BLEND);
}
