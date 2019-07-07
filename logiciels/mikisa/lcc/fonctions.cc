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

#include "fonctions.hh"

#include "code_inst.hh"
#include "contexte_execution.hh"

namespace lcc {

/* ************************************************************************** */

static bool peut_convertir(type_var de, type_var vers)
{
	if (vers == type_var::POLYMORPHIQUE) {
		return true;
	}

	return code_inst_conversion(de, vers) != code_inst::TERMINE;
}

static double poids_compatibilite(types_entrees const &types_fonctions, types_entrees const &types_params)
{
	if (types_fonctions.types.taille() != types_params.types.taille()) {
		return 0.0;
	}

	auto taille = types_params.types.taille();

	/* Les fonctions n'ayant aucuns paramètres sont pleinement compatibles. */
	if (taille == 0) {
		return 1.0;
	}

	auto poids = 0.0;

	for (auto i = 0; i < taille; ++i) {
		auto type_param = types_params.types[i];
		auto type_fonction = types_fonctions.types[i];

		if (type_param == type_fonction) {
			poids += 1.0;
			continue;
		}

		if (peut_convertir(type_param, type_fonction)) {
			poids += 0.5;
			continue;
		}
	}

	return poids;
}

struct triade_types {
	types_entrees entrees;
	types_sorties sorties;
	type_var type;
};

static triade_types types_fonctions_finaux(
		types_entrees const &entrees,
		types_sorties const &sorties,
		types_entrees const &types_params)
{
	if (entrees.types.taille() != types_params.types.taille()) {
		return { entrees, sorties, type_var::INVALIDE };
	}

	auto taille = types_params.types.taille();

	auto type_specialise = type_var::INVALIDE;

	/* cherche le plus gros type pour la spécialisation */
	for (auto i = 0; i < taille; ++i) {
		if (entrees.types[i] == type_var::POLYMORPHIQUE) {
			if (type_specialise == type_var::INVALIDE) {
				type_specialise = types_params.types[i];
			}
			else {
				if (taille_type(types_params.types[i]) > taille_type(type_specialise)) {
					type_specialise = types_params.types[i];
				}
			}
		}
	}

	/* crée la spécialisation au besoin */
	if (type_specialise == type_var::INVALIDE) {
		return { entrees, sorties, type_var::INVALIDE };
	}

	auto types_finaux_entrees = types_entrees{};
	types_finaux_entrees.types.reserve(taille);

	for (auto i = 0; i < taille; ++i) {
		if (entrees.types[i] == type_var::POLYMORPHIQUE) {
			types_finaux_entrees.types.pousse(type_specialise);
		}
		else {
			types_finaux_entrees.types.pousse(entrees.types[i]);
		}
	}

	taille = sorties.types.taille();
	auto types_finaux_sorties = types_sorties{};
	types_finaux_sorties.types.reserve(taille);

	for (auto i = 0; i < taille; ++i) {
		if (sorties.types[i] == type_var::POLYMORPHIQUE) {
			types_finaux_sorties.types.pousse(type_specialise);
		}
		else {
			types_finaux_sorties.types.pousse(sorties.types[i]);
		}
	}

	return { types_finaux_entrees, types_finaux_sorties, type_specialise };
}

/* ************************************************************************** */

signature::signature(types_entrees _entrees_, types_sorties _sorties_)
	: entrees(std::move(_entrees_))
	, sorties(std::move(_sorties_))
{}

/* ************************************************************************** */

void magasin_fonctions::ajoute_fonction(const dls::chaine &nom, code_inst type, const signature &seing, ctx_script ctx)
{
	auto iter = table.trouve(nom);

	if (iter == table.fin()) {
		auto tableau = dls::tableau<donnees_fonction>{};
		tableau.pousse({seing, type, ctx});

		table.insere({nom, tableau});
	}
	else {
		iter->second.pousse({seing, type, ctx});
	}
}

donnees_fonction_generation magasin_fonctions::meilleure_candidate(
		dls::chaine const &nom,
		types_entrees const &types_params)
{
	auto res = donnees_fonction_generation{};
	res.donnees = nullptr;
	res.type = type_var::INVALIDE;
	res.entrees = {};
	res.sorties = {};

	auto iter = table.trouve(nom);

	if (iter == table.fin()) {
		return res;
	}

	auto &tableau = iter->second;

	auto poids = 0.0;

	for (auto &donnee : tableau) {
		/* si la fonction est a des paramètres polymorphiques, il faut spécialiser */
		auto [types_entrees_, types_sorties_, type] = types_fonctions_finaux(
				donnee.seing.entrees,
				donnee.seing.sorties,
				types_params);

		auto poids_fonction = poids_compatibilite(types_entrees_, types_params);

		if (poids_fonction > poids) {
			poids = poids_fonction;
			res.donnees = &donnee;
			res.entrees = types_entrees_;
			res.sorties = types_sorties_;
			res.type = type;
		}
	}

	return res;
}

/* ************************************************************************** */

static void enregistre_fonctions_mathematiques(magasin_fonctions &magasin)
{
	magasin.ajoute_fonction(
				"traduit",
				code_inst::FN_TRADUIT,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"bruit_turbulent",
				code_inst::FN_BRUIT_TURBULENT,
				signature(
					types_entrees(type_var::ENT32, type_var::DEC, type_var::VEC3),
					types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"complément",
				code_inst::FN_COMPLEMENT,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"aléa",
				code_inst::FN_ALEA,
				signature(
					types_entrees(type_var::DEC, type_var::DEC),
					types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"restreint",
				code_inst::FN_RESTREINT,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"enligne",
				code_inst::FN_ENLIGNE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite1",
				code_inst::FN_HERMITE1,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite2",
				code_inst::FN_HERMITE2,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite3",
				code_inst::FN_HERMITE3,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite4",
				code_inst::FN_HERMITE4,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite5",
				code_inst::FN_HERMITE5,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"hermite6",
				code_inst::FN_HERMITE6,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	/* fonctions mathématiques */

	magasin.ajoute_fonction(
				"cos",
				code_inst::FN_COSINUS,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"sin",
				code_inst::FN_SINUS,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"tan",
				code_inst::FN_TANGEANTE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"acos",
				code_inst::FN_ARCCOSINUS,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"asin",
				code_inst::FN_ARCSINUS,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"atan",
				code_inst::FN_ARCTANGEANTE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"abs",
				code_inst::FN_ABSOLU,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"racine_carrée",
				code_inst::FN_RACINE_CARREE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"exp",
				code_inst::FN_EXPONENTIEL,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"log",
				code_inst::FN_LOGARITHME,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"frac",
				code_inst::FN_FRACTION,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"plafond",
				code_inst::FN_PLAFOND,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"sol",
				code_inst::FN_SOL,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"arrondis",
				code_inst::FN_ARRONDIS,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"nie",
				code_inst::NIE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"inverse",
				code_inst::FN_INVERSE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"atan2",
				code_inst::FN_ARCTAN2,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"max",
				code_inst::FN_MAX,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"min",
				code_inst::FN_MIN,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"plus_grand_que",
				code_inst::FN_PLUS_GRAND_QUE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"plus_petit_que",
				code_inst::FN_PLUS_PETIT_QUE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"puissance",
				code_inst::FN_PUISSANCE,
				signature(
					types_entrees(type_var::POLYMORPHIQUE, type_var::POLYMORPHIQUE),
					types_sorties(type_var::POLYMORPHIQUE)),
				ctx_script::tous);
}

static void enregistre_fonctions_vectorielles(magasin_fonctions &magasin)
{
	magasin.ajoute_fonction(
				"sépare_vec3",
				code_inst::FN_SEPARE_VEC3,
				signature(
					types_entrees(type_var::VEC3),
					types_sorties(type_var::DEC, type_var::DEC, type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"combine_vec3",
				code_inst::FN_COMBINE_VEC3,
				signature(
					types_entrees(type_var::DEC, type_var::DEC, type_var::DEC),
					types_sorties(type_var::VEC3)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"base_orthonormale",
				code_inst::FN_BASE_ORTHONORMALE,
				signature(
					types_entrees(type_var::VEC3),
					types_sorties(type_var::VEC3, type_var::VEC3)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"produit_scalaire",
				code_inst::FN_PRODUIT_SCALAIRE_VEC3,
				signature(
					types_entrees(type_var::VEC3, type_var::VEC3),
					types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"produit_croix",
				code_inst::FN_PRODUIT_CROIX_VEC3,
				signature(
					types_entrees(type_var::VEC3, type_var::VEC3),
					types_sorties(type_var::VEC3)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"longueur",
				code_inst::FN_LONGUEUR_VEC3,
				signature(
					types_entrees(type_var::VEC3),
					types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"normalise",
				code_inst::FN_NORMALISE_VEC3,
				signature(
					types_entrees(type_var::VEC3),
					types_sorties(type_var::VEC3, type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"fresnel",
				code_inst::FN_FRESNEL,
				signature(
					types_entrees(type_var::VEC3, type_var::VEC3, type_var::DEC),
					types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"réfracte",
				code_inst::FN_REFRACTE,
				signature(
					types_entrees(type_var::VEC3, type_var::VEC3, type_var::DEC),
					types_sorties(type_var::VEC3)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"réfléchi",
				code_inst::FN_REFLECHI,
				signature(
					types_entrees(type_var::VEC3, type_var::VEC3),
					types_sorties(type_var::VEC3)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"échantillone_sphère",
				code_inst::FN_ECHANTILLONE_SPHERE,
				signature(
					types_entrees(),
					types_sorties(type_var::VEC3)),
				ctx_script::tous);
}

static void enregistre_fonctions_corps(magasin_fonctions &magasin)
{
	magasin.ajoute_fonction(
				"ajoute_point",
				code_inst::FN_AJOUTE_POINT,
				signature(types_entrees(type_var::VEC3),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);

	magasin.ajoute_fonction(
				"ajoute_primitive",
				code_inst::FN_AJOUTE_PRIMITIVE,
				signature(types_entrees(type_var::ENT32),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);

	magasin.ajoute_fonction(
				"ajoute_primitive",
				code_inst::FN_AJOUTE_PRIMITIVE_SOMMETS,
				signature(types_entrees(type_var::ENT32, type_var::TABLEAU),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);

	magasin.ajoute_fonction(
				"ajoute_sommet",
				code_inst::FN_AJOUTE_SOMMET,
				signature(types_entrees(type_var::ENT32, type_var::ENT32),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);

	magasin.ajoute_fonction(
				"ajoute_sommets",
				code_inst::FN_AJOUTE_SOMMETS,
				signature(types_entrees(type_var::ENT32, type_var::TABLEAU),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);

	/* ajoute une ligne selon une position et une direction, retourne l'index
	 * de la primitive ajoutée */
	magasin.ajoute_fonction(
				"ajoute_ligne",
				code_inst::FN_AJOUTE_LIGNE,
				signature(types_entrees(type_var::VEC3, type_var::VEC3),
						  types_sorties(type_var::ENT32)),
				ctx_script::tous & ~ctx_script::detail);
}

static void enregistre_fonctions_colorimetriques(magasin_fonctions &magasin)
{
	magasin.ajoute_fonction(
				"sature",
				code_inst::FN_SATURE,
				signature(types_entrees(type_var::COULEUR, type_var::DEC),
						  types_sorties(type_var::COULEUR)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"luminance",
				code_inst::FN_LUMINANCE,
				signature(types_entrees(type_var::COULEUR),
						  types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"contraste",
				code_inst::FN_CONTRASTE,
				signature(types_entrees(type_var::COULEUR, type_var::COULEUR),
						  types_sorties(type_var::DEC)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"corps_noir",
				code_inst::FN_CORPS_NOIR,
				signature(types_entrees(type_var::DEC),
						  types_sorties(type_var::COULEUR)),
				ctx_script::tous);

	magasin.ajoute_fonction(
				"longueur_onde",
				code_inst::FN_LONGUEUR_ONDE,
				signature(types_entrees(type_var::DEC),
						  types_sorties(type_var::COULEUR)),
				ctx_script::tous);
}

static void enregistre_fonctions_types(magasin_fonctions &magasin)
{
	magasin.ajoute_fonction(
				"taille",
				code_inst::FN_TAILLE_TABLEAU,
				signature(
					types_entrees(type_var::TABLEAU),
					types_sorties(type_var::ENT32)),
				ctx_script::tous);
}

void enregistre_fonctions_base(magasin_fonctions &magasin)
{
	enregistre_fonctions_mathematiques(magasin);
	enregistre_fonctions_vectorielles(magasin);
	enregistre_fonctions_corps(magasin);
	enregistre_fonctions_colorimetriques(magasin);
	enregistre_fonctions_types(magasin);
}

}  /* namespace lcc */
