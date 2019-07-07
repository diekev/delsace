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

#include "biblinternes/image/flux/ecriture.h"

#include "biblinternes/commandes/commande.h"

#include "../evaluation/evaluation.hh"

#include "../composite.h"
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

static void ecris_exr(const char *chemin, ParametresImage const &parametres)
{
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	auto const &hauteur = parametres.hauteur;
	auto const &largeur = parametres.largeur;

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

	auto const decalage_x = sizeof(float) * static_cast<size_t>(parametres.composant);
	auto const decalage_y = sizeof(float) * largeur * static_cast<size_t>(parametres.composant);

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

static void corrige_chemin_pour_ecriture(dls::chaine &chemin, int temps)
{
	auto const pos_debut = chemin.trouve_premier_de('#');

	if (pos_debut == dls::chaine::npos) {
		return;
	}

	auto pos_fin = pos_debut + 1;

	while (chemin[pos_fin] == '#') {
		pos_fin++;
	}

	auto compte = pos_fin - pos_debut;

	auto chaine_nombre = dls::chaine(std::to_string(temps));
	chaine_nombre.insere(0, compte - chaine_nombre.taille(), '0');

	chemin.remplace(pos_debut, chaine_nombre.taille(), chaine_nombre);
}

static bool ecris_image(
		Composite *composite,
		dls::chaine const &nom_calque,
		dls::chaine const &chemin,
		int temps)
{
	/* calcul le chemin */
	auto chemin_image = chemin;
	corrige_chemin_pour_ecriture(chemin_image, temps);

	/* récupère les données */
	auto const &image = composite->image();
	auto tampon = image.calque(nom_calque);

	if (tampon == nullptr) {
		/* À FAIRE : erreur. */
		return false;
	}

	/* écris l'image */
	if (chemin_image.trouve(".exr") != dls::chaine::npos) {
		ParametresImage parametres;
		parametres.composant = 4;
		parametres.hauteur = static_cast<size_t>(tampon->tampon.nombre_lignes());
		parametres.largeur = static_cast<size_t>(tampon->tampon.nombre_colonnes());
		parametres.pointeur = &tampon->tampon[0][0].r;

		ecris_exr(chemin_image.c_str(), parametres);
	}
	else {
		dls::image::flux::ecris(chemin_image.c_str(), tampon->tampon);
	}

	return true;
}

/* ************************************************************************** */

class CommandeRenduImage final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
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

		/* À FAIRE : vérifie l'éligibilité du chemin de sortie, graphe rendu */

		requiers_evaluation(*mikisa, RENDU_REQUIS, "commande rendu image");

		ecris_image(mikisa->composite,
					mikisa->nom_calque_sortie,
					mikisa->chemin_sortie,
					mikisa->temps_courant);

		mikisa->notifie_observatrices(type_evenement::image | type_evenement::traite);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRenduSequence final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto mikisa = std::any_cast<Mikisa *>(pointeur);

		auto const temps_originale = mikisa->temps_courant;

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

			mikisa->ajourne_pour_nouveau_temps("commande rendu séquence");

			ecris_image(mikisa->composite,
						mikisa->nom_calque_sortie,
						mikisa->chemin_sortie,
						mikisa->temps_courant);

			mikisa->notifie_observatrices(
						type_evenement::image | type_evenement::traite);
		}

		mikisa->temps_courant = temps_originale;

		mikisa->ajourne_pour_nouveau_temps("fin commande rendu séquence");

		mikisa->notifie_observatrices(type_evenement::temps | type_evenement::modifie);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_commandes_rendu(UsineCommande &usine)
{
	usine.enregistre_type("rendu_image",
						   description_commande<CommandeRenduImage>(
							   "rendu", 0, 0, 0, false));

	usine.enregistre_type("rendu_sequence",
						   description_commande<CommandeRenduSequence>(
							   "rendu", 0, 0, 0, false));
}

#pragma clang diagnostic pop
