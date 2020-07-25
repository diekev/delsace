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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "impression.hh"

#include "biblinternes/outils/numerique.hh"

#include "compilation/identifiant.hh"
#include "compilation/typage.hh"

#include "instructions.hh"

static void imprime_atome(Atome const *atome, std::ostream &os)
{
	if (atome->genre_atome == Atome::Genre::CONSTANTE) {
		auto atome_const = static_cast<AtomeConstante const *>(atome);

		switch (atome_const->genre) {
			case AtomeConstante::Genre::GLOBALE:
			{
				break;
			}
			case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
			{
				auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
				os << "  transtype ";
				imprime_atome(transtype_const->valeur, os);
				os << " vers " << chaine_type(transtype_const->type) << '\n';
				break;
			}
			case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
			{
				break;
			}
			case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
			{
				break;
			}
			case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
			{
				break;
			}
			case AtomeConstante::Genre::VALEUR:
			{
				auto valeur_constante = static_cast<AtomeValeurConstante const *>(atome);

				switch (valeur_constante->valeur.genre) {
					case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
					{
						os << valeur_constante->valeur.valeur_booleenne;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::TYPE:
					{
						os << valeur_constante->valeur.type->index_dans_table_types;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::ENTIERE:
					{
						os << valeur_constante->valeur.valeur_entiere;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::REELLE:
					{
						os << valeur_constante->valeur.valeur_reelle;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::NULLE:
					{
						os << "nul";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::CARACTERE:
					{
						os << valeur_constante->valeur.valeur_reelle;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
					{
						os << "indéfinie";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
					{
						auto type = static_cast<TypeCompose *>(atome->type);
						auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

						auto virgule = "{ ";

						auto index_membre = 0;

						POUR (type->membres) {
							os << virgule;
							os << it.nom << " = ";
							imprime_atome(tableau_valeur[index_membre], os);
							index_membre += 1;
							virgule = ", ";
						}

						os << " }";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
					{
						os << "À FAIRE(ri) : tableau fixe";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
					{
						os << "À FAIRE(ri) : tableau données constantes";
						break;
					}
				}
			}
		}
	}
	else if (atome->genre_atome == Atome::Genre::FONCTION) {
		auto atome_fonction = static_cast<AtomeFonction const *>(atome);
		os << atome_fonction->nom;
	}
	else {
		auto inst_valeur = static_cast<Instruction const *>(atome);
		os << "%" << inst_valeur->numero;
	}
}

void imprime_instruction(Instruction const *inst, std::ostream &os)
{
	switch (inst->genre) {
		case Instruction::Genre::INVALIDE:
		{
			os << "  invalide\n";
			break;
		}
		case Instruction::Genre::ENREGISTRE_LOCALES:
		{
			os << " enregistre locales\n";
			break;
		}
		case Instruction::Genre::RESTAURE_LOCALES:
		{
			os << " restaure locales\n";
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto type_pointeur = static_cast<TypePointeur *>(inst->type);
			os << "  alloue " << chaine_type(type_pointeur->type_pointe) << ' ';

			if (inst->ident != nullptr) {
				os << inst->ident->nom << '\n';
			}
			else {
				os << "val" << inst->numero << '\n';
			}

			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = static_cast<InstructionAppel const *>(inst);
			os << "  appel " << chaine_type(inst_appel->type) << ' ';
			imprime_atome(inst_appel->appele, os);

			auto virgule = "(";

			POUR (inst_appel->args) {
				os << virgule;
				imprime_atome(it, os);
				virgule = ", ";
			}

			if (inst_appel->args.taille == 0) {
				os << virgule;
			}

			os << ")\n";

			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto inst_branche = static_cast<InstructionBranche const *>(inst);
			os << "  branche %" << inst_branche->label->numero << "\n";
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
			os << "  si ";
			imprime_atome(inst_branche->condition, os);
			os << " alors %" << inst_branche->label_si_vrai->numero
			   << " sinon %" << inst_branche->label_si_faux->numero
			   << '\n';
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto inst_charge = static_cast<InstructionChargeMem const *>(inst);
			auto charge = inst_charge->chargee;

			os << "  charge " << chaine_type(inst->type);

			if (charge->genre_atome == Atome::Genre::GLOBALE) {
				os << " @globale" << charge;
			}
			else if (charge->genre_atome == Atome::Genre::CONSTANTE) {
				os << " @constante" << charge;
			}
			else {
				auto inst_chargee = static_cast<Instruction const *>(charge);
				os << " %" << inst_chargee->numero;
			}

			os << '\n';
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
			auto ou = inst_stocke->ou;

			os << "  stocke " << chaine_type(ou->type);

			if (ou->genre_atome == Atome::Genre::GLOBALE) {
				os << " @globale" << ou;
			}
			else {
				auto inst_chargee = static_cast<Instruction const *>(ou);
				os << " %" << inst_chargee->numero;
			}

			os << ", " << chaine_type(inst_stocke->valeur->type) << ' ';
			imprime_atome(inst_stocke->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto inst_label = static_cast<InstructionLabel const *>(inst);
			os << "label " << inst_label->id << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_un->op)
			   << ' ' << chaine_type(inst_un->type);
			imprime_atome(inst_un->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_bin->op)
			   << ' ' << chaine_type(inst_bin->type) << ' ';
			imprime_atome(inst_bin->valeur_gauche, os);
			os << ", ";
			imprime_atome(inst_bin->valeur_droite, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto inst_retour = static_cast<InstructionRetour const *>(inst);
			os << "  retourne ";
			if (inst_retour->valeur != nullptr) {
				auto atome = inst_retour->valeur;
				os << chaine_type(atome->type);
				os << ' ';

				imprime_atome(atome, os);
			}
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
			os << "  index " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << ", ";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);
			os << "  membre " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << ", ";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
			os << "  transtype (" << static_cast<int>(inst_transtype->op) << ") ";
			imprime_atome(inst_transtype->valeur, os);
			os << " vers " << chaine_type(inst_transtype->type) << '\n';
			break;
		}
	}
}

void imprime_fonction(AtomeFonction const *atome_fonc, std::ostream &os, bool inclus_nombre_utilisations, bool surligne_inutilisees)
{
	os << "fonction " << atome_fonc->nom;

	auto virgule = "(";

	for (auto param : atome_fonc->params_entrees) {
		os << virgule;
		os << param->ident->nom << ' ';

		auto type_pointeur = static_cast<TypePointeur *>(param->type);
		os << chaine_type(type_pointeur->type_pointe);

		virgule = ", ";
	}

	if (atome_fonc->params_entrees.taille == 0) {
		os << virgule;
	}

	auto type_fonction = static_cast<TypeFonction *>(atome_fonc->type);

	virgule = ") -> ";

	POUR (type_fonction->types_sorties) {
		os << virgule;
		os << chaine_type(it);
		virgule = ", ";
	}

	os << '\n';

	auto numero_instruction = static_cast<int>(atome_fonc->params_entrees.taille);
	auto max_utilisations = 0;

	POUR (atome_fonc->instructions) {
		it->numero = numero_instruction++;
		max_utilisations = std::max(max_utilisations, it->nombre_utilisations);
	}

	using dls::num::nombre_de_chiffres;

	POUR (atome_fonc->instructions) {
		if (surligne_inutilisees && it->nombre_utilisations == 0) {
			std::cerr << "\033[0;31m";
		}

		if (inclus_nombre_utilisations) {
			auto nombre_zero_avant_numero = nombre_de_chiffres(max_utilisations) - nombre_de_chiffres(it->nombre_utilisations);
			os << '(';

			for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
				os << ' ';
			}

			os << it->nombre_utilisations << ") ";
		}

		auto nombre_zero_avant_numero = nombre_de_chiffres(atome_fonc->instructions.taille) - nombre_de_chiffres(it->numero);

		for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
			os << ' ';
		}

		os << "%" << it->numero << ' ';

		imprime_instruction(it, os);

		if (surligne_inutilisees && it->nombre_utilisations == 0) {
			std::cerr << "\033[0m";
		}
	}

	os << '\n';
}
