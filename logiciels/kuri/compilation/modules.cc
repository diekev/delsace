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

#include "syntaxeuse.hh"
#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "lexeuse.hh"

/* ************************************************************************** */

DonneesFonction::iteratrice_arg DonneesFonction::trouve(const dls::vue_chaine_compacte &nom)
{
	return std::find_if(args.debut(), args.fin(), [&](DonneesArgument const &da)
	{
		return da.nom == nom;
	});
}

/* ************************************************************************** */

Fichier::Fichier()
{
	/* Tous les fichiers importent implicitement Kuri. */
	modules_importes.insere("Kuri");
}

bool Fichier::importe_module(dls::vue_chaine_compacte const &nom_module) const
{
	return modules_importes.trouve(nom_module) != modules_importes.fin();
}

bool DonneesModule::possede_fonction(dls::vue_chaine_compacte const &nom_fonction) const
{
	return fonctions_exportees.trouve(nom_fonction) != fonctions_exportees.fin();
}

void DonneesModule::ajoute_donnees_fonctions(dls::vue_chaine_compacte const &nom_fonction, DonneesFonction const &donnees)
{
	auto iter = fonctions.trouve(nom_fonction);

	if (iter == fonctions.fin()) {
		auto liste = dls::liste<DonneesFonction>();
		liste.pousse(donnees);
		fonctions.insere({nom_fonction, liste});
	}
	else {
		iter->second.pousse(donnees);
	}
}

dls::liste<DonneesFonction> &DonneesModule::donnees_fonction(dls::vue_chaine_compacte const &nom_fonction) noexcept
{
	auto iter = fonctions.trouve(nom_fonction);

	if (iter == fonctions.fin()) {
		assert(false);
	}

	return iter->second;
}

bool DonneesModule::fonction_existe(dls::vue_chaine_compacte const &nom_fonction) const noexcept
{
	return fonctions.trouve(nom_fonction) != fonctions.fin();
}

size_t DonneesModule::memoire_utilisee() const noexcept
{
	auto memoire = static_cast<size_t>(fonctions.taille()) * (sizeof(dls::tableau<DonneesFonction>) + sizeof(dls::vue_chaine_compacte));

	for (auto const &df : fonctions) {
		for (auto const &fonc : df.second) {
			memoire += static_cast<size_t>(fonc.args.taille()) * (sizeof(DonneesArgument) + sizeof(dls::vue_chaine_compacte));
		}
	}

	return memoire;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		ContexteGenerationCode &contexte,
		DonneesLexeme const &lexeme)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					contexte,
					lexeme,
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
		DonneesLexeme const &lexeme)
{
	auto chemin = module->chemin + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					contexte,
					lexeme,
					erreur::type_erreur::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					contexte,
					lexeme,
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

	auto lexeuse = Lexeuse(fichier);
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
		DonneesLexeme const &lexeme)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						contexte,
						lexeme,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					contexte,
					lexeme,
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

/* ************************************************************************** */

static double verifie_compatibilite(
		ContexteGenerationCode &contexte,
		long idx_type_arg,
		long idx_type_enf,
		noeud::base *enfant,
		TransformationType &transformation)
{
	transformation = cherche_transformation(contexte, idx_type_enf, idx_type_arg);

	if (transformation.type == TypeTransformation::INUTILE) {
		return 1.0;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return 0.0;
	}

	if (transformation.type == TypeTransformation::PREND_REFERENCE) {
		return est_valeur_gauche(enfant->genre_valeur) ? 1.0 : 0.0;
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return 0.5;
}

static DonneesCandidate verifie_donnees_fonction(
		ContexteGenerationCode &contexte,
		DonneesFonction &donnees_fonction,
		dls::liste<dls::vue_chaine_compacte> &noms_arguments,
		dls::liste<noeud::base *> const &exprs)
{
	auto res = DonneesCandidate{};

	auto const nombre_args = donnees_fonction.args.taille();

	if (!donnees_fonction.est_variadique && (exprs.taille() > nombre_args)) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = MECOMPTAGE_ARGS;
		res.df = &donnees_fonction;
		return res;
	}

	if (nombre_args == 0 && exprs.taille() == 0) {
		res.poids_args = 1.0;
		res.etat = FONCTION_TROUVEE;
		res.raison = AUCUNE_RAISON;
		res.df = &donnees_fonction;
		return res;
	}

	dls::tableau<noeud::base *> slots(nombre_args - donnees_fonction.est_variadique);

	for (auto i = 0; i < slots.taille(); ++i) {
		slots[i] = donnees_fonction.args[i].expression_defaut;
	}

	auto index = 0l;
	auto iter_expr = exprs.debut();
	auto arguments_nommes = false;
	auto dernier_arg_variadique = false;
	dls::ensemble<dls::vue_chaine_compacte> args;

	for (auto const &nom_arg : noms_arguments) {
		if (nom_arg != "") {
			arguments_nommes = true;

			auto iter = donnees_fonction.trouve(nom_arg);

			if (iter == donnees_fonction.args.fin()) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = nom_arg;
				res.df = &donnees_fonction;
				return res;
			}

			auto &donnees = *iter;

			if ((args.trouve(nom_arg) != args.fin()) && !donnees.est_variadic) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = nom_arg;
				res.df = &donnees_fonction;
				return res;
			}

			dernier_arg_variadique = donnees.est_variadic;

			args.insere(nom_arg);

			auto index_arg = std::distance(donnees_fonction.args.debut(), iter);

			if (dernier_arg_variadique || index_arg >= slots.taille()) {
				slots.pousse(*iter_expr++);
			}
			else {
				slots[index_arg] = *iter_expr++;
			}
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.df = &donnees_fonction;
				return res;
			}

			if (dernier_arg_variadique || index >= slots.taille()) {
				slots.pousse(*iter_expr++);
				index++;
			}
			else {
				slots[index++] = *iter_expr++;
			}
		}
	}

	for (auto slot : slots) {
		if (slot == nullptr) {
			// À FAIRE : on pourrait donner les noms des arguments manquants
			res.etat = FONCTION_INTROUVEE;
			res.raison = MECOMPTAGE_ARGS;
			res.df = &donnees_fonction;
			return res;
		}
	}

	auto paires_expansion_gabarit = dls::tableau<std::pair<dls::vue_chaine_compacte, long>>();

	auto poids_args = 1.0;
	auto fonction_variadique_interne = donnees_fonction.est_variadique
			&& !donnees_fonction.est_externe;
	auto expansion_rencontree = false;

	auto transformations = dls::tableau<TransformationType>(slots.taille());

	auto nombre_arg_variadiques_rencontres = 0;

	for (auto i = 0l; i < slots.taille(); ++i) {
		auto index_arg = std::min(i, donnees_fonction.args.taille() - 1);
		auto &arg = donnees_fonction.args[index_arg];
		auto slot = slots[i];

		if (slot == arg.expression_defaut) {
			continue;
		}

		auto const index_type_enf = slot->index_type;
		auto const &type_enf = contexte.typeuse[index_type_enf];

		auto index_type_arg = arg.index_type;

		if (arg.type_declare.est_gabarit) {
			// trouve l'argument
			auto type_trouve = false;
			auto type_errone = false;
			for (auto &paire : paires_expansion_gabarit) {
				if (paire.first == arg.type_declare.nom_gabarit) {
					type_trouve = true;

					if (paire.second != index_type_enf) {
						type_errone = true;
						// erreur À FAIRE
						poids_args = 0.0;
						res.raison = METYPAGE_ARG;
						//res.type1 = type_enf;
						res.type2 = type_enf;
						res.noeud_decl = slot;
						break;
					}
				}
			}

			if (type_errone) {
				break;
			}

			if (!type_trouve) {
				auto index_type_gabarit = index_type_enf;

				// résoud le type selon la déclaration
				auto type_declare = arg.type_declare.plage();
				auto plg_type_enf = type_enf.plage();

				// permet de trouver les types du style *[]$T
				while (type_declare.front() == (plg_type_enf.front() & 0xff)) {
					plg_type_enf.effronte();
					type_declare.effronte();
				}

				if (type_declare.front() != GenreLexeme::DOLLAR) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					//res.type1 = type_enf;
					res.type2 = type_enf;
					res.noeud_decl = slot;
					res.df = &donnees_fonction;
					return res;
				}

				index_type_gabarit = contexte.typeuse.ajoute_type(plg_type_enf);

				/* reconstruit le type de l'argument dans le cas où nous avons
				 * un tableau fixe devant être converti en un tableau dynamique */
				type_declare = arg.type_declare.plage();
				auto dt_final = DonneesTypeFinal{};

				while (!type_declare.est_finie()) {
					if (type_declare.front() == GenreLexeme::DOLLAR) {
						dt_final.pousse(contexte.typeuse[index_type_gabarit]);
						break;
					}

					dt_final.pousse(type_declare.front());
					type_declare.effronte();
				}

				index_type_arg = contexte.typeuse.ajoute_type(dt_final);

				paires_expansion_gabarit.pousse({ arg.type_declare.nom_gabarit, index_type_gabarit });
			}
			else {
				index_type_arg = index_type_enf;
			}
		}

		auto const &type_arg = (index_type_arg == -1l) ? DonneesTypeFinal{} : contexte.typeuse[index_type_arg];

		if (arg.est_variadic) {
			if (!est_invalide(type_arg.dereference())) {
				auto transformation = TransformationType();
				auto index_type_deref = contexte.typeuse.type_dereference_pour(index_type_arg);
				auto poids_pour_enfant = 0.0;

				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						erreur::lance_erreur("Impossible d'utiliser une expansion variadique dans une fonction variadique externe", contexte, slot->lexeme);
					}

					if (expansion_rencontree) {
						erreur::lance_erreur("Ne peut utiliser qu'une seule expansion d'argument variadique", contexte, slot->lexeme);
					}

					auto index_type_deref_enf = contexte.typeuse.type_dereference_pour(index_type_enf);

					poids_pour_enfant = verifie_compatibilite(contexte, index_type_deref, index_type_deref_enf, slot, transformation);

					// aucune transformation acceptée sauf si nous avons un tableau fixe qu'il faudra convertir en un tableau dynamique
					if (poids_pour_enfant != 1.0) {
						poids_pour_enfant = 0.0;
					}
					else {
						auto &dt_enf = contexte.typeuse[index_type_enf];

						if (est_type_tableau_fixe(dt_enf)) {
							transformation = TypeTransformation::CONVERTI_TABLEAU;
						}
					}

					expansion_rencontree = true;
				}
				else {
					poids_pour_enfant = verifie_compatibilite(contexte, index_type_deref, index_type_enf, slot, transformation);
				}

				// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
				if (arg.type_declare.est_gabarit) {
					poids_pour_enfant *= 0.95;
				}

				poids_args *= poids_pour_enfant;

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type1 = type_arg.dereference();
					res.type2 = type_enf;
					res.noeud_decl = slot;
					break;
				}

				if (fonction_variadique_interne) {
					if (expansion_rencontree && nombre_arg_variadiques_rencontres != 0) {
						if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
							erreur::lance_erreur("Tentative d'utiliser une expansion d'arguments variadiques alors que d'autres arguments ont déjà été précisés", contexte, slot->lexeme);
						}
						else {
							erreur::lance_erreur("Tentative d'ajouter des arguments variadiques supplémentaire alors qu'une expansion est également utilisée", contexte, slot->lexeme);
						}
					}
				}

				transformations[i] = transformation;
			}
			else {
				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						erreur::lance_erreur("Impossible d'utiliser une expansion variadique dans une fonction variadique externe", contexte, slot->lexeme);
					}
				}

				transformations[i] = TransformationType();
			}

			nombre_arg_variadiques_rencontres += 1;
		}
		else {
			auto transformation = TransformationType();

			/* il est possible que le type final ne soit pas encore résolu car
			 * la déclaration de la candidate n'a pas encore été validée */
			if (!est_invalide(type_arg.plage())) {
				std::cerr << "cherche transformation entre "
						  << chaine_type(contexte.typeuse[index_type_arg], contexte) << " et " << chaine_type(contexte.typeuse[index_type_enf], contexte) << '\n';
				auto poids_pour_enfant = verifie_compatibilite(contexte, index_type_arg, index_type_enf, slot, transformation);

				// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
				if (arg.type_declare.est_gabarit) {
					poids_pour_enfant *= 0.95;
				}

				poids_args *= poids_pour_enfant;
			}

			if (poids_args == 0.0) {
				poids_args = 0.0;
				res.raison = METYPAGE_ARG;
				res.type1 = type_arg;
				res.type2 = type_enf;
				res.noeud_decl = slot;
				break;
			}

			transformations[i] = transformation;
		}
	}

	if (fonction_variadique_interne) {
		auto index_premier_var_arg = nombre_args - 1;

		if (slots.taille() != nombre_args || slots[index_premier_var_arg]->genre != GenreNoeud::EXPANSION_VARIADIQUE) {
			/* Pour les fonctions variadiques interne, nous créons un tableau
			 * correspondant au types des arguments. */
			auto noeud_tableau = contexte.assembleuse->cree_noeud(
						GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES, exprs.front()->lexeme);
			noeud_tableau->valeur_calculee = slots.taille() - index_premier_var_arg;
			noeud_tableau->drapeaux |= EST_CALCULE;

			auto index_dt_var = donnees_fonction.args.back().index_type;
			auto &dt_var = contexte.typeuse[index_dt_var];
			noeud_tableau->index_type = contexte.typeuse.ajoute_type(dt_var.dereference());

			for (auto i = index_premier_var_arg; i < slots.taille(); ++i) {
				noeud_tableau->ajoute_noeud(slots[i]);
			}

			if (index_premier_var_arg >= slots.taille()) {
				slots.pousse(noeud_tableau);
			}
			else {
				slots[index_premier_var_arg] = noeud_tableau;
			}

			slots.redimensionne(nombre_args);
		}
	}

	res.df = &donnees_fonction;
	res.poids_args = poids_args;
	res.exprs = slots;
	res.etat = FONCTION_TROUVEE;
	res.transformations = transformations;
	res.paires_expansion_gabarit = paires_expansion_gabarit;

	return res;
}

ResultatRecherche cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom,
		dls::liste<dls::vue_chaine_compacte> &noms_arguments,
		dls::liste<noeud::base *> const &exprs,
		size_t index_fichier,
		size_t index_fichier_appel)
{
	auto res = ResultatRecherche{};

	if (index_fichier != index_fichier_appel) {
		/* l'appel est qualifié (À FAIRE, méthode plus robuste) */
		auto fichier = contexte.fichier(index_fichier_appel);
		auto module = fichier->module;

		if (!module->possede_fonction(nom)) {
			return {};
		}

		auto &vdf = module->donnees_fonction(nom);

		for (auto &df : vdf) {
			auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
			res.candidates.pousse(dc);
		}

		return res;
	}

	auto fichier = contexte.fichier(index_fichier);
	auto module = fichier->module;

	if (module->possede_fonction(nom)) {
		auto &vdf = module->donnees_fonction(nom);

		for (auto &df : vdf) {
			auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
			res.candidates.pousse(dc);
		}
	}

	/* cherche dans les modules importés */
	for (auto &nom_module : fichier->modules_importes) {
		module = contexte.module(nom_module);

		if (module->possede_fonction(nom)) {
			auto &vdf = module->donnees_fonction(nom);

			for (auto &df : vdf) {
				auto dc = verifie_donnees_fonction(contexte, df, noms_arguments, exprs);
				res.candidates.pousse(dc);
			}
		}
	}

	return res;
}

PositionMorceau trouve_position(const DonneesLexeme &lexeme, Fichier *fichier)
{
	auto ptr = lexeme.chaine.pointeur();
	auto pos = PositionMorceau{};

	for (auto i = 0ul; i < fichier->tampon.nombre_lignes() - 1; ++i) {
		auto l0 = fichier->tampon[static_cast<long>(i)];
		auto l1 = fichier->tampon[static_cast<long>(i + 1)];

		if (ptr >= l0.begin() && ptr < l1.begin()) {
			pos.index_ligne = static_cast<long>(i);
			pos.numero_ligne = pos.index_ligne + 1;
			pos.pos = ptr - l0.begin();
			break;
		}
	}

	return pos;
}
