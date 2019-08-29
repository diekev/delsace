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

#include "operatrices_3d.h"

#include "biblinternes/graphe/noeud.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/texture/texture.h"
#include "biblinternes/vision/camera.h"

#include "coeur/contexte_evaluation.hh"
#include "coeur/manipulatrice.h"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceTexture final : public OperatriceImage {
	vision::Camera3D *m_camera = nullptr;
	TextureImage m_texture{};

public:
	static constexpr auto NOM = "Texture";
	static constexpr auto AIDE = "Crée une texture.";

	OperatriceTexture(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	OperatriceTexture(OperatriceTexture const &) = default;
	OperatriceTexture &operator=(OperatriceTexture const &) = default;

	int type() const override
	{
		return OPERATRICE_IMAGE;
	}

	type_prise type_entree(int n) const override
	{
		if (n == 0) {
			return type_prise::IMAGE;
		}

		return type_prise::INVALIDE;
	}

	const char *nom_entree(int n) override
	{
		if (n == 0) {
			return "image";
		}

		return "caméra";
	}

	type_prise type_sortie(int) const override
	{
		return type_prise::IMAGE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_texture.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		if (entree(0)->connectee() == false) {
			ajoute_avertissement("Aucune image connectée pour la texture");
			return EXECUTION_ECHOUEE;
		}

		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		auto tampon = m_image.calque_pour_lecture("image");

		if (tampon == nullptr) {
			ajoute_avertissement("Impossible de trouver un calque nommé 'image'");
			return EXECUTION_ECHOUEE;
		}

		//m_texture.charge_donnees(tampon->tampon);

		auto entrepolation = evalue_enum("entrepolation");

		if (entrepolation == "linéaire") {
			m_texture.entrepolation(ENTREPOLATION_LINEAIRE);
		}
		else if (entrepolation == "proximité") {
			m_texture.entrepolation(ENTREPOLATION_VOISINAGE_PROCHE);
		}

		auto enveloppage = evalue_enum("enveloppage");

		if (enveloppage == "répétition") {
			m_texture.enveloppage(ENVELOPPAGE_REPETITION);
		}
		else if (enveloppage == "répétition_mirroir") {
			m_texture.enveloppage(ENVELOPPAGE_REPETITION_MIRROIR);
		}
		else if (enveloppage == "restriction") {
			m_texture.enveloppage(ENVELOPPAGE_RESTRICTION);
		}

		auto projection = evalue_enum("projection");

		if (projection == "planaire") {
			m_texture.projection(PROJECTION_PLANAIRE);
		}
		else if (projection == "triplanaire") {
			m_texture.projection(PROJECTION_TRIPLANAIRE);
		}
		else if (projection == "caméra") {
			m_texture.projection(PROJECTION_CAMERA);
			//m_camera = entree(1)->requiers_camera(contexte, donnees_aval);

			if (m_camera == nullptr) {
				ajoute_avertissement("Aucune caméra trouvée pour la projection caméra !");
			}

			m_texture.camera(m_camera);
		}
		else if (projection == "cubique") {
			m_texture.projection(PROJECTION_CUBIQUE);
		}
		else if (projection == "cylindrique") {
			m_texture.projection(PROJECTION_CYLINDRIQUE);
		}
		else if (projection == "sphèrique") {
			m_texture.projection(PROJECTION_SPHERIQUE);
		}
		else if (projection == "uv") {
			m_texture.projection(PROJECTION_UV);
		}

		auto taille_texture = evalue_vecteur("taille_texture", contexte.temps_courant);

		m_texture.taille(dls::math::vec3f(taille_texture.x,
										  taille_texture.y,
										  taille_texture.z));
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_3d(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceTexture>());
}

#pragma clang diagnostic pop
