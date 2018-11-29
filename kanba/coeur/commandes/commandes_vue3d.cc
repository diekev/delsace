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

#include "commandes_vue3d.h"

#include <iostream>
#include <QKeyEvent>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/objets/creation.h"
#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/vision/camera.h"

#include "adaptrice_creation_maillage.h"

#include "../brosse.h"
#include "../evenement.h"
#include "../kanba.h"
#include "../maillage.h"
#include "../melange.h"

/* ************************************************************************** */

class CommandeZoomCamera : public Commande {
public:
	CommandeZoomCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);
		const auto delta = donnees.x;

		auto camera = kanba->camera;

		if (delta >= 0) {
			auto distance = camera->distance() + camera->vitesse_zoom();
			camera->distance(distance);
		}
		else {
			const float temp = camera->distance() - camera->vitesse_zoom();
			auto distance = std::max(0.0f, temp);
			camera->distance(distance);
		}

		camera->ajuste_vitesse();
		camera->besoin_ajournement(true);

		kanba->notifie_auditeurs(static_cast<type_evenement>(-1));

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeTourneCamera : public Commande {
	float m_vieil_x;
	float m_vieil_y;

public:
	CommandeTourneCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);
		auto camera = kanba->camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		camera->tete(camera->tete() + dy * camera->vitesse_chute());
		camera->inclinaison(camera->inclinaison() + dx * camera->vitesse_chute());
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		kanba->notifie_auditeurs(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

class CommandePanCamera : public Commande {
	float m_vieil_x;
	float m_vieil_y;

public:
	CommandePanCamera() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		INUTILISE(pointeur);
		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;
		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);
		auto camera = kanba->camera;

		const float dx = (donnees.x - m_vieil_x);
		const float dy = (donnees.y - m_vieil_y);

		auto cible = (dy * camera->haut() - dx * camera->droite()) * camera->vitesse_laterale();
		camera->cible(camera->cible() + cible);
		camera->besoin_ajournement(true);

		m_vieil_x = donnees.x;
		m_vieil_y = donnees.y;

		kanba->notifie_auditeurs(static_cast<type_evenement>(-1));
	}
};

/* ************************************************************************** */

static constexpr auto MIN_SEAUX = 4;
static constexpr auto MAX_SEAUX = 256;

template <typename T>
auto restreint(T a, T min, T max)
{
	if (a < min) {
		return min;
	}

	if (a > max) {
		return max;
	}

	return a;
}

struct TexelProjete {
	/* La position du texel sur l'écran. */
	dls::math::point2f pos;

	/* L'index du polygone possédant le texel. */
	size_t index;

	/* La position u du texel. */
	int u;

	/* La position v du texel. */
	int v;
};

struct Seau {
	std::list<TexelProjete> texels = std::list<TexelProjete>{};
	dls::math::vec2f min = dls::math::vec2f(0.0);
	dls::math::vec2f max = dls::math::vec2f(0.0);

	Seau() = default;
};

Seau *cherche_seau(std::vector<Seau> &seaux, const dls::math::point2f &pos, int seaux_x, int seaux_y, int largeur, int hauteur)
{
	auto x = pos.x / largeur;
	auto y = pos.y / hauteur;

	auto sx = seaux_x * x;
	auto sy = seaux_y * y;

	auto index = static_cast<size_t>(sx + sy * seaux_y);

	index = restreint(index, 0ul, seaux.size() - 1);

	return &seaux[sx + sy * seaux_y];
}

class CommandePeinture3D : public Commande {
public:
	CommandePeinture3D() = default;

	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);

		if (kanba->maillage == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto maillage = kanba->maillage;
		auto calque = maillage->calque_actif();

		if (calque == nullptr) {
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto camera = kanba->camera;
		auto brosse = kanba->brosse;

		const auto &diametre_brosse = brosse->rayon * 2;

		auto seaux_x = camera->largeur() / diametre_brosse + 1;
		auto seaux_y = camera->hauteur() / diametre_brosse + 1;

		seaux_x = restreint(seaux_x, MIN_SEAUX, MAX_SEAUX);
		seaux_y = restreint(seaux_y, MIN_SEAUX, MAX_SEAUX);

		//		std::cerr << "Il y a " << seaux_x << "x" << seaux_y << " seaux en tout\n";
		//		std::cerr << "Taille écran " << camera->largeur() << "x" << camera->hauteur() << "\n";
		//		std::cerr << "Taille seaux " << seaux_x * diametre_brosse << "x" << seaux_y * diametre_brosse << "\n";

		std::vector<Seau> seaux(seaux_x * seaux_y);

		for (auto &seau : seaux) {
			seau = Seau();
		}

		auto nombre_polys = maillage->nombre_polygones();

		const auto &dir = dls::math::vec3f(
							  -camera->dir().x,
							  -camera->dir().y,
							  -camera->dir().z);

		for (size_t i = 0; i < nombre_polys; ++i) {
			auto poly = maillage->polygone(i);
			const auto &angle = produit_scalaire(poly->nor, dir);

			//std::cerr << "Angle : " << angle << '\n';

			if (angle <= 0.0f || angle >= 1.0f) {
				//std::cerr << "Le polygone " << poly->index << " ne fait pas face à l'écran !\n";
				continue;
			}

			//std::cerr << "Le polygone " << poly->index << " fait face à l'écran !\n";

			// projette texel sur l'écran
			const auto &v0 = poly->s[0]->pos;

#if 1
			const auto &v1 = poly->s[1]->pos;
			const auto &v3 = ((poly->s[3] != nullptr) ? poly->s[3]->pos : poly->s[2]->pos);

			const auto &e1 = v1 - v0;
			const auto &e2 = v3 - v0;

			const auto &du = e1 / static_cast<float>(poly->res_u);
			const auto &dv = e2 / static_cast<float>(poly->res_v);
#else
			const auto &du = poly->du;
			const auto &dv = poly->dv;
#endif

			for (int j = 0; j < poly->res_u; ++j) {
				for (int k = 0; k < poly->res_v; ++k) {
					const auto &pos3d = v0 + static_cast<float>(j) * du + static_cast<float>(k) * dv;

					// calcul position 2D du texel
					const auto &pos2d = camera->pos_ecran(dls::math::point3f(pos3d));

					// cherche seau
					auto seau = cherche_seau(seaux, pos2d, seaux_x, seaux_y, camera->largeur(), camera->hauteur());

					TexelProjete texel;
					texel.pos = pos2d;
					texel.index = i;
					texel.u = j;
					texel.v = k;

					seau->texels.push_back(texel);
				}
			}
		}

		auto pos_brosse = dls::math::point2f(donnees.x, donnees.y);

		auto tampon = static_cast<dls::math::vec4f *>(calque->tampon);

		const auto &rayon_inverse = 1.0f / brosse->rayon;

		for (const auto &seau : seaux) {
			if (seau.texels.empty()) {
				continue;
			}

			//std::cerr << "Il y a " << seau.texels.size() << " texels dans le seau !\n";

			for (const auto &texel : seau.texels) {
				auto dist = longueur(texel.pos - pos_brosse);

				if (dist > brosse->rayon) {
					continue;
				}

				auto opacite = dist * rayon_inverse;
				opacite = 1.0f - opacite * opacite;

				auto poly = maillage->polygone(texel.index);
				auto tampon_poly = tampon + (poly->x + poly->y * maillage->largeur_texture());
				auto index = texel.v + texel.u * maillage->largeur_texture();

				tampon_poly[index] = melange(tampon_poly[index],
											 brosse->couleur,
											 opacite * brosse->opacite,
											 brosse->mode_fusion);
			}
		}

		fusionne_calques(maillage->canaux_texture());

		maillage->marque_texture_surrannee(true);
		kanba->notifie_auditeurs(type_evenement::dessin | type_evenement::fini);

		return EXECUTION_COMMANDE_MODALE;
	}

	void ajourne_execution_modale(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		execute(pointeur, donnees);
	}
};

/* ************************************************************************** */

class CommandeAjouteCube : public Commande {
public:
	CommandeAjouteCube() = default;

	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);

		if (kanba->maillage) {
			delete kanba->maillage;
		}

		auto adaptrice = AdaptriceCreationMaillage();
		adaptrice.maillage = new Maillage;

		objets::cree_boite(&adaptrice, 1.0f, 1.0f, 1.0f);

		kanba->maillage = adaptrice.maillage;
		kanba->maillage->cree_tampon();

		kanba->notifie_auditeurs(type_evenement::calque | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeAjouteSphere : public Commande {
public:
	CommandeAjouteSphere() = default;

	int execute(std::any const &pointeur, const DonneesCommande &/*donnees*/) override
	{
		auto kanba = std::any_cast<Kanba *>(pointeur);

		if (kanba->maillage) {
			delete kanba->maillage;
		}

		auto adaptrice = AdaptriceCreationMaillage();
		adaptrice.maillage = new Maillage;

		objets::cree_sphere_uv(&adaptrice, 1.0f);

		kanba->maillage = adaptrice.maillage;
		kanba->maillage->cree_tampon();

		kanba->notifie_auditeurs(type_evenement::calque | type_evenement::ajoute);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_vue3d(UsineCommande *usine)
{
	usine->enregistre_type("commande_zoom_camera",
						   description_commande<CommandeZoomCamera>(
							   "vue_3d", Qt::MiddleButton, 0, 0, true));

	usine->enregistre_type("commande_tourne_camera",
						   description_commande<CommandeTourneCamera>(
							   "vue_3d", Qt::MiddleButton, 0, 0, false));

	usine->enregistre_type("commande_pan_camera",
						   description_commande<CommandePanCamera>(
							   "vue_3d", Qt::MiddleButton, Qt::ShiftModifier, 0, false));

	usine->enregistre_type("commande_peinture_3D",
						   description_commande<CommandePeinture3D>(
							   "vue_3d", Qt::LeftButton, 0, 0, false));

	usine->enregistre_type("ajouter_cube",
						   description_commande<CommandeAjouteCube>(
							   "vue_3d", 0, 0, 0, false));

	usine->enregistre_type("ajouter_sphere",
						   description_commande<CommandeAjouteSphere>(
							   "vue_3d", 0, 0, 0, false));
}
