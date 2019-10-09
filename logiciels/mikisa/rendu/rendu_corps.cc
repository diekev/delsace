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

#include "rendu_corps.h"

#include <numeric>

#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/texture/texture.h"
#include "biblinternes/vision/camera.h"

#include "biblinternes/structures/tableau.hh"

#include "corps/attribut.h"
#include "corps/corps.h"
#include "corps/sphere.hh"
#include "corps/volume.hh"

#include "wolika/grille_dense.hh"
#include "wolika/grille_eparse.hh"
#include "wolika/iteration.hh"

/* ************************************************************************** */

static TamponRendu *cree_tampon_surface(bool possede_uvs, bool instances)
{
	auto tampon = memoire::loge<TamponRendu>("TamponRendu");

	auto nom_fichier = (instances) ? "nuanceurs/diffus_instances.vert" : "nuanceurs/diffus.vert";

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::contenu_fichier(nom_fichier));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/diffus.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("normaux");
	parametre_programme.ajoute_attribut("uvs");
	parametre_programme.ajoute_attribut("couleurs");

	if (instances) {
		parametre_programme.ajoute_attribut("matrices_instances");
	}

	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("MV");
	parametre_programme.ajoute_uniforme("P");
	parametre_programme.ajoute_uniforme("N");
	parametre_programme.ajoute_uniforme("image");
	parametre_programme.ajoute_uniforme("direction_camera");
	parametre_programme.ajoute_uniforme("methode");
	parametre_programme.ajoute_uniforme("taille_texture");
	parametre_programme.ajoute_uniforme("possede_uvs");

	tampon->parametres_programme(parametre_programme);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("possede_uvs", possede_uvs);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

void ajoute_polygone_surface(
		Polygone *polygone,
		AccesseusePointLecture const &liste_points,
		Attribut const *attr_normaux,
		Attribut const *attr_couleurs,
		dls::tableau<dls::math::vec3f> &points,
		dls::tableau<dls::math::vec3f> &normaux,
		dls::tableau<dls::math::vec3f> &couleurs)
{
	for (long i = 2; i < polygone->nombre_sommets(); ++i) {
		points.pousse(liste_points.point_local(polygone->index_point(0)));
		points.pousse(liste_points.point_local(polygone->index_point(i - 1)));
		points.pousse(liste_points.point_local(polygone->index_point(i)));

		if (attr_normaux) {
			auto idx = normaux.taille();
			normaux.redimensionne(idx + 3);

			if (attr_normaux->portee == portee_attr::POINT) {
				extrait(attr_normaux->r32(polygone->index_point(0)), normaux[idx]);
				extrait(attr_normaux->r32(polygone->index_point(i - 1)), normaux[idx + 1]);
				extrait(attr_normaux->r32(polygone->index_point(i)), normaux[idx + 2]);
			}
			else if (attr_normaux->portee == portee_attr::PRIMITIVE) {
				extrait(attr_normaux->r32(polygone->index), normaux[idx]);
				extrait(attr_normaux->r32(polygone->index), normaux[idx + 1]);
				extrait(attr_normaux->r32(polygone->index), normaux[idx + 2]);
			}
			else if (attr_normaux->portee == portee_attr::CORPS) {
				extrait(attr_normaux->r32(0), normaux[idx]);
				extrait(attr_normaux->r32(0), normaux[idx + 1]);
				extrait(attr_normaux->r32(0), normaux[idx + 2]);
			}
		}

		if (attr_couleurs) {
			auto idx = couleurs.taille();
			couleurs.redimensionne(idx + 3);

			if (attr_couleurs->portee == portee_attr::POINT) {
				extrait(attr_couleurs->r32(polygone->index_point(0)), couleurs[idx]);
				extrait(attr_couleurs->r32(polygone->index_point(i - 1)), couleurs[idx + 1]);
				extrait(attr_couleurs->r32(polygone->index_point(i)), couleurs[idx + 2]);
			}
			else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
				extrait(attr_couleurs->r32(polygone->index), couleurs[idx]);
				extrait(attr_couleurs->r32(polygone->index), couleurs[idx + 1]);
				extrait(attr_couleurs->r32(polygone->index), couleurs[idx + 2]);
			}
			else if (attr_couleurs->portee == portee_attr::VERTEX) {
				extrait(attr_couleurs->r32(polygone->index_sommet(0)), couleurs[idx]);
				extrait(attr_couleurs->r32(polygone->index_sommet(i - 1)), couleurs[idx + 1]);
				extrait(attr_couleurs->r32(polygone->index_sommet(i)), couleurs[idx + 2]);
			}
			else if (attr_couleurs->portee == portee_attr::CORPS) {
				extrait(attr_couleurs->r32(0), couleurs[idx]);
				extrait(attr_couleurs->r32(0), couleurs[idx + 1]);
				extrait(attr_couleurs->r32(0), couleurs[idx + 2]);
			}
		}
		else {
			couleurs.pousse(dls::math::vec3f(0.8f));
			couleurs.pousse(dls::math::vec3f(0.8f));
			couleurs.pousse(dls::math::vec3f(0.8f));
		}
	}
}

void ajoute_polygone_segment(
		Polygone *polygone,
		AccesseusePointLecture const &liste_points,
		Attribut const *attr_couleurs,
		dls::tableau<dls::math::vec3f> &points,
		dls::tableau<dls::math::vec3f> &couleurs)
{
	for (long i = 0; i < polygone->nombre_segments(); ++i) {
		points.pousse(liste_points.point_local(polygone->index_point(i)));
		points.pousse(liste_points.point_local(polygone->index_point(i + 1)));

		if (attr_couleurs) {
			auto idx = couleurs.taille();
			couleurs.redimensionne(idx + 2);

			if (attr_couleurs->portee == portee_attr::POINT) {
				extrait(attr_couleurs->r32(polygone->index_point(0)), couleurs[idx]);
				extrait(attr_couleurs->r32(polygone->index_point(i + 1)), couleurs[idx + 1]);
			}
			else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
				extrait(attr_couleurs->r32(polygone->index), couleurs[idx]);
				extrait(attr_couleurs->r32(polygone->index), couleurs[idx + 1]);
			}
			else if (attr_couleurs->portee == portee_attr::CORPS) {
				extrait(attr_couleurs->r32(0), couleurs[idx]);
				extrait(attr_couleurs->r32(0), couleurs[idx + 1]);
			}
		}
		else {
			couleurs.pousse(dls::math::vec3f(0.8f));
			couleurs.pousse(dls::math::vec3f(0.8f));
		}
	}
}

static dls::math::vec3f points_cercle_XZ[32] = {
	dls::math::vec3f( 0.000000f, 0.000000f,  1.000000f),
	dls::math::vec3f(-0.195090f, 0.000000f,  0.980785f),
	dls::math::vec3f(-0.382683f, 0.000000f,  0.92388f),
	dls::math::vec3f(-0.555570f, 0.000000f,  0.83147f),
	dls::math::vec3f(-0.707107f, 0.000000f,  0.707107f),
	dls::math::vec3f(-0.831470f, 0.000000f,  0.55557f),
	dls::math::vec3f(-0.923880f, 0.000000f,  0.382683f),
	dls::math::vec3f(-0.980785f, 0.000000f,  0.19509f),
	dls::math::vec3f(-1.000000f, 0.000000f,  0.000000f),
	dls::math::vec3f(-0.980785f, 0.000000f, -0.19509f),
	dls::math::vec3f(-0.923880f, 0.000000f, -0.382683f),
	dls::math::vec3f(-0.831470f, 0.000000f, -0.55557f),
	dls::math::vec3f(-0.707107f, 0.000000f, -0.707107f),
	dls::math::vec3f(-0.555570f, 0.000000f, -0.83147f),
	dls::math::vec3f(-0.382683f, 0.000000f, -0.92388f),
	dls::math::vec3f(-0.195090f, 0.000000f, -0.980785f),
	dls::math::vec3f( 0.000000f, 0.000000f, -1.000000f),
	dls::math::vec3f( 0.195091f, 0.000000f, -0.980785f),
	dls::math::vec3f( 0.382684f, 0.000000f, -0.923879f),
	dls::math::vec3f( 0.555571f, 0.000000f, -0.831469f),
	dls::math::vec3f( 0.707107f, 0.000000f, -0.707106f),
	dls::math::vec3f( 0.831470f, 0.000000f, -0.55557f),
	dls::math::vec3f( 0.923880f, 0.000000f, -0.382683f),
	dls::math::vec3f( 0.980785f, 0.000000f, -0.195089f),
	dls::math::vec3f( 1.000000f, 0.000000f,  0.000000f),
	dls::math::vec3f( 0.980785f, 0.000000f,  0.195091f),
	dls::math::vec3f( 0.923879f, 0.000000f,  0.382684f),
	dls::math::vec3f( 0.831469f, 0.000000f,  0.555571f),
	dls::math::vec3f( 0.707106f, 0.000000f,  0.707108f),
	dls::math::vec3f( 0.555569f, 0.000000f,  0.831470f),
	dls::math::vec3f( 0.382682f, 0.000000f,  0.923880f),
	dls::math::vec3f( 0.195089f, 0.000000f,  0.980786f),
};
static dls::math::vec3f points_cercle_XY[32] = {
	dls::math::vec3f( 0.000000f,  1.000000f, 0.000000f),
	dls::math::vec3f(-0.195090f,  0.980785f, 0.000000f),
	dls::math::vec3f(-0.382683f,  0.923880f, 0.000000f),
	dls::math::vec3f(-0.555570f,  0.831470f, 0.000000f),
	dls::math::vec3f(-0.707107f,  0.707107f, 0.000000f),
	dls::math::vec3f(-0.831470f,  0.555570f, 0.000000f),
	dls::math::vec3f(-0.923880f,  0.382683f, 0.000000f),
	dls::math::vec3f(-0.980785f,  0.195090f, 0.000000f),
	dls::math::vec3f(-1.000000f,  0.000000f, 0.000000f),
	dls::math::vec3f(-0.980785f, -0.195090f, 0.000000f),
	dls::math::vec3f(-0.923880f, -0.382683f, 0.000000f),
	dls::math::vec3f(-0.831470f, -0.555570f, 0.000000f),
	dls::math::vec3f(-0.707107f, -0.707107f, 0.000000f),
	dls::math::vec3f(-0.555570f, -0.831470f, 0.000000f),
	dls::math::vec3f(-0.382683f, -0.923880f, 0.000000f),
	dls::math::vec3f(-0.195090f, -0.980785f, 0.000000f),
	dls::math::vec3f( 0.000000f, -1.000000f, 0.000000f),
	dls::math::vec3f( 0.195091f, -0.980785f, 0.000000f),
	dls::math::vec3f( 0.382684f, -0.923879f, 0.000000f),
	dls::math::vec3f( 0.555571f, -0.831469f, 0.000000f),
	dls::math::vec3f( 0.707107f, -0.707106f, 0.000000f),
	dls::math::vec3f( 0.831470f, -0.555570f, 0.000000f),
	dls::math::vec3f( 0.923880f, -0.382683f, 0.000000f),
	dls::math::vec3f( 0.980785f, -0.195089f, 0.000000f),
	dls::math::vec3f( 1.000000f,  0.000000f, 0.000000f),
	dls::math::vec3f( 0.980785f,  0.195091f, 0.000000f),
	dls::math::vec3f( 0.923879f,  0.382684f, 0.000000f),
	dls::math::vec3f( 0.831469f,  0.555571f, 0.000000f),
	dls::math::vec3f( 0.707106f,  0.707108f, 0.000000f),
	dls::math::vec3f( 0.555569f,  0.831470f, 0.000000f),
	dls::math::vec3f( 0.382682f,  0.923880f, 0.000000f),
	dls::math::vec3f( 0.195089f,  0.980786f, 0.000000f),
};
static dls::math::vec3f points_cercle_YZ[32] = {
	dls::math::vec3f(0.000000f,  0.000000f,  1.000000f),
	dls::math::vec3f(0.000000f, -0.195090f,  0.980785f),
	dls::math::vec3f(0.000000f, -0.382683f,  0.923880f),
	dls::math::vec3f(0.000000f, -0.555570f,  0.831470f),
	dls::math::vec3f(0.000000f, -0.707107f,  0.707107f),
	dls::math::vec3f(0.000000f, -0.831470f,  0.555570f),
	dls::math::vec3f(0.000000f, -0.923880f,  0.382683f),
	dls::math::vec3f(0.000000f, -0.980785f,  0.195090f),
	dls::math::vec3f(0.000000f, -1.000000f,  0.000000f),
	dls::math::vec3f(0.000000f, -0.980785f, -0.195090f),
	dls::math::vec3f(0.000000f, -0.923880f, -0.382683f),
	dls::math::vec3f(0.000000f, -0.831470f, -0.555570f),
	dls::math::vec3f(0.000000f, -0.707107f, -0.707107f),
	dls::math::vec3f(0.000000f, -0.555570f, -0.831470f),
	dls::math::vec3f(0.000000f, -0.382683f, -0.923880f),
	dls::math::vec3f(0.000000f, -0.195090f, -0.980785f),
	dls::math::vec3f(0.000000f,  0.000000f, -1.000000f),
	dls::math::vec3f(0.000000f,  0.195091f, -0.980785f),
	dls::math::vec3f(0.000000f,  0.382684f, -0.923879f),
	dls::math::vec3f(0.000000f,  0.555571f, -0.831469f),
	dls::math::vec3f(0.000000f,  0.707107f, -0.707106f),
	dls::math::vec3f(0.000000f,  0.831470f, -0.555570f),
	dls::math::vec3f(0.000000f,  0.923880f, -0.382683f),
	dls::math::vec3f(0.000000f,  0.980785f, -0.195089f),
	dls::math::vec3f(0.000000f,  1.000000f,  0.000000f),
	dls::math::vec3f(0.000000f,  0.980785f,  0.195091f),
	dls::math::vec3f(0.000000f,  0.923879f,  0.382684f),
	dls::math::vec3f(0.000000f,  0.831469f,  0.555571f),
	dls::math::vec3f(0.000000f,  0.707106f,  0.707108f),
	dls::math::vec3f(0.000000f,  0.555569f,  0.831470f),
	dls::math::vec3f(0.000000f,  0.382682f,  0.923880f),
	dls::math::vec3f(0.000000f,  0.195089f,  0.980786f),
};

static void ajoute_primitive_sphere(
		Sphere *sphere,
		AccesseusePointLecture const &liste_points,
		Attribut const *attr_couleurs,
		dls::tableau<dls::math::vec3f> &points,
		dls::tableau<dls::math::vec3f> &couleurs)
{
	auto pos_sphere = liste_points.point_local(sphere->idx_point);

	for (auto i = 0; i < 32; ++i) {
		points.pousse(points_cercle_XZ[i] * sphere->rayon + pos_sphere);
		points.pousse(points_cercle_XZ[(i + 1) % 32] * sphere->rayon + pos_sphere);
	}

	for (auto i = 0; i < 32; ++i) {
		points.pousse(points_cercle_XY[i] * sphere->rayon + pos_sphere);
		points.pousse(points_cercle_XY[(i + 1) % 32] * sphere->rayon + pos_sphere);
	}

	for (auto i = 0; i < 32; ++i) {
		points.pousse(points_cercle_YZ[i] * sphere->rayon + pos_sphere);
		points.pousse(points_cercle_YZ[(i + 1) % 32] * sphere->rayon + pos_sphere);
	}

	auto couleur = dls::math::vec3f(0.8f);

	if (attr_couleurs) {
		if (attr_couleurs->portee == portee_attr::POINT) {
			extrait(attr_couleurs->r32(sphere->idx_point), couleur);
		}
		else if (attr_couleurs->portee == portee_attr::PRIMITIVE) {
			extrait(attr_couleurs->r32(sphere->index), couleur);
		}
		else if (attr_couleurs->portee == portee_attr::CORPS) {
			extrait(attr_couleurs->r32(0), couleur);
		}
	}

	auto idx = couleurs.taille();
	couleurs.redimensionne(idx + 32 * 3 * 2);

	for (auto i = 0; i < 32 * 3 * 2; ++i) {
		couleurs[idx + i] = couleur;
	}
}

static TamponRendu *cree_tampon_segments(bool instances)
{
	auto tampon = memoire::loge<TamponRendu>("TamponRendu");

	auto nom_fichier = (instances) ? "nuanceurs/simple_instances.vert" : "nuanceurs/simple.vert";

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::contenu_fichier(nom_fichier));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/simple.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_attribut("couleur_sommet");

	if (instances) {
		parametre_programme.ajoute_attribut("matrices_instances");
	}

	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("couleur");
	parametre_programme.ajoute_uniforme("possede_couleur_sommet");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("couleur", 0.0f, 0.0f, 0.0f, 1.0f);
	programme->desactive();

	return tampon;
}

/* ************************************************************************** */

static auto slice(
		dls::math::vec3f const &view_dir,
		size_t m_axis,
		TamponRendu *m_renderbuffer,
		dls::math::vec3f const &m_min,
		dls::math::vec3f const &m_max)
{
	auto axis = axe_dominant_abs(view_dir);

	if (m_axis == axis) {
		return;
	}

	auto m_dimensions = m_max - m_min;
	auto m_num_slices = 256l;
	auto m_elements = m_num_slices * 6;

	m_axis = axis;
	auto depth = m_min[m_axis];
	auto slice_size = m_dimensions[m_axis] / static_cast<float>(m_num_slices);

	/* always process slices in back to front order! */
	if (view_dir[m_axis] > 0.0f) {
		depth = m_max[m_axis];
		slice_size = -slice_size;
	}

	const dls::math::vec3f vertices[3][4] = {
		{
			dls::math::vec3f(0.0f, m_min[1], m_min[2]),
			dls::math::vec3f(0.0f, m_max[1], m_min[2]),
			dls::math::vec3f(0.0f, m_max[1], m_max[2]),
			dls::math::vec3f(0.0f, m_min[1], m_max[2])
		},
		{
			dls::math::vec3f(m_min[0], 0.0f, m_min[2]),
			dls::math::vec3f(m_min[0], 0.0f, m_max[2]),
			dls::math::vec3f(m_max[0], 0.0f, m_max[2]),
			dls::math::vec3f(m_max[0], 0.0f, m_min[2])
		},
		{
			dls::math::vec3f(m_min[0], m_min[1], 0.0f),
			dls::math::vec3f(m_min[0], m_max[1], 0.0f),
			dls::math::vec3f(m_max[0], m_max[1], 0.0f),
			dls::math::vec3f(m_max[0], m_min[1], 0.0f)
		}
	};


	dls::tableau<GLuint> indices(m_elements);
	unsigned idx = 0;
	auto idx_count = 0;

	dls::tableau<dls::math::vec3f> points;
	points.reserve(m_num_slices * 4);

	for (auto slice(0l); slice < m_num_slices; slice++) {
		dls::math::vec3f v0 = vertices[m_axis][0];
		dls::math::vec3f v1 = vertices[m_axis][1];
		dls::math::vec3f v2 = vertices[m_axis][2];
		dls::math::vec3f v3 = vertices[m_axis][3];

		v0[m_axis] = depth;
		v1[m_axis] = depth;
		v2[m_axis] = depth;
		v3[m_axis] = depth;

		points.pousse(v0); //  * glm::mat3(m_inv_matrix)
		points.pousse(v1);
		points.pousse(v2);
		points.pousse(v3);

		indices[idx_count++] = idx + 0;
		indices[idx_count++] = idx + 1;
		indices[idx_count++] = idx + 2;
		indices[idx_count++] = idx + 0;
		indices[idx_count++] = idx + 2;
		indices[idx_count++] = idx + 3;

		depth += slice_size;
		idx += 4;
	}

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = points.donnees();
	parametres_tampon.taille_octet_sommets = static_cast<size_t>(points.taille()) * sizeof(dls::math::vec3f);
	parametres_tampon.elements = static_cast<size_t>(idx_count);
	parametres_tampon.pointeur_index = indices.donnees();
	parametres_tampon.taille_octet_index = static_cast<size_t>(idx_count) * sizeof(GLuint);

	m_renderbuffer->remplie_tampon(parametres_tampon);
}

static auto cree_tampon_volume(Volume *volume, dls::math::vec3f const &view_dir)
{
	auto tampon = memoire::loge<TamponRendu>("TamponRendu");

	tampon->charge_source_programme(
				dls::ego::Nuanceur::VERTEX,
				dls::contenu_fichier("nuanceurs/volume.vert"));

	tampon->charge_source_programme(
				dls::ego::Nuanceur::FRAGMENT,
				dls::contenu_fichier("nuanceurs/volume.frag"));

	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("vertex");
	parametre_programme.ajoute_uniforme("sommets");
	parametre_programme.ajoute_uniforme("offset");
	parametre_programme.ajoute_uniforme("dimension");
	parametre_programme.ajoute_uniforme("volume");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");

	tampon->parametres_programme(parametre_programme);

	ParametresDessin parametres_dessin;
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	tampon->ajoute_texture_3d();
	auto texture = tampon->texture_3d();

	auto etendue = limites3f{};
	auto resolution = dls::math::vec3i{};
	auto voxels = dls::tableau<float>{};
	auto ptr_donnees = static_cast<const void *>(nullptr);

	auto grille = volume->grille;

	if (!grille->est_eparse()) {
		etendue = grille->desc().etendue;
		resolution = grille->desc().resolution;

		if (grille->desc().type_donnees == wlk::type_grille::R32) {
			auto grille_scalaire = dynamic_cast<wlk::grille_dense_3d<float> *>(grille);
			ptr_donnees = grille_scalaire->donnees();
		}
	}
	else {
		auto grille_eprs = dynamic_cast<wlk::grille_eparse<float> *>(grille);
		auto min_grille = dls::math::vec3i{100000};
		auto max_grille = dls::math::vec3i{-100000};

		if (grille_eprs->nombre_tuile() == 0) {
			/* la grille est vide */
			min_grille = dls::math::vec3i{0};
			max_grille = dls::math::vec3i{0};
		}
		else {
			wlk::pour_chaque_tuile(*grille_eprs, [&](wlk::tuile_scalaire<float> *tuile)
			{
				extrait_min_max(tuile->min, min_grille, max_grille);
				extrait_min_max(tuile->max, min_grille, max_grille);
			});
		}

		resolution = max_grille - min_grille;
		etendue.min = grille->index_vers_monde(min_grille);
		etendue.max = grille->index_vers_monde(max_grille);

		auto nombre_voxels = resolution.x * resolution.y * resolution.z;
		voxels.redimensionne(nombre_voxels);

		auto index_voxel = 0l;
#if 0
		for (auto z = 0; z < resolution.z; ++z) {
			for (auto y = 0; y < resolution.y; ++y) {
				for (auto x = 0; x < resolution.x; ++x, ++index_voxel) {
					voxels[index_voxel] = grille->valeur(x, y, z);
				}
			}
		}
#else
		wlk::pour_chaque_tuile_parallele(*grille_eprs, [&](wlk::tuile_scalaire<float> *tuile)
		{
			auto index_tuile = 0;

			for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
				for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
					for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
						/* décale par rapport à min_grille au cas où le minimum
						 * n'est pas zéro */
						auto pos_tuile = tuile->min - min_grille;
						pos_tuile.x += i;
						pos_tuile.y += j;
						pos_tuile.z += k;

						index_voxel = pos_tuile.x + (pos_tuile.y + pos_tuile.z * resolution.y) * resolution.x;

						voxels[index_voxel] = tuile->donnees[index_tuile];
					}
				}
			}
		});
#endif
		ptr_donnees = voxels.donnees();
	}

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("volume", texture->code_attache());
	programme->uniforme("offset", etendue.min.x, etendue.min.y, etendue.min.z);
	programme->uniforme("dimension", etendue.taille().x, etendue.taille().y, etendue.taille().z);
	programme->desactive();

	/* crée vertices */
	slice(view_dir, -1ul, tampon, etendue.min, etendue.max);

	/* crée texture 3d */

	texture->attache();
	texture->type(GL_FLOAT, GL_RED, GL_RED);
	texture->filtre_min_mag(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP_TO_BORDER);

	int res[3] = {
		resolution[0],
		resolution[1],
		resolution[2]
	};

	texture->remplie(ptr_donnees, res);
	texture->genere_mip_map(0, 4);
	texture->detache();

	return tampon;
}

/* ************************************************************************** */

RenduCorps::RenduCorps(Corps const *corps)
	: m_corps(corps)
{}

RenduCorps::~RenduCorps()
{
	memoire::deloge("TamponRendu", m_tampon_points);
	memoire::deloge("TamponRendu", m_tampon_polygones);
	memoire::deloge("TamponRendu", m_tampon_segments);
	memoire::deloge("TamponRendu", m_tampon_volume);
}

void RenduCorps::initialise(
		ContexteRendu const &contexte,
		StatistiquesRendu &stats,
		dls::tableau<dls::math::mat4x4f> &matrices)
{
	stats.nombre_objets += 1;
	auto est_instance = matrices.taille() != 0;

	auto liste_points = m_corps->points_pour_lecture();
	auto liste_prims = m_corps->prims();

	if (liste_points.taille() == 0l && liste_prims->taille() == 0l) {
		return;
	}

	dls::tableau<char> point_utilise(liste_points.taille(), 0);

	if (liste_prims->taille() != 0l) {
		auto attr_N = m_corps->attribut("N");
		auto attr_C = m_corps->attribut("C");
		dls::tableau<dls::math::vec3f> points_polys;
		dls::tableau<dls::math::vec3f> points_segment;
		dls::tableau<dls::math::vec3f> normaux;
		dls::tableau<dls::math::vec3f> couleurs_polys;
		dls::tableau<dls::math::vec3f> couleurs_segment;

		for (auto ip = 0; ip < liste_prims->taille(); ++ip) {
			auto prim = liste_prims->prim(ip);
			if (prim->type_prim() == type_primitive::POLYGONE) {
				auto polygone = dynamic_cast<Polygone *>(prim);
				if (polygone->type == type_polygone::FERME) {
					stats.nombre_polygones += 1;
					ajoute_polygone_surface(polygone, liste_points, attr_N, attr_C, points_polys, normaux, couleurs_polys);
				}
				else if (polygone->type == type_polygone::OUVERT) {
					stats.nombre_polylignes += 1;
					ajoute_polygone_segment(polygone, liste_points, attr_C, points_segment, couleurs_segment);
				}

				for (auto i = 0; i < polygone->nombre_sommets(); ++i) {
					point_utilise[polygone->index_point(i)] = 1;
				}
			}
			else if (prim->type_prim() == type_primitive::SPHERE) {
				auto sphere = dynamic_cast<Sphere *>(prim);
				ajoute_primitive_sphere(sphere, liste_points, attr_C, points_segment, couleurs_segment);
			}
			else if (prim->type_prim() == type_primitive::VOLUME) {
				if (m_tampon_volume == nullptr) {
					stats.nombre_volumes += 1;
					m_tampon_volume = cree_tampon_volume(dynamic_cast<Volume *>(prim), contexte.vue());
				}
			}
		}

		if (points_polys.taille() != 0) {
			m_tampon_polygones = cree_tampon_surface(false, est_instance);

			ParametresTampon parametres_tampon;
			parametres_tampon.attribut = "sommets";
			parametres_tampon.dimension_attribut = 3;
			parametres_tampon.pointeur_sommets = points_polys.donnees();
			parametres_tampon.taille_octet_sommets = static_cast<size_t>(points_polys.taille()) * sizeof(dls::math::vec3f);
			parametres_tampon.elements = static_cast<size_t>(points_polys.taille());

			m_tampon_polygones->remplie_tampon(parametres_tampon);

			if (normaux.taille() != 0) {
				parametres_tampon.attribut = "normaux";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = normaux.donnees();
				parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(normaux.taille()) * sizeof(dls::math::vec3f);
				parametres_tampon.elements = static_cast<size_t>(normaux.taille());

				m_tampon_polygones->remplie_tampon_extra(parametres_tampon);
			}

			if (couleurs_polys.taille() != 0) {
				parametres_tampon.attribut = "couleurs";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = couleurs_polys.donnees();
				parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(couleurs_polys.taille()) * sizeof(dls::math::vec3f);
				parametres_tampon.elements = static_cast<size_t>(couleurs_polys.taille());

				m_tampon_polygones->remplie_tampon_extra(parametres_tampon);
			}
		}

		if (points_segment.taille() != 0) {
			m_tampon_segments = cree_tampon_segments(est_instance);

			ParametresTampon parametres_tampon;
			parametres_tampon.attribut = "sommets";
			parametres_tampon.dimension_attribut = 3;
			parametres_tampon.pointeur_sommets = points_segment.donnees();
			parametres_tampon.taille_octet_sommets = static_cast<size_t>(points_segment.taille()) * sizeof(dls::math::vec3f);
			parametres_tampon.elements = static_cast<size_t>(points_segment.taille());

			m_tampon_segments->remplie_tampon(parametres_tampon);

			if (couleurs_segment.taille() != 0) {
				parametres_tampon.attribut = "couleur_sommet";
				parametres_tampon.dimension_attribut = 3;
				parametres_tampon.pointeur_donnees_extra = couleurs_segment.donnees();
				parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(couleurs_segment.taille()) * sizeof(dls::math::vec3f);
				parametres_tampon.elements = static_cast<size_t>(couleurs_segment.taille());

				m_tampon_segments->remplie_tampon_extra(parametres_tampon);

				auto programme = m_tampon_segments->programme();
				programme->active();
				programme->uniforme("possede_couleur_sommet", 1);
				programme->desactive();
			}
		}
	}

//	if (m_tampon_segments || m_tampon_polygones) {
//		return;
//	}

	dls::tableau<dls::math::vec3f> points;
	dls::tableau<dls::math::vec3f> couleurs;
	points.reserve(liste_points.taille());
	couleurs.reserve(liste_points.taille());

	auto attr_C = m_corps->attribut("C");

	stats.nombre_points += liste_points.taille();

	for (auto i = 0; i < liste_points.taille(); ++i) {
		if (point_utilise[i]) {
			continue;
		}

		points.pousse(liste_points.point_local(i));

		if ((attr_C != nullptr) && (attr_C->portee == portee_attr::POINT)) {
			auto idx = couleurs.taille();
			couleurs.redimensionne(idx + 1);
			extrait(attr_C->r32(i), couleurs[idx]);
		}
	}

	auto parametres_tampon_instance = ParametresTampon{};

	if (est_instance) {
		parametres_tampon_instance.attribut = "matrices_instances";
		parametres_tampon_instance.dimension_attribut = 4;
		parametres_tampon_instance.pointeur_donnees_extra = matrices.donnees();
		parametres_tampon_instance.taille_octet_donnees_extra = static_cast<size_t>(matrices.taille()) * sizeof(dls::math::mat4x4f);
		parametres_tampon_instance.nombre_instances = static_cast<size_t>(matrices.taille());

		if (m_tampon_segments) {
			m_tampon_segments->remplie_tampon_matrices_instance(parametres_tampon_instance);
		}

		if (m_tampon_polygones) {
			m_tampon_polygones->remplie_tampon_matrices_instance(parametres_tampon_instance);
		}
	}

	if (points.est_vide()) {
		return;
	}

	m_tampon_points = cree_tampon_segments(est_instance);

	ParametresTampon parametres_tampon;
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 3;
	parametres_tampon.pointeur_sommets = points.donnees();
	parametres_tampon.taille_octet_sommets = static_cast<size_t>(points.taille()) * sizeof(dls::math::vec3f);
	parametres_tampon.elements = static_cast<size_t>(points.taille());

	ParametresDessin parametres_dessin;
	parametres_dessin.taille_point(2.0);
	parametres_dessin.type_dessin(GL_POINTS);
	m_tampon_points->parametres_dessin(parametres_dessin);

	m_tampon_points->remplie_tampon(parametres_tampon);

	if (couleurs.taille() != 0l) {
		parametres_tampon.attribut = "couleur_sommet";
		parametres_tampon.dimension_attribut = 3;
		parametres_tampon.pointeur_donnees_extra = couleurs.donnees();
		parametres_tampon.taille_octet_donnees_extra = static_cast<size_t>(couleurs.taille()) * sizeof(dls::math::vec3f);
		parametres_tampon.elements = static_cast<size_t>(couleurs.taille());

		m_tampon_points->remplie_tampon_extra(parametres_tampon);
		auto programme = m_tampon_points->programme();
		programme->active();
		programme->uniforme("possede_couleur_sommet", 1);
		programme->desactive();
	}

	if (est_instance) {
		m_tampon_points->remplie_tampon_matrices_instance(parametres_tampon_instance);
	}
}

void RenduCorps::dessine(ContexteRendu const &contexte)
{
	if (m_tampon_points != nullptr) {
		m_tampon_points->dessine(contexte);
	}

	if (m_tampon_polygones != nullptr) {
		auto programme = m_tampon_polygones->programme();
		programme->active();
		programme->uniforme("methode", -1);
		programme->desactive();

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

	if (m_tampon_volume != nullptr) {
		ParametresDessin parametres_dessin;
		parametres_dessin.taille_point(2.0);
		parametres_dessin.type_dessin(GL_TRIANGLES);
		m_tampon_volume->parametres_dessin(parametres_dessin);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		m_tampon_volume->dessine(contexte);
		glDisable(GL_BLEND);
	}
}
