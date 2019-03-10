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

#include "operatrices_corps.hh"

#include "bibliotheques/kelvinlet/kelvinlet.hh"
#include "bibliotheques/objets/creation.h"
#include "bibliotheques/objets/import_objet.h"
#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/outils/parallelisme.h"

#include "../corps/adaptrice_creation_corps.h"

#include "../contexte_evaluation.hh"
#include "../donnees_aval.hh"
#include "../gestionnaire_fichier.hh"
#include "../manipulatrice.h"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static void ajourne_portee_attr_normaux(Corps *corps)
{
	auto attr_normaux = corps->attribut("N");
	auto attr_normaux_polys = corps->attribut("N_polys");

	if (attr_normaux_polys != nullptr) {
		if (attr_normaux != nullptr) {
			corps->supprime_attribut("N");
		}

		attr_normaux_polys->nom("N");
		return;
	}

	if (attr_normaux != nullptr) {
		if (attr_normaux->taille() == corps->prims()->taille()) {
			attr_normaux->portee = portee_attr::PRIMITIVE;
		}
		else  if (attr_normaux->taille() == corps->points()->taille()) {
			attr_normaux->portee = portee_attr::POINT;
		}
	}
}

/* ************************************************************************** */

class OperatriceCreationCorps : public OperatriceCorps {
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

public:
	explicit OperatriceCreationCorps(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
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
		INUTILISE(temps); /* À FAIRE : animation */
		if (type == MANIPULATION_POSITION) {
			auto position = dls::math::vec3f(m_manipulatrice_position.pos());
			valeur_vecteur("position", position);
		}
		else if (type == MANIPULATION_ECHELLE) {
			auto taille = dls::math::vec3f(m_manipulatrice_echelle.taille());
			valeur_vecteur("taille", taille);
		}
		else if (type == MANIPULATION_ROTATION) {
			auto rotation = dls::math::vec3f(m_manipulatrice_rotation.rotation());
			valeur_vecteur("rotation", rotation * constantes<float>::POIDS_RAD_DEG);
		}

		ajourne_transforme(temps);
	}

	void ajourne_transforme(int const temps)
	{
		auto position = evalue_vecteur("position", temps);
		auto rotation = evalue_vecteur("rotation", temps);
		auto taille = evalue_vecteur("taille", temps);

		m_corps.transformation = math::transformation();
		m_corps.transformation *= math::translation(position.x, position.y, position.z);
		m_corps.transformation *= math::rotation_x(rotation.x * constantes<float>::POIDS_DEG_RAD);
		m_corps.transformation *= math::rotation_y(rotation.y * constantes<float>::POIDS_DEG_RAD);
		m_corps.transformation *= math::rotation_z(rotation.z * constantes<float>::POIDS_DEG_RAD);
		m_corps.transformation *= math::echelle(taille.x, taille.y, taille.z);

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));
	}
};

class OperatriceCreationCube final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cube";
	static constexpr auto AIDE = "Crée un cube.";

	explicit OperatriceCreationCube(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_cube.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto rayon = evalue_vecteur("rayon", contexte.temps_courant);

		objets::cree_boite(&adaptrice, rayon.x, rayon.y, rayon.z);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationSphereUV final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Sphère UV";
	static constexpr auto AIDE = "Crée une sphère UV.";

	explicit OperatriceCreationSphereUV(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_sphere_uv.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const rayon = evalue_decimal("rayon", contexte.temps_courant);
		auto const res_u = evalue_entier("res_u", contexte.temps_courant);
		auto const res_v = evalue_entier("res_v", contexte.temps_courant);

		objets::cree_sphere_uv(&adaptrice, rayon, res_u, res_v);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationCylindre final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cylindre";
	static constexpr auto AIDE = "Crée un cylindre.";

	explicit OperatriceCreationCylindre(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_cylindre.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const segments = evalue_entier("segments", contexte.temps_courant);
		auto const rayon_mineur = evalue_decimal("rayon_mineur", contexte.temps_courant);
		auto const rayon_majeur = evalue_decimal("rayon_majeur", contexte.temps_courant);
		auto const profondeur = evalue_decimal("profondeur", contexte.temps_courant);

		objets::cree_cylindre(&adaptrice, segments, rayon_mineur, rayon_majeur, profondeur);

		calcul_normaux(m_corps, true, false);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationCone final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cone";
	static constexpr auto AIDE = "Crée un cone.";

	explicit OperatriceCreationCone(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_cone.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const segments = evalue_entier("segments", contexte.temps_courant);
		auto const rayon_majeur = evalue_decimal("rayon_majeur", contexte.temps_courant);
		auto const profondeur = evalue_decimal("profondeur", contexte.temps_courant);

		objets::cree_cylindre(&adaptrice, segments, 0.0f, rayon_majeur, profondeur);

		calcul_normaux(m_corps, true, false);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationGrille final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Grille";
	static constexpr auto AIDE = "Crée une grille.";

	explicit OperatriceCreationGrille(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_grille.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const lignes   = evalue_entier("lignes", contexte.temps_courant);
		auto const colonnes = evalue_entier("colonnes", contexte.temps_courant);
		auto const taille_x = evalue_decimal("taille_x", contexte.temps_courant);
		auto const taille_y = evalue_decimal("taille_y", contexte.temps_courant);

		objets::cree_grille(&adaptrice, taille_x, taille_y, lignes, colonnes);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationSphereIco final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Sphère Ico";
	static constexpr auto AIDE = "Crée une sphère ico.";

	explicit OperatriceCreationSphereIco(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_sphere_ico.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const rayon = evalue_decimal("rayon", contexte.temps_courant);

		objets::cree_icosphere(&adaptrice, rayon);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationTorus final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Torus";
	static constexpr auto AIDE = "Crée un torus.";

	explicit OperatriceCreationTorus(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_torus.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const rayon_mineur = evalue_decimal("rayon_mineur", contexte.temps_courant);
		auto const rayon_majeur = evalue_decimal("rayon_majeur", contexte.temps_courant);
		auto const segment_mineur = evalue_entier("segment_mineur", contexte.temps_courant);
		auto const segment_majeur = evalue_entier("segment_majeur", contexte.temps_courant);

		objets::cree_torus(&adaptrice, rayon_mineur, rayon_majeur, segment_mineur, segment_majeur);

		calcul_normaux(m_corps, true, false);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

class OperatriceCreationCercle final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cercle";
	static constexpr auto AIDE = "Crée un cercle.";

	explicit OperatriceCreationCercle(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCreationCorps(graphe_parent, noeud)
	{}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_cercle.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const segment = evalue_entier("segment", contexte.temps_courant);
		auto const rayon   = evalue_decimal("rayon", contexte.temps_courant);

		objets::cree_cercle(&adaptrice, segment, rayon);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationLigne final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Ligne";
	static constexpr auto AIDE = "Crée une ligne.";

	explicit OperatriceCreationLigne(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_ligne.jo";
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
		INUTILISE(contexte);
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto const segments = evalue_entier("segments");
		auto const taille = evalue_decimal("taille");
		auto const direction = normalise(evalue_vecteur("direction"));
		auto const origine = evalue_vecteur("origine");

		auto const taille_segment = taille / static_cast<float>(segments);

		auto t = 0.0f;

		for (auto i = 0; i <= segments; ++i) {
			auto p = origine + t * direction;

			m_corps.ajoute_point(p.x, p.y, p.z);

			t += taille_segment;
		}

		auto poly = Polygone::construit(&m_corps, type_polygone::OUVERT, segments + 1);

		for (auto i = 0; i <= segments; ++i) {
			poly->ajoute_sommet(i);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceLectureObjet final : public OperatriceCorps {
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

	math::transformation m_transformation{};

	std::string m_dernier_chemin = "";

	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Lecture Objet";
	static constexpr auto AIDE = "Charge un objet depuis un fichier externe.";

	explicit OperatriceLectureObjet(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	OperatriceLectureObjet(OperatriceLectureObjet const &) = default;
	OperatriceLectureObjet &operator=(OperatriceLectureObjet const &) = default;

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
		INUTILISE(temps); /* À FAIRE : animation */
		if (type == MANIPULATION_POSITION) {
			auto position = dls::math::vec3f(m_manipulatrice_position.pos());
			valeur_vecteur("position", position);
		}
		else if (type == MANIPULATION_ECHELLE) {
			auto taille = dls::math::vec3f(m_manipulatrice_echelle.taille());
			valeur_vecteur("taille", taille);
		}
		else if (type == MANIPULATION_ROTATION) {
			auto rotation = dls::math::vec3f(m_manipulatrice_rotation.rotation());
			valeur_vecteur("rotation", rotation * constantes<float>::POIDS_RAD_DEG);
		}
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		auto chemin = evalue_fichier_entree("chemin");

		if (chemin == "") {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return EXECUTION_ECHOUEE;
		}

		if (m_dernier_chemin != chemin) {
			m_corps.reinitialise();

			AdaptriceCreationCorps adaptrice;
			adaptrice.corps = &m_corps;

			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			auto donnees = std::any(&adaptrice);

			if (chemin.find(".obj") != std::string::npos) {
				m_poignee_fichier->lecture_chemin(
							[](const char *chemin_, std::any const &donnees_)
				{
					objets::charge_fichier_OBJ(std::any_cast<AdaptriceCreationCorps *>(donnees_), chemin_);
				},
				donnees);

			}
			else if (chemin.find(".stl") != std::string::npos) {
				m_poignee_fichier->lecture_chemin(
							[](const char *chemin_, std::any const &donnees_)
				{
					objets::charge_fichier_STL(std::any_cast<AdaptriceCreationCorps *>(donnees_), chemin_);
				},
				donnees);
			}

			ajourne_portee_attr_normaux(&m_corps);

			m_dernier_chemin = chemin;
		}

		auto position = dls::math::point3f(evalue_vecteur("position", contexte.temps_courant));
		auto rotation = evalue_vecteur("rotation", contexte.temps_courant);
		auto taille = evalue_vecteur("taille", contexte.temps_courant);
		auto echelle_uniforme = evalue_decimal("echelle_uniforme", contexte.temps_courant);

		m_transformation = math::transformation();
		m_transformation *= math::translation(position.x, position.y, position.z);
		m_transformation *= math::rotation_x(rotation.x * constantes<float>::POIDS_DEG_RAD);
		m_transformation *= math::rotation_y(rotation.y * constantes<float>::POIDS_DEG_RAD);
		m_transformation *= math::rotation_z(rotation.z * constantes<float>::POIDS_DEG_RAD);
		m_transformation *= math::echelle(taille.x * echelle_uniforme,
										  taille.y * echelle_uniforme,
										  taille.z * echelle_uniforme);

		m_corps.transformation = m_transformation;

		m_manipulatrice_position.pos(position);
		m_manipulatrice_rotation.pos(position);
		m_manipulatrice_echelle.pos(position);

		return EXECUTION_REUSSIE;
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
		m_corps.reinitialise();

		auto corps1 = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps1 == nullptr) {
			ajoute_avertissement("1er corps manquant !");
			return EXECUTION_ECHOUEE;
		}

		auto corps2 = entree(1)->requiers_corps(contexte, donnees_aval);

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

		for (auto i = 0; i < liste_point1->taille(); ++i) {
			auto point = liste_point1->point(i);
			auto const p_monde = corps1->transformation(
						dls::math::point3d(point));

			auto p = new Point3D;
			p->x = static_cast<float>(p_monde.x);
			p->y = static_cast<float>(p_monde.y);
			p->z = static_cast<float>(p_monde.z);
			liste_point->pousse(p);
		}

		for (auto i = 0; i < liste_point2->taille(); ++i) {
			auto point = liste_point2->point(i);
			auto const p_monde = corps2->transformation(
						dls::math::point3d(point));

			auto p = new Point3D;
			p->x = static_cast<float>(p_monde.x);
			p->y = static_cast<float>(p_monde.y);
			p->z = static_cast<float>(p_monde.z);
			liste_point->pousse(p);
		}
	}

	void fusionne_primitives(Corps const *corps1, Corps const *corps2)
	{
		auto liste_prims  = m_corps.prims();
		auto liste_prims1 = corps1->prims();
		auto liste_prims2 = corps2->prims();

		liste_prims->reserve(liste_prims1->taille() + liste_prims2->taille());

		for (auto ip = 0; ip < liste_prims1->taille(); ++ip) {
			auto prim = liste_prims1->prim(ip);

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

		for (auto ip = 0; ip < liste_prims2->taille(); ++ip) {
			auto prim = liste_prims2->prim(ip);

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
		using paire_groupe = std::pair<GroupePoint const *, GroupePoint const *>;
		using tableau_groupe = std::unordered_map<std::string, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto const &groupe : corps1->groupes_points()) {
			tableau.insert({groupe.nom, {&groupe, nullptr}});
		}

		for (auto const &groupe : corps2->groupes_points()) {
			auto iter = tableau.find(groupe.nom);

			if (iter == tableau.end()) {
				tableau.insert({groupe.nom, std::make_pair(nullptr, &groupe)});
			}
			else {
				tableau[groupe.nom].second = &groupe;
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

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_point(groupe1->index(i));
				}

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_point(static_cast<size_t>(decalage_points) + groupe2->index(i));
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_point(groupe1->index(i));
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_point(static_cast<size_t>(decalage_points) + groupe2->index(i));
				}
			}
		}
	}

	void fusionne_groupe_prims(Corps const *corps1, Corps const *corps2)
	{
		using paire_groupe = std::pair<GroupePrimitive const *, GroupePrimitive const *>;
		using tableau_groupe = std::unordered_map<std::string, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto const &groupe : corps1->groupes_prims()) {
			tableau.insert({groupe.nom, std::make_pair(&groupe, nullptr)});
		}

		for (auto const &groupe : corps2->groupes_prims()) {
			auto iter = tableau.find(groupe.nom);

			if (iter == tableau.end()) {
				tableau.insert({groupe.nom, std::make_pair(nullptr, &groupe)});
			}
			else {
				tableau[groupe.nom].second = &groupe;
			}
		}

		auto const decalage_prims = corps1->prims()->taille();

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			auto groupe = m_corps.ajoute_groupe_primitive(alveole.first);

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto groupe1 = paire_attr.first;
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe1->taille() + groupe2->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_primitive(groupe1->index(i));
				}

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_primitive(static_cast<size_t>(decalage_prims) + groupe2->index(i));
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_primitive(groupe1->index(i));
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_primitive(static_cast<size_t>(decalage_prims) + groupe2->index(i));
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

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
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSeparationPrims final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Séparation Primitives";
	static constexpr auto AIDE = "Sépare les primitives du corps d'entrée de sorte que chaque primitive du corps de sortie n'ait que des points uniques.";

	explicit OperatriceSeparationPrims(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
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
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps en entrée.");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();
		auto points_entree = corps_entree->points();

		for (auto i = 0; i < prims_entree->taille(); ++i) {
			auto prim = prims_entree->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			auto npoly = Polygone::construit(&m_corps, poly->type, poly->nombre_sommets());

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				/* À FAIRE : transforme pos monde ? */
				auto point = points_entree->point(poly->index_point(j));

				auto index = m_corps.ajoute_point(point.x, point.y, point.z);

				npoly->ajoute_sommet(static_cast<long>(index));
			}
		}

		m_corps.transformation = corps_entree->transformation;

		/* À FAIRE : transfère attribut */
		if (corps_entree->attribut("N") != nullptr) {
			/* Les normaux sont forcément plats, car les primitives ont été
			 *  séparées. */
			calcul_normaux(m_corps, true, false);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Kelvinlets dynamiques.
 * Pour l'instant nous utilisons le code de Pixar. Dans le future il
 * serait bien d'utiliser un code orienté données, avec un tableau de
 * déformeur par type de déformation, et en ne considérant que ceux qui
 * sont actifs, (temps_courant - temps_activation) > 0
 */

using Scalar   = Kelvinlet::Scalar;
using Vector3  = Kelvinlet::Vector3;
using Matrix33 = Kelvinlet::Matrix33;
using Deformer = std::vector<Kelvinlet::DynaBase::Ptr>;

template <class DynaType>
static auto ajoute_deformeur(
		Deformer &deformeurs,
		dls::math::vec3f const &pos,
		typename DynaType::Force const &force,
		Scalar nu,
		Scalar mu,
		Scalar eps,
		Scalar time)
{
	auto pos_eigen = Vector3();
	pos_eigen << static_cast<double>(pos.x), static_cast<double>(pos.y), static_cast<double>(pos.z);

	auto def = DynaType{};
	def.SetPoint(pos_eigen);
	def.SetTime(time);
	def.SetEps(eps);
	def.SetMaterial(mu, nu);
	def.SetForce(force);
	def.Calibrate();

	Kelvinlet::DynaBase::Ptr ptr;
	ptr.reset(new DynaType(def));

	deformeurs.push_back(ptr);
}

#include <numeric>

static auto deforme_kelvinlet(
		const Vector3& p,
		const Scalar&  t,
		const Deformer& deformer,
		bool rk4)
{
	Vector3 u = Vector3::Zero();

	if (rk4) {
		auto op = [&](Vector3 const &a, Kelvinlet::DynaBase::Ptr def)
		{
			return a + def->EvalDispRK4(p, t);
		};

		return std::accumulate(deformer.begin(), deformer.end(), u, op);
	}

	auto op = [&](Vector3 const &a, Kelvinlet::DynaBase::Ptr def)
	{
		return a + def->EvalDisp(p, t);
	};

	return std::accumulate(deformer.begin(), deformer.end(), u, op);
}

static void ajoute_deformeur(
		Deformer &deformeur,
		dls::math::vec3f const &pos,
		bool pousse,
		std::string const &action,
		double echelle,
		double vitesse,
		double incompressibilite,
		double temps)
{
	if (action == "grab") {
		Vector3 f = 0.01 * Vector3::UnitY();

		if (pousse) {
			using DynaType = Kelvinlet::DynaPushGrab;
			ajoute_deformeur<DynaType>(deformeur, pos, f, incompressibilite, vitesse, echelle, temps);
		}
		else {
			using DynaType = Kelvinlet::DynaPulseGrab;
			ajoute_deformeur<DynaType>(deformeur, pos, f, incompressibilite, vitesse, echelle, temps);
		}
	}
	/* affine */
	else {
		Matrix33 F = Matrix33::Zero();

		if (action == "twist") {
			Vector3 axisAngle = 0.25 * M_PI * Vector3::UnitZ();
			F = Kelvinlet::AssembleSkewSymMatrix(axisAngle);
		}
		else if (action == "scale") {
			F = Matrix33::Identity();
		}
		else if (action == "pinch") {
			F(0, 0) =  1.0;
			F(1, 1) = -1.0;
		}

		if (pousse) {
			using DynaType = Kelvinlet::DynaPushAffine;
			ajoute_deformeur<DynaType>(deformeur, pos, F, incompressibilite, vitesse, echelle, temps);
		}
		else {
			using DynaType = Kelvinlet::DynaPulseAffine;
			ajoute_deformeur<DynaType>(deformeur, pos, F, incompressibilite, vitesse, echelle, temps);
		}
	}
}

class OpCreationKelvinlet final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Kelvinlet";
	static constexpr auto AIDE = "Ajout un ou plusieurs déformeurs pour calculer les dynamiques de Kelvinlets.";

	explicit OpCreationKelvinlet(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1); // À FAIRE : une seule connexion de sortie
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_kelvinlet.jo";
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
		if (donnees_aval == nullptr || !donnees_aval->possede("déformeurs_kelvinlet")) {
			this->ajoute_avertissement("Aucune opératrice d'évaluation de kelvinlets en aval");
			return EXECUTION_ECHOUEE;
		}

		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		/* À FAIRE : composition et paramétrage des déformeurs,
		 * Il faut pouvoir choisir le point de départ, la rotation, l'échelle,
		 * l'activation, etc. voir vidéo de démonstration.
		 * Il est également possible de définir le temps d'activation, donc il
		 * faut peut-être stocker les déformeurs. */
		auto incompressibilite = static_cast<double>(evalue_decimal("incompressibilité"));
		auto vitesse = static_cast<double>(evalue_decimal("vitesse"));
		auto echelle = static_cast<double>(evalue_decimal("échelle"));
		auto pousse = evalue_bool("pousse");
		auto action = evalue_enum("action_");

		incompressibilite = std::min(incompressibilite, 0.5);
		vitesse = std::max(vitesse, 0.00001);
		echelle = std::max(echelle, 0.00001);

		auto deformeur = std::any_cast<Deformer *>(donnees_aval->table["déformeurs_kelvinlet"]);

		if (!deformeur) {
			this->ajoute_avertissement("Le deformeur est nul");
			return EXECUTION_ECHOUEE;
		}

		/* À FAIRE : réinitialisation, trouver quand créer les déformeurs. */
		deformeur->clear();

		/* À FAIRE :
		 * - contrainte par groupe
		 * - temps d'activation + aléa
		 */

		if (corps_entree != nullptr) {
			// ajoute un déformeur pour chaque points de l'entrée
			auto points = corps_entree->points();

			for (auto i = 0; i < points->taille(); ++i) {
				auto point = corps_entree->point_transforme(i);

				ajoute_deformeur(
							*deformeur,
							point,
							pousse,
							action,
							echelle,
							vitesse,
							incompressibilite,
							contexte.temps_courant);
			}
		}
		else {
			ajoute_deformeur(
						*deformeur,
						dls::math::vec3f(0.0f),
						pousse,
						action,
						echelle,
						vitesse,
						incompressibilite,
						contexte.temps_courant);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpEvaluationKelvinlet final : public OperatriceCorps {
	Deformer m_deformeurs{};

public:
	static constexpr auto NOM = "Évaluation Kelvinlet";
	static constexpr auto AIDE = "";

	explicit OpEvaluationKelvinlet(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_evaluation_kelvinlet.jo";
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
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (m_corps.points()->taille() == 0) {
			this->ajoute_avertissement("Le corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto temps = static_cast<double>(evalue_decimal("temps", contexte.temps_courant));
		auto integration = evalue_enum("intégration");
		auto debut = evalue_entier("début");

		if (contexte.temps_courant <= debut) {
			m_deformeurs.clear();
			return EXECUTION_REUSSIE;
		}

		/* À FAIRE : définit quand les déformeurs sont ajoutés, accumule les
		 *  déformeurs */
		auto mes_donnnes = DonneesAval{};
		mes_donnnes.table.insert({"déformeurs_kelvinlet", &m_deformeurs});

		entree(1)->requiers_corps(contexte, &mes_donnnes);

		/* calcule la déformation */
		auto points_entree = m_corps.points();

		auto const rk4 = integration == "rk4";

		points_entree->detache();

		boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto p = m_corps.point_transforme(i);

				auto point_eigen = Vector3();
				point_eigen << static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(p.z);

				auto dist = deforme_kelvinlet(point_eigen, temps, m_deformeurs, rk4);

				p.x += static_cast<float>(dist[0]);
				p.y += static_cast<float>(dist[1]);
				p.z += static_cast<float>(dist[2]);

				points_entree->point(i, p);
			}
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_corps(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationGrille>());
	usine.enregistre_type(cree_desc<OperatriceCreationSphereUV>());
	usine.enregistre_type(cree_desc<OperatriceCreationSphereIco>());
	usine.enregistre_type(cree_desc<OperatriceCreationCube>());
	usine.enregistre_type(cree_desc<OperatriceCreationCylindre>());
	usine.enregistre_type(cree_desc<OperatriceCreationCone>());
	usine.enregistre_type(cree_desc<OperatriceCreationCercle>());
	usine.enregistre_type(cree_desc<OperatriceCreationTorus>());
	usine.enregistre_type(cree_desc<OperatriceCreationLigne>());
	usine.enregistre_type(cree_desc<OperatriceLectureObjet>());
	usine.enregistre_type(cree_desc<OperatriceSortieCorps>());
	usine.enregistre_type(cree_desc<OperatriceFusionnageCorps>());
	usine.enregistre_type(cree_desc<OperatriceTransformation>());
	usine.enregistre_type(cree_desc<OperatriceSeparationPrims>());
	usine.enregistre_type(cree_desc<OpEvaluationKelvinlet>());
	usine.enregistre_type(cree_desc<OpCreationKelvinlet>());
}

#pragma clang diagnostic pop
