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

#include "operatrices_rendu.hh"

#include "biblinternes/outils/gna.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/base_de_donnees.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/donnees_aval.hh"
#include "coeur/noeud.hh"
#include "coeur/objet.h"
#include "coeur/operatrice_graphe_detail.hh"
#include "coeur/operatrice_image.h"
#include "coeur/nuanceur.hh"
#include "coeur/usine_operatrice.h"

#include "rendu/moteur_rendu.hh"
#include "rendu/moteur_rendu_cycles.hh"
#include "rendu/moteur_rendu_koudou.hh"
#include "rendu/moteur_rendu_opengl.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static inline auto extrait_moteur_rendu(DonneesAval *da)
{
	return std::any_cast<MoteurRendu *>(da->table["moteur_rendu"]);
}

/* ************************************************************************** */

class OpRenduChercheObjets : public OperatriceImage {
public:
	static constexpr auto NOM = "Cherche Objets";
	static constexpr auto AIDE = "";

	OpRenduChercheObjets(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
		m_execute_toujours = true;
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
		if (!donnees_aval || !donnees_aval->possede("moteur_rendu")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return res_exec::ECHOUEE;
		}

		auto moteur_rendu = extrait_moteur_rendu(donnees_aval);
		auto delegue = moteur_rendu->delegue();

		delegue->objets.efface();

		for (auto objet : contexte.bdd->objets()) {
			delegue->objets.ajoute({objet, {}});
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Instançage (possibilité d'avoir une seule représentation d'un objet en mémoire et de le rendre plusieurs fois)
 * (Arnold) pouvoir redéfinir les attributs non-géométrique pour chaque instance (UV, couleurs, nuanceurs, etc.)
 * (Arnold) pouvoir avoir une instance d'un objet, au lieu de multiples (il suffit d'un seul point)
 * (Houdini) l'instance se fait dans les objets avec un objet instance spéciale
 * (Blender) l'instance se fait soit via particules, soit via un parentage sur les points, arêtes, ou primitives
 * (Général) avoir un index par instance
 */
class OpRenduInstance : public OperatriceImage {
public:
	static constexpr auto NOM = "Instance Objet";
	static constexpr auto AIDE = "";

	OpRenduInstance(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(1);
		m_execute_toujours = true;
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_rendu_instance.jo"};
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
		if (!donnees_aval || !donnees_aval->possede("moteur_rendu")) {
			this->ajoute_avertissement("Il n'y a pas de simulation de gaz en aval.");
			return res_exec::ECHOUEE;
		}

		entree(0)->requiers_image(contexte, donnees_aval);

		auto moteur_rendu = extrait_moteur_rendu(donnees_aval);
		auto delegue = moteur_rendu->delegue();

		auto nom_objet = evalue_chaine("nom_objet");

		if (nom_objet == "") {
			this->ajoute_avertissement("Le nom de l'objet à instancier est vide");
			return res_exec::ECHOUEE;
		}

		auto nom_points = evalue_chaine("points");

		if (nom_points == "") {
			this->ajoute_avertissement("Le nom de l'objet de points où instancier est vide");
			return res_exec::ECHOUEE;
		}

		auto objet_instance = static_cast<ObjetRendu *>(nullptr);
		auto points_instance = static_cast<ObjetRendu *>(nullptr);

		for (auto &objet : delegue->objets) {
			if (objet.objet->noeud->nom == nom_objet) {
				objet_instance = &objet;
			}
			else if (objet.objet->noeud->nom == nom_points) {
				points_instance = &objet;
			}

			if (objet_instance != nullptr && points_instance != nullptr) {
				break;
			}
		}

		if (objet_instance == nullptr) {
			this->ajoute_avertissement("Impossible de trouver l'objet à instancier");
			return res_exec::ECHOUEE;
		}

		if (points_instance == nullptr) {
			this->ajoute_avertissement("Impossible de trouver l'objet de points où instancier");
			return res_exec::ECHOUEE;
		}

		points_instance->objet->donnees.accede_lecture([&](DonneesObjet const *donnees)
		{
			if (points_instance->objet->type != type_objet::CORPS) {
				return;
			}

			auto gna = GNA();

			auto rot = evalue_vecteur("rotation", contexte.temps_courant);
			auto ech = evalue_vecteur("échelle", contexte.temps_courant);
			ech *= evalue_decimal("échelle_uniforme", contexte.temps_courant);
			auto rot_alea = evalue_vecteur("rotation_aléatoire", contexte.temps_courant);
			auto ech_alea = evalue_vecteur("échelle_aléatoire", contexte.temps_courant);

			auto const &corps = extrait_corps(donnees);

			auto points = corps.points_pour_lecture();
			auto nombre_points = points.taille();
			objet_instance->matrices.redimensionne(nombre_points);

			for (int i = 0; i < nombre_points; ++i) {
				auto pnt = points.point_monde(i);

				auto rot_locale = rot;
				auto ech_locale = ech;

				rot_locale.x += gna.uniforme(0.0f, 360.0f) * rot_alea.x;
				rot_locale.y += gna.uniforme(0.0f, 360.0f) * rot_alea.y;
				rot_locale.z += gna.uniforme(0.0f, 360.0f) * rot_alea.z;

				ech_locale.x *= 1.0f + gna.uniforme(0.0f, 1.0f) * ech_alea.x;
				ech_locale.y *= 1.0f + gna.uniforme(0.0f, 1.0f) * ech_alea.y;
				ech_locale.z *= 1.0f + gna.uniforme(0.0f, 1.0f) * ech_alea.z;

				auto transforme = math::construit_transformation(
							pnt, rot_locale, ech_locale);

				objet_instance->matrices[i] = math::matf_depuis_matd(transforme.matrice());
			}
		});

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpMoteurRendu : public OperatriceImage {
	MoteurRendu *m_moteur_rendu = nullptr;

public:
	static constexpr auto NOM = "Moteur Rendu";
	static constexpr auto AIDE = "";

	OpMoteurRendu(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);
	}

	COPIE_CONSTRUCT(OpMoteurRendu);

	~OpMoteurRendu() override
	{
		reinitialise();
	}

	void reinitialise()
	{
		if (m_moteur_rendu == nullptr) {
			return;
		}

		if (m_moteur_rendu->id() == dls::chaine("opengl")) {
			auto moteur_rendu = dynamic_cast<MoteurRenduOpenGL *>(m_moteur_rendu);
			memoire::deloge("MoteurRenduOpenGL", moteur_rendu);
		}
		else if (m_moteur_rendu->id() == dls::chaine("koudou")) {
			auto moteur_rendu = dynamic_cast<MoteurRenduKoudou *>(m_moteur_rendu);
			memoire::deloge("MoteurRenduKoudou", moteur_rendu);
		}
		else if (m_moteur_rendu->id() == dls::chaine("cycles")) {
			auto moteur_rendu = dynamic_cast<MoteurRenduCycles *>(m_moteur_rendu);
			memoire::deloge("MoteurRenduCycles", moteur_rendu);
		}
	}

	ResultatCheminEntreface chemin_entreface() const override
	{
		return CheminFichier{"entreface/operatrice_rendu_moteur.jo"};
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

		auto id_moteur = evalue_enum("id_moteur");

		if (m_moteur_rendu == nullptr || m_moteur_rendu->id() != id_moteur) {
			reinitialise();

			if (id_moteur == "opengl") {
				m_moteur_rendu = memoire::loge<MoteurRenduOpenGL>("MoteurRenduOpenGL");
			}
			else if (id_moteur == "koudou") {
				m_moteur_rendu = memoire::loge<MoteurRenduKoudou>("MoteurRenduKoudou");
			}
			else if (id_moteur == "cycles") {
				m_moteur_rendu = memoire::loge<MoteurRenduCycles>("MoteurRenduCycles");
			}
		}

		for (auto nuanceur : contexte.bdd->nuanceurs()) {
			if (nuanceur->temps_modifie > nuanceur->temps_compilation_glsl) {
				compile_nuanceur_opengl(contexte, *nuanceur);
				nuanceur->temps_compilation_glsl = nuanceur->temps_modifie;
			}
		}

		auto da = DonneesAval{};
		da.table.insere({ "moteur_rendu", m_moteur_rendu });

		entree(0)->requiers_image(contexte, &da);

		auto tampon = contexte.tampon_rendu;
		auto camera = contexte.camera_rendu;
		auto &stats = contexte.stats_rendu;

		m_moteur_rendu->camera(camera);

		m_moteur_rendu->calcule_rendu(
					*stats,
					tampon,
					camera->hauteur(),
					camera->largeur(),
					contexte.rendu_final);

		return res_exec::REUSSIE;
	}

	void performe_versionnage() override
	{
		if (propriete("id_moteur") == nullptr) {
			ajoute_propriete("id_moteur", danjo::TypePropriete::ENUM, dls::chaine("opengl"));
		}
	}
};

/* ************************************************************************** */

void enregistre_operatrices_rendu(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpRenduChercheObjets>());
	usine.enregistre_type(cree_desc<OpRenduInstance>());
	usine.enregistre_type(cree_desc<OpMoteurRendu>());
}

#pragma clang diagnostic pop
