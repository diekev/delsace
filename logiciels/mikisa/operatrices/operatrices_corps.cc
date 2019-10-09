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

#include <numeric>

#include "biblexternes/kelvinlet/kelvinlet.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/objets/creation.h"
#include "biblinternes/objets/import_objet.h"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/dico_desordonne.hh"

#include "corps/adaptrice_creation_corps.h"
#include "corps/iteration_corps.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/gestionnaire_fichier.hh"
#include "coeur/manipulatrice.h"
#include "coeur/noeud.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "evaluation/reseau.hh"

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
		else  if (attr_normaux->taille() == corps->points_pour_lecture().taille()) {
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
	OperatriceCreationCorps(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

		m_corps.transformation = math::construit_transformation(position, rotation, taille);

		m_manipulatrice_position.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_rotation.pos(dls::math::point3f(position.x, position.y, position.z));
		m_manipulatrice_echelle.pos(dls::math::point3f(position.x, position.y, position.z));
	}
};

class OperatriceCreationCube final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cube";
	static constexpr auto AIDE = "Crée un cube.";

	OperatriceCreationCube(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto rayon = evalue_vecteur("rayon", contexte.temps_courant);

		objets::cree_boite(&adaptrice, rayon.x, rayon.y, rayon.z);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationSphereUV final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Sphère UV";
	static constexpr auto AIDE = "Crée une sphère UV.";

	OperatriceCreationSphereUV(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationCylindre final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cylindre";
	static constexpr auto AIDE = "Crée un cylindre.";

	OperatriceCreationCylindre(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationCone final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cone";
	static constexpr auto AIDE = "Crée un cone.";

	OperatriceCreationCone(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationGrille final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Grille";
	static constexpr auto AIDE = "Crée une grille.";

	OperatriceCreationGrille(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationSphereIco final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Sphère Ico";
	static constexpr auto AIDE = "Crée une sphère ico.";

	OperatriceCreationSphereIco(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		AdaptriceCreationCorps adaptrice;
		adaptrice.corps = &m_corps;

		auto const rayon = evalue_decimal("rayon", contexte.temps_courant);

		objets::cree_icosphere(&adaptrice, rayon);

		ajourne_portee_attr_normaux(&m_corps);

		ajourne_transforme(contexte.temps_courant);

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationTorus final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Torus";
	static constexpr auto AIDE = "Crée un torus.";

	OperatriceCreationTorus(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

class OperatriceCreationCercle final : public OperatriceCreationCorps {
public:
	static constexpr auto NOM = "Création Cercle";
	static constexpr auto AIDE = "Crée un cercle.";

	OperatriceCreationCercle(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCreationCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationPrimSphere final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Primitive Sphère";
	static constexpr auto AIDE = "Crée une primitive de type sphère.";

	OperatriceCreationPrimSphere(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_primitive_sphere.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto position = evalue_vecteur("pos_sphère", contexte.temps_courant);
		auto rayon = evalue_decimal("rayon_sphère", contexte.temps_courant);

		auto points = m_corps.points_pour_ecriture();
		auto idx_point = points.ajoute_point(position);
		m_corps.ajoute_sphere(idx_point, rayon);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCreationLigne final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Ligne";
	static constexpr auto AIDE = "Crée une ligne.";

	OperatriceCreationLigne(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		auto points = m_corps.points_pour_ecriture();
		auto poly = m_corps.ajoute_polygone(type_polygone::OUVERT, segments + 1);

		for (auto i = 0; i <= segments; ++i) {
			auto p = origine + t * direction;

			auto idx = points.ajoute_point(p.x, p.y, p.z);
			m_corps.ajoute_sommet(poly, idx);

			t += taille_segment;
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpImportObjet final : public OperatriceCorps {
	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

	math::transformation m_transformation{};

	dls::chaine m_dernier_chemin = "";

	PoigneeFichier *m_poignee_fichier = nullptr;

public:
	static constexpr auto NOM = "Import Objet";
	static constexpr auto AIDE = "Charge un objet depuis un fichier externe.";

	OpImportObjet(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	OpImportObjet(OpImportObjet const &) = default;
	OpImportObjet &operator=(OpImportObjet const &) = default;

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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		auto chemin = evalue_fichier_entree("chemin");

		if (chemin == "") {
			ajoute_avertissement("Le chemin de fichier est vide !");
			return res_exec::ECHOUEE;
		}

		if (m_dernier_chemin != chemin) {
			m_corps.reinitialise();

			AdaptriceCreationCorps adaptrice;
			adaptrice.corps = &m_corps;

			m_poignee_fichier = contexte.gestionnaire_fichier->poignee_fichier(chemin);
			auto donnees = std::any(&adaptrice);

			if (chemin.trouve(".obj") != dls::chaine::npos) {
				m_poignee_fichier->lecture_chemin(
							[](const char *chemin_, std::any const &donnees_)
				{
					objets::charge_fichier_OBJ(std::any_cast<AdaptriceCreationCorps *>(donnees_), chemin_);
				},
				donnees);

			}
			else if (chemin.trouve(".stl") != dls::chaine::npos) {
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

		m_transformation = math::construit_transformation(
					dls::math::vec3f(position),
					rotation,
					taille * echelle_uniforme);

		m_corps.transformation = m_transformation;

		m_manipulatrice_position.pos(position);
		m_manipulatrice_rotation.pos(position);
		m_manipulatrice_echelle.pos(position);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceFusionnageCorps final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fusionnage Corps";
	static constexpr auto AIDE = "Fusionnage Corps.";

	OperatriceFusionnageCorps(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps1 = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps1 == nullptr) {
			ajoute_avertissement("1er corps manquant !");
			return res_exec::ECHOUEE;
		}

		auto corps2 = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps2 == nullptr) {
			ajoute_avertissement("2ème corps manquant !");
			return res_exec::ECHOUEE;
		}

		fusionne_points(corps1, corps2);
		fusionne_primitives(corps1, corps2);
		fusionne_attributs(corps1, corps2);
		fusionne_groupe_points(corps1, corps2);
		fusionne_groupe_prims(corps1, corps2);

		return res_exec::REUSSIE;
	}

	void fusionne_points(Corps const *corps1, Corps const *corps2)
	{
		auto liste_point  = m_corps.points_pour_ecriture();
		auto liste_point1 = corps1->points_pour_lecture();
		auto liste_point2 = corps2->points_pour_lecture();

		liste_point.reserve(liste_point1.taille() + liste_point2.taille());

		for (auto i = 0; i < liste_point1.taille(); ++i) {
			auto p = liste_point1.point_monde(i);
			liste_point.ajoute_point(p);
		}

		for (auto i = 0; i < liste_point2.taille(); ++i) {
			auto p = liste_point2.point_monde(i);
			liste_point.ajoute_point(p);
		}
	}

	void fusionne_primitives(Corps const *corps1, Corps const *corps2)
	{
		auto liste_prims  = m_corps.prims();
		auto liste_prims1 = corps1->prims();
		auto liste_prims2 = corps2->prims();

		liste_prims->reserve(liste_prims1->taille() + liste_prims2->taille());

		pour_chaque_polygone(*corps1,
							 [&](Corps const &, Polygone *poly)
		{
			auto polygone = m_corps.ajoute_polygone(poly->type, poly->nombre_sommets());

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				m_corps.ajoute_sommet(polygone, poly->index_point(i));
			}
		});

		auto const decalage_point = corps1->points_pour_lecture().taille();

		pour_chaque_polygone(*corps2,
							 [&](Corps const &, Polygone *poly)
		{
			auto polygone = m_corps.ajoute_polygone(poly->type, poly->nombre_sommets());

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				m_corps.ajoute_sommet(polygone, decalage_point + poly->index_point(i));
			}
		});
	}

	void fusionne_attributs(Corps const *corps1, Corps const *corps2)
	{
		using paire_attribut = std::pair<Attribut const *, Attribut const *>;
		using tableau_attribut = dls::dico_desordonne<dls::chaine, paire_attribut>;

		auto tableau = tableau_attribut{};

		for (auto const &attr : corps1->attributs()) {
			tableau.insere({ attr.nom(), std::make_pair(&attr, nullptr) });
		}

		for (auto const &attr : corps2->attributs()) {
			auto iter = tableau.trouve(attr.nom());

			if (iter == tableau.fin()) {
				tableau.insere({ attr.nom(), std::make_pair(nullptr, &attr) });
			}
			else {
				tableau[attr.nom()].second = &attr;
			}
		}

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto attr1 = paire_attr.first;
				auto attr2 = paire_attr.second;

				if (attr1->type() != attr2->type()) {
					ajoute_avertissement("Les types des attributs '", attr1->nom(), "' sont différents !");
					continue;
				}

				if (attr1->portee != attr2->portee) {
					ajoute_avertissement("Les portées des attributs '", attr1->nom(), "' sont différentes !");
					continue;
				}

				auto attr = m_corps.ajoute_attribut(
							attr1->nom(),
							attr1->type(),
							attr1->dimensions,
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
							attr1->dimensions,
							attr1->portee);

				std::memcpy(attr->donnees(), attr1->donnees(), static_cast<size_t>(attr1->taille_octets()));
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto attr2 = paire_attr.second;

				auto attr = m_corps.ajoute_attribut(
							attr2->nom(),
							attr2->type(),
							attr2->dimensions,
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
		using tableau_groupe = dls::dico_desordonne<dls::chaine, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto const &groupe : corps1->groupes_points()) {
			tableau.insere({groupe.nom, {&groupe, nullptr}});
		}

		for (auto const &groupe : corps2->groupes_points()) {
			auto iter = tableau.trouve(groupe.nom);

			if (iter == tableau.fin()) {
				tableau.insere({groupe.nom, std::make_pair(nullptr, &groupe)});
			}
			else {
				tableau[groupe.nom].second = &groupe;
			}
		}

		auto const decalage_points = corps1->points_pour_lecture().taille();

		for (auto const &alveole : tableau) {
			auto const &paire_attr = alveole.second;

			auto groupe = m_corps.ajoute_groupe_point(alveole.first);

			if (paire_attr.first != nullptr && paire_attr.second != nullptr) {
				auto groupe1 = paire_attr.first;
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe1->taille() + groupe2->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_index(groupe1->index(i));
				}

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_index(decalage_points + groupe2->index(i));
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_index(groupe1->index(i));
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_index(decalage_points + groupe2->index(i));
				}
			}
		}
	}

	void fusionne_groupe_prims(Corps const *corps1, Corps const *corps2)
	{
		using paire_groupe = std::pair<GroupePrimitive const *, GroupePrimitive const *>;
		using tableau_groupe = dls::dico_desordonne<dls::chaine, paire_groupe>;

		auto tableau = tableau_groupe{};

		for (auto const &groupe : corps1->groupes_prims()) {
			tableau.insere({groupe.nom, std::make_pair(&groupe, nullptr)});
		}

		for (auto const &groupe : corps2->groupes_prims()) {
			auto iter = tableau.trouve(groupe.nom);

			if (iter == tableau.fin()) {
				tableau.insere({groupe.nom, std::make_pair(nullptr, &groupe)});
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
					groupe->ajoute_index(groupe1->index(i));
				}

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_index(decalage_prims + groupe2->index(i));
				}
			}
			else if (paire_attr.first != nullptr && paire_attr.second == nullptr) {
				auto groupe1 = paire_attr.first;

				groupe->reserve(groupe1->taille());

				for (auto i = 0; i < groupe1->taille(); ++i) {
					groupe->ajoute_index(groupe1->index(i));
				}
			}
			else if (paire_attr.first == nullptr && paire_attr.second != nullptr) {
				auto groupe2 = paire_attr.second;

				groupe->reserve(groupe2->taille());

				for (auto i = 0; i < groupe2->taille(); ++i) {
					groupe->ajoute_index(decalage_prims + groupe2->index(i));
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

	OperatriceTransformation(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
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

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSortieCorps final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Sortie Corps";
	static constexpr auto AIDE = "Crée une sortie d'un graphe de corps.";

	OperatriceSortieCorps(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);

		noeud.est_sortie = true;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSeparationPrims final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Séparation Primitives";
	static constexpr auto AIDE = "Sépare les primitives du corps d'entrée de sorte que chaque primitive du corps de sortie n'ait que des points uniques.";

	OperatriceSeparationPrims(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps en entrée.");
			return res_exec::ECHOUEE;
		}

		auto points_entree = corps_entree->points_pour_lecture();
		auto points_sortie = m_corps.points_pour_ecriture();

		/* À FAIRE : transfère attributs groupes */
		auto transfere = TRANSFERE_ATTR_CORPS
				| TRANSFERE_ATTR_PRIMS
				| TRANSFERE_ATTR_POINTS
				| TRANSFERE_ATTR_SOMMETS;

		auto transferante = TransferanteAttribut(*corps_entree, m_corps, transfere);
		transferante.transfere_attributs_corps(0, 0);

		pour_chaque_polygone(*corps_entree,
							 [&](Corps const &, Polygone *poly)
		{
			auto npoly = m_corps.ajoute_polygone(poly->type, poly->nombre_sommets());
			transferante.transfere_attributs_prims(poly->index, npoly->index);

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {
				auto idx_pnt_orig = poly->index_point(j);
				auto idx_smt_orig = poly->index_sommet(j);
				auto point = points_entree.point_local(poly->index_point(j));

				auto index = points_sortie.ajoute_point(point.x, point.y, point.z);
				transferante.transfere_attributs_points(idx_pnt_orig, index);

				auto idx_sommet = m_corps.ajoute_sommet(npoly, index);
				transferante.transfere_attributs_sommets(idx_smt_orig, idx_sommet);
			}
		});

		m_corps.transformation = corps_entree->transformation;

		return res_exec::REUSSIE;
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
using Deformer = dls::tableau<Kelvinlet::DynaBase::Ptr>;

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

	deformeurs.pousse(ptr);
}

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

		return std::accumulate(deformer.debut(), deformer.fin(), u, op);
	}

	auto op = [&](Vector3 const &a, Kelvinlet::DynaBase::Ptr def)
	{
		return a + def->EvalDisp(p, t);
	};

	return std::accumulate(deformer.debut(), deformer.fin(), u, op);
}

static void ajoute_deformeur(
		Deformer &deformeur,
		dls::math::vec3f const &pos,
		bool pousse,
		dls::chaine const &action,
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

	OpCreationKelvinlet(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		if (donnees_aval == nullptr || !donnees_aval->possede("déformeurs_kelvinlet")) {
			this->ajoute_avertissement("Aucune opératrice d'évaluation de kelvinlets en aval");
			return res_exec::ECHOUEE;
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
			return res_exec::ECHOUEE;
		}

		/* À FAIRE : réinitialisation, trouver quand créer les déformeurs. */
		deformeur->efface();

		/* À FAIRE :
		 * - contrainte par groupe
		 * - temps d'activation + aléa
		 */

		if (corps_entree != nullptr) {
			// ajoute un déformeur pour chaque points de l'entrée
			auto points = corps_entree->points_pour_lecture();

			for (auto i = 0; i < points.taille(); ++i) {
				auto point = points.point_monde(i);

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

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpEvaluationKelvinlet final : public OperatriceCorps {
	Deformer m_deformeurs{};

public:
	static constexpr auto NOM = "Évaluation Kelvinlet";
	static constexpr auto AIDE = "";

	OpEvaluationKelvinlet(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, false)) {
			return res_exec::ECHOUEE;
		}

		auto temps = static_cast<double>(evalue_decimal("temps", contexte.temps_courant));
		auto integration = evalue_enum("intégration");
		auto debut = evalue_entier("début");

		if (contexte.temps_courant <= debut) {
			m_deformeurs.efface();
			return res_exec::REUSSIE;
		}

		/* À FAIRE : définit quand les déformeurs sont ajoutés, accumule les
		 *  déformeurs */
		auto mes_donnnes = DonneesAval{};
		mes_donnnes.table.insere({"déformeurs_kelvinlet", &m_deformeurs});

		entree(1)->requiers_corps(contexte, &mes_donnnes);

		/* calcule la déformation */
		auto points_entree = m_corps.points_pour_ecriture();

		auto const rk4 = integration == "rk4";

		boucle_parallele(tbb::blocked_range<long>(0, points_entree.taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto p = points_entree.point_monde(i);

				auto point_eigen = Vector3();
				point_eigen << static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(p.z);

				auto dist = deforme_kelvinlet(point_eigen, temps, m_deformeurs, rk4);

				p.x += static_cast<float>(dist[0]);
				p.y += static_cast<float>(dist[1]);
				p.z += static_cast<float>(dist[2]);

				points_entree.point(i, p);
			}
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

using DeformeursBrosse = dls::tableau<Kelvinlet::BrushBase::Ptr>;

template<class BrushType>
static void ajoute_deformeur(
		DeformeursBrosse& deformer,
		const typename BrushType::Force& force,
		double incompressibilite,
		double echelle)
{
	//-----------------------//
	// Hard-coded Parameters //
	//-----------------------//
	auto vitesse  = 5.0; // > 0.

	BrushType def;
	def.SetEps(echelle);
	def.SetMaterial(vitesse, incompressibilite);
	def.SetForce(force);
	def.Calibrate();

	Kelvinlet::BrushBase::Ptr ptr;
	ptr.reset(new BrushType(def));
	deformer.pousse(ptr);
}

static Vector3 deforme_kelvinlet_brosse(
		const Vector3& p,
		const DeformeursBrosse& deformer)
{
	Vector3 u = Vector3::Zero();
	for (const auto& def : deformer) {
		u += def->Eval(p);
	}
	return u;
}

class OpDeformationKelvinlet final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Déformation Kelvinlet";
	static constexpr auto AIDE = "";

	OpDeformationKelvinlet(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_deformation_kelvinlet.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		if (!valide_corps_entree(*this, &m_corps, true, false)) {
			return res_exec::ECHOUEE;
		}

		auto const repetition = evalue_entier("répétition");
		auto const incompressibilite = static_cast<double>(evalue_decimal("incompressibilité"));
		auto const echelle = static_cast<double>(evalue_decimal("échelle"));
		auto const type = evalue_enum("type");

		auto deformeurs = DeformeursBrosse{};

		if (type == "regulier") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrab;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "biscale") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabBiScale;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "triscale") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabTriScale;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "laplacien") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabLaplacian;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "bilaplacien") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabBiLaplacian;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "cusp") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabCusp;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "cusp_laplacien") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabCuspLaplacian;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "cusp_bilaplacien") {
			Vector3 f = Vector3::UnitY();
			using BrushType = Kelvinlet::BrushGrabCuspBiLaplacian;
			ajoute_deformeur<BrushType>(deformeurs, f, incompressibilite, echelle);
		}
		else if (type == "twist") {
			Vector3 axisAngle = 0.25 * M_PI * Vector3::UnitZ();
			Matrix33 F = Kelvinlet::AssembleSkewSymMatrix(axisAngle);
			using BrushType = Kelvinlet::BrushAffine;
			ajoute_deformeur<BrushType>(deformeurs, F, incompressibilite, echelle);
		}
		else if (type == "scale") {
			Matrix33 F = Matrix33::Identity();
			using BrushType = Kelvinlet::BrushAffine;
			ajoute_deformeur<BrushType>(deformeurs, F, incompressibilite, echelle);
		}
		else if (type == "pinch") {
			Matrix33 F = Matrix33::Zero();
			F(0,0) =  0.75;
			F(1,1) = -F(0,0);
			using BrushType = Kelvinlet::BrushAffine;
			ajoute_deformeur<BrushType>(deformeurs, F, incompressibilite, echelle);
		}
		else {
			this->ajoute_avertissement("Type de kelvinlet inconnu");
			return res_exec::ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("déformation kelvinlet");
		chef->indique_progression(0.0f);

		/* calcule la déformation */
		auto points_entree = m_corps.points_pour_ecriture();

		boucle_parallele(tbb::blocked_range<long>(0, points_entree.taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto p = points_entree.point_monde(i);

				auto point_eigen = Vector3();

				for (auto j = 0; j < repetition; ++j) {
					point_eigen << static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(p.z);

					auto dist = deforme_kelvinlet_brosse(point_eigen, deformeurs);

					p.x += static_cast<float>(dist[0]);
					p.y += static_cast<float>(dist[1]);
					p.z += static_cast<float>(dist[2]);
				}

				points_entree.point(i, p);
			}

			auto delta = static_cast<float>(plage.end() - plage.begin()) / static_cast<float>(points_entree.taille());
			chef->indique_progression_parallele(delta);
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpCreationPancarte final : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Création Pancartes";
	static constexpr auto AIDE = "Crée des pancartes qui font toujours face à la caméra.";

	OpCreationPancarte(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
	}

	COPIE_CONSTRUCT(OpCreationPancarte);

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_pancartes.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_caméra");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();

		auto corps_ref = entree(0)->requiers_corps(contexte, donnees_aval);

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Ne peut pas trouver l'objet caméra !");
			return res_exec::ECHOUEE;
		}

		if (m_objet->type != type_objet::CAMERA) {
			this->ajoute_avertissement("L'objet n'est pas une caméra !");
			return res_exec::ECHOUEE;
		}

		auto camera = static_cast<vision::Camera3D *>(nullptr);

		m_objet->donnees.accede_ecriture([&](DonneesObjet *donnees)
		{
			camera = &extrait_camera(donnees);
		});

		auto attr_UV = m_corps.ajoute_attribut(
					"UV", type_attribut::R32, 2, portee_attr::VERTEX);

		auto taille_uniforme = evalue_decimal("rayon_uniforme", contexte.temps_courant);
		auto taille_x = evalue_decimal("rayon_x", contexte.temps_courant);
		auto taille_y = evalue_decimal("rayon_y", contexte.temps_courant);

		taille_x *= taille_uniforme;
		taille_y *= taille_uniforme;

		dls::math::vec3f points_pancate[4] = {
			dls::math::vec3f(-taille_x, 0.0f, -taille_y),
			dls::math::vec3f( taille_x, 0.0f, -taille_y),
			dls::math::vec3f( taille_x, 0.0f,  taille_y),
			dls::math::vec3f(-taille_x, 0.0f,  taille_y),
		};

		if (corps_ref != nullptr) {
			auto points_ref = corps_ref->points_pour_lecture();

			for (auto i = 0; i < points_ref.taille(); ++i) {
				auto point = points_ref.point_monde(i);
				cree_pancarte(camera, point, *attr_UV, points_pancate);
			}
		}
		else {
			auto const pos = evalue_vecteur("position", contexte.temps_courant);
			cree_pancarte(camera, pos, *attr_UV, points_pancate);
		}

		return res_exec::REUSSIE;
	}

	void cree_pancarte(
			vision::Camera3D *camera,
			dls::math::vec3f const &pos,
			Attribut &attr_UV,
			dls::math::vec3f *points_pancate)
	{
		dls::math::vec2f uvs_pancarte[4] = {
			dls::math::vec2f(-1.0f, -1.0f),
			dls::math::vec2f( 1.0f, -1.0f),
			dls::math::vec2f( 1.0f,  1.0f),
			dls::math::vec2f(-1.0f,  1.0f),
		};

		auto axe_y = normalise(camera->pos() - pos);
		auto axe_x = normalise(vec_ortho(axe_y));
		auto axe_z = normalise(produit_croix(axe_y, axe_x));

		auto mat = dls::math::mat3x3f(
					axe_x.x, axe_x.y, axe_x.z,
					axe_y.x, axe_y.y, axe_y.z,
					axe_z.x, axe_z.y, axe_z.z);

		auto points = m_corps.points_pour_ecriture();
		auto poly = m_corps.ajoute_polygone(type_polygone::FERME, 4);

		for (auto i = 0; i < 4; ++i) {
			auto idx_point = points.ajoute_point(mat * points_pancate[i] + pos);
			auto idx_sommet = m_corps.ajoute_sommet(poly, idx_point);
			assigne(attr_UV.r32(idx_sommet), uvs_pancarte[i]);
		}
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud_reseau, m_objet->noeud);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_caméra") {
			for (auto &objet : contexte.bdd->objets()) {
				liste.pousse(objet->noeud->nom);
			}
		}
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
	usine.enregistre_type(cree_desc<OperatriceCreationPrimSphere>());
	usine.enregistre_type(cree_desc<OpImportObjet>());
	usine.enregistre_type(cree_desc<OperatriceSortieCorps>());
	usine.enregistre_type(cree_desc<OperatriceFusionnageCorps>());
	usine.enregistre_type(cree_desc<OperatriceTransformation>());
	usine.enregistre_type(cree_desc<OperatriceSeparationPrims>());
	usine.enregistre_type(cree_desc<OpEvaluationKelvinlet>());
	usine.enregistre_type(cree_desc<OpCreationKelvinlet>());
	usine.enregistre_type(cree_desc<OpDeformationKelvinlet>());
	usine.enregistre_type(cree_desc<OpCreationPancarte>());
}

#pragma clang diagnostic pop
