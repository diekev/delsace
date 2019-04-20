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

#include "coulisse_c.hh"

#include <delsace/chrono/chronometrage.hh>

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "modules.hh"
#include "nombres.h"

namespace noeud {

/* ************************************************************************** */

template <typename Conteneur>
static void cree_appel(
		std::ostream &os,
		ContexteGenerationCode &contexte,
		std::string const &nom_broye,
		Conteneur const &enfants)
{
	for (auto enf : enfants) {
		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			auto nom_tableau = std::string("__tabl_dyn_").append(enf->chaine());
			enf->valeur_calculee = nom_tableau;

			os << "Tableau_";
			auto &dt = contexte.magasin_types.donnees_types[enf->donnees_type];
			contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);

			os << ' ' << nom_tableau << ";\n";
			os << nom_tableau << ".taille = " << static_cast<size_t>(dt.type_base() >> 8) << ";\n";
			os << nom_tableau << ".pointeur = " << enf->chaine() << ";\n";
		}
	}

	os << nom_broye;
	auto virgule = '(';

	for (auto enf : enfants) {
		os << virgule;

		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			os << std::any_cast<std::string>(enf->valeur_calculee);
		}
		else {
			genere_code_C(enf, contexte, false, os);
		}

		virgule = ',';
	}

	os << ')';
}

static void declare_structures_C(
		ContexteGenerationCode &contexte,
		std::ostream &os)
{
	for (auto const &paire : contexte.structures) {
		os << "typedef struct " << paire.first << "{\n";

		auto const &donnees = paire.second;

		for (auto i = 0ul; i < donnees.donnees_types.size(); ++i) {
			auto index_dt = donnees.donnees_types[i];
			auto &dt = contexte.magasin_types.donnees_types[index_dt];

			for (auto paire_idx_mb : donnees.index_membres) {
				if (paire_idx_mb.second == i) {
					auto nom = paire_idx_mb.first;

					auto est_tableau = contexte.magasin_types.converti_type_C(contexte,
								nom,
								dt,
								os);

					if (!est_tableau) {
						os << ' ' << nom;
					}

					os << ";\n";
					break;
				}
			}
		}

		os << "} " << paire.first << ";\n\n";
	}

	contexte.magasin_types.declare_structures_C(contexte, os);
}

void genere_code_C(
		base *b,
		ContexteGenerationCode &contexte,
		bool expr_gauche,
		std::ostream &os)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			auto temps_validation = 0.0;
			auto temps_generation = 0.0;

			for (auto noeud : b->enfants) {
				auto debut_validation = dls::chrono::maintenant();
				performe_validation_semantique(noeud, contexte);
				temps_validation += dls::chrono::delta(debut_validation);
			}

			declare_structures_C(contexte, os);

			for (auto noeud : b->enfants) {
				auto debut_generation = dls::chrono::maintenant();
				genere_code_C(noeud, contexte, false, os);
				temps_generation += dls::chrono::delta(debut_generation);
			}

			contexte.temps_generation = temps_generation;
			contexte.temps_validation = temps_validation;

			break;
		}
		case type_noeud::DECLARATION_FONCTION:
		{
			/* À FAIRE : inférence de type
			 * - considération du type de retour des fonctions récursive
			 * - il est possible que le retour dépende des variables locales de la
			 *   fonction, donc il faut d'abord générer le code ou faire une prépasse
			 *   pour générer les données nécessaires.
			 */

			auto module = contexte.module(static_cast<size_t>(b->morceau.module));
			auto &donnees_fonction = module->donnees_fonction(b->morceau.chaine);

			/* Pour les fonctions variadiques nous transformons la liste d'argument en
			 * un tableau dynamique transmis à la fonction. La raison étant que les
			 * instruction de LLVM pour les arguments variadiques ne fonctionnent
			 * vraiment que pour les types simples et les pointeurs. Pour pouvoir passer
			 * des structures, il faudrait manuellement gérer les instructions
			 * d'incrémentation et d'extraction des arguments pour chaque plateforme.
			 * Nos tableaux, quant à eux, sont portables.
			 */

			if (b->est_externe) {
				return;
			}

			/* broyage du nom */
			auto nom_module = contexte.module(static_cast<size_t>(b->morceau.module))->nom;
			auto nom_fonction = std::string(b->morceau.chaine);
			auto nom_broye = (b->est_externe || nom_module.empty()) ? nom_fonction : nom_module + '_' + nom_fonction;

			/* Crée fonction */

			auto est_tableau = contexte.magasin_types.converti_type_C(contexte,
						nom_broye,
						contexte.magasin_types.donnees_types[b->donnees_type],
					os);

			if (!est_tableau) {
				os << " " << nom_broye;
			}

			contexte.commence_fonction(nullptr);

			/* Crée code pour les arguments */

			auto virgule = '(';

			for (auto const &nom : donnees_fonction.nom_args) {
				os << virgule;

				auto &argument = donnees_fonction.args[nom];
				auto index_type = argument.donnees_type;

				/* à faire : type */
				//				if (argument.est_variadic) {
				//					auto dt = DonneesType{};
				//					dt.pousse(id_morceau::TABLEAU);
				//					dt.pousse(contexte.magasin_types.donnees_types[argument.donnees_type]);

				//				}
				//				else {
				//					dt = contexte.magasin_types.donnees_types[argument.donnees_type];
				//				}

				est_tableau = contexte.magasin_types.converti_type_C(contexte,
							nom,
							contexte.magasin_types.donnees_types[argument.donnees_type],
						os);

				if (!est_tableau) {
					os << " " << nom;
				}

				virgule = ',';

				contexte.pousse_locale(nom, nullptr, index_type, argument.est_dynamic, argument.est_variadic);
			}

			os << ")\n";

			/* Crée code pour le bloc. */
			auto bloc = b->enfants.front();
			os << "{\n";
			genere_code_C(bloc, contexte, false, os);
			os << "}\n";

			contexte.termine_fonction();

			break;
		}
		case type_noeud::APPEL_FONCTION:
		{
			/* À FAIRE */
			/* broyage du nom */
			auto module = contexte.module(static_cast<size_t>(b->module_appel));
			auto nom_module = module->nom;
			auto nom_fonction = std::string(b->morceau.chaine);
			auto nom_broye = nom_module.empty() ? nom_fonction : nom_module + '_' + nom_fonction;

			auto est_pointeur_fonction = (contexte.locale_existe(b->morceau.chaine));

			/* Cherche la liste d'arguments */
			if (est_pointeur_fonction) {
				auto index_type = contexte.type_locale(b->morceau.chaine);
				auto &dt_fonc = contexte.magasin_types.donnees_types[index_type];
				auto dt_params = donnees_types_parametres(dt_fonc);

				auto enfant = b->enfants.begin();

				/* Validation des types passés en paramètre. */
				for (size_t i = 0; i < dt_params.size() - 1; ++i) {
					auto &type_enf = contexte.magasin_types.donnees_types[(*enfant)->donnees_type];
					verifie_compatibilite(b, contexte, dt_params[i], type_enf, *enfant);
					++enfant;
				}

				cree_appel(os, contexte, nom_broye, b->enfants);
				return;
			}

			auto &donnees_fonction = module->donnees_fonction(b->morceau.chaine);

			auto fonction_variadique_interne = donnees_fonction.est_variadique && !donnees_fonction.est_externe;

			/* Réordonne les enfants selon l'apparition des arguments car LLVM est
			 * tatillon : ce n'est pas l'ordre dans lequel les valeurs apparaissent
			 * dans le vecteur de paramètres qui compte, mais l'ordre dans lequel le
			 * code est généré. */
			auto noms_arguments = std::any_cast<std::list<std::string_view>>(&b->valeur_calculee);
			std::vector<base *> enfants;

			if (fonction_variadique_interne) {
				enfants.resize(donnees_fonction.args.size());
			}
			else {
				enfants.resize(noms_arguments->size());
			}

			auto noeud_tableau = static_cast<base *>(nullptr);

			if (fonction_variadique_interne) {
				/* Pour les fonctions variadiques interne, nous créons un tableau
				 * correspondant au types des arguments. */

				auto nombre_args = donnees_fonction.args.size();
				auto nombre_args_var = std::max(0ul, noms_arguments->size() - (nombre_args - 1));
				auto index_premier_var_arg = nombre_args - 1;

				noeud_tableau = new base(contexte, {});
				noeud_tableau->valeur_calculee = static_cast<long>(nombre_args_var);
				noeud_tableau->calcule = true;
				auto nom_arg = donnees_fonction.nom_args.back();
				noeud_tableau->donnees_type = donnees_fonction.args[nom_arg].donnees_type;

				enfants[index_premier_var_arg] = noeud_tableau;
			}

			auto enfant = b->enfants.begin();
			auto nombre_arg_variadic = 0ul;

			for (auto const &nom : *noms_arguments) {
				/* Pas la peine de vérifier qu'iter n'est pas égal à la fin de la table
				 * car ça a déjà été fait dans l'analyse grammaticale. */
				auto const iter = donnees_fonction.args.find(nom);
				auto index_arg = iter->second.index;
				auto const index_type_arg = iter->second.donnees_type;
				auto const index_type_enf = (*enfant)->donnees_type;
				auto const &type_arg = index_type_arg == -1ul ? DonneesType{} : contexte.magasin_types.donnees_types[index_type_arg];
				auto const &type_enf = contexte.magasin_types.donnees_types[index_type_enf];

				if (iter->second.est_variadic) {
					if (!type_arg.est_invalide()) {
						verifie_compatibilite(b, contexte, type_arg, type_enf, *enfant);

						if (noeud_tableau) {
							noeud_tableau->ajoute_noeud(*enfant);
						}
						else {
							enfants[index_arg + nombre_arg_variadic] = *enfant;
							++nombre_arg_variadic;
						}
					}
					else {
						enfants[index_arg + nombre_arg_variadic] = *enfant;
						++nombre_arg_variadic;
					}
				}
				else {
					verifie_compatibilite(b, contexte, type_arg, type_enf, *enfant);

					enfants[index_arg] = *enfant;
				}

				++enfant;
			}

			cree_appel(os, contexte, nom_broye, enfants);

			delete noeud_tableau;

			break;
		}
		case type_noeud::VARIABLE:
		{
			auto drapeaux = contexte.drapeaux_variable(b->morceau.chaine);

			if ((drapeaux & BESOIN_DEREF) != 0) {
				os << '*';
			}

			os << b->morceau.chaine;
			break;
		}
		case type_noeud::ACCES_MEMBRE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();

			auto const &index_type = structure->donnees_type;
			auto type_structure = contexte.magasin_types.donnees_types[index_type];

			auto est_pointeur = type_structure.type_base() == id_morceau::POINTEUR;

			if (est_pointeur) {
				type_structure = type_structure.derefence();
			}

			if ((type_structure.type_base() & 0xff) == id_morceau::TABLEAU) {
				if (!contexte.non_sur() && expr_gauche) {
					erreur::lance_erreur(
								"Modification des membres du tableau hors d'un bloc 'nonsûr' interdite",
								contexte,
								b->morceau,
								erreur::type_erreur::ASSIGNATION_INVALIDE);
				}

				auto taille = static_cast<size_t>(type_structure.type_base() >> 8);

				if (taille != 0) {
					os << taille;
					return;
				}
			}

			os << structure->chaine() << ((est_pointeur) ? "->" : ".") << membre->chaine();
			break;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			/* À FAIRE */
			break;
		}
		case type_noeud::CONSTANTE:
		{
			/* À FAIRE : énumération avec des expressions contenant d'autres énums.
			 * différents types (réel, bool, etc..)
			 */

			auto n = converti_chaine_nombre_entier(
						b->enfants.front()->chaine(),
						b->enfants.front()->identifiant());

			os << "int " << b->morceau.chaine << '=' << n << ';';

			contexte.pousse_globale(b->morceau.chaine, nullptr, b->donnees_type, false);
			break;
		}
		case type_noeud::DECLARATION_VARIABLE:
		{
			auto est_tableau = contexte.magasin_types.converti_type_C(contexte,
						b->chaine(),
						contexte.magasin_types.donnees_types[b->donnees_type],
					os);

			if (!est_tableau) {
				os << " " << b->chaine();
			}

			if ((b->drapeaux & GLOBAL) != 0) {
				contexte.pousse_globale(b->chaine(), nullptr, b->donnees_type, (b->drapeaux & DYNAMIC) != 0);
				return;
			}

			/* À FAIRE : déplace ça */
			/* Mets à zéro les valeurs des tableaux dynamics. */
			//			if (type.type_base() == id_morceau::TABLEAU) {
			//				os << b->chaine() << ".pointeur = 0;";
			//				os << b->chaine() << ".taille = 0;";
			//			}

			contexte.pousse_locale(b->morceau.chaine, nullptr, b->donnees_type, (b->drapeaux & DYNAMIC) != 0, false);

			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.size() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);

			/* À FAIRE */
			if (compatibilite == niveau_compat::converti_tableau) {
				expression->drapeaux |= CONVERTI_TABLEAU;
			}

			genere_code_C(variable, contexte, true, os);
			os << " = ";
			genere_code_C(expression, contexte, false, os);

			break;
		}
		case type_noeud::NOMBRE_REEL:
		{
			auto const valeur = b->calcule ? std::any_cast<double>(b->valeur_calculee) :
											 converti_chaine_nombre_reel(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			os << valeur;
			break;
		}
		case type_noeud::NOMBRE_ENTIER:
		{
			auto const valeur = b->calcule ? std::any_cast<long>(b->valeur_calculee) :
											 converti_chaine_nombre_entier(
												 b->morceau.chaine,
												 b->morceau.identifiant);

			os << valeur;
			break;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			auto const index_type1 = enfant1->donnees_type;
			auto const index_type2 = enfant2->donnees_type;

			auto const &type1 = contexte.magasin_types.donnees_types[index_type1];
			auto &type2 = contexte.magasin_types.donnees_types[index_type2];

			if ((b->morceau.identifiant != id_morceau::CROCHET_OUVRANT)) {
				if (!peut_operer(type1, type2, enfant1->type, enfant2->type)) {
					erreur::lance_erreur_type_operation(
								type1,
								type2,
								contexte,
								b->morceau);
				}
			}

			/* À FAIRE : typage */

			/* Ne crée pas d'instruction de chargement si nous avons un tableau. */
			auto const valeur2_brut = ((type2.type_base() & 0xff) == id_morceau::TABLEAU);

			switch (b->morceau.identifiant) {
				default:
				{
					genere_code_C(enfant1, contexte, false, os);
					os << b->morceau.chaine;
					genere_code_C(enfant2, contexte, valeur2_brut, os);
					break;
				}
				case id_morceau::CROCHET_OUVRANT:
				{
					if (type2.type_base() != id_morceau::POINTEUR && (type2.type_base() & 0xff) != id_morceau::TABLEAU) {
						erreur::lance_erreur(
									"Le type ne peut être déréférencé !",
									contexte,
									b->morceau,
									erreur::type_erreur::TYPE_DIFFERENTS);
					}

					os << enfant2->chaine();
					os << '[';
					genere_code_C(enfant1, contexte, valeur2_brut, os);
					os << ']';
					break;
				}
			}

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();

			switch (b->morceau.identifiant) {
				case id_morceau::EXCLAMATION:
				{
					os << "!" << enfant->morceau.chaine;
					break;
				}
				case id_morceau::TILDE:
				{
					os << "~" << enfant->morceau.chaine;
					break;
				}
				case id_morceau::AROBASE:
				{
					os << "&" << enfant->morceau.chaine;
					break;
				}
				case id_morceau::PLUS_UNAIRE:
				{
					os << enfant->morceau.chaine;
					break;
				}
				case id_morceau::MOINS_UNAIRE:
				{
					os << "-" << enfant->morceau.chaine;
					break;
				}
				default:
				{
					break;
				}
			}

			break;
		}
		case type_noeud::RETOUR:
		{
			/* À FAIRE : différé. */

			os << "return ";

			if (!b->enfants.empty()) {
				assert(b->enfants.size() == 1);
				genere_code_C(b->enfants.front(), contexte, false, os);
			}

			/* NOTE : le code différé doit être crée après l'expression de retour, car
			 * nous risquerions par exemple de déloger une variable utilisée dans
			 * l'expression de retour. */
			//genere_code_extra_pre_retour(contexte);

			break;
		}
		case type_noeud::CHAINE_LITTERALE:
		{
			auto chaine = std::any_cast<std::string>(b->valeur_calculee);
			os << '"';

			for (auto c : chaine) {
				if (c == '\n') {
					os << '\\' << 'n';
				}
				else if (c == '\t') {
					os << '\\' << 't';
				}
				else {
					os << c;
				}
			}

			os << '"';
			break;
		}
		case type_noeud::BOOLEEN:
		{
			auto const valeur = b->calcule ? std::any_cast<bool>(b->valeur_calculee)
										   : (b->chaine() == "vrai");
			os << valeur;
			break;
		}
		case type_noeud::CARACTERE:
		{
			auto valeur = caractere_echape(&b->morceau.chaine[0]);
			os << '\'' << valeur << '\'';
			break;
		}
		case type_noeud::SI:
		{
			auto const nombre_enfants = b->enfants.size();
			auto iter_enfant = b->enfants.begin();

			/* noeud 1 : condition */
			auto enfant1 = *iter_enfant++;

			os << "if (";
			genere_code_C(enfant1, contexte, false, os);
			os << ") {\n";

			/* noeud 2 : bloc */
			auto enfant2 = *iter_enfant++;
			genere_code_C(enfant2, contexte, false, os);
			os << "}\n";

			/* noeud 3 : sinon (optionel) */
			if (nombre_enfants == 3) {
				os << "else {\n";
				auto enfant3 = *iter_enfant++;
				genere_code_C(enfant3, contexte, false, os);
				os << "}\n";
			}

			break;
		}
		case type_noeud::BLOC:
		{
			contexte.empile_nombre_locales();

			for (auto enfant : b->enfants) {
				genere_code_C(enfant, contexte, true, os);
				os << ";\n";
			}

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::POUR:
		{
			//auto nombre_enfants = b->enfants.size();
			auto iter = b->enfants.begin();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;

			/* À FAIRE */
			//auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			//auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;


			//auto enfant_sans_arret = enfant4;
			//auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

			auto index_type = enfant2->donnees_type;
			auto const &type_debut = contexte.magasin_types.donnees_types[index_type];
			auto const type = type_debut.type_base();

			enfant1->donnees_type = index_type;

			contexte.empile_nombre_locales();

			if (enfant2->type == type_noeud::PLAGE) {
				os << "\nfor (int " << enfant1->chaine() << " = ";
				genere_code_C(enfant2->enfants.front(), contexte, false, os);

				os << "; "
				   << enfant1->chaine() << " <= ";

				genere_code_C(enfant2->enfants.back(), contexte, false, os);
				os <<"; ++" << enfant1->chaine()
				  << ") {\n";

				contexte.pousse_locale(enfant1->chaine(), index_type, 0);
			}
			else if (enfant2->type == type_noeud::VARIABLE) {
				const auto taille_tableau = static_cast<uint64_t>(type >> 8);

				/* À FAIRE: nom unique pour les boucles dans les boucles */

				if (taille_tableau != 0) {
					os << "\nfor (int __i = 0; __i <= " << taille_tableau << "-1; ++__i) {\n";
					contexte.magasin_types.converti_type_C(contexte, "", type_debut.derefence(), os);
					os << " *" << enfant1->chaine() << " = &" << enfant2->chaine() << "[__i];\n";
				}
				else {
					os << "\nfor (int __i = 0; __i <= " << enfant2->chaine() << ".taille - 1; ++__i) {\n";
					contexte.magasin_types.converti_type_C(contexte, "", type_debut.derefence(), os);
					os << " *" << enfant1->chaine() << " = &" << enfant2->chaine() << ".pointeur[__i];\n";
				}

				contexte.pousse_locale(enfant1->chaine(), index_type, BESOIN_DEREF);
			}

			genere_code_C(enfant3, contexte, false, os);

			os << "}\n";

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
			//			auto chaine_var = b->enfants.empty() ? std::string_view{""} : b->enfants.front()->chaine();

			//			auto bloc = (b->morceau.identifiant == id_morceau::CONTINUE)
			//						? contexte.bloc_continue(chaine_var)
			//						: contexte.bloc_arrete(chaine_var);

			//			if (bloc == nullptr) {
			//				if (chaine_var.empty()) {
			//					erreur::lance_erreur(
			//								"'continue' ou 'arrête' en dehors d'une boucle",
			//								contexte,
			//								b->morceau,
			//								erreur::type_erreur::CONTROLE_INVALIDE);
			//				}
			//				else {
			//					erreur::lance_erreur(
			//								"Variable inconnue",
			//								contexte,
			//								b->enfants.front()->donnees_morceau(),
			//								erreur::type_erreur::VARIABLE_INCONNUE);
			//				}
			//			}

			/* À FAIRE : variable de continuation ou d'arrête. */
			if (b->morceau.identifiant == id_morceau::ARRETE) {
				os << " break ";
			}
			else {
				os << " continue ";
			}
			break;
		}
		case type_noeud::BOUCLE:
		{
			/* boucle:
			 *	corps
			 *  br boucle
			 *
			 * apres_boucle:
			 *	...
			 */

			/* À FAIRE : bloc sinon. */

			auto iter = b->enfants.begin();
			auto enfant1 = *iter++;
			//	auto enfant2 = (b->enfants.size() == 2) ? *iter++ : nullptr;

			/* création des blocs */

			//	contexte.empile_bloc_continue("", bloc_boucle);
			//contexte.empile_bloc_arrete("", (enfant2 != nullptr) ? bloc_sinon : bloc_apres);

			os << "while true {\n";

			/* on crée une branche explicite dans le bloc */
			genere_code_C(enfant1, contexte, false, os);

			os << "}\n";

			//			if (false) {
			//				genere_code_C(enfant2, contexte, false, os);
			//			}

			contexte.depile_bloc_continue();
			contexte.depile_bloc_arrete();

			break;
		}
		case type_noeud::TRANSTYPE:
		{
			/* À FAIRE */
			break;
		}
		case type_noeud::NUL:
		{
			os << "0";
			break;
		}
		case type_noeud::TAILLE_DE:
		{
			auto index_dt = std::any_cast<size_t>(b->valeur_calculee);
			auto const &donnees = contexte.magasin_types.donnees_types[index_dt];
			os << "sizeof(";
			contexte.magasin_types.converti_type_C(contexte, "", donnees, os);
			os << ")";
			break;
		}
		case type_noeud::PLAGE:
		{
			/* pris en charge dans type_noeud::POUR */
			break;
		}
		case type_noeud::DIFFERE:
		{
			auto noeud = b->enfants.front();
			contexte.differe_noeud(noeud);
			break;
		}
		case type_noeud::NONSUR:
		{
			contexte.non_sur(true);
			genere_code_C(b->enfants.front(), contexte, false, os);
			contexte.non_sur(false);
			break;
		}
		case type_noeud::TABLEAU:
		{
			/* converti un tableau fixe un en tableau dynamic */

			//			auto taille_tableau = b->enfants.size();

			//			if (b->calcule) {
			//				assert(static_cast<long>(taille_tableau) == std::any_cast<long>(b->valeur_calculee));
			//			}

			//			auto &type = contexte.magasin_types.donnees_types[b->donnees_type];

			//			/* alloue un tableau fixe */
			//			auto dt_tfixe = DonneesType{};
			//			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			//			dt_tfixe.pousse(type);

			//			auto type_llvm = contexte.magasin_types.converti_type(contexte, dt_tfixe);

			//			auto pointeur_tableau = new llvm::AllocaInst(
			//										type_llvm,
			//										0,
			//										nullptr,
			//										"",
			//										contexte.bloc_courant());

			//			/* copie les valeurs dans le tableau fixe */
			//			auto index = 0ul;

			//			for (auto enfant : b->enfants) {
			//				auto valeur_enfant = genere_code_enfant(contexte, enfant);

			//				auto index_tableau = accede_element_tableau(
			//										 contexte,
			//										 pointeur_tableau,
			//										 type_llvm,
			//										 index++);

			//				new llvm::StoreInst(valeur_enfant, index_tableau, contexte.bloc_courant());
			//			}

			//			/* alloue un tableau dynamique */
			//			return converti_vers_tableau_dyn(contexte, pointeur_tableau, dt_tfixe);
			break;
		}
		case type_noeud::CONSTRUIT_STRUCTURE:
		{
			auto liste_params = std::any_cast<std::vector<std::string_view>>(&b->valeur_calculee);

			auto enfant = b->enfants.begin();
			auto nom_param = liste_params->begin();
			auto virgule = '{';

			for (auto i = 0ul; i < liste_params->size(); ++i) {
				os << virgule;

				os << '.' << *nom_param << '=';
				genere_code_C(*enfant, contexte, expr_gauche, os);
				++enfant;
				++nom_param;

				virgule = ',';
			}

			os << '}';

			break;
		}
		case type_noeud::CONSTRUIT_TABLEAU:
		{
			std::vector<base *> feuilles;
			rassemble_feuilles(b, feuilles);

			auto virgule = '{';

			for (auto f : feuilles) {
				os << virgule;
				genere_code_C(f, contexte, false, os);
				virgule = ',';
			}

			os << '}';

			break;
		}
	}
}

}  /* namespace noeud */
