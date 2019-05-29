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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "validation_semantique.hh"

#include <cassert>
#include <sstream>

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "donnees_type.h"
#include "erreur.h"

namespace noeud {

/* ************************************************************************** */

static void drapeau_depuis_niveau_compat(
		base *enfant,
		niveau_compat compat)
{
	if ((compat & niveau_compat::converti_tableau) != niveau_compat::aucune) {
		enfant->drapeaux |= CONVERTI_TABLEAU;
	}

	if ((compat & niveau_compat::converti_eini) != niveau_compat::aucune) {
		enfant->drapeaux |= CONVERTI_EINI;
	}

	if ((compat & niveau_compat::extrait_chaine_c) != niveau_compat::aucune) {
		enfant->drapeaux |= EXTRAIT_CHAINE_C;
	}

	if ((compat & niveau_compat::converti_tableau_octet) != niveau_compat::aucune) {
		enfant->drapeaux |= CONVERTI_TABLEAU_OCTET;
	}

	if ((compat & niveau_compat::prend_reference) != niveau_compat::aucune) {
		enfant->drapeaux |= PREND_REFERENCE;
	}
}

static void verifie_compatibilite(
		base *b,
		ContexteGenerationCode &contexte,
		const DonneesType &type_arg,
		const DonneesType &type_enf,
		base *enfant)
{
	auto compat = sont_compatibles(type_arg, type_enf, enfant->type);

	if (compat == niveau_compat::aucune) {
		erreur::lance_erreur_type_arguments(
					type_arg,
					type_enf,
					contexte,
					enfant->donnees_morceau(),
					b->morceau);
	}

	if (compat == niveau_compat::prend_reference) {
		if (enfant->type != type_noeud::VARIABLE) {
			erreur::lance_erreur(
						"Ne peut pas prendre la référence d'une valeur n'étant pas une variable",
						contexte,
						enfant->morceau,
						erreur::type_erreur::TYPE_DIFFERENTS);
		}
	}

	drapeau_depuis_niveau_compat(enfant, compat);
}

bool peut_operer(
		const DonneesType &type1,
		const DonneesType &type2,
		type_noeud type_gauche,
		type_noeud type_droite)
{
	/* À FAIRE : cas spécial pour les énums */
	if (type1.type_base() == type2.type_base()) {
		return true;
	}

	if (type1.type_base() == id_morceau::FONC) {
		/* x : fonc()rien = nul; */
		if (type2.type_base() == id_morceau::POINTEUR && type2.derefence().type_base() == id_morceau::NUL) {
			return true;
		}
	}

	if (est_type_entier(type1.type_base())) {
		if (est_type_entier(type2.type_base())) {
			return true;
		}

		if (type_droite == type_noeud::NOMBRE_ENTIER) {
			return true;
		}

		return false;
	}

	if (est_type_entier(type2.type_base())) {
		if (est_type_entier(type1.type_base())) {
			return true;
		}

		if (type_gauche == type_noeud::NOMBRE_ENTIER) {
			return true;
		}

		return false;
	}

	if (est_type_reel(type1.type_base())) {
		if (est_type_reel(type2.type_base())) {
			return true;
		}

		if (type_droite == type_noeud::NOMBRE_REEL) {
			return true;
		}

		return false;
	}

	if (est_type_reel(type2.type_base())) {
		if (est_type_reel(type1.type_base())) {
			return true;
		}

		if (type_gauche == type_noeud::NOMBRE_REEL) {
			return true;
		}

		return false;
	}

	return false;
}

/* ************************************************************************** */

bool est_constant(base *b)
{
	switch (b->type) {
		default:
		case type_noeud::TABLEAU:
		{
			return false;
		}
		case type_noeud::BOOLEEN:
		case type_noeud::CARACTERE:
		case type_noeud::NOMBRE_ENTIER:
		case type_noeud::NOMBRE_REEL:
		case type_noeud::CHAINE_LITTERALE:
		{
			return true;
		}
	}
}

static bool est_assignation_operee(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::MOINS_EGAL:
		case id_morceau::PLUS_EGAL:
		case id_morceau::MULTIPLIE_EGAL:
		case id_morceau::DIVISE_EGAL:
		case id_morceau::MODULO_EGAL:
		case id_morceau::ET_EGAL:
		case id_morceau::OU_EGAL:
		case id_morceau::OUX_EGAL:
		case id_morceau::DEC_DROITE_EGAL:
		case id_morceau::DEC_GAUCHE_EGAL:
		{
			return true;
		}
	}
}

/* ************************************************************************** */

static bool peut_etre_assigne(base *b, ContexteGenerationCode &contexte)
{
	switch (b->type) {
		default:
		{
			return false;
		}
		case type_noeud::VARIABLE:
		{
			auto iter_local = contexte.iter_locale(b->morceau.chaine);

			if (iter_local != contexte.fin_locales()) {
				if (!iter_local->second.est_dynamique) {
					erreur::lance_erreur(
								"Ne peut pas assigner une variable locale non-dynamique",
								contexte,
								b->donnees_morceau(),
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				return true;
			}

			auto iter_globale = contexte.iter_globale(b->morceau.chaine);

			if (iter_globale != contexte.fin_globales()) {
				if (!contexte.non_sur()) {
					erreur::lance_erreur(
								"Ne peut pas assigner une variable globale en dehors d'un bloc 'nonsûr'",
								contexte,
								b->donnees_morceau(),
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				if (!iter_globale->second.est_dynamique) {
					erreur::lance_erreur(
								"Ne peut pas assigner une variable globale non-dynamique",
								contexte,
								b->donnees_morceau(),
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				return true;
			}

			return false;
		}
		case type_noeud::ACCES_MEMBRE:
		{
			return peut_etre_assigne(b->enfants.back(), contexte);
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			if (b->morceau.identifiant == id_morceau::CROCHET_OUVRANT) {
				return peut_etre_assigne(b->enfants.front(), contexte);
			}

			return false;
		}
	}
}

/* ************************************************************************** */

static auto valides_enfants(base *b, ContexteGenerationCode &contexte)
{
	for (auto enfant : b->enfants) {
		performe_validation_semantique(enfant, contexte);
	}
}

static auto valide_appel_pointeur_fonction(
		base *b,
		ContexteGenerationCode &contexte,
		std::list<std::string_view> const &noms_arguments,
		std::string const &nom_fonction)
{
	for (auto const &nom : noms_arguments) {
		if (nom.empty()) {
			continue;
		}

		/* À FAIRE : trouve les données morceaux idoines. */
		erreur::lance_erreur(
					"Les arguments d'un pointeur fonction ne peuvent être nommés",
					contexte,
					b->donnees_morceau(),
					erreur::type_erreur::ARGUMENT_INCONNU);
	}

	auto index_type = (b->aide_generation_code == GENERE_CODE_PTR_FONC_MEMBRE)
			? b->index_type
			: contexte.type_locale(b->morceau.chaine);
	auto &dt_fonc = contexte.magasin_types.donnees_types[index_type];

	/* À FAIRE : bouge ça, trouve le type retour du pointeur de fonction. */

	if (dt_fonc.type_base() != id_morceau::FONC) {
		erreur::lance_erreur(
					"La variable doit être un pointeur vers une fonction",
					contexte,
					b->donnees_morceau(),
					erreur::type_erreur::FONCTION_INCONNUE);
	}

	auto debut = dt_fonc.end() - 1;
	auto fin   = dt_fonc.begin() - 1;

	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		--debut;
	}

	--debut;

	auto dt = DonneesType{};

	while (debut != fin) {
		dt.pousse(*debut--);
	}

	b->index_type = contexte.magasin_types.ajoute_type(dt);
	b->aide_generation_code = APPEL_POINTEUR_FONCTION;

	valides_enfants(b, contexte);

	/* vérifie la compatibilité des arguments pour déterminer
	 * s'il y aura besoin d'une conversion. */
	auto dt_params = donnees_types_parametres(dt_fonc);

	auto enfant = b->enfants.begin();

	/* Validation des types passés en paramètre. */
	for (size_t i = 0; i < dt_params.size() - 1; ++i) {
		auto &type_enf = contexte.magasin_types.donnees_types[(*enfant)->index_type];

		if (dt_params[i].type_base() == id_morceau::TROIS_POINTS) {
			verifie_compatibilite(b, contexte, dt_params[i].derefence(), type_enf, *enfant);
		}
		else {
			verifie_compatibilite(b, contexte, dt_params[i], type_enf, *enfant);
		}

		++enfant;
	}

	b->nom_fonction_appel = nom_fonction;
}

void performe_validation_semantique(base *b, ContexteGenerationCode &contexte)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			/* À FAIRE : inférence de type
			 * - considération du type de retour des fonctions récursive
			 */

			auto const est_externe = possede_drapeau(b->drapeaux, EST_EXTERNE);

			auto module = contexte.module(static_cast<size_t>(b->morceau.module));
			auto nom_fonction = b->morceau.chaine;
			auto &vdf = module->donnees_fonction(nom_fonction);
			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);

			for (auto &df : vdf) {
				if (df.noeud_decl == b) {
					donnees_fonction = &df;
					break;
				}
			}

			auto est_principale = (nom_fonction == "principale");

			if (!est_externe && !est_principale) {
				donnees_fonction->nom_broye = broye_nom_fonction(nom_fonction, module->nom, donnees_fonction->index_type);
			}
			else {
				donnees_fonction->nom_broye = nom_fonction;

				if (est_principale) {
					donnees_fonction->est_utilisee = true;
				}
			}

			if (est_externe) {
				return;
			}

			contexte.commence_fonction(donnees_fonction);

			/* Pousse les paramètres sur la pile. */
			for (auto const &nom : donnees_fonction->nom_args) {
				auto const &argument = donnees_fonction->args[nom];

				auto index_dt = argument.donnees_type;

				auto dt = DonneesType{};

				if (argument.est_variadic) {
					auto &dt_var = contexte.magasin_types.donnees_types[argument.donnees_type];

					dt.pousse(id_morceau::TABLEAU);
					dt.pousse(dt_var.derefence());
				}
				else {
					dt = contexte.magasin_types.donnees_types[argument.donnees_type];
				}

				index_dt = contexte.magasin_types.ajoute_type(dt);

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = argument.est_dynamic;
				donnees_var.est_variadic = argument.est_variadic;
				donnees_var.donnees_type = index_dt;
				donnees_var.est_argument = true;

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
					dt = dt.derefence();
					donnees_var.donnees_type = contexte.magasin_types.ajoute_type(dt);
				}

				contexte.pousse_locale(nom, donnees_var);

				if (argument.est_employe) {
					auto &dt_var = contexte.magasin_types.donnees_types[argument.donnees_type];
					auto id_structure = 0ul;

					if (dt_var.type_base() == id_morceau::POINTEUR || dt_var.type_base() == id_morceau::REFERENCE) {
						id_structure = static_cast<size_t>(dt_var.derefence().type_base() >> 8);
					}
					else {
						id_structure = static_cast<size_t>(dt_var.type_base() >> 8);
					}

					auto &ds = contexte.donnees_structure(id_structure);

					/* pousse chaque membre de la structure sur la pile */

					for (auto &dm : ds.donnees_membres) {
						auto index_dt_m = ds.donnees_types[dm.second.index_membre];

						donnees_var.est_dynamique = argument.est_dynamic;
						donnees_var.donnees_type = index_dt_m;
						donnees_var.est_argument = true;
						donnees_var.est_membre_emploie = true;

						contexte.pousse_locale(dm.first, donnees_var);
					}
				}
			}

			/* vérifie le type du bloc */
			auto bloc = b->enfants.front();

			performe_validation_semantique(bloc, contexte);
			auto type_bloc = bloc->index_type;
			auto dernier = bloc->dernier_enfant();

			auto dt = contexte.magasin_types.donnees_types[b->index_type];

			/* si le bloc est vide -> vérifie qu'aucun type n'a été spécifié */
			if (dernier == nullptr) {
				if (dt.type_base() != id_morceau::RIEN && !donnees_fonction->est_coroutine) {
					erreur::lance_erreur(
								"Instruction de retour manquante",
								contexte,
								b->morceau,
								erreur::type_erreur::TYPE_DIFFERENTS);
				}
			}
			/* si le bloc n'est pas vide */
			else {
				/* si le dernier noeud n'est pas un noeud de retour -> vérifie qu'aucun type n'a été spécifié */
				if (dernier->type != type_noeud::RETOUR) {
					if (dt.type_base() != id_morceau::RIEN && !donnees_fonction->est_coroutine) {
						erreur::lance_erreur(
									"Instruction de retour manquante",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
				}
				/* vérifie que le type du bloc correspond au type de la fonction */
				else {
					if (b->index_type != type_bloc) {
						erreur::lance_erreur(
									"Le type de retour est invalide",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}
				}
			}

			contexte.termine_fonction();
			break;
		}
		case type_noeud::APPEL_FONCTION:
		{
			auto const nom_fonction = std::string(b->morceau.chaine);
			auto noms_arguments = std::any_cast<std::list<std::string_view>>(&b->valeur_calculee);

			/* Nous avons un pointeur vers une fonction. */
			if (b->aide_generation_code == GENERE_CODE_PTR_FONC_MEMBRE
					|| contexte.locale_existe(b->morceau.chaine))
			{
				valide_appel_pointeur_fonction(b, contexte, *noms_arguments, nom_fonction);
				return;
			}

			/* Commence par valider les enfants puisqu'il nous faudra leurs
			 * types pour déterminer la fonction à appeler. */
			valides_enfants(b, contexte);

			auto res = cherche_donnees_fonction(
						contexte,
						nom_fonction,
						*noms_arguments,
						b->enfants,
						static_cast<size_t>(b->morceau.module),
						static_cast<size_t>(b->module_appel));

			auto donnees_fonction = static_cast<DonneesFonction *>(nullptr);
			auto candidate = static_cast<DonneesCandidate *>(nullptr);

			auto poids = 0.0;

			for (auto &dc : res.candidates) {
				if (dc.etat == FONCTION_TROUVEE) {

					if (dc.poids_args > poids) {
						candidate = &dc;
						poids = dc.poids_args;
					}
				}
			}

			if (candidate == nullptr || candidate->df == nullptr) {
				erreur::lance_erreur_fonction_inconnue(
							contexte,
							b,
							res.candidates);
			}

			donnees_fonction = candidate->df;

			/* met en place les drapeaux sur les enfants */

			auto i = 0ul;
			auto nombre_args_simples = candidate->exprs.size();
			auto nombre_args_variadics = nombre_args_simples;

			if (!candidate->exprs.empty() && candidate->exprs.back()->type == type_noeud::TABLEAU) {
				/* ne compte pas le tableau */
				nombre_args_simples -= 1;
				nombre_args_variadics = candidate->drapeaux.size();
			}

			/* les drapeaux pour les arguments simples */
			for (; i < nombre_args_simples; ++i) {
				auto ncompat = candidate->drapeaux[i];
				auto enfant = candidate->exprs[i];
				drapeau_depuis_niveau_compat(enfant, ncompat);
			}

			/* les drapeaux pour les arguments variadics */
			if (!candidate->exprs.empty()) {
				auto noeud_tableau = candidate->exprs.back();
				auto enfant_tabl = noeud_tableau->enfants.begin();

				for (; i < nombre_args_variadics; ++i) {
					auto ncompat = candidate->drapeaux[i];
					auto enfant = *enfant_tabl++;
					drapeau_depuis_niveau_compat(enfant, ncompat);
				}
			}

			b->df = candidate->df;
			b->df->est_utilisee = true;

			if (b->index_type == -1ul) {
				b->index_type = donnees_fonction->index_type_retour;
			}

			b->enfants.clear();

			for (auto enfant : candidate->exprs) {
				b->enfants.push_back(enfant);
			}

			if (donnees_fonction->nom_broye.empty()) {
				b->nom_fonction_appel = nom_fonction;
			}
			else {
				b->nom_fonction_appel = donnees_fonction->nom_broye;
			}

			break;
		}
		case type_noeud::VARIABLE:
		{
			/* Logique pour déclarer sans mot-clé.
			 * x = 5; # déclaration car n'existe pas
			 * x = 6; # illégale car non dynamique
			 *
			 * soit x = 5; # déclaration car taggé
			 * x = 6; # illégal car non dynamique
			 *
			 * soit x = 5; # déclaration car taggé
			 * x = 6; # illégal car non dynamique
			 */

			if (possede_drapeau(b->drapeaux, DECLARATION)) {
				auto existe = contexte.locale_existe(b->morceau.chaine);

				if (existe) {
					erreur::lance_erreur(
								"Redéfinition de la variable locale",
								contexte,
								b->morceau,
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}
				else {
					existe = contexte.globale_existe(b->morceau.chaine);

					if (existe) {
						erreur::lance_erreur(
									"Redéfinition de la variable globale",
									contexte,
									b->morceau,
									erreur::type_erreur::VARIABLE_REDEFINIE);
					}
				}

				if (b->index_type == -1ul) {
					erreur::lance_erreur(
								"Aucun type précisé",
								contexte,
								b->morceau,
								erreur::type_erreur::TYPE_INCONNU);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.donnees_type = b->index_type;

				auto &dt = contexte.magasin_types.donnees_types[donnees_var.donnees_type];

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
				}

				if (contexte.donnees_fonction == nullptr) {
					contexte.pousse_globale(b->morceau.chaine, donnees_var);
				}
				else {
					contexte.pousse_locale(b->morceau.chaine, donnees_var);
				}

				b->aide_generation_code = GENERE_CODE_DECL_VAR;

				return;
			}

			auto const &iter_locale = contexte.iter_locale(b->morceau.chaine);

			if (iter_locale != contexte.fin_locales()) {
				b->aide_generation_code = GENERE_CODE_ACCES_VAR;
				b->index_type = iter_locale->second.donnees_type;
				return;
			}

			auto const &iter_globale = contexte.iter_globale(b->morceau.chaine);

			if (iter_globale != contexte.fin_globales()) {
				b->aide_generation_code = GENERE_CODE_ACCES_VAR;
				b->index_type = iter_globale->second.donnees_type;
				return;
			}
			if (b->aide_generation_code == GAUCHE_ASSIGNATION) {
				b->aide_generation_code = GENERE_CODE_DECL_VAR;

				if (b->index_type == -1ul) {
					erreur::lance_erreur(
								"Aucun type précisé",
								contexte,
								b->morceau,
								erreur::type_erreur::TYPE_INCONNU);
				}

				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.donnees_type = b->index_type;

				auto &dt = contexte.magasin_types.donnees_types[donnees_var.donnees_type];

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
				}

				if (contexte.donnees_fonction == nullptr) {
					contexte.pousse_globale(b->morceau.chaine, donnees_var);
				}
				else {
					contexte.pousse_locale(b->morceau.chaine, donnees_var);
				}

				return;
			}

			b->aide_generation_code = GENERE_CODE_ACCES_VAR;

			/* Vérifie si c'est une fonction. */
			auto module = contexte.module(static_cast<size_t>(b->morceau.module));

			/* À FAIRE : trouve la fonction selon le type */
			if (module->fonction_existe(b->morceau.chaine)) {
				auto &donnees_fonction = module->donnees_fonction(b->morceau.chaine);
				b->index_type = donnees_fonction.front().index_type;
				b->nom_fonction_appel = donnees_fonction.front().nom_broye;
				donnees_fonction.front().est_utilisee = true;
				return;
			}

			/* Nous avons peut-être une énumération. */
			if (contexte.structure_existe(b->morceau.chaine)) {
				auto &donnees_structure = contexte.donnees_structure(b->morceau.chaine);

				if (donnees_structure.est_enum) {
					b->index_type = donnees_structure.index_type;
					return;
				}
			}

			/* déclare la variable */
			if (b->index_type != -1ul) {
				b->aide_generation_code = GENERE_CODE_DECL_VAR;
				auto donnees_var = DonneesVariable{};
				donnees_var.est_dynamique = (b->drapeaux & DYNAMIC) != 0;
				donnees_var.donnees_type = b->index_type;

				auto &dt = contexte.magasin_types.donnees_types[donnees_var.donnees_type];

				if (dt.type_base() == id_morceau::REFERENCE) {
					donnees_var.drapeaux |= BESOIN_DEREF;
				}

				if (contexte.donnees_fonction == nullptr) {
					contexte.pousse_globale(b->morceau.chaine, donnees_var);
				}
				else {
					contexte.pousse_locale(b->morceau.chaine, donnees_var);
				}

				return;
			}

			erreur::lance_erreur(
						"Variable inconnue",
						contexte,
						b->morceau,
						erreur::type_erreur::VARIABLE_INCONNUE);
		}
		case type_noeud::ACCES_MEMBRE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();

			performe_validation_semantique(structure, contexte);

			auto const &index_type = structure->index_type;
			auto type_structure = contexte.magasin_types.donnees_types[index_type];

			if (type_structure.type_base() == id_morceau::POINTEUR || type_structure.type_base() == id_morceau::REFERENCE) {
				type_structure = type_structure.derefence();
			}

			if (type_structure.type_base() == id_morceau::CHAINE) {
				if (membre->chaine() == "taille") {
					b->index_type = contexte.magasin_types[TYPE_Z64];
					return;
				}

				if (membre->chaine() == "pointeur") {
					b->index_type = contexte.magasin_types[TYPE_PTR_Z8];
					return;
				}

				erreur::lance_erreur(
							"'chaine' ne possède pas cette propriété !",
							contexte,
							membre->donnees_morceau(),
							erreur::type_erreur::MEMBRE_INCONNU);
			}

			if (type_structure.type_base() == id_morceau::EINI) {
				if (membre->chaine() == "info") {
					auto id_info_type = contexte.donnees_structure("InfoType").id;

					auto dt = DonneesType{};
					dt.pousse(id_morceau::POINTEUR);
					dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(id_info_type << 8));

					b->index_type = contexte.magasin_types.ajoute_type(dt);
					return;
				}

				if (membre->chaine() == "pointeur") {
					b->index_type = contexte.magasin_types[TYPE_PTR_Z8];
					return;
				}

				erreur::lance_erreur(
							"'eini' ne possède pas cette propriété !",
							contexte,
							membre->donnees_morceau(),
							erreur::type_erreur::MEMBRE_INCONNU);
			}

			if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
#ifdef NONSUR
				if (!contexte.non_sur() && expr_gauche) {
					erreur::lance_erreur(
								"Modification des membres du tableau hors d'un bloc 'nonsûr' interdite",
								contexte,
								b->morceau,
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}
#endif
				if (membre->chaine() == "pointeur") {
					auto dt = DonneesType{};
					dt.pousse(id_morceau::POINTEUR);
					dt.pousse(type_structure.derefence());

					b->index_type = contexte.magasin_types.ajoute_type(dt);
					return;
				}

				if (membre->chaine() == "taille") {
					b->index_type = contexte.magasin_types[TYPE_N64];
					return;
				}

				erreur::lance_erreur(
							"Le tableau ne possède pas cette propriété !",
							contexte,
							membre->donnees_morceau(),
							erreur::type_erreur::MEMBRE_INCONNU);
			}

			if ((type_structure.type_base() & 0xff) == id_morceau::CHAINE_CARACTERE) {
				auto const index_structure = size_t(type_structure.type_base() >> 8);

				auto const &nom_membre = membre->chaine();

				auto &donnees_structure = contexte.donnees_structure(index_structure);

				if (donnees_structure.est_enum) {
					b->index_type = donnees_structure.index_type;
					return;
				}

				auto const iter = donnees_structure.donnees_membres.find(nom_membre);

				if (iter == donnees_structure.donnees_membres.end()) {
					/* À FAIRE : proposer des candidats possibles ou imprimer la structure. */
					erreur::lance_erreur(
								"Membre inconnu",
								contexte,
								membre->morceau,
								erreur::type_erreur::MEMBRE_INCONNU);
				}

				auto const &donnees_membres = iter->second;

				b->index_type = donnees_structure.donnees_types[donnees_membres.index_membre];

				/* pointeur vers une fonction */
				if (membre->type == type_noeud::APPEL_FONCTION) {
					/* ceci est le type de la fonction, l'analyse de l'appel
					 * vérifiera le type des arguments et ajournera le type du
					 * membre pour être celui du type de retour */
					membre->index_type = b->index_type;
					membre->aide_generation_code = GENERE_CODE_PTR_FONC_MEMBRE;

					performe_validation_semantique(membre, contexte);

					/* le type de l'accès est celui du retour de la fonction */
					b->index_type = membre->index_type;
				}

				return;
			}

			erreur::lance_erreur(
						"Impossible d'accéder au membre d'un objet n'étant pas une structure",
						contexte,
						structure->donnees_morceau(),
						erreur::type_erreur::TYPE_DIFFERENTS);
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const nom_module = enfant1->chaine();

			auto module = contexte.module(static_cast<size_t>(b->morceau.module));

			if (!module->importe_module(nom_module)) {
				erreur::lance_erreur(
							"module inconnu",
							contexte,
							enfant1->donnees_morceau(),
							erreur::type_erreur::MODULE_INCONNU);
			}

			auto module_importe = contexte.module(nom_module);

			if (module_importe == nullptr) {
				erreur::lance_erreur(
							"module inconnu",
							contexte,
							enfant1->donnees_morceau(),
							erreur::type_erreur::MODULE_INCONNU);
			}

			auto const nom_fonction = enfant2->chaine();

			if (!module_importe->possede_fonction(nom_fonction)) {
				erreur::lance_erreur(
							"Le module ne possède pas la fonction",
							contexte,
							enfant2->donnees_morceau(),
							erreur::type_erreur::FONCTION_INCONNUE);
			}

			enfant2->module_appel = static_cast<int>(module_importe->id);

			performe_validation_semantique(enfant2, contexte);

			b->index_type = enfant2->index_type;
			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			performe_validation_semantique(expression, contexte);

			b->index_type = expression->index_type;

			if (b->index_type == -1ul) {
				erreur::lance_erreur(
							"Impossible de définir le type de la variable !",
							contexte,
							b->morceau,
							erreur::type_erreur::TYPE_INCONNU);
			}

			/* NOTE : l'appel à performe_validation_semantique plus bas peut
			 * changer le vecteur et invalider une référence ou un pointeur,
			 * donc nous faisons une copie... */
			auto const dt = contexte.magasin_types.donnees_types[b->index_type];

			if (dt.type_base() == id_morceau::RIEN) {
				erreur::lance_erreur(
							"Impossible d'assigner une expression de type 'rien' à une variable !",
							contexte,
							b->morceau,
							erreur::type_erreur::ASSIGNATION_RIEN);
			}

			/* Ajourne les données du premier enfant si elles sont invalides, dans le
			 * cas d'une déclaration de variable. */
			if (variable->index_type == -1ul) {
				variable->index_type = b->index_type;
			}

			variable->aide_generation_code = GAUCHE_ASSIGNATION;
			performe_validation_semantique(variable, contexte);

			/* À cause du mélange des opérateurs "[]" et "de", il faut attendre
			 * que toutes les validations sémantiques soient faites pour pouvoir
			 * calculer la validité de l'assignation, car la validation de
			 * l'opérateur '[]' met les noeuds dans l'ordre. */
			if (variable->aide_generation_code != GENERE_CODE_DECL_VAR && !peut_etre_assigne(variable, contexte)) {
				erreur::lance_erreur(
							"Impossible d'assigner l'expression à la variable !",
							contexte,
							b->morceau,
							erreur::type_erreur::ASSIGNATION_INVALIDE);
			}

			auto const &type_gauche = contexte.magasin_types.donnees_types[variable->index_type];
			auto const niveau_compat = sont_compatibles(type_gauche, dt, expression->type);

			b->valeur_calculee = niveau_compat;

			if (niveau_compat == niveau_compat::aucune) {
				erreur::lance_erreur_assignation_type_differents(
							type_gauche,
							dt,
							contexte,
							b->morceau);
			}

			if (niveau_compat == niveau_compat::prend_reference) {
				expression->drapeaux |= PREND_REFERENCE;
			}

			break;
		}
		case type_noeud::NOMBRE_REEL:
		{
			b->index_type = contexte.magasin_types[TYPE_R64];
			break;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			b->index_type = contexte.magasin_types[TYPE_Z32];
			break;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			if ((b->morceau.identifiant == id_morceau::CROCHET_OUVRANT)
					&& (enfant2->type == type_noeud::ACCES_MEMBRE
						|| enfant2->morceau.identifiant == id_morceau::CROCHET_OUVRANT))
			{
				/* Pour corriger les accès membres via 'de' ou les accès chainés
				 * des opérateurs[], il faut interchanger le premier enfant des
				 * noeuds. */

				auto enfant1de = enfant2->enfants.front();
				auto enfant2de = enfant2->enfants.back();

				b->enfants.clear();
				enfant2->enfants.clear();

				/* inverse les enfants pour que le 'pointeur' soit à gauche et
				 * l'index à droite */
				b->enfants.push_back(enfant2);
				b->enfants.push_back(enfant1de);

				enfant2->enfants.push_back(enfant1);
				enfant2->enfants.push_back(enfant2de);

				enfant1 = b->enfants.front();
				enfant2 = b->enfants.back();
			}

			performe_validation_semantique(enfant1, contexte);
			performe_validation_semantique(enfant2, contexte);

			auto const index_type1 = enfant1->index_type;
			auto const index_type2 = enfant2->index_type;

			auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
			auto const &type2 = contexte.magasin_types.donnees_types[index_type2];

			if ((b->morceau.identifiant != id_morceau::CROCHET_OUVRANT)) {
				if (!peut_operer(type1, type2, enfant1->type, enfant2->type)) {
					erreur::lance_erreur_type_operation(
								type1,
								type2,
								contexte,
								b->morceau);
				}
			}

			if (est_assignation_operee(b->morceau.identifiant)) {
				if (!peut_etre_assigne(enfant1, contexte)) {
					erreur::lance_erreur(
								"Impossible d'assigner l'expression à la variable !",
								contexte,
								b->morceau,
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}
			}

			switch (b->identifiant()) {
				default:
				{
					b->index_type = index_type1;
					break;
				}
				case id_morceau::CROCHET_OUVRANT:
				{
					auto type_base = type1.type_base();

					switch (type_base & 0xff) {
						case id_morceau::TABLEAU:
						case id_morceau::POINTEUR:
						{
							b->index_type = contexte.magasin_types.ajoute_type(type1.derefence());
							break;
						}
						case id_morceau::CHAINE:
						{
							b->index_type = contexte.magasin_types[TYPE_Z8];
							break;
						}
						default:
						{
							std::stringstream ss;
							ss << "Le type '" << chaine_type(type1, contexte)
							   << "' ne peut être déréférencé par opérateur[] !";

							erreur::lance_erreur(
										ss.str(),
										contexte,
										b->morceau,
										erreur::type_erreur::TYPE_DIFFERENTS);
						}
					}

					break;
				}
				case id_morceau::EGALITE:
				case id_morceau::DIFFERENCE:
				case id_morceau::INFERIEUR:
				case id_morceau::INFERIEUR_EGAL:
				case id_morceau::SUPERIEUR:
				case id_morceau::SUPERIEUR_EGAL:
				case id_morceau::ESP_ESP:
				case id_morceau::BARRE_BARRE:
				{
					b->index_type = contexte.magasin_types[TYPE_BOOL];
					break;
				}
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte);
			auto index_type = enfant->index_type;
			auto const &type = contexte.magasin_types.donnees_types[index_type];

			if (b->index_type == -1ul) {
				switch (b->identifiant()) {
					default:
					{
						b->index_type = enfant->index_type;
						break;
					}
					case id_morceau::AROBASE:
					{
						auto dt = DonneesType{};
						dt.pousse(id_morceau::POINTEUR);
						dt.pousse(type);

						b->index_type = contexte.magasin_types.ajoute_type(dt);
						break;
					}
					case id_morceau::EXCLAMATION:
					{
						if (type.type_base() != id_morceau::BOOL) {
							erreur::lance_erreur(
										"L'opérateur '!' doit recevoir une expression de type 'bool'",
										contexte,
										enfant->donnees_morceau(),
										erreur::type_erreur::TYPE_DIFFERENTS);
						}

						b->index_type = contexte.magasin_types[TYPE_BOOL];
						break;
					}
				}
			}

			break;
		}
		case type_noeud::RETOUR:
		{
			if (b->enfants.empty()) {
				b->index_type = contexte.magasin_types[TYPE_RIEN];
				return;
			}

			performe_validation_semantique(b->enfants.front(), contexte);
			b->index_type = b->enfants.front()->index_type;

			break;
		}
		case type_noeud::CHAINE_LITTERALE:
		{
			/* fais en sorte que les caractères échappés ne soient pas comptés
			 * comme deux caractères distincts, ce qui ne peut se faire avec la
			 * std::string_view */
			std::string corrigee;
			corrigee.reserve(b->morceau.chaine.size());

			for (size_t i = 0; i < b->morceau.chaine.size(); ++i) {
				auto c = b->morceau.chaine[i];

				if (c == '\\') {
					c = caractere_echape(&b->morceau.chaine[i]);
					++i;
				}

				corrigee.push_back(c);
			}

			/* À FAIRE : ceci ne fonctionne pas dans le cas des noeuds différés
			 * où la valeur calculee est redéfinie. */
			b->valeur_calculee = corrigee;
			b->index_type = contexte.magasin_types[TYPE_CHAINE];

			break;
		}
		case type_noeud::BOOLEEN:
		{
			b->index_type = contexte.magasin_types[TYPE_BOOL];
			break;
		}
		case type_noeud::CARACTERE:
		{
			b->index_type = contexte.magasin_types[TYPE_Z8];
			break;
		}
		case type_noeud::SAUFSI:
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.size();
			auto iter_enfant = b->enfants.begin();

			auto enfant1 = *iter_enfant++;
			auto enfant2 = *iter_enfant++;

			performe_validation_semantique(enfant1, contexte);
			auto index_type = enfant1->index_type;
			auto const &type_condition = contexte.magasin_types.donnees_types[index_type];

			if (type_condition.type_base() != id_morceau::BOOL) {
				erreur::lance_erreur("Attendu un type booléen pour l'expression 'si'",
									 contexte,
									 enfant1->donnees_morceau(),
									 erreur::type_erreur::TYPE_DIFFERENTS);
			}

			performe_validation_semantique(enfant2, contexte);

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				auto enfant3 = *iter_enfant++;
				performe_validation_semantique(enfant3, contexte);
			}

			break;
		}
		case type_noeud::BLOC:
		{
#ifdef AVEC_LLVM
			/* Évite les crash lors de l'estimation du bloc suivant les
			 * contrôles de flux. */
			b->valeur_calculee = static_cast<llvm::BasicBlock *>(nullptr);
#endif

			contexte.empile_nombre_locales();

			valides_enfants(b, contexte);

			if (b->enfants.empty()) {
				b->index_type = contexte.magasin_types[TYPE_RIEN];
			}
			else {
				b->index_type = b->enfants.back()->index_type;
			}

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::POUR:
		{
			auto const nombre_enfants = b->enfants.size();
			auto iter = b->enfants.begin();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;
			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto verifie_redefinition_variable = [](base *b_local, ContexteGenerationCode &contexte_loc)
			{
				if (contexte_loc.locale_existe(b_local->chaine())) {
					erreur::lance_erreur(
								"(Boucle pour) rédéfinition de la variable",
								contexte_loc,
								b_local->donnees_morceau(),
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}

				if (contexte_loc.globale_existe(b_local->chaine())) {
					erreur::lance_erreur(
								"(Boucle pour) rédéfinition de la variable globale",
								contexte_loc,
								b_local->donnees_morceau(),
								erreur::type_erreur::VARIABLE_REDEFINIE);
				}
			};

			auto const requiers_index = enfant1->morceau.identifiant == id_morceau::VIRGULE;

			if (requiers_index) {
				auto var = enfant1->enfants.front();
				auto idx = enfant1->enfants.back();
				verifie_redefinition_variable(var, contexte);
				verifie_redefinition_variable(idx, contexte);
			}
			else {
				verifie_redefinition_variable(enfant1, contexte);
			}

			performe_validation_semantique(enfant2, contexte);

			auto index_type = enfant2->index_type;
			auto &type = contexte.magasin_types.donnees_types[index_type];
			auto drapeaux = static_cast<char>(0);

			/* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
			 * type de retour d'une coroutine n'interfère avec le type d'une
			 * variable (par exemple quand nous retournons une chaine). */
			if (enfant2->type == type_noeud::PLAGE) {
				if (requiers_index) {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE_INDEX;
				}
				else {
					b->aide_generation_code = GENERE_BOUCLE_PLAGE;
				}
			}
			else if (enfant2->type == type_noeud::APPEL_FONCTION && enfant2->df->est_coroutine) {
				enfant1->index_type = enfant2->index_type;

				if (requiers_index) {
					b->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
				}
				else {
					b->aide_generation_code = GENERE_BOUCLE_COROUTINE;
				}
			}
			else {
				if ((type.type_base() & 0xff) == id_morceau::TABLEAU) {
					index_type = contexte.magasin_types.ajoute_type(type.derefence());

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}

					drapeaux = BESOIN_DEREF;
				}
				else if (type.type_base() == id_morceau::CHAINE) {
					index_type = contexte.magasin_types[TYPE_Z8];
					enfant1->index_type = index_type;

					if (requiers_index) {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU_INDEX;
					}
					else {
						b->aide_generation_code = GENERE_BOUCLE_TABLEAU;
					}

					drapeaux = BESOIN_DEREF;
				}
				else {
					auto valeur = contexte.est_locale_variadique(enfant2->chaine());

					if (!valeur) {
						erreur::lance_erreur(
									"La variable n'est ni un argument variadic, ni un tableau",
									contexte,
									enfant2->donnees_morceau());
					}
				}

				/* À FAIRE : ceci est là pour sauvegarder l'index des coroutines
				 * lors de l'itération de chaines ou de tableaux, ceci duplique
				 * le code dans la coulisse C pour le nom de la variable. */
				auto nom_var = "__i" + std::to_string(b->morceau.ligne_pos);
				contexte.magasin_chaines.push_back(nom_var);

				auto donnees_var = DonneesVariable{};
				donnees_var.donnees_type = contexte.magasin_types[TYPE_Z32];
				donnees_var.drapeaux = 0;

				contexte.pousse_locale(contexte.magasin_chaines.back(), donnees_var);
			}

			contexte.empile_nombre_locales();

			auto est_dynamique = false;
			auto iter_locale = contexte.iter_locale(enfant2->chaine());

			if (iter_locale != contexte.fin_locales()) {
				est_dynamique = iter_locale->second.est_dynamique;
			}
			else {
				auto iter_globale = contexte.iter_globale(enfant2->chaine());

				if (iter_globale != contexte.fin_globales()) {
					est_dynamique = iter_globale->second.est_dynamique;
				}
			}

			if (requiers_index) {
				auto var = enfant1->enfants.front();
				auto idx = enfant1->enfants.back();
				var->index_type = index_type;

				auto donnees_var = DonneesVariable{};
				donnees_var.donnees_type = index_type;
				donnees_var.drapeaux = drapeaux;
				donnees_var.est_dynamique = est_dynamique;

				contexte.pousse_locale(var->chaine(), donnees_var);

				index_type = contexte.magasin_types[TYPE_Z32];
				idx->index_type = index_type;

				donnees_var.donnees_type = index_type;
				donnees_var.drapeaux = 0;
				donnees_var.est_dynamique = est_dynamique;

				contexte.pousse_locale(idx->chaine(), donnees_var);
			}
			else {
				auto donnees_var = DonneesVariable{};
				donnees_var.donnees_type = index_type;
				donnees_var.drapeaux = drapeaux;
				donnees_var.est_dynamique = est_dynamique;

				contexte.pousse_locale(enfant1->chaine(), donnees_var);
			}

			/* À FAIRE : ceci duplique logique coulisse. */
			auto goto_continue = "__continue_boucle_pour" + std::to_string(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + std::to_string(b->morceau.ligne_pos);
			auto goto_brise = "__boucle_pour_brise" + std::to_string(b->morceau.ligne_pos);

			contexte.empile_goto_continue(enfant1->chaine(), goto_continue);
			contexte.empile_goto_arrete(enfant1->chaine(), (enfant4 != nullptr) ? goto_brise : goto_apres);

			performe_validation_semantique(enfant3, contexte);

			if (enfant4 != nullptr) {
				performe_validation_semantique(enfant4, contexte);

				if (enfant5 != nullptr) {
					performe_validation_semantique(enfant5, contexte);
				}
			}

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::TRANSTYPE:
		{
			if (b->index_type == -1ul) {
				erreur::lance_erreur(
							"Ne peut transtyper vers un type invalide",
							contexte,
							b->donnees_morceau(),
							erreur::type_erreur::TYPE_INCONNU);
			}

			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte);

			if (enfant->index_type == -1ul) {
				erreur::lance_erreur(
							"Ne peut calculer le type d'origine",
							contexte,
							enfant->donnees_morceau(),
							erreur::type_erreur::TYPE_INCONNU);
			}

			break;
		}
		case type_noeud::NUL:
		{
			b->index_type = contexte.magasin_types[TYPE_PTR_NUL];
			break;
		}
		case type_noeud::TAILLE_DE:
		{
			b->index_type = contexte.magasin_types[TYPE_N32];
			valides_enfants(b, contexte);
			break;
		}
		case type_noeud::PLAGE:
		{
			auto iter = b->enfants.begin();

			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			performe_validation_semantique(enfant1, contexte);
			performe_validation_semantique(enfant2, contexte);

			auto index_type_debut = enfant1->index_type;
			auto index_type_fin   = enfant2->index_type;

			if (index_type_debut == -1ul || index_type_fin == -1ul) {
				erreur::lance_erreur(
							"Les types de l'expression sont invalides !",
							contexte,
							b->morceau,
							erreur::type_erreur::TYPE_INCONNU);
			}

			auto const &type_debut = contexte.magasin_types.donnees_types[index_type_debut];
			auto const &type_fin   = contexte.magasin_types.donnees_types[index_type_fin];

			if (type_debut.est_invalide() || type_fin.est_invalide()) {
				erreur::lance_erreur(
							"Les types de l'expression sont invalides !",
							contexte,
							b->morceau,
							erreur::type_erreur::TYPE_INCONNU);
			}

			if (index_type_debut != index_type_fin) {
				erreur::lance_erreur_type_operation(
							type_debut,
							type_fin,
							contexte,
							b->morceau);
			}

			auto const type = type_debut.type_base();

			if (!est_type_entier_naturel(type) && !est_type_entier_relatif(type) && !est_type_reel(type)) {
				erreur::lance_erreur(
							"Attendu des types réguliers dans la plage de la boucle 'pour'",
							contexte,
							b->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			b->index_type = index_type_debut;

			break;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
			valides_enfants(b, contexte);

			auto chaine_var = b->enfants.empty() ? std::string_view{""} : b->enfants.front()->chaine();

			auto label_goto = (b->morceau.identifiant == id_morceau::CONTINUE)
					? contexte.goto_continue(chaine_var)
					: contexte.goto_arrete(chaine_var);

			if (label_goto.empty()) {
				if (chaine_var.empty()) {
					erreur::lance_erreur(
								"'continue' ou 'arrête' en dehors d'une boucle",
								contexte,
								b->morceau,
								erreur::type_erreur::CONTROLE_INVALIDE);
				}
				else {
					erreur::lance_erreur(
								"Variable inconnue",
								contexte,
								b->enfants.front()->donnees_morceau(),
								erreur::type_erreur::VARIABLE_INCONNUE);
				}
			}

			break;
		}
		case type_noeud::BOUCLE:
		{
			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + std::to_string(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + std::to_string(b->morceau.ligne_pos);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(b->enfants.front(), contexte);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::DIFFERE:
		case type_noeud::TABLEAU:
		{
			valides_enfants(b, contexte);
			break;
		}
		case type_noeud::TANTQUE:
		{
			assert(b->enfants.size() == 2);
			auto iter = b->enfants.begin();
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;

			performe_validation_semantique(enfant1, contexte);

			/* À FAIRE : ceci duplique logique coulisse */
			auto goto_continue = "__continue_boucle_pour" + std::to_string(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + std::to_string(b->morceau.ligne_pos);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			performe_validation_semantique(enfant2, contexte);

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			auto &dt = contexte.magasin_types.donnees_types[enfant1->index_type];

			/* À FAIRE : tests */
			if (dt.type_base() != id_morceau::BOOL) {
				erreur::lance_erreur(
							"Une expression booléenne est requise pour la boucle 'tantque'",
							contexte,
							enfant1->morceau,
							erreur::type_erreur::TYPE_ARGUMENT);
			}

			break;
		}
		case type_noeud::NONSUR:
		{
			contexte.non_sur(true);
			performe_validation_semantique(b->enfants.front(), contexte);
			contexte.non_sur(false);

			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			std::vector<base *> feuilles;
			rassemble_feuilles(b, feuilles);

			for (auto f : feuilles) {
				performe_validation_semantique(f, contexte);
			}

			if (feuilles.empty()) {
				return;
			}

			auto premiere_feuille = feuilles.front();

			auto type_feuille = premiere_feuille->index_type;

			for (auto f : feuilles) {
				/* À FAIRE : test */
				if (f->index_type != type_feuille) {
					auto dt_feuille0 = contexte.magasin_types.donnees_types[type_feuille];
					auto dt_feuille1 = contexte.magasin_types.donnees_types[f->index_type];

					erreur::lance_erreur_assignation_type_differents(
								dt_feuille0,
								dt_feuille1,
								contexte,
								f->morceau);
				}
			}

			DonneesType dt;
			dt.pousse(id_morceau::TABLEAU | static_cast<int>(feuilles.size() << 8));
			dt.pousse(contexte.magasin_types.donnees_types[type_feuille]);

			b->index_type = contexte.magasin_types.ajoute_type(dt);
			break;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			/* cherche la structure dans le tableau de structure */
			if (!contexte.structure_existe(b->chaine())) {
				erreur::lance_erreur(
							"Structure inconnue",
							contexte,
							b->morceau,
							erreur::type_erreur::STRUCTURE_INCONNUE);
			}

			auto &donnees_struct = contexte.donnees_structure(b->chaine());

			DonneesType dt;
			dt.pousse(id_morceau::CHAINE_CARACTERE | (static_cast<int>(donnees_struct.id) << 8));

			b->index_type = contexte.magasin_types.ajoute_type(dt);
			break;
		}
		case type_noeud::TYPE_DE:
		{
			/* À FAIRE : retourne une structure InfoType */
			b->index_type = contexte.magasin_types[TYPE_N64];
			performe_validation_semantique(b->enfants.front(), contexte);
			break;
		}
		case type_noeud::MEMOIRE:
		{
			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte);

			auto &dt_enfant = contexte.magasin_types.donnees_types[enfant->index_type];
			b->index_type = contexte.magasin_types.ajoute_type(dt_enfant.derefence());

			if (dt_enfant.type_base() != id_morceau::POINTEUR) {
				erreur::lance_erreur(
							"Un pointeur est requis pour le déréférencement via 'mémoire'",
							contexte,
							enfant->donnees_morceau(),
							erreur::type_erreur::TYPE_DIFFERENTS);
			}

			break;
		}
		case type_noeud::LOGE:
		{
			auto nombre_enfant = b->enfants.size();
			auto enfant = b->enfants.begin();
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			if ((dt.type_base() & 0xff) == id_morceau::TABLEAU) {
				/* transforme en type tableau dynamic */
				auto taille = (static_cast<int>(dt.type_base() >> 8));
				b->drapeaux |= EST_CALCULE;
				b->valeur_calculee = taille;

				auto ndt = DonneesType{};
				ndt.pousse(id_morceau::TABLEAU);
				ndt.pousse(dt.derefence());

				b->index_type = contexte.magasin_types.ajoute_type(ndt);
			}
			else if (dt.type_base() == id_morceau::CHAINE) {
				performe_validation_semantique(*enfant++, contexte);
				nombre_enfant -= 1;
			}
			else {
				auto dt_loge = DonneesType{};
				dt_loge.pousse(id_morceau::POINTEUR);
				dt_loge.pousse(dt);

				b->index_type = contexte.magasin_types.ajoute_type(dt_loge);
			}

			/* À FAIRE : détermine ce qui doit se passer dans un bloc suite à un
			 * échec d'allocation. */
			if (nombre_enfant == 1) {
				performe_validation_semantique(*enfant++, contexte);
			}

			break;
		}
		case type_noeud::RELOGE:
		{
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			auto nombre_enfant = b->enfants.size();
			auto enfant = b->enfants.begin();
			auto enfant1 = *enfant++;
			performe_validation_semantique(enfant1, contexte);

			if ((dt.type_base() & 0xff) == id_morceau::TABLEAU) {
				/* transforme en type tableau dynamic */
				auto taille = (static_cast<int>(dt.type_base() >> 8));
				b->drapeaux |= EST_CALCULE;
				b->valeur_calculee = taille;

				auto ndt = DonneesType{};
				ndt.pousse(id_morceau::TABLEAU);
				ndt.pousse(dt.derefence());

				b->index_type = contexte.magasin_types.ajoute_type(ndt);
			}
			else if (dt.type_base() == id_morceau::CHAINE) {
				performe_validation_semantique(*enfant++, contexte);
				nombre_enfant -= 1;
			}
			else {
				auto dt_loge = DonneesType{};
				dt_loge.pousse(id_morceau::POINTEUR);
				dt_loge.pousse(dt);

				b->index_type = contexte.magasin_types.ajoute_type(dt_loge);
			}

			if (enfant1->index_type != b->index_type) {
				auto &dt_enf = contexte.magasin_types.donnees_types[enfant1->index_type];
				erreur::lance_erreur_type_arguments(
							dt,
							dt_enf,
							contexte,
							b->morceau,
							enfant1->morceau);
			}

			/* À FAIRE : détermine ce qui doit se passer dans un bloc suite à un
			 * échec d'allocation. */
			if (nombre_enfant == 2) {
				performe_validation_semantique(*enfant++, contexte);
			}

			break;
		}
		case type_noeud::DELOGE:
		{
			auto enfant = b->enfants.front();
			performe_validation_semantique(enfant, contexte);
			break;
		}
		case type_noeud::DECLARATION_STRUCTURE:
		{
			auto &ds = contexte.donnees_structure(b->chaine());

			auto verifie_inclusion_valeur = [&ds, &contexte](base *enf)
			{
				if (enf->index_type == ds.index_type) {
					erreur::lance_erreur(
								"Ne peut inclure la structure dans elle-même par valeur",
								contexte,
								enf->morceau,
								erreur::type_erreur::TYPE_ARGUMENT);
				}
				else {
					auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
					auto type_base = dt.type_base();

					if ((type_base & 0xff) == id_morceau::TABLEAU && type_base != id_morceau::TABLEAU) {
						auto dt_deref = dt.derefence();

						if (dt_deref == contexte.magasin_types.donnees_types[ds.index_type]) {
							erreur::lance_erreur(
										"Ne peut inclure la structure dans elle-même par valeur",
										contexte,
										enf->morceau,
										erreur::type_erreur::TYPE_ARGUMENT);
						}
					}
				}
			};

			auto verifie_redefinition_membre = [&ds, &contexte](base *enf)
			{
				if (ds.donnees_membres.find(enf->chaine()) != ds.donnees_membres.end()) {
					erreur::lance_erreur(
								"Redéfinition du membre",
								contexte,
								enf->morceau,
								erreur::type_erreur::MEMBRE_REDEFINI);
				}
			};

			for (auto enfant : b->enfants) {
				if (enfant->type == type_noeud::ASSIGNATION_VARIABLE) {
					if (enfant->morceau.identifiant != id_morceau::EGAL) {
						erreur::lance_erreur(
									"Déclaration impossible dans la déclaration du membre",
									contexte,
									enfant->morceau,
									erreur::type_erreur::NORMAL);
					}

					auto decl_membre = enfant->enfants.front();
					auto decl_expr = enfant->enfants.back();
					auto nom_membre = decl_membre->chaine();

					verifie_redefinition_membre(decl_membre);

					performe_validation_semantique(decl_expr, contexte);

					if (decl_membre->index_type != decl_expr->index_type) {
						if (decl_membre->index_type == -1ul) {
							decl_membre->index_type = decl_expr->index_type;
						}
						else {
							auto &dt_enf = contexte.magasin_types.donnees_types[decl_membre->index_type];
							auto &dt_exp = contexte.magasin_types.donnees_types[decl_expr->index_type];

							auto compat = sont_compatibles(
										dt_enf,
										dt_exp,
										decl_expr->type);

							if (compat != niveau_compat::ok) {
								erreur::lance_erreur_type_arguments(
											dt_enf,
											dt_exp,
											contexte,
											decl_membre->morceau,
											decl_expr->morceau);
							}
						}
					}

					verifie_inclusion_valeur(decl_membre);

					ds.donnees_membres.insert({nom_membre, { ds.donnees_types.size(), decl_expr }});
					ds.donnees_types.push_back(decl_membre->index_type);
				}
				else if (enfant->type == type_noeud::VARIABLE) {
					verifie_redefinition_membre(enfant);
					verifie_inclusion_valeur(enfant);

					ds.donnees_membres.insert({enfant->chaine(), { ds.donnees_types.size(), nullptr }});
					ds.donnees_types.push_back(enfant->index_type);
				}
			}

			break;
		}
		case type_noeud::DECLARATION_ENUM:
		{
			/* À FAIRE : vérification nom unique, valeur unique + test */
			break;
		}
		case type_noeud::ASSOCIE:
		case type_noeud::PAIRE_ASSOCIATION:
		{
			/* TESTS : si énum -> vérifie que toutes les valeurs soient prises
			 * en compte, sauf s'il y a un bloc sinon après. */
			valides_enfants(b, contexte);
			break;
		}
		case type_noeud::RETIENS:
		{
			contexte.donnees_fonction->est_coroutine = true;
			valides_enfants(b, contexte);

			auto enfant = b->enfants.front();

			if (enfant->index_type != contexte.donnees_fonction->index_type_retour) {
				auto const &dt_arg = contexte.magasin_types.donnees_types[contexte.donnees_fonction->index_type_retour];
				auto const &dt_enf = contexte.magasin_types.donnees_types[enfant->index_type];
				erreur::lance_erreur_type_retour(
							dt_arg,
							dt_enf,
							contexte,
							enfant->morceau,
							b->morceau);
			}

			auto debut = contexte.debut_locales();
			auto fin   = contexte.fin_locales();

			auto &donnees_coroutine = contexte.donnees_fonction->donnees_coroutine;
			donnees_coroutine.nombre_retenues += 1;

			auto &variables = donnees_coroutine.variables;

			for (; debut != fin; ++debut) {
				if (debut->second.est_argument) {
					continue;
				}

				auto duplique = false;

				for (auto const &var : variables) {
					if (var.first == debut->first) {
						duplique = true;
						break;
					}
				}

				if (!duplique) {
					variables.push_back({std::string(debut->first), {debut->second.donnees_type, debut->second.drapeaux}});
				}
			}

			break;
		}
	}
}

}  /* namespace noeud */
