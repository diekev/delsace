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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <iostream>
#include <filesystem>

#include "biblinternes/chrono/chronometre_de_portee.hh"
#include "biblinternes/flux/outils.h"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"
#include "biblinternes/structures/chaine.hh"

namespace filesystem = std::filesystem;

static auto commence_par(dls::chaine const &ligne, dls::chaine const &motif)
{
	if (motif.taille() > ligne.taille()) {
		return false;
	}

	auto index = 0l;

	/* Évite les espaces blancs. */
	while (index < ligne.taille() && (ligne[index] == '\t' || ligne[index] == ' ')) {
		++index;
	}

	return ligne.sous_chaine(index, motif.taille()) == motif;
}

static auto fini_par(dls::chaine const &ligne, dls::chaine const &motif)
{
	if (motif.taille() > ligne.taille()) {
		return false;
	}

	return ligne.trouve(motif) != dls::chaine::npos;

//	auto index = ligne.taille() - 1;

//	/* Évite les espaces blancs. */
//	while (index >= 0 && (ligne[index] == '\t' || ligne[index] == ' ')) {
//		--index;
//	}

//	return ligne.substr((index + 1) - motif.taille(), motif.taille()) == motif;
}

/* Considère les lignes comme étant vides si elles ne contiennent que des
 * caractères blancs ou des séquences de début ou de fin d'instructions comme
 * }); à la fin des lambdas. */
static auto est_ligne_vide(dls::chaine const &ligne)
{
	for (auto c : ligne) {
		if (!dls::outils::est_element(c, ' ', '\t', '\n', '\r', '\v', '{', '}', '(', ')', '[', ']', ';')) {
			return false;
		}
	}

	return true;
}

static auto compte_lignes(std::ifstream &is)
{
	auto nombre_lignes = 0;
	auto nombre_commentaires = 0;
	auto nombre_inutiles = 0;

	auto commentaire_c = false;

	dls::flux::pour_chaque_ligne(is, [&](dls::chaine const &ligne)
	{
		if (ligne.est_vide()) {
			return;
		}

		if (commentaire_c) {
			if (fini_par(ligne, "*/")) {
				commentaire_c = false;
			}

			nombre_commentaires++;
			return;
		}

		if (commence_par(ligne, "//")) {
			nombre_commentaires++;
			return;
		}

		if (commence_par(ligne, "/*")) {
			if (!fini_par(ligne, "*/")) {
				commentaire_c = true;
			}

			nombre_commentaires++;
			return;
		}

		if (est_ligne_vide(ligne)) {
			++nombre_inutiles;
			return;
		}

		nombre_lignes++;
	});

	return std::pair<int, int>(nombre_lignes, nombre_commentaires);
}

enum {
	FICHIER_SOURCE,
	FICHIER_ENTETE,
	FICHIER_INCONNU,
};

static auto est_fichier_entete(filesystem::path const &extension)
{
	return dls::outils::est_element(extension, ".h", ".hh", ".hpp");
}

static auto est_fichier_source(filesystem::path const &extension)
{
	return dls::outils::est_element(extension, ".c", ".cc", ".cpp", ".tcc");
}

static auto type_fichier(filesystem::path const &chemin)
{
	auto const extension = chemin.extension();

	if (est_fichier_entete(extension)) {
		return FICHIER_ENTETE;
	}

	if (est_fichier_source(extension)) {
		return FICHIER_SOURCE;
	}

	return FICHIER_INCONNU;
}

int main()
{
	auto nombre_total_lignes = 0;
	auto nombre_total_lignes_source = 0;
	auto nombre_total_lignes_entete = 0;
	auto nombre_total_commentaires = 0;
	auto nombre_total_commentaires_source = 0;
	auto nombre_total_commentaires_entete = 0;
	auto nombre_fichiers = 0;
	auto nombre_fichiers_entete = 0;
	auto nombre_fichiers_source = 0;

	std::ostream &os = std::cout;

	CHRONOMETRE_PORTEE("Temps d'exécution", os);

	for (const auto &entree : filesystem::recursive_directory_iterator(filesystem::current_path())) {
		const auto chemin_fichier = entree.path();

		if (filesystem::is_directory(chemin_fichier)) {
			continue;
		}

		auto type = type_fichier(chemin_fichier);

		if (type == FICHIER_INCONNU) {
			continue;
		}

		++nombre_fichiers;

		std::ifstream fichier;
		fichier.open(chemin_fichier.c_str());

		if (!fichier.is_open()) {
			continue;
		}

		auto const nombre_lignes = compte_lignes(fichier);
		nombre_total_lignes += nombre_lignes.first;
		nombre_total_commentaires += nombre_lignes.second;

		if (type == FICHIER_ENTETE) {
			nombre_fichiers_entete += 1;
			nombre_total_lignes_entete += nombre_lignes.first;
			nombre_total_commentaires_entete += nombre_lignes.second;
		}
		else if (type == FICHIER_SOURCE) {
			nombre_fichiers_source += 1;
			nombre_total_lignes_source += nombre_lignes.first;
			nombre_total_commentaires_source += nombre_lignes.second;
		}
	}

	auto tableau = Tableau({ "Fichiers", "Nombre", "Lignes", "Lignes/Fichiers", "Sources", "Commentaires", "Sources/Commentaires" });
	tableau.alignement(1, Alignement::DROITE);
	tableau.alignement(2, Alignement::DROITE);
	tableau.alignement(3, Alignement::DROITE);
	tableau.alignement(4, Alignement::DROITE);
	tableau.alignement(5, Alignement::DROITE);
	tableau.alignement(6, Alignement::DROITE);

	auto nombre_lignes_fichiers_sources = nombre_total_lignes_source + nombre_total_commentaires_source;
	auto chn_fichiers = formatte_nombre(nombre_fichiers_source);
	auto chn_lignes = formatte_nombre(nombre_lignes_fichiers_sources);
	auto chn_lignes_fichiers = dls::vers_chaine(static_cast<double>(nombre_lignes_fichiers_sources) / nombre_fichiers_source);
	auto chn_sources = formatte_nombre(nombre_total_lignes_source);
	auto chn_commentaires = formatte_nombre(nombre_total_commentaires_source);
	auto chn_ratio = dls::vers_chaine(static_cast<double>(nombre_total_commentaires_source) / nombre_total_lignes_source);

	tableau.ajoute_ligne({ "Sources", chn_fichiers, chn_lignes, chn_lignes_fichiers, chn_sources, chn_commentaires, chn_ratio });

	auto nombre_lignes_fichiers_entetes = nombre_total_lignes_entete + nombre_total_commentaires_entete;
	chn_fichiers = formatte_nombre(nombre_fichiers_entete);
	chn_lignes = formatte_nombre(nombre_lignes_fichiers_entetes);
	chn_lignes_fichiers = dls::vers_chaine(static_cast<double>(nombre_lignes_fichiers_entetes) / nombre_fichiers_entete);
	chn_sources = formatte_nombre(nombre_total_lignes_entete);
	chn_commentaires = formatte_nombre(nombre_total_commentaires_entete);
	chn_ratio = dls::vers_chaine(static_cast<double>(nombre_total_commentaires_entete) / nombre_total_lignes_entete);

	tableau.ajoute_ligne({ "Entêtes", chn_fichiers, chn_lignes, chn_lignes_fichiers, chn_sources, chn_commentaires, chn_ratio });

	auto nombre_lignes_fichiers = nombre_total_lignes + nombre_total_commentaires;
	chn_fichiers = formatte_nombre(nombre_fichiers);
	chn_lignes = formatte_nombre(nombre_lignes_fichiers);
	chn_lignes_fichiers = dls::vers_chaine(static_cast<double>(nombre_lignes_fichiers) / nombre_fichiers);
	chn_sources = formatte_nombre(nombre_total_lignes);
	chn_commentaires = formatte_nombre(nombre_total_commentaires);
	chn_ratio = dls::vers_chaine(static_cast<double>(nombre_total_commentaires) / nombre_total_lignes);

	tableau.ajoute_ligne({ "Total", chn_fichiers, chn_lignes, chn_lignes_fichiers, chn_sources, chn_commentaires, chn_ratio });

	imprime_tableau(tableau);
}
