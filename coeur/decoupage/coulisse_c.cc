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

#include <iostream>

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "erreur.h"
#include "modules.hh"
#include "nombres.h"

namespace noeud {

/* ************************************************************************** */

#if 0
struct Tampon {
	char tampon[16384];
	char *ptr;
};

// imprime(tampon, "for (%0 %1 = %2; %1 <= %3 - 1; ++%1) {\n", {str_type, str_nom, str_expr_debut, str_expr_fin});

static auto imprime(
		Tampon &tampon,
		const char *chn_fmt,
		std::vector<const char *> const &args)
{
	auto ptr = chn_fmt;
	auto ptr_tampon = tampon.ptr;

	for (;*ptr != '\0'; ++ptr) {
		if (*ptr != '%') {
			*ptr_tampon++ = *ptr;
			continue;
		}

		auto suivant = *(ptr + 1);
		++ptr;

		if (suivant == '%') {
			*ptr_tampon++ = suivant;
			continue;
		}

		auto index = static_cast<size_t>('0' - *ptr);

		auto chn_arg = args[index];

		while (*chn_arg != '\0') {
			*ptr_tampon++ = *chn_arg++;
		}
	}
}
#endif

/* À FAIRE : trouve une bonne manière de générer des noms uniques. */
/* À FAIRE : variables globales pour les info types. */
static int index = 0;

static auto cree_info_type_defaul_C(
		std::ostream &os,
		std::string const &id_type,
		int nombre_base,
		int profondeur)
{
	auto nom_info_type = "__info_type"
			+ id_type
			+ std::to_string(nombre_base)
			+ std::to_string(profondeur)
			+ std::to_string(index++);

	os << "InfoTypeEntier " << nom_info_type << ";\n";
	os << nom_info_type << ".id = " << id_type << ";\n";

	return nom_info_type;
}

static auto cree_info_type_entier_C(
		std::ostream &os,
		int taille_en_octet,
		bool est_signe,
		int nombre_base,
		int profondeur)
{
	auto nom_info_type = "__info_type_entier"
			+ std::to_string(nombre_base)
			+ std::to_string(profondeur)
			+ std::to_string(index++);

	os << "InfoTypeEntier " << nom_info_type << ";\n";
	os << nom_info_type << ".id = TYPE_ENTIER;\n";
	os << nom_info_type << ".est_signe = " << est_signe << ";\n";
	os << nom_info_type << ".taille_en_octet = " << taille_en_octet << ";\n";

	return nom_info_type;
}

static auto cree_info_type_reel_C(
		std::ostream &os,
		int taille_en_octet,
		int nombre_base,
		int profondeur)
{
	auto nom_info_type = "__info_type_reel"
			+ std::to_string(nombre_base)
			+ std::to_string(profondeur)
			+ std::to_string(index++);

	os << "InfoTypeReel " << nom_info_type << ";\n";
	os << nom_info_type << ".id = TYPE_REEL;\n";
	os << nom_info_type << ".taille_en_octet = " << taille_en_octet << ";\n";

	return nom_info_type;
}

static int taille_type_octet(DonneesType const &donnees_type)
{
	switch (donnees_type.type_base() & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
		{
			return 1;
		}
		case id_morceau::N16:
		case id_morceau::Z16:
		{
			return 2;
		}
		case id_morceau::R16:
		{
			/* À FAIRE : type r16 */
			return 4;
		}
		case id_morceau::N32:
		case id_morceau::Z32:
		case id_morceau::R32:
		{
			return 4;
		}
		case id_morceau::N64:
		case id_morceau::Z64:
		case id_morceau::R64:
		{
			return 8;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			/* À FAIRE */
			return 0;
		}
		case id_morceau::POINTEUR:
		case id_morceau::FONCTION:
		{
			/* À FAIRE : pointeur 32-bit/64-bit */
			return 8;
		}
		case id_morceau::TABLEAU:
		case id_morceau::EINI:
		case id_morceau::CHAINE:
		{
			return 16;
		}
		case id_morceau::RIEN:
		{
			return 0;
		}
	}
}

static std::string cree_info_type_C(
		ContexteGenerationCode &contexte,
		std::ostream &os,
		DonneesType const &donnees_type,
		int nombre_base,
		int profondeur)
{
	auto valeur = std::string("");

	switch (donnees_type.type_base() & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case id_morceau::BOOL:
		{
			valeur = cree_info_type_defaul_C(os, "TYPE_BOOLEEN", nombre_base, profondeur);
			break;
		}
		case id_morceau::N8:
		{
			valeur = cree_info_type_entier_C(os, 8, false, nombre_base, profondeur);
			break;
		}
		case id_morceau::Z8:
		{
			valeur = cree_info_type_entier_C(os, 8, true, nombre_base, profondeur);
			break;
		}
		case id_morceau::N16:
		{
			valeur = cree_info_type_entier_C(os, 16, false, nombre_base, profondeur);
			break;
		}
		case id_morceau::Z16:
		{
			valeur = cree_info_type_entier_C(os, 16, true, nombre_base, profondeur);
			break;
		}
		case id_morceau::N32:
		{
			valeur = cree_info_type_entier_C(os, 32, false, nombre_base, profondeur);
			break;
		}
		case id_morceau::Z32:
		{
			valeur = cree_info_type_entier_C(os, 32, true, nombre_base, profondeur);
			break;
		}
		case id_morceau::N64:
		{
			valeur = cree_info_type_entier_C(os, 64, false, nombre_base, profondeur);
			break;
		}
		case id_morceau::Z64:
		{
			valeur = cree_info_type_entier_C(os, 64, true, nombre_base, profondeur);
			break;
		}
		case id_morceau::R16:
		{
			/* À FAIRE : type r16 */
			valeur = cree_info_type_reel_C(os, 32, nombre_base, profondeur);
			break;
		}
		case id_morceau::R32:
		{
			valeur = cree_info_type_reel_C(os, 32, nombre_base, profondeur);
			break;
		}
		case id_morceau::R64:
		{
			valeur = cree_info_type_reel_C(os, 64, nombre_base, profondeur);
			break;
		}
		case id_morceau::POINTEUR:
		{
			auto deref = donnees_type.derefence();
			auto valeur_pointee = cree_info_type_C(contexte, os, deref, nombre_base, profondeur + 1);

			auto nom_info_type = "__info_type_tableau"
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			os << "InfoTypePointeur " << nom_info_type << ";\n";
			os << nom_info_type << ".id = TYPE_POINTEUR;\n";
			os << nom_info_type << ".type_pointe = (InfoType *)(&" << valeur_pointee << ");\n";

			valeur = nom_info_type;
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto id_structure = static_cast<size_t>(donnees_type.type_base() >> 8);
			auto donnees_structure = contexte.donnees_structure(id_structure);

			auto nom_info_type = "__info_type_struct"
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			/* crée la chaîne pour le nom */
			auto nom_struct = contexte.nom_struct(id_structure);
			auto nom_chaine = "__nom_"
					+ nom_struct
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			os << "chaine " << nom_chaine << " = ";
			os << "{.pointeur = \"" << nom_struct << "\""
			   << ", .taille = " << nom_struct.size() << "};\n";

			os << "InfoTypeStructure " << nom_info_type << ";\n";
			os << nom_info_type << ".id = TYPE_STRUCTURE;\n";
			os << nom_info_type << ".nom = " << nom_chaine << ";\n";

			/* crée un tableau fixe puis converti le en tableau dyn */
			auto nombre_membres = donnees_structure.donnees_types.size();

			/* crée des structures pour chaque membre, et rassemble les pointeurs */
			std::vector<std::string> pointeurs;
			pointeurs.reserve(nombre_membres);

			auto decalage = 0;

			for (auto i = 0ul; i < donnees_structure.donnees_types.size(); ++i) {
				auto index_dt = donnees_structure.donnees_types[i];
				auto &dt = contexte.magasin_types.donnees_types[index_dt];

				for (auto paire_idx_mb : donnees_structure.index_membres) {
					if (paire_idx_mb.second != i) {
						continue;
					}

					auto suffixe = std::to_string(i)
							+ std::to_string(nombre_base)
							+ std::to_string(profondeur);

					auto nom_info_type_membre = "__info_type_membre" + suffixe;

					auto struct_info_type_membre = cree_info_type_C(
								contexte, os, dt, nombre_base + static_cast<int>(i), profondeur + 1);

					os << "InfoTypeMembreStructure " << nom_info_type_membre << ";\n";
					os << nom_info_type_membre << ".nom.pointeur = \"" << paire_idx_mb.first << "\";\n";
					os << nom_info_type_membre << ".nom.taille = " << paire_idx_mb.first.size()  << ";\n";
					os << nom_info_type_membre << ".decalage = " << decalage  << ";\n";
					os << nom_info_type_membre << ".id = (InfoType *)(&" << struct_info_type_membre  << ");\n";

					decalage += taille_type_octet(dt);

					pointeurs.push_back(nom_info_type_membre);
					break;
				}
			}

			/* alloue un tableau fixe pour stocker les pointeurs */
			auto idx_type_tabl = contexte.donnees_structure("InfoTypeMembreStructure").id;
			auto type_struct_membre = DonneesType{};
			type_struct_membre.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(idx_type_tabl << 8));

			auto dt_tfixe = DonneesType{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(nombre_membres << 8));
			dt_tfixe.pousse(type_struct_membre);

			auto nom_tableau_fixe = std::string("__tabl_fix_membres")
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tfixe, os);

			os << " = ";

			auto virgule = '{';

			for (auto const &ptr : pointeurs) {
				os << virgule;
				os << ptr;
				virgule = ',';
			}

			os << "};\n";

			/* alloue un tableau dynamique */
			auto dt_tdyn = DonneesType{};
			dt_tdyn.pousse(id_morceau::TABLEAU);
			dt_tdyn.pousse(type_struct_membre);

			auto nom_tableau_dyn = std::string("__tabl_dyn_membres")
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tdyn, os);

			os << ' ' << nom_tableau_dyn << ";\n";
			os << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			os << nom_tableau_dyn << ".taille = " << nombre_membres << ";\n";

			os << nom_info_type << ".membres = " << nom_tableau_dyn << ";\n";

			valeur = nom_info_type;
			break;
		}
		case id_morceau::TABLEAU:
		{
			auto deref = donnees_type.derefence();
			auto valeur_pointee = cree_info_type_C(contexte, os, deref, nombre_base, profondeur + 1);

			auto nom_info_type = "__info_type_tableau"
					+ std::to_string(nombre_base)
					+ std::to_string(profondeur);

			os << "InfoTypeTableau " << nom_info_type << ";\n";
			os << nom_info_type << ".id = TYPE_TABLEAU;\n";
			os << nom_info_type << ".type_pointe = (InfoType *)(&" << valeur_pointee << ");\n";

			valeur = nom_info_type;
			break;
		}
		case id_morceau::FONCTION:
		{
			/* À FAIRE : type params */
			valeur = cree_info_type_defaul_C(os, "TYPE_FONCTION", nombre_base, profondeur);
			break;
		}
		case id_morceau::EINI:
		{
			valeur = cree_info_type_defaul_C(os, "TYPE_EINI", nombre_base, profondeur);
			break;
		}
		case id_morceau::RIEN:
		{
			valeur = cree_info_type_defaul_C(os, "TYPE_RIEN", nombre_base, profondeur);
			break;
		}
		case id_morceau::CHAINE:
		{
			valeur = cree_info_type_defaul_C(os, "TYPE_CHAINE", nombre_base, profondeur);
			break;
		}
	}

	return valeur;
}

static auto cree_eini(ContexteGenerationCode &contexte, std::ostream &os, base *b)
{
	auto nom_eini = std::string("__eini_")
			.append(std::to_string(b->morceau.ligne_pos));

	auto nom_var = std::string(b->chaine());

	auto &dt = contexte.magasin_types.donnees_types[b->index_type];

	/* dans le cas d'un nombre ou d'un tableau, etc. */
	if (b->type != type_noeud::VARIABLE) {
		nom_var = std::string("__var_").append(nom_eini);

		auto est_tableau = contexte.magasin_types.converti_type_C(
					contexte,
					nom_var,
					dt,
					os);

		if (!est_tableau) {
			os << " " << nom_var;
		}

		os << " = ";
		genere_code_C(b, contexte, false, os);
		os << ";\n";
	}

	auto nom_info_type = cree_info_type_C(
				contexte,
				os,
				dt,
				static_cast<int>(b->morceau.ligne_pos >> 32),
				0);

	os << "eini " << nom_eini << ";\n";
	os << nom_eini << ".pointeur = &" << nom_var << ";\n";
	os << nom_eini << ".info = (InfoType *)(&" << nom_info_type << ");\n";

	return nom_eini;
}

template <typename Conteneur>
static void cree_appel(
		base *b,
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
			auto &dt = contexte.magasin_types.donnees_types[enf->index_type];
			contexte.magasin_types.converti_type_C(contexte, "", dt.derefence(), os);

			os << ' ' << nom_tableau << ";\n";
			os << nom_tableau << ".taille = " << static_cast<size_t>(dt.type_base() >> 8) << ";\n";
			os << nom_tableau << ".pointeur = " << enf->chaine() << ";\n";
		}
		else if ((enf->drapeaux & CONVERTI_EINI) != 0) {
			enf->valeur_calculee = cree_eini(contexte, os, enf);
		}
		else if ((enf->drapeaux & EXTRAIT_CHAINE_C) != 0) {
			auto nom_var_chaine = std::string("");

			if (enf->type != type_noeud::VARIABLE) {
				nom_var_chaine = "__chaine" + std::to_string(enf->morceau.ligne_pos);

				os << "chaine " << nom_var_chaine << " = ";
				genere_code_C(enf, contexte, false, os);
				os << ";\n";
			}
			else {
				nom_var_chaine = enf->chaine();
			}

			auto nom_var = "__pointeur" + std::to_string(enf->morceau.ligne_pos);

			os << "const char *" + nom_var;
			os << " = " << nom_var_chaine << ".pointeur;\n";

			enf->valeur_calculee = nom_var;
		}
		else if (enf->type == type_noeud::TABLEAU) {
			genere_code_C(enf, contexte, false, os);
		}
	}

	if ((b->drapeaux & INDIRECTION_APPEL) != 0) {
		auto nom_indirection = "__ret_" + nom_broye + std::to_string(b->morceau.ligne_pos);
		auto &dt = contexte.magasin_types.donnees_types[b->index_type];
		auto est_tableau = contexte.magasin_types.converti_type_C(contexte, nom_indirection, dt, os);

		if (!est_tableau) {
			os << ' ' << nom_indirection;
		}

		os << " = ";
		b->valeur_calculee = nom_indirection;
	}

	os << nom_broye;

	if (enfants.size() == 0) {
		os << '(';
	}

	auto virgule = '(';

	for (auto enf : enfants) {
		os << virgule;

		if ((enf->drapeaux & CONVERTI_TABLEAU) != 0) {
			os << std::any_cast<std::string>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & CONVERTI_EINI) != 0) {
			os << std::any_cast<std::string>(enf->valeur_calculee);
		}
		else if ((enf->drapeaux & EXTRAIT_CHAINE_C) != 0) {
			os << std::any_cast<std::string>(enf->valeur_calculee);
		}
		else if (enf->type == type_noeud::TABLEAU) {
			os << std::any_cast<std::string>(enf->valeur_calculee);
		}
		else {
			genere_code_C(enf, contexte, false, os);
		}

		virgule = ',';
	}

	os << ')';

	if ((b->drapeaux & INDIRECTION_APPEL) != 0) {
		os << ';';
	}
}

static void declare_structures_C(
		ContexteGenerationCode &contexte,
		std::ostream &os)
{
	contexte.magasin_types.declare_structures_C(contexte, os);

	/* À FAIRE : optimise. */
	for (auto is = 0ul; is < contexte.structures.size(); ++is) {
		auto const &nom_struct = contexte.nom_struct(is);
		os << "typedef struct " << contexte.nom_struct(is) << "{\n";

		auto const &donnees = contexte.donnees_structure(is);

		for (auto i = 0ul; i < donnees.donnees_types.size(); ++i) {
			auto index_dt = donnees.donnees_types[i];
			auto &dt = contexte.magasin_types.donnees_types[index_dt];

			for (auto paire_idx_mb : donnees.index_membres) {
				if (paire_idx_mb.second == i) {
					auto nom = paire_idx_mb.first;

					auto est_tableau = contexte.magasin_types.converti_type_C(
								contexte,
								nom,
								dt,
								os,
								false,
								true);

					if (!est_tableau) {
						os << ' ' << nom;
					}

					os << ";\n";
					break;
				}
			}
		}

		os << "} " << nom_struct << ";\n\n";
	}
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
						contexte.magasin_types.donnees_types[b->index_type],
					os);

			if (!est_tableau) {
				os << " " << nom_broye;
			}

			contexte.commence_fonction(nullptr);

			/* Crée code pour les arguments */

			if (donnees_fonction.nom_args.size() == 0) {
				os << '(';
			}

			auto virgule = '(';

			for (auto const &nom : donnees_fonction.nom_args) {
				os << virgule;

				auto &argument = donnees_fonction.args[nom];
				auto index_type = argument.donnees_type;

				auto dt = DonneesType{};

				if (argument.est_variadic) {
					dt.pousse(id_morceau::TABLEAU);
					dt.pousse(contexte.magasin_types.donnees_types[argument.donnees_type]);

					contexte.magasin_types.ajoute_type(dt);
				}
				else {
					dt = contexte.magasin_types.donnees_types[argument.donnees_type];
				}

				est_tableau = contexte.magasin_types.converti_type_C(
							contexte,
							nom,
							dt,
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
					auto &type_enf = contexte.magasin_types.donnees_types[(*enfant)->index_type];
					verifie_compatibilite(b, contexte, dt_params[i], type_enf, *enfant);
					++enfant;
				}

				cree_appel(b, os, contexte, nom_broye, b->enfants);
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

				noeud_tableau = new base(contexte, b->morceau);
				noeud_tableau->type = type_noeud::TABLEAU;
				noeud_tableau->valeur_calculee = static_cast<long>(nombre_args_var);
				noeud_tableau->calcule = true;
				auto nom_arg = donnees_fonction.nom_args.back();
				noeud_tableau->index_type = donnees_fonction.args[nom_arg].donnees_type;

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
				auto const index_type_enf = (*enfant)->index_type;
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

			cree_appel(b, os, contexte, nom_broye, enfants);

			delete noeud_tableau;

			break;
		}
		case type_noeud::VARIABLE:
		{
			auto drapeaux = contexte.drapeaux_variable(b->morceau.chaine);

			if ((drapeaux & BESOIN_DEREF) != 0) {
				os << "(*" << b->chaine() << ")";
			}
			else {
				os << b->chaine();
			}

			break;
		}
		case type_noeud::ACCES_MEMBRE:
		{
			auto structure = b->enfants.back();
			auto membre = b->enfants.front();

			auto const &index_type = structure->index_type;
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

			genere_code_C(structure, contexte, true, os);
			os << ((est_pointeur) ? "->" : ".") << membre->chaine();

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

			os << "static int " << b->morceau.chaine << '=' << n << ";\n";

			contexte.pousse_globale(b->morceau.chaine, nullptr, b->index_type, false);
			break;
		}
		case type_noeud::DECLARATION_VARIABLE:
		{
			auto est_tableau = contexte.magasin_types.converti_type_C(contexte,
						b->chaine(),
						contexte.magasin_types.donnees_types[b->index_type],
					os);

			if (!est_tableau) {
				os << " " << b->chaine();
			}

			if ((b->drapeaux & GLOBAL) != 0) {
				contexte.pousse_globale(b->chaine(), nullptr, b->index_type, (b->drapeaux & DYNAMIC) != 0);
				return;
			}

			/* À FAIRE : déplace ça */
			/* Mets à zéro les valeurs des tableaux dynamics. */
			//			if (type.type_base() == id_morceau::TABLEAU) {
			//				os << b->chaine() << ".pointeur = 0;";
			//				os << b->chaine() << ".taille = 0;";
			//			}

			contexte.pousse_locale(b->morceau.chaine, nullptr, b->index_type, (b->drapeaux & DYNAMIC) != 0, false);

			break;
		}
		case type_noeud::ASSIGNATION_VARIABLE:
		{
			assert(b->enfants.size() == 2);

			auto variable = b->enfants.front();
			auto expression = b->enfants.back();

			auto compatibilite = std::any_cast<niveau_compat>(b->valeur_calculee);
			auto expression_modifiee = false;
			auto nouvelle_expr = std::string();

			/* À FAIRE */
			if ((compatibilite & niveau_compat::converti_tableau) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_TABLEAU;
			}

			if ((compatibilite & niveau_compat::converti_eini) != niveau_compat::aucune) {
				expression->drapeaux |= CONVERTI_EINI;
				expression_modifiee = true;

				nouvelle_expr = cree_eini(contexte, os, expression);
			}

			if ((compatibilite & niveau_compat::extrait_eini) != niveau_compat::aucune) {
				expression->drapeaux |= EXTRAIT_EINI;
				expression->index_type = variable->index_type;
				expression_modifiee = true;

				auto nom_eini = std::string("__eini_ext_")
						.append(std::to_string(expression->morceau.ligne_pos >> 32));

				auto &dt = contexte.magasin_types.donnees_types[expression->index_type];

				contexte.magasin_types.converti_type_C(contexte, "", dt, os);

				os << " " << nom_eini << " = *(";

				contexte.magasin_types.converti_type_C(contexte, "", dt, os);

				os << " *)(";
				genere_code_C(expression, contexte, false, os);
				os << ".pointeur);\n";

				nouvelle_expr = nom_eini;
			}

			if (expression->type == type_noeud::LOGE) {
				expression_modifiee = true;
				genere_code_C(expression, contexte, false, os);
				nouvelle_expr = std::any_cast<std::string>(expression->valeur_calculee);
			}

			if (expression->type == type_noeud::APPEL_FONCTION) {
				expression_modifiee = true;
				expression->drapeaux |= INDIRECTION_APPEL;
				genere_code_C(expression, contexte, false, os);
				nouvelle_expr = std::any_cast<std::string>(expression->valeur_calculee);
			}

			genere_code_C(variable, contexte, true, os);
			os << " = ";

			if (!expression_modifiee) {
				genere_code_C(expression, contexte, false, os);
			}
			else {
				os << nouvelle_expr;
			}

			/* pour les globales */
			os << ";\n";

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

			auto const index_type1 = enfant1->index_type;
			auto const index_type2 = enfant2->index_type;

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

					genere_code_C(enfant2, contexte, valeur2_brut, os);
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
					os << "!(";
					break;
				}
				case id_morceau::TILDE:
				{
					os << "~(";
					break;
				}
				case id_morceau::AROBASE:
				{
					os << "&(";
					break;
				}
				case id_morceau::PLUS_UNAIRE:
				{
					os << "(";
					break;
				}
				case id_morceau::MOINS_UNAIRE:
				{
					os << "-(";
					break;
				}
				default:
				{
					break;
				}
			}

			genere_code_C(enfant, contexte, expr_gauche, os);
			os << ")";

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

			os << "{.pointeur=";
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
			os << ", .taille=" << chaine.size() << "}";
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
			auto c = b->morceau.chaine[0];

			os << '\'';
			if (c == '\\') {
				os << c << b->morceau.chaine[1];
			}
			else {
				os << c;
			}

			os << '\'';
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
			auto nombre_enfants = b->enfants.size();
			auto iter = b->enfants.begin();

			/* on génère d'abord le type de la variable */
			auto enfant1 = *iter++;
			auto enfant2 = *iter++;
			auto enfant3 = *iter++;

			/* À FAIRE */
			auto enfant4 = (nombre_enfants >= 4) ? *iter++ : nullptr;
			auto enfant5 = (nombre_enfants == 5) ? *iter++ : nullptr;

			auto enfant_sans_arret = enfant4;
			auto enfant_sinon = (nombre_enfants == 5) ? enfant5 : enfant4;

			auto index_type = enfant2->index_type;
			auto const &type_debut = contexte.magasin_types.donnees_types[index_type];
			auto const type = type_debut.type_base();

			enfant1->index_type = index_type;

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

				/* À FAIRE: nom unique pour les boucles dans les boucles */
				if ((type & 0xff) == id_morceau::TABLEAU) {
					const auto taille_tableau = static_cast<uint64_t>(type >> 8);

					if (taille_tableau != 0) {
						os << "\nfor (int __i = 0; __i <= " << taille_tableau << "-1; ++__i) {\n";
						contexte.magasin_types.converti_type_C(contexte, "", type_debut.derefence(), os);
						os << " *" << enfant1->chaine() << " = &" << enfant2->chaine() << "[__i];\n";
					}
					else {
						auto nom_var = "__i" + std::to_string(b->morceau.ligne_pos);

						os << "\nfor (int "<< nom_var <<" = 0; "<< nom_var <<" <= ";
						genere_code_C(enfant2, contexte, false, os);
						os << ".taille - 1; ++"<< nom_var <<") {\n";
						contexte.magasin_types.converti_type_C(contexte, "", type_debut.derefence(), os);
						os << " *" << enfant1->chaine() << " = &";
						genere_code_C(enfant2, contexte, false, os);
						os << ".pointeur["<< nom_var <<"];\n";
					}
				}
				else if (type == id_morceau::CHAINE) {
					auto dt = DonneesType{};
					dt.pousse(id_morceau::Z8);

					index_type = contexte.magasin_types.ajoute_type(dt);
					enfant1->index_type = index_type;

					os << "\nfor (int __i = 0; __i <= " << enfant2->chaine() << ".taille - 1; ++__i) {\n";
					contexte.magasin_types.converti_type_C(contexte, "", dt, os);
					os << " *" << enfant1->chaine() << " = &" << enfant2->chaine() << ".pointeur[__i];\n";
				}

				contexte.pousse_locale(enfant1->chaine(), index_type, BESOIN_DEREF);
			}

			auto goto_continue = "__continue_boucle_pour" + std::to_string(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + std::to_string(b->morceau.ligne_pos);
			auto goto_brise = "__boucle_pour_brise" + std::to_string(b->morceau.ligne_pos);

			contexte.empile_goto_continue(enfant1->chaine(), goto_continue);
			contexte.empile_goto_arrete(enfant1->chaine(), (enfant_sinon != nullptr) ? goto_brise : goto_apres);

			genere_code_C(enfant3, contexte, false, os);

			os << goto_continue << ":;\n";
			os << "}\n";

			if (enfant_sans_arret) {
				genere_code_C(enfant_sans_arret, contexte, false, os);
				os << "goto " << goto_apres << ";";
			}

			if (enfant_sinon) {
				os << goto_brise << ":;\n";
				genere_code_C(enfant_sinon, contexte, false, os);
			}

			os << goto_apres << ":;\n";

			contexte.depile_goto_arrete();
			contexte.depile_goto_continue();

			contexte.depile_nombre_locales();

			break;
		}
		case type_noeud::CONTINUE_ARRETE:
		{
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

			os << "goto " << label_goto;
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

			auto goto_continue = "__continue_boucle_pour" + std::to_string(b->morceau.ligne_pos);
			auto goto_apres = "__boucle_pour_post" + std::to_string(b->morceau.ligne_pos);

			contexte.empile_goto_continue("", goto_continue);
			contexte.empile_goto_arrete("", goto_apres);

			os << "while (1) {\n";

			/* on crée une branche explicite dans le bloc */
			genere_code_C(enfant1, contexte, false, os);

			os << goto_continue << ":;\n";
			os << "}\n";
			os << goto_apres << ":;\n";

			//if (false) {
			//	genere_code_C(enfant2, contexte, false, os);
			//}

			contexte.depile_goto_continue();
			contexte.depile_goto_arrete();

			break;
		}
		case type_noeud::TRANSTYPE:
		{
			auto enfant = b->enfants.front();
			auto const &index_type_de = enfant->index_type;

			if (index_type_de == b->index_type) {
				genere_code_C(enfant, contexte, false, os);
				return;
			}

			auto const &donnees_type_de = contexte.magasin_types.donnees_types[index_type_de];

			auto const &dt = contexte.magasin_types.donnees_types[b->index_type];

			auto type_de = donnees_type_de.type_base();
			auto type_vers = dt.type_base();

			/* À FAIRE : vérifie compatibilité */
			if (type_de == id_morceau::POINTEUR && type_vers == id_morceau::POINTEUR) {
				os << "(";
				contexte.magasin_types.converti_type_C(contexte, "", dt, os);
				os << ")(";
				genere_code_C(enfant, contexte, false, os);
				os << ")";
				return;
			}

			os << "(";
			contexte.magasin_types.converti_type_C(contexte, "", dt, os);
			os << ")(";
			genere_code_C(enfant, contexte, false, os);
			os << ")";
			return;

			/* À FAIRE : BitCast (Type Cast) */
			erreur::lance_erreur_type_operation(
						donnees_type_de,
						dt,
						contexte,
						b->donnees_morceau());
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
			/* utilisé principalement pour convertir les listes d'arguments
			 * variadics en un tableau */

			auto taille_tableau = b->enfants.size();

			if (b->calcule) {
				assert(static_cast<long>(taille_tableau) == std::any_cast<long>(b->valeur_calculee));
			}

			auto &type = contexte.magasin_types.donnees_types[b->index_type];

			/* cherche si une conversion est requise */
			for (auto enfant : b->enfants) {
				if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
					enfant->valeur_calculee = cree_eini(contexte, os, enfant);
				}
			}

			/* alloue un tableau fixe */
			auto dt_tfixe = DonneesType{};
			dt_tfixe.pousse(id_morceau::TABLEAU | static_cast<int>(taille_tableau << 8));
			dt_tfixe.pousse(type);

			auto nom_tableau_fixe = std::string("__tabl_fix")
					.append(std::to_string(b->morceau.ligne_pos >> 32));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tfixe, os);

			os << " = ";

			auto virgule = '{';

			for (auto enfant : b->enfants) {
				os << virgule;
				if ((enfant->drapeaux & CONVERTI_EINI) != 0) {
					os << std::any_cast<std::string>(enfant->valeur_calculee);
				}
				else {
					genere_code_C(enfant, contexte, false, os);
				}

				virgule = ',';
			}

			os << "};\n";

			/* alloue un tableau dynamique */
			auto dt_tdyn = DonneesType{};
			dt_tdyn.pousse(id_morceau::TABLEAU);
			dt_tdyn.pousse(type);

			auto nom_tableau_dyn = std::string("__tabl_dyn")
					.append(std::to_string(b->morceau.ligne_pos >> 32));

			contexte.magasin_types.converti_type_C(
						contexte, nom_tableau_fixe, dt_tdyn, os);

			os << ' ' << nom_tableau_dyn << ";\n";
			os << nom_tableau_dyn << ".pointeur = " << nom_tableau_fixe << ";\n";
			os << nom_tableau_dyn << ".taille = " << taille_tableau << ";\n";

			b->valeur_calculee = nom_tableau_dyn;
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
		case type_noeud::TYPE_DE:
		{
			auto enfant = b->enfants.front();
			os << enfant->index_type;
			break;
		}
		case type_noeud::MEMOIRE:
		{
			os << "*(";
			genere_code_C(b->enfants.front(), contexte, false, os);
			os << ")";
			break;
		}
		case type_noeud::LOGE:
		{
			/* À FAIRE */
			auto &dt = contexte.magasin_types.donnees_types[b->index_type];

			if (dt.type_base() == id_morceau::TABLEAU) {
				auto nom_ptr = "__ptr" + std::to_string(b->morceau.ligne_pos);
				auto nom_tabl = "__tabl" + std::to_string(b->morceau.ligne_pos);
				auto taille_tabl = std::any_cast<int>(b->valeur_calculee);

				auto dt_ptr = DonneesType{};
				dt_ptr.pousse(id_morceau::POINTEUR);
				dt_ptr.pousse(dt.derefence());

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_ptr,
							os);

				os << nom_ptr << " = (";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt_ptr,
							os);

				os << ")(malloc(sizeof(";
				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt.derefence(),
							os);
				os << ") * " << taille_tabl << "));\n";

				contexte.magasin_types.converti_type_C(
							contexte,
							"",
							dt,
							os);

				os << " " << nom_tabl << ";\n";
				os << nom_tabl << ".pointeur = " << nom_ptr << ";\n";
				os << nom_tabl << ".taille = " << taille_tabl << ";\n";

				b->valeur_calculee = nom_tabl;
			}
			else if (dt.type_base() == id_morceau::CHAINE) {
				auto nom_ptr = "__ptr" + std::to_string(b->morceau.ligne_pos);
				auto nom_chaine = "__chaine" + std::to_string(b->morceau.ligne_pos);
				auto nom_taille = "__taille" + std::to_string(b->morceau.ligne_pos);
				auto enfant = b->enfants.front();

				os << "long " << nom_taille << " = ";

				genere_code_C(enfant, contexte, false, os);

				os << ";\n";

				os << "char *" << nom_ptr << " = (char *)(malloc(sizeof(char) * (";
				os << nom_taille << ")));\n";

				os << "chaine " << nom_chaine << ";\n";
				os << nom_chaine << ".pointeur = " << nom_ptr << ";\n";
				os << nom_chaine << ".taille = " << nom_taille << ";\n";

				b->valeur_calculee = nom_chaine;
			}

			break;
		}
		case type_noeud::DELOGE:
		{
			/* À FAIRE */
			auto enfant = b->enfants.front();

			os << "free(" << enfant->chaine() << ".pointeur);\n";
			os << enfant->chaine() << ".pointeur = 0;\n";
			os << enfant->chaine() << ".taille = 0;\n";

			break;
		}
		case type_noeud::RELOGE:
		{
			/* À FAIRE */
			// reloge [taille]z32 tabl;

			// reloge chaine(taille) chn;
			auto enfant = b->enfants.front();

			os << "ptr = (type *)(realloc(ptr, nouvelle_taille);\n";
			os << enfant->chaine() << ".pointeur = (char *)(realloc("
			   << enfant->chaine() << ".pointeur, nouvelle_taille);\n";

			os << enfant->chaine() << ".taille = 0;\n";
			break;
		}
	}
}

}  /* namespace noeud */
