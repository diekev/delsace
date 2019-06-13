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

#include "modules.hh"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "biblinternes/chrono/outils.hh"

#include "analyseuse_grammaire.hh"
#include "assembleuse_arbre.hh"
#include "contexte_generation_code.hh"
#include "decoupeuse.hh"
#include "erreur.hh"

/* ************************************************************************** */

bool DonneesModule::importe_module(std::string_view const &nom_module) const
{
	return modules_importes.find(nom_module) != modules_importes.end();
}

bool DonneesModule::possede_fonction(std::string_view const &nom_fonction) const
{
	return fonctions_exportees.find(nom_fonction) != fonctions_exportees.end();
}

void DonneesModule::ajoute_donnees_fonctions(std::string_view const &nom_fonction, DonneesFonction const &donnees)
{
	auto iter = fonctions.find(nom_fonction);

	if (iter == fonctions.end()) {
		fonctions.insert({nom_fonction, {donnees}});
	}
	else {
		iter->second.push_back(donnees);
	}
}

std::vector<DonneesFonction> &DonneesModule::donnees_fonction(std::string_view const &nom_fonction) noexcept
{
	auto iter = fonctions.find(nom_fonction);

	if (iter == fonctions.end()) {
		assert(false);
	}

	return iter->second;
}

bool DonneesModule::fonction_existe(std::string_view const &nom_fonction) const noexcept
{
	return fonctions.find(nom_fonction) != fonctions.end();
}

size_t DonneesModule::memoire_utilisee() const noexcept
{
	auto memoire = fonctions.size() * (sizeof(std::vector<DonneesFonction>) + sizeof(std::string_view));

	for (auto const &df : fonctions) {
		for (auto const &fonc : df.second) {
			memoire += fonc.args.size() * (sizeof(DonneesArgument) + sizeof(std::string_view));
		}
	}

	return memoire;
}

/* ************************************************************************** */

std::string charge_fichier(
		std::string const &chemin,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = static_cast<std::string::size_type>(fichier.tellg());
	fichier.seekg(0, fichier.beg);

	std::string tampon;
	std::string res;
	res.reserve(taille_fichier);

	while (std::getline(fichier, tampon)) {
		res += tampon;
		res.append(1, '\n');
	}

	return res;
}

void charge_module(
		std::ostream &os,
		std::string const &racine_kuri,
		std::string const &nom,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau,
		bool est_racine)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin)) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/bibliotheques/" + chemin;

		if (!std::filesystem::exists(chemin)) {
			erreur::lance_erreur(
						"Impossible de trouver le fichier correspondant au module",
						contexte,
						morceau,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_regular_file(chemin)) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un fichier régulier",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module */
	auto chemin_absolu = std::filesystem::absolute(chemin);

	/* Le module racine n'a pas de nom, afin que les noms de ses fonctions ne
	 * soient pas broyés. */
	auto module = contexte.cree_module(est_racine ? "" : nom, chemin_absolu);

	if (module == nullptr) {
		/* le module a déjà été chargé */
		return;
	}

	os << "Chargement du module : " << nom << " (" << chemin_absolu << ")" << std::endl;

	auto debut_chargement = dls::chrono::maintenant();
	auto tampon = charge_fichier(chemin, contexte, morceau);
	module->temps_chargement = dls::chrono::delta(debut_chargement);

	auto debut_tampon = dls::chrono::maintenant();
	module->tampon = lng::tampon_source(tampon);
	module->temps_tampon = dls::chrono::delta(debut_tampon);

	auto decoupeuse = decoupeuse_texte(module);
	auto debut_decoupage = dls::chrono::maintenant();
	decoupeuse.genere_morceaux();
	module->temps_decoupage = dls::chrono::delta(debut_decoupage);

	decoupeuse.imprime_morceaux(std::cerr);

	auto analyseuse = analyseuse_grammaire(
						  contexte,
						  module,
						  racine_kuri);

	analyseuse.lance_analyse(os);
}

/* ************************************************************************** */

