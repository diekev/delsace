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

#include "arbre_syntactic.h"

#include "biblinternes/structures/flux_chaine.hh"

#include "contexte_generation_code.h"
#include "code_inst.hh"

namespace lcc {

int ajoute_conversion(
		compileuse_lng &compileuse,
		type_var type1,
		type_var type2,
		int decalage_pile)
{
	compileuse.ajoute_instructions(code_inst_conversion(type1, type2));
	compileuse.ajoute_instructions(decalage_pile);
	auto ptr = compileuse.donnees().loge_donnees(taille_type(type2));
	compileuse.ajoute_instructions(ptr);
	return ptr;
}

/* ************************************************************************** */

namespace noeud {

static auto chaine_type_noeud(type_noeud type)
{
	switch (type) {
		case type_noeud::RACINE:
			return "racine";
		case type_noeud::PROPRIETE:
			return "propriété";
		case type_noeud::VARIABLE:
			return "variable";
		case type_noeud::VALEUR:
			return "valeur";
		case type_noeud::FONCTION:
			return "fonction";
		case type_noeud::ATTRIBUT:
			return "attribut";
		case type_noeud::OPERATION_UNAIRE:
			return "opération unaire";
		case type_noeud::OPERATION_BINAIRE:
			return "opération binaire";
		case type_noeud::ASSIGNATION:
			return "assignation";
		case type_noeud::ACCES_MEMBRE_POINT:
			return "accès membre point";
		case type_noeud::SI:
			return "si";
		case type_noeud::BLOC:
			return "bloc";
		case type_noeud::POUR:
			return "pour";
		case type_noeud::PLAGE:
			return "plage";
		case type_noeud::RETOURNE:
			return "retourne";
		case type_noeud::ARRETE:
			return "arrête";
		case type_noeud::CONTINUE:
			return "continue";
	}

	return "invalide";
}

/* ************************************************************************** */

static void rassemble_feuilles(base *noeud_base, dls::tableau<base *> &feuilles)
{
	for (auto enfant : noeud_base->enfants) {
		if (enfant->identifiant() == id_morceau::VIRGULE) {
			rassemble_feuilles(enfant, feuilles);
		}
		else {
			feuilles.pousse(enfant);
		}
	}
}

/* ************************************************************************** */

base::base(ContexteGenerationCode &contexte, DonneesMorceaux const &donnees_, type_noeud type_)
	: donnees(donnees_)
	, type(type_)
	, donnees_type(type_var::INVALIDE)
{
	INUTILISE(contexte);
}

void base::ajoute_noeud(base *noeud)
{
	enfants.pousse(noeud);
}

const DonneesMorceaux &base::donnees_morceau() const
{
	return donnees;
}

id_morceau base::identifiant() const
{
	return donnees.identifiant;
}

dls::vue_chaine base::chaine() const
{
	return donnees.chaine;
}

void base::imprime_code(std::ostream &os, int profondeur)
{
	for (auto i = 0; i < profondeur; ++i) {
		os << ' ' << ' ';
	}

	os << chaine_type_noeud(this->type);

	if (this->type != type_noeud::RACINE) {
		os << " : " << donnees.chaine
		   << " (" << chaine_type_var(this->donnees_type) << ')';
	}

	os << '\n';

	for (auto const &enfant : this->enfants) {
		enfant->imprime_code(os, profondeur + 1);
	}
}

/* ************************************************************************** */

static auto extrait_decimal(dls::vue_chaine const &vue_chaine)
{
	dls::flux_chaine ss;
	ss << vue_chaine;

	float r;
	ss >> r;
	return r;
}

static auto extrait_entier(dls::vue_chaine const &vue_chaine)
{
	dls::flux_chaine ss;
	ss << vue_chaine;

	int r;
	ss >> r;
	return r;
}

static auto type_var_depuis_id(id_morceau id)
{
	switch (id) {
		default:
			return type_var::INVALIDE;
		case id_morceau::CHAINE_LITTERALE:
			return type_var::CHAINE;
		case id_morceau::TABLEAU:
			return type_var::TABLEAU;
		case id_morceau::NOMBRE_ENTIER:
		case id_morceau::NOMBRE_OCTAL:
		case id_morceau::NOMBRE_HEXADECIMAL:
		case id_morceau::NOMBRE_BINAIRE:
			return type_var::ENT32;
		case id_morceau::NOMBRE_REEL:
			return type_var::DEC;
		case id_morceau::VEC2:
			return type_var::VEC2;
		case id_morceau::VEC3:
			return type_var::VEC3;
		case id_morceau::VEC4:
			return type_var::VEC4;
		case id_morceau::MAT3:
			return type_var::MAT3;
		case id_morceau::MAT4:
			return type_var::MAT4;
	}
}

enum conv_op {
	converti_type1,
	converti_type2,
	aucune,
};

static auto calcul_conversion(
		type_var type1,
		type_var type2)
{
	if (type1 == type2) {
		return conv_op::aucune;
	}

	if (type1 == type_var::DEC && type2 == type_var::ENT32) {
		return conv_op::converti_type2;
	}

	if (type1 == type_var::ENT32 && type2 == type_var::DEC) {
		return conv_op::converti_type1;
	}

	if (taille_type(type1) > taille_type(type2)) {
		return conv_op::converti_type2;
	}

	return conv_op::converti_type1;
}

/* ************************************************************************** */

int genere_code(
		base *b,
		ContexteGenerationCode &contexte_generation,
		compileuse_lng &compileuse,
		bool expr_gauche)
{
	switch (b->type) {
		case type_noeud::RACINE:
		{
			for (auto enfant : b->enfants) {
				genere_code(enfant, contexte_generation, compileuse, expr_gauche);
			}

			compileuse.ajoute_instructions(code_inst::TERMINE);
			break;
		}
		case type_noeud::PROPRIETE:
		{
			auto &gest_props = contexte_generation.gest_props;
			b->donnees_type = gest_props.type_propriete(b->chaine());
			b->pointeur_donnees = gest_props.pointeur_donnees(b->chaine());

			break;
		}
		case type_noeud::ATTRIBUT:
		{
			auto &gest_attrs = contexte_generation.gest_attrs;

			if (!gest_attrs.propriete_existe(b->chaine())) {
				if (expr_gauche) {
					gest_attrs.requiers_attr(dls::chaine(b->chaine()), b->donnees_type, b->pointeur_donnees);
				}
				else {
					// À FAIRE : erreur
				}
			}
			else {
				b->donnees_type = gest_attrs.type_propriete(b->chaine());
				b->pointeur_donnees = gest_attrs.pointeur_donnees(b->chaine());
			}

			break;
		}
		case type_noeud::VARIABLE:
		{
			if (expr_gauche) {
				/* si la locale existe, ne fais rien */
				if (contexte_generation.locale_existe(b->chaine())) {
					b->pointeur_donnees = contexte_generation.valeur_locale(b->chaine());
					b->donnees_type  = contexte_generation.donnees_type(b->chaine());
				}
				else {
					/* sinon, loge de la place pour elle */
					b->pointeur_donnees = compileuse.donnees().loge_donnees(taille_type(b->donnees_type));
					contexte_generation.pousse_locale(b->chaine(), b->pointeur_donnees, b->donnees_type);
				}
			}
			else {
				/* retrouve le pointeur */
				b->pointeur_donnees = contexte_generation.valeur_locale(b->chaine());
				b->donnees_type  = contexte_generation.donnees_type(b->chaine());
			}

			break;
		}
		case type_noeud::VALEUR:
		{
			b->donnees_type = type_var_depuis_id(b->identifiant());

			switch (b->donnees_type) {
				case type_var::ENT32:
				{
					b->pointeur_donnees = compileuse.donnees().loge_donnees(1);
					/* évite de modifier le pointeur */
					auto decalage = b->pointeur_donnees;
					compileuse.donnees().stocke(decalage, extrait_entier(b->chaine()));
					break;
				}
				case type_var::DEC:
				{
					b->pointeur_donnees = compileuse.donnees().loge_donnees(1);
					/* évite de modifier le pointeur */
					auto decalage = b->pointeur_donnees;
					compileuse.donnees().stocke(decalage, extrait_decimal(b->chaine()));
					break;
				}
				case type_var::VEC2:
				case type_var::VEC3:
				case type_var::VEC4:
				case type_var::MAT3:
				case type_var::MAT4:
				{
					auto taille = taille_type(b->donnees_type);

					if (b->enfants.est_vide()) {
						b->pointeur_donnees = compileuse.donnees().loge_donnees(taille);
					}
					else {
						auto feuilles = dls::tableau<base *>();
						rassemble_feuilles(b, feuilles);

						/* À FAIRE :
						 * - vérifie que tous les noeuds sont bel et bien constants
						 * - converti les valeurs au besoin
						 */

						b->pointeur_donnees = compileuse.donnees().loge_donnees(taille);

						auto decalage = b->pointeur_donnees;

						for (auto const &feuille : feuilles) {
							compileuse.donnees().stocke(decalage, extrait_decimal(feuille->chaine()));
						}

						if ((decalage - b->pointeur_donnees) > taille) {
							// il y trop de paramètres
						}
					}

					break;
				}
				case type_var::TABLEAU:
				{
					auto feuilles = dls::tableau<base *>();
					rassemble_feuilles(b, feuilles);

					// À FAIRE : erreur si tableau vide, INFÉRENCE TYPE
					auto type_feuille = type_var{};

					for (auto feuille : feuilles) {
						genere_code(feuille, contexte_generation, compileuse, expr_gauche);
						type_feuille = feuille->donnees_type;
					}

					compileuse.ajoute_instructions(code_inst::CONSTRUIT_TABLEAU);
					compileuse.ajoute_instructions(type_feuille);
					compileuse.ajoute_instructions(feuilles.taille());

					for (auto feuille : feuilles) {
						compileuse.ajoute_instructions(feuille->pointeur_donnees);
					}

					// là où se trouve l'index du tableau
					b->pointeur_donnees = compileuse.donnees().loge_donnees(1);

					compileuse.ajoute_instructions(b->pointeur_donnees);

					break;
				}
				case type_var::CHAINE:
				{
					/* là où se trouve l'index de la chaine */
					b->pointeur_donnees = compileuse.donnees().loge_donnees(1);
					auto ptr = b->pointeur_donnees;
					compileuse.donnees().stocke(ptr, static_cast<int>(contexte_generation.chaines.taille()));
					contexte_generation.chaines.pousse(b->chaine());
					break;
				}
				case type_var::COULEUR:
				case type_var::INVALIDE:
				case type_var::POLYMORPHIQUE:
				{
					break;
				}
			}

			break;
		}
		case type_noeud::FONCTION:
		{
			/* génére le code des enfants pour avoir les données sur leurs types */
			auto types_params = types_entrees{};

			for (auto enfant : b->enfants) {
				genere_code(enfant, contexte_generation, compileuse, expr_gauche);
				types_params.types.pousse(enfant->donnees_type);
			}

			auto donnees_fonc = contexte_generation.fonctions.meilleure_candidate(
						dls::chaine(b->chaine()),
						types_params);

			if (donnees_fonc.donnees == nullptr) {
				dls::flux_chaine ss;
				ss << "Impossible de trouver la fonction '" << b->chaine() << "'";
				throw std::runtime_error(ss.chn().c_str());
			}

			auto type_instance = donnees_fonc.type;

			contexte_generation.requetes.insere(donnees_fonc.donnees->requete);

			/* rassemble les pointeurs et crée les conversions au besoin */
			dls::tableau<int> pointeurs;
			auto taille = donnees_fonc.entrees.types.taille();
			auto enfant = b->enfants.debut();

			for (auto i = 0; i < taille; ++i) {
				auto type_entree = donnees_fonc.entrees.types[i];

				if ((*enfant)->donnees_type == type_entree) {
					pointeurs.pousse((*enfant)->pointeur_donnees);
				}
				else {
					/* À FAIRE : erreur si conversion impossible. */
					auto ptr = ajoute_conversion(
								compileuse,
								(*enfant)->donnees_type,
								type_entree,
								(*enfant)->pointeur_donnees);

					pointeurs.pousse(ptr);
				}

				++enfant;
			}

			/* ****************** crée les données pour les appels ****************** */

			/* ajoute le code_inst de la fonction */
			compileuse.ajoute_instructions(donnees_fonc.donnees->type);

			/* ajoute le type de la fonction pour choisir la bonne surcharge */
			if (type_instance != type_var::INVALIDE) {
				compileuse.ajoute_instructions(type_instance);
			}

			/* ajoute le pointeur de chaque paramètre */
			for (auto ptr : pointeurs) {
				compileuse.ajoute_instructions(ptr);
			}

			/* pour chaque sortie, nous réservons de la place sur la pile de données */
			for (auto const &type_seing : donnees_fonc.sorties.types) {
				auto pointeur = compileuse.donnees().loge_donnees(taille_type(type_seing));

				/* le pointeur sur la pile est celui du premier paramètre  */
				if (b->pointeur_donnees == 0) {
					b->pointeur_donnees = pointeur;
				}
			}

			/* ajoute le pointeur aux instructions pour savoir où écrire */
			compileuse.ajoute_instructions(b->pointeur_donnees);

			/* par défaut, seule la première sortie est utilisée, sauf si on assigne
			 * plusieurs variables à la fois, c-à-d : a, b = foo(); */
			b->donnees_type = donnees_fonc.sorties.types[0];
			/* pour les données sur les sorties multiple */
			b->valeur_calculee = donnees_fonc;

			break;
		}
		case type_noeud::OPERATION_UNAIRE:
		{
			auto enfant = b->enfants.front();
			auto pointeur = genere_code(enfant, contexte_generation, compileuse, expr_gauche);

			b->donnees_type = enfant->donnees_type;

			switch (b->identifiant()) {
				case id_morceau::MOINS_UNAIRE:
				{
					auto taille = taille_type(b->donnees_type);
					compileuse.ajoute_instructions(code_inst::NIE);
					compileuse.ajoute_instructions(b->donnees_type);
					compileuse.ajoute_instructions(pointeur);
					b->pointeur_donnees = compileuse.donnees().loge_donnees(taille);
					compileuse.ajoute_instructions(b->pointeur_donnees);

					break;
				}
				case id_morceau::PLUS_UNAIRE:
				{
					b->pointeur_donnees = pointeur;
					break;
				}
				default:
				{
					/* À FAIRE : erreur */
					break;
				}
			}

			break;
		}
		case type_noeud::OPERATION_BINAIRE:
		{
			/* À FAIRE : type booléen pour le retour des opérateurs de comparaison. */

			auto enfant1 = b->enfants.front();
			auto enfant2 = b->enfants.back();

			genere_code(enfant1, contexte_generation, compileuse, expr_gauche);
			genere_code(enfant2, contexte_generation, compileuse, expr_gauche);

			auto decalage1 = enfant1->pointeur_donnees;
			auto decalage2 = enfant2->pointeur_donnees;

			/* À FAIRE : multiplication mat/vec, etc. */

			auto conversion = calcul_conversion(enfant1->donnees_type, enfant2->donnees_type);

			if (conversion == conv_op::aucune) {
				b->donnees_type = enfant1->donnees_type;
			}
			else if (conversion == conv_op::converti_type1) {
				b->donnees_type = enfant2->donnees_type;

				decalage1 = ajoute_conversion(
							compileuse,
							enfant1->donnees_type,
							enfant2->donnees_type,
							enfant1->pointeur_donnees);
			}
			else {
				b->donnees_type = enfant1->donnees_type;

				decalage2 = ajoute_conversion(
							compileuse,
							enfant2->donnees_type,
							enfant1->donnees_type,
							enfant2->pointeur_donnees);
			}

			auto ajoute_insts_op_assign = [&](code_inst inst)
			{
				compileuse.ajoute_instructions(inst);
				compileuse.ajoute_instructions(b->donnees_type);
				compileuse.ajoute_instructions(decalage1);
				compileuse.ajoute_instructions(decalage2);

				b->pointeur_donnees = compileuse.donnees().loge_donnees(taille_type(b->donnees_type));

				compileuse.ajoute_instructions(b->pointeur_donnees);

				/* ajoute assignement */
				compileuse.ajoute_instructions(code_inst::ASSIGNATION);
				compileuse.ajoute_instructions(b->donnees_type);
				compileuse.ajoute_instructions(b->pointeur_donnees);
				compileuse.ajoute_instructions(enfant1->pointeur_donnees);

				return b->pointeur_donnees;
			};

			switch (b->identifiant()) {
				case id_morceau::PLUS:
				{
					compileuse.ajoute_instructions(code_inst::FN_AJOUTE);
					break;
				}
				case id_morceau::MOINS:
				{
					compileuse.ajoute_instructions(code_inst::FN_SOUSTRAIT);
					break;
				}
				case id_morceau::FOIS:
				{
					/* cas spéciale en cas de matrices */
					if (b->donnees_type == type_var::MAT3 || b->donnees_type == type_var::MAT4) {
						compileuse.ajoute_instructions(code_inst::FN_MULTIPLIE_MAT);
					}
					else {
						compileuse.ajoute_instructions(code_inst::FN_MULTIPLIE);
					}

					break;
				}
				case id_morceau::DIVISE:
				{
					compileuse.ajoute_instructions(code_inst::FN_DIVISE);
					break;
				}
				case id_morceau::PLUS_EGAL:
				{
					return ajoute_insts_op_assign(code_inst::FN_AJOUTE);
				}
				case id_morceau::MOINS_EGAL:
				{
					return ajoute_insts_op_assign(code_inst::FN_SOUSTRAIT);
				}
				case id_morceau::DIVISE_EGAL:
				{
					return ajoute_insts_op_assign(code_inst::FN_DIVISE);
				}
				case id_morceau::FOIS_EGAL:
				{
					/* cas spéciale en cas de matrices */
					if (b->donnees_type == type_var::MAT3 || b->donnees_type == type_var::MAT4) {
						return ajoute_insts_op_assign(code_inst::FN_MULTIPLIE_MAT);
					}

					return ajoute_insts_op_assign(code_inst::FN_MULTIPLIE);
				}
				case id_morceau::POURCENT:
				{
					compileuse.ajoute_instructions(code_inst::FN_MODULO);
					break;
				}
				case id_morceau::EGALITE:
				{
					compileuse.ajoute_instructions(code_inst::FN_EGALITE);
					break;
				}
				case id_morceau::DIFFERENCE:
				{
					compileuse.ajoute_instructions(code_inst::FN_INEGALITE);
					break;
				}
				case id_morceau::SUPERIEUR:
				{
					compileuse.ajoute_instructions(code_inst::FN_SUPERIEUR);
					break;
				}
				case id_morceau::SUPERIEUR_EGAL:
				{
					compileuse.ajoute_instructions(code_inst::FN_SUPERIEUR_EGAL);
					break;
				}
				case id_morceau::INFERIEUR:
				{
					compileuse.ajoute_instructions(code_inst::FN_INFERIEUR);
					break;
				}
				case id_morceau::INFERIEUR_EGAL:
				{
					compileuse.ajoute_instructions(code_inst::FN_INFERIEUR_EGAL);
					break;
				}
				case id_morceau::BARRE:
				case id_morceau::BARRE_BARRE:
				{
					compileuse.ajoute_instructions(code_inst::FN_COMP_OU);
					break;
				}
				case id_morceau::ESPERLUETTE:
				case id_morceau::ESP_ESP:
				{
					compileuse.ajoute_instructions(code_inst::FN_COMP_ET);
					break;
				}
				case id_morceau::CHAPEAU:
				{
					compileuse.ajoute_instructions(code_inst::FN_COMP_OUX);
					break;
				}
				default:
				{
					/* À FAIRE : erreur */
					break;
				}
			}

			compileuse.ajoute_instructions(b->donnees_type);
			compileuse.ajoute_instructions(decalage1);
			compileuse.ajoute_instructions(decalage2);

			b->pointeur_donnees = compileuse.donnees().loge_donnees(taille_type(b->donnees_type));

			compileuse.ajoute_instructions(b->pointeur_donnees);

			break;
		}
		case type_noeud::ASSIGNATION:
		{
			auto enfant_gauche = b->enfants.front();
			auto enfant_droite = b->enfants.back();

			// l'idée est que les sorties des fonctions sont des pointeurs vers la où se trouve les valeurs
			auto pointeur = genere_code(enfant_droite, contexte_generation, compileuse, false);

			if (enfant_gauche->identifiant() == id_morceau::VIRGULE) {
				auto feuilles = dls::tableau<base *>();

				rassemble_feuilles(enfant_gauche, feuilles);

				auto donnees_fonc = donnees_fonction_generation{};

				if (enfant_droite->type == type_noeud::ACCES_MEMBRE_POINT) {
					auto noeud_fonc = enfant_droite->enfants.back();

					donnees_fonc = std::any_cast<donnees_fonction_generation>(
										noeud_fonc->valeur_calculee);
				}
				else if (enfant_droite->type == type_noeud::FONCTION) {
					donnees_fonc = std::any_cast<donnees_fonction_generation>(
										enfant_droite->valeur_calculee);
				}
				else {
					throw std::runtime_error("enfant_droite inattendu dans l'assignation");
				}

				for (auto i = 0; i < feuilles.taille(); ++i) {
					auto feuille = feuilles[i];

					auto ptr = pointeur;
					auto dt = donnees_fonc.sorties.types[i];

					if (i > 0) {
						auto type_sortie = donnees_fonc.sorties.types[i - 1];
						ptr += taille_type(type_sortie);
					}

					feuille->pointeur_donnees = ptr;
					feuille->donnees_type = dt;
					genere_code(feuille, contexte_generation, compileuse, true);

					compileuse.ajoute_instructions(code_inst::ASSIGNATION);
					compileuse.ajoute_instructions(dt);
					compileuse.ajoute_instructions(ptr);
					compileuse.ajoute_instructions(feuille->pointeur_donnees);
				}
			}
			else {
				enfant_gauche->pointeur_donnees = pointeur;
				enfant_gauche->donnees_type = enfant_droite->donnees_type;
				genere_code(enfant_gauche, contexte_generation, compileuse, true);

				compileuse.ajoute_instructions(code_inst::ASSIGNATION);
				compileuse.ajoute_instructions(enfant_droite->donnees_type);
				compileuse.ajoute_instructions(enfant_droite->pointeur_donnees);
				compileuse.ajoute_instructions(enfant_gauche->pointeur_donnees);
			}

			break;
		}
		case type_noeud::ACCES_MEMBRE_POINT:
		{
			auto enfant_gauche = b->enfants.front();
			auto enfant_droite = b->enfants.back();

			if (enfant_droite->type == type_noeud::FONCTION) {
				/* le premier paramètre doit être la 'structure' que l'on accède */
				enfant_droite->enfants.insere(enfant_droite->enfants.debut(), enfant_gauche);

				genere_code(enfant_droite, contexte_generation, compileuse, expr_gauche);

				b->donnees_type = enfant_droite->donnees_type;
				b->pointeur_donnees = enfant_droite->pointeur_donnees;
			}
			else if (enfant_droite->type == type_noeud::VARIABLE) {
				if (enfant_droite->identifiant() != id_morceau::CHAINE_CARACTERE) {
					throw std::runtime_error("accès à un membre qui n'est pas une chaîne");
				}

				genere_code(enfant_gauche, contexte_generation, compileuse, expr_gauche);

				switch (enfant_gauche->donnees_type) {
					case type_var::ENT32:
					case type_var::DEC:
					case type_var::MAT3:
					case type_var::MAT4:
					case type_var::TABLEAU:
					case type_var::CHAINE:
					case type_var::INVALIDE:
					case type_var::POLYMORPHIQUE:
					{
						dls::flux_chaine ss;
						ss << "La variable '"
						   << enfant_droite->chaine()
						   << "' n'est pas une structure !"
						   << " Accès au membre '"
						   << enfant_gauche->chaine()
						   << "' impossible.";

						throw std::runtime_error(ss.chn().c_str());
					}
					case type_var::VEC2:
					{
						b->donnees_type = type_var::DEC;

						if (enfant_droite->chaine() == "x") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees;
						}
						else if (enfant_droite->chaine() == "y") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 1;
						}
						else {
							throw std::runtime_error("membre inconnu");
						}

						break;
					}
					case type_var::VEC3:
					{
						b->donnees_type = type_var::DEC;

						if (enfant_droite->chaine() == "x") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees;
						}
						else if (enfant_droite->chaine() == "y") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 1;
						}
						else if (enfant_droite->chaine() == "z") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 2;
						}
						else {
							throw std::runtime_error("membre inconnu");
						}

						break;
					}
					case type_var::VEC4:
					{
						b->donnees_type = type_var::DEC;

						if (enfant_droite->chaine() == "x") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees;
						}
						else if (enfant_droite->chaine() == "y") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 1;
						}
						else if (enfant_droite->chaine() == "z") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 2;
						}
						else if (enfant_droite->chaine() == "w") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 3;
						}
						else {
							throw std::runtime_error("membre inconnu");
						}

						break;
					}
					case type_var::COULEUR:
					{
						b->donnees_type = type_var::DEC;

						if (enfant_droite->chaine() == "r") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees;
						}
						else if (enfant_droite->chaine() == "v") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 1;
						}
						else if (enfant_droite->chaine() == "b") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 2;
						}
						else if (enfant_droite->chaine() == "a") {
							b->pointeur_donnees = enfant_gauche->pointeur_donnees + 3;
						}
						else {
							throw std::runtime_error("membre inconnu");
						}

						break;
					}
				}
			}
			else {
				throw std::runtime_error("accès impossible");
			}

			break;
		}
		case type_noeud::SI:
		{
			auto iter_enfant = b->enfants.debut();
			auto enfant1 = *iter_enfant++;
			auto enfant2 = *iter_enfant++;
			auto enfant3 = (b->enfants.taille() == 3) ? *iter_enfant++ : nullptr;

			/* génère le code de l'expression */
			genere_code(enfant1, contexte_generation, compileuse, expr_gauche);

			/* instructions : inst_branche ptr_comp bloc_si_vrai bloc_si_faux */
			auto &instructions = compileuse.instructions();

			compileuse.ajoute_instructions(code_inst::IN_BRANCHE_CONDITION);
			compileuse.ajoute_instructions(enfant1->pointeur_donnees);
			auto decalage_branche_si_vrai = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);
			auto decalage_branche_si_faux = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);

			/* si la condition est vraie, le bloc est normalement juste après, donc
			 * cette instruction n'est pas nécessaire */
			instructions.stocke(decalage_branche_si_vrai, static_cast<int>(instructions.taille()));

			/* génère le code du bloc 'si' */
			genere_code(enfant2, contexte_generation, compileuse, expr_gauche);

			/* génère le code du bloc 'sinon' */
			if (enfant3 != nullptr) {
				/* ajout d'une branche pour sauter le code si la condition était vraie */
				compileuse.ajoute_instructions(code_inst::IN_BRANCHE);
				auto decalage_branche = static_cast<int>(instructions.taille());
				compileuse.ajoute_instructions(0);

				/* prend en compte la branche ajoutée */
				instructions.stocke(decalage_branche_si_faux, static_cast<int>(instructions.taille()));

				genere_code(enfant3, contexte_generation, compileuse, expr_gauche);

				instructions.stocke(decalage_branche, static_cast<int>(instructions.taille()));
			}
			else {
				instructions.stocke(decalage_branche_si_faux, static_cast<int>(instructions.taille()));
			}

			break;
		}
		case type_noeud::BLOC:
		{
			for (auto enfant : b->enfants) {
				genere_code(enfant, contexte_generation, compileuse, expr_gauche);
			}

			break;
		}
		case type_noeud::POUR:
		{
			/* 3 enfants : var plage bloc */
			auto iter_enfant = b->enfants.debut();
			auto enfant1 = *iter_enfant++;
			auto enfant2 = *iter_enfant++;
			auto enfant3 = *iter_enfant++;

			/* génère le code de l'expresion pour avoir le type de la variable */
			genere_code(enfant2, contexte_generation, compileuse, expr_gauche);
			auto variable_debut = enfant2->enfants.front()->pointeur_donnees;
			auto variable_fin = enfant2->enfants.back()->pointeur_donnees;

			/* alloue la variable, À FAIRE : portée variables */
			enfant1->donnees_type = enfant2->enfants.front()->donnees_type;
			genere_code(enfant1, contexte_generation, compileuse, true);

			compileuse.ajoute_instructions(code_inst::ASSIGNATION);
			compileuse.ajoute_instructions(enfant1->donnees_type);
			compileuse.ajoute_instructions(variable_debut);
			compileuse.ajoute_instructions(enfant1->pointeur_donnees);

			/* entrée de la boucle */
			auto &instructions = compileuse.instructions();
			auto ptr_debut_boucle = instructions.taille();

			/* test de la variable */

			/* 1. compare avec fin */
			compileuse.ajoute_instructions(code_inst::FN_INEGALITE);
			compileuse.ajoute_instructions(enfant1->donnees_type);
			compileuse.ajoute_instructions(enfant1->pointeur_donnees);
			compileuse.ajoute_instructions(variable_fin);
			auto ptr_comp = compileuse.donnees().loge_donnees(taille_type(enfant1->donnees_type));
			compileuse.ajoute_instructions(ptr_comp);

			/* 2. branche si vrai */
			compileuse.ajoute_instructions(code_inst::IN_BRANCHE_CONDITION);
			compileuse.ajoute_instructions(ptr_comp);
			auto decalage_branche_si_vrai = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);
			auto decalage_branche_si_faux = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);

			/* si la condition est vraie, le bloc est normalement juste après, donc
			 * cette instruction n'est pas nécessaire */
			instructions.stocke(decalage_branche_si_vrai, static_cast<int>(instructions.taille()));

			/* données pour cette boucle */
			auto db = donnees_boucles{};
			contexte_generation.boucles.empile(&db);

			/* génère le code du bloc */
			genere_code(enfant3, contexte_generation, compileuse, expr_gauche);

			/* on « continue » après le bloc, mais avant d'incrémenter la
			 * variable bouclée */
			auto ptr_inst_continue = static_cast<int>(instructions.taille());

			/* incrémente la variable */
			compileuse.ajoute_instructions(code_inst::IN_INCREMENTE);
			compileuse.ajoute_instructions(enfant1->donnees_type);
			compileuse.ajoute_instructions(enfant1->pointeur_donnees);

			/* branche vers l'entrée de la boucle */
			compileuse.ajoute_instructions(code_inst::IN_BRANCHE);
			compileuse.ajoute_instructions(ptr_debut_boucle);

			auto ptr_fin_boucle = static_cast<int>(instructions.taille());

			instructions.stocke(decalage_branche_si_faux, ptr_fin_boucle);

			contexte_generation.boucles.depile();

			for (auto ptr_arrete : db.arretes) {
				instructions.stocke(ptr_arrete, ptr_fin_boucle);
			}

			for (auto ptr_continue : db.continues) {
				instructions.stocke(ptr_continue, ptr_inst_continue);
			}

			break;
		}
		case type_noeud::PLAGE:
		{
			for (auto enfant : b->enfants) {
				genere_code(enfant, contexte_generation, compileuse, expr_gauche);
			}

			break;
		}
		case type_noeud::ARRETE:
		{
			auto &instructions = compileuse.instructions();

			compileuse.ajoute_instructions(code_inst::IN_BRANCHE);
			auto decalage_inst = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);

			auto donnees_boucle = contexte_generation.boucles.haut();
			donnees_boucle->arretes.pousse(decalage_inst);
			break;
		}
		case type_noeud::CONTINUE:
		{
			auto &instructions = compileuse.instructions();
			compileuse.ajoute_instructions(code_inst::IN_BRANCHE);
			auto decalage_inst = static_cast<int>(instructions.taille());
			compileuse.ajoute_instructions(0);

			auto donnees_boucle = contexte_generation.boucles.haut();
			donnees_boucle->continues.pousse(decalage_inst);
			break;
		}
		case type_noeud::RETOURNE:
		{
			compileuse.ajoute_instructions(code_inst::TERMINE);
			break;
		}
	}

	return b->pointeur_donnees;
}

} /* namespace noeud */
} /* namespace lcc */
