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
#include "biblinternes/structures/chaine.hh"

#include "biblinternes/chrono/outils.hh"

#include "analyseuse_grammaire.hh"
#include "assembleuse_arbre.hh"
#include "contexte_generation_code.hh"
#include "decoupeuse.hh"
#include "erreur.hh"

/* ************************************************************************** */

bool DonneesModule::importe_module(dls::vue_chaine const &nom_module) const
{
	return modules_importes.trouve(nom_module) != modules_importes.fin();
}

bool DonneesModule::possede_fonction(dls::vue_chaine const &nom_fonction) const
{
	return fonctions_exportees.trouve(nom_fonction) != fonctions_exportees.fin();
}

void DonneesModule::ajoute_donnees_fonctions(dls::vue_chaine const &nom_fonction, DonneesFonction const &donnees)
{
	auto iter = fonctions.trouve(nom_fonction);

	if (iter == fonctions.fin()) {
		fonctions.insere({nom_fonction, {donnees}});
	}
	else {
		iter->second.pousse(donnees);
	}
}

dls::tableau<DonneesFonction> &DonneesModule::donnees_fonction(dls::vue_chaine const &nom_fonction) noexcept
{
	auto iter = fonctions.trouve(nom_fonction);

	if (iter == fonctions.fin()) {
		assert(false);
	}

	return iter->second;
}

bool DonneesModule::fonction_existe(dls::vue_chaine const &nom_fonction) const noexcept
{
	return fonctions.trouve(nom_fonction) != fonctions.fin();
}

size_t DonneesModule::memoire_utilisee() const noexcept
{
	auto memoire = static_cast<size_t>(fonctions.taille()) * (sizeof(dls::tableau<DonneesFonction>) + sizeof(dls::vue_chaine));

	for (auto const &df : fonctions) {
		for (auto const &fonc : df.second) {
			memoire += static_cast<size_t>(fonc.args.taille()) * (sizeof(DonneesArgument) + sizeof(dls::vue_chaine));
		}
	}

	return memoire;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		dls::chaine const &chemin,
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
	auto const taille_fichier = fichier.tellg();
	fichier.seekg(0, fichier.beg);

	std::string tampon;
	dls::chaine res;
	res.reserve(taille_fichier);

	while (std::getline(fichier, tampon)) {
		res += tampon;
		res.pousse('\n');
	}

	return res;
}

void charge_module(
		std::ostream &os,
		dls::chaine const &racine_kuri,
		dls::chaine const &nom,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau,
		bool est_racine)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/bibliotheques/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le fichier correspondant au module",
						contexte,
						morceau,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un fichier régulier",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	/* Le module racine n'a pas de nom, afin que les noms de ses fonctions ne
	 * soient pas broyés. */
	auto module = contexte.cree_module(est_racine ? "" : nom, chemin_absolu.c_str());

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

