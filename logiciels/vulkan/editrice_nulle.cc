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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editrice_nulle.hh"

#include <ego/outils.h>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/tampon_rendu.h"

#include <cstring>

#include "texte/freetype-gl.h"
#include "texte/texture-atlas.h"
#include "texte/mat4.h"
#include "texte/shader.h"
#include "texte/vertex-buffer.h"

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

/* ************************************************************************** */

static void dessine_texte(
		const char *texte,
		dls::math::vec2d const &pos,
		dls::math::vec2d const &taille)
{
	auto tampon = TamponRendu{};

	tampon.charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				source_vertex_texte);

	tampon.charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				source_fragment_texte);

	tampon.finalise_programme();

	auto params = ParametresProgramme{};
	params.ajoute_attribut("vertex");
	params.ajoute_attribut("tex_coord");
	params.ajoute_attribut("color");
	params.ajoute_uniforme("MVP");
	params.ajoute_uniforme("matrice");
	params.ajoute_uniforme("atlas");

	tampon.parametres_programme(params);

	const char * filename = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
	auto atlas = texture_atlas_new( 512, 512, 1 );
	auto font = texture_font_new_from_file(atlas, 16, filename);
	texture_font_load_glyphs(font, texte);

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

	float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
	for(size_t i = 0; i < std::strlen(texte); ++i ) {
		texture_glyph_t *glyph = texture_font_get_glyph( font, texte + i );

		if (glyph == nullptr) {
			continue;
		}

		float kerning = 0.0f;

		if (i > 0) {
			kerning = texture_glyph_get_kerning( glyph, texte + i - 1 );
		}

		pen.x += kerning;
		auto x0 = static_cast<float>( pen.x + glyph->offset_x );
		auto y0 = static_cast<float>( pen.y + glyph->offset_y );
		auto x1 = static_cast<float>( x0 + glyph->width );
		auto y1 = static_cast<float>( y0 - glyph->height );
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

	glGenTextures( 1, &atlas->id );
	glBindTexture( GL_TEXTURE_2D, atlas->id );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, atlas->width, atlas->height,
				  0, GL_RED, GL_UNSIGNED_BYTE, atlas->data );

	{
		auto programme = tampon.programme();

		if (!programme->est_valide()) {
			std::cerr << "Programme invalide !\n";
			return;
		}

		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon avant utilisation programme");

		programme->active();

		programme->uniforme("atlas", 0);

		tampon.donnees()->bind();

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après attache texture");

		mat4   model, view, projection;
		mat4_set_identity( &projection );
		mat4_set_identity( &model );
		mat4_set_identity( &view );
		glUniformMatrix4fv( (*programme)("MVP"), 1, 0, model.data);
		glUniformMatrix4fv( (*programme)("matrice" ), 1, 0, view.data);
		//glUniformMatrix4fv( (*programme)("projection" ), 1, 0, projection.data);

		glDrawElements(tampon.parametres_dessin().type_dessin(),
					   static_cast<int>(indices.size()),
					   tampon.parametres_dessin().type_donnees(),
					   nullptr);

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après dessin indexé");

		tampon.donnees()->unbind();
		programme->desactive();
	}

	glDeleteTextures( 1, &atlas->id );
	atlas->id = 0;
	texture_atlas_delete( atlas );
}

/* ************************************************************************** */

static auto calcule_taille_texte(const char *texte)
{
	const char * filename = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
	auto font = texture_font_new_from_file(nullptr, 16, filename);

	auto taille = dls::math::vec2f(0.0f);

	for(size_t i = 0; i < std::strlen(texte); ++i ) {
		texture_glyph_t *glyph = texture_font_get_glyph( font, texte + i );

		if (glyph == nullptr) {
			continue;
		}

		float kerning = 0.0f;

		if (i > 0) {
			kerning = texture_glyph_get_kerning( glyph, texte + i - 1 );
		}

		taille.x += kerning + glyph->width + glyph->offset_x + glyph->advance_x;
		taille.y += glyph->height + glyph->offset_y;
	}

	return taille;
}

/* ************************************************************************** */

static const char *source_vertex_rectangle =
		"#version 330 core\n"
		"layout(location = 0) in vec2 vertex;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(vertex.x * 2.0 - 1.0, (vertex.y) * 2.0 - 1.0, 0.0, 1.0);\n"
		"}\n";

static const char *source_fragment_rectangle =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		"uniform vec4 couleur;\n"
		"void main()\n"
		"{\n"
		"	fragment_color = couleur;\n"
		"}\n";

static void dessine_rectangle(
		dls::math::vec2d const &pos,
		float largeur,
		float hauteur,
		dls::math::vec2d const &taille)
{
	auto min_x = static_cast<float>(pos.x);
	auto max_x = static_cast<float>(min_x) + largeur;
	auto min_y = static_cast<float>(taille.y - pos.y) - hauteur;
	auto max_y = static_cast<float>(min_y) + hauteur;

	min_x /= static_cast<float>(taille.x);
	max_x /= static_cast<float>(taille.x);
	min_y /= static_cast<float>(taille.y);
	max_y /= static_cast<float>(taille.y);

	unsigned int indices[6] = {
		0, 1, 2, 0, 2, 3
	};

	float sommets[8] = {
		min_x, min_y,
		min_x, max_y,
		max_x, max_y,
		max_x, min_y,
	};

	auto tampon = TamponRendu{};

	tampon.charge_source_programme(
				numero7::ego::Nuanceur::VERTEX,
				source_vertex_rectangle);

	tampon.charge_source_programme(
				numero7::ego::Nuanceur::FRAGMENT,
				source_fragment_rectangle);

	tampon.finalise_programme();

	auto params = ParametresProgramme{};
	params.ajoute_attribut("vertex");
	params.ajoute_uniforme("couleur");

	tampon.parametres_programme(params);

	auto params_tampon = ParametresTampon{};
	params_tampon.attribut = "vertex";
	params_tampon.dimension_attribut = 2;
	params_tampon.pointeur_sommets = sommets;
	params_tampon.taille_octet_sommets = sizeof(dls::math::vec2f) * 4;
	params_tampon.elements = 6;
	params_tampon.pointeur_index = indices;
	params_tampon.taille_octet_index = sizeof(unsigned int) * 6;

	tampon.remplie_tampon(params_tampon);

	{
		auto programme = tampon.programme();

		if (!programme->est_valide()) {
			std::cerr << "Programme invalide !\n";
			return;
		}

		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon avant utilisation programme");

		programme->active();

		programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);

		tampon.donnees()->bind();

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après attache texture");

		glDrawElements(tampon.parametres_dessin().type_dessin(),
					   6,
					   tampon.parametres_dessin().type_donnees(),
					   nullptr);

		//		numero7::ego::util::GPU_check_errors("Erreur lors du rendu du tampon après dessin indexé");

		tampon.donnees()->unbind();
		programme->desactive();
	}
}

static void dessine_bouton(
		const char *texte,
		dls::math::vec2d const &pos,
		dls::math::vec2d const &taille)
{
	auto taille_texte = dls::math::vec2f(50.0f, 16.0f);//calcule_taille_texte(texte);

	/* dessine l'arrière plan */
	dessine_rectangle(pos, taille_texte.x + 10.0f, taille_texte.y + 4.0f, taille);

	/* dessine le texte */
	dessine_texte(texte, pos + dls::math::vec2d(5.0, 0.0), taille);
}

/* ************************************************************************** */

static const char *source_vertex =
		"#version 330 core\n"
		"layout(location = 0) in vec2 vertex;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(vertex.x * 2.0 - 1.0, (vertex.y) * 2.0 - 1.0, 0.0, 1.0);\n"
		"}\n";

static const char *source_fragment =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		"uniform vec4 couleur;\n"
		"void main()\n"
		"{\n"
		"	fragment_color = couleur;\n"
		"}\n";

TamponRendu *cree_tampon_image(
		dls::math::vec2d const &pos,
		dls::math::vec2d const &taille,
		dls::math::vec4f const &couleur)
{
	auto tampon = new TamponRendu{};

	tampon->charge_source_programme(numero7::ego::Nuanceur::VERTEX, source_vertex);
	tampon->charge_source_programme(numero7::ego::Nuanceur::FRAGMENT, source_fragment);
	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("vertex");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", couleur.x, couleur.y, couleur.z, couleur.w);
	programme->desactive();

#if 0
	auto min = pos;
	auto max = pos + taille;

	float sommets[8] = {
		static_cast<float>(min.x), static_cast<float>(min.y),
		static_cast<float>(max.x), static_cast<float>(min.y),
		static_cast<float>(max.x), static_cast<float>(max.y),
		static_cast<float>(min.x), static_cast<float>(max.y)
	};

	unsigned int index[6] = { 0, 1, 2, 0, 2, 3 };

	auto parametres_tampon = ParametresTampon();
	parametres_tampon.attribut = "vertex";
	parametres_tampon.dimension_attribut = 2;
	parametres_tampon.pointeur_sommets = sommets;
	parametres_tampon.taille_octet_sommets = sizeof(float) * 8;
	parametres_tampon.pointeur_index = index;
	parametres_tampon.taille_octet_index = sizeof(unsigned int) * 6;
	parametres_tampon.elements = 6;
#else
	dls::math::vec2f roundRect[41] = {
		dls::math::vec2f{ 0.92f, 0.0f },
		dls::math::vec2f{ 0.933892f, 0.001215f },
		dls::math::vec2f{ 0.947362f, 0.004825f },
		dls::math::vec2f{ 0.96f, 0.010718f },
		dls::math::vec2f{ 0.971423f, 0.018716f },
		dls::math::vec2f{ 0.981284f, 0.028577f },
		dls::math::vec2f{ 0.989282f, 0.04f },
		dls::math::vec2f{ 0.995175f, 0.052638f },
		dls::math::vec2f{ 0.998785f, 0.066108f },
		dls::math::vec2f{ 1.0f, 0.08f },
		dls::math::vec2f{ 1.0f, 0.92f },
		dls::math::vec2f{ 0.998785f, 0.933892f },
		dls::math::vec2f{ 0.995175f, 0.947362f },
		dls::math::vec2f{ 0.989282f, 0.96f },
		dls::math::vec2f{ 0.981284f, 0.971423f },
		dls::math::vec2f{ 0.971423f, 0.981284f },
		dls::math::vec2f{ 0.96f, 0.989282f },
		dls::math::vec2f{ 0.947362f, 0.995175f },
		dls::math::vec2f{ 0.933892f, 0.998785f },
		dls::math::vec2f{ 0.92f, 1.0f },
		dls::math::vec2f{ 0.08f, 1.0f },
		dls::math::vec2f{ 0.066108f, 0.998785f },
		dls::math::vec2f{ 0.052638f, 0.995175f },
		dls::math::vec2f{ 0.04f, 0.989282f },
		dls::math::vec2f{ 0.028577f, 0.981284f },
		dls::math::vec2f{ 0.018716f, 0.971423f },
		dls::math::vec2f{ 0.010718f, 0.96f },
		dls::math::vec2f{ 0.004825f, 0.947362f },
		dls::math::vec2f{ 0.001215f, 0.933892f },
		dls::math::vec2f{ 0.0f, 0.92f },
		dls::math::vec2f{ 0.0f, 0.08f },
		dls::math::vec2f{ 0.001215f, 0.066108f },
		dls::math::vec2f{ 0.004825f, 0.052638f },
		dls::math::vec2f{ 0.010718f, 0.04f },
		dls::math::vec2f{ 0.018716f, 0.028577f },
		dls::math::vec2f{ 0.028577f, 0.018716f },
		dls::math::vec2f{ 0.04f, 0.010718f },
		dls::math::vec2f{ 0.052638f, 0.004825f },
		dls::math::vec2f{ 0.066108f, 0.001215f },
		dls::math::vec2f{ 0.08f, 0.0f }
	};

	for (auto &p : roundRect) {
		p.x = p.x * static_cast<float>(taille.x) + static_cast<float>(pos.x);
		p.y = p.y * static_cast<float>(taille.y) + static_cast<float>(pos.y);
	}

	auto parametres_tampon = ParametresTampon();
	parametres_tampon.attribut = "vertex";
	parametres_tampon.dimension_attribut = 2;
	parametres_tampon.pointeur_sommets = roundRect;
	parametres_tampon.taille_octet_sommets = sizeof(dls::math::vec2f) * 41;
	parametres_tampon.elements = 41;

	auto params_dessin = ParametresDessin{};
	params_dessin.type_dessin(GL_TRIANGLE_FAN);

	tampon->parametres_dessin(params_dessin);
#endif

	tampon->remplie_tampon(parametres_tampon);

	return tampon;
}

void EditriceNulle::souris_bougee(Evenement const &)
{}

void EditriceNulle::souris_pressee(Evenement const &)
{}

void EditriceNulle::souris_relachee(Evenement const &)
{}

void EditriceNulle::cle_pressee(Evenement const &)
{}

void EditriceNulle::cle_relachee(Evenement const &)
{}

void EditriceNulle::cle_repetee(Evenement const &)
{}

void EditriceNulle::roulette(Evenement const &)
{}

void EditriceNulle::double_clic(Evenement const &)
{}

void EditriceNulle::dessine()
{
	auto contexte = ContexteRendu{};
	auto tampon = cree_tampon_image(
				this->pos,
				this->taille_norm,
				dls::math::vec4f(1.0f, 0.0f, 0.5f, 1.0f));

	tampon->dessine(contexte);

	delete tampon;

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	dessine_texte("Mémoire allouée   :  7 Mo", this->pos, this->taille);
	dessine_texte("Mémoire consommée : 10 Mo", this->pos + dls::math::vec2d(0.0, 16.0), this->taille);

	dessine_bouton("13000", this->pos + dls::math::vec2d(0.0, 32.0), this->taille);
}
