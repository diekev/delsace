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

#include "rendu_texte.h"

#include "biblinternes/ego/outils.h"
#include "biblinternes/math/vecteur.hh"

#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

#include "../texture/texture.h"

#include "tampon_rendu.h"

#if 0
static constexpr auto HAUTEUR_TEXTURE_POLICE = 36;
static constexpr auto LARGEUR_TEXTURE_POLICE = 1254;
static constexpr auto LARGEUR_PAR_LETTRE = 19;

dls::dico_desordonne<char, int> table_uv_texte({
	{'a', 0}, {'b', 1}, {'c', 2}, {'d', 3}, {'e', 4}, {'f', 5}, {'g', 6},
	{'h', 7}, {'i', 8}, {'j', 9}, {'k', 10}, {'l', 11}, {'m', 12}, {'n', 13},
	{'o', 14}, {'p', 15}, {'q', 16}, {'r', 17}, {'s', 18}, {'t', 19}, {'u', 20},
	{'v', 21}, {'w', 22}, {'x', 23}, {'y', 24}, {'z', 25}, {'A', 26}, {'B', 27},
	{'C', 28}, {'D', 29}, {'E', 30}, {'F', 31}, {'G', 32}, {'H', 33}, {'I', 34},
	{'J', 35}, {'K', 36}, {'L', 37}, {'M', 38}, {'N', 39}, {'O', 40}, {'P', 41},
	{'Q', 42}, {'R', 43}, {'S', 44}, {'T', 45}, {'U', 46}, {'V', 47}, {'W', 48},
	{'X', 49}, {'Y', 50}, {'Z', 51}, {':', 52}, {' ', 53}, {',', 54}, {'.', 55},
	{'0', 56}, {'1', 57}, {'2', 58}, {'3', 59}, {'4', 60}, {'5', 61}, {'6', 62},
	{'7', 63}, {'8', 64}, {'9', 65}
});
#else
static constexpr auto HAUTEUR_TEXTURE_POLICE = 40;
static constexpr auto LARGEUR_TEXTURE_POLICE = 1748;
static constexpr auto LARGEUR_PAR_LETTRE = 19;

static dls::dico_desordonne<int, int> table_uv_texte({
	{97, 0}, {98, 1}, {99, 2}, {100, 3}, {101, 4}, {102, 5}, {103, 6}, {104, 7}, {105, 8}, {106, 9}, {107, 10}, {108, 11}, {109, 12}, {110, 13}, {111, 14}, {112, 15}, {113, 16}, {114, 17}, {115, 18}, {116, 19}, {117, 20}, {118, 21}, {119, 22}, {120, 23}, {121, 24}, {122, 25}, {65, 26}, {66, 27}, {67, 28}, {68, 29}, {69, 30}, {70, 31}, {71, 32}, {72, 33}, {73, 34}, {74, 35}, {75, 36}, {76, 37}, {77, 38}, {78, 39}, {79, 40}, {80, 41}, {81, 42}, {82, 43}, {83, 44}, {84, 45}, {85, 46}, {86, 47}, {87, 48}, {88, 49}, {89, 50}, {90, 51}, {32, 52}, {44, 53}, {63, 54}, {59, 55}, {46, 56}, {58, 57}, {47, 58}, {33, 59}, {45, 60}, {42, 61}, {48, 62}, {49, 63}, {50, 64}, {51, 65}, {52, 66}, {53, 67}, {54, 68}, {55, 69}, {56, 70}, {57, 71}, {201, 72}, {202, 73}, {200, 74}, {192, 75}, {194, 76}, {207, 77}, {206, 78}, {212, 79}, {217, 80}, {220, 81}, {233, 82}, {234, 83}, {232, 84}, {224, 85}, {226, 86}, {239, 87}, {238, 88}, {244, 89}, {249, 90}, {252, 91},
});

static void rempli_uv(std::string const &chaine, dls::tableau<dls::math::vec2f> &uvs)
{
	auto const echelle_lettres = 1.0f / static_cast<float>(table_uv_texte.taille());

	for (size_t i = 0; i < chaine.size();) {
		int valeur = -1;
		if (static_cast<unsigned char>(chaine[i]) < 192) {
			valeur = static_cast<int>(chaine[i]);
			i += 1;
		}
		else if (static_cast<unsigned char>(chaine[i]) < 224 && i + 1 < chaine.size() && static_cast<unsigned char>(chaine[i + 1]) > 127) {
			// double byte character
			valeur = ((static_cast<unsigned char>(chaine[i]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 1]) & 0x3f);
			i += 2;
		}
		else if (static_cast<unsigned char>(chaine[i]) < 240 && i + 2 < chaine.size() && static_cast<unsigned char>(chaine[i + 1]) > 127 && static_cast<unsigned char>(chaine[i + 2]) > 127) {
			// triple byte character
			valeur = ((static_cast<unsigned char>(chaine[i]) & 0x0f) << 12) | ((static_cast<unsigned char>(chaine[i + 1]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 2]) & 0x3f);

			i += 3;
		}
		else if (static_cast<unsigned char>(chaine[i]) < 248 && i + 3 < chaine.size() && static_cast<unsigned char>(chaine[i + 1]) > 127 && static_cast<unsigned char>(chaine[i + 2]) > 127 && static_cast<unsigned char>(chaine[i + 3]) > 127) {
			// 4-byte character
			valeur = ((static_cast<unsigned char>(chaine[i]) & 0x07) << 18) | ((static_cast<unsigned char>(chaine[i + 1]) & 0x0f) << 12) | ((static_cast<unsigned char>(chaine[i + 2]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 3]) & 0x3f);

			i += 4;
		}

		if (valeur == -1) {
			++i;
			continue;
		}

		auto const decalage = static_cast<float>(table_uv_texte[valeur]) * echelle_lettres;

		uvs.pousse(dls::math::vec2f(decalage, 1.0f));
		uvs.pousse(dls::math::vec2f(decalage + echelle_lettres, 1.0f));
		uvs.pousse(dls::math::vec2f(decalage + echelle_lettres, 0.0f));
		uvs.pousse(dls::math::vec2f(decalage, 0.0f));
	}
}
#endif

/* ************************************************************************** */

static const char *source_vertex =
		"#version 330 core\n"
		"layout(location = 0) in vec2 sommets;\n"
		"layout(location = 1) in vec2 uvs;\n"
		"smooth out vec2 UV;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(sommets, 0.0, 1.0);\n"
		"	UV = uvs;\n"
		"}\n";

static const char *source_fragment =
		"#version 330 core\n"
		"layout (location = 0) out vec4 couleur_fragment;\n"
		"uniform sampler2D texture_texte;\n"
		"smooth in vec2 UV;\n"
		" void main()\n"
		"{\n"
		"	vec2 UV_inverse = vec2(UV.x, 1.0f - UV.y);\n"
		"	vec3 couleur = texture2D(texture_texte, UV_inverse).rgb;\n"
		"	couleur_fragment = vec4(couleur, couleur.r);\n"
		"}\n";

static TamponRendu *cree_tampon()
{
	auto tampon = new TamponRendu;

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				source_vertex);

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				source_fragment);

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("uvs");
	parametre_programme.ajoute_uniforme("texture_texte");
	parametre_programme.ajoute_uniforme("couleur");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("matrice");

	tampon->parametres_programme(parametre_programme);

	tampon->ajoute_texture();

	auto texture = tampon->texture();

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 1.0f, 0.0f, 1.0f, 1.0f);
	programme->uniforme("texture_texte", texture->code_attache());
	programme->desactive();

	return tampon;
}

static void genere_texture(dls::ego::Texture2D *texture, const void *data, GLint size[2])
{
	texture->deloge(true);
	texture->attache();
	texture->type(GL_FLOAT, GL_RGB, GL_RGB);
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP);
	texture->remplie(data, size);
	texture->detache();
}

/* ************************************************************************** */

RenduTexte::~RenduTexte()
{
	delete m_texture;
	delete m_tampon;
}

void RenduTexte::reinitialise()
{
	m_decalage = 1.0f;
}

void RenduTexte::ajourne(std::string const &texte)
{
	if (!m_tampon) {
		m_tampon = cree_tampon();
		m_texture = charge_texture("texte/texture_texte.png");

		int taille[2] = { m_texture->largeur(), m_texture->hauteur() };

		genere_texture(m_tampon->texture(), m_texture->donnees(), taille);
	}

	/* Une texture avec toutes les lettres. */

	dls::tableau<dls::math::vec2f> vertex;
	dls::tableau<unsigned int> index;

	/* Calcul taille des lettres relativement à la fenêtre. */
	auto const largeur_lettre = LARGEUR_PAR_LETTRE / static_cast<float>(m_largeur);
	auto const hauteur_lettre = HAUTEUR_TEXTURE_POLICE / static_cast<float>(m_hauteur);

	/* Un quad par lettre du mot. */
	for (size_t i = 0; i < texte.size(); ++i) {
		auto const origine_x = static_cast<float>(i) * largeur_lettre;

		vertex.pousse(dls::math::vec2f(origine_x, hauteur_lettre));
		vertex.pousse(dls::math::vec2f(origine_x + largeur_lettre, hauteur_lettre));
		vertex.pousse(dls::math::vec2f(origine_x + largeur_lettre, 0.0f));
		vertex.pousse(dls::math::vec2f(origine_x, 0.0f));

		index.pousse(static_cast<unsigned>(i * 4 + 0));
		index.pousse(static_cast<unsigned>(i * 4 + 1));
		index.pousse(static_cast<unsigned>(i * 4 + 2));
		index.pousse(static_cast<unsigned>(i * 4 + 0));
		index.pousse(static_cast<unsigned>(i * 4 + 2));
		index.pousse(static_cast<unsigned>(i * 4 + 3));
	}

	/* Placement en haut à gauche de l'écran. */
	m_decalage += 1.0f;

	const dls::math::vec2f decalage_vertex(
				-1.0f + largeur_lettre,
				1.0f - m_decalage * hauteur_lettre);

	for (auto &v : vertex) {
		v += decalage_vertex;
	}

	/* Une fonction qui associe lettre -> coordonnées UV */
	dls::tableau<dls::math::vec2f> uvs;

#if 0
	auto const echelle_lettres = 1.0 / table_uv_texte.size();

	for (auto const &caractere : texte) {
		auto const decalage = table_uv_texte[caractere] * echelle_lettres;

		uvs.pousse(dls::math::vec2f(decalage, 1.0f));
		uvs.pousse(dls::math::vec2f(decalage + echelle_lettres, 1.0f));
		uvs.pousse(dls::math::vec2f(decalage + echelle_lettres, 0.0f));
		uvs.pousse(dls::math::vec2f(decalage, 0.0f));
	}
#else
	rempli_uv(texte, uvs);
#endif

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.elements = static_cast<size_t>(index.taille());
	parametres_tampon.pointeur_index = index.donnees();
	parametres_tampon.pointeur_sommets = vertex.donnees();
	parametres_tampon.taille_octet_sommets = static_cast<size_t>(vertex.taille()) * sizeof(dls::math::vec2f);
	parametres_tampon.taille_octet_index = static_cast<size_t>(index.taille()) * sizeof(unsigned int);
	parametres_tampon.dimension_attribut = 2;

	m_tampon->remplie_tampon(parametres_tampon);

	parametres_tampon.attribut = "uvs";
	parametres_tampon.pointeur_donnees_extra = uvs.donnees();
	parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(uvs.taille()) * sizeof(dls::math::vec2f);
	parametres_tampon.dimension_attribut = 2;

	m_tampon->remplie_tampon_extra(parametres_tampon);
}

void RenduTexte::etablie_dimension_fenetre(int largeur, int hauteur)
{
	m_largeur = largeur;
	m_hauteur = hauteur;
}

void RenduTexte::dessine(ContexteRendu const &contexte, std::string const &texte)
{
	this->ajourne(texte);
	m_tampon->dessine(contexte);
}
