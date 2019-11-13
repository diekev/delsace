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
#include "biblinternes/outils/chemin.hh"
#include "biblinternes/patrons_conception/commande.h"

#include "evaluation/evaluation.hh"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/jorjala.hh"

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

static bool ecris_image(
		Jorjala &jorjala,
		Composite *composite,
		dls::chaine const &nom_calque,
		dls::chaine const &chemin,
		int temps)
{
	/* calcul le chemin */
	auto chemin_image = chemin;
	dls::corrige_chemin_pour_ecriture(chemin_image, temps);

	/* récupère les données */
	auto const &image = composite->image();
	auto calque_entree = image.calque_pour_lecture(nom_calque);

	if (calque_entree == nullptr) {
		jorjala.affiche_erreur("Calque introuvable dans l'image du composite.");
		return false;
	}

	auto tampon = extrait_grille_couleur(calque_entree);

	/* écris l'image */
	if (chemin_image.trouve(".exr") != dls::chaine::npos) {
		ParametresImage parametres;
		parametres.composant = 4;
		parametres.hauteur = static_cast<size_t>(tampon->desc().resolution.y);
		parametres.largeur = static_cast<size_t>(tampon->desc().resolution.x);
		auto ptr = static_cast<void const *>(&tampon->valeur(0).r);
		parametres.pointeur = const_cast<void *>(ptr);

		ecris_exr(chemin_image.c_str(), parametres);
	}
	else {
		jorjala.affiche_erreur("L'écriture de fichier autre que EXR n'est pas disponible.");
		return false;
		//dls::image::flux::ecris(chemin_image.c_str(), tampon->tampon);
	}

	return true;
}

/* ************************************************************************** */

class CommandeRenduImage final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);

		if (jorjala->nom_calque_sortie == "") {
			jorjala->affiche_erreur("Le nom du calque de sortie est vide.");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (jorjala->chemin_sortie == "") {
			jorjala->affiche_erreur("Le chemin de sortie est vide.");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &noeud_composite = jorjala->bdd.graphe_composites()->noeud_actif;

		if (noeud_composite == nullptr) {
			jorjala->affiche_erreur("Aucun noeud composite sélectionné");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &composite = extrait_composite(noeud_composite->donnees);

		/* À FAIRE : vérifie l'éligibilité du chemin de sortie, graphe rendu */

		requiers_evaluation(*jorjala, RENDU_REQUIS, "commande rendu image");

		ecris_image(*jorjala,
					composite,
					jorjala->nom_calque_sortie,
					jorjala->chemin_sortie,
					jorjala->temps_courant);

		jorjala->notifie_observatrices(type_evenement::image | type_evenement::traite);

		return EXECUTION_COMMANDE_REUSSIE;
	}
};

/* ************************************************************************** */

class CommandeRenduSequence final : public Commande {
public:
	int execute(std::any const &pointeur, DonneesCommande const &donnees) override
	{
		auto jorjala = extrait_jorjala(pointeur);

		auto const temps_originale = jorjala->temps_courant;

		if (jorjala->nom_calque_sortie == "") {
			jorjala->affiche_erreur("Le nom du calque de sortie est vide.");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		if (jorjala->chemin_sortie == "") {
			jorjala->affiche_erreur("Le chemin de sortie est vide.");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &noeud_composite = jorjala->bdd.graphe_composites()->noeud_actif;

		if (noeud_composite == nullptr) {
			jorjala->affiche_erreur("Aucun noeud composite sélectionné");
			return EXECUTION_COMMANDE_ECHOUEE;
		}

		auto const &composite = extrait_composite(noeud_composite->donnees);

		/* À FAIRE : vérifie l'éligibilité du chemin de sortie. */

		for (int i = jorjala->temps_debut; i <= jorjala->temps_fin; ++i) {
			jorjala->temps_courant = i;

			jorjala->ajourne_pour_nouveau_temps("commande rendu séquence");

			auto ok = ecris_image(
						*jorjala,
						composite,
						jorjala->nom_calque_sortie,
						jorjala->chemin_sortie,
						jorjala->temps_courant);

			if (!ok) {
				break;
			}

			jorjala->notifie_observatrices(
						type_evenement::image | type_evenement::traite);
		}

		jorjala->temps_courant = temps_originale;

		jorjala->ajourne_pour_nouveau_temps("fin commande rendu séquence");

		jorjala->notifie_observatrices(type_evenement::temps | type_evenement::modifie);

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
