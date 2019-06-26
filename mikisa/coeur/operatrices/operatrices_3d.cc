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

#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "../contexte_evaluation.hh"
#include "../manipulatrice.h"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCamera final : public OperatriceImage {
	vision::Camera3D m_camera;
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

public:
	static constexpr auto NOM = "Caméra";
	static constexpr auto AIDE = "Crée une caméra.";

	explicit OperatriceCamera(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
		, m_camera(0, 0)
	{
		entrees(0);
		sorties(1);
	}

	int type() const override
	{
		return OPERATRICE_CAMERA;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CAMERA;
	}

	vision::Camera3D *camera() override
	{
		return &m_camera;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_camera.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	bool possede_manipulatrice_3d(int type) const override
	{
		return type == MANIPULATION_POSITION || type == MANIPULATION_ROTATION;
	}

	Manipulatrice3D *manipulatrice_3d(int type) override
	{
		if (type == MANIPULATION_POSITION) {
			return &m_manipulatrice_position;
		}

		if (type == MANIPULATION_ROTATION) {
			return &m_manipulatrice_rotation;
		}

		return nullptr;
	}

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override
	{
		dls::math::vec3f position;

		if (type == MANIPULATION_POSITION) {
			position = dls::math::vec3f(m_manipulatrice_position.pos());
			m_camera.position(position);
			m_camera.ajourne_pour_operatrice();

			valeur_vecteur("position", position);
		}
		else if (type == MANIPULATION_ROTATION) {
			auto rotation = dls::math::vec3f(m_manipulatrice_rotation.rotation());
			m_camera.rotation(rotation);
			m_camera.ajourne_pour_operatrice();

			position = evalue_vecteur("position", temps);

			valeur_vecteur("rotation", rotation * constantes<float>::POIDS_RAD_DEG);
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);

		auto const largeur = evalue_entier("largeur");
		auto const hauteur = evalue_entier("hauteur");
		auto const longueur_focale = evalue_decimal("longueur_focale");
		auto const largeur_senseur = evalue_decimal("largeur_senseur");
		auto const proche = evalue_decimal("proche");
		auto const eloigne = evalue_decimal("éloigné");
		auto const projection = evalue_enum("projection");
		auto const position = evalue_vecteur("position", contexte.temps_courant);
		auto const rotation = evalue_vecteur("rotation", contexte.temps_courant);

		if (projection == "perspective") {
			m_camera.projection(vision::TypeProjection::PERSPECTIVE);
		}
		else if (projection == "orthographique") {
			m_camera.projection(vision::TypeProjection::ORTHOGRAPHIQUE);
		}

		m_camera.redimensionne(largeur, hauteur);
		m_camera.longueur_focale(longueur_focale);
		m_camera.largeur_senseur(largeur_senseur);
		m_camera.profondeur(proche, eloigne);
		m_camera.position(position);
		m_camera.rotation(rotation * constantes<float>::POIDS_DEG_RAD);
		m_camera.ajourne_pour_operatrice();

		m_manipulatrice_position.pos(dls::math::point3f(position));
		m_manipulatrice_rotation.pos(dls::math::point3f(position));

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceTexture final : public OperatriceImage {
	vision::Camera3D *m_camera = nullptr;
	TextureImage m_texture{};

public:
	static constexpr auto NOM = "Texture";
	static constexpr auto AIDE = "Crée une texture.";

	explicit OperatriceTexture(Graphe &graphe_parent, Noeud *noeud)
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

	int type_entree(int n) const override
	{
		if (n == 0) {
			return OPERATRICE_IMAGE;
		}

		return OPERATRICE_CAMERA;
	}

	const char *nom_entree(int n) override
	{
		if (n == 0) {
			return "image";
		}

		return "caméra";
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_IMAGE;
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

	TextureImage *texture() override
	{
		return &m_texture;
	}

	vision::Camera3D *camera() override
	{
		return m_camera;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		if (entree(0)->connectee() == false) {
			ajoute_avertissement("Aucune image connectée pour la texture");
			return EXECUTION_ECHOUEE;
		}

		entree(0)->requiers_image(m_image, contexte, donnees_aval);
		auto tampon = m_image.calque("image");

		if (tampon == nullptr) {
			ajoute_avertissement("Impossible de trouver un calque nommé 'image'");
			return EXECUTION_ECHOUEE;
		}

		m_texture.charge_donnees(tampon->tampon);

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
			m_camera = entree(1)->requiers_camera(contexte, donnees_aval);

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
	usine.enregistre_type(cree_desc<OperatriceCamera>());
	usine.enregistre_type(cree_desc<OperatriceTexture>());
}

#pragma clang diagnostic pop
