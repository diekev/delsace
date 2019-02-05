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

#include <random>

#include "bibliotheques/objets/creation.h"
#include "bibliotheques/objets/import_objet.h"
#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/texture/texture.h"
#include "bibliotheques/vision/camera.h"

#include "../corps/adaptrice_creation_corps.h"
#include "../corps/groupes.h"

#include "../attribut.h"
#include "../manipulatrice.h"
#include "../objet.h"
#include "../operatrice_corps.h"
#include "../operatrice_image.h"
#include "../operatrice_objet.h"
#include "../operatrice_scene.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceSortieCorps final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Sortie Corps";
	static constexpr auto AIDE = "Crée une sortie d'un graphe de corps.";

	explicit OperatriceSortieCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(0);
	}

	int type() const override
	{
		return OPERATRICE_SORTIE_CORPS;
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static void ajourne_portee_attr_normaux(Corps *corps)
{
	auto attr_normaux = corps->attribut("N");

	if (attr_normaux != nullptr) {
		if (attr_normaux->taille() == corps->prims()->taille()) {
			attr_normaux->portee = portee_attr::PRIMITIVE;
		}
		else if (attr_normaux->taille() == corps->points()->taille()) {
			attr_normaux->portee = portee_attr::POINT;
		}
	}
}

class OperatriceCreationCorps final : public OperatriceCorps {
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

public:
	static constexpr auto NOM = "Création Corps";
	static constexpr auto AIDE = "Crée un corps.";

	explicit OperatriceCreationCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_objet.jo";
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
		return type == MANIPULATION_POSITION
				|| type == MANIPULATION_ECHELLE
				|| type == MANIPULATION_ROTATION;
	}

	Manipulatrice3D *manipulatrice_3d(int type) override
	{
		if (type == MANIPULATION_POSITION) {
			return &m_manipulatrice_position;
		}

		if (type == MANIPULATION_ECHELLE) {
			return &m_manipulatrice_echelle;
		}

		if (type == MANIPULATION_ROTATION) {
			return &m_manipulatrice_rotation;
		}

		return nullptr;
	}

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override
	{
		dls::math::vec3f position, rotation, taille;

		if (type == MANIPULATION_POSITION) {
			position = dls::math::vec3f(m_manipulatrice_position.pos());
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("position", position);
		}
		else if (type == MANIPULATION_ECHELLE) {
			position = evalue_vecteur("position", temps);
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			taille = dls::math::vec3f(m_manipulatrice_echelle.taille());

			valeur_vecteur("taille", taille);
		}
		else if (type == MANIPULATION_ROTATION) {
			position = evalue_vecteur("position", temps);
			rotation = dls::math::vec3f(m_manipulatrice_rotation.rotation());
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("rotation", rotation * static_cast<float>(POIDS_RAD_DEG));
		}
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto forme = evalue_enum("forme");

		if (forme == "plan") {
			objets::cree_grille(&adaptrice, 1.0f, 1.0f, 2, 2);
		}
		else if (forme == "cylindre") {
			objets::cree_cylindre(&adaptrice, 32, 1.0f, 1.0f, 1.0f);
		}
		else if (forme == "cube") {
			objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);
		}
		else if (forme == "sphère") {
			objets::cree_sphere_uv(&adaptrice, 1.0f);
		}

		ajourne_portee_attr_normaux(&m_corps);

		auto position = evalue_vecteur("position", temps);
		auto rotation = evalue_vecteur("rotation", temps);
		auto taille = evalue_vecteur("taille", temps);

		m_corps.transformation = math::transformation();
		m_corps.transformation *= math::translation(position.x, position.y, position.z);
		m_corps.transformation *= math::echelle(taille.x, taille.y, taille.z);
		m_corps.transformation *= math::rotation_x(rotation.x * static_cast<float>(POIDS_DEG_RAD));
		m_corps.transformation *= math::rotation_y(rotation.y * static_cast<float>(POIDS_DEG_RAD));
		m_corps.transformation *= math::rotation_z(rotation.z * static_cast<float>(POIDS_DEG_RAD));

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));

//		if (input(0)->connectee() == false) {
//			ajoute_avertissement("Aucune texture connectée");
//		}

//		auto texture = input(0)->requiers_texture(rectangle, temps);

//		maillage->texture(texture);

		return EXECUTION_REUSSIE;
	}
};

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

			valeur_vecteur("rotation", rotation * static_cast<float>(POIDS_RAD_DEG));
		}
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		auto const largeur = evalue_entier("largeur");
		auto const hauteur = evalue_entier("hauteur");
		auto const longueur_focale = evalue_decimal("longueur_focale");
		auto const largeur_senseur = evalue_decimal("largeur_senseur");
		auto const proche = evalue_decimal("proche");
		auto const eloigne = evalue_decimal("éloigné");
		auto const projection = evalue_enum("projection");
		auto const position = evalue_vecteur("position", temps);
		auto const rotation = evalue_vecteur("rotation", temps);

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
		m_camera.rotation(rotation * static_cast<float>(POIDS_DEG_RAD));
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

	int execute(Rectangle const &rectangle, const int temps) override
	{
		if (entree(0)->connectee() == false) {
			ajoute_avertissement("Aucune image connectée pour la texture");
			return EXECUTION_ECHOUEE;
		}

		entree(0)->requiers_image(m_image, rectangle, temps);
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
			m_camera = entree(1)->requiers_camera(rectangle, temps);

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

		auto taille_texture = evalue_vecteur("taille_texture", temps);

		m_texture.taille(dls::math::vec3f(taille_texture.x,
											  taille_texture.y,
											  taille_texture.z));
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceLectureObjet final : public OperatriceCorps {
	vision::Camera3D *m_camera = nullptr;
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

	math::transformation m_transformation{};

	std::string m_dernier_chemin = "";

public:
	static constexpr auto NOM = "Lecture Objet";
	static constexpr auto AIDE = "Charge un objet depuis un fichier externe.";

	explicit OperatriceLectureObjet(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	OperatriceLectureObjet(OperatriceLectureObjet const &) = default;
	OperatriceLectureObjet &operator=(OperatriceLectureObjet const &) = default;

	int type_entree(int n) const override
	{
		INUTILISE(n);
		return OPERATRICE_IMAGE;
	}

	const char *nom_entree(int n) override
	{
		INUTILISE(n);
		return "image";
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_lecture_objet.jo";
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
		return type == MANIPULATION_POSITION
				|| type == MANIPULATION_ECHELLE
				|| type == MANIPULATION_ROTATION;
	}

	Manipulatrice3D *manipulatrice_3d(int type) override
	{
		if (type == MANIPULATION_POSITION) {
			return &m_manipulatrice_position;
		}

		if (type == MANIPULATION_ECHELLE) {
			return &m_manipulatrice_echelle;
		}

		if (type == MANIPULATION_ROTATION) {
			return &m_manipulatrice_rotation;
		}

		return nullptr;
	}

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override
	{
		dls::math::vec3f position, rotation, taille;

		if (type == MANIPULATION_POSITION) {
			position = dls::math::vec3f(m_manipulatrice_position.pos());
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("position", position);
		}
		else if (type == MANIPULATION_ECHELLE) {
			position = evalue_vecteur("position", temps);
			rotation = evalue_vecteur("rotation", temps) * static_cast<float>(POIDS_DEG_RAD);
			taille = dls::math::vec3f(m_manipulatrice_echelle.taille());

			valeur_vecteur("taille", taille);
		}
		else if (type == MANIPULATION_ROTATION) {
			position = evalue_vecteur("position", temps);
			rotation = dls::math::vec3f(m_manipulatrice_rotation.rotation());
			taille = evalue_vecteur("taille", temps);

			valeur_vecteur("rotation", rotation * static_cast<float>(POIDS_RAD_DEG));
		}
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		INUTILISE(rectangle);

		auto chemin = evalue_fichier_entree("chemin");

		if (chemin == "") {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (m_dernier_chemin != chemin) {
			m_corps.reinitialise();

			AdaptriceCreationCorps adaptrice;
			adaptrice.corps = &m_corps;

			if (chemin.find(".obj") != std::string::npos) {
				objets::charge_fichier_OBJ(&adaptrice, chemin);
			}
			else if (chemin.find(".stl") != std::string::npos) {
				objets::charge_fichier_STL(&adaptrice, chemin);
			}

			ajourne_portee_attr_normaux(&m_corps);

			m_dernier_chemin = chemin;
		}

		auto position = dls::math::point3f(evalue_vecteur("position", temps));
		auto rotation = evalue_vecteur("rotation", temps);
		auto taille = evalue_vecteur("taille", temps);
		auto echelle_uniforme = evalue_decimal("echelle_uniforme", temps);

		m_transformation = math::transformation();
		m_transformation *= math::translation(position.x, position.y, position.z);
		m_transformation *= math::echelle(taille.x * echelle_uniforme,
										  taille.y * echelle_uniforme,
										  taille.z * echelle_uniforme);
		m_transformation *= math::rotation_x(rotation.x * static_cast<float>(POIDS_DEG_RAD));
		m_transformation *= math::rotation_y(rotation.y * static_cast<float>(POIDS_DEG_RAD));
		m_transformation *= math::rotation_z(rotation.z * static_cast<float>(POIDS_DEG_RAD));

		m_corps.transformation = m_transformation;

		m_manipulatrice_position.pos(position);
		m_manipulatrice_rotation.pos(position);
		m_manipulatrice_echelle.pos(position);

//		if (input(0)->connectee() == false) {
//			ajoute_avertissement("Aucune texture connectée");
//		}

//		auto texture = input(0)->requiers_texture(rectangle, temps);

//		maillage->texture(texture);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationNormaux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Normaux";
	static constexpr auto AIDE = "Crée des normaux pour les maillages d'entrée.";

	explicit OperatriceCreationNormaux(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_normaux.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto type = evalue_enum("type_normaux");
		auto inverse_normaux = evalue_bool("inverse_direction");

		auto attr_normaux = m_corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT, true);

		if (attr_normaux->taille() != 0l) {
			attr_normaux->reinitialise();
		}

		auto liste_prims = m_corps.prims();
		auto nombre_prims = liste_prims->taille();

		if (nombre_prims == 0l) {
			ajoute_avertissement("Aucun polygone trouvé pour calculer les vecteurs normaux");
			return EXECUTION_ECHOUEE;
		}

		auto liste_points = m_corps.points();

		if (type == "plats") {
			attr_normaux->reserve(nombre_prims);
			attr_normaux->portee = portee_attr::PRIMITIVE;

			for (Primitive *prim : liste_prims->prims()) {
				if (prim->type_prim() != type_primitive::POLYGONE) {
					continue;
				}

				auto poly = dynamic_cast<Polygone *>(prim);

				if (poly->type != type_polygone::FERME || poly->nombre_sommets() < 3) {
					attr_normaux->pousse_vec3(dls::math::vec3f(0.0f));
					continue;
				}

				auto const &v0 = liste_points->point(poly->index_point(0));
				auto const &v1 = liste_points->point(poly->index_point(1));
				auto const &v2 = liste_points->point(poly->index_point(2));

				auto const e1 = v1 - v0;
				auto const e2 = v2 - v0;

				auto const nor = normalise(produit_croix(e1, e2));

				if (inverse_normaux) {
					attr_normaux->pousse_vec3(-nor);
				}
				else {
					attr_normaux->pousse_vec3(nor);
				}
			}
		}
		else {
			auto nombre_sommets = m_corps.points()->taille();
			attr_normaux->redimensionne(nombre_sommets);
			attr_normaux->portee = portee_attr::POINT;

			/* calcul le normal de chaque polygone */
			for (Primitive *prim : liste_prims->prims()) {
				if (prim->type_prim() != type_primitive::POLYGONE) {
					continue;
				}

				auto poly = dynamic_cast<Polygone *>(prim);

				if (poly->type != type_polygone::FERME || poly->nombre_sommets() < 3) {
					poly->nor = dls::math::vec3f(0.0f);
					continue;
				}

				auto const &v0 = liste_points->point(poly->index_point(0));
				auto const &v1 = liste_points->point(poly->index_point(1));
				auto const &v2 = liste_points->point(poly->index_point(2));

				auto const e1 = v1 - v0;
				auto const e2 = v2 - v0;

				poly->nor = normalise(produit_croix(e1, e2));
			}

			/* accumule les normaux pour chaque sommets */
			std::vector<int> poids(static_cast<size_t>(nombre_sommets), 0);

			for (Primitive *prim : liste_prims->prims()) {
				if (prim->type_prim() != type_primitive::POLYGONE) {
					continue;
				}

				auto poly = dynamic_cast<Polygone *>(prim);

				if (poly->type == type_polygone::OUVERT || poly->nombre_sommets() < 3) {
					continue;
				}

				for (long i = 0; i < poly->nombre_segments(); ++i) {
					auto const index_sommet = poly->index_point(i);

					if (poids[static_cast<size_t>(index_sommet)] != 0) {
						auto nor = attr_normaux->vec3(index_sommet);
						nor += poly->nor;
						attr_normaux->vec3(index_sommet, nor);
					}
					else {
						attr_normaux->vec3(index_sommet, poly->nor);
					}

					poids[static_cast<size_t>(index_sommet)] += 1;
				}
			}

			/* normalise les normaux */
			for (long n = 0; n < nombre_sommets; ++n) {
				auto nor = attr_normaux->vec3(n);
				nor /= static_cast<float>(poids[static_cast<size_t>(n)]);
				nor = normalise(nor);

				if (inverse_normaux) {
					attr_normaux->vec3(n, -nor);
				}
				else {
					attr_normaux->vec3(n, nor);
				}
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#pragma GCC diagnostic pop

struct SommetOSD {
	SommetOSD() = default;

	SommetOSD(SommetOSD const & src)
	{
		_position[0] = src._position[0];
		_position[1] = src._position[1];
		_position[2] = src._position[2];
	}

	void Clear( void * = nullptr )
	{
		_position[0] = _position[1] = _position[2] = 0.0f;
	}

	void AddWithWeight(SommetOSD const & src, float weight)
	{
		_position[0] += weight * src._position[0];
		_position[1] += weight * src._position[1];
		_position[2] += weight * src._position[2];
	}

	// Public entreface ------------------------------------
	void SetPosition(float x, float y, float z)
	{
		_position[0] = x;
		_position[1] = y;
		_position[2] = z;
	}

	const float *GetPosition() const
	{
		return _position;
	}

private:
	float _position[3];
};

class OperatriceOpenSubDiv final : public OperatriceCorps {
public:
	static constexpr auto NOM = "OpenSubDiv";
	static constexpr auto AIDE = "Sousdivise les maillages d'entrée en utilisant la bibliothèque OpenSubDiv.";

	explicit OperatriceOpenSubDiv(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_opensubdiv.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		/* peuple un descripteur avec nos données crues */
		using Descripteur   = OpenSubdiv::Far::TopologyDescriptor;
		using Refineur      = OpenSubdiv::Far::TopologyRefiner;
		using UsineRafineur = OpenSubdiv::Far::TopologyRefinerFactory<Descripteur>;

		auto niveau_max = evalue_entier("niveau_max");

		auto schema = evalue_enum("schéma");
		OpenSubdiv::Sdc::SchemeType type_subdiv;

		if (schema == "catmark") {
			type_subdiv = OpenSubdiv::Sdc::SCHEME_CATMARK;
		}
		else if (schema == "bilineaire") {
			type_subdiv = OpenSubdiv::Sdc::SCHEME_BILINEAR;
		}
		else if (schema == "boucle") {
			/* À FAIRE : CRASH */
			//type_subdiv = OpenSubdiv::Sdc::SCHEME_LOOP;
			type_subdiv = OpenSubdiv::Sdc::SCHEME_CATMARK;
		}
		else {
			ajoute_avertissement("Type de schéma invalide !");
			return EXECUTION_ECHOUEE;
		}

		OpenSubdiv::Sdc::Options options;

		auto entrep_bord = evalue_enum("entrep_bord");

		if (entrep_bord == "aucune") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_NONE);
		}
		else if (entrep_bord == "segment") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
		}
		else if (entrep_bord == "segment_coin") {
			options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_AND_CORNER);
		}
		else {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
		}

		auto entrep_fvar = evalue_enum("entrep_fvar");

		if (entrep_fvar == "aucune") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_NONE);
		}
		else if (entrep_fvar == "coins_seuls") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_ONLY);
		}
		else if (entrep_fvar == "coins_p1") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS1);
		}
		else if (entrep_fvar == "coins_p2") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS2);
		}
		else if (entrep_fvar == "bordures") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_BOUNDARIES);
		}
		else if (entrep_fvar == "tout") {
			options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_ALL);
		}
		else {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
		}

		auto pliage = evalue_enum("pliage");

		if (pliage == "uniforme") {
			options.SetCreasingMethod(OpenSubdiv::Sdc::Options::CREASE_UNIFORM);
		}
		else if (pliage == "chaikin") {
			options.SetCreasingMethod(OpenSubdiv::Sdc::Options::CREASE_CHAIKIN);
		}
		else {
			ajoute_avertissement("Type de pliage invalide !");
		}

		auto sousdivision_triangle = evalue_enum("sousdivision_triangle");

		if (sousdivision_triangle == "catmark") {
			options.SetTriangleSubdivision(OpenSubdiv::Sdc::Options::TRI_SUB_CATMARK);
		}
		else if (sousdivision_triangle == "lisse") {
			options.SetTriangleSubdivision(OpenSubdiv::Sdc::Options::TRI_SUB_SMOOTH);
		}
		else {
			ajoute_avertissement("Type de sousdivision triangulaire invalide !");
		}

		auto nombre_sommets = m_corps.points()->taille();
		auto nombre_polygones = m_corps.prims()->taille();

		Descripteur desc;
		desc.numVertices = static_cast<int>(nombre_sommets);
		desc.numFaces    = static_cast<int>(nombre_polygones);

		std::vector<int> nombre_sommets_par_poly;
		nombre_sommets_par_poly.reserve(static_cast<size_t>(nombre_polygones));

		std::vector<int> index_sommets_polys;
		index_sommets_polys.reserve(static_cast<size_t>(nombre_sommets * nombre_polygones));

		auto const liste_prims = m_corps.prims();
		for (Primitive *prim : liste_prims->prims()) {
			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);
			nombre_sommets_par_poly.push_back(static_cast<int>(poly->nombre_sommets()));

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				index_sommets_polys.push_back(static_cast<int>(poly->index_point(i)));
			}
		}

		desc.numVertsPerFace = &nombre_sommets_par_poly[0];
		desc.vertIndicesPerFace = &index_sommets_polys[0];

		/* Crée un rafineur depuis le descripteur. */
		auto rafineur = UsineRafineur::Create(
							desc, UsineRafineur::Options(type_subdiv, options));

		/* Rafine uniformément la topologie jusque 'niveau_max'. */
		rafineur->RefineUniform(Refineur::UniformOptions(niveau_max));

		/* Alloue un tampon pouvant contenir le nombre total de sommets à
			 * 'niveau_max' de rafinement. */
		std::vector<SommetOSD> sommets_osd(static_cast<size_t>(rafineur->GetNumVerticesTotal()));
		SommetOSD *sommets = &sommets_osd[0];

		/* Initialise les positions du maillage grossier. */
		auto index_point = 0;
		for (Point3D *point : m_corps.points()->points()) {
			sommets[index_point++].SetPosition(point->x, point->y, point->z);
		}

		/* Entrepole les sommets */
		OpenSubdiv::Far::PrimvarRefiner rafineur_primvar(*rafineur);

		SommetOSD *src_sommets = sommets;
		for (int niveau = 1; niveau <= niveau_max; ++niveau) {
			auto dst_sommets = src_sommets + rafineur->GetLevel(niveau - 1).GetNumVertices();
			rafineur_primvar.Interpolate(niveau, src_sommets, dst_sommets);
			src_sommets = dst_sommets;
		}

		{
			auto const ref_der_niv = rafineur->GetLevel(niveau_max);
			nombre_sommets = ref_der_niv.GetNumVertices();
			nombre_polygones = ref_der_niv.GetNumFaces();

			auto premier_sommet = rafineur->GetNumVerticesTotal() - nombre_sommets;

			m_corps.reinitialise();
			m_corps.points()->reserve(nombre_sommets);
			m_corps.prims()->reserve(nombre_polygones);

			auto liste_points = m_corps.points();

			for (long vert = 0; vert < nombre_sommets; ++vert) {
				float const * pos = sommets[premier_sommet + vert].GetPosition();
				auto p3d = new Point3D;
				p3d->x = pos[0];
				p3d->y = pos[1];
				p3d->z = pos[2];
				liste_points->pousse(p3d);
			}

			for (long face = 0; face < nombre_polygones; ++face) {
				auto fverts = ref_der_niv.GetFaceVertices(static_cast<int>(face));

				auto poly = Polygone::construit(&m_corps, type_polygone::FERME, static_cast<size_t>(fverts.size()));

				for (int i = 0; i < fverts.size(); ++i) {
					poly->ajoute_sommet(static_cast<size_t>(fverts[i]));
				}
			}
		}

		delete rafineur;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Attribut";
	static constexpr auto AIDE = "Crée un attribut.";

	explicit OperatriceCreationAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_creation_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const chaine_type = evalue_enum("type_attribut");
		auto const chaine_portee = evalue_enum("portee_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		type_attribut type;

		if (chaine_type == "ent8") {
			type = type_attribut::ENT8;
		}
		else if (chaine_type == "ent32") {
			type = type_attribut::ENT32;
		}
		else if (chaine_type == "décimal") {
			type = type_attribut::DECIMAL;
		}
		else if (chaine_type == "chaine") {
			type = type_attribut::CHAINE;
		}
		else if (chaine_type == "vec2") {
			type = type_attribut::VEC2;
		}
		else if (chaine_type == "vec3") {
			type = type_attribut::VEC3;
		}
		else if (chaine_type == "vec4") {
			type = type_attribut::VEC4;
		}
		else if (chaine_type == "mat3") {
			type = type_attribut::MAT3;
		}
		else if (chaine_type == "mat4") {
			type = type_attribut::MAT4;
		}
		else {
			ajoute_avertissement("Type d'attribut invalide !");
			return EXECUTION_ECHOUEE;
		}

		portee_attr portee;

		if (chaine_portee == "points") {
			portee = portee_attr::POINT;
		}
		else if (chaine_portee == "primitives") {
			portee = portee_attr::PRIMITIVE;
		}
		else if (chaine_portee == "vertex") {
			portee = portee_attr::VERTEX;
		}
		else if (chaine_portee == "groupe") {
			portee = portee_attr::GROUPE;
		}
		else if (chaine_portee == "corps") {
			portee = portee_attr::CORPS;
		}
		else {
			ajoute_avertissement("Portée d'attribut invalide !");
			return EXECUTION_ECHOUEE;
		}

		m_corps.ajoute_attribut(nom_attribut, type, portee);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSuppressionAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Suppression Attribut";
	static constexpr auto AIDE = "Supprime un attribut.";

	explicit OperatriceSuppressionAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_suppression_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto const nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		m_corps.supprime_attribut(nom_attribut);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* À FAIRE : considère avoir un noeud spécial pour les couleurs. */
class OperatriceRandomisationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Randomisation Attribut";
	static constexpr auto AIDE = "Randomise un attribut.";

	explicit OperatriceRandomisationAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_randomisation_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const graine = evalue_entier("graine");
		auto const distribution = evalue_enum("distribution");
		auto const constante = evalue_decimal("constante");
		auto const val_min = evalue_decimal("valeur_min");
		auto const val_max = evalue_decimal("valeur_max");
		auto const moyenne = evalue_decimal("moyenne");
		auto const ecart_type = evalue_decimal("écart_type");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto attrib = m_corps.attribut(nom_attribut);

		if (attrib == nullptr) {
			ajoute_avertissement("Aucun attribut ne correspond au nom spécifié !");
			return EXECUTION_ECHOUEE;
		}

		switch (attrib->type()) {
			case type_attribut::ENT8:
			{
				std::mt19937 rng(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->ent8()) {
						v = static_cast<char>(constante);
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_int_distribution<char> dist(-128, 127);

					for (auto &v : attrib->ent8()) {
						v = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->ent8()) {
						v = static_cast<char>(dist(rng));
					}
				}

				break;
			}
			case type_attribut::ENT32:
			{
				std::mt19937 rng(graine);
				if (distribution == "constante") {
					for (auto &v : attrib->ent32()) {
						v = static_cast<int>(constante);
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_int_distribution<int> dist(0, std::numeric_limits<int>::max() - 1);

					for (auto &v : attrib->ent32()) {
						v = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->ent32()) {
						v = static_cast<int>(dist(rng));
					}
				}

				break;
			}
			case type_attribut::DECIMAL:
			{
				std::mt19937 rng(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->decimal()) {
						v = constante;
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_real_distribution<float> dist(val_min, val_max);

					for (auto &v : attrib->decimal()) {
						v = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->decimal()) {
						v = dist(rng);
					}
				}

				break;
			}
			case type_attribut::CHAINE:
			{
				ajoute_avertissement("La randomisation d'attribut de type chaine n'est pas supportée !");
				break;
			}
			case type_attribut::VEC2:
			{
				std::mt19937 rng(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec2()) {
						v.x = constante;
						v.y = constante;
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_real_distribution<float> dist(val_min, val_max);

					for (auto &v : attrib->vec2()) {
						v.x = dist(rng);
						v.y = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->vec2()) {
						v.x = dist(rng);
						v.y = dist(rng);
					}
				}

				break;
			}
			case type_attribut::VEC3:
			{
				std::mt19937 rng(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec3()) {
						v.x = constante;
						v.y = constante;
						v.z = constante;
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_real_distribution<float> dist(val_min, val_max);

					for (auto &v : attrib->vec3()) {
						v.x = dist(rng);
						v.y = dist(rng);
						v.z = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->vec3()) {
						v.x = dist(rng);
						v.y = dist(rng);
						v.z = dist(rng);
					}
				}

				break;
			}
			case type_attribut::VEC4:
			{
				std::mt19937 rng(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec4()) {
						v.x = constante;
						v.y = constante;
						v.z = constante;
						v.w = constante;
					}
				}
				else if (distribution == "uniforme") {
					std::uniform_real_distribution<float> dist(val_min, val_max);

					for (auto &v : attrib->vec4()) {
						v.x = dist(rng);
						v.y = dist(rng);
						v.z = dist(rng);
						v.w = dist(rng);
					}
				}
				else if (distribution == "gaussienne") {
					std::normal_distribution<float> dist(moyenne, ecart_type);

					for (auto &v : attrib->vec4()) {
						v.x = dist(rng);
						v.y = dist(rng);
						v.z = dist(rng);
						v.w = dist(rng);
					}
				}

				break;
			}
			case type_attribut::MAT3:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat3 n'est pas supportée !");
				break;
			}
			case type_attribut::MAT4:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat4 n'est pas supportée !");
				break;
			}
			case type_attribut::INVALIDE:
			{
				ajoute_avertissement("Type d'attribut invalide !");
				break;
			}
		}

		return EXECUTION_REUSSIE;
	}

	bool ajourne_proprietes() override
	{
#if 0 /* À FAIRE : ajournement de l'entreface. */
		auto const distribution = evalue_enum("distribution");

		rend_propriete_visible("constante", distribution == "constante");
		rend_propriete_visible("min_value", distribution == "uniforme");
		rend_propriete_visible("max_value", distribution == "uniforme");
		rend_propriete_visible("moyenne", distribution == "gaussienne");
		rend_propriete_visible("ecart_type", distribution == "gaussienne");
#endif
		return true;
	}
};

/* ************************************************************************** */

class OperatriceFusionnageCorps final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fusionnage Corps";
	static constexpr auto AIDE = "Fusionnage Corps.";

	explicit OperatriceFusionnageCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps1 = entree(0)->requiers_corps(rectangle, temps);

		if (corps1 == nullptr) {
			ajoute_avertissement("1er corps manquant !");
			return EXECUTION_ECHOUEE;
		}

		auto corps2 = entree(1)->requiers_corps(rectangle, temps);

		if (corps2 == nullptr) {
			ajoute_avertissement("2ème corps manquant !");
			return EXECUTION_ECHOUEE;
		}

		fusionne_points(corps1, corps2);
		fusionne_primitives(corps1, corps2);
		fusionne_attributs(corps1, corps2);
		fusionne_groupe_points(corps1, corps2);
		fusionne_groupe_prims(corps1, corps2);

		return EXECUTION_REUSSIE;
	}

	void fusionne_points(Corps const *corps1, Corps const *corps2)
	{
		auto liste_point  = m_corps.points();
		auto liste_point1 = corps1->points();
		auto liste_point2 = corps2->points();

		liste_point->reserve(liste_point1->taille() + liste_point2->taille());

		for (Point3D *point : liste_point1->points()) {
			auto p = new Point3D;
			p->x = point->x;
			p->y = point->y;
			p->z = point->z;
			liste_point->pousse(p);
		}

		for (Point3D *point : liste_point2->points()) {
			auto p = new Point3D;
			p->x = point->x;
			p->y = point->y;
			p->z = point->z;
			liste_point->pousse(p);
		}
	}

	void fusionne_primitives(Corps const *corps1, Corps const *corps2)
	{
		auto liste_prims  = m_corps.prims();
		auto liste_prims1 = corps1->prims();
		auto liste_prims2 = corps2->prims();

		liste_prims->reserve(liste_prims1->taille() + liste_prims2->taille());

		for (Primitive *prim : liste_prims->prims()) {
			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);
			auto polygone = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				polygone->ajoute_sommet(poly->index_point(i));
			}
		}

		auto const decalage_point = corps1->points()->taille();

		for (Primitive *prim : liste_prims->prims()) {
			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);
			auto polygone = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				polygone->ajoute_sommet(decalage_point + poly->index_point(i));
			}
		}
	}

	void fusionne_attributs(Corps const *corps1, Corps const *corps2)
	{
		using paire_attribut = std::pair<Attribut *, Attribut *>;
		using tableau_attribut = std::unordered_map<std::string, paire_attribut>;

		auto tableau = tableau_attribut{};

		for (auto attr : corps1->attributs()) {
			tableau.insert({attr->nom(), std::make_pair(attr, nullptr)});
		}

		for (auto attr : corps2->attributs()) {
			auto iter = tableau.find(attr->nom());

			if (iter == tableau.end()) {
				tableau.insert({attr->nom(), std::make_pair(nullptr, attr)});
			}
			else {
				tableau[attr->nom()].second = attr;
			}
		}

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto attr1 = paire_attr.first;
				auto attr2 = paire_attr.second;

				if (attr1->type() != attr2->type()) {
					std::stringstream ss;
					ss << "Les types des attributs '" << attr1->nom() << "' sont différents !";
					ajoute_avertissement(ss.str());
					continue;
				}

				if (attr1->portee != attr2->portee) {
					std::stringstream ss;
					ss << "Les portées des attributs '" << attr1->nom() << "' sont différentes !";
					ajoute_avertissement(ss.str());
					continue;
				}

				auto attr = m_corps.ajoute_attribut(
								attr1->nom(),
								attr1->type(),
								attr1->portee);

				std::memcpy(attr->donnees(), attr1->donnees(), static_cast<size_t>(attr1->taille_octets()));
				auto ptr = static_cast<char *>(attr->donnees()) + attr1->taille_octets();
				std::memcpy(ptr, attr2->donnees(), static_cast<size_t>(attr2->taille_octets()));
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto attr1 = paire_attr.first;

				auto attr = m_corps.ajoute_attribut(
								attr1->nom(),
								attr1->type(),
								attr1->portee);

				std::memcpy(attr->donnees(), attr1->donnees(), static_cast<size_t>(attr1->taille_octets()));
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto attr2 = paire_attr.second;

				auto attr = m_corps.ajoute_attribut(
								attr2->nom(),
								attr2->type(),
								attr2->portee);

				auto decalage = attr->taille_octets() - attr2->taille_octets();
				auto ptr = static_cast<char *>(attr->donnees()) + decalage;
				std::memcpy(ptr, attr2->donnees(), static_cast<size_t>(attr2->taille_octets()));
			}
		}
	}

	void fusionne_groupe_points(Corps const *corps1, Corps const *corps2)
	{
		using paire_groupe = std::pair<GroupePoint *, GroupePoint *>;
		using tableau_groupe = std::unordered_map<std::string, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto groupe : corps1->groupes_points()) {
			tableau.insert({groupe->nom, std::make_pair(groupe, nullptr)});
		}

		for (auto groupe : corps2->groupes_points()) {
			auto iter = tableau.find(groupe->nom);

			if (iter == tableau.end()) {
				tableau.insert({groupe->nom, std::make_pair(nullptr, groupe)});
			}
			else {
				tableau[groupe->nom].second = groupe;
			}
		}

		auto const decalage_points = corps1->points()->taille();

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			auto groupe = m_corps.ajoute_groupe_point(alveole.first);

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto groupe1 = paire_attr.first;
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe1->taille() + groupe2->taille());

				for (auto index : groupe1->index()) {
					groupe->ajoute_point(index);
				}

				for (auto index : groupe2->index()) {
					groupe->ajoute_point(static_cast<size_t>(decalage_points) + index);
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto index : groupe1->index()) {
					groupe->ajoute_point(index);
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto index : groupe2->index()) {
					groupe->ajoute_point(static_cast<size_t>(decalage_points) + index);
				}
			}
		}
	}

	void fusionne_groupe_prims(Corps const *corps1, Corps const *corps2)
	{
		using paire_groupe = std::pair<GroupePrimitive *, GroupePrimitive *>;
		using tableau_groupe = std::unordered_map<std::string, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto groupe : corps1->groupes_prims()) {
			tableau.insert({groupe->nom, std::make_pair(groupe, nullptr)});
		}

		for (auto groupe : corps2->groupes_prims()) {
			auto iter = tableau.find(groupe->nom);

			if (iter == tableau.end()) {
				tableau.insert({groupe->nom, std::make_pair(nullptr, groupe)});
			}
			else {
				tableau[groupe->nom].second = groupe;
			}
		}

		auto const decalage_prims = corps1->prims()->taille();

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			auto groupe = m_corps.ajoute_groupe_point(alveole.first);

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto groupe1 = paire_attr.first;
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe1->taille() + groupe2->taille());

				for (auto index : groupe1->index()) {
					groupe->ajoute_point(index);
				}

				for (auto index : groupe2->index()) {
					groupe->ajoute_point(static_cast<size_t>(decalage_prims) + index);
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto index : groupe1->index()) {
					groupe->ajoute_point(index);
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto index : groupe2->index()) {
					groupe->ajoute_point(static_cast<size_t>(decalage_prims) + index);
				}
			}
		}
	}
};

/* ************************************************************************** */

class OperatriceTransformation : public OperatriceCorps {
public:
	static constexpr auto NOM = "Transformation";
	static constexpr auto AIDE = "Transformer les matrices des primitives d'entrées.";

	OperatriceTransformation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_transformation.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(Rectangle const &rectangle, int temps) override
	{
		entree(0)->requiers_copie_corps(&m_corps, rectangle, temps);

		auto const translate = dls::math::vec3d(evalue_vecteur("translation"));
		auto const rotate = dls::math::vec3d(evalue_vecteur("rotation"));
		auto const scale = dls::math::vec3d(evalue_vecteur("taille"));
		auto const pivot = dls::math::vec3d(evalue_vecteur("pivot"));
		auto const uniform_scale = static_cast<double>(evalue_decimal("échelle"));
		auto const transform_type = evalue_enum("ordre_transformation");
		auto const rot_order = evalue_enum("ordre_rotation");
		auto index_ordre = -1;

		if (rot_order == "xyz") {
			index_ordre = 0;
		}
		else if (rot_order == "xzy") {
			index_ordre = 1;
		}
		else if (rot_order == "yxz") {
			index_ordre = 2;
		}
		else if (rot_order == "yzx") {
			index_ordre = 3;
		}
		else if (rot_order == "zxy") {
			index_ordre = 4;
		}
		else if (rot_order == "zyx") {
			index_ordre = 5;
		}

		/* determine the rotatation order */
		size_t rot_ord[6][3] = {
			{ 0, 1, 2 }, // X Y Z
			{ 0, 2, 1 }, // X Z Y
			{ 1, 0, 2 }, // Y X Z
			{ 1, 2, 0 }, // Y Z X
			{ 2, 0, 1 }, // Z X Y
			{ 2, 1, 0 }, // Z Y X
		};

		dls::math::vec3d axis[3] = {
			dls::math::vec3d(1.0f, 0.0f, 0.0f),
			dls::math::vec3d(0.0f, 1.0f, 0.0f),
			dls::math::vec3d(0.0f, 0.0f, 1.0f),
		};

		auto const X = rot_ord[index_ordre][0];
		auto const Y = rot_ord[index_ordre][1];
		auto const Z = rot_ord[index_ordre][2];

		auto matrice = dls::math::mat4x4d(1.0);
		auto const angle_x = dls::math::degrees_vers_radians(rotate[X]);
		auto const angle_y = dls::math::degrees_vers_radians(rotate[Y]);
		auto const angle_z = dls::math::degrees_vers_radians(rotate[Z]);

		if (transform_type == "pre") {
			matrice = dls::math::pre_translation(matrice, pivot);
			matrice = dls::math::pre_rotation(matrice, angle_x, axis[X]);
			matrice = dls::math::pre_rotation(matrice, angle_y, axis[Y]);
			matrice = dls::math::pre_rotation(matrice, angle_z, axis[Z]);
			matrice = dls::math::pre_dimension(matrice, scale * uniform_scale);
			matrice = dls::math::pre_translation(matrice, -pivot);
			matrice = dls::math::pre_translation(matrice, translate);
			matrice = matrice * m_corps.transformation.matrice();
		}
		else {
			matrice = dls::math::post_translation(matrice, pivot);
			matrice = dls::math::post_rotation(matrice, angle_x, axis[X]);
			matrice = dls::math::post_rotation(matrice, angle_y, axis[Y]);
			matrice = dls::math::post_rotation(matrice, angle_z, axis[Z]);
			matrice = dls::math::post_dimension(matrice, scale * uniform_scale);
			matrice = dls::math::post_translation(matrice, -pivot);
			matrice = dls::math::post_translation(matrice, translate);
			matrice = m_corps.transformation.matrice() * matrice;
		}

		m_corps.transformation = math::transformation(matrice);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_3d(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCamera>());
	usine.enregistre_type(cree_desc<OperatriceScene>());
	usine.enregistre_type(cree_desc<OperatriceObjet>());
	usine.enregistre_type(cree_desc<OperatriceLectureObjet>());
	usine.enregistre_type(cree_desc<OperatriceTexture>());

	usine.enregistre_type(cree_desc<OperatriceSortieCorps>());
	usine.enregistre_type(cree_desc<OperatriceCreationCorps>());
	usine.enregistre_type(cree_desc<OperatriceCreationNormaux>());
	usine.enregistre_type(cree_desc<OperatriceOpenSubDiv>());

	usine.enregistre_type(cree_desc<OperatriceCreationAttribut>());
	usine.enregistre_type(cree_desc<OperatriceSuppressionAttribut>());
	usine.enregistre_type(cree_desc<OperatriceRandomisationAttribut>());

	usine.enregistre_type(cree_desc<OperatriceFusionnageCorps>());

	usine.enregistre_type(cree_desc<OperatriceTransformation>());
}

#pragma clang diagnostic pop
