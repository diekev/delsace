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

#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

#include "biblinternes/texture/texture.h"

#include "texte/freetype-gl.h"
#include "texte/texture-atlas.h"
#include "texte/mat4.h"
#include "texte/markup.h"
#include "texte/shader.h"
#include "texte/vertex-buffer.h"

#include "tampon_rendu.h"

/* ************************************************************************** */

static const char *source_vertex_texte =
		"#version 330 core\n"
		"layout(location = 0) in vec2 vertex;\n"
		"layout(location = 1) in vec2 tex_coord;\n"
		"layout(location = 2) in vec4 color;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"smooth out vec2 UV;\n"
		"smooth out vec4 couleur;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(vertex.x * 2.0 - 1.0, (vertex.y) * 2.0 - 1.0, 0.0, 1.0);\n"
		"	UV = tex_coord;\n"
		"   couleur = color;\n"
		"}\n";

static const char *source_fragment_texte =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		"uniform sampler2D atlas;\n"
		"smooth in vec4 couleur;\n"
		"smooth in vec2 UV;\n"
		"void main()\n"
		"{\n"
		"	float a = texture2D(atlas, UV).r;\n"
		"	fragment_color = vec4(couleur.xyz, couleur.a*a);\n"
		"}\n";

static TamponRendu *cree_tampon()
{
	auto tampon = new TamponRendu{};

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				source_vertex_texte);

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				source_fragment_texte);

	tampon->finalise_programme();

	auto params = ParametresProgramme{};
	params.ajoute_attribut("vertex");
	params.ajoute_attribut("tex_coord");
	params.ajoute_attribut("color");
	params.ajoute_uniforme("MVP");
	params.ajoute_uniforme("matrice");
	params.ajoute_uniforme("atlas");

	tampon->parametres_programme(params);

	return tampon;
}

static void genere_texture(dls::ego::Texture2D *texture, texture_atlas_t *atlas)
{
	int taille[2] = { static_cast<int>(atlas->width), static_cast<int>(atlas->height) };

	texture->deloge(true);
	texture->attache();
	texture->type(GL_UNSIGNED_BYTE, GL_RED, GL_RED);
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP_TO_EDGE);
	texture->remplie(atlas->data, taille);
	texture->detache();

	atlas->id = texture->code_attache();
}

/* ************************************************************************** */

RenduTexte::~RenduTexte()
{
	delete m_texture;
	delete m_tampon;

	if (m_atlas != nullptr) {
		m_atlas->id = 0;
		texture_atlas_delete(m_atlas);
		texture_font_delete(m_font);
	}
}

void RenduTexte::reinitialise()
{
	m_decalage = 1.0f;
}

void RenduTexte::ajourne(dls::chaine const &texte)
{
}

void RenduTexte::etablie_dimension_fenetre(int largeur, int hauteur)
{
	m_largeur = largeur;
	m_hauteur = hauteur;
}

static void rend_lettres(
		TamponRendu &tampon,
		dls::chaine const &texte,
		texture_atlas_t *atlas,
		markup_t &markup,
		dls::math::vec2d const &pos,
		dls::math::vec2d const &taille)
{
	vec2 pen = {
		{static_cast<float>(pos.x),
		 static_cast<float>(taille.y - pos.y - 16.0)}
	};

	std::vector<dls::math::vec2f> sommets;
	std::vector<dls::math::vec2f> uvs;
	std::vector<dls::math::vec4f> colors;
	std::vector<unsigned int> indices;

	auto inv_taille = dls::math::vec2f(1.0f /
									   static_cast<float>(taille.x),
									   1.0f / static_cast<float>(taille.y));

	auto r = markup.foreground_color.r;
	auto g = markup.foreground_color.g;
	auto b = markup.foreground_color.b;
	auto a = markup.foreground_color.a;

	for(auto i = 0; i < texte.taille(); ++i) {
		texture_glyph_t *glyph = texture_font_get_glyph(markup.font, &texte[i]);

		if (glyph == nullptr) {
			continue;
		}

		float kerning = 0.0f;

		if (i > 0) {
			kerning = texture_glyph_get_kerning(glyph, &texte[i - 1]);
		}

		pen.x += kerning;
		auto x0 = pen.x + static_cast<float>(glyph->offset_x);
		auto y0 = pen.y + static_cast<float>(glyph->offset_y);
		auto x1 = x0 + static_cast<float>(glyph->width);
		auto y1 = y0 - static_cast<float>(glyph->height);
		auto s0 = glyph->s0;
		auto t0 = glyph->t0;
		auto s1 = glyph->s1;
		auto t1 = glyph->t1;

		auto decalage_index = static_cast<unsigned int>(sommets.size());
		indices.push_back(0 + decalage_index);
		indices.push_back(1 + decalage_index);
		indices.push_back(2 + decalage_index);
		indices.push_back(0 + decalage_index);
		indices.push_back(2 + decalage_index);
		indices.push_back(3 + decalage_index);

		sommets.push_back(dls::math::vec2f(x0, y0) * inv_taille);
		sommets.push_back(dls::math::vec2f(x0, y1) * inv_taille);
		sommets.push_back(dls::math::vec2f(x1, y1) * inv_taille);
		sommets.push_back(dls::math::vec2f(x1, y0) * inv_taille);

		uvs.push_back(dls::math::vec2f(s0,t0));
		uvs.push_back(dls::math::vec2f(s0,t1));
		uvs.push_back(dls::math::vec2f(s1,t1));
		uvs.push_back(dls::math::vec2f(s1,t0));

		colors.push_back(dls::math::vec4f(r, g, b, a));
		colors.push_back(dls::math::vec4f(r, g, b, a));
		colors.push_back(dls::math::vec4f(r, g, b, a));
		colors.push_back(dls::math::vec4f(r, g, b, a));

		pen.x += glyph->advance_x;
	}

	auto params_tampon = ParametresTampon{};
	params_tampon.attribut = "vertex";
	params_tampon.dimension_attribut = 2;
	params_tampon.pointeur_sommets = sommets.data();
	params_tampon.taille_octet_sommets = sizeof(dls::math::vec2f) * sommets.size();
	params_tampon.elements = indices.size();
	params_tampon.pointeur_index = indices.data();
	params_tampon.taille_octet_index = sizeof(unsigned int) * indices.size();

	tampon.remplie_tampon(params_tampon);

	params_tampon.attribut = "tex_coord";
	params_tampon.dimension_attribut = 2;
	params_tampon.pointeur_donnees_extra = uvs.data();
	params_tampon.taille_octet_donnees_extra = sizeof(dls::math::vec2f) * uvs.size();

	tampon.remplie_tampon_extra(params_tampon);

	params_tampon.attribut = "color";
	params_tampon.dimension_attribut = 4;
	params_tampon.pointeur_donnees_extra = colors.data();
	params_tampon.taille_octet_donnees_extra = sizeof(dls::math::vec4f) * colors.size();

	tampon.remplie_tampon_extra(params_tampon);

	if (atlas == nullptr) {
		std::cerr << "L'atlas est nul !\n";
	}
	else if (atlas->data == nullptr) {
		std::cerr << "Les données de l'atlas sont nuls !\n";
	}

	//genere_texture(tampon.texture(), atlas);

	glGenTextures( 1, &atlas->id );
	glBindTexture( GL_TEXTURE_2D, atlas->id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, static_cast<int>(atlas->width), static_cast<int>(atlas->height),
				  0, GL_RED, GL_UNSIGNED_BYTE, atlas->data );

	{
		auto programme = tampon.programme();

		if (!programme->est_valide()) {
			std::cerr << "Programme invalide !\n";
			return;
		}

		dls::ego::util::GPU_check_errors("Erreur lors du rendu du tampon avant utilisation programme");

		programme->active();

		programme->uniforme("atlas", 0);

		tampon.donnees()->attache();
		//tampon.texture()->attache();

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après attache texture");

		mat4   model, view, projection;
		mat4_set_identity(&projection);
		mat4_set_identity(&model);
		mat4_set_identity(&view);
		glUniformMatrix4fv((*programme)("MVP"), 1, 0, model.data);
		glUniformMatrix4fv((*programme)("matrice"), 1, 0, view.data);
		//glUniformMatrix4fv((*programme)("projection"), 1, 0, projection.data);

		glDrawElements(tampon.parametres_dessin().type_dessin(),
					   static_cast<int>(indices.size()),
					   tampon.parametres_dessin().type_donnees(),
					   nullptr);

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après dessin indexé");

		//tampon.texture()->detache();
		tampon.donnees()->detache();
		programme->desactive();
	}

	glDeleteTextures( 1, &atlas->id );
}

void RenduTexte::dessine(
		ContexteRendu const &contexte,
		dls::chaine const &texte,
		dls::math::vec4f const &couleur)
{
	char const *filename = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";

	vec4 white  = {{couleur.r, couleur.g, couleur.b, couleur.a}};
	vec4 noir   = {{0.0, 0.0, 0.0, 1.0}};
	vec4 none   = {{1.0, 1.0, 1.0, 0.0}};

	markup_t markup;
	markup.family  = const_cast<char *>(filename);
	markup.size    = static_cast<float>(m_taille_fonte);
	markup.bold    = 0;
	markup.italic  = 0;
	markup.spacing = 0.0;
	markup.gamma   = 1.5;
	markup.foreground_color    = white;
	markup.background_color    = none;
	markup.underline           = 0;
	markup.underline_color     = white;
	markup.overline            = 0;
	markup.overline_color      = white;
	markup.strikethrough       = 0;
	markup.strikethrough_color = white;

	if (!m_tampon) {
		m_tampon = cree_tampon();
		m_atlas = texture_atlas_new(512, 512, 1);
		m_font = texture_font_new_from_file(m_atlas, markup.size, markup.family);

		//m_tampon->ajoute_texture();
		//genere_texture(m_tampon->texture(), m_atlas);
	}

	markup.font = m_font;

	auto pos = dls::math::vec2d(8.0, 8.0 + m_decalage);
	auto taille = dls::math::vec2d(this->m_largeur, this->m_hauteur);

	markup.foreground_color = noir;
	markup.font->rendermode = RENDER_OUTLINE_EDGE;
	markup.font->outline_thickness = 0.2f;

	rend_lettres(*m_tampon, texte, m_atlas, markup, pos, taille);

	markup.foreground_color = white;
	markup.font->rendermode = RENDER_NORMAL;

	rend_lettres(*m_tampon, texte, m_atlas, markup, pos, taille);

	m_decalage += m_taille_fonte;
}
