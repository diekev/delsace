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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <chrono/outils.hh>

#include "analyseuse_grammaire.h"
#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"
#include "erreur.h"

/* ************************************************************************** */

bool DonneesModule::importe_module(const std::string_view &nom_module) const
{
	for (const auto &mi : modules_importes) {
		if (mi == nom_module) {
			return true;
		}
	}

	return false;
}

bool DonneesModule::possede_fonction(const std::string_view &nom_fonction) const
{
	for (const auto &fi : fonctions_exportees) {
		if (fi == nom_fonction) {
			return true;
		}
	}

	return false;
}

void DonneesModule::ajoute_donnees_fonctions(const std::string_view &nom_fonction, const DonneesFonction &donnees)
{
	fonctions.insert({nom_fonction, donnees});
}

DonneesFonction &DonneesModule::donnees_fonction(const std::string_view &nom_fonction) noexcept
{
	auto iter = fonctions.find(nom_fonction);

	if (iter == fonctions.end()) {
		return m_donnees_invalides;
	}

	return iter->second;
}

bool DonneesModule::fonction_existe(const std::string_view &nom_fonction) const noexcept
{
	return fonctions.find(nom_fonction) != fonctions.end();
}

size_t DonneesModule::memoire_utilisee() const noexcept
{
	auto memoire = fonctions.size() * (sizeof(DonneesFonction) + sizeof(std::string_view));

	for (const auto &fonc : fonctions) {
		memoire += fonc.second.args.size() * (sizeof(DonneesArgument) + sizeof(std::string_view));
	}

	return memoire;
}

/* ************************************************************************** */

static auto charge_fichier(
		const std::string &chemin,
		ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau)
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
	const auto taille_fichier = static_cast<std::string::size_type>(fichier.tellg());
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
		const std::string &nom,
		ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau,
		bool est_racine)
{
	os << "Chargement du module : " << nom << std::endl;

	auto chemin = nom + ".kuri";

	if (!std::filesystem::exists(chemin)) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin)) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un fichier régulier",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* Le module racine n'a pas de nom, afin que les noms de ses fonctions ne
	 * soient pas broyés. */
	auto module = contexte.cree_module(est_racine ? "" : nom);
	auto debut_chargement = dls::chrono::maintenant();
	auto tampon = charge_fichier(chemin, contexte, morceau);
	module->temps_chargement = dls::chrono::delta(debut_chargement);

	auto debut_tampon = dls::chrono::maintenant();
	module->tampon = TamponSource(tampon);
	module->temps_tampon = dls::chrono::delta(debut_tampon);

	auto decoupeuse = decoupeuse_texte(module);
	auto debut_decoupage = dls::chrono::maintenant();
	decoupeuse.genere_morceaux();
	module->temps_decoupage = dls::chrono::delta(debut_decoupage);

	auto analyseuse = analyseuse_grammaire(
						  contexte,
						  module->morceaux,
						  contexte.assembleuse,
						  module);

	auto debut_analyse = dls::chrono::maintenant();
	analyseuse.lance_analyse(os);
	module->temps_analyse = dls::chrono::delta(debut_analyse);
}
