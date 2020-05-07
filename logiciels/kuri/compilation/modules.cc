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

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/flux/outils.h"
#include "biblinternes/outils/conditions.h"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "lexeuse.hh"
#include "portee.hh"
#include "syntaxeuse.hh"

/* ************************************************************************** */

bool Fichier::importe_module(dls::vue_chaine_compacte const &nom_module) const
{
	return modules_importes.possede(nom_module);
}

DonneesModule::DonneesModule(ContexteGenerationCode &contexte)
	: assembleuse(memoire::loge<assembleuse_arbre>("assembleuse_arbre", contexte))
	, bloc(assembleuse->bloc_courant())
{
	assert(bloc != nullptr);
}

DonneesModule::~DonneesModule()
{
	memoire::deloge("assembleuse_arbre", assembleuse);
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		ContexteGenerationCode &contexte,
		Lexeme const &lexeme)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					contexte,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = fichier.tellg();
	fichier.seekg(0, fichier.beg);

	dls::chaine res;
	res.reserve(taille_fichier);

	dls::flux::pour_chaque_ligne(fichier, [&](dls::chaine const &ligne)
	{
		res += ligne;
		res.pousse('\n');
	});

	return res;
}

void charge_fichier(
		std::ostream &os,
		DonneesModule *module,
		dls::chaine const &racine_kuri,
		dls::chaine const &nom,
		ContexteGenerationCode &contexte,
		Lexeme const &lexeme)
{
	auto chemin = module->chemin + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					contexte,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					contexte,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du fichier */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto fichier = contexte.cree_fichier(nom.c_str(), chemin_absolu.c_str());

	if (fichier == nullptr) {
		/* le fichier a déjà été chargé */
		return;
	}

	os << "Chargement du fichier : " << chemin << std::endl;

	fichier->module = module;

	auto debut_chargement = dls::chrono::compte_seconde();
	auto tampon = charge_fichier(chemin, contexte, lexeme);
	fichier->temps_chargement = debut_chargement.temps();

	auto debut_tampon = dls::chrono::compte_seconde();
	fichier->tampon = lng::tampon_source(tampon);
	fichier->temps_tampon = debut_tampon.temps();

	auto lexeuse = Lexeuse(contexte, fichier);
	auto debut_decoupage = dls::chrono::compte_seconde();
	lexeuse.performe_lexage();
	fichier->temps_decoupage = debut_decoupage.temps();

	auto analyseuse = Syntaxeuse(
						  contexte,
						  fichier,
						  racine_kuri);

	analyseuse.lance_analyse(os);
}

void importe_module(
		std::ostream &os,
		dls::chaine const &racine_kuri,
		dls::chaine const &nom,
		ContexteGenerationCode &contexte,
		Lexeme const &lexeme)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						contexte,
						&lexeme,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					contexte,
					&lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());
	auto module = contexte.cree_module(nom.c_str(), chemin_absolu.c_str());

	if (module->importe) {
		return;
	}

	module->importe = true;

	os << "Importation du module : " << nom << " (" << chemin_absolu << ")" << std::endl;

	for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
		auto chemin_entree = entree.path();

		if (!std::filesystem::is_regular_file(chemin_entree)) {
			continue;
		}

		if (chemin_entree.extension() != ".kuri") {
			continue;
		}

		charge_fichier(os, module, racine_kuri, chemin_entree.stem().c_str(), contexte, {});
	}
}

PositionLexeme position_lexeme(Lexeme const &lexeme)
{
	auto pos = PositionLexeme{};
	pos.pos = lexeme.colonne;
	pos.numero_ligne = lexeme.ligne + 1;
	pos.index_ligne = lexeme.ligne;
	return pos;
}

NoeudDeclarationFonction *cherche_fonction_dans_module(
		ContexteGenerationCode &contexte,
		DonneesModule *module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto ident = contexte.table_identifiants.identifiant_pour_chaine(nom_fonction);
	auto decl = trouve_dans_bloc(module->bloc, ident);

	return static_cast<NoeudDeclarationFonction *>(decl);
}

NoeudDeclarationFonction *cherche_fonction_dans_module(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto module = contexte.module(nom_module);
	return cherche_fonction_dans_module(contexte, module, nom_fonction);
}

NoeudDeclarationFonction *cherche_symbole_dans_module(
		ContexteGenerationCode &contexte,
		DonneesModule *module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto ident = contexte.table_identifiants.identifiant_pour_chaine(nom_fonction);
	auto decl = trouve_dans_bloc(module->bloc, ident);

	return static_cast<NoeudDeclarationFonction *>(decl);
}

NoeudDeclarationFonction *cherche_symbole_dans_module(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto module = contexte.module(nom_module);
	return cherche_fonction_dans_module(contexte, module, nom_fonction);
}

void imprime_fichier_ligne(ContexteGenerationCode &contexte, const Lexeme &lexeme)
{
	auto fichier = contexte.fichier(static_cast<size_t>(lexeme.fichier));
	std::cerr << fichier->chemin << ':' << lexeme.ligne + 1 << '\n';
}
