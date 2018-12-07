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

#include "commandes_rendu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wconversion"
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfOutputFile.h>
#pragma GCC diagnostic pop

#include <numero7/image/flux/ecriture.h>

#include "bibliotheques/commandes/commande.h"

#include "../composite.h"
#include "../evaluation.h"
#include "../evenement.h"
#include "../mikisa.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

struct ParametresImage {
	size_t hauteur;
	size_t largeur;
	unsigned char composant; // 1, 3, 4
//	char profondeur; // 8, 16, 32
	void *pointeur;
};

static void ecris_exr(const char *chemin, const ParametresImage &parametres)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	const auto &hauteur = parametres.hauteur;
	const auto &largeur = parametres.largeur;

	/* À FAIRE : écriture selon le nombre de composant de l'image. */
	openexr::Header en_tete(static_cast<int>(largeur), static_cast<int>(hauteur));
	en_tete.channels().insert("R", openexr::Channel(openexr::FLOAT));
	en_tete.channels().insert("G", openexr::Channel(openexr::FLOAT));
	en_tete.channels().insert("B", openexr::Channel(openexr::FLOAT));
	en_tete.channels().insert("A", openexr::Channel(openexr::FLOAT));

	openexr::OutputFile fichier(chemin, en_tete);

	auto debut_R = static_cast<float *>(parametres.pointeur);
	auto debut_G = static_cast<float *>(parametres.pointeur) + 1;
	auto debut_B = static_cast<float *>(parametres.pointeur) + 2;
	auto debut_A = static_cast<float *>(parametres.pointeur) + 3;

	const auto decalage_x = sizeof(float) * static_cast<size_t>(parametres.composant);
	const auto decalage_y = sizeof(float) * largeur * static_cast<size_t>(parametres.composant);

	openexr::FrameBuffer tampon_image;

	tampon_image.insert("R",
						openexr::Slice(openexr::FLOAT,
									   reinterpret_cast<char *>(debut_R),
									   decalage_x,
									   decalage_y));

	tampon_image.insert("G",
						openexr::Slice(openexr::FLOAT,
									   reinterpret_cast<char *>(debut_G),
									   decalage_x,
									   decalage_y));

	tampon_image.insert("B",
						openexr::Slice(openexr::FLOAT,
									   reinterpret_cast<char *>(debut_B),
									   decalage_x,
									   decalage_y));

	tampon_image.insert("A",
						openexr::Slice(openexr::FLOAT,
									   reinterpret_cast<char *>(debut_A),
									   decalage_x,
									   decalage_y));

	fichier.setFrameBuffer(tampon_image);
	fichier.writePixels(static_cast<int>(hauteur));
}

static void corrige_chemin_pour_ecriture(std::string &chemin, int temps)
{
	const auto pos_debut = chemin.find_first_of('#');

	if (pos_debut == std::string::npos) {
		return;
	}

	auto pos_fin = pos_debut + 1;

	while (chemin[pos_fin] == '#') {
		pos_fin++;
	}

	auto compte = pos_fin - pos_debut;

	auto chaine_nombre = std::to_string(temps);
	chaine_nombre.insert(0, compte - chaine_nombre.size(), '0');

	chemin.replace(pos_debut, chaine_nombre.size(), chaine_nombre);
}

static bool ecris_image(
		Composite *composite,
		const std::string &nom_calque,
		const std::string &chemin,
		int temps)
{
	/* calcul le chemin */
	auto chemin_image = chemin;
	corrige_chemin_pour_ecriture(chemin_image, temps);

	/* récupère les données */
	const auto &image = composite->image();
	auto tampon = image.calque(nom_calque);

	if (tampon == nullptr) {
		/* À FAIRE : erreur. */
		return false;
	}

	/* écris l'image */
	if (chemin_image.find(".exr") != std::string::npos) {
		ParametresImage parametres;
		parametres.composant = 4;
		parametres.hauteur = static_cast<size_t>(tampon->tampon.nombre_lignes());
		parametres.largeur = static_cast<size_t>(tampon->tampon.nombre_colonnes());
		parametres.pointeur = &tampon->tampon[0][0].r;

		ecris_exr(chemin_image.c_str(), parametres);
	}
	else {
		numero7::image::flux::ecris(chemin_image, tampon->tampon);
	}

	return true;
}

/* ************************************************************************** */

class CommandeRenduImage final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		if (mikisa->nom_calque_sortie == "") {
			/* À FAIRE : erreur. */
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (mikisa->chemin_sortie == "") {
			/* À FAIRE : erreur. */
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* À FAIRE : vérifie l'éligibilité du chemin de sortie. */

		evalue_graphe(mikisa);

		ecris_image(mikisa->composite,
					mikisa->nom_calque_sortie,
					mikisa->chemin_sortie,
					mikisa->temps_courant);

		mikisa->notifie_auditeurs(type_evenement::image | type_evenement::traite);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRenduSequence final : public Commande {
public:
	int execute(std::any const &pointeur, const DonneesCommande &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		const auto temps_originale = mikisa->temps_courant;

		if (mikisa->nom_calque_sortie == "") {
			/* À FAIRE : erreur. */
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (mikisa->chemin_sortie == "") {
			/* À FAIRE : erreur. */
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		/* À FAIRE : vérifie l'éligibilité du chemin de sortie. */

		for (int i = mikisa->temps_debut; i <= mikisa->temps_fin; ++i) {
			mikisa->temps_courant = i;

			mikisa->ajourne_pour_nouveau_temps();

			ecris_image(mikisa->composite,
						mikisa->nom_calque_sortie,
						mikisa->chemin_sortie,
						mikisa->temps_courant);

			mikisa->notifie_auditeurs(
						type_evenement::image | type_evenement::traite);
		}

		mikisa->temps_courant = temps_originale;

		mikisa->ajourne_pour_nouveau_temps();

		mikisa->notifie_auditeurs(type_evenement::temps | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_rendu(UsineCommande *usine)
{
	usine->enregistre_type("rendu_image",
						   description_commande<CommandeRenduImage>(
							   "rendu", 0, 0, 0, false));

	usine->enregistre_type("rendu_sequence",
						   description_commande<CommandeRenduSequence>(
							   "rendu", 0, 0, 0, false));
}

#pragma clang diagnostic pop
