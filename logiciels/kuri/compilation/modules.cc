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

#include "analyseuse_grammaire.h"
#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "decoupeuse.h"

/* ************************************************************************** */

DonneesFonction::iteratrice_arg DonneesFonction::trouve(const dls::vue_chaine_compacte &nom)
{
	return std::find_if(args.debut(), args.fin(), [&](DonneesArgument const &da)
	{
		return da.nom == nom;
	});
}

/* ************************************************************************** */

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
		fonctions.insere({nom_fonction, {donnees}});
	}
	else {
		iter->second.pousse(donnees);
	}
}

dls::tableau<DonneesFonction> &DonneesModule::donnees_fonction(dls::vue_chaine_compacte const &nom_fonction) noexcept
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
		DonneesMorceau const &morceau)
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
		DonneesMorceau const &morceau)
{
	auto chemin = module->chemin + nom + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					contexte,
					morceau,
					erreur::type_erreur::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					contexte,
					morceau,
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
	auto tampon = charge_fichier(chemin, contexte, morceau);
	fichier->temps_chargement = debut_chargement.temps();

	auto debut_tampon = dls::chrono::compte_seconde();
	fichier->tampon = lng::tampon_source(tampon);
	fichier->temps_tampon = debut_tampon.temps();

	auto decoupeuse = decoupeuse_texte(fichier);
	auto debut_decoupage = dls::chrono::compte_seconde();
	decoupeuse.genere_morceaux();
	fichier->temps_decoupage = debut_decoupage.temps();

	auto analyseuse = analyseuse_grammaire(
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
		DonneesMorceau const &morceau)
{
	auto chemin = nom;

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = racine_kuri + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						contexte,
						morceau,
						erreur::type_erreur::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					contexte,
					morceau,
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
		const DonneesTypeFinal &type_arg,
		const DonneesTypeFinal &type_enf,
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

	if ((drapeau & niveau_compat::prend_reference) != niveau_compat::aucune) {
		/* À FAIRE : ceci est pour différencier les valeurs gauches des valeurs
		 * droites (littérales), il manque d'autres cas... */
		if (dls::outils::est_element(enfant->type, type_noeud::VARIABLE, type_noeud::ACCES_MEMBRE_DE, type_noeud::ACCES_MEMBRE_POINT)) {
			return 1.0;
		}

		return 0.0;
	}

	return 1.0;
}

static DonneesCandidate verifie_donnees_fonction(
		ContexteGenerationCode &contexte,
		DonneesFonction &donnees_fonction,
		dls::liste<dls::vue_chaine_compacte> &noms_arguments_,
		dls::liste<noeud::base *> const &exprs)
{
	auto res = DonneesCandidate{};

	auto const nombre_args = donnees_fonction.args.taille();

	if (!donnees_fonction.est_variadique && (exprs.taille() != nombre_args)) {
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
	dls::ensemble<dls::vue_chaine_compacte> args;
	auto dernier_arg_variadique = false;

	/* crée une copie pour ne pas polluer la liste pour les appels suivants */
	auto noms_arguments = noms_arguments_;
	auto index = 0l;
	auto const index_max = nombre_args - donnees_fonction.est_variadique;
	for (auto &nom_arg : noms_arguments) {
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

#ifdef NONSUR
			auto &dt = contexte.magasin_types.donnees_types[donnees.donnees_type];

			if (dt.type_base() == id_morceau::POINTEUR && !contexte.non_sur()) {
				res.arg_pointeur = true;
			}
#endif

			dernier_arg_variadique = donnees.est_variadic;

			args.insere(nom_arg);
		}
		else {
			if (arguments_nommes == true && dernier_arg_variadique == false) {
				res.etat = FONCTION_INTROUVEE;
				res.raison = MANQUE_NOM_APRES_VARIADIC;
				res.df = &donnees_fonction;
				return res;
			}

			if (nombre_args != 0) {
				auto nom_argument = donnees_fonction.args[index].nom;
				args.insere(nom_argument);
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
	dls::tableau<noeud::base *> enfants;

	if (fonction_variadique_interne) {
		enfants.redimensionne(donnees_fonction.args.taille());
	}
	else {
		enfants.redimensionne(noms_arguments.taille());
	}

	auto enfant = exprs.debut();
	auto noeud_tableau = static_cast<noeud::base *>(nullptr);

	if (fonction_variadique_interne) {
		/* Pour les fonctions variadiques interne, nous créons un tableau
		 * correspondant au types des arguments. */

		auto nombre_args_var = std::max(0l, noms_arguments.taille() - (nombre_args - 1));
		auto index_premier_var_arg = nombre_args - 1;

		noeud_tableau = contexte.assembleuse->cree_noeud(
					type_noeud::TABLEAU, contexte, (*enfant)->morceau);
		noeud_tableau->valeur_calculee = nombre_args_var;
		noeud_tableau->drapeaux |= EST_CALCULE;

		auto index_dt_var = donnees_fonction.args.back().index_type;
		auto &dt_var = contexte.magasin_types.donnees_types[index_dt_var];
		noeud_tableau->index_type = contexte.magasin_types.ajoute_type(dt_var.dereference());

		enfants[index_premier_var_arg] = noeud_tableau;
	}

	res.raison = AUCUNE_RAISON;

	auto nombre_arg_variadic = 0l;
	auto nombre_arg_variadic_drapeau = 0l;

	dls::tableau<niveau_compat> drapeaux;
	drapeaux.redimensionne(exprs.taille());

	for (auto const &nom : noms_arguments) {
		/* Pas la peine de vérifier qu'iter n'est pas égal à la fin de la table
		 * car ça a déjà été fait plus haut. */
		auto const iter = donnees_fonction.trouve(nom);
		auto index_arg = std::distance(donnees_fonction.args.debut(), iter);
		auto const index_type_arg = iter->index_type;
		auto const index_type_enf = (*enfant)->index_type;
		auto const &type_arg = (index_type_arg == -1l) ? DonneesTypeFinal{} : contexte.magasin_types.donnees_types[index_type_arg];
		auto const &type_enf = contexte.magasin_types.donnees_types[index_type_enf];

		/* À FAIRE : arguments variadics : comment les passer d'une
		 * fonction à une autre. */
		if (iter->est_variadic) {
			if (!est_invalide(type_arg.dereference())) {
				auto drapeau = niveau_compat::ok;
				poids_args *= verifie_compatibilite(type_arg.dereference(), type_enf, *enfant, drapeau);

				if (poids_args == 0.0) {
					poids_args = 0.0;
					res.raison = METYPAGE_ARG;
					res.type1 = type_arg.dereference();
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

			/* il est possible que le type final ne soit pas encore résolu car
			 * la déclaration de la candidate n'a pas encore été validée */
			if (!est_invalide(type_arg.plage())) {
				poids_args *= verifie_compatibilite(type_arg, type_enf, *enfant, drapeau);
			}

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

PositionMorceau trouve_position(const DonneesMorceau &morceau, Fichier *fichier)
{
	auto ptr = morceau.chaine.pointeur();
	auto pos = PositionMorceau{};

	for (auto i = 0ul; i < fichier->tampon.nombre_lignes() - 1; ++i) {
		auto l0 = fichier->tampon[static_cast<long>(i)];
		auto l1 = fichier->tampon[static_cast<long>(i + 1)];

		if (ptr >= l0.begin() && ptr < l1.begin()) {
			pos.ligne = static_cast<long>(i);
			pos.pos = ptr - l0.begin();
			break;
		}
	}

	return pos;
}
