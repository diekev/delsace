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

Fichier::Fichier()
{
	/* Tous les fichiers importent implicitement Kuri. */
	modules_importes.insere("Kuri");
}

bool Fichier::importe_module(dls::vue_chaine_compacte const &nom_module) const
{
	return modules_importes.trouve(nom_module) != modules_importes.fin();
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
		DonneesLexeme const &lexeme)
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
		DonneesLexeme const &lexeme)
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

/* ************************************************************************** */

static double verifie_compatibilite(
		Type *type_arg,
		Type *type_enf,
		NoeudBase *enfant,
		TransformationType &transformation)
{
	transformation = cherche_transformation(type_enf, type_arg);

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

static Type *apparie_type_gabarit(Type *type, DonneesTypeDeclare const &type_declare)
{
	auto type_courant = type;

	for (auto i = 0; i < type_declare.taille(); ++i) {
		auto lexeme = type_declare[i];

		if (lexeme == GenreLexeme::DOLLAR) {
			break;
		}

		if (lexeme == GenreLexeme::POINTEUR) {
			if (type_courant->genre != GenreType::POINTEUR) {
				return nullptr;
			}

			type_courant = static_cast<TypePointeur *>(type_courant)->type_pointe;
		}
		else if (lexeme == GenreLexeme::REFERENCE) {
			if (type_courant->genre != GenreType::REFERENCE) {
				return nullptr;
			}

			type_courant = static_cast<TypeReference *>(type_courant)->type_pointe;
		}
		else if (lexeme == GenreLexeme::TABLEAU) {
			if (type_courant->genre != GenreType::TABLEAU_DYNAMIQUE) {
				return nullptr;
			}

			type_courant = static_cast<TypeTableauDynamique *>(type_courant)->type_pointe;
		}
		// À FAIRE : type tableau fixe
		else {
			return nullptr;
		}
	}

	return type_courant;
}

static DonneesCandidate verifie_donnees_fonction(
		ContexteGenerationCode &contexte,
		NoeudDeclarationFonction *decl,
		kuri::tableau<IdentifiantCode *> &noms_arguments,
		kuri::tableau<NoeudExpression *> const &exprs)
{
	auto res = DonneesCandidate{};

	auto const nombre_args = decl->params.taille;

	if (!decl->est_variadique && (exprs.taille > nombre_args)) {
		res.etat = FONCTION_INTROUVEE;
		res.raison = MECOMPTAGE_ARGS;
		res.decl_fonc = decl;
		return res;
	}

	if (nombre_args == 0 && exprs.taille == 0) {
		res.poids_args = 1.0;
		res.etat = FONCTION_TROUVEE;
		res.raison = AUCUNE_RAISON;
		res.decl_fonc = decl;
		return res;
	}

	dls::tablet<NoeudExpression *, 10> slots;
	slots.redimensionne(nombre_args - decl->est_variadique);

	for (auto i = 0; i < slots.taille(); ++i) {
		auto param = static_cast<NoeudDeclarationVariable *>(decl->params[i]);
		slots[i] = param->expression;
	}

	auto index = 0l;
	auto iter_expr = exprs.begin();
	auto arguments_nommes = false;
	auto dernier_arg_variadique = false;
	dls::ensemble<IdentifiantCode *> args;

	for (auto const &nom_arg : noms_arguments) {
		if (nom_arg != nullptr) {
			arguments_nommes = true;

			auto param = static_cast<NoeudDeclaration *>(nullptr);
			auto index_param = 0l;

			for (auto i = 0; i < decl->params.taille; ++i) {
				auto it = decl->params[i];

				if (it->ident == nom_arg) {
					param = it;
					index_param = i;
					break;
				}
			}

			if (param == nullptr) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MENOMMAGE_ARG;
				res.nom_arg = nom_arg->nom;
				res.decl_fonc = decl;
				return res;
			}

			if ((args.trouve(nom_arg) != args.fin()) && (param->drapeaux & EST_VARIADIQUE) == 0) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = RENOMMAGE_ARG;
				res.nom_arg = nom_arg->nom;
				res.decl_fonc = decl;
				return res;
			}

			dernier_arg_variadique = (param->drapeaux & EST_VARIADIQUE) != 0;

			args.insere(nom_arg);

			if (dernier_arg_variadique || index_param >= slots.taille()) {
				slots.pousse(*iter_expr++);
			}
			else {
				slots[index_param] = *iter_expr++;
			}
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.decl_fonc = decl;
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
			res.decl_fonc = decl;
			return res;
		}
	}

	auto paires_expansion_gabarit = dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>>();

	auto poids_args = 1.0;
	auto fonction_variadique_interne = decl->est_variadique && !decl->est_externe;
	auto expansion_rencontree = false;

	auto transformations = dls::tableau<TransformationType>(slots.taille());

	auto nombre_arg_variadiques_rencontres = 0;

	for (auto i = 0l; i < slots.taille(); ++i) {
		auto index_arg = std::min(i, decl->params.taille - 1);
		auto param = static_cast<NoeudDeclarationVariable *>(decl->params[index_arg]);
		auto arg = param->valeur;
		auto slot = slots[i];

		if (slot == param->expression) {
			continue;
		}

		auto type_enf = slot->type;
		auto type_arg = arg->type;

		if (arg->type_declare.est_gabarit) {
			// trouve l'argument
			auto type_trouve = false;
			auto type_errone = false;
			for (auto &paire : paires_expansion_gabarit) {
				if (paire.first == arg->type_declare.nom_gabarit) {
					type_trouve = true;
					break;
				}
			}

			if (type_errone) {
				break;
			}

			if (!type_trouve) {
				auto type_gabarit = apparie_type_gabarit(type_enf, arg->type_declare);

				if (type_gabarit == nullptr) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					//res.type1 = type_enf;
					res.type2 = type_enf;
					res.noeud_decl = slot;
					res.decl_fonc = decl;
					return res;
				}

				paires_expansion_gabarit.pousse({ arg->type_declare.nom_gabarit, type_gabarit });
			}

			type_arg = type_enf;
		}

		if ((param->drapeaux & EST_VARIADIQUE) != 0) {
			if (contexte.typeuse.type_dereference_pour(type_arg) != nullptr) {
				auto transformation = TransformationType();
				auto type_deref = contexte.typeuse.type_dereference_pour(type_arg);
				auto poids_pour_enfant = 0.0;

				if (slot->genre == GenreNoeud::EXPANSION_VARIADIQUE) {
					if (!fonction_variadique_interne) {
						erreur::lance_erreur("Impossible d'utiliser une expansion variadique dans une fonction variadique externe", contexte, slot->lexeme);
					}

					if (expansion_rencontree) {
						erreur::lance_erreur("Ne peut utiliser qu'une seule expansion d'argument variadique", contexte, slot->lexeme);
					}

					auto type_deref_enf = contexte.typeuse.type_dereference_pour(type_enf);

					poids_pour_enfant = verifie_compatibilite(type_deref, type_deref_enf, slot, transformation);

					// aucune transformation acceptée sauf si nous avons un tableau fixe qu'il faudra convertir en un tableau dynamique
					if (poids_pour_enfant != 1.0) {
						poids_pour_enfant = 0.0;
					}
					else {
						if (type_enf->genre == GenreType::TABLEAU_FIXE) {
							transformation = TypeTransformation::CONVERTI_TABLEAU;
						}
					}

					expansion_rencontree = true;
				}
				else {
					poids_pour_enfant = verifie_compatibilite(type_deref, type_enf, slot, transformation);
				}

				// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
				if (arg->type_declare.est_gabarit) {
					poids_pour_enfant *= 0.95;
				}

				poids_args *= poids_pour_enfant;

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type1 = contexte.typeuse.type_dereference_pour(type_arg);
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
			auto poids_pour_enfant = verifie_compatibilite(type_arg, type_enf, slot, transformation);

			// À FAIRE: trouve une manière de trouver les fonctions gabarits déjà instantiées
			if (arg->type_declare.est_gabarit) {
				poids_pour_enfant *= 0.95;
			}

			poids_args *= poids_pour_enfant;

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
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(contexte.assembleuse->cree_noeud(
						GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES, exprs[0]->lexeme));
			noeud_tableau->valeur_calculee = slots.taille() - index_premier_var_arg;
			noeud_tableau->drapeaux |= EST_CALCULE;

			auto type_var = decl->params[decl->params.taille - 1]->type;
			noeud_tableau->type = contexte.typeuse.type_dereference_pour(type_var);
			noeud_tableau->exprs.reserve(slots.taille() - index_premier_var_arg);

			for (auto i = index_premier_var_arg; i < slots.taille(); ++i) {
				noeud_tableau->exprs.pousse(slots[i]);
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

	res.decl_fonc = decl;
	res.poids_args = poids_args;
	res.exprs = slots;
	res.etat = FONCTION_TROUVEE;
	res.transformations = transformations;
	res.paires_expansion_gabarit = paires_expansion_gabarit;

	return res;
}

static void trouve_fonctions_dans_module(
		ContexteGenerationCode &contexte,
		DonneesModule *module,
		IdentifiantCode *nom,
		kuri::tableau<IdentifiantCode *> &noms_arguments,
		kuri::tableau<NoeudExpression *> const &exprs,
		ResultatRecherche &res)
{
	assert(module);
	assert(module->bloc);

	POUR (module->bloc->expressions) {
		if (it->genre != GenreNoeud::DECLARATION_FONCTION) {
			continue;
		}

		if (it->ident != nom) {
			continue;
		}

		auto decl_fonc = static_cast<NoeudDeclarationFonction *>(it);

		auto dc = verifie_donnees_fonction(contexte, decl_fonc, noms_arguments, exprs);
		res.candidates.pousse(dc);
	}
}

ResultatRecherche cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		IdentifiantCode *nom,
		kuri::tableau<IdentifiantCode *> &noms_arguments,
		kuri::tableau<NoeudExpression *> const &exprs,
		size_t index_fichier,
		size_t index_fichier_appel)
{
	auto res = ResultatRecherche{};

	if (index_fichier != index_fichier_appel) {
		/* l'appel est qualifié (À FAIRE, méthode plus robuste) */
		auto fichier = contexte.fichier(index_fichier_appel);
		auto module = fichier->module;

		trouve_fonctions_dans_module(contexte, module, nom, noms_arguments, exprs, res);

		return res;
	}

	auto fichier = contexte.fichier(index_fichier);
	auto module = fichier->module;

	trouve_fonctions_dans_module(contexte, module, nom, noms_arguments, exprs, res);

	/* cherche dans les modules importés */
	for (auto &nom_module : fichier->modules_importes) {
		module = contexte.module(nom_module);

		trouve_fonctions_dans_module(contexte, module, nom, noms_arguments, exprs, res);
	}

	return res;
}

PositionLexeme position_lexeme(DonneesLexeme const &lexeme)
{
	auto pos = PositionLexeme{};
	pos.pos = lexeme.colonne;
	pos.numero_ligne = lexeme.ligne + 1;
	pos.index_ligne = lexeme.ligne;
	return pos;
}

NoeudDeclarationFonction *cherche_fonction_dans_module(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction)
{
	auto module = contexte.module(nom_module);
	auto ident = contexte.table_identifiants.identifiant_pour_chaine(nom_fonction);
	auto decl = trouve_dans_bloc(module->bloc, ident);

	return static_cast<NoeudDeclarationFonction *>(decl);
}
