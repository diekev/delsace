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

static auto charge_fichier(
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
	auto chemin = nom + ".kuri";

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

	auto analyseuse = analyseuse_grammaire(
						  contexte,
						  module,
						  racine_kuri);

	analyseuse.lance_analyse(os);
}

/* ************************************************************************** */
static double verifie_compatibilite(
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		noeud::base *enfant,
		niveau_compat &drapeau)
{
	drapeau = sont_compatibles(type_arg, type_enf, enfant->type);

	if (drapeau == niveau_compat::aucune) {
		return 0.0;
	}

	if ((drapeau & niveau_compat::converti_tableau) != niveau_compat::aucune) {
		return 0.5;
	}

	if ((drapeau & niveau_compat::converti_eini) != niveau_compat::aucune) {
		return 0.5;
	}

	if ((drapeau & niveau_compat::extrait_chaine_c) != niveau_compat::aucune) {
		return 0.5;
	}

	if ((drapeau & niveau_compat::converti_tableau_octet) != niveau_compat::aucune) {
		return 0.5;
	}

	return 1.0;
}

static DonneesCandidate verifie_donnees_fonction(
		ContexteGenerationCode &contexte,
		DonneesFonction &donnees_fonction,
		std::list<std::string_view> &noms_arguments_,
		std::list<noeud::base *> const &exprs)
{
	auto res = DonneesCandidate{};

	auto const nombre_args = donnees_fonction.args.size();

	if (!donnees_fonction.est_variadique && (exprs.size() != nombre_args)) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = MECOMPTAGE_ARGS;
		res.df = &donnees_fonction;
		return res;
	}

	if (nombre_args == 0) {
		res.poids_args = 1.0;
		res.etat = FONCTION_TROUVEE;
		res.raison = AUCUNE_RAISON;
		res.df = &donnees_fonction;
		return res;
	}

	/* ***************** vérifie si les noms correspondent ****************** */

	auto arguments_nommes = false;
	std::set<std::string_view> args;
	auto dernier_arg_variadique = false;

	/* crée une copie pour ne pas polluer la liste pour les appels suivants */
	auto noms_arguments = noms_arguments_;
	auto index = 0ul;
	auto const index_max = nombre_args - donnees_fonction.est_variadique;
	for (auto &nom_arg : noms_arguments) {
		if (nom_arg != "") {
			arguments_nommes = true;

			auto iter = donnees_fonction.args.find(nom_arg);

			if (iter == donnees_fonction.args.end()) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = nom_arg;
				res.df = &donnees_fonction;
				return res;
			}

			auto &donnees = iter->second;

			if ((args.find(nom_arg) != args.end()) && !donnees.est_variadic) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = nom_arg;
				res.df = &donnees_fonction;
				return res;
			}

#ifdef NONSUR
			auto &dt = contexte.magasin_types.donnees_types[donnees.donnees_type];

			if (dt.type_base() == id_morceau::POINTEUR && !contexte.non_sur()) {
				res.arg_pointeur = true;
			}
#endif

			dernier_arg_variadique = iter->second.est_variadic;

			args.insert(nom_arg);
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.df = &donnees_fonction;
				return res;
			}

			if (nombre_args != 0) {
				auto nom_argument = donnees_fonction.nom_args[index];
				args.insert(nom_argument);
				nom_arg = nom_argument;

#ifdef NONSUR
				/* À FAIRE : meilleur stockage, ceci est redondant */
				auto iter = donnees_fonction.args.find(nom_argument);
				auto &donnees = iter->second;

				/* il est possible que le type soit non-spécifié (variadic) */
				if (donnees.donnees_type != -1ul) {
					auto &dt = contexte.magasin_types.donnees_types[donnees.donnees_type];

					if (dt.type_base() == id_morceau::POINTEUR && !contexte.non_sur()) {
						res.arg_pointeur = true;
					}
				}
#endif
			}
		}

		index = std::min(index + 1, index_max);
	}

	/* ********************** réordonne selon les noms ********************** */
	/* ***************** vérifie si les types correspondent ***************** */

	auto poids_args = 1.0;
	auto fonction_variadique_interne = donnees_fonction.est_variadique
			&& !donnees_fonction.est_externe;

	/* Réordonne les enfants selon l'apparition des arguments car LLVM est
	 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
	 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
	 * code est généré. */
	std::vector<noeud::base *> enfants;

	if (fonction_variadique_interne) {
		enfants.resize(donnees_fonction.args.size());
	}
	else {
		enfants.resize(noms_arguments.size());
	}

	auto enfant = exprs.begin();
	auto noeud_tableau = static_cast<noeud::base *>(nullptr);

	if (fonction_variadique_interne) {
		/* Pour les fonctions variadiques interne, nous créons un tableau
		 * correspondant au types des arguments. */

		auto nombre_args_var = std::max(0ul, noms_arguments.size() - (nombre_args - 1));
		auto index_premier_var_arg = nombre_args - 1;

		noeud_tableau = contexte.assembleuse->cree_noeud(
					type_noeud::TABLEAU, contexte, (*enfant)->morceau);
		noeud_tableau->valeur_calculee = static_cast<long>(nombre_args_var);
		noeud_tableau->drapeaux |= EST_CALCULE;
		auto nom_arg = donnees_fonction.nom_args.back();

		auto index_dt_var = donnees_fonction.args[nom_arg].donnees_type;
		auto &dt_var = contexte.magasin_types.donnees_types[index_dt_var];
		noeud_tableau->index_type = contexte.magasin_types.ajoute_type(dt_var.derefence());

		enfants[index_premier_var_arg] = noeud_tableau;
	}

	res.raison = AUCUNE_RAISON;

	auto nombre_arg_variadic = 0ul;
	auto nombre_arg_variadic_drapeau = 0ul;

	std::vector<niveau_compat> drapeaux;
	drapeaux.resize(exprs.size());

	for (auto const &nom : noms_arguments) {
		/* Pas la peine de vérifier qu'iter n'est pas égal à la fin de la table
		 * car ça a déjà été fait plus haut. */
		auto const iter = donnees_fonction.args.find(nom);
		auto index_arg = iter->second.index;
		auto const index_type_arg = iter->second.donnees_type;
		auto const index_type_enf = (*enfant)->index_type;
		auto const &type_arg = index_type_arg == -1ul ? DonneesType{} : contexte.magasin_types.donnees_types[index_type_arg];
		auto const &type_enf = contexte.magasin_types.donnees_types[index_type_enf];

		/* À FAIRE : arguments variadics : comment les passer d'une
		 * fonction à une autre. */
		if (iter->second.est_variadic) {
			if (!type_arg.derefence().est_invalide()) {
				auto drapeau = niveau_compat::ok;
				poids_args *= verifie_compatibilite(type_arg.derefence(), type_enf, *enfant, drapeau);

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type1 = type_arg.derefence();
					res.type2 = type_enf;
					res.noeud_decl = *enfant;
					break;
				}

				if (noeud_tableau) {
					noeud_tableau->ajoute_noeud(*enfant);
					drapeaux[index_arg + nombre_arg_variadic_drapeau] = drapeau;
					++nombre_arg_variadic_drapeau;
				}
				else {
					enfants[index_arg + nombre_arg_variadic] = *enfant;
					drapeaux[index_arg + nombre_arg_variadic_drapeau] = drapeau;
					++nombre_arg_variadic;
					++nombre_arg_variadic_drapeau;
				}
			}
			else {
				enfants[index_arg + nombre_arg_variadic] = *enfant;
				drapeaux[index_arg + nombre_arg_variadic_drapeau] = niveau_compat::ok;
				++nombre_arg_variadic;
				++nombre_arg_variadic_drapeau;
			}
		}
		else {
			auto drapeau = niveau_compat::ok;
			poids_args *= verifie_compatibilite(type_arg, type_enf, *enfant, drapeau);

			if (poids_args == 0.0) {
				poids_args = 0.0;
				res.raison = METYPAGE_ARG;
				res.type1 = type_arg;
				res.type2 = type_enf;
				res.noeud_decl = *enfant;
				break;
			}

			enfants[index_arg] = *enfant;
			drapeaux[index_arg] = drapeau;
		}

		++enfant;
	}

	res.df = &donnees_fonction;
	res.poids_args = poids_args;
	res.exprs = enfants;
	res.etat = FONCTION_TROUVEE;
	res.drapeaux = drapeaux;

	return res;
}

ResultatRecherche cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		std::string_view const &nom,
		std::list<std::string_view> &noms_arguments,
		std::list<noeud::base *> const &exprs,
		size_t index_module,
		size_t index_module_appel)
{
	auto res = ResultatRecherche{};

	if (index_module != index_module_appel) {
		/* l'appel est qualifié (À FAIRE, méthode plus robuste) */
		auto module = contexte.module(index_module_appel);

		if (!module->possede_fonction(nom)) {
			return {};
		}

		auto &vdf = module->donnees_fonction(nom);

		for (auto &df : vdf) {
			auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
			res.candidates.push_back(dc);
		}

		return res;
	}

	auto module = contexte.module(index_module);

	if (module->possede_fonction(nom)) {
		auto &vdf = module->donnees_fonction(nom);

		for (auto &df : vdf) {
			auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
			res.candidates.push_back(dc);
		}
	}

	/* cherche dans les modules importés */
	for (auto &nom_module : module->modules_importes) {
		module = contexte.module(nom_module);

		if (module->possede_fonction(nom)) {
			auto &vdf = module->donnees_fonction(nom);

			for (auto &df : vdf) {
				auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
				res.candidates.push_back(dc);
			}
		}
	}

	return res;
}
